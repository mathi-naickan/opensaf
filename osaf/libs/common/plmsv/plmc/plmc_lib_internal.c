/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <plmc_lib_internal.h>
#include <plmc_cmds.h>

extern int plmc_cmd_string_to_enum(char *cmd);
extern char * plmc_get_listening_ip_addr(char *plmc_config_file);

static thread_entry *HEAD = NULL;
static thread_entry *TAIL = NULL; 

static pthread_mutex_t td_list_lock;        /* List-wide lock */

FILE *plmc_lib_debug;

cb_functions callbacks;


/* returns pointer to thread entry on success, NULL otherwise */
thread_entry *find_thread_entry(char *ee_id) {
	thread_entry *curr_entry, *retval = NULL;

	/*** Begin Critical Section ***/
	if (pthread_mutex_lock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "error: plmc library could not lock thread "
									"list");
		return(NULL);
	}

	curr_entry = HEAD;
	while(curr_entry != NULL) {
		if (strcmp(curr_entry->thread_d.ee_id, ee_id) == 0) {
			retval = curr_entry;
			break;
		}
		else
			curr_entry = curr_entry->next;
	}

	if (pthread_mutex_unlock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "error: plmc library could not unlock thread "
									"list");
		return(NULL);
	}
	/*** End Critical Section ***/

	return(retval);
}

/* returns pointer to new thread entry on success, NULL otherwise */
thread_entry *create_thread_entry(char *ee_id, int sock) {
	thread_entry *new_entry = NULL;
	thread_data *tdata = NULL;

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "create_thread_entry was called with ee_id "
								"%s\n", ee_id);
	fflush(plmc_lib_debug);
	#endif
	/* 
	 * Check if the entry exists already. If so, check socket and switch
	 * kill to one if socket exist.
	 */
	new_entry = find_thread_entry(ee_id);

	if (new_entry != NULL) {
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "create_thread_entry: known entry "
							"ee_id=%s\n", ee_id);
		fflush(plmc_lib_debug);
		#endif
		tdata = &(new_entry->thread_d);
		/* Get lock, we are changing stuff here */
		if (pthread_mutex_lock(&tdata->td_lock) != 0) {
			syslog(LOG_ERR, "plmc_lib: create_thread_entry "
			"encountered an error getting a lock for a client");
		}
		if(tdata->socketfd != 0) {
			/* So there is still a socket open here
			* We'll need to set kill to 1 so the connection_mgr
			* can deal with him
			*/
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "create_thread_entry: "
				"socketfd was not zero. This guy needs to be "
				"killed. Setting kill to 1. Ee_id=%s\n", ee_id);
			fflush(plmc_lib_debug);
			#endif
			tdata->kill = 1;
			if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
				syslog(LOG_ERR, "plmc_lib: create_thread_entry "
				"encountered an error unlockng a client");
			}
			return NULL;
		} else {
			/* So the socket is zero. This means that the 
			* connection_mgr or client manager already took care 
			* of the socket and client_mgr has exited
			*/
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "create_thread_entry: socketfd "
				"was already zero. Resetting and reusing. "
							"Ee_id=%s\n", ee_id);
			fflush(plmc_lib_debug);
			#endif
			strncpy(tdata->ee_id, ee_id, PLMC_EE_ID_MAX_LENGTH);
			tdata->ee_id[PLMC_EE_ID_MAX_LENGTH - 1] = '\0';
			tdata->socketfd = sock;
			tdata->command[0] = '\0';
			tdata->callback = NULL;
			tdata->done = 1;
			tdata->td_id = 0;
			tdata->kill = 0;
		}
		if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
			syslog(LOG_ERR, "plmc_lib: create thread_entry "
				"encountered an error unlockng a client");
		}
		return new_entry;
	}

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "create_thread_entry: This is a new entry "
							"ee_id=%s\n", ee_id);
	fflush(plmc_lib_debug);
	#endif

	new_entry = calloc(1, sizeof(thread_entry));
	if(new_entry == NULL) {
		syslog(LOG_ERR, "Out of memory");
		return(NULL);
	} else {
		/* Initialize the contents of the thread data portion */
		pthread_mutex_init(&(new_entry->thread_d.td_lock), NULL);
		
		/* We need a copy of the ee_id 
		 *because we are running in our own thread 
		*/
		strncpy(new_entry->thread_d.ee_id, ee_id, PLMC_EE_ID_MAX_LENGTH);
		new_entry->thread_d.ee_id[PLMC_EE_ID_MAX_LENGTH - 1] = '\0';
		new_entry->thread_d.socketfd = sock;
		new_entry->thread_d.command[0] = '\0';
		new_entry->thread_d.callback = NULL;
		new_entry->thread_d.done = 1;
		new_entry->thread_d.td_id = 0;
		new_entry->thread_d.kill = 0;
	}

  /*** Begin Critical Section ***/
	if (pthread_mutex_lock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: create thread_entry encountered an "
						"error locking the list");
		free(new_entry);
		return(NULL);
	}

	/* Add new entry to the end of the linked list */
	if (HEAD == NULL) {
		/* This new entry is the only entry in the list */
		new_entry->previous = NULL;
		new_entry->next = NULL;
		HEAD = new_entry;
		TAIL = new_entry;
	} else {
		/* We know there is already an entry in the list, 
		* so update TAIL 
		*/
		TAIL->next = new_entry;
		new_entry->previous = TAIL;
		new_entry->next = NULL;
		TAIL = new_entry;
	}

	if (pthread_mutex_unlock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: create threa_entry encountered an "
						"error unlocking the list");
		free(new_entry);
		return(NULL);
	}
	/*** End Critical Section ***/
	return(new_entry);
}

