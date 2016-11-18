/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This module contains routines related to module execution.
..............................................................................

  FUNCTIONS INCLUDED in this module:

 ******************************************************************************
 */

#include "base/sysf_exc_scr.h"
#include "base/ncssysf_def.h"

#include <sched.h>

SYSF_EXECUTE_MODULE_CB module_cb;

/***************************************************************************** 

  PROCEDURE        : ncs_exc_mdl_start_timer

  DESCRIPTION:       This function is used to start a timer 

  ARGUMENTS:

  RETURNS:           Nothing.

  NOTES:

*****************************************************************************/
void ncs_exc_mdl_start_timer(SYSF_PID_LIST *exec_pid)
{
	/* Timer does not exist, create it. */
	m_NCS_TMR_CREATE(exec_pid->tmr_id,
			 (exec_pid->timeout_in_ms / 10), ncs_exec_module_timer_hdlr, (void *)exec_pid->pid);

	/*  Now Start the timer. */
	m_NCS_TMR_START(exec_pid->tmr_id,
			(exec_pid->timeout_in_ms / 10),
			ncs_exec_module_timer_hdlr, NCS_INT32_TO_PTR_CAST(exec_pid->pid));
}

/***************************************************************************** 
                                                                              
  PROCEDURE          :    ncs_exc_mdl_stop_timer                                          
                                                                               
  DESCRIPTION:       This function is used to stop a timer 
                                                                               
  ARGUMENTS:                                                                   
                                                                               
  RETURNS:           Nothing.                                                 
                                                                               
  NOTES:                                                                
                                                                               
*****************************************************************************/
void ncs_exc_mdl_stop_timer(SYSF_PID_LIST *exec_pid)
{
	m_NCS_TMR_STOP(exec_pid->tmr_id);

	m_NCS_TMR_DESTROY(exec_pid->tmr_id);
	exec_pid->tmr_id = NULL;

}

/**************************************************************************\
 *
 * ncs_exec_module_signal_hdlr
 *
 * Description: Signal handler for the child process executing script..
 *
 * Synopsis:
 *
 * Call Arguments:
 *   Signal - Signal for which signal handler is called..
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
void ncs_exec_module_signal_hdlr(int signal)
{
	if (signal == SIGCHLD) {
		EXEC_MOD_INFO info = {
			.pid = 0,
			.status = 0,
			.type = SYSF_EXEC_INFO_SIG_CHLD
		};

		/*  printf("\n In  SIGCHLD Handler \n"); */

		if(-1 == write(module_cb.write_fd, (const void *)&info,
					sizeof(EXEC_MOD_INFO))){
			perror("ncs_exec_module_signal_hdlr: write");
		}
		
	}

}

/**************************************************************************\
 *
 * ncs_exec_module_timer_hdlr
 *
 * Description: Timer handler for the child process executing script.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   pid - Process ID for which timeout has occurrend.
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
void ncs_exec_module_timer_hdlr(void *uarg)
{
	EXEC_MOD_INFO info = {
		.pid = NCS_PTR_TO_INT32_CAST(uarg),
		.status = 0,
		.type = SYSF_EXEC_INFO_TIME_OUT
	};

	if(-1 == write(module_cb.write_fd, (const void *)&info,
				sizeof(EXEC_MOD_INFO))){
		perror("ncs_exec_module_timer_hdlr: write");
	} 
}

/**************************************************************************\
 *
 * ncs_exec_mod_hdlr
 *
 * Description: Task to handle the signals.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   pid - Process ID for which timeout has occurrend.
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
void ncs_exec_mod_hdlr(void)
{
	EXEC_MOD_INFO info;
	uint32_t maxsize = sizeof(EXEC_MOD_INFO);
	int count = 0, ret_val = 0;
	SYSF_PID_LIST *exec_pid = NULL;
	int status = -1;
	int pid = -1;

	while (1) {
		while ((ret_val = read(module_cb.read_fd, (((uint8_t *)&info) + count),
				       (maxsize - count))) != (maxsize - count)) {
			if (ret_val <= 0) {
				if (errno == EBADF)
					return;

				perror("ncs_exec_mod_hdlr: read fail:");
				continue;
			}
			count += ret_val;
		}		/* while */

		if (info.type == SYSF_EXEC_INFO_TIME_OUT) {
			/* printf("Time out signal \n"); */
			pid = info.pid;
			give_exec_mod_cb(info.pid, info.status, info.type);

		} /* if */
		else {
 repeat_srch_from_beginning:
			m_NCS_LOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

			for (exec_pid = (SYSF_PID_LIST *)ncs_patricia_tree_getnext(&module_cb.pid_list, NULL);
			     exec_pid != NULL;
			     exec_pid =
			     (SYSF_PID_LIST *)ncs_patricia_tree_getnext(&module_cb.pid_list,
									(const uint8_t *)&exec_pid->pid)) {
				pid = exec_pid->pid;
				/*printf(" Going to wait on waitpid  %d \n", pid); */

				if ((pid == waitpid(pid, &status, WNOHANG))) {
					/* TIMED OUT CHILDS which are terminated by sending  SIG CHILD */
					if (exec_pid->exec_info_type == SYSF_EXEC_INFO_TIME_OUT) {
						ncs_patricia_tree_del(&module_cb.pid_list,
								      (NCS_PATRICIA_NODE *)exec_pid);

						free(exec_pid);
						m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

					} else {
						info.pid = pid;
						info.status = status;
						info.type = SYSF_EXEC_INFO_SIG_CHLD;
						m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
						give_exec_mod_cb(info.pid, info.status, info.type);
					}
					goto repeat_srch_from_beginning;
				}
			}	/*for */
			m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
		}		/* else */
		count = 0;
	}			/* while(1) */
}

