/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Hewlett-Packard Development Company, L.P.
 *
 */

/*
 * PLMc daemon program
 *
 * plmcd is a small daemon program that responds to PLMc commands
 * sent by a PLM server (or by the plmc_tcp_talker test program.)
 *
 * Unless there is a networking issue, plmcd will always respond to
 * a given command with one of the following responses:
 *
 *   "PLMC_ACK_MSG_RECD"
 *   "PLMC_ACK_SUCCESS"
 *   "PLMC_ACK_FAILURE"
 *   "PLMC_ACK_TIMEOUT"
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <arpa/inet.h>

#include "plmc.h"
#include "plmc_cmds.h"

/* We need these utility functions. */
extern int plmc_read_config(char *plmc_config_file, PLMC_config_data *config);
extern int plmc_cmd_string_to_enum(char *cmd);
extern void plmc_print_config(PLMC_config_data *config);

void plmc_termination_handler(int signum);
static int plmc_send_udp_msg(char *msg);
int sysexec(char *cmd, char *opt);
static void usage(char *prog);
int setsig(int sig, void (*hand)(int), int flags, sigset_t *mask);
int nbconnect(struct sockaddr_in *sin);
int process_cmd(char *msg);

/* A variable to hold the PLMc config file data - used by parent */
/* and child threads.                                            */
static PLMC_config_data config;

/* A global socket variable */
int sockd = 0;

/* A simple structure to provide mutex locking between the parent  */
/* and child threads.  It also provides a flag telling the parent  */
/* thread when the child thread is done, and provides the child    */
/* thread return value.                                            */
typedef struct plmc_thread_data {
	pthread_mutex_t   td_lock;
	pthread_t         td_id;
	int               plmc_cmd_i;
	int               child_done;
	char              *child_retval;
} plmc_tdata;

int sysexec(char *cmd, char *opt)
{
	char buf[PLMC_MAX_TCP_DATA_LEN];
	int ret;
	if((strlen(cmd) <= 0)){
		syslog(LOG_INFO, "Trying to run empty command");
		return 0;
	}
	if (opt)
		sprintf(buf, "%s %s>/dev/null 2>&1", cmd, opt);
	else
		sprintf(buf, "%s >/dev/null 2>&1", cmd);

	syslog(LOG_INFO, "Running, system('%s')", buf);
	ret = system(buf);
	if (ret)
		syslog(LOG_ERR, "Error, %d from system('%s')", ret, buf);

	return ret;
}

/*
 * plmc_child_thread()
 *
 * A child thread that will perform the long-running PLMc commands, and
 * copy the result to the mutex locking structure.
 *
 * Inputs:
 *   arg : A pointer to the thread data structure.
 *
 */