/* returns 0 on success, 1 on error */
int delete_thread_entry(char *ee_id) {
	thread_entry *entry_to_delete;

	/* Make sure we have this entry in the list. */
	entry_to_delete = find_thread_entry(ee_id);
	if (entry_to_delete == NULL) {
		return(1);
	}

	/*** Begin Critical Section ***/
	if (pthread_mutex_lock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: delete_thread_entry encountered an "
						"error locking the list");
		return(1);
	}

	/* First, fix up the list - then free the memory for this entry */
	if (entry_to_delete->previous != NULL) {
		/* entry_to_delete is not the HEAD */
		entry_to_delete->previous->next = entry_to_delete->next;
		/* We need to update TAIL if entry_to_delete is also the 
		* last entry 
		*/
		if (entry_to_delete == TAIL) {
			TAIL = entry_to_delete->previous;
		} else {
			entry_to_delete->next->previous = 
						entry_to_delete->previous;
		}
	} else {
		/* entry_to_delete is the HEAD */
		HEAD = entry_to_delete->next;
		if (entry_to_delete->next != NULL) {
			/* entry_to_delete is not the last entry */
			entry_to_delete->next->previous = NULL;
		} else {
			/* update TAIL */
			TAIL = NULL;
		}
	}

	if (pthread_mutex_unlock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: delete_thread_entry encountered an "
						"error unlocking the list");
		return(1);
	}
	/*** End Critical Section ***/

	pthread_mutex_destroy(&(entry_to_delete->thread_d.td_lock));
	free(entry_to_delete);
	return(0);
}

/* delete all thread entries */
void delete_all_thread_entries() {
	thread_entry *curr_entry, *next_entry = NULL;
	int sockfd;
	char result[PLMC_MAX_TCP_DATA_LEN];
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "delete_all_thread_entries was called\n");
	fflush(plmc_lib_debug);
	#endif

	/*** Begin Critical Section ***/
	if (pthread_mutex_lock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: delete_thread_entry encountered an "
						"error locking the list");
		return;
	}

	curr_entry = HEAD;
	while(curr_entry != NULL) {
		sockfd = curr_entry->thread_d.socketfd;
		next_entry = curr_entry->next;
		pthread_mutex_destroy(&(curr_entry->thread_d.td_lock));
		if(curr_entry->thread_d.td_id != 0) {
			pthread_cancel(curr_entry->thread_d.td_id);
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "delete_all_thread_entries "
						"killed a client_mgr thread\n");
			fflush(plmc_lib_debug);
			#endif
		}
		if (sockfd != 0 && recv(sockfd, result, PLMC_MAX_TCP_DATA_LEN, 
					MSG_PEEK | MSG_DONTWAIT) != 0) {
			close(sockfd);
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "delete_all_thread_entries "
							"closed socket\n");
			fflush(plmc_lib_debug);
			#endif
		}

		free(curr_entry);
		curr_entry = next_entry;
	}
	TAIL = NULL;
	HEAD = NULL;

	if (pthread_mutex_unlock(&td_list_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: delete_thread_entry encountered an "
						"error unlocking the list");
		return;
	}
	/*** End Critical Section ***/
}

static char *PLMC_action_msg[] = {
	"Action not defined",
	"Closing client socket",
	"Exiting thread",
	"Destroying library",
	"Ignoring error"
};

static char *PLMC_error_msg[] = {
	"Misc error",
	"A system call failed",
	"Lost connection with client",
	"Message from client invalid",
	"Internal timeout while waiting for client",
	"Can't locate a valid configuration file",
	"Callback return non-zero",
	"Internal plmc library error"
};

/*********************************************************************
*	This function takes care of shutting down and closing a socket
*	
*	Parameters:
*	socketfd - The socket file descriptor
*
*	Returns:
*	This function return 0 if successful and a 1 if unsuccessful
*********************************************************************/

int close_socket(int sockfd) {
	if (close(sockfd) != 0) {
		syslog(LOG_ERR, "plmc_lib: encountered an error during a call "
								"to close()");
		return 1;
	}
	return 0;
}

/*********************************************************************
*	This function sends and error to PLMSv using the registered
*	callback function (see plmc_initialize)
*	
*	Parameters:
*	error - An integer that represents the error (see plmc_lib.h)
*	action - An integer that represents the action taken to deal
*	with this error (see plmc_lib.h)
*
*	Returns:
*	This function return 0 if successful and a 1 if unsuccessful
*********************************************************************/

int send_error(int error, int action, char *ee_id, PLMC_cmd_idx cmd_enum) {
	
	plmc_lib_error myerr;
	
	myerr.ecode = error;
	myerr.acode = action;
	strncpy(myerr.errormsg, PLMC_error_msg[error], PLMC_MAX_ERROR_MSG_LENGTH);
	myerr.errormsg[PLMC_MAX_ERROR_MSG_LENGTH - 1] = '\0';
	strncpy(myerr.action, PLMC_action_msg[action], PLMC_MAX_ACTION_LENGTH);
	myerr.action[PLMC_MAX_ACTION_LENGTH - 1] = '\0';
	if (ee_id != NULL) {
		strncpy(myerr.ee_id, ee_id, PLMC_EE_ID_MAX_LENGTH);
		myerr.ee_id[PLMC_EE_ID_MAX_LENGTH - 1] = '\0';
	} else {
		strcpy(myerr.ee_id, "");
	}
	myerr.cmd_enum = cmd_enum;
	if ((callbacks.err_cb(&myerr)) != 0) {
		syslog(LOG_ERR, "plmc_lib: encountered an error during the "
						"error callback call to PLMSv");
		return(1);
	}
	return(0);
}

