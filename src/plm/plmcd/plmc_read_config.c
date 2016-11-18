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

/*
 * plmc_read_config() utility function.
 *
 * plmc_read_config() is an internal utility function used by
 * plmc, and the plmc_lib to read a plmcd.conf
 * configuration file.  Users of PLMc should not attempt to use
 * this function as the library APIs provide all of the necessary
 * means to access the capabilities of PLMc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>

#include "plm/plmcd/plmc.h"

static int checkfile(char *buf)
{
	struct stat statbuf;
	int ii;
	char cmd[PLMC_MAX_TAG_LEN];

	strncpy(cmd, buf, PLMC_MAX_TAG_LEN -1);
	for (ii = 0; ii < strlen(cmd); ii++)
		if (cmd[ii] == ' ')
			cmd[ii] = '\0';

	if (stat(cmd, &statbuf) == -1) {
		syslog(LOG_ERR, "plmc_read_config: File does not exsist: %s",
									 cmd);
		return -1;
	}

	return 0;
}

/*
 * plmc_read_config()
 *
 * A function to read the plmcd.conf confguration file.
 *
 * Inputs:
 *   plmc_config_file : A file pathname.
 *   config           : Pointer to location where file data is stored.
 *
 * Returns: 0 on success.
 * Returns: errno on error.
 *
 */