/**************************************************************************\
 *
 * give_exec_mod_cb
 *
 * Description: Task to handle the signals.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   pid - Process ID for which timeout has occurrend.
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
void give_exec_mod_cb(int pid, uint32_t status, int type)
{
	NCS_OS_PROC_EXECUTE_TIMED_CB_INFO cb_info;
	SYSF_PID_LIST *exec_pid;

	m_NCS_LOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

	memset(&cb_info, '\0', sizeof(NCS_OS_PROC_EXECUTE_TIMED_CB_INFO));

	if (NULL != (exec_pid = (SYSF_PID_LIST *)ncs_patricia_tree_get(&module_cb.pid_list, (const uint8_t *)&pid))) {

		if (SYSF_EXEC_INFO_TIME_OUT == type) {
			cb_info.exec_stat.value = NCS_OS_PROC_EXIT_WAIT_TIMEOUT;
			kill(exec_pid->pid, SIGKILL);
			exec_pid->exec_info_type = SYSF_EXEC_INFO_TIME_OUT;
			cb_info.exec_stat.info.exit_with_code.exit_code = WEXITSTATUS(status);
		} else {

			/* Initialize the exit-code value. May be overridden below */
			cb_info.exec_stat.info.exit_with_code.exit_code = WEXITSTATUS(status);

			/* First stop timer */
			exec_pid->exec_info_type = SYSF_EXEC_INFO_SIG_CHLD;
			ncs_exc_mdl_stop_timer(exec_pid);

			/* Earlier "status = status >>8" now replaced with WEXITSTATUS macro */
			if (WIFEXITED(status) && (WEXITSTATUS(status) == 128)) {
				cb_info.exec_stat.value = NCS_OS_PROC_EXEC_FAIL;
			} else if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
				cb_info.exec_stat.value = NCS_OS_PROC_EXIT_NORMAL;
			} else if (WIFSIGNALED(status)) {
				cb_info.exec_stat.value = NCS_OS_PROC_EXIT_ON_SIGNAL;
				cb_info.exec_stat.info.exit_on_signal.signal_num = WTERMSIG(status);
			} else {	/* Just consider it to tbe EXIT-WITH-CODE .... */

				cb_info.exec_stat.value = NCS_OS_PROC_EXIT_WITH_CODE;
			}
		}

		cb_info.i_usr_hdl = exec_pid->usr_hdl;
		cb_info.i_exec_hdl = exec_pid->exec_hdl;

		exec_pid->exec_cb(&cb_info);
		if (type != SYSF_EXEC_INFO_TIME_OUT) {

			/* Remove entry from pat tree */
			ncs_patricia_tree_del(&module_cb.pid_list, (NCS_PATRICIA_NODE *)exec_pid);

			free(exec_pid);
		}
	}
	m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
}

