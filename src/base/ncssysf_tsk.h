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

  H&J Tasking Facility.

  ******************************************************************************
  */

/** Module Inclusion Control...
 **/
#ifndef BASE_NCSSYSF_TSK_H_
#define BASE_NCSSYSF_TSK_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"

extern uint32_t ncs_task_join(void *task_handle);

#ifdef  __cplusplus
extern "C" {
#endif

/** This typedef is being maintained for backward compatibility
 ** with ech.
 **/
typedef void *SYSF_THREAD_CB;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  H&J Tasking Definitions

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/****************************************************************************
 * m_NCS_TASK_CREATE
 *
 * This macro is invoked in order to allocate and/or initialize a task
 * thread.  Upon return from this macro, "p_t_handle" should be dereferenced
 * and the handle of the allocated task thread should be stored in *p_t_handle.
 * The task should be created but should not be running (ready).
 *
 * ARGUMENTS:
 *
 * "entry" is the task entry point (NCS_OS_CB).
 * "arg" is the argument that should be passed to "entry" upon startup (void *)
 * "name" is the task name (char *).
 * "prio" is the tasks priority (unsigned int).
 * "stack_size" is the size of the stack in bytes (unsigned int).
 * "p_t_handle" is a pointer to a task handle (void **).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task created and initialized successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_CREATE(entry, arg, name, prio, policy, stack_size, p_t_handle) \
  ncs_task_create(entry, arg,                                           \
                  name, prio, policy,                                   \
                  stack_size,                                           \
                  p_t_handle)

/****************************************************************************
 * m_NCS_TASK_RELEASE
 *
 * This macro is invoked in order to release a task thread.  When this macro
 * is invoked, the task should already have been stopped.
 *
 * ARGUMENTS:
 *
 * "t_handle" is a task handle (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task released successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_RELEASE(t_handle)           ncs_task_release(t_handle)

/****************************************************************************
 * m_NCS_TASK_JOIN
 *
 *  This macro is invoked in order to suspend the execution of
 *   the calling thread until the thread identified by `task_handle` terminates.
 * ARGUMENTS:
 *
 * "t_handle" is a task handle (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task Joins successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_JOIN(t_handle)           ncs_task_join(t_handle)

/****************************************************************************
 * m_NCS_TASK_DETACH
 *
 * This macro is invoked in order to make a task thread detachable
 *
 * ARGUMENTS:
 *
 * "t_handle" is a task handle (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task made detachable successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_DETACH(t_handle)             ncs_task_detach(t_handle)

/****************************************************************************
 * m_NCS_TASK_START
 *
 * This macro is invoked in order to start a task thread running, i.e., add
 * it to the "ready" list.
 *
 * ARGUMENTS:
 *
 * "t_handle" is a task handle (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task started successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_START(t_handle)             ncs_task_start(t_handle)

/****************************************************************************
 * m_NCS_TASK_STOP
 *
 * This macro is invoked in order to stop a task thread running, i.e., remove
 * it from the "ready" list.
 *
 * ARGUMENTS:
 *
 * "t_handle" is a task handle (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task stopped successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_STOP(t_handle)              ncs_task_stop(t_handle)

/****************************************************************************
 * m_NCS_TASK_SLEEP
 *
 * This macro is invoked in order to stop a task thread running for a specified
 * number of milliseconds.
 *
 * ARGUMENTS:
 *
 * "delay_ms" is the number of milliseconds to stop the task for.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task slept successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_SLEEP(delay_ms)   ncs_task_sleep(delay_ms)

/****************************************************************************
 * m_NCS_TASK_CURRENT
 *
 * Obtains the current task handle.
 * Upon return from this macro, "p_t_handle" should be dereferenced
 * and the handle of the current task should be stored in *p_t_handle.
 *
 * ARGUMENTS:
 *
 * "p_t_handle" is a pointer to a task handle (void **).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if task created and initialized successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_TASK_CURRENT(p_t_handle) ncs_task_current(p_t_handle)

    /*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

      FUNCTION PROTOTYPES

      @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

uint32_t ncs_task_create(NCS_OS_CB, void *, const char *, unsigned int, int, unsigned int, void **);
uint32_t ncs_task_release(void *);
uint32_t ncs_task_detach(void *);
uint32_t ncs_task_start(void *);
uint32_t ncs_task_stop(void *);
uint32_t ncs_task_sleep(unsigned int);
uint32_t ncs_task_current(void **);
int ncs_task_entry(NCS_OS_TASK *task);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSF_TSK_H_
