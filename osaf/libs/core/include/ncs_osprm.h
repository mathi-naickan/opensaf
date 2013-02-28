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

  MODULE NAME:  OS_PRIMS.H

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  04/22/98  1.0     Bellucci      Original

  -------- -------  ------------  --------------------------------------------

..............................................................................

  DESCRIPTION:

  This module contains declarations for Operating System Interfaces.

  CONTENTS:

    m_NCS_OS_MEMALLOC( nbytes ) ............. memory allocation interface
    m_NCS_OS_MEMFREE( mem_p ) ............... memory free interface
    m_NCS_OS_TIMER( pncs_timer, request ) .... to request timer services
    m_NCS_OS_IPC( pncs_ipc, request ) ........ to request IPC services
    m_NCS_OS_SEM( pncs_sem, request ) ........ to request Semaphore services
    m_NCS_OS_TASK( pncs_task, request ) ...... to request task services
    m_NCS_OS_LOCK( pncs_lock, request,type ) . to request lock services
    m_NCS_OS_INIT_TASK_LOCK ..................to init for a task lock
    m_NCS_OS_START_TASK_LOCK .................to start task lock
    m_NCS_OS_END_TASK_LOCK ...................to stop task lock
    m_NCS_OS_ATOMIC_INC( p_uint32_t ) .......... atomic increment
    m_NCS_OS_ATOMIC_DEC( p_uint32_t ) .......... atomic decrement
    m_NCS_OS_CLEANUP( ) ..................... to request OS cleanup services
    m_NCS_OS_FILE( pncs_file, request ) ...... to request OS file operations

******************************************************************************
*/

#ifndef NCS_OSPRM_H
#define NCS_OSPRM_H

#include <stdlib.h>
#include <sys/select.h>
#include "logtrace.h"
#include "ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                       General Definitions                              **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/***************************************************************************
 ** Typedefs for callback functions
 **
 ** NCS_OS_CB = "Void Returning CallBack Function using Void Pointer arg"
 ** VR_CB_V  = "Void Returning CallBack Function using Void arg"
 **
 ** ********************************************************************** **/
	typedef void (*NCS_OS_CB) (void *);
	typedef void (*VR_CBF_V) (void);

	typedef void (*sighandler_t) (int);

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                         Task Interface Primitives                      **
 **                                                                        **
 ** This interface is used by the client to request task services from the **
 ** operating system. The interface utilizes the NCS_OS_TASK control        **
 ** structure defined below processing task requests.                      **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/****************************************************************************
 * Control Structure Definition
 ***************************************************************************/
	typedef struct ncs_os_task_tag {
		union {
			struct {
				NCS_OS_CB i_entry_point;
				char *i_name;
				unsigned int i_priority;
				unsigned int i_stack_nbytes;
				int i_policy;
				void *i_ep_arg;
				void *o_handle;
			} create;

			struct {
				void *i_handle;
			} release;

			struct {
				void *i_handle;
			} detach;

			struct {
				void *i_handle;
			} start;

			struct {
				void *i_handle;
			} stop;

			struct {
				unsigned int i_delay_in_ms;
			} sleep;

			struct {
				void *o_handle;
			} current_handle;

		} info;

	} NCS_OS_TASK;

/****************************************************************************
 * Supported Operations
 *  NCS_OS_TASK_CREATE  (mandatory) Create/Initialize a NCS_OS_TASK task.
 *  NCS_OS_TASK_RELEASE (mandatory) Release/Terminate a NCS_OS_TASK task.
 *  NCS_OS_TASK_START   (mandatory) Start a NCS_OS_TASK task.
 *  NCS_OS_TASK_STOP    (mandatory) Stop a NCS_OS_TASK task.
 *  NCS_OS_TASK_SLEEP   (mandatory) Sleep/delay a NCS_OS_TASK task.
 *  NCS_OS_TASK_CURRENT_HANDLE (mandatory) Return handle of current NCS_OS_TASK task.
 ***************************************************************************/
	typedef enum {
		NCS_OS_TASK_CREATE = 1,
		NCS_OS_TASK_RELEASE,
		NCS_OS_TASK_DETACH,
		NCS_OS_TASK_START,
		NCS_OS_TASK_STOP,
		NCS_OS_TASK_SLEEP,
		NCS_OS_TASK_CURRENT_HANDLE,
		NCS_OS_TASK_JOIN,
		NCS_OS_TASK_MAX
	} NCS_OS_TASK_REQUEST;

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                        Timer Interface Primitives                      **
 **                                                                        **
 **                                                                        **
 ** Various operating systems provide a varity of timer services, often    **
 ** with vastly different capabilities.  However, the m_NCS_OS_TIMER        **
 ** interface here only depends on the native os  for a a simple "tick"    **
 ** in the range of 10 - 100 millisecond intervals.  The port services     **
 ** layer provides the full timer services required by the H&J subsystems  **
 ** using this simple primitive.                                           **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/****************************************************************************
 * Control Structure Definition
 ***************************************************************************/
	typedef struct ncs_os_timer_tag {
		union {
			struct {
				void *o_handle;	/* timer identifier                         */
				NCS_OS_CB i_callback;	/* expiration callback function             */
				void *i_cb_arg;	/* callback function argument               */
				unsigned long i_period_in_ms;	/* timer expiration time                    */
			} create;

			struct {
				void *i_handle;	/* timer identifier                         */
			} release;

		} info;

	} NCS_OS_TIMER;

/****************************************************************************
 * Supported Operations
 *  NCS_OS_TIMER_CREATE  (mandatory) Create/Initialize NCS_OS_TIMER object
 *  NCS_OS_TIMER_RELEASE (mandatory) Release resources for NCS_OS_TIMER object.
 ***************************************************************************/
	typedef enum {
		NCS_OS_TIMER_CREATE = 1,
		NCS_OS_TIMER_RELEASE,
		NCS_OS_TIMER_REQUEST_MAX
	} NCS_OS_TIMER_REQUEST;

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                         Lock Interface Primitives                      **
 **                                                                        **
 ** This interface is used by the client to control access to data or      **
 ** other shared resources. The interface utilizes the typedef NCS_OS_LOCK  **
 ** for processing object access requests. The NCS_OS_LOCK typedef is       **
 ** DEFINED BY THE TARGET SYSTEM in any manner necessary to carry the      **
 ** information needed to implement the interface.                         **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/****************************************************************************
 * Control Structure Definition
 * NCS_OS_LOCK must be defined in the os_defs.h file for each operating
 * system.
 ***************************************************************************/

/****************************************************************************
 * Supported Operations
 *  NCS_OS_LOCK_CREATE  (mandatory) Initialize a NCS_OS_LOCK object
 *  NCS_OS_LOCK_LOCK    (mandatory) Lock the object.
 *  NCS_OS_LOCK_UNLOCK  (mandatory) Unlock the object.
 *  NCS_OS_LOCK_RELEASE (mandatory) Release resources for this lock object.
 ***************************************************************************/
	typedef enum {
		NCS_OS_LOCK_CREATE = 1,
		NCS_OS_LOCK_RELEASE,
		NCS_OS_LOCK_LOCK,
		NCS_OS_LOCK_UNLOCK,
		NCS_OS_LOCK_REQUEST_MAX
	} NCS_OS_LOCK_REQUEST;

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                 Counting   Semaphore Interface Primitives              **
 **                                                                        **
 **                                                                        **
 ** This interface is used by the client to enable using a counting        **
 ** semaphore for messaging between concurrently running tasks.            **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/****************************************************************************
 * Control Structure Definition
 ***************************************************************************/
	typedef struct ncs_os_sem_tag {
		union {
			struct {
				void *o_handle;
			} create;

			struct {
				void *i_handle;
			} give;

			struct {
				void *i_handle;
			} take;

			struct {
				void *i_handle;
			} release;

		} info;

	} NCS_OS_SEM;

/****************************************************************************
 * Supported Operations
 *  NCS_OS_SEM_CREATE  (mandatory) Create an NCS_OS_SEM object
 *  NCS_OS_SEM_GIVE    (mandatory) Increment/unblock a NCS_OS_SEM object.
 *  NCS_OS_SEM_TAKE    (mandatory) Wait for a NCS_OS_SEM object.
 *  NCS_OS_SEM_RELEASE (mandatory) Release resources for this NCS_OS_SEM object.
 ***************************************************************************/
	typedef enum {
		NCS_OS_SEM_CREATE = 1,
		NCS_OS_SEM_GIVE,
		NCS_OS_SEM_TAKE,
		NCS_OS_SEM_RELEASE,
		NCS_OS_SEM_REQUEST_MAX
	} NCS_OS_SEM_REQUEST;

/***************************************************************************
***************************************************************************
***************************************************************************
*  File operations
*  Control Structure Definitions
*
***************************************************************************
***************************************************************************
**************************************************************************/