void *plmc_child_thread(void *arg)
{
	plmc_tdata *td = (plmc_tdata *) arg;
	int i, retval = 0, tmp_retval = 0;

	if (td == NULL) {
		syslog(LOG_ERR, "Error reading thread data");
		exit(PLMC_EXIT_FAILURE);
	}

	/* Switch statement here to process the PLMc command. */
	switch (td->plmc_cmd_i) {
		case PLMC_SA_PLM_ADMIN_UNLOCK_CMD:
			/* Unlock the OS - start OpenSAF-related services, then start OpenSAF. */
			for(i=0; i<config.num_services; i++) {
				tmp_retval = sysexec(config.services[i], "start");
				if (tmp_retval) retval = tmp_retval;
			}
			tmp_retval = sysexec(config.osaf, "start");
			if (tmp_retval) retval = tmp_retval;
			break;

		case PLMC_SA_PLM_ADMIN_LOCK_CMD:
			/* Lock the OS - stop OpenSAF, then stop OpenSAF-related services. */
			tmp_retval = sysexec(config.osaf, "stop");
			if (tmp_retval) retval = tmp_retval;
			for(i = config.num_services -1; i >= 0;  i--) {
				tmp_retval = sysexec(config.services[i], "stop");
				if (tmp_retval) retval = tmp_retval;
			}
			break;

		case PLMC_OSAF_START_CMD:
			/* Start OpenSAF. */
			retval = sysexec(config.osaf, "start");
			break;

		case PLMC_OSAF_STOP_CMD:
			/* Stop OpenSAF. */
			retval = sysexec(config.osaf, "stop");
			break;

		case PLMC_OSAF_SERVICES_START_CMD:
			/* Start OpenSAF-related services. */
			for(i=0; i<config.num_services; i++) {
				tmp_retval = sysexec(config.services[i], "start");
				if (tmp_retval) retval = tmp_retval;
			}
			break;

		case PLMC_OSAF_SERVICES_STOP_CMD:
			/* Stop OpenSAF-related services. */
			for(i = config.num_services -1; i >= 0;  i--) {
				tmp_retval = sysexec(config.services[i], "stop");
				if (tmp_retval) retval = tmp_retval;
			}
			break;
	}

	/* Begin critical section. */
	if (pthread_mutex_lock(&(td->td_lock))) {
		exit(PLMC_EXIT_FAILURE);
	}

	/* Copy result of command into the thread_data structure.              */
	if (retval == 0) 
		td->child_retval = PLMC_ACK_SUCCESS;
	else
		td->child_retval = PLMC_ACK_FAILURE;
	td->child_done = 1;

	if (pthread_mutex_unlock(&(td->td_lock))) {
		exit(PLMC_EXIT_FAILURE);
	}
	/* End critical section. */

	pthread_exit((void *)NULL);
}

/*
 * plmc_start_child_thread()
 *
 * A function to start the PLMc child thread on a long-running PLMc command,
 * and to monitor the child thread so that it either completes, fails, or
 * times out.
 *
 * Inputs:
 *   plmc_cmd_i : The numeric value of the PLMc command to execute.
 *
 * Returns: "PLMC_ACK_SUCCESS" for a command that successfully completes.
 * Returns: "PLMC_ACK_FAILURE" for a command that fails.
 * Returns: "PLMC_ACK_TIMEOUT" for a command that times out.
 *
 */
char* plmc_start_child_thread(int plmc_cmd_i) {
	plmc_tdata th_data;
	int child_not_done = 1;
	void *join_res;
	time_t start_time, current_time;

	th_data.plmc_cmd_i = plmc_cmd_i;
	th_data.child_done = 0;
	th_data.child_retval = NULL; 

	/* Get the starting time. */
	time(&start_time);

	/* Initialize the mutex. */
	pthread_mutex_init(&(th_data.td_lock), NULL);

	/* Create the child thread. */
	if (pthread_create(&(th_data.td_id), NULL, plmc_child_thread, &th_data)){
		syslog(LOG_ERR, "Error pthread_create: %m");
		exit(PLMC_EXIT_FAILURE);
	}

	/* Main watchdog loop - checks periodically to see if child thread is done.  */
	/* If timeout is reached, then we cancel child thread and return value of    */
	/* PLMC_ACK_TIMEOUT.                                                         */
	while(child_not_done) {
		sleep(PLMC_CHILD_THREAD_WATCHDOG_PERIOD_SECS);

		/* See if we've run out of time. */
		time(&current_time);

		/* We've run out of time, cancel the child thread. */
		if ((current_time - start_time) > config.plmc_cmd_timeout_secs) {
			pthread_cancel(th_data.td_id);
			break;
		}

		/* Begin critical section. */
		if (pthread_mutex_trylock(&(th_data.td_lock)) == 0) {
			if (th_data.child_done)
				child_not_done = 0;
			if (pthread_mutex_unlock(&(th_data.td_lock))) {
				syslog(LOG_ERR, "Error pthread_mutex_unlock");
				exit(PLMC_EXIT_FAILURE);
			}
		}
		/* End critical section. */
	}

	pthread_join(th_data.td_id, &join_res);
	if (join_res == PTHREAD_CANCELED) {
		th_data.child_retval = PLMC_ACK_TIMEOUT;
	}

	/* Destroy the mutex. */
	pthread_mutex_destroy(&(th_data.td_lock));

	return(th_data.child_retval);
}