/*********************************************************************
*	This function takes care of extracting the essential from
*	the incoming UDP Message
*	This function will use this to populate a struct of the type 
*	udp_msg 
*	
*	Parameters:
*	parsed - a pointer to the udp_msg struct (see plmc_lib.h)
*	incoming - a pointer to the char that contains the message
*
*	Returns:
*	This function return 0 if successful and a 1 if unsuccessful
*********************************************************************/

int parse_udp (udp_msg *parsed, char *incoming)
{
	char *tmpstring;
	int i;
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "parse_udp was called with: %s\n", incoming);
	fflush(plmc_lib_debug);
	#endif
	tmpstring = incoming;
	tmpstring = strstr(tmpstring, " ");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 0)
		tmpstring = tmpstring + 1;
	else
		return 1;
	i = 0;
	while (tmpstring[i] != ':') {
		i++;
		if (i >= strlen(tmpstring) || i > PLMC_EE_ID_MAX_LENGTH) {
			return 1;
		}
	}
	strncpy(parsed->ee_id, tmpstring, i - 1);
	parsed->ee_id[i - 1] = '\0';

	tmpstring = strstr(tmpstring, ":");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 1)
		tmpstring = tmpstring + 2;
	else
		return 1;
	i = 0;
	while (tmpstring[i] != ':') {
			i++;
		if (i >= strlen(tmpstring) || i > PLMC_CMD_RESULT_MAX_LENGTH) {
			return 1;
		}
	}
	strncpy(parsed->msg, tmpstring, i - 1);
	parsed->msg[i - 1] = '\0';
	
	tmpstring = strstr(tmpstring, ":");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 1)
		tmpstring = tmpstring + 2;
	else
		return 1;
	strncpy(parsed->os_info, tmpstring, PLMC_CMD_RESULT_MAX_LENGTH);
	parsed->os_info[PLMC_CMD_RESULT_MAX_LENGTH - 1] = '\0';
	if (strcmp(parsed->msg, "EE_INSTANTIATING") == 0)
		parsed->msg_idx = EE_INSTANTIATING;
	else if (strcmp(parsed->msg, "EE_TERMINATED") == 0)
		parsed->msg_idx = EE_TERMINATED;
	else if (strcmp(parsed->msg, "EE_INSTANTIATED") == 0)
		parsed->msg_idx = EE_INSTANTIATED;
	else if (strcmp(parsed->msg, "EE_TERMINATING") == 0)
		parsed->msg_idx = EE_TERMINATING;

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "parse_udp ee_id is: %s\n", parsed->ee_id);
	fprintf(plmc_lib_debug, "parse_udp msg is: %s\n", parsed->msg);
	fprintf(plmc_lib_debug, "parse_udp os_info is: %s\n", parsed->os_info);
	fflush(plmc_lib_debug);
	#endif
	return 0;
}

/***************************************************************
*	This is a thread cleanup function for the listerner
*	This guy will close the main socket and then call
* 	pthread_cancel on the plmc_connection_mgr thread
*
*	Parameters:
*	arg - a pointer to the socket file descriptor
*
*	Returns:
*	This function doesn't return anything
***************************************************************/

void tcp_listener_cleaner(void *arg)
{
	int socket;
	socket = *((int *) arg);
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "tcp_listener_cleaner was called\n");
	fflush(plmc_lib_debug);
	#endif

	/* Close socket so no more connection can happen */
	close_socket(socket);
	if (pthread_cancel(plmc_connection_mgr_id) != 0) {
		syslog(LOG_ERR, "plmc_lib: tcp_listener_cleaner "
			"encountered an error canceling a thread");
        }
}

/***************************************************************
*	This is a thread cleanup function for the connection
*	mgr. This guy will just call the 
* 	delete_all_thread_entries, 
*	which in turn will call pthreat_cancel on all threads
*
*	Parameters:
*	arg - NULL
*
*	Returns:
*	This function doesn't return anything
***************************************************************/

void connection_mgr_cleaner(void *arg)
{
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "connection_mgr_cleaner was called\n");
	fflush(plmc_lib_debug);
	#endif

	/* Delete all entries */
        delete_all_thread_entries();
}


/*************************************************************
*	This is a thread cleanup function for the client_mgrs
*	This guy will just close the socket
*
*	Parameters:
*	arg - a pointer to the socket file descriptor
*
*	Returns:
*	This function doesn't return anything
**************************************************************/

void client_mgr_cleaner(void *arg)
{
	int socket;
	socket = *((int *) arg);
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "client_mgr_cleaner was called\n");
	fflush(plmc_lib_debug);
	#endif
	/* Close socket so no more connections can happen */
	close_socket(socket);
}

/*************************************************************
*	This is a thread cleanup function for the udp_listener
*	This guy will just close the socket
*
*	Parameters:
*	arg - a pointer to the socket file descriptor
*
*	Returns:
*	This function doesn't return anything
**************************************************************/

void udp_listener_cleaner(void *arg)
{
	int socket;
	socket = *((int *) arg);
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "udp_listener_cleaner was called\n");
	fflush(plmc_lib_debug);
	#endif
	/* Close socket so no more connection can happen */
	close(socket);
}


/*********************************************************************
*	This function takes care of extracting the essential from
*	the incoming TCP Message
*	An example of an incomin message would be 
*	"plmc_d PLMC_SA_PLM_ADMIN_UNLOCK ack 0020f : 0020f". This function 
*	will use this to populate a struct of the type tcp_msg 
*	
*	Parameters:
*	parsed - a pointer to the tcp_msg struct
*	incoming - a pointer to the char that contains the message
*
*	Returns:
*	This function return 0 if successful and a 1 if unsuccessful
*********************************************************************/