#define NCS_OS_FILE_PERM_READ   0x01
#define NCS_OS_FILE_PERM_WRITE  0x02
#define NCS_OS_FILE_PERM_APPEND 0x04

	typedef struct ncs_os_file_op_create {
		char *i_file_name;	/* with full directory path */
		void *o_file_handle;	/* handle for future reference */
	} NCS_OS_FILE_OP_CREATE;

	typedef struct ncs_os_file_op_open {
		char *i_file_name;
		uint8_t i_read_write_mask;	/* to specify read-write permissions */
		void *o_file_handle;	/* handle for future reference */
	} NCS_OS_FILE_OP_OPEN;

	typedef struct ncs_os_file_op_read {
		void *i_file_handle;
		uint32_t i_buf_size;
		uint8_t *i_buffer;
		uint32_t o_bytes_read;
	} NCS_OS_FILE_OP_READ;

	typedef struct ncs_os_file_op_close {
		void *i_file_handle;
	} NCS_OS_FILE_OP_CLOSE;

	typedef struct ncs_os_file_op_write {
		void *i_file_handle;
		uint32_t i_buf_size;
		uint8_t *i_buffer;
		uint32_t o_bytes_written;
	} NCS_OS_FILE_OP_WRITE;

	typedef struct ncs_os_file_op_seek {
		void *i_file_handle;
		uint32_t i_offset;
	} NCS_OS_FILE_OP_SEEK;

	typedef struct ncs_os_file_op_copy {
		char *i_file_name;
		uint8_t *i_new_file_name;
	} NCS_OS_FILE_OP_COPY;

	typedef struct ncs_os_file_op_rename {
		char *i_file_name;
		uint8_t *i_new_file_name;
	} NCS_OS_FILE_OP_RENAME;

	typedef struct ncs_os_file_op_remove {
		char *i_file_name;
	} NCS_OS_FILE_OP_REMOVE;

	typedef struct ncs_os_file_op_size {
		char *i_file_name;
		uint32_t o_file_size;
	} NCS_OS_FILE_OP_SIZE;

	typedef struct ncs_os_file_op_file_exists {
		char *i_file_name;
		bool o_file_exists;
	} NCS_OS_FILE_OP_FILE_EXISTS;

	typedef struct ncs_os_file_op_dir_path {
		char *i_main_dir;
		char *i_sub_dir;
		uint8_t *io_buffer;
		uint32_t i_buf_size;
	} NCS_OS_FILE_OP_DIR_PATH;

	typedef struct ncs_os_file_op_create_dir {
		char *i_dir_name;
	} NCS_OS_FILE_OP_CREATE_DIR;

	typedef struct ncs_os_file_op_delete_dir {
		char *i_dir_name;
	} NCS_OS_FILE_OP_DELETE_DIR;

	typedef struct ncs_os_file_op_copy_dir {
		char *i_dir_name;
		char *i_new_dir_name;
	} NCS_OS_FILE_OP_COPY_DIR;

	typedef struct ncs_os_file_op_dir_exists {
		char *i_dir_name;
		bool o_exists;
	} NCS_OS_FILE_OP_DIR_EXISTS;

	typedef struct ncs_os_file_op_get_next {
		char *i_dir_name;
		char *i_file_name;
		uint8_t *io_next_file;
		uint32_t i_buf_size;
	} NCS_OS_FILE_OP_GET_NEXT;

	typedef struct ncs_os_file_op_get_list {
		char *i_dir_name;
		char **o_namelist;
		uint32_t o_list_count;
	} NCS_OS_FILE_OP_GET_LIST;

	typedef struct ncs_os_file_tag {
		union {
			NCS_OS_FILE_OP_CREATE create;
			NCS_OS_FILE_OP_OPEN open;
			NCS_OS_FILE_OP_CLOSE close;
			NCS_OS_FILE_OP_READ read;
			NCS_OS_FILE_OP_WRITE write;
			NCS_OS_FILE_OP_SEEK seek;
			NCS_OS_FILE_OP_COPY copy;
			NCS_OS_FILE_OP_RENAME rename;
			NCS_OS_FILE_OP_REMOVE remove;
			NCS_OS_FILE_OP_SIZE size;
			NCS_OS_FILE_OP_FILE_EXISTS file_exists;
			NCS_OS_FILE_OP_DIR_PATH dir_path;
			NCS_OS_FILE_OP_CREATE_DIR create_dir;
			NCS_OS_FILE_OP_DELETE_DIR delete_dir;
			NCS_OS_FILE_OP_COPY_DIR copy_dir;
			NCS_OS_FILE_OP_DIR_EXISTS dir_exists;
			NCS_OS_FILE_OP_GET_NEXT get_next;
			NCS_OS_FILE_OP_GET_LIST get_list;
		} info;
	} NCS_OS_FILE;

/***************************************************************************
*  Supported Operations
*
**************************************************************************/

	typedef enum {
		NCS_OS_FILE_CREATE = 1,
		NCS_OS_FILE_OPEN,
		NCS_OS_FILE_CLOSE,
		NCS_OS_FILE_READ,
		NCS_OS_FILE_WRITE,
		NCS_OS_FILE_SEEK,
		NCS_OS_FILE_COPY,
		NCS_OS_FILE_RENAME,
		NCS_OS_FILE_REMOVE,
		NCS_OS_FILE_SIZE,
		NCS_OS_FILE_EXISTS,
		NCS_OS_FILE_DIR_PATH,
		NCS_OS_FILE_CREATE_DIR,
		NCS_OS_FILE_DELETE_DIR,
		NCS_OS_FILE_COPY_DIR,
		NCS_OS_FILE_DIR_EXISTS,
		NCS_OS_FILE_GET_NEXT,
		NCS_OS_FILE_GET_LIST,
		NCS_OS_FILE_MAX
	} NCS_OS_FILE_REQUEST;

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **           Operating System Specific Implementation Include File        **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

#include "os_defs.h"

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                  Memory Pool Interface Primitives                      **
 **                                                                        **
 **   The Memory Pool Manager was concieved in the context of needing a    **
 **   means to allocate memory from specific (perhaps specialized) memory  **
 **   pools for 'transport services'.                                      **
 **                                                                        **
 **   Specifically, USRBUFs (actually USRDATAs) represent payload data     **
 **   that will be sent out or recieved over a particular transport,       **
 **   which requires memory to come from specific pools that are 'driver   **
 **   friendly' for the particular transport technology that will          **
 **   actually do the work.                                                **
 **                                                                        **
 **   A case in point is the OSE operating system, which has a 'general    **
 **   link handler' protocol, which insists that the buffers sent come     **
 **   from a particular memory space, which is other than 'normal' heap.   **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/***************************************************************************
 * NCS_POOL_MALLOC : function prototype of function returned by the pool
 *                  manager for allocating mem from a particular pool.
 *
 * NCS_POOL_MFREE    function prototype of function returned by the pool
 *                  manager for freeing mem back to a particular pool.
 *
 * These prototypes are used by the USRBUF Pool manager to describe the
 * malloc and free functions that it uses for manipulating memory.
 *
 ***************************************************************************/

	typedef void *(*NCS_POOL_MALLOC)(uint32_t nbytes, uint8_t pool_id, uint8_t priority);

	typedef void (*NCS_POOL_MFREE) (void *data, uint8_t pool_id);

/****************************************************************************
 * User Defined Pool Memory Allocate Primitive definition
 *
 * Macro arguments
 *  'size'             is the number of bytes of memory to allocate
 *  'pool'             pool id from which mem is to come from
 *  'pri;              Priority of request
 *
 * Macro return codes
 * The ncs_os_udef_alloc implemention must return one of the following codes:
 *   valid pointer     (successful), void pointer to allocated object.
 *   NULL pointer      (failure)
 *
 ***************************************************************************/

#ifndef m_OS_UDEF_ALLOC

#define m_OS_UDEF_ALLOC  ncs_os_udef_alloc
	void *ncs_os_udef_alloc(uint32_t size, uint8_t pool_id, uint8_t pri);
#endif

/****************************************************************************
 * User Defined Pool Memory Free Primitive definition
 *
 * Macro arguments
 *  'ptr'            is void pointer to memory object to be freed.
 *  'pool'           pool id from which mem came from
 *
 * Macro return codes
 * none                void
 *
 ***************************************************************************/

#ifndef m_OS_UDEF_FREE

#define m_OS_UDEF_FREE   ncs_os_udef_free
	void ncs_os_udef_free(void *ptr, uint8_t pool);
#endif

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **           Default Operating System Primitive Definitions               **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

/****************************************************************************
 * m_NCS_OS_TARGET_INIT Primitive definition
 * This macro should be called at application genesis to initialize
 * OS and target specific functionality needed by NetPlane products.
 * The macro can be mapped to a ncs_os_target_init() function in
 * os_defs.h file.
 *
 * Macro arguments
 *  none
 *
 * Macro return codes
 *   void 
 *
 ***************************************************************************/

#ifndef m_NCS_OS_TARGET_INIT
#define m_NCS_OS_TARGET_INIT
#endif

/****************************************************************************
 * Task Primitive definition
 * The actual function ncs_os_task must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'pncs_os_task' must be a pointer to a NCS_OS_TASK provided by the caller
 *  'req'         is an NCS_OS_TASK_REQUEST enum.
 *
 * Macro return codes
 * The ncs_os_task implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/

#ifndef m_NCS_OS_TASK

#define m_NCS_OS_TASK(pncs_os_task,req) ncs_os_task (pncs_os_task,req)
	unsigned int ncs_os_task(NCS_OS_TASK *, NCS_OS_TASK_REQUEST);
#endif

/****************************************************************************
* Task Primitive definition
* The actual function ncs_os_file must be resolved in the os_defs.h file.
*
* Macro arguments
*  'pncs_os_file' must be a pointer to a NCS_OS_FILE provided by the caller
*  'req'         is an NCS_OS_FILE_REQUEST enum.
*
* Macro return codes
* The ncs_os_file implemention must return one of the following codes:
*   NCSCC_RC_SUCCESS - interface call successful (normal return code)
*   NCSCC_RC_FAILURE - interface call failed.
*
***************************************************************************/

