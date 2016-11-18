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

  This module contains declarations related to the target system Timer Services.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_SYSF_EXC_SCR_H_
#define BASE_SYSF_EXC_SCR_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/os_defs.h"
#include "base/ncssysf_def.h"
#include "base/ncspatricia.h"
#include "base/ncssysf_tmr.h"
#include "signal.h"
#include "base/ncssysf_tsk.h"
#include "base/ncssysf_lck.h"
#include "base/ncssysfpool.h"
/* Fix Ends */

typedef enum sysf_exec_info_type {
  SYSF_EXEC_INFO_SIG_CHLD,
  SYSF_EXEC_INFO_TIME_OUT
} SYSF_EXEC_INFO_TYPE;

typedef struct sysf_pid_list {
  NCS_PATRICIA_NODE pat_node;
  uint32_t pid;

  NCS_EXEC_USR_HDL usr_hdl;
  NCS_OS_PROC_EXECUTE_CB exec_cb;
  NCS_EXEC_HDL exec_hdl;

  /* Timer Params */
  tmr_t tmr_id;
  int64_t timeout_in_ms;
  int exec_info_type;

} SYSF_PID_LIST;

typedef struct exec_mod_info {
  int pid;
  int status;
  int type;
} EXEC_MOD_INFO;

typedef struct sysf_execute_module_cb {

  NCS_LOCK tree_lock;
  NCSCONTEXT em_task_handle;
  NCS_PATRICIA_TREE pid_list;
  int read_fd;
  int write_fd;
  bool init;
} SYSF_EXECUTE_MODULE_CB;


#ifndef NCS_EXEC_MOD_STACKSIZE
#define NCS_EXEC_MOD_STACKSIZE     NCS_STACKSIZE_HUGE
#endif

extern SYSF_EXECUTE_MODULE_CB module_cb;

extern void ncs_exc_mdl_start_timer(SYSF_PID_LIST *exec_pid);
extern void ncs_exc_mdl_stop_timer(SYSF_PID_LIST *exec_pid);
extern void ncs_exec_module_signal_hdlr(int signal);
extern void ncs_exec_module_timer_hdlr(void *uarg);
extern void ncs_exec_mod_hdlr(void);
extern uint32_t add_new_req_pid_in_list(NCS_OS_PROC_EXECUTE_TIMED_INFO *req, uint32_t pid);
extern uint32_t init_exec_mod_cb(void);
extern uint32_t start_exec_mod_cb(void);
extern uint32_t exec_mod_cb_destroy(void);
extern void give_exec_mod_cb(int pid, uint32_t stat, int type);

#endif  // BASE_SYSF_EXC_SCR_H_