int parse_tcp(tcp_msg *parsed, char *incoming)
{
	char *tmpstring;
	int i;
	tmpstring = incoming;
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "parse_tcp_msg was called with: %s\n", 
								incoming);
	fflush(plmc_lib_debug);
	#endif
	tmpstring = strstr(tmpstring, " ");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 0)
		tmpstring = tmpstring + 1;
	else
		return 1;
	i = 0;
	while (tmpstring[i] != ' ') {
		i++;
		if (i >= strlen(tmpstring) || i > PLMC_CMD_NAME_MAX_LENGTH) {
			return 1;
		}
	}
	strncpy(parsed->cmd, tmpstring, i);
	parsed->cmd[i] = '\0';
	tmpstring = strstr(tmpstring, " ");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 0)
		tmpstring = tmpstring + 1;
	else
		return 1;
	tmpstring = strstr(tmpstring, " ");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 0)
		tmpstring = tmpstring + 1; /* Beginning of ee_id */
	else
		return 1;
	i = 0;
	while (tmpstring[i] != ':') {
		i++;
		if (i >= strlen(tmpstring) || i > PLMC_EE_ID_MAX_LENGTH) {
			return 1;
		}
	}
	strncpy(parsed->ee_id, tmpstring, i - 1);
	parsed->ee_id[i - 1] = '\0';
	tmpstring = strstr(tmpstring, ":");
	if (tmpstring == NULL)
		return 1;
	if (strlen(tmpstring) > 1)
		tmpstring = tmpstring + 2;
	else
		return 1;
	strncpy(parsed->result, tmpstring, PLMC_CMD_RESULT_MAX_LENGTH);
	parsed->result[PLMC_CMD_RESULT_MAX_LENGTH - 1] = '\0';

	parsed->cmd_enum = plmc_cmd_string_to_enum(parsed->cmd);
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "parse_tcp ee_id is: %s\n", parsed->ee_id);
	fprintf(plmc_lib_debug, "parse_tcp cmd is: %s\n", parsed->cmd);
	fprintf(plmc_lib_debug, "parse_tcp result is: %s\n", parsed->result);
	fflush(plmc_lib_debug);
	#endif 

	return 0;

}

/**********************************************************************
*	plmc_connection_mgr - This is the thread that is responsible for 
*	monitoring all the connections
*	This guy will periodically wakeup, walk through the list
*	and ping all the plmc clients. If they are not anymore available
*	plmc_connection_mgr will delete the entry and kill off any thread
*	that is talking to them
*	
*	Parameters:
*	No parameters
*
*	Returns:
*	This function returns a *void
***********************************************************************/
void *plmc_connection_mgr(void *arguments)
{
	thread_entry *tentry;
	thread_data *tdata;
	char result[PLMC_MAX_TCP_DATA_LEN]; 
	pthread_t plmc_client_mgr_id;
	PLMC_cmd_idx cmd_enum;
	int sockfd, o_state, kill;

	pthread_cleanup_push(connection_mgr_cleaner, NULL);	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_connection_mgr started up\n");
	fflush(plmc_lib_debug);
	#endif 
	while (1) {
		/* Start by sleeping 5 seconds */
		sleep(5); /* Perhaps a definition in a header file */
		tentry = HEAD;

		while(tentry != NULL) {
			/* Do not allow to be canceled */
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 
								&o_state);

			/* Lock to get the socket */
			if (pthread_mutex_lock(&tentry->thread_d.td_lock) 
									!= 0) {
				syslog(LOG_ERR, "plmc_lib: "
					"plmc_connection_mgr encountered an "
					"error getting a lock for a client");
				send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
					PLMC_LIBACT_IGNORING, NULL,
								PLMC_NOOP_CMD);
				break;
			}
			tdata = &(tentry->thread_d);
			sockfd = tdata->socketfd;
			plmc_client_mgr_id = tdata->td_id;
			kill = tdata->kill;
			if(tdata->done == 0) {
				cmd_enum = plmc_cmd_string_to_enum(
								tdata->command);
				if (cmd_enum < 0)
					cmd_enum = PLMC_NOOP_CMD;
			} else {
				cmd_enum = PLMC_NOOP_CMD;
			}

			if (sockfd != 0 && recv(sockfd, result, 
					PLMC_MAX_TCP_DATA_LEN, MSG_PEEK | 
							MSG_DONTWAIT) == 0) {
				kill = 1;
				#ifdef PLMC_LIB_DEBUG
				fprintf(plmc_lib_debug, "plmc_connection_mgr: "
					"Discovered a client that didn't "
					"answer. Setting kill = 1. EE_ID is "
					"%s\n", tdata->ee_id);
				fflush(plmc_lib_debug);
				#endif 
			}

			/* Check if kill is 1. If it is it means we potentially
			* need to kill the client and close the socket 
			*/
			if (kill == 1) {
				#ifdef PLMC_LIB_DEBUG
				fprintf(plmc_lib_debug, "plmc_connection_mgr: "
					"Kill is set to 1. Cleaning up "
					"for EE_ID %s\n", tdata->ee_id);
				fflush(plmc_lib_debug);
				#endif 
				/* 
				* If the client_mgr thread is running, 
				* kill him
				*/
				if (plmc_client_mgr_id != 0) {
					#ifdef PLMC_LIB_DEBUG
					fprintf(plmc_lib_debug, 
						"plmc_connection_mgr: Client "
						"mgr is running. We will need "
						"to kill him. EE_ID is %s\n", 
								tdata->ee_id);
					fflush(plmc_lib_debug);
					#endif 
					if (pthread_cancel(plmc_client_mgr_id) 
									!= 0) {
						syslog(LOG_ERR, "plmc_lib: "
							"encountered an error "
							"trying to cancel a "
							"client mgr thread");
					}
				}
				if (sockfd != 0) {
					send_error(PLMC_LIBERR_LOST_CONNECTION, 
						PLMC_LIBACT_CLOSE_SOCKET,
						tdata->ee_id, cmd_enum);
					#ifdef PLMC_LIB_DEBUG
					fprintf(plmc_lib_debug, 
						"plmc_connection_mgr: We are "
						"closing the socket. EE_ID is "
						"%s\n", tdata->ee_id);
					fflush(plmc_lib_debug);
					#endif 
					/* Now, close the socket */
					close_socket(sockfd);
					if ((callbacks.connect_cb(tdata->ee_id,
						PLMC_DISCONN_CB_MSG)) != 0) {
						syslog(LOG_ERR, "plmc_lib: "
							"encountered an error "
							"during the disconnect "
							"callback");
					}
				}
				tdata->socketfd = 0;
				tdata->td_id = 0;
				tdata->kill = 0;
				tdata->done = 1;
			}

			/* 
			* Ok, time to unlock
			*/
			if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
				syslog(LOG_ERR, "plmc_lib: encountered an "
					"error unlocking a client mutex");
				send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
							PLMC_LIBACT_UNDEFINED, 
						tdata->ee_id, cmd_enum);
			}			
			/* Allow to be canceled again */
			pthread_setcancelstate(o_state, &o_state);
			tentry = tentry->next;
		}
	}
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_connection_mgr: I am exiting\n");
	fflush(plmc_lib_debug);
	#endif 
	pthread_cleanup_pop(0);
	pthread_exit((void *)NULL);
}