#ifndef m_NCS_OS_FILE

#define m_NCS_OS_FILE(pncs_os_file,req) ncs_os_file (pncs_os_file,req)
	unsigned int ncs_os_file(NCS_OS_FILE *, NCS_OS_FILE_REQUEST);
#endif

/****************************************************************************
 * m_NCS_OS_TASK_PRELUDE Primitive definition
 * This macro is called as a prelude to all tasks created through
 * the m_NCS_OS_TASK  macro. The actual function ncs_os_task_prelude
 * must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *   none
 *
 * Macro return codes
 *  void 
 ***************************************************************************/
#ifndef m_NCS_OS_TASK_PRELUDE
#define m_NCS_OS_TASK_PRELUDE   {setbuf(stdin,NULL);setbuf(stdout,NULL);setbuf(stderr,NULL);}
#endif

/****************************************************************************
 * OS Cleanup Primitive definition
 * The actual function ncs_os_cleanup must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *   NONE
 *
 * Macro return codes
 *   NONE
 *
 * This function is assumed to be called at system shutdown.
 *
 ***************************************************************************/

#ifndef m_NCS_OS_CLEANUP

#define m_NCS_OS_CLEANUP ncs_os_cleanup()
	void ncs_os_cleanup(void);
#endif

/****************************************************************************
 * Timer Primitive definition
 * The actual macro ncs_os_timer must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'pncs_os_timer' must be a pointer to a NCS_OS_TIMER provided by the caller
 *  'req'          is an NCS_OS_TIMER_REQUEST enum.
 *
 * Macro return codes
 * The ncs_os_timer implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/
#if 0
#ifndef m_NCS_OS_TIMER

#define m_NCS_OS_TIMER(pncs_os_timer,req) ncs_os_timer(pncs_os_timer,req)
	unsigned int ncs_os_timer(NCS_OS_TIMER *, NCS_OS_TIMER_REQUEST);
#endif
#endif

/****************************************************************************
 * LOCK Primitive definition
 * The actual macro ncs_os_lock must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'pncs_os_lock' must be a pointer to a NCS_OS_LOCK provided by the caller
 *  'req'         is an NCS_OS_LOCK_REQUEST enum.
 *  'type'        is an NCS_OS_LOCK_TYPE enum.
 *
 * Macro return codes
 * The ncs_os_lock implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/

#ifndef NCS_OS_LOCK
#define NCS_OS_LOCK uint32_t*
#endif

#ifndef m_NCS_OS_LOCK

#define m_NCS_OS_LOCK(pncs_os_lock,req,type) ncs_os_lock(pncs_os_lock,req,type)
	unsigned int ncs_os_lock(NCS_OS_LOCK *, NCS_OS_LOCK_REQUEST, unsigned int);
#endif

/****************************************************************************
 * Semaphore Primitive definition
 * The actual function ncs_os_sem must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'pncs_os_sem' must be a pointer to a NCS_OS_SEM provided by the caller
 *  'req'        is an NCS_OS_SEM_REQUEST enum.
 *
 * Macro return codes
 * The ncs_os_sem implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/

#ifndef m_NCS_OS_SEM

#define m_NCS_OS_SEM(pncs_os_sem,req) ncs_os_sem(pncs_os_sem,req)
	unsigned int ncs_os_sem(NCS_OS_SEM *, NCS_OS_SEM_REQUEST);
#endif

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **             Operating System Task Preemption Lock macros                **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/
#ifndef m_NCS_OS_INIT_TASK_LOCK
#define m_NCS_OS_INIT_TASK_LOCK
#endif

#ifndef m_NCS_OS_START_TASK_LOCK
#define m_NCS_OS_START_TASK_LOCK   ncs_os_start_task_lock()
	void ncs_os_start_task_lock(void);
#endif

#ifndef m_NCS_OS_END_TASK_LOCK
#define m_NCS_OS_END_TASK_LOCK     ncs_os_end_task_lock()
	void ncs_os_end_task_lock(void);
#endif

/****************************************************************************
 * Message-queues Primitive definition
 * The actual function ncs_os_mq must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'req'        is an NCS_OS_MQ_REQ_INFO structure.
 *
 * Macro return codes
 * The ncs_os_mq implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/

	typedef enum {
		NCS_OS_MQ_REQ_MIN,
		NCS_OS_MQ_REQ_CREATE,	/* Strictly create */
		NCS_OS_MQ_REQ_OPEN,	/* Strictly open i.e. do not create */
		NCS_OS_MQ_REQ_DESTROY,
		NCS_OS_MQ_REQ_MSG_SEND,
		NCS_OS_MQ_REQ_MSG_SEND_ASYNC,
		NCS_OS_MQ_REQ_MSG_RECV,	/* Blocking recv call */
		NCS_OS_MQ_REQ_MSG_RECV_ASYNC,	/* NonBlocking recv call */
		NCS_OS_MQ_REQ_RESIZE,
		NCS_OS_MQ_REQ_MAX
	} NCS_OS_MQ_REQ_TYPE;

/* If MQ has a real implementation for the platform we are building, then 
** "os_defs.c" will have the appropriate definitions. Otherwise the 
** following dummy definitions will apply and will allow successful 
** compilation on unsupported platforms.
*/
#ifndef NCS_OS_MQ_KEY
#define NCS_OS_MQ_KEY int	/* Dummy definition */
#endif
#ifndef NCS_OS_MQ_HDL
#define NCS_OS_MQ_HDL int	/* Dummy definition */
#endif
#ifndef NCS_OS_MQ_MSG_LL_HDR
#define NCS_OS_MQ_MSG_LL_HDR int	/* Dummy definition */
#endif
#ifndef NCS_OS_MQ_MAX_PAYLOAD
#define NCS_OS_MQ_MAX_PAYLOAD 100	/* Dummy definition */
#endif
	typedef struct ncs_os_mq_msg {
		/* ll_hdr is filled by the MQ-implementation. A MQ-user is expected
		   to fill in the "data" portion only.
		 */
		NCS_OS_MQ_MSG_LL_HDR ll_hdr;
		uint8_t data[NCS_OS_MQ_MAX_PAYLOAD];
	} NCS_OS_MQ_MSG;

/*-----------------------------------*/
	typedef struct ncs_mq_req_create_info {
		NCS_OS_MQ_KEY *i_key;
		NCS_OS_MQ_HDL o_hdl;
	} NCS_OS_MQ_REQ_CREATE_INFO;

/*-----------------------------------*/
	typedef NCS_OS_MQ_REQ_CREATE_INFO NCS_OS_MQ_REQ_OPEN_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_destroy_info {
		NCS_OS_MQ_HDL i_hdl;
	} NCS_OS_MQ_REQ_DESTROY_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_msg_send_info {
		NCS_OS_MQ_HDL i_hdl;
		NCS_OS_MQ_MSG *i_msg;
		uint32_t i_len;
		uint32_t i_mtype;	/* Can be used for priority */
	} NCS_OS_MQ_REQ_MSG_SEND_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_msg_recv_info {
		NCS_OS_MQ_HDL i_hdl;
		NCS_OS_MQ_MSG *i_msg;
		uint32_t i_max_recv;
		int32_t i_mtype;	/* the first message on the queue with the 
				   lowest type less than or equal to the 
				   absolute value of i_mtype will be read */
	} NCS_OS_MQ_REQ_MSG_RECV_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_resize_info {
		NCS_OS_MQ_HDL i_hdl;
		uint32_t i_newqsize;	/* new queue size */
	} NCS_OS_MQ_REQ_RESIZE_INFO;

/*-----------------------------------*/

	typedef struct ncs_mq_req {
		NCS_OS_MQ_REQ_TYPE req;

		union {
			NCS_OS_MQ_REQ_CREATE_INFO create;
			NCS_OS_MQ_REQ_OPEN_INFO open;
			NCS_OS_MQ_REQ_DESTROY_INFO destroy;
			NCS_OS_MQ_REQ_MSG_SEND_INFO send;
			NCS_OS_MQ_REQ_MSG_RECV_INFO recv;
			NCS_OS_MQ_REQ_RESIZE_INFO resize;
		} info;
	} NCS_OS_MQ_REQ_INFO;

#define m_NCS_OS_MQ ncs_os_mq
	uint32_t ncs_os_mq(NCS_OS_MQ_REQ_INFO *req);