void sndack(int cmd, char *msg)
{
	char buf[PLMC_MAX_TCP_DATA_LEN];
	int ret;

	sprintf(buf, "plmc_d %s ack %s : %s", PLMC_cmd_name[cmd], config.ee_id, msg);
	ret = send(sockd, buf, strlen(buf), 0);
	if (ret == -1){
		syslog(LOG_ERR, "Error send: %m");
		exit(PLMC_EXIT_FAILURE);
	}
}

int process_cmd(char *msg)
{
	int plmc_cmd; 
	char *ret;

	syslog(LOG_INFO, "RECV command: %s", msg);

	plmc_cmd = plmc_cmd_string_to_enum(msg);
	if (plmc_cmd == -1) {
		syslog(LOG_ERR, "Error, could not decode command: %s", msg);
		return -1;
	}

	switch(plmc_cmd) {
		case PLMC_NOOP_CMD:
			/* Just send an acknowledgement. */
			sndack(plmc_cmd, PLMC_ACK_MSG_RECD);
			break;

		case PLMC_GET_ID_CMD:
			/* Send the ee_id in the acknowledgement. */
			sndack(plmc_cmd, config.ee_id);
			break;

		case PLMC_GET_PROTOCOL_VER_CMD:
			/* Send the msg_protocol_version in the acknowledgement. */
			sndack(plmc_cmd, config.msg_protocol_version);
			break;

		case PLMC_GET_OSINFO_CMD:
			/* Send the os_type in the acknowledgement. */
			sndack(plmc_cmd, config.os_type);
			break;

		case PLMC_SA_PLM_ADMIN_LOCK_INSTANTIATION_CMD:
			/* Send PLMC_ACK_MSG_RECD in the acknowledgement - before executing command. */
			sndack(plmc_cmd, PLMC_ACK_MSG_RECD);

			/* Shutdown the OS. */
			sysexec(config.os_shutdown_cmd, NULL);
			break;

		case PLMC_SA_PLM_ADMIN_RESTART_CMD:
			/* Send PLMC_ACK_MSG_RECD in the acknowledgement - before executing command. */
			sndack(plmc_cmd, PLMC_ACK_MSG_RECD);

			/* Reboot the OS */
			sysexec(config.os_reboot_cmd, NULL);
			break;

		case PLMC_PLMCD_RESTART_CMD:
			/* Send PLMC_ACK_MSG_RECD in the acknowledgement - before executing command. */
			sndack(plmc_cmd, PLMC_ACK_MSG_RECD);

			/* Close the socket to force reconnect/restart */
			close(sockd);
			break;

		case PLMC_SA_PLM_ADMIN_UNLOCK_CMD:
		case PLMC_SA_PLM_ADMIN_LOCK_CMD:
		case PLMC_OSAF_START_CMD:
		case PLMC_OSAF_STOP_CMD:
		case PLMC_OSAF_SERVICES_START_CMD:
		case PLMC_OSAF_SERVICES_STOP_CMD:
			/* These are long-running commands - so create a child thread to handle this case. */
			ret = plmc_start_child_thread(plmc_cmd);
			/* Send the acknowledgement. */
			sndack(plmc_cmd, ret);
			break;
	}

	return 0;
}

/*
 * By default TCP timeouts can be very long. This can lead to blocking for a
 * very long time waiting on connect(). This function sets the socket to 
 * non-blocking mode for the connect and returns the socket to blocking mode
 * once the connect has been established. 
 *
 * \param socket file descriptor
 * \param socket address structure
 *
 * \return < 0 on error, 0 on success
 */ 