int plmc_read_config(char *plmc_config_file, PLMC_config_data *config) 
{
	FILE *plmc_conf_file;
	char line[PLMC_MAX_TAG_LEN];
	int i, n, comment_line, fieldmissing = 0, tag = 0;
	int num_services = 0;

	memset(config, 0, sizeof(PLMC_config_data));
	/*
	* Initialize the socket timeout values in case they are not
	* set in the plmcd.conf file
	*/
	config->so_keepalive = SOCK_KEEPALIVE;
	config->tcp_keepidle_time = KEEPIDLE_TIME;
	config->tcp_keepalive_intvl = KEEPALIVE_INTVL;
	config->tcp_keepalive_probes = KEEPALIVE_PROBES;

	/* 
	* Set timeout to a negative value so we can detect if the value 
	* is missing in the conf file
	*/
	config->plmc_cmd_timeout_secs = -1;

	/* Open plmcd.conf config file. */
	plmc_conf_file = fopen(plmc_config_file, "r");
	if (plmc_conf_file == NULL) {
		/* Problem with conf file - return errno value */
		syslog(LOG_ERR, "plmc_read_cofig: there was a problem opening "
							"the plmcd.conf file");
		return(errno);
	}

	/* Read file. */
	while(fgets(line, PLMC_MAX_TAG_LEN, plmc_conf_file)){
		/* If blank line, skip it - and set tag back to 0. */
		if (strcmp(line, "\n") == 0) {
			tag = 0;
			continue;
		}

		/* If a comment line, skip it. */
		n = strlen(line);
		comment_line = 0;
		for (i=0; i<n; i++) {
			if ((line[i] == ' ') || (line[i] == '\t'))
				continue;
			else
				if (line[i] == '#') {
					comment_line = 1;
					break;
				}
				else
					break;
		}
		if (comment_line)
			continue;

		/* Remove the new-line char at the end of the line. */
		line[n-1] = 0;

		/* Set tag if we are not in a tag. */
		if ((tag == 0) || (line[0] == '[')) {
			if (strcmp(line, "[ee_id]") == 0)
				tag = PLMC_EE_ID;
			if (strcmp(line, "[plmc_msg_protocol_version]") == 0)
				tag = PLMC_MSG_PROTOCOL_VERSION;
			if (strcmp(line, "[controller_1_ip]") == 0)
				tag = PLMC_CONTROLLER_1_IP;
			if (strcmp(line, "[controller_2_ip]") == 0)
				tag = PLMC_CONTROLLER_2_IP;
			if (strcmp(line, "[services]") == 0)
				tag = PLMC_SERVICES;
			if (strcmp(line, "[osaf]") == 0)
				tag = PLMC_OSAF;
			if (strcmp(line, "[tcp_plms_listening_port]") == 0)
				tag = PLMC_TCP_PLMS_LISTENING_PORT;
			if (strcmp(line, "[udp_broadcast_port]") == 0)
				tag = PLMC_UDP_BROADCAST_PORT;
			if (strcmp(line, "[os_type]") == 0)
				tag = PLMC_OS_TYPE;
			if (strcmp(line, "[plmc_cmd_timeout_secs]") == 0)
				tag = PLMC_CMD_TIMEOUT_SECS;
			if (strcmp(line, "[os_reboot_cmd]") == 0)
				tag = PLMC_OS_REBOOT_CMD;
			if (strcmp(line, "[os_shutdown_cmd]") == 0)
				tag = PLMC_OS_SHUTDOWN_CMD;
			if (strcmp(line, "[so_keepalive]") == 0)
				tag = SKEEPALIVE;
			if (strcmp(line, "[tcp_keepidle_time]") == 0)
				tag = TCP_KEEPIDLE_TIME;
			if (strcmp(line, "[tcp_keepalive_intvl]") == 0)
				tag = TCP_KEEPALIVE_INTVL;
			if (strcmp(line, "[tcp_keepalive_probes]") == 0)
				tag = TCP_KEEPALIVE_PROBES;
		}
		else {
			/* Set the value of the tag in config data structure. */
			switch (tag) {
				case PLMC_EE_ID:
					strncpy(config->ee_id, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_MSG_PROTOCOL_VERSION:
					strncpy(config->msg_protocol_version, 
						line, PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_CONTROLLER_1_IP:
					strncpy(config->controller_1_ip, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_CONTROLLER_2_IP:
					strncpy(config->controller_2_ip, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_SERVICES:
#if (ENABLE_SYSTEMD == 0)
					if (checkfile(line)) {
						syslog(LOG_ERR, "plmc_read_"
							"config.c: Service "
							"to start and stop "
							"%s doesn't exist", 
									line);
						return -1;
					}
#endif
					strncpy(config->services[num_services],
						 line, PLMC_MAX_TAG_LEN -1);
					num_services++;
					/*
					* Don't want to overrun our array of 
					* services. 
					*/
					if (num_services >= PLMC_MAX_SERVICES){
						syslog(LOG_ERR, "plmc_read_"
							"config.c: Number of "
							"services exceeded "
							"MAX_SERVIES: %d\n", 
							PLMC_MAX_SERVICES);
						return -1;
					}
					break;
				case PLMC_OSAF:
#if (ENABLE_SYSTEMD == 0)
					if (checkfile(line)) {
						syslog(LOG_ERR, "plmc_read_"
							"config.c: Osaf "
							"to start and stop "
							"%s doesn't exist", 
									line);
						return -1;
					}
#endif
					strncpy(config->osaf, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_TCP_PLMS_LISTENING_PORT:
					strncpy(config->tcp_plms_listening_port,
						 line, PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_UDP_BROADCAST_PORT:
					strncpy(config->udp_broadcast_port,
						 line, PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_OS_TYPE:
					strncpy(config->os_type, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_CMD_TIMEOUT_SECS:
					config->plmc_cmd_timeout_secs = 
								atoi(line);
					if (config->plmc_cmd_timeout_secs 
									< 1 ) {
						syslog(LOG_ERR, 
							"plmc_cmd_timeout_secs "
						"must be a positive integer");
						return -1;
					}
					tag = 0;
					break;
				case PLMC_OS_REBOOT_CMD:
					if (checkfile(line)) {
						syslog(LOG_ERR, "plmc_read_"
							"config.c: Reboot "
							"command %s doesn't "
								"exist", line);
						return -1;
					}
					strncpy(config->os_reboot_cmd, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case PLMC_OS_SHUTDOWN_CMD:
					if (checkfile(line)) {
						syslog(LOG_ERR, "plmc_read_"
							"config.c: Shutdown "
							"command %s doesn't "
								"exist", line);
						return -1;
					}
					strncpy(config->os_shutdown_cmd, line, 
							PLMC_MAX_TAG_LEN -1);
					tag = 0;
					break;
				case SKEEPALIVE:
					config-> so_keepalive = atoi(line);
					if (config->so_keepalive < 0 || 
						config->so_keepalive > 1) {
						syslog(LOG_ERR, "so_keepalive "
							"needs to be 0 or 1");
						return -1;
					}
					tag = 0;
					break;
				case TCP_KEEPIDLE_TIME:
					config-> tcp_keepidle_time = atoi(line);
					if (config->tcp_keepidle_time < 1 ) {
						syslog(LOG_ERR, 
							"tcp_keepidle_time "
						"must be a positive integer");
						return -1;
					}
					tag = 0;
					break;
				case TCP_KEEPALIVE_INTVL:
					config-> tcp_keepalive_intvl = atoi(line);
					if (config->tcp_keepalive_intvl < 1 ) {
						syslog(LOG_ERR, 
							"tcp_keepalive_intvl "
						"must be a positive integer");
						return -1;
					}
					tag = 0;
					break;
				case TCP_KEEPALIVE_PROBES:
					config-> tcp_keepalive_probes = atoi(line);
					if (config->tcp_keepalive_probes < 1 ) {
						syslog(LOG_ERR, 
							"tcp_keepalive_probes "
						"must be a positive integer");
						return -1;
					}
					tag = 0;
					break;
				default:
					break;
			}
		}
	}

	/* End-of-file or error? */
	int err = feof(plmc_conf_file) ? 0 : errno;

	config->num_services = num_services;

	/* Close file. */
	fclose(plmc_conf_file);

	/* Test so we have all mandatory fields */
	if (strlen(config->ee_id) == 0) {
		syslog(LOG_ERR, "plmc_read_config: ee_id is missing in conf "
								"file");
		fieldmissing = 1;
	} else if (strlen(config->msg_protocol_version) == 0) {
		syslog(LOG_ERR, "plmc_read_config: msg_protocol_version "
						"is missing in conf file");
		fieldmissing = 1;
	} else if (strlen(config->controller_1_ip) == 0) {
		syslog(LOG_ERR, "plmc_read_config: controller_1_ip is missing "
							"in conf file");
		fieldmissing = 1;
	} else if (strlen(config->controller_2_ip) == 0) {
		syslog(LOG_ERR, "plmc_read_config: controller_2_ip is missing "
							"in conf file");
		fieldmissing = 1;
	} else if (strlen(config->tcp_plms_listening_port) == 0) {
		syslog(LOG_ERR, "plmc_read_config: tcp_plms_listening_port is "
						"missing in conf file");
		fieldmissing = 1;
	} else if (strlen(config->udp_broadcast_port) == 0) {
		syslog(LOG_ERR, "plmc_read_config: udp_broadcast_port is "
						"missing in conf file");
		fieldmissing = 1;
	} else if (strlen(config->os_type) == 0) {
		syslog(LOG_ERR, "plmc_read_config: os_type is missing in "
								"conf file");
		fieldmissing = 1;
	} else if (config->plmc_cmd_timeout_secs < 0) {
		syslog(LOG_ERR, "plmc_read_config: plmc_cmd_timeout_secs "
					"is missing or invalid in conf file");
		fieldmissing = 1;
	} else if (strlen(config->os_reboot_cmd) == 0) {
		syslog(LOG_ERR, "plmc_read_config: os_reboot_cmd is missing "
								"in conf file");
		fieldmissing = 1;
	} else if (strlen(config->os_shutdown_cmd) == 0) {
		syslog(LOG_ERR, "plmc_read_config: os_shutdown_cmd is missing "
								"in conf file");
		fieldmissing = 1;
	}

	if (fieldmissing == 1)
		return -1;
	/* All done. */
	return(err);
}

/*
 * plmc_print_config()
 *
 * A function to print the plmcd.conf confguration file.
 *
 * Inputs:
 *   config : Pointer to location where file data is stored.
 *
 */
void plmc_print_config(PLMC_config_data *config)
{
	int i;

	printf("plmcd.conf: config file contains:\n");

	printf("  [ee_id]\n");
	printf("  %s\n", config->ee_id);

	printf("  [plmc_msg_protocol_version]\n");
	printf("  %s\n", config->msg_protocol_version);

	printf("  [controller_1_ip]\n");
	printf("  %s\n", config->controller_1_ip);

	printf("  [controller_2_ip]\n");
	printf("  %s\n", config->controller_2_ip);

	printf("  [services]\n");
	for (i=0; i<config->num_services; i++) {
		printf("  %s\n", config->services[i]);
	}

	printf("  [osaf]\n");
	printf("  %s\n", config->osaf);

	printf("  [tcp_plms_listening_port]\n");
	printf("  %s\n", config->tcp_plms_listening_port);

	printf("  [udp_broadcast_port]\n");
	printf("  %s\n", config->udp_broadcast_port);

	printf("  [os_type]\n");
	printf("  %s\n", config->os_type);

	printf("  [plmc_cmd_timeout_secs]\n");
	printf("  %d\n", config->plmc_cmd_timeout_secs);

	printf("  [os_reboot_cmd]\n");
	printf("  %s\n", config->os_reboot_cmd);

	printf("  [os_shutdown_cmd]\n");
	printf("  %s\n", config->os_shutdown_cmd);

	printf("  [so_keepalive]\n");
	printf("  %d\n", config->so_keepalive);

	printf("  [tcp_keepidle_time]\n");
	printf("  %d\n", config->tcp_keepidle_time);

	printf("  [tcp_keepalive_intvl]\n");
	printf("  %d\n", config->tcp_keepalive_intvl);

	printf("  [tcp_keepalive_probes]\n");
	printf("  %d\n", config->tcp_keepalive_probes);

	printf("\n");
}