/****************************************************************************
 * POSIX Message-queues Primitive definition
 * The actual function ncs_os_posix_mq must be resolved in the os_defs.h file.
 *
 * Macro arguments
 *  'req'        is an NCS_OS_POSIX_MQ_REQ_INFO structure.
 *
 * Macro return codes
 * The ncs_os_mq implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/

	typedef enum {
		NCS_OS_POSIX_MQ_REQ_MIN = 1,
		NCS_OS_POSIX_MQ_REQ_OPEN,	/* Strictly open i.e. do not create */
		NCS_OS_POSIX_MQ_REQ_CLOSE,
		NCS_OS_POSIX_MQ_REQ_UNLINK,
		NCS_OS_POSIX_MQ_REQ_MSG_SEND,
		NCS_OS_POSIX_MQ_REQ_MSG_SEND_ASYNC,
		NCS_OS_POSIX_MQ_REQ_MSG_RECV,	/* Blocking recv call */
		NCS_OS_POSIX_MQ_REQ_MSG_RECV_ASYNC,	/* NonBlocking recv call */
		NCS_OS_POSIX_MQ_REQ_GET_ATTR,	/* Get attributes */
		NCS_OS_POSIX_MQ_REQ_RESIZE,
		NCS_OS_POSIX_MQ_REQ_MAX
	} NCS_OS_POSIX_MQ_REQ_TYPE;

/* If POSIX MQ has a real implementation for the platform we are building, then 
** "os_defs.c" will have the appropriate definitions. Otherwise the 
** following dummy definitions will apply and will allow successful 
** compilation on unsupported platforms.
*/
#ifndef NCS_OS_POSIX_MQD
#define NCS_OS_POSIX_MQD  uint32_t
#endif

#ifndef NCS_OS_POSIX_TIMESPEC
	typedef struct posix_timespec {
		uint32_t tv_nsec;
		uint32_t tv_sec;
	} posix_timespec;

#define NCS_OS_POSIX_TIMESPEC struct posix_timespec
#endif

#ifndef NCS_OS_POSIX_MQ_ATTR
	typedef struct ncs_os_posix_mq_attr {
		uint32_t mq_flags;
		uint32_t mq_maxmsg;
		uint32_t mq_msgsize;
		uint32_t mq_curmsgs;
		uint32_t mq_stime;
	} NCS_OS_POSIX_MQ_ATTR;

#define NCS_OS_POSIX_MQ_ATTR struct ncs_os_posix_mq_attr
#endif

/*-----------------------------------*/
	typedef struct ncs_mq_req_open_info {
		char *qname;
		uint32_t node;
		uint32_t iflags;
		NCS_OS_POSIX_MQD o_mqd;
		NCS_OS_POSIX_MQ_ATTR attr;
	} NCS_OS_POSIX_MQ_REQ_OPEN_INFO;

	typedef struct ncs_mq_req_attr_info {
		NCS_OS_POSIX_MQD i_mqd;
		NCS_OS_POSIX_MQ_ATTR o_attr;
	} NCS_OS_POSIX_MQ_REQ_ATTR_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_close_info {
		NCS_OS_POSIX_MQD mqd;
	} NCS_OS_POSIX_MQ_REQ_CLOSE_INFO;

/*-----------------------------------*/
	typedef struct ncs_mq_req_unlink_info {
		uint8_t *qname;
		uint32_t node;
	} NCS_OS_POSIX_MQ_REQ_UNLINK_INFO;

/*-----------------------------------*/
	typedef struct ncs_posix_mq_req_msg_send_info {
		NCS_OS_POSIX_MQD mqd;
		uint32_t datalen;
		uint32_t dataprio;
		NCS_OS_MQ_MSG *i_msg;
		uint32_t i_mtype;	/* Can be used for priority */
	} NCS_OS_POSIX_MQ_REQ_MSG_SEND_INFO;

/*-----------------------------------*/
	typedef struct ncs_posix_mq_req_msg_recv_info {
		NCS_OS_POSIX_MQD mqd;
		uint32_t datalen;
		uint32_t dataprio;
		NCS_OS_POSIX_TIMESPEC timeout;
		NCS_OS_MQ_MSG *i_msg;
		int32_t i_mtype;	/* the first message on the queue with the
				   lowest type less than or equal to the
				   absolute value of i_mtype will be read */
	} NCS_OS_POSIX_MQ_REQ_MSG_RECV_INFO;

/*-----------------------------------*/
	typedef struct ncs_posix_mq_req_resize_info {
		NCS_OS_POSIX_MQD mqd;
		uint32_t i_newqsize;	/* new queue size */
	} NCS_OS_POSIX_MQ_REQ_RESIZE_INFO;

/*-----------------------------------*/
	typedef struct ncs_posix_mq_req_info {
		NCS_OS_POSIX_MQ_REQ_TYPE req;

		union {
			NCS_OS_POSIX_MQ_REQ_OPEN_INFO open;
			NCS_OS_POSIX_MQ_REQ_CLOSE_INFO close;
			NCS_OS_POSIX_MQ_REQ_UNLINK_INFO unlink;
			NCS_OS_POSIX_MQ_REQ_MSG_SEND_INFO send;
			NCS_OS_POSIX_MQ_REQ_MSG_RECV_INFO recv;
			NCS_OS_POSIX_MQ_REQ_ATTR_INFO attr;
			NCS_OS_POSIX_MQ_REQ_RESIZE_INFO resize;
		} info;
	} NCS_OS_POSIX_MQ_REQ_INFO;

#define m_NCS_OS_POSIX_MQ ncs_os_posix_mq
	uint32_t ncs_os_posix_mq(NCS_OS_POSIX_MQ_REQ_INFO *req);

/****************************************************************************
 * POSIX shm_memory Primitive definition
 * The actual function ncs_os_posix_shm must be resolved in the os_defs.c file.
 *
 * Macro arguments
 *  'req'        is an NCS_OS_POSIX_SHM_REQ_INFO structure.
 *
 * Macro return codes
 * The ncs_os_posix_shm implemention must return one of the following codes:
 *   NCSCC_RC_SUCCESS - interface call successful (normal return code)
 *   NCSCC_RC_FAILURE - interface call failed.
 *
 ***************************************************************************/
	typedef enum {

		NCS_OS_POSIX_SHM_REQ_MIN = 1,
		NCS_OS_POSIX_SHM_REQ_OPEN,	/* opens and mmaps */
		NCS_OS_POSIX_SHM_REQ_CLOSE,	/* close is munmap */
		NCS_OS_POSIX_SHM_REQ_UNLINK,	/* unlink is shm_unlink */
		NCS_OS_POSIX_SHM_REQ_READ,
		NCS_OS_POSIX_SHM_REQ_WRITE,
		NCS_OS_POSIX_SHM_REQ_MAX
	} NCS_OS_POSIX_SHM_REQ_TYPE;
	typedef struct ncs_os_posix_shm_req_open_info_tag {
		char *i_name;
		uint32_t i_flags;
		uint32_t i_map_flags;
		uint64_t i_size;
		int32_t i_offset;
		void *o_addr;
		int32_t o_fd;
		uint32_t o_hdl;

	} NCS_OS_POSIX_SHM_REQ_OPEN_INFO;
	typedef struct ncs_os_posix_shm_req_close_info_tag {
		uint32_t i_hdl;
		void *i_addr;
		int32_t i_fd;
		uint64_t i_size;
	} NCS_OS_POSIX_SHM_REQ_CLOSE_INFO;

	typedef struct ncs_os_posix_shm_req_unlink_info_tag {
		char *i_name;
	} NCS_OS_POSIX_SHM_REQ_UNLINK_INFO;

	typedef struct ncs_os_posix_shm_req_read_info {
		uint32_t i_hdl;
		void *i_addr;
		void *i_to_buff;
		uint32_t i_read_size;
		int32_t i_offset;
	} NCS_OS_POSIX_SHM_REQ_READ_INFO;

	typedef struct ncs_os_posix_shm_req_write_info {
		uint32_t i_hdl;
		void *i_addr;
		void *i_from_buff;
		uint32_t i_write_size;
		int32_t i_offset;
	} NCS_OS_POSIX_SHM_REQ_WRITE_INFO;

	typedef struct ncs_shm_req_info {
		NCS_OS_POSIX_SHM_REQ_TYPE type;

		union {
			NCS_OS_POSIX_SHM_REQ_OPEN_INFO open;
			NCS_OS_POSIX_SHM_REQ_CLOSE_INFO close;
			NCS_OS_POSIX_SHM_REQ_UNLINK_INFO unlink;
			NCS_OS_POSIX_SHM_REQ_READ_INFO read;
			NCS_OS_POSIX_SHM_REQ_WRITE_INFO write;
		} info;

	} NCS_OS_POSIX_SHM_REQ_INFO;

	uint32_t ncs_os_posix_shm(NCS_OS_POSIX_SHM_REQ_INFO *req);

/****************************************************************************\
 * B E G I N  :  S E L E C T I O N - O B J E C T    P R I M I T I V E S     *
\****************************************************************************/
/****************************************************************************\

  NCS_SEL_OBJ  :        A data type for a single selection object.  It is
                        an object on which a POSIX "select()" or the
                        m_NCS_SEL_OBJ_SELECT() could be invoked.

  NCS_SEL_OBJ_SET:      A data type for a "set of selection-objects". Such 
                        a set needs to be constructed for invoking a
                        m_NCS_SEL_OBJ_SELECT() API.

  The real definitions are present in respective osprims/<os>/os_defs.h. The
  following are dummy definitions
\****************************************************************************/
#ifndef NCS_SEL_OBJ_DEFINED

	typedef int NCS_SEL_OBJ;	/* DUMMY DEFINITION for unsupported OSes */
	typedef int NCS_SEL_OBJ_SET;	/* DUMMY DEFINITION for unsupported OSes */
