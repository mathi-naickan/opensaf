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

  H&J InterProcess Communication Facility
  -------------------------------------
  Interprocess and Intraprocess Event Posting.  We use this for posting events
  (received pdu's, API requests) at sub-system  boundaries.  It is also used
  for timer expirations, and posting of events within subsystems.

  ******************************************************************************
  */

/** Module Inclusion Control...
 **/
#ifndef BASE_NCSSYSF_IPC_H_
#define BASE_NCSSYSF_IPC_H_

#include "base/ncs_osprm.h"
#include "base/ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

/** H&J SYSF_MBX Message Element
 **/
typedef uint32_t SYSF_MBX;

/** Prototype for "leave on queue" callback functions.  See description of
 ** m_NCS_IPC_FLUSH below for more information.
 **/
typedef bool (*NCS_IPC_CB) (void *arg, void *msg);

/** Enumerated values for IPC message priorities.
 **/
typedef enum ncs_ipc_priority {
  NCS_IPC_PRIORITY_LOW = 1,
  NCS_IPC_PRIORITY_NORMAL,
  NCS_IPC_PRIORITY_HIGH,
  NCS_IPC_PRIORITY_VERY_HIGH,
  NCS_IPC_PRIORITY_MAX,
} NCS_IPC_PRIORITY;

#define NCS_IPC_PRIO_LEVELS 4

/** structures for maintaining IPC related items...
 ** Note: this queue element exists to force the requirement that the first
 ** field of the object to be queued must be "next" (void *).
 **/
typedef struct ncs_ipc_msg {
  struct ncs_ipc_msg *next;
} NCS_IPC_MSG;

/****************************************************************************
 * m_NCS_IPC_CREATE
 *
 * This macro is invoked in order to allocate and/or initialize an IPC
 * mailbox.  Upon return from this macro, "p_mbx" should be dereferenced
 * and the allocated SYSF_MBX should be stored in *p_mbx.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if IPC mailbox created and initialized successfully.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_CREATE(p_mbx)               ncs_ipc_create(p_mbx)

/****************************************************************************
 * m_NCS_IPC_RELEASE
 *
 * This macro is invoked in order to cease use of an IPC mailbox.  In the
 * current implementation, ncs_ipc_release() locks the IPC queue, sets the
 * "releasing" flag, flushes the queue, and wakes up the receiver.  Then,
 * ncs_ipc_recv() "sees" the "releasing" flag is true, and does the actual
 * work to free the resources "owned" by the IPC mailbox.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "callback" is a pointer to a subroutine of prototype NCS_IPC_CB.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if IPC mailbox use successfully ceased.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_RELEASE(p_mbx, callback)      \
  ncs_ipc_release(p_mbx, callback)

/****************************************************************************
 * m_NCS_IPC_ATTACH
 *
 * This macro is invoked in order to attach to an IPC mailbox. This means
 * that a reference count will be incremented.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if IPC mailbox successfully attached.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_ATTACH(p_mbx)    ncs_ipc_attach(p_mbx)

/****************************************************************************
 * m_NCS_IPC_ATTACH_EXT
 *
 * This macro is invoked in order to attach to an IPC mailbox. This means
 * that a reference count will be incremented.  The IPC mailbox will be
 * associated with a task identiified by the task_name.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "task_name" is a task name.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if IPC mailbox successfully attached.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_ATTACH_EXT(p_mbx,task_name)    ncs_ipc_attach_ext(p_mbx,task_name)

/****************************************************************************
 * m_NCS_IPC_DETACH
 *
 * This macro is invoked in order to detach from an IPC mailbox. The
 * "callback" routine is H&J subsystem specific, i.e., there is a separate
 * "callback" routine for IPOA, LEC, Signalling, etc.  The "callback" routine
 * is passed a pointer to the message and a "context".  The "callback"
 * routine should inspect the message and determine if it pertains to the
 * "context", and if so, free the message and any resources contained within
 * and then return true, meaning 'remove from queue'.  If the "context"
 * is NULL, then ALL messages should be freed and false should be returned
 * for ALL messages.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "callback" is a pointer to a subroutine of prototype NCS_IPC_CB.
 * "context" is a context of type (void *).
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if IPC mailbox successfully detached.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_DETACH(p_mbx, callback, context)      \
  ncs_ipc_detach(p_mbx, callback, context)

/****************************************************************************
 * m_NCS_IPC_RECEIVE
 *
 * This macro is invoked in order to receive the next message from an IPC
 * mailbox.  This macro should return a void pointer to the message at the
 * "head" of the IPC mailbox.  It should block, waiting for a message to
 * be posted to the IPC mailbox if the mailbox is empty.  A return value
 * of NULL tells the caller to exit the thread.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "messagebuf" is an on-stack structure for storage of the message in case
 *              the IPC mechanism must copy the message into a subystem
 *              owned storage area.  This argument is unused in our
 *              implementation.
 *
 * RETURNS:
 *
 * void pointer to the message at the head of the IPC mailbox.
 * NULL if the IPC mailbox has been released.
 */
#define m_NCS_IPC_RECEIVE(p_mbx, messagebuf)  ncs_ipc_recv(p_mbx)

/****************************************************************************
 * m_NCS_IPC_NON_BLK_RECEIVE
 *
 * This macro is invoked in order to receive the next message from an IPC
 * mailbox.  This macro should return a void pointer to the message at the
 * "head" of the IPC mailbox.  It returns NULL if it dosen't have any message
 * in the queue.
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "messagebuf" is an on-stack structure for storage of the message in case
 *              the IPC mechanism must copy the message into a subystem
 *              owned storage area.  This argument is unused in our
 *              implementation.
 *
 * RETURNS:
 *
 * void pointer to the message at the head of the IPC mailbox.
 * NULL if there is no messages in IPC mailbox.
 */
#define m_NCS_IPC_NON_BLK_RECEIVE(p_mbx, messagebuf)  ncs_ipc_non_blk_recv(p_mbx)
#if 0                           /* The following macro don't seem to be getting used anywhere:PM */
#define m_NCS_IPC_NON_BLK_SEND(p_mbx, msg, prio)        \
  ncs_ipc_non_blk_send(p_mbx, (NCS_IPC_MSG *)msg, prio)
#define m_NCS_IPC_NON_BLK_MAX_MSG_PROC  (10)
#endif

/****************************************************************************
 * m_NCS_IPC_GET_SEL_OBJ
 *
 * This macro is invoked in order to extract the NCS_SEL_OBJ stored
 * in the mailbox. Note that NCS_SEL_OBJ replaces semaphores in the
 * modified implementation of NCS_IPC under NCS. 27/Feb/2004
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 *
 * RETURNS:
 *
 *  An  NCS_SEL_OBJ structure.
 *
 * EXAMPLE:
 *  -------------
 *   NCS_SEL_OBJ  sel_obj;
 *   SYSF_MBX     *mbx;
 *   ...
 *   sel_obj = m_NCS_IPC_GET_SEL_OBJ(mbx);
 *  -------------
 *
 * Modified macro definition to use a function call as NCS_IPC definition
 * is abtracted from users. Therefore, a function call is used to prevent
 * the need to expose NCS_IPC definition.
 *
 *
 */
#define m_NCS_IPC_GET_SEL_OBJ(p_mbx)  (ncs_ipc_get_sel_obj(p_mbx))

/****************************************************************************
 * m_NCS_IPC_SEND
 *
 * This macro is invoked in order to send/post a message to an IPC mailbox.
 * This macro should return value of NCSCC_RC_SUCCESS if the message is
 * successfully posted to the IPC mailbox, else an error return such as
 * NCSCC_RC_FAILURE.
 *
 * Note:
 * if message count of this priority level queue execeds threshold limit then
 * drop that message with out posting into mail box
 * if threshold limit for this priority level queue is set to Zero then
 * above check will be disabled.
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "msg" is a pointer to a message of subsystem specific type.
 * "prio" is the relative priority of the message of type NCS_IPC_PRIORITY
 *       If message count of the queue at this priority level execeds threshold
 *       limit then this message is not enqueued into mailbox.By default the
 *       threshold is "infinite" and can be set using the (optional)
 *       m_NCS_IPC_CONFIG_MAX_MSGS
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS  if message is successfully posted to the IPC mailbox.
 * <error return>   otherwise (such as NCSCC_RC_FAILURE).
 */
#define m_NCS_IPC_SEND(p_mbx, msg, prio)        \
  ncs_ipc_send(p_mbx, (NCS_IPC_MSG *)msg, prio)

/* H&J base code directly ref's this macro, but has been obsoleted, so leave
 * as no definition!
 */
#define m_SYSF_RELEASE_MBX(mbx)

/*******************************************************************************
 * m_NCS_IPC_CONFIG_MAX_MSGS
 *
 *"This macro is invoked in order to set a limit on the maximum number of messages that
 * can be queued at a certain priority level in a LEAP mailbox. Unless such a threshold
 * is set, there is no limit on number of messages that can be queued. See description
 * of m_NCS_IPC_SEND() for the behaviour when number of message in the queue reaches the
 * threshold limit."
 *
 * ARGUMENTS:
 *
 * "p_mbx" is a pointer to a SYSF_MBX.
 * "prio"  relative priority  of type NCS_IPC_PRIORITY
 * "max_limit" Maximum  number of messages i.e threshold limit on messages
 *   .If max limit is not configured for a priority type then its threshold will
 *    be set to zero by default.
 *   .If threshold limit is zero then in ncs_ipc_send, Check for msg-count
 *    execeds the threshold for this priority level will be disabled.
 *
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS:  if threshold values are set successfully .
 *  NCSCC_RC_FAILURE:
 *         . Invalid input .
 *         . Invalid mailbox handle.
 *
 *******************************************************************************/

#define m_NCS_IPC_CONFIG_MAX_MSGS(p_mbx,prio,max_limit)  ncs_ipc_config_max_msgs(p_mbx, prio, max_limit)

/*******************************************************************************
 * m_NCS_IPC_CONFIG_USR_COUNTERS
 *
 * "This macro allows a user to supply the address of a 32-bit counter to track
 *  the number of messages lying in LEAP mailbox queues.  The LEAP mailbox
 *  operations will internally update this 32-bit counter whenever messages
 *  are enqueued or dequeued from the LEAP mailbox. This API should be invoked
 *  only after a LEAP mailbox has been created. Invocation of this API is, however,
 *  optional. It is possible to configure one counter per priority queue of a LEAP
 *  mailbox. The LEAP user should never modify this variable contents on his own.
 *  Furthermore, the user should note that while this variable is being accessed
 *  by user code, it may be undergoing updation in another thread-context due to
 *  mailbox send-receive operations
 *
 * ARGUMENTS:
 *
 * "i_p_mbx" is a pointer to a SYSF_MBX.
 * "i_prio"  relative priority  of type NCS_IPC_PRIORITY
 * "i_usr_counter" is a pointer to uint32_t
 *               Holds the address of uint32_t user counter
 * RETURNS:
 *
 * NCSCC_RC_SUCCESS:  if usr_counter addresses are set Successfully  .
 *  NCSCC_RC_FAILURE:
 *         . Invalid input .
 *         . Invalid mailbox handle.
 *
 *******************************************************************************/

#define m_NCS_IPC_CONFIG_USR_COUNTERS(i_p_mbx,i_prio,i_usr_counter)  ncs_ipc_config_usr_counters(i_p_mbx,i_prio,i_usr_counter)
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  FUNCTION PROTOTYPES

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

    uint32_t ncs_ipc_create(SYSF_MBX *mbx);
uint32_t ncs_ipc_release(SYSF_MBX *mbx, NCS_IPC_CB cb);
NCS_SEL_OBJ ncs_ipc_get_sel_obj(SYSF_MBX *mbx);
uint32_t ncs_ipc_attach(SYSF_MBX *mbx);
uint32_t ncs_ipc_attach_ext(SYSF_MBX *mbx, char *task_name);
uint32_t ncs_ipc_detach(SYSF_MBX *mbx, NCS_IPC_CB cb, void *cb_arg);
NCS_IPC_MSG *ncs_ipc_recv(SYSF_MBX *mbx);
uint32_t ncs_ipc_send(SYSF_MBX *mbx, NCS_IPC_MSG *msg, NCS_IPC_PRIORITY prio);
NCS_IPC_MSG *ncs_ipc_non_blk_recv(SYSF_MBX *mbx);
uint32_t ncs_ipc_non_blk_send(SYSF_MBX *mbx, NCS_IPC_MSG *msg, NCS_IPC_PRIORITY prio);
uint32_t ncs_ipc_config_max_msgs(SYSF_MBX *mbx, NCS_IPC_PRIORITY prio, uint32_t max_limit);
uint32_t ncs_ipc_config_usr_counters(SYSF_MBX *i_mbx, NCS_IPC_PRIORITY i_prio,
                                     uint32_t *i_usr_counter);
#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSF_IPC_H_
