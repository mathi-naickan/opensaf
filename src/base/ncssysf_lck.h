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

 @@@@@  @     @  @@@@@  @@@@@@@         @        @@@@@  @    @          @     @
@     @  @   @  @     @ @               @       @     @ @   @           @     @
@         @ @   @       @               @       @       @  @            @     @
 @@@@@     @     @@@@@  @@@@@           @       @       @@@             @@@@@@@
      @    @          @ @               @       @       @  @      @@@   @     @
@     @    @    @     @ @               @       @     @ @   @     @@@   @     @
 @@@@@     @     @@@@@  @       @@@@@@@ @@@@@@@  @@@@@  @    @    @@@   @     @

..............................................................................

  DESCRIPTION:

  This module contains target system specific declarations related to
  resource locking using abstract semaphores.

..............................................................................
*/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSSYSF_LCK_H_
#define BASE_NCSSYSF_LCK_H_

#include "base/ncs_svd.h"

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **                   Lock Interface Definitions                           **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Depending on the target system, you may need to implement 'LOCK'       **
 ** macros to provide read and add/delete locking to various portions of   **
 ** the H&J subsystem.  This would especially be true if multiple          **
 ** processes (with preemption, or on multiple PROCESSORs) are active.     **
 **                                                                        **
 ** H&J Relies on a simple locking primitives.  The NCS_LOCK object type    **
 ** needs to be defined by you (below) if locking is required.             **
 **                                                                        **
 ** There are two versions of LOCK macros.                                 **
 **   (1) Original: provide only NCS_LOCK * and READ/Write Flag             **
 **   (2) V2      : The version 2 locks provide some additional info       **
 **                 specifying the Subsystem ID (sid) and the              **
 **                 Specific Lock ID (lid).  This additional information   **
 **                 is provided to allow selective use of locks, based on  **
 **                 either the Subsystem (Signalling, ILMI, LEC, etc) or   **
 **                 on a particular lock within a subsystem - e.g., don't  **
 **                 bother to implement locks on QSCB chains.              **
 **                                                                        **
 ** Our OSPRIMS interface does not include these additional Version 2      **
 ** arguments.                                                             **
 **                                                                        **
 ** A implementation desiring to use this selective locking feature will   **
 ** need to implement its own OS_SVCS lock service.                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

#define NCS_LOCKS_NEEDED 1
#define NCS_LOCK_READ    1
#define NCS_LOCK_WRITE   2

#if (NCSSYSM_LOCK_DBG_ENABLE != 0)      /* Debug locks.. Check for existence */

#define NCS_LOCK_EXISTS 0xAABBCCDD

typedef struct ncs_lock_dbg {
  uint32_t exists;        /* exists marker   */
  NCS_OS_LOCK lock;       /* the 'real' lock */

} NCS_LOCK_DBG;

#define NCS_LOCK  NCS_LOCK_DBG
#else

#define NCS_LOCK  NCS_OS_LOCK
#endif

/****************************************************************************
 * Initialize a LOCK
 *
 * uint32_t m_NCS_LOCK_INIT(lock)
 * uint32_t m_NCS_LOCK_INIT_V2(lock,sid,lid)
 * This function must be invoked before using a lock for the first time.
 *
 * Function arguments
 *  'lock' a pointer to an NCS_LOCK.
 *  'sid'  an enumerated H&J Subsystem IDentifier, NCS_SERVICE_ID.
 *  'lid'  an enumerated Lock IDentifier, private to each subsystem.
 *
 * Macro return codes
 *   NCSCC_RC_SUCCESS
 *   NCSCC_RC_FAILURE
 *
 ***************************************************************************/

#if (NCSSYSM_LOCK_DBG_ENABLE != 0)
#define m_NCS_LOCK_INIT(lock)                   \
  ncs_lock_init(lock, 0, 0, __LINE__, __FILE__)
#define m_NCS_LOCK_INIT_V2(lock,sid,lid)                \
  ncs_lock_init(lock, sid, lid, __LINE__, __FILE__)
#else
#define m_NCS_LOCK_INIT(lock)            m_NCS_OS_LOCK(lock,NCS_OS_LOCK_CREATE,0)
#define m_NCS_LOCK_INIT_V2(lock,sid,lid) m_NCS_OS_LOCK(lock,NCS_OS_LOCK_CREATE,0)
#endif

/****************************************************************************
 * Destroy a LOCK
 *
 * uint32_t m_NCS_LOCK_DESTROY(lock)
 * uint32_t m_NCS_LOCK_DESROY_V2(lock,sid,lid)
 * This function must be invoked to release any resources.
 *
 * Function arguments
 *  'lock' a pointer to an NCS_LOCK.
 *  'sid'  an enumerated H&J Subsystem IDentifier, NCS_SERVICE_ID.
 *  'lid'  an enumerated Lock IDentifier, private to each subsystem.
 *
 * Macro return codes
 *   NCSCC_RC_SUCCESS
 *   NCSCC_RC_FAILURE
 *
 ***************************************************************************/

#if (NCSSYSM_LOCK_DBG_ENABLE != 0)
#define m_NCS_LOCK_DESTROY(lock)                        \
  ncs_lock_destroy(lock, 0, 0, __LINE__, __FILE__)
#define m_NCS_LOCK_DESTROY_V2(lock,sid,lid)             \
  ncs_lock_destroy(lock, sid, lid, __LINE__, __FILE__)