/**********************************************************************
*	plmc_client_mgr - This is the thread that will be started
*	by a library call when PLMSv wants to issue a command to a 
*	specific PLMc. This thread will hang around till PLMc returns
*	a response to the command or times out. After it gets a message
*	back from PLMc it will issue a callback to the thread that 
*	invoked the command
*	
*	Parameters:
*	argument => The incoming data struct for the thread. should
*	be casted to thread_entry
*
*	Returns:
*	This function returns a *void
***********************************************************************/

void *plmc_client_mgr(void *arguments)
{
	thread_entry *tentry;
	thread_data *tdata;
	char command[PLMC_CMD_NAME_MAX_LENGTH];
	char result[PLMC_MAX_TCP_DATA_LEN];
	char ee_id[PLMC_EE_ID_MAX_LENGTH];
	PLMC_cmd_idx cmd_enum;
	int sockfd, retval, bytes_received;
	struct timeval tv;
	int( *callback)(tcp_msg *);
	tentry = (thread_entry *) arguments;
	tdata = &(tentry->thread_d);
	command[0] ='\0';
	
	/* Adding 10 seconds to the plmc_cmd_timeout_secs in the 
	*  plmcd.conf file
	*/
	tv.tv_sec = config.plmc_cmd_timeout_secs + 10;
	tv.tv_usec = 0;
	tcp_msg tcpmsg;
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_client_mgr started up\n");
	fflush(plmc_lib_debug);
	#endif 
	
	/* Lock to get the socket */
	if (pthread_mutex_lock(&tdata->td_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: plmc_client_mgr encountered an "
				"error getting a lock for a client");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES,
				PLMC_LIBACT_EXIT_THREAD, NULL, PLMC_NOOP_CMD);
		pthread_exit((void *)NULL);
	}
	sockfd = tentry->thread_d.socketfd;
	strncpy(ee_id, tdata->ee_id, PLMC_EE_ID_MAX_LENGTH);
	ee_id[PLMC_EE_ID_MAX_LENGTH - 1] = '\0';
	
	/* 
	* There is a new command, copy it locally and return the 
	* lock 
	*/
	strncpy(command, tdata->command, PLMC_CMD_NAME_MAX_LENGTH);
	command[PLMC_CMD_NAME_MAX_LENGTH - 1] = '\0';
	cmd_enum = plmc_cmd_string_to_enum(tdata->command);
	/* Get the callback as well */
	callback = tdata->callback;
		
	/* Unlock */
	if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: encountered an "
			"error unlocking a client mutex");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_UNDEFINED, ee_id, cmd_enum);
	}			
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_client_mgr got a new command %s for "
						"ee_id %s\n", command, ee_id);
	fflush(plmc_lib_debug);
	#endif 
	
	/* 
	* Change the socket option so that it waits for tv.tv_sec seconds 
	* for a respons 
	*/
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, 
							sizeof tv) < 0)  {
		syslog(LOG_ERR, "plmc_lib: encountered an error during "
				"a call to setsockopt() from client_mgr");
		syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
		syslog(LOG_ERR, "plmc_lib: Closing socket and exiting "
								"client_mgr");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_EXIT_THREAD, ee_id, cmd_enum);
		if (close_socket(sockfd) !=0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error "
					"during a call to close_socket");
		}
		pthread_exit((void *)NULL);
	}

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_client_mgr: Sending command to client\n");
	fflush(plmc_lib_debug);
	#endif 
	/* Send the command to PLMc */
	retval = send(sockfd, command, strlen(command), 0);
	if (retval <= 0) {
		syslog(LOG_ERR, "plmc_lib: Client Mgr encountered an error "
				"during a call to send(). Leaving cleanup for "
				"connection mgr");
		syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
		/* Connection MGR will send error to plmc_lib */
		pthread_exit((void *)NULL);
	}
		
	/* Get the response */
	bytes_received = recv(sockfd, result, PLMC_MAX_TCP_DATA_LEN, 0);
	if (bytes_received <= 0 ){
		/* 
		* The client didn't come back in time. Can't 
		* hang around waiting for this guy. Close this
		* socket. He'll just have to reconnect 
		*/
		syslog(LOG_ERR, "plmc_lib: Client Mgr encountered an "
				"error during a call to recv(). Leaving "
				"cleanup for connection mgr");
		/* Send a timeout error */
		send_error(PLMC_LIBERR_TIMEOUT, PLMC_LIBACT_CLOSE_SOCKET, ee_id,
								cmd_enum);
		/* Lock to change the socket */
		if (pthread_mutex_lock(&tdata->td_lock) != 0) {
			syslog(LOG_ERR, "plmc_lib: client_mgr encountered an "
					"error getting a lock for a client");
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 	
				PLMC_LIBACT_EXIT_THREAD, ee_id, cmd_enum);
			pthread_exit((void *)NULL);
		}
		if (close_socket(sockfd) !=0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error "
					"during a call to close_socket");
		}
		/* 
		* Set socketfd to zero to indicate to connection_mgr
		*  that we are gone
		*/
		tentry->thread_d.socketfd = 0; 
		tdata->td_id = 0;
		tdata->kill = 1;
		if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
			syslog(LOG_ERR, "plmc_lib: plmc_client_mgr encountered "
					"an error unlocking a client mutex");
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_UNDEFINED, ee_id, cmd_enum);
		}
		pthread_exit((void *)NULL);
	}
	result[bytes_received] = '\0';
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_client_mgr: Got command back %s\n", 
									result);
	fflush(plmc_lib_debug);
	#endif 

	if (parse_tcp(&tcpmsg, result) != 0) {
		syslog(LOG_ERR, "plmc_lib: TCP message " "invalid %s", result);
		send_error(PLMC_LIBERR_MSG_INVALID, PLMC_LIBACT_IGNORING, ee_id,
								cmd_enum);
		pthread_exit((void *)NULL);
	}
	
	/* Get lock */
	if (pthread_mutex_lock(&tdata->td_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: client_mgr encountered an error "
						"getting a lock for a client");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_UNDEFINED, ee_id, cmd_enum);
		if (close_socket(sockfd) !=0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error during "
						      "a call to close_socket");
		}
		pthread_exit((void *)NULL);
	}

	/* Tell the world we are done with this guy */
	tdata->done = 1;
	tdata->td_id = 0; /* We are exiting */

	/* Unlock */
	if (pthread_mutex_unlock(&tdata->td_lock) != 0) {
		syslog(LOG_ERR, "plmc_lib: plmc_client_mgr encountered an "
					"error unlocking a client mutex");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_UNDEFINED, ee_id, cmd_enum);
	}
	
	/* Send callback */
	if ((callback(&tcpmsg)) != 0) {
		syslog(LOG_ERR, "plmc_lib: encountered an error during the "
					      "result callback call to PLMSv");
		send_error(PLMC_LIBERR_ERR_CB, PLMC_LIBACT_IGNORING, ee_id,
								cmd_enum);
		pthread_exit((void *)NULL);
	}

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_client_mgr: Done with command, "
								"exiting\n");
	fflush(plmc_lib_debug);
	#endif 

	pthread_exit((void *)NULL);
}