int nbconnect(struct sockaddr_in *sin)
{
	struct pollfd wset;
	socklen_t len; 
	int flags, ret, opt;

	/* Set the socket fd to non-blocking mode */
	if( (flags = fcntl(sockd, F_GETFL, NULL)) < 0) { 
		syslog(LOG_ERR, "Error fcntl(..., F_GETFL):  %m");
		exit(PLMC_EXIT_FAILURE);
	} 
	flags |= O_NONBLOCK; 
	if( fcntl(sockd, F_SETFL, flags) < 0) { 
		syslog(LOG_ERR, "Error fcntl(..., F_SETFL):  %m");
		exit(PLMC_EXIT_FAILURE);
	} 

	/* connect with timeout */
	ret = connect(sockd, (struct sockaddr *)sin, sizeof(struct sockaddr_in)); 
	if (ret < 0) { 
		if (errno == EINPROGRESS) { 
			/* poll the fd until we get a connection, timeout, or error  */
			while(1) { 
				wset.fd = sockd;
				wset.events = POLLOUT;

				ret = poll(&wset, 1, PLMC_TCP_TIMEOUT_SECS * 1000);
				if (ret < 0 && errno != EINTR) { 
					syslog(LOG_ERR, "Error poll:  %m");
					return -1;

				} else if (ret > 0) { 
					// Socket polled for write
					len = sizeof(int); 
					if (getsockopt(sockd, SOL_SOCKET, SO_ERROR, (void*)(&opt), &len) < 0) { 
						syslog(LOG_ERR, "Error getsockopt:  %m");
						return -1;
					} 
					// Check the value returned... 
					if (opt) { 
						syslog(LOG_ERR, "Error getsockopt:  %m");
						return -1;
					} 
					break; 
				} else { /* Timeout */
					syslog(LOG_ERR, "Timeout in connect():  %m");
					return -2;
				} 
			} 
		} else {  /* Real error returned from connect */
			syslog(LOG_ERR, "Error connect:  %m");
			return -1;
		} 
	} 


	/* Connection has been established switch back to blocking mode */
	flags &= (~O_NONBLOCK); 
	if( fcntl(sockd, F_SETFL, flags) < 0) { 
		syslog(LOG_ERR, "Error fcntl:  %m");
		exit(PLMC_EXIT_FAILURE);
	} 

	return 0;
}

/** SIGTERM handler terminate the PLMc daemon.
 *
 * Inputs:
 *   signum : The signal that was received.
 */
void plmc_termination_handler(int signum)
{
	/* Send EE_TERMINATING message, replace plmc_boot */
	plmc_send_udp_msg(PLMC_D_STOP_MSG);
	if (sockd != 0)
		close(sockd);
	unlink(PLMCD_PID);
	exit(PLMC_EXIT_SUCCESS);
}

/**
 * Simple helper function to register signal handlers
 *
 * \param sig signal number
 * \param handler function to register
 * \param flags 
 * \param mask signal mask
 * 
 * \return results of thh sigaction call
 */
int setsig(int sig, void (*handler)(int), int flags, sigset_t *mask)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));

	sa.sa_handler = handler;
	if (mask) 
		sa.sa_mask = *mask;
	sa.sa_flags = flags;
	return sigaction(sig, &sa, NULL); 
} 

/* 
 * Send a UDP datagram to both controllers
 * \param msg message to send
 * \param * config 
 *
 * \return 0 on success < 0 on error
 */