#endif

/****************************************************************************\
  
  m_GET_FD_FROM_SEL_OBJ:A user should not directly access the members of
                        a "ncs_sel_obj" structure. He is expected to use a 
                        m_GET_FD_FROM_SEL_OBJ macro to extract a selectable 
                        FD from an NCS_SEL_OBJ variable.

                        (Motivation - Data type abstraction. Will be useful
                        in future if we wish to change the data-type of
                        NCS_SEL_OBJ, without effecting user code)

  m_SET_FD_IN_SEL_OBJ:  This API is provided for the purpose of mixing
                        FDs generated by means external to these primitives
                        with selection-objects generated by these primitives.
                        Such a mixing of FDs and selection-objects is only
                        useful while making a m_NCS_SEL_OBJ_SELECT call.

  m_GET_HIGHER_SEL_OBJ: This API returns the "higher" selection-object (in a
                        sense that is meaningful for the m_NCS_SEL_OBJ_SELECT
                        call.) It is required in determining the highest
                        selection-object that needs to be provided for
                        an m_NCS_SEL_OBJ_SELECT call.
\****************************************************************************/
#define m_GET_FD_FROM_SEL_OBJ(sel_obj)           \
            (sel_obj.rmv_obj)

#define m_SET_FD_IN_SEL_OBJ(fd, sel_obj)         \
            (sel_obj.raise_obj=-1,sel_obj.rmv_obj=fd)

#define m_GET_HIGHER_SEL_OBJ(sel_obj1, sel_obj2) \
            ((sel_obj1).rmv_obj > (sel_obj2).rmv_obj? (sel_obj1):(sel_obj2))

/****************************************************************************\

   ncs_sel_obj_create:  Returns a selection-object pair. It internally may 
                        involve socket creations etc. The caller is
                        transparent to this.

   RETURN VALUE :       NCSCC_RC_SUCCESS on success.
                        NCSCC_RC_FAILURE on failure.

   ARGUMENTS    :  

    o_sel_obj   :       A selection-object which can be used to pass
                        indications from one task to another. It contains
                        a POSIX style file-descriptor on which a 
                        POSIX select() can be invoked. 

                        (This is an OUT argument and is set to a valid value
                        if the return value is success)

\****************************************************************************/
	uint32_t ncs_sel_obj_create(NCS_SEL_OBJ *o_sel_obj);
#define     m_NCS_SEL_OBJ_CREATE(o_sel_obj) ncs_sel_obj_create(o_sel_obj)

/****************************************************************************\ 

   ncs_sel_obj_destroy: Destroys a selection-object

   RETURN VALUE :       NCSCC_RC_SUCCESS on success
                        NCSCC_RC_FAILURE if the selection-object is
                        not in use currently and it is therefore a bad
                        selection-object.

   ARGUMENTS    :

    i_sel_obj   :       Frees a selection-object created by the
                        m_NCS_SEL_OBJ_CREATE.
                        
                        (This is an IN argument)

\****************************************************************************/
	uint32_t ncs_sel_obj_destroy(NCS_SEL_OBJ i_sel_obj);
#define     m_NCS_SEL_OBJ_DESTROY(i_sel_obj) ncs_sel_obj_destroy(i_sel_obj)

/****************************************************************************\ 

   ncs_sel_obj_rmv_operation_shut: Destroys a selection-object rmv fd

   RETURN VALUE :       NCSCC_RC_SUCCESS on success
                        NCSCC_RC_FAILURE if the selection-object is
                        not in use currently and it is therefore a bad
                        selection-object.

   ARGUMENTS    :

    *i_sel_obj   :      Closes  selection-objects rmv fd created by the
                        m_NCS_SEL_OBJ_CREATE.
                        
                        (This is an IN/OUT argument and  rmv fd is set to  -1 
                        if the return value is success)

\****************************************************************************/

	uint32_t ncs_sel_obj_rmv_operation_shut(NCS_SEL_OBJ *i_sel_obj);
#define     m_NCS_SEL_OBJ_RMV_OPERATION_SHUT(i_sel_obj) ncs_sel_obj_rmv_operation_shut(i_sel_obj)

/****************************************************************************\ 

   ncs_sel_obj_raise_operation_shut: Destroys a selection-object raise fd

   RETURN VALUE :       NCSCC_RC_SUCCESS on success
                        NCSCC_RC_FAILURE if the selection-object is
                        not in use currently and it is therefore a bad
                        selection-object.

   ARGUMENTS    :

    *i_sel_obj   :      Closes  selection-objects raise fd created by the
                        m_NCS_SEL_OBJ_CREATE.
                        
                        (This is an IN/OUT argument and  raise fd is set to  -1 
                        if the return value is success)

\****************************************************************************/
	uint32_t ncs_sel_obj_raise_operation_shut(NCS_SEL_OBJ *i_sel_obj);
#define     m_NCS_SEL_OBJ_RAISE_OPERATION_SHUT(i_sel_obj) ncs_sel_obj_raise_operation_shut(i_sel_obj)

/****************************************************************************\ 
   ncs_sel_obj_ind:     Queues exactly 1 indication on a selection-object. It
                        is a non-blocking call.

   RETURN VALUE :       Success: If an indication was successfully queued.
                        Failure: If and indication could not be queued 
                        (FAILURE is expected to be a very rare condition)

   ARGUMENTS    :       

    i_sel_obj   :       A selection-object created by the
                        m_NCS_SEL_OBJ_CREATE.

                        (This is an IN argument)

\****************************************************************************/
	uint32_t ncs_sel_obj_ind(NCS_SEL_OBJ i_sel_obj);
#define     m_NCS_SEL_OBJ_IND(i_sel_obj)  ncs_sel_obj_ind(i_sel_obj)

/****************************************************************************\ 
   ncs_sel_obj_rmv_ind: Removes 1 or more indications queued on 
                        selection-object. It supports blocking and 
                        non-blocking option

   RETURN VALUE :       
      
     -1:                The selection-object is no longer valid. This
                        selection-object should no longer be used.

     0:                 There are no indications queued up on the 
                        selection-object. (Such a value can be returned
                        only by a non-blocking call)

     +ve:               The number of indications on the selection-object
                        removed by this call.  
                         
   ARGUMENTS    :       
   
    i_sel_obj   :       A selection-object created by m_NCS_SEL_OBJ_CREATE

    i_no_blocking_flag: If true, this call returns immediately even if no
                        no indications are queued up. If false, this call
                        blocks until either an indication arrives or
                        blocks until the selection-object is destroyed, 
                        whichever happens earlier. 

    i_rmv_only_one flag:If true, only 1 indication is dequeued. The user
                        is expected to call RMV-IND API again to further
                        remove indications. 

                        If false, _ALL_ inications that are queued up
                        at the time of invocation are dequeued. The number
                        of indications dequeued is indicated through 
                        the return value.
 
\***************************************************************************/
	int ncs_sel_obj_rmv_ind(NCS_SEL_OBJ i_sel_obj, bool i_no_blocking_flag, bool i_rmv_only_one_flag);

#define     m_NCS_SEL_OBJ_RMV_IND(sel_obj, noblock_flag, rmv_only_one_flag)\
            ncs_sel_obj_rmv_ind(sel_obj, noblock_flag,  rmv_only_one_flag)

/****************************************************************************\ 
   ncs_sel_obj_select:  Identical to POSIX select except for the data types
                        of the argument and return values

  RETURN VALUE  :       Identical to POSIX select
                        -1 = in case of an error 
                        0  = if select() returns due to timeout
                        n  = No. of objects set in the bitmasks.

  ARGUMENTS     :       
   
      highest_sel_obj:  Highest selection-object present in the 
                        sel-obj-sets provided (viz. io_readfds,
                        io_writefds, io_exceptfds)

                        NOTE : POSIX select() API expects highest-fd 
                        plus 1. But this API is happy even with a
                        highest-fd.

                        (Highest selection-object should be determined
                        using the m_GET_HIGHER_SEL_OBJ() macro)
      
      io_readfds:       Similar to "readfds" in a POSIX select call
      io_writefds:      Similar to "writefds" in a POSIX select call
      io_exceptfds:     Similar to "exceptfds" in a POSIX select call
                              
      io_timeout:       Similar to "timeout " in a POSIX select call. But
                        instead of a "struct timeval" it is a timeout
                        in multiples of 10-milliseconds (can be
                        called centisecond units?) ('io_timeout" will
                        return time elapsed since m_NCS_SEL_OBJ_SELECT
                        invocation, if supported by underlying Operating
                        System, for example Linux)
  
   NOTES        :       The io_*fds provided to this call NEED-NOT necessarily 
                        be created by m_NCS_SEL_OBJ_CREATE only. They could
                        contain objects created by POSIX APIs (viz. open,
                        etc.) too.

\***************************************************************************/
/* Select is returnig uint32_t it should be int */

	int ncs_sel_obj_select(NCS_SEL_OBJ highest_sel_obj,
			       NCS_SEL_OBJ_SET *io_readfds,
			       NCS_SEL_OBJ_SET *io_writefds, NCS_SEL_OBJ_SET *io_exceptfds, uint32_t *io_timeout);
