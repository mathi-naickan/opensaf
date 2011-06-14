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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <plmc_lib.h>

char clear[] = "/usr/bin/clear";

void print_option_menu(char *ee_id, char *type, char *msg, char *cmd, int myenum)
{	
	system(clear);
	printf("\n********************************************************************************\n");
	printf("*                                Pick an Option                                *\n");
	printf("*                                --------------                                *\n");
	printf("********************************************************************************\n");
	printf("*     1  - REFRESH                                                             *\n");
	printf("*     2  - PLMC_GET_PROTOCOL_VER                                               *\n");
	printf("*     3  - PLMC_GET_OSINFO                                                     *\n");
	printf("*     4  - PLMC_SA_PLM_ADMIN_UNLOCK                                            *\n");
	printf("*     5  - PLMC_SA_PLM_ADMIN_LOCK_INSTANTIATION                                *\n");
	printf("*     6  - PLMC_SA_PLM_ADMIN_LOCK                                              *\n"); 
	printf("*     7  - PLMC_SA_PLM_ADMIN_RESTART                                           *\n");
	printf("*     8  - PLMC_OSAF_START                                                     *\n");
	printf("*     9  - PLMC_OSAF_STOP                                                      *\n");
	printf("*     10 - PLMC_OSAF_SERVICES_START                                            *\n");
	printf("*     11 - PLMC_OSAF_SERVICES_STOP                                             *\n");
	printf("*     12 - PLMC_PLMCD_RESTART                                                  *\n");
	printf("*     13 - EXIT                                                                *\n");
	printf("********************************************************************************\n");
	printf("*     ee_id: %s      Type: %s     \n", ee_id, type);
	if (cmd != NULL){
		printf("*     Command sent:     %s  \n", cmd);
		printf("*     Result is:       %s  \n", msg);
	} else {
		printf("*     Message: %s  \n", msg);
	}
	if (strcmp(type, "Command Callback") == 0)
		printf("*     CMD ENUM: %d  \n", myenum);
	if (strcmp(type, "UDP") == 0)
		printf("*     MSG ENUM: %d  \n", myenum);
	printf("********************************************************************************\n");
}

int connect_callback(char *eeid, char *status)
{
	print_option_menu(eeid, "Connect / Disconnect", status, NULL, 0);
	return 0;
}

int error_callback(plmc_lib_error *err)
{
	char *message;
	
	message = calloc(1, strlen(err->errormsg) + strlen(err->action) +
			strlen("%s. %s. \n*      CMD ENUM: %d") + 3);
	sprintf(message, "%s. %s. \n*     CMD ENUM: %d", err->errormsg, err->action, err->cmd_enum);
	print_option_menu("  	", "Error ", message, NULL, 0);
	free(message);
	return 0;
}

int udp_callback(udp_msg *status)
{
	print_option_menu(status->ee_id, "UDP", status->msg, NULL, status->msg_idx);
	return 0;
}


int command_callback(tcp_msg *tcpmsg)
{
	print_option_menu(tcpmsg->ee_id, "Command Callback", tcpmsg->result, tcpmsg->cmd, tcpmsg->cmd_enum);
	return 0;
}

void print_ee_menu()
{	
	printf("EE_ID: \n");
}

void print_error()
{
	char dummy;
	printf("*                                Error in Input                                *\n");
	printf("*                                --------------                                *\n");
	printf("Press any ket to get back \n");
	dummy = getchar();
	
}


int main ()
{
	char cmd[10], ee_id[36];
	int number, len;
	if(plmc_initialize(connect_callback, udp_callback, error_callback) != 0) {
		printf("Could not initialize library\n");
		exit(1);
	}
	print_option_menu("         ", "         ", "                   ", NULL, 0);
	while(1) {
		fgets(cmd, 10, stdin);
		number = atoi(cmd);
		switch (number) {
		case 1:
			print_option_menu("         ", "         ", "     ", NULL, 0);
			break;
		case 2:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_get_protocol_ver(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 3:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_get_os_info(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 4:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_sa_plm_admin_unlock(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 5:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_sa_plm_admin_lock_instantiation(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 6:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_sa_plm_admin_lock(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 7:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_sa_plm_admin_restart(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 8:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_osaf_start(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 9:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_osaf_stop(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 10:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_osaf_services_start(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 11:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_osaf_services_stop(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 12:
			print_ee_menu();
			fgets(ee_id, 36, stdin);
			len = strlen(ee_id);
			ee_id[len-1] = '\0';
			if(plmc_plmcd_restart(ee_id, command_callback) != 0) {
				printf("Operation failed due to wrong EE_ID\n");
			}
			break;
		case 13:
			plmc_destroy();
			pthread_exit(NULL);
			break;
		default : 
			print_error();
			print_option_menu("         ", "         ", "     ", NULL, 0);

		}
	}
}