static int plmc_send_udp_msg(char *msg)
{
	int udpsockfd;
	struct sockaddr_in sin;
	char send_msg[PLMC_MAX_TCP_DATA_LEN];

	/* create an IPv4 UDP socket */
	udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpsockfd == -1)
	{
		syslog(LOG_ERR, "Error socket:  %m");
		return -1;
	}

	/* server address */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (!(inet_aton(config.controller_1_ip, &sin.sin_addr))){
		syslog(LOG_ERR, "Invalid Controller 1 Address:  %s",
				config.controller_1_ip);
		return -2;
	}
	sin.sin_port = htons(atoi(config.udp_broadcast_port));

	/* Send UDP datagram that the EE is instantiated. */
	sprintf(send_msg, "ee_id %s : %s : %s", config.ee_id, msg,
			config.os_type);

	/* Send UDP datagram to controller_1_ip. */
	sendto(udpsockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&sin, sizeof(sin));

	/* Now send same UDP datagram to controller_2_ip. */
	sin.sin_addr.s_addr=inet_addr(config.controller_2_ip);
	if (!(inet_aton(config.controller_2_ip, &sin.sin_addr))){
		syslog(LOG_ERR, "Invalid Controller 2 Address:  %s",
				config.controller_2_ip);
		return -2;
	}
	sendto(udpsockfd, send_msg, strlen(send_msg), 0, (struct sockaddr *)&sin, sizeof(sin));

	syslog(LOG_INFO, "%s", send_msg);
	close(udpsockfd);

	return 0;
}

/*
 * Write the pid file
 */
int writepid(char *pidfile)
{
	FILE *file;
	int fd;
	int pid;

	/* open the file and associate a stream with it */
	if ( ((fd = open(pidfile, O_RDWR|O_CREAT, 0644)) == -1)
			|| ((file = fdopen(fd, "r+")) == NULL) ) { 
		syslog(LOG_ERR, "Error, open %s: %m", pidfile);
		return 0;
	}

	/* Lock the file */
	if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
		syslog(LOG_ERR, "Error, flock %s: %m", pidfile);
		fclose(file);
		return 0;
	}

	pid = getpid();
	if (!fprintf(file,"%d\n", pid)) {
		syslog(LOG_ERR, "Error, fprintf %s: %m", pidfile);
		fclose(file);
		return 0;
	}
	fflush(file);

	if (flock(fd, LOCK_UN) == -1) {
		syslog(LOG_ERR, "Error, flock %s: %m", pidfile);
		fclose(file);
		return 0;
	}
	fclose(file);

	return pid;
}

/*
 * Check to see if we are already runing.
 *
 * \param pid file
 */
int chkpid(char *pidfile)
{
	FILE *file;
	int pid;

	if (!(file=fopen(pidfile,"r")))
		return 0;

	if(fscanf(file, "%d", &pid) == EOF){
		syslog(LOG_ERR, "Error, reading %s: %m", pidfile);
		fclose(file);
		return 0;
	}

	fclose(file);

	/* Is it my pid file? */
	if (pid == getpid())
		return 0;

	/* Test to see if the daemon is still running by issuing a fake kill */
	if (kill(pid, 0) && errno == ESRCH)
		return 0;

	return pid;
}

/* 
 * Display the program usage and exit
 * \param prog argv[0] name
 */
static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [-c plmc.conf] [-s || -x]\n\n", prog);
	fprintf(stderr, "options are:\n"
			"\t-c\t\tfull path to plmc.conf\n"
			"\t-s\t\tOS is starting up, send EE_INSTANTIATING message\n"
			"\t-x\t\tOS is shutting down, send EE_TERMINATED message\n"
	       );
	exit(1);
}