/**********************************************************************
*	plmc_udp_listener - This is the thread that is responsible for 
*	dealing with incoming UDP message
*	
*	Parameters:
*	argument => The incoming data struct for the thread.
*	This is not being used
*
*	Returns:
*	This function returns a *void
***********************************************************************/

void *plmc_udp_listener(void *arguments) 
{
	int sockfd, msg_length;
	struct sockaddr_in servaddr, cliaddr;
	struct in_addr inp;
	socklen_t addr_length;
	char mesg[PLMC_MAX_UDP_DATA_LEN];
	udp_msg udpmsg;
	char *match_ip;
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_udp_listener started\n");
	fflush(plmc_lib_debug);
	#endif 
	/* Get the socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		syslog(LOG_ERR, "plmc_lib: encountered an error opening a "
								"socket");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
			PLMC_LIBACT_DESTROY_LIBRARY, NULL, PLMC_NOOP_CMD);
		if (pthread_cancel(tcp_listener_id) != 0) {
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_IGNORING, NULL, PLMC_NOOP_CMD);
			syslog(LOG_ERR, "plmc_lib: encountered an error "
						"canceling tcp_listener");
		}
		pthread_exit((void *)NULL);
	}

	/*
	* Here we need to register a cleanup function that will be called in 
	* case the UDP listener gets canceled by the library. This is to close 
	* the socket.
	*/
	pthread_cleanup_push(udp_listener_cleaner, (void *)&sockfd);

	match_ip = plmc_get_listening_ip_addr(plmc_config_file);
	if (strlen(match_ip) == 0) {
		syslog(LOG_ERR, "plmc_udp_listener cannot match available "
				"network insterfaces with IPs of controllers "
				"specified in the plmcd.conf file");
		send_error(PLMC_LIBERR_NO_CONF, PLMC_LIBACT_DESTROY_LIBRARY, 
							NULL, PLMC_NOOP_CMD);
		pthread_exit((void *)NULL);
	}
	inet_aton(match_ip, &inp);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inp.s_addr;
	servaddr.sin_port=htons(atoi(config.udp_broadcast_port));
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
		syslog(LOG_ERR, "plmc_lib: encountered an error during a "
							"call to bind()");
		syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
						PLMC_LIBACT_DESTROY_LIBRARY,
						NULL, PLMC_NOOP_CMD);
		if (pthread_cancel(tcp_listener_id) != 0) {
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_IGNORING, NULL, PLMC_NOOP_CMD);
			syslog(LOG_ERR, "plmc_lib: encountered an error "
						"canceling tcp_listener");
		}
		pthread_exit((void *)NULL);
	}

	/*
	We are now entering the while loop. We will hang out here till somebody
	cancels us
	*/
	while (1) {
		addr_length = sizeof(cliaddr);
		msg_length = recvfrom(sockfd,mesg,PLMC_MAX_UDP_DATA_LEN,
				0,(struct sockaddr *)&cliaddr,&addr_length);
		mesg[msg_length] = '\0';
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_udp_listener got a new message: "
								"%s\n", mesg);
		fflush(plmc_lib_debug);
		#endif 

		/* Get the struct setup */
		if (parse_udp(&udpmsg, mesg) != 0) {
			syslog(LOG_ERR, "plmc_lib: UDP message invalid %s", 
									mesg);
			send_error(PLMC_LIBERR_MSG_INVALID, 
				PLMC_LIBACT_IGNORING, NULL, PLMC_NOOP_CMD);
			continue;
		}
		if ((callbacks.udp_cb(&udpmsg)) != 0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error "
				"during the UDP callback call to PLMSv");
			send_error(PLMC_LIBERR_ERR_CB, PLMC_LIBACT_IGNORING,
					udpmsg.ee_id, PLMC_NOOP_CMD);
			continue;
		}
	}
	/* 
	Remove the cleanup for the connected socket
	Normally the code will never get here.
	*/
	pthread_cleanup_pop(0);
}