#define     m_NCS_SEL_OBJ_SELECT(m, r, w, e, t)\
            ncs_sel_obj_select(m, r, w, e, t)

/****************************************************************************\ 
   ncs_sel_obj_poll_single_obj:  Invokes poll() over a single NCS_SEL_OBJ. 
                         Looks for POLLIN event. 

  RETURN VALUE  :       -1 = If call interrupt with (errno != EINTR)
                        0  = If poll() times out, before POLLIN is received.
                        1  = If POLLIN is received

  ARGUMENTS     :       

      sel_obj   :       The selection object that needs to be polled.
   
      io_timeout:       Maximum time to block (in-out variable). 
                        It has the following usage

                        NULL => Infinite timeout

                        non-NULL => maximum time to block. This is an
                                    in/out variable. On return from function
                                    call, this argument will containing the
                                    remaining time.
  
   NOTES        :       This function can be invoked on a greater range
                        of "fds" than that accepted by select(). 

\***************************************************************************/
	int32_t ncs_sel_obj_poll_single_obj(NCS_SEL_OBJ sel_obj, uint32_t *io_timeout);
#define     m_NCS_SEL_OBJ_POLL_SINGLE_OBJ(obj, io_timeout)\
            ncs_sel_obj_poll_single_obj(obj, io_timeout)

/****************************************************************************\ 
   The following macros are currently based on the macros defined for use with
   POSIX 'select()' and have identical semantics.
\****************************************************************************/
static inline void m_NCS_SEL_OBJ_ZERO(NCS_SEL_OBJ_SET* o_sel_obj_set)
{
	FD_ZERO(o_sel_obj_set);
}

static inline void m_NCS_SEL_OBJ_SET(NCS_SEL_OBJ i_sel_obj,
	NCS_SEL_OBJ_SET* io_sel_obj_set)
{
	if (i_sel_obj.rmv_obj >= 0 && i_sel_obj.rmv_obj < FD_SETSIZE) {
		FD_SET(i_sel_obj.rmv_obj, io_sel_obj_set);
	} else {
		LOG_ER("Process terminated due to file descriptor %d outside "
			"supported range [0, 1024)", i_sel_obj.rmv_obj);
		abort();
	}
}

static inline int m_NCS_SEL_OBJ_ISSET(NCS_SEL_OBJ i_sel_obj,
	const NCS_SEL_OBJ_SET* i_sel_obj_set)
{
	if (i_sel_obj.rmv_obj >= 0 && i_sel_obj.rmv_obj < FD_SETSIZE) {
		return FD_ISSET(i_sel_obj.rmv_obj, i_sel_obj_set);
	} else {
		LOG_ER("Process terminated due to file descriptor %d outside "
			"supported range [0, 1024)", i_sel_obj.rmv_obj);
		abort();
	}
}

static inline void m_NCS_SEL_OBJ_CLR(NCS_SEL_OBJ i_sel_obj,
	NCS_SEL_OBJ_SET* io_sel_obj_set)
{
	if (i_sel_obj.rmv_obj >= 0 && i_sel_obj.rmv_obj < FD_SETSIZE) {
		FD_CLR(i_sel_obj.rmv_obj, io_sel_obj_set);
	} else {
		LOG_ER("Process terminated due to file descriptor %d outside "
			"supported range [0, 1024)", i_sel_obj.rmv_obj);
		abort();
	}
}

/****************************************************************************\
 * E N D      :  S E L E C T I O N - O B J E C T    P R I M I T I V E S     *
\****************************************************************************/

/****************************************************************************
 * Atomic Increment definition
 *
 * Macro arguments
 *  'p_uns32' must be a pointer to a uint32_t to increment
 *
 * Macro return codes
 * uns32
 *
 ***************************************************************************/

#ifndef m_NCS_OS_ATOMIC_INC
#error Warning! Implementation needed for Atomic Increment
#endif

/****************************************************************************
 * Atomic Decrement definition
 *
 * Macro arguments
 *  'p_uns32' must be a pointer to a uint32_t to increment
 *
 * Macro return codes
 * uns32
 *
 ***************************************************************************/

#ifndef m_NCS_OS_ATOMIC_DEC
#error Warning! Implementation needed for Atomic Decrement
#endif

/****************************************************************************
 *                            Console I0 Macros                            *
 *                                                                         *
 ***************************************************************************/

#ifndef m_NCS_OS_DBG_PRINTF
#error Warning! Implementation needed for DBG Printf
#endif

/****************************************************************************
 * OS CPU usage Primitive definition
 *
 * returns current CPU usage in percents 
 *
 ***************************************************************************/

#ifndef m_NCS_OS_CUR_CPU_USAGE
#define m_NCS_OS_CUR_CPU_USAGE  os_cur_cpu_usage()
	unsigned int os_cur_cpu_usage(void);
#endif

/****************************************************************************
 * OS CPU Monitor Primitive definitions
 *
 * returns current CPU usage in percents 
 *
 ***************************************************************************/

#ifndef m_NCS_OS_INIT_CPU_MON
#define m_NCS_OS_INIT_CPU_MON  ncs_cpu_mon_init()
	unsigned int ncs_cpu_mon_init(void);
#endif

#ifndef m_NCS_OS_SHUTDOWN_CPU_MON
#define m_NCS_OS_SHUTDOWN_CPU_MON  ncs_cpu_mon_shutdown()
	unsigned int ncs_cpu_mon_shutdown(void);
#endif

/****************************************************************************
 * Memory Region Identifiers Definitions
 *
 * These are default definitions for the memory regions identifiers.  By
 * default, they are all set to NULL.  In order to override the default
 * setting, add #defines in os_defs.h.
 *
 * All regions should be visible to the CPU.  Some regions should also
 * be visible by other components (noted below).
 *
 * NCS_MEM_REGION_IO_DATA
 *   This region is used for payload data in packet buffers.  It should be
 *   visible to all I/O interfaces (e.g., SAR drivers, HDLC drivers).
 * NCS_MEM_REGION_IO_DATA_HDR
 *   This region is used for buffer overhead in packet buffers.  It may
 *   need to be visible to the I/O interfaces.
 * NCS_MEM_REGION_INTERPROCESSOR_DATA
 *   This region is used for payload data used to transfer information
 *   between processors (e.g., for redundancy or for multiprocessor PNNI).
 *   It should be visible to the interprocessor backplane.
 * NCS_MEM_REGION_INTERPROCESSOR_DATA_HDR
 *   This region is used for buffer overhead used to transfer information
 *   between processors (e.g., for redundancy or for multiprocessor PNNI).
 *   It may need to be visible to the interprocessor backplane.
 * NCS_MEM_REGION_PERSISTENT
 *   This region is used for general memory allocations.  It may be mapped
 *   directly to the system heap.  Structures allocated from this region
 *   persist for a relatively long time (e.g., an ATM signalling interface
 *   descriptor (QSCB)).
 * NCS_MEM_REGION_TRANSIENT
 *   This region is used for memory allocations which occur on a per-packet/
 *   per-call basis (or more frequently), such as event descriptors, call
 *   descriptors, etc.  These allocations will directly impact per-packet/
 *   per-call performance, and thus should probably not use malloc/free from
 *   the system heap.  Requests for memory in this region should be optimized.
 *   One way to do this is to have a pool of pre-sized memory chunks on a
 *   free list.
 *
 ***************************************************************************/
#ifndef NCS_MEM_REGION_IO_DATA
#define NCS_MEM_REGION_IO_DATA                   NULL
#endif
#ifndef NCS_MEM_REGION_IO_DATA_HDR
#define NCS_MEM_REGION_IO_DATA_HDR               NULL
#endif
#ifndef NCS_MEM_REGION_INTERPROCESSOR_DATA
#define NCS_MEM_REGION_INTERPROCESSOR_DATA       NULL
#endif
#ifndef NCS_MEM_REGION_INTERPROCESSOR_DATA_HDR
#define NCS_MEM_REGION_INTERPROCESSOR_DATA_HDR   NULL
#endif
#ifndef NCS_MEM_REGION_PERSISTENT
#define NCS_MEM_REGION_PERSISTENT                NULL
#endif
#ifndef NCS_MEM_REGION_TRANSIENT
#define NCS_MEM_REGION_TRANSIENT                 NULL
#endif

/****************************************************************************
 * Memory Allocate Primitive definition
 *
 * Macro arguments
 *  'nbytes'           is the number of bytes of memory to allocate
 *  'region'           is the memory region to allocate the object from
 *
 * Macro return codes
 * The ncs_os_memalloc implemention must return one of the following codes:
 *   valid pointer     (successful), void pointer to allocated object.
 *   NULL pointer      (failure)
 *
 ***************************************************************************/
#if (USE_MY_MALLOC==1)
#ifndef MY_MALLOC_SIZE
#define MY_MALLOC_SIZE 256	/* bytes */
#endif
#endif

#ifndef m_NCS_OS_MEMALLOC
#if (USE_MY_MALLOC==1)
#define m_NCS_OS_MEMALLOC(nbytes, region) my_malloc(nbytes)
	void *my_malloc(size_t nbytes);