int main(int argc, char** argv)
{
	char *plmc_config_file = NULL, msg[PLMC_MAX_TCP_DATA_LEN];
	struct stat statbuf;
	int ii, ret; 
	int tcp_keepidle_time, tcp_keepalive_intvl, tcp_keepalive_probes;
	int so_keepalive;
	socklen_t optlen;
	int controller = 1, time = 1, pid=0; 
	struct sockaddr_in sin;
	unsigned int option= 0x0;

	openlog(basename(argv[0]), LOG_PID|LOG_CONS, LOG_DAEMON);

	if (geteuid()){
		fprintf(stderr, "Must be run as root or have the SUID attribute set\n");
		syslog(LOG_ERR, "Must be run as root or have the SUID attribute set");
		exit(2);
	}

	while ((ii = getopt(argc, argv, "c:sxph?")) != -1){
		switch (ii) {
			case 'c':
				if (strlen(optarg) > PATH_MAX) {
					fprintf(stderr, "Invalid config file: %s", optarg);
					syslog(LOG_ERR, "Invalid config file: %s", optarg);
					exit(3);
				}
				if ((plmc_config_file=(char *)malloc(strlen(optarg) + 1)) == NULL){
					fprintf(stderr, "Can not allocate memory\n");
					syslog(LOG_ERR, "Can not allocate memory");
					exit(4);
				}
				strncpy(plmc_config_file, optarg, strlen(optarg) + 1);
				break;
			case 's':
				option = option | 1;
				break;
			case 'x':
				option = option | 2;
				break;
			case 'p':
				option = option | 4;
				break;
			case 'h':
			case '?':
			default:
				usage(basename(argv[0]));
		}
	}

	/* Use default config if user did not specify one */
	if (!plmc_config_file){
		plmc_config_file = PLMC_CONFIG_FILE_LOC;
		if (stat(plmc_config_file, &statbuf) == -1) {
			fprintf(stderr, "Invalid config file: %s\n", plmc_config_file);
			syslog(LOG_ERR, "Invalid config file: %s", plmc_config_file);
			exit(5);
		}
	}

	/* Read the plmcd.conf config file. */
	if ((ret = plmc_read_config(plmc_config_file, &config))) {
		fprintf(stderr, "Error, reading %s. See syslog for more "
					"information\n", plmc_config_file);
		syslog(LOG_ERR, "Error reading %s.  errno = %d", 
							plmc_config_file, ret);
		exit(6);
	}

	switch (option) {
		/* No options, break*/
		case 0:
			break;
		case 1:
			/* Send EE_INSTANTIATING message, replace plmc_boot */
			plmc_send_udp_msg(PLMC_BOOT_START_MSG);
			exit(PLMC_EXIT_SUCCESS);
			break;
		case 2:
			/* Send EE_TERMINATED message, replace plmc_boot */
			plmc_send_udp_msg(PLMC_BOOT_STOP_MSG);
			exit(PLMC_EXIT_SUCCESS);
			break;
		case 4:
			/* Print the parsed config */
			plmc_print_config(&config);
			exit(PLMC_EXIT_SUCCESS);
			break;
		default:
			usage(basename(argv[0]));
	}
	
	/* Check the lock file to make sure we are not already runing */
	if ((pid=chkpid(PLMCD_PID))){
		fprintf(stderr, "plmcd already running? %d\n", pid);
		syslog(LOG_ERR, "plmcd already running? %d", pid);
		exit(7);
	}



	if (daemon(0, 0) < 0 ){
		fprintf(stderr, "Error, daemon:  %s", strerror(errno));
		syslog(LOG_ERR, "Error, daemon:  %m");
		exit(PLMC_EXIT_FAILURE);
	}

	umask(027);

	/* write a PID file */
	/* Check the lock file to make sure we are not already runing */
	if (!writepid(PLMCD_PID)){
		syslog(LOG_ERR, "Error, can not create pid file %s", PLMCD_PID);
		exit(PLMC_EXIT_FAILURE);
	}


	/* Set up termination signal handler here. */
	setsig(SIGTERM, plmc_termination_handler, 0, NULL);

	/* Send UDP datagram that the EE has instantiated.  */
	plmc_send_udp_msg(PLMC_D_START_MSG);


	/* Attempts to establish a connection to a PLM server. If the connection
	 * to controller 1 fails or times out try to connect to conrtoller 2. If
	 * that fails or times out sleep then try again. Loop until a
	 * connection is established. */

	/* Start with setting the tcp keepalive stuff */
	so_keepalive = config.so_keepalive;
	tcp_keepidle_time = config.tcp_keepidle_time;
	tcp_keepalive_intvl = config.tcp_keepalive_intvl;
	tcp_keepalive_probes =  config.tcp_keepalive_probes;

	while (1) {
		/* create an IPv4 UDP socket */
		sockd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockd == -1) {
			syslog(LOG_ERR, "Error  socket:  %m");
			exit(PLMC_EXIT_FAILURE);
		}
	
		if (so_keepalive == 1) {
			/* Set SO_KEEPALIVE */
			optlen = sizeof(so_keepalive);
			if(setsockopt(sockd, SOL_SOCKET, SO_KEEPALIVE, 
						&so_keepalive, optlen) < 0){
				syslog(LOG_ERR,"setsockopt SO_KEEPALIVE "
					"failed error:%s",strerror(errno));
				perror("setsockopt()");
				close(sockd);
				exit(PLMC_EXIT_FAILURE);
			}

			/* Set TCP_KEEPIDLE */
			optlen = sizeof(tcp_keepidle_time);
			if(setsockopt(sockd, SOL_TCP, TCP_KEEPIDLE, 
					&tcp_keepidle_time, optlen) < 0){
				syslog(LOG_ERR,"setsockopt TCP_KEEPIDLE "
					"failed " "error:%s",strerror(errno));
				perror("setsockopt()");
				close(sockd);
				exit(PLMC_EXIT_FAILURE);
			}
			
			/* Set TCP_KEEPINTVL */
			optlen = sizeof(tcp_keepalive_intvl);
			if(setsockopt(sockd, SOL_TCP, TCP_KEEPINTVL, 
					&tcp_keepalive_intvl, optlen) < 0){
				syslog(LOG_ERR,"setsockopt TCP_KEEPINTVL failed"
						" error:%s",strerror(errno));
				perror("setsockopt()");
				close(sockd);
				exit(PLMC_EXIT_FAILURE);
			}

			/* Set TCP_KEEPCNT */
			optlen = sizeof(tcp_keepalive_probes);
			if(setsockopt(sockd, SOL_TCP, TCP_KEEPCNT, 
					&tcp_keepalive_probes, optlen) < 0){
				syslog(LOG_ERR,"setsockopt TCP_KEEPCNT  failed"
						" error:%s",strerror(errno));
				perror("setsockopt()");
				close(sockd);
				exit(PLMC_EXIT_FAILURE);
			}
		}

		if ((setsockopt(sockd, SOL_SOCKET, SO_REUSEADDR, (void *)&ret, sizeof(ret)) == -1)) {
			syslog(LOG_ERR, "Error setsockpot:  %m");
			exit(PLMC_EXIT_FAILURE);
		}


		/* server address */
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(config.tcp_plms_listening_port));

		if (controller % 2){
			syslog(LOG_INFO, "Attempting to connect to controller 1");
			if (!(inet_aton(config.controller_1_ip, &sin.sin_addr))){
				syslog(LOG_ERR, "Invalid Controller 1 Address:  %s",
						config.controller_1_ip);
				exit(PLMC_EXIT_FAILURE);
			}
		} else {
			syslog(LOG_INFO, "Attempting to connect to controller 2");
			if (!(inet_aton(config.controller_2_ip, &sin.sin_addr))){
				syslog(LOG_ERR, "Invalid Controller 2 Address:  %s",
						config.controller_2_ip);
				exit(PLMC_EXIT_FAILURE);
			}
		}

		ret = nbconnect(&sin);
		if (ret == 0){
			syslog(LOG_ERR, "Connected to controller %d", controller % 2 ? 1 : 2);
			time = 1;
			while (1){
				ret=recv(sockd, msg, PLMC_MAX_TCP_DATA_LEN, 0);

				if (ret < 1) {
					syslog(LOG_ERR, "Empty message received");
					break;
				}

				msg[ret] = '\0';
				process_cmd(msg);
			}

		}

		syslog(LOG_ERR, "Pausing %d seconds before retrying", time);
		sleep(time);
		if (time <= 4)
			time *= 2;

		controller++;
		close(sockd);
	}

	return 0;
}