/**************************************************************************\
 *
 * add_new_req_pid_in_list
 *
 * Description: Add new request in the list..
 *
 * Synopsis:
 *
 * Call Arguments:
 *   pid - Process ID for which timeout has occurrend.
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
uint32_t add_new_req_pid_in_list(NCS_OS_PROC_EXECUTE_TIMED_INFO *req, uint32_t pid)
{
	SYSF_PID_LIST *list_entry;

	if (module_cb.init == false)
		return m_LEAP_DBG_SINK(NCSCC_RC_SUCCESS);

	if (NULL == (list_entry = (SYSF_PID_LIST *) malloc(sizeof(SYSF_PID_LIST))))
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	list_entry->timeout_in_ms = req->i_timeout_in_ms;
	list_entry->exec_cb = req->i_cb;
	list_entry->usr_hdl = req->i_usr_hdl;
	list_entry->exec_hdl = req->o_exec_hdl = NCS_PTR_TO_UNS64_CAST(list_entry);
	list_entry->pid = pid;
	list_entry->pat_node.key_info = (uint8_t *)&list_entry->pid;
	list_entry->exec_info_type = SYSF_EXEC_INFO_SIG_CHLD;

	m_NCS_LOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

	if (NCSCC_RC_SUCCESS != ncs_patricia_tree_add(&module_cb.pid_list, (NCS_PATRICIA_NODE *)list_entry)) {
		free(list_entry);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	ncs_exc_mdl_start_timer(list_entry);

	m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 *
 * init_exec_mod_cb
 *
 * Description: Initialize execute module control block lock.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   SUCCESS/FAILURE
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
uint32_t init_exec_mod_cb(void)
{
	memset(&module_cb, '\0', sizeof(SYSF_EXECUTE_MODULE_CB));
	m_NCS_LOCK_INIT(&module_cb.tree_lock);
	return NCSCC_RC_SUCCESS;
}

/**************************************************************************\
 *
 * start_exec_mod_cb
 *
 * Description: Initialize execute module control block.
 *
 * Synopsis:
 *
 * Call Arguments:
 *   SUCCESS/FAILURE
 *
 * Returns:
 *   None.
 *
 * Notes:
 *
\**************************************************************************/
uint32_t start_exec_mod_cb(void)
{
	NCS_PATRICIA_PARAMS pt_params = { .key_size = sizeof(uint32_t) };
	int spair[2];

	if (ncs_patricia_tree_init(&module_cb.pid_list, &pt_params) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	if (0 != socketpair(AF_UNIX, SOCK_DGRAM, 0, spair)) {
		perror("init_exec_mod_cb: socketpair: ");
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	module_cb.read_fd = spair[0];
	module_cb.write_fd = spair[1];

	/* Create a task which will handle the signal and give call back  */

	int policy = SCHED_OTHER; /*root defaults */
	int prio_val = sched_get_priority_min(policy);

	if (m_NCS_TASK_CREATE((NCS_OS_CB)ncs_exec_mod_hdlr,
			      0,
			      (char *)"OSAF_EXEC_MOD",
			      prio_val, policy,
			      NCS_EXEC_MOD_STACKSIZE, &module_cb.em_task_handle) != NCSCC_RC_SUCCESS) {
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);;
	}

	if (m_NCS_TASK_START(module_cb.em_task_handle) != NCSCC_RC_SUCCESS) {
		m_NCS_TASK_RELEASE(module_cb.em_task_handle);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);;
	}

	module_cb.init = true;

	signal(SIGCHLD, ncs_exec_module_signal_hdlr);

	return NCSCC_RC_SUCCESS;

}

/**************************************************************************\
 * exec_mod_cb_destroy
 *
 * Description       : Destroys module control block.
 *              
 * Call Arguments    : None  
 *
 * Returns:

 * SUCCESS/FAILURE   : NCSCC_RC_SUCCESS / NCSCC_RC_FAILURE 
 *
 * Notes:
 *
\**************************************************************************/
uint32_t exec_mod_cb_destroy(void)
{
	uint8_t pid = 0;

	if (module_cb.init == true) {
		module_cb.init = false;
		signal(SIGCHLD, SIG_DFL);

		close(module_cb.write_fd);
		close(module_cb.read_fd);

		m_NCS_TASK_RELEASE(module_cb.em_task_handle);

		m_NCS_LOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);

		SYSF_PID_LIST* exec_pid;
		while (NULL != (exec_pid = (SYSF_PID_LIST *)ncs_patricia_tree_getnext(&module_cb.pid_list,
										      (const uint8_t *)&pid))) {

			ncs_patricia_tree_del(&module_cb.pid_list, (NCS_PATRICIA_NODE *)exec_pid);

			if (exec_pid->tmr_id != NULL)
				m_NCS_TMR_DESTROY(exec_pid->tmr_id);

			free(exec_pid);
		}

		if (ncs_patricia_tree_destroy(&module_cb.pid_list) != NCSCC_RC_SUCCESS) {
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		}

		m_NCS_UNLOCK(&module_cb.tree_lock, NCS_LOCK_WRITE);
	}

	m_NCS_LOCK_DESTROY(&module_cb.tree_lock);

	return NCSCC_RC_SUCCESS;
}