#else
#define m_NCS_OS_MEMALLOC(nbytes, region) malloc(nbytes)
#endif
#endif

/****************************************************************************
 * Memory Free Primitive definition
 *
 * Macro arguments
 *  'mem_p'            is void pointer to memory object to be freed.
 *  'region'           is the memory region to free the object to
 *
 * Macro return codes
 * none                void
 *
 ***************************************************************************/
#ifndef m_NCS_OS_MEMFREE
#if (USE_MY_MALLOC==1)
#define m_NCS_OS_MEMFREE(mem_p, region)  my_free(mem_p)
	void my_free(void *mem_p);
#else
#define m_NCS_OS_MEMFREE(mem_p, region)  free(mem_p)
#endif
#endif

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **                   System Timestamp Interface Macros                     **
 **                                                                         **
 ** ncs_os_get_time_stamp:       Return the current timestamp as "time_t" in **
 **                             the argument tod.                           **
 **                                                                         **
 ** ncs_os_get_ascii_time_stamp: Fetch the current timestamp (tod), as an    **
 **                             ascii string, in asc_tod. Note the size of  **
 **                             the ASCII string is limited to 32 octets.   **
 **                                                                         **
 ****************************************************************************/
#ifndef m_NCS_OS_GET_TIME_STAMP
#define m_NCS_OS_GET_TIME_STAMP(timestamp) timestamp=time((time_t*)0)
#endif

/* Note: localtime() is not reentrant 
 * Whereever reentrant version of localtime() is available it is 
 * overridden in corresponding os_defs.h 
 * For eg. In linux localtime_r() is available reentrant version 
 * of localtime_r() so in linux/os_defs.h it is overridden.
 */

#ifndef m_NCS_OS_TIME_TO_STR
#define m_NCS_OS_TIME_TO_STR(timestamp, asc_timestamp)  \
{ \
    strftime((char *)(asc_timestamp), 32, "%X", localtime(&timestamp)); \
}
#endif

#ifndef m_NCS_OS_DATE_TIME_TO_STR
#define m_NCS_OS_DATE_TIME_TO_STR(timestamp, asc_timestamp)  \
{ \
    strftime((char *)(asc_timestamp), 40, "%d%b%Y_%H.%M.%S", localtime(&timestamp)); \
}
#endif

#ifndef m_NCS_OS_GET_ASCII_TIME_STAMP
#define m_NCS_OS_GET_ASCII_TIME_STAMP(timestamp, asc_timestamp)  \
{ \
    (timestamp) = (time_t) (time((time_t *) 0)); \
    strftime((char *)(asc_timestamp), 32, "%X", localtime(&timestamp)); \
}
#endif

#ifndef m_NCS_OS_GET_ASCII_DATE_TIME_STAMP
#define m_NCS_OS_GET_ASCII_DATE_TIME_STAMP(timestamp, asc_timestamp)  \
{ \
    timestamp=(time_t) (time((time_t *) 0)); \
    strftime((char *)(asc_timestamp), 40, "%d%b%Y_%H.%M.%S", localtime(&timestamp)); \
}
#endif

#ifndef m_NCS_OS_GET_ASCII_HDR_DATE_TIME_STAMP
#define m_NCS_OS_GET_ASCII_HDR_DATE_TIME_STAMP(timestamp, asc_timestamp)  \
{ \
    timestamp=(time_t) (time((time_t *) 0)); \
    strftime((char *)(asc_timestamp), 40, "%d %B %Y %H:%M:%S", localtime(&timestamp)); \
}
#endif

#ifndef m_NCS_OS_GET_TIME_MS
#define m_NCS_OS_GET_TIME_MS     0
#endif

#ifndef m_NCS_OS_GET_TIME_NS
#define m_NCS_OS_GET_TIME_NS     0
#endif

#ifndef m_NCS_OS_GET_UPTIME
#error Warning! macro for uptime usage to be defined
#endif

/****************************************************************************
 **                                                                        **
 **                  Standard CLIB/STDIO Interfaces                        **
 **                                                                        **
 **                                                                        **
 **  The following interfaces only have to be defined in os_defs.h         **
 **  if the default definition is not available on a given target          **
 **                                                                        **
 ***************************************************************************/

#ifndef m_NCS_OS_LOG_FOPEN
#define m_NCS_OS_LOG_FOPEN           fopen
#endif

#ifndef m_NCS_OS_START
#define m_NCS_OS_START(a,b)          NCSCC_RC_FAILURE
#endif

#ifndef m_NCS_OS_UNBUF_GETCHAR
#define m_NCS_OS_UNBUF_GETCHAR       getchar
#endif

#ifndef m_NCS_OS_NORMALIZE_CHR
#define m_NCS_OS_NORMALIZE_CHR       getchar
#endif

#ifndef m_NCS_OS_NTOHL
#define m_NCS_OS_NTOHL(x)            ntohl(x)
#endif

#ifndef m_NCS_OS_HTONL
#define m_NCS_OS_HTONL(x)            htonl(x)
#endif

#ifndef m_NCS_OS_NTOHS
#define m_NCS_OS_NTOHS(s)            ntohs(s)
#endif

#ifndef m_NCS_OS_HTONS
#define m_NCS_OS_HTONS(s)            htons(s)
#endif

/*********************************************************\
   m_NCS_OS_HTONLL_P :  Encodes a  64-bit integer into a 
                        byte stream in big-endian format.

   EXAMPLE :   
               { 
                  SaInt64T  my_long_long;
                  uint8_t      buff_64bit[8];
      
                  ...
                  m_NCS_OS_HTONLL_P(buff_64bit, my_long_long);
                  ...
               }

   NOTE :               Since there in no know "htonll()" or
                        equivalent, the macro has same definition 
                        irrespective of the 
                        NCS_CPU_MULTIBYTE_ACCESS_ALIGNMENT 
                        flag.

\*********************************************************/
#ifndef m_NCS_OS_HTONLL_P
#define m_NCS_OS_HTONLL_P(p8, in_long_long) {    \
     ((uint8_t*)p8)[0] = (uint8_t)(in_long_long>>56);  \
     ((uint8_t*)p8)[1] = (uint8_t)(in_long_long>>48);  \
     ((uint8_t*)p8)[2] = (uint8_t)(in_long_long>>40);  \
     ((uint8_t*)p8)[3] = (uint8_t)(in_long_long>>32);  \
     ((uint8_t*)p8)[4] = (uint8_t)(in_long_long>>24);  \
     ((uint8_t*)p8)[5] = (uint8_t)(in_long_long>>16);  \
     ((uint8_t*)p8)[6] = (uint8_t)(in_long_long>> 8);  \
     ((uint8_t*)p8)[7] = (uint8_t)(in_long_long    );  \
}
#endif

/*********************************************************\
   m_NCS_OS_NTOHLL_P :  Decodes a 64-bit integer from a 
                        big-endian encoded byte stream.

   EXAMPLE :   
               { 
                  SaInt64T  my_long_long;
                  uint8_t      buff_64bit[8];
      
                  ...
                  my_long_long = m_NCS_OS_NTOHLL_P(buff_64bit);
                  ...
               }

   NOTE :               Since there in no know "ntohll()" or
                        equivalent, the macro has same definition 
                        irrespective of the 
                        NCS_CPU_MULTIBYTE_ACCESS_ALIGNMENT 
                        flag.
\*********************************************************/
#ifndef m_NCS_OS_NTOHLL_P
#define m_NCS_OS_NTOHLL_P(p8) (               \
      ((uint64_t)((uint8_t*)(p8))[0] <<56) |        \
      ((uint64_t)((uint8_t*)(p8))[1] <<48) |        \
      ((uint64_t)((uint8_t*)(p8))[2] <<40) |        \
      ((uint64_t)((uint8_t*)(p8))[3] <<32) |        \
      ((uint64_t)((uint8_t*)(p8))[4] <<24) |        \
      ((uint64_t)((uint8_t*)(p8))[5] <<16) |        \
      ((uint64_t)((uint8_t*)(p8))[6] <<8 ) |        \
      ((uint64_t)((uint8_t*)(p8))[7]     )          \
     )
#endif

#if (NCS_CPU_MULTIBYTE_ACCESS_ALIGNMENT == 0)	/* no CPU alignment requirement */

#ifndef m_NCS_OS_NTOHL_P
#define m_NCS_OS_NTOHL_P(p8)         ntohl(*(uint32_t*)p8)
#endif

#ifndef m_NCS_OS_HTONL_P
#define m_NCS_OS_HTONL_P(p8,v32)     (*(uint32_t*)p8 = htonl(v32))
#endif

#ifndef m_NCS_OS_NTOHS_P
#define m_NCS_OS_NTOHS_P(p8)         ntohs(*(uns16*)p8)
#endif

#ifndef m_NCS_OS_HTONS_P
#define m_NCS_OS_HTONS_P(p8,v16)     (*(uns16*)p8 = htons(v16))
#endif
#else				/* CPU requires alignment access */

#ifndef m_NCS_OS_NTOHL_P
#define m_NCS_OS_NTOHL_P(p8) (uint32_t)((*(uint8_t*)p8<<24)|(*(uint8_t*)(p8+1)<<16)| \
                           (*(uint8_t*)(p8+2)<<8)|(*(uint8_t*)(p8+3)))