#else
#define m_NCS_LOCK_DESTROY(lock)            m_NCS_OS_LOCK(lock,NCS_OS_LOCK_RELEASE,0)
#define m_NCS_LOCK_DESTROY_V2(lock,sid,lid) m_NCS_OS_LOCK(lock,NCS_OS_LOCK_RELEASE,0)
#endif

/****************************************************************************
 * LOCK a Resource
 *
 * uint32_t m_NCS_LOCK(lock, flag)
 * uint32_t m_NCS_LOCK_V2(lock, flag, sid, lid)
 * This function must be invoked to lock a resource based on the flag:
 *
 *   flag= NCS_LOCK_READ:  Requests a read-only lock.  Any number of
 *                          'readers' may have access at a time, but
 *                          if any process has a 'write' lock, this
 *                          macro must BLOCK (suspend, whatever...)
 *                          until the process doing the WRITE gives up the
 *                          lock.
 *   flag= NCS_LOCK_WRITE: Requests a read/write lock.  Only one process
 *                          can write at a time.  If any processes have
 *                          ANY lock, this macro must BLOCK until the
 *                          semaphore (or whatever) is freed up completely.
 *
 * Function arguments
 *  'lock' a pointer to an NCS_LOCK.
 *  'flag' either NCS_LOCK_READ or NCS_LOCK_WRITE
 *  'sid'  an enumerated H&J Subsystem IDentifier, NCS_SERVICE_ID.
 *  'lid'  an enumerated Lock IDentifier, private to each subsystem.
 *
 * Macro return codes
 *   NCSCC_RC_SUCCESS
 *   NCSCC_RC_FAILURE
 *
 ***************************************************************************/
#if (NCSSYSM_LOCK_DBG_ENABLE != 0)
#define m_NCS_LOCK(lock, flag)           ncs_lock(lock,flag,0,0,__LINE__,__FILE__)
#define m_NCS_LOCK_V2(lock,flag,sid,lid) ncs_lock(lock,flag,sid,lid,__LINE__,__FILE__)
#else
#define m_NCS_LOCK(lock, flag)           m_NCS_OS_LOCK(lock,NCS_OS_LOCK_LOCK,flag)
#define m_NCS_LOCK_V2(lock,flag,sid,lid) m_NCS_OS_LOCK(lock,NCS_OS_LOCK_LOCK,flag)
#endif

/****************************************************************************
 * UNLOCK a Resource
 *
 * uint32_t m_NCS_UNLOCK(lock, flag)
 * uint32_t m_NCS_UNLOCK_V2(lock, flag, sid, lid)
 * This function must be invoked to unlock a resource
Your code
*                          must be capable of UNBLOCKING any processes
*                          who are waiting for the lockstruct. *
* Function arguments
*  'lock' a pointer to an NCS_LOCK.
*  'flag' either NCS_LOCK_READ or NCS_LOCK_WRITE
*  'sid'  an enumerated H&J Subsystem IDentifier, NCS_SERVICE_ID.
*  'lid'  an enumerated Lock IDentifier, private to each subsystem.
*
* Macro return codes
*   NCSCC_RC_SUCCESS
*   NCSCC_RC_FAILURE
*
***************************************************************************/
#if (NCSSYSM_LOCK_DBG_ENABLE != 0)
#define m_NCS_UNLOCK(lock, flag)           ncs_unlock(lock,flag,0,0,__LINE__,__FILE__)
#define m_NCS_UNLOCK_V2(lock,flag,sid,lid) ncs_unlock(lock,flag,sid,lid,__LINE__,__FILE__)
#else
#define m_NCS_UNLOCK(lock,flag)            m_NCS_OS_LOCK(lock,NCS_OS_LOCK_UNLOCK,flag)
#define m_NCS_UNLOCK_V2(lock,flag,sid,lid) m_NCS_OS_LOCK(lock,NCS_OS_LOCK_UNLOCK,flag)
#endif

/****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 **                                                                        **
 **                                                                        **
 **             Reference Count Atomic Increment/Decrement                 **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Reference counts are used by H&J code to ensure that objects set for   **
 ** deletion are deleted only when the reference count associated with the **
 ** object goes tozero. The reference counts need to be incremented or     **
 ** decremented in an atomic fashion. The following macros need to ensure  **
 ** that the operations are atomic.                                        **
 **                                                                        **
 **                                                                        **
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 ***************************************************************************/

#define m_NCS_ATOMIC_INC(c)  m_NCS_OS_ATOMIC_INC(c)
#define m_NCS_ATOMIC_DEC(c)  m_NCS_OS_ATOMIC_DEC(c)

#if (NCSSYSM_LOCK_DBG_ENABLE != 0)
unsigned int ncs_lock_init(NCS_LOCK * lock,
                           NCS_SERVICE_ID service_id,
                           unsigned int local_id, unsigned int line, char *file);

unsigned int ncs_lock_destroy(NCS_LOCK * lock,
                              NCS_SERVICE_ID service_id,
                              unsigned int local_id, unsigned int line, char *file);

unsigned int ncs_lock(NCS_LOCK * lock,
                      unsigned int flag,
                      NCS_SERVICE_ID service_id,
                      unsigned int local_id, unsigned int line, char *file);

unsigned int ncs_unlock(NCS_LOCK * lock,
                        unsigned int flag,
                        NCS_SERVICE_ID service_id,
                        unsigned int local_id, unsigned int line, char *file);
#endif
uint32_t ncs_lock_get_init_count(NCS_SERVICE_ID service_id);
uint32_t ncs_lock_get_destroy_count(NCS_SERVICE_ID service_id);
void ncs_lock_stats(char *filename);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSSYSF_LCK_H_