/**********************************************************************
*	plmc_tcp_listener - This is the thread that is responsible for 
*	dealing with new connections over the TCP channel. For
*	Each new connection it will get the ee_id from the connecting
*	client and store the information in the global list.
*	
*	Parameters:
*	argument => The incoming data struct for the thread. should
*	be casted to cb_functions.
*
*	Returns:
*	This function returns a *void
***********************************************************************/

void *plmc_tcp_listener(void *arguments) 
{
	int sockfd, connected, true=1, bytes_received;
	unsigned int sin_size;
	struct sockaddr_in servaddr, cliaddr;
	struct timeval tv;
	struct in_addr inp;
	char recv_mesg[PLMC_MAX_TCP_DATA_LEN], *match_ip;

	pthread_attr_t udp_attr;
	thread_entry *tentry;
	int retval = 0;
	tcp_msg tcpmsg;

	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_tcp_listener started\n");
	fflush(plmc_lib_debug);
	#endif 

	/* 
	Set the timeout value for recieve messages from clients
	*/
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	/* Get the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR, "plmc_lib: encountered an error opening "
								"a socket");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
			PLMC_LIBACT_DESTROY_LIBRARY, NULL, PLMC_NOOP_CMD);
		pthread_exit((void *)NULL);
	}

	/*
	Here we need to register a cleanup function that will be called in case
	the listener gets canceled by the library. This is to close the socket
	and to cancel all children
	*/
	pthread_cleanup_push(tcp_listener_cleaner, (void *)&sockfd);

	/* Set some socket options, not sure we need this */
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) < 0) {
		syslog(LOG_ERR, "plmc_lib: encountered an error during a "
							"call to setsockopt()");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, PLMC_LIBACT_IGNORING,
							NULL, PLMC_NOOP_CMD);
	}

	/* Set up the IP stuff */
	match_ip = plmc_get_listening_ip_addr(plmc_config_file);
	if (strlen(match_ip) == 0) {
		syslog(LOG_ERR, "plmc_tcp_listener cannot match available "
				"network insterfaces with IPs of controllers "
				"specified in the plmcd.conf file");
		send_error(PLMC_LIBERR_NO_CONF, PLMC_LIBACT_DESTROY_LIBRARY, 
							NULL, PLMC_NOOP_CMD);
		pthread_exit((void *)NULL);
	}
	inet_aton(match_ip, &inp);

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(config.tcp_plms_listening_port));
	servaddr.sin_addr.s_addr = inp.s_addr;
	bzero(&(servaddr.sin_zero),8);

	/* Socket Address to server */
	if (bind(sockfd, (struct sockaddr *)&servaddr, 
						sizeof(struct sockaddr)) == -1){
		syslog(LOG_ERR, "plmc_lib: encountered an error during a "
							"call to bind()");
		syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
			PLMC_LIBACT_DESTROY_LIBRARY, NULL, PLMC_NOOP_CMD);
		close(sockfd);
		pthread_exit((void *)NULL);
	}

	/* Spec max # queued up plmc connections */
	if (listen(sockfd, 5) == -1) {
		syslog(LOG_ERR, "plmc_lib: encountered an error during a call "
								"to listen()");
		syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
			PLMC_LIBACT_DESTROY_LIBRARY, NULL, PLMC_NOOP_CMD);
		close(sockfd);
		pthread_exit((void *)NULL);
	}

	sin_size = sizeof(struct sockaddr_in);
	
	/* Set threads detached as we don't want to join them */
	pthread_attr_init(&udp_attr);
	pthread_attr_setdetachstate(&udp_attr, PTHREAD_CREATE_DETACHED); 
	
	#ifdef PLMC_LIB_DEBUG
	fprintf(plmc_lib_debug, "plmc_tcp_listener starting the UDP thread\n");
	fflush(plmc_lib_debug);
	#endif 
	/* Create the UDP thread */
	if (pthread_create(&(udp_listener_id), &udp_attr, plmc_udp_listener, 
								NULL) != 0) { 
		syslog(LOG_ERR, "plmc_lib: Could not create the UDP thread");
		send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
			PLMC_LIBACT_DESTROY_LIBRARY, NULL, PLMC_NOOP_CMD);
		close(sockfd);
		pthread_exit((void *)NULL);
	}
	
	
	/* 
	* This is the while loop that will take in new connections and 
	* start client_mgr threads that deal with those connections.
	*/
	while (1) {
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_tcp_listener entering the while "
								"loop\n");
		fflush(plmc_lib_debug);
		#endif 
		connected = accept(sockfd, (struct sockaddr *)&cliaddr, 
								&sin_size);
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_tcp_listener got a new "
								"connection\n");
		fflush(plmc_lib_debug);
		#endif 
		if (connected < 0) {
			syslog(LOG_ERR, "plmc_lib: encoutered a problem in "
							"the accept() call");
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_IGNORING, NULL, PLMC_NOOP_CMD);
			continue;
		}

		/* 
		* Push another cleanup function as we have opened a new 
		* socket (connected) and have not yet handed it over to a child 
		* thread (client_mgr). If we get canceled here we need to close 
		* that socket.
		*/
		pthread_cleanup_push(client_mgr_cleaner, (void *)&connected);

		/*
		Set the timeout SO_RCVTIMEO for the new socket
		This so a client can't hang the listener for too long
		*/
		if (setsockopt (connected, SOL_SOCKET, SO_RCVTIMEO, 
						(char *)&tv, sizeof tv) < 0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error during "
						"a call to setsockopt()");
			syslog(LOG_ERR, "plmc_lib: errnor is %d", errno);
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
				PLMC_LIBACT_CLOSE_SOCKET, NULL, PLMC_NOOP_CMD);
			if (close_socket(connected) !=0) {
				syslog(LOG_ERR, "plmc_lib: encountered an "
					"error during a call to close_socket");
			}
			continue;
		}
	
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_tcp_listener sending a request "
							"for the ee_id\n");
		fflush(plmc_lib_debug);
		#endif 
		/* Send a request for the ee_id to the client */
		retval = send(connected, PLMC_cmd_name[PLMC_GET_ID_CMD],
				strlen(PLMC_cmd_name[PLMC_GET_ID_CMD]), 0);
		if (retval < 0) {
			syslog(LOG_ERR, "plmc_lib: encountered an error "
						"during a call to send()");
			send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
					PLMC_LIBACT_CLOSE_SOCKET, NULL, 
					PLMC_GET_ID_CMD);
			if (close_socket(connected) !=0) {
				syslog(LOG_ERR, "plmc_lib: encountered an "
					"error during a call to close_socket");
				send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
					PLMC_LIBACT_CLOSE_SOCKET, NULL, 
					PLMC_GET_ID_CMD);
			}
			continue;
		}
	
		/* Now wait for the response for tv time */
		bytes_received = recv(connected, recv_mesg, 
						PLMC_MAX_TCP_DATA_LEN, 0);
		if (bytes_received < 0 ){
			/* The client didn't come back in time. Can't hang
			around waiting for this guy. Close this socket.
			He'll just have to reconnect */
			syslog(LOG_ERR, "plmc_lib: The client is taking too "
					"long to respond, closing socket");
			send_error(PLMC_LIBERR_TIMEOUT, 
						PLMC_LIBACT_CLOSE_SOCKET, NULL,
					       	PLMC_GET_ID_CMD);
			if (close_socket(connected) !=0) {
				syslog(LOG_ERR, "plmc_lib: encountered an "
					"error during a call to close_socket");
				send_error(PLMC_LIBERR_SYSTEM_RESOURCES, 
					PLMC_LIBACT_CLOSE_SOCKET, NULL, 
					PLMC_GET_ID_CMD);
			}
			continue;
		}
		
		recv_mesg[bytes_received] = '\0';
		
		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_tcp_listener got response back: "
							"%s\n", recv_mesg);
		fflush(plmc_lib_debug);
		#endif 
		if (parse_tcp(&tcpmsg, recv_mesg) != 0) {
			syslog(LOG_ERR, "plmc_lib: Invalid TCP message "
							"during GET_EE_ID");
			send_error(PLMC_LIBERR_MSG_INVALID, 
						PLMC_LIBACT_CLOSE_SOCKET, NULL,
						PLMC_GET_ID_CMD);
			if (close_socket(connected) !=0) {
				syslog(LOG_ERR, "plmc_lib: encountered an "
					"error during a call to close_socket");
			}
			continue;
		}

		#ifdef PLMC_LIB_DEBUG
		fprintf(plmc_lib_debug, "plmc_tcp_listener creating thread "
				"entry with ee_id %s and socket %d\n", 
				tcpmsg.ee_id, connected);
		fflush(plmc_lib_debug);
		#endif 
		
		/* Create the thread_entry - This is thread safe */
		tentry = create_thread_entry(tcpmsg.ee_id, connected);
		if (tentry == NULL) {
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "plmc_tcp_listener found an "
					"entry that has to be closed, "
					"closing socket on current client\n");
			fflush(plmc_lib_debug);
			#endif 
			close_socket(connected);
		} else { 
			#ifdef PLMC_LIB_DEBUG
			fprintf(plmc_lib_debug, "plmc_tcp_listener didn't "
					"find an entry that has to be closed. "
					"We can reuse this entry\n");
			fflush(plmc_lib_debug);
			#endif 
			if ((callbacks.connect_cb(tcpmsg.ee_id, 
						PLMC_CONN_CB_MSG)) != 0) {
				send_error(PLMC_LIBERR_ERR_CB, 
					PLMC_LIBACT_IGNORING, tcpmsg.ee_id, 
					PLMC_GET_ID_CMD);
			}
		}
		
		/* 
		* Remove the cleanup for the connected socket as we don't care 
		* about that anymore. This socket is now handled by the 
		* client_mgr and this guy will close it if it is canceled
		*/
		pthread_cleanup_pop(0);
	}
	pthread_cleanup_pop(1);
	pthread_exit((void *)NULL);
}