#endif

#ifndef m_NCS_OS_NTOHS_P
#define m_NCS_OS_NTOHS_P(p8) (uint16_t)((*(uint8_t*)p8<<8)|*((uint8_t*)(p8+1)))
#endif

#ifndef m_NCS_OS_HTONL_P
#define m_NCS_OS_HTONL_P(p8,v32) { \
     *p8     = (uint8_t)(v32>>24); \
     *(p8+1) = (uint8_t)(v32>>16); \
     *(p8+2) = (uint8_t)(v32>>8);  \
     *(p8+3) = (uint8_t)v32; }
#endif

#ifndef m_NCS_OS_HTONS_P
#define m_NCS_OS_HTONS_P(p8,v16) { \
     *p8     = (uint8_t)(v16>>8); \
     *(p8+1) = (uint8_t)v16; }
#endif
#endif   /* CPU alignment */

/*
 * Macro primitives to encode 2byte and 4byte values from host order into
 * a "uns32"-string in network order.
 */
#ifndef m_NCS_OS_HTONS_UNS32_P
#define m_NCS_OS_HTONS_UNS32_P(p32, v16)    \
      {*p32        = (v16 >> 8);           \
      *(p32 + 1)  =  (v16 & 0x00FF); }
#endif

#ifndef m_NCS_OS_HTONL_UNS32_P
#define m_NCS_OS_HTONL_UNS32_P(p32, v32)      \
      {*p32       = (v32 >>24) & 0x000000ff; \
      *(p32 + 1)  = (v32 >>16) & 0x000000ff; \
      *(p32 + 2)  = (v32 >>8) & 0x000000ff;  \
      *(p32 + 3)  =  v32 & 0x000000ff;}
#endif

/****************************************************************************
 **                                                                        **
 **                  Process Library Interface                             **
 **                                                                        **
 **                                                                        **
 ***************************************************************************/

	typedef uint64_t NCS_EXEC_HDL;
	typedef uint64_t NCS_EXEC_USR_HDL;

	/* This Structure assists in passing the environment arguments needed to be
	   set in the newly created process. */

	typedef struct ncs_os_environ_set_node_tag {
		char *name;
		char *value;
		int overwrite;	/* zero the value is not overwritten else overwritten */
	} NCS_OS_ENVIRON_SET_NODE;

	typedef struct ncs_os_environ_args_tag {
		unsigned int num_args;
		NCS_OS_ENVIRON_SET_NODE *env_arg;
	} NCS_OS_ENVIRON_ARGS;

	typedef enum {
		/* Exec of script failed (script not readable or path wrong) */
		NCS_OS_PROC_EXEC_FAIL,

		/* Exec of script success, and script exits with status zero */
		NCS_OS_PROC_EXIT_NORMAL,

		/* Script did not exit within time */
		NCS_OS_PROC_EXIT_WAIT_TIMEOUT,

		/* Exec of script success, but script exits with non-zero status */
		NCS_OS_PROC_EXIT_WITH_CODE,

		/* Exec of script success, but script exits due to a signal */
		NCS_OS_PROC_EXIT_ON_SIGNAL
	} NCS_OS_PROC_EXEC_STATUS;

	typedef struct {
		NCS_OS_PROC_EXEC_STATUS value;
		union {
			struct {
				uint32_t exit_code;
			} exit_with_code;

			struct {
				uint32_t signal_num;
			} exit_on_signal;
		} info;
	} NCS_OS_PROC_EXEC_STATUS_INFO;

/* CALLBACK structure definition */
	typedef struct NCS_OS_PROC_EXECUTE_TIMED_CB_INFO {
		NCS_EXEC_HDL i_exec_hdl;
		NCS_EXEC_USR_HDL i_usr_hdl;
		NCS_OS_PROC_EXEC_STATUS_INFO exec_stat;

	} NCS_OS_PROC_EXECUTE_TIMED_CB_INFO;

/* CALLBACK function prototype */
	typedef uint32_t (*NCS_OS_PROC_EXECUTE_CB) (NCS_OS_PROC_EXECUTE_TIMED_CB_INFO *);

/* REQUEST structure definition */
	typedef struct NCS_OS_PROC_EXECUTE_TIMED_INFO {
		/* INPUTS */
		char *i_script;	/* Command  */
		uint32_t i_argc;
		char **i_argv;
		NCS_OS_ENVIRON_ARGS *i_set_env_args;
		uint32_t i_timeout_in_ms;
		NCS_EXEC_USR_HDL i_usr_hdl;
		NCS_OS_PROC_EXECUTE_CB i_cb;
		/* OUTPUTS */
		NCS_EXEC_HDL o_exec_hdl;	/*  */
	} NCS_OS_PROC_EXECUTE_TIMED_INFO;

#define m_NCS_OS_PROCESS_EXECUTE_TIMED(arg)           ncs_os_process_execute_timed(arg)
#define m_NCS_OS_PROCESS_EXECUTE(script,argv)         ncs_os_process_execute((char *)script,(char **)argv,(NCS_OS_ENVIRON_ARGS *)NULL)
#define m_NCS_OS_PROCESS_TERMINATE(proc_id)           ncs_os_process_terminate((unsigned int)proc_id)

#define m_NCS_OS_EXECUTE_SCRIPT(script)               ncs_os_process_execute((char *)script,(char *)NULL,(NCS_OS_ENVIRON_ARGS *))

#define m_NCS_OS_PROCESS_SET_ENV_AND_EXECUTE(script,argv,set_env_args)         ncs_os_process_execute((char *)script,(char *)argv,(NCS_OS_ENVIRON_ARGS *)set_env_args)

#define m_NCS_SIGNAL(signal,handler)                  ncs_os_signal(signal,handler)

/* declarations */
	uint32_t ncs_os_process_execute_timed(NCS_OS_PROC_EXECUTE_TIMED_INFO *req);

	unsigned int ncs_os_process_execute(char *exec_mod, char *argv[], NCS_OS_ENVIRON_ARGS *set_env_args);

	int ncs_os_process_terminate(unsigned int proc_id);

	sighandler_t ncs_os_signal(int signalnum, sighandler_t handler);

/****************************************************************************
 **                                                                        **
 **                  Math Library Interface                                **
 **                                                                        **
 **                                                                        **
 ***************************************************************************/
/* The natural logarithm [ln(x)] */
#ifndef m_NCS_OS_LOG
#define m_NCS_OS_LOG(double_x)     log(double_x)
#endif

/* The exponential function [e raised to x] */
#ifndef m_NCS_OS_EXP
#define m_NCS_OS_EXP(double_x)     exp(double_x)
#endif

/*****************************************************************************
 **                                                                         **
 **                   Task Priorities                                       **
 **                                                                         **
 ****************************************************************************/
#ifndef NCS_OS_TASK_PRIORITY_0
#define NCS_OS_TASK_PRIORITY_0            0
#endif

#ifndef NCS_OS_TASK_PRIORITY_1
#define NCS_OS_TASK_PRIORITY_1           16
#endif

#ifndef NCS_OS_TASK_PRIORITY_2
#define NCS_OS_TASK_PRIORITY_2           32
#endif

#ifndef NCS_OS_TASK_PRIORITY_3
#define NCS_OS_TASK_PRIORITY_3           48
#endif

#ifndef NCS_OS_TASK_PRIORITY_4
#define NCS_OS_TASK_PRIORITY_4           63
#endif

#ifndef NCS_OS_TASK_PRIORITY_5
#define NCS_OS_TASK_PRIORITY_5           79
#endif

#ifndef NCS_OS_TASK_PRIORITY_6
#define NCS_OS_TASK_PRIORITY_6           95
#endif

#ifndef NCS_OS_TASK_PRIORITY_7
#define NCS_OS_TASK_PRIORITY_7          111
#endif

#ifndef NCS_OS_TASK_PRIORITY_8
#define NCS_OS_TASK_PRIORITY_8          127
#endif

#ifndef NCS_OS_TASK_PRIORITY_9
#define NCS_OS_TASK_PRIORITY_9          143
#endif

#ifndef NCS_OS_TASK_PRIORITY_10
#define NCS_OS_TASK_PRIORITY_10         159
#endif

#ifndef NCS_OS_TASK_PRIORITY_11
#define NCS_OS_TASK_PRIORITY_11         175
#endif

#ifndef NCS_OS_TASK_PRIORITY_12
#define NCS_OS_TASK_PRIORITY_12         191
#endif

#ifndef NCS_OS_TASK_PRIORITY_13
#define NCS_OS_TASK_PRIORITY_13         207
#endif

#ifndef NCS_OS_TASK_PRIORITY_14
#define NCS_OS_TASK_PRIORITY_14         223
#endif

#ifndef NCS_OS_TASK_PRIORITY_15
#define NCS_OS_TASK_PRIORITY_15         239
#endif

#ifndef NCS_OS_TASK_PRIORITY_16
#define NCS_OS_TASK_PRIORITY_16         255
#endif

/*****************************************************************************
 **                                                                         **
 **                              min and max                                **
 **                                                                         **
 ****************************************************************************/

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif   /* min */

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif   /* max */

#ifdef  __cplusplus
}
#endif

#endif   /* NCS_OSPRM_H */
