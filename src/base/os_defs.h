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

  DESCRIPTION:

  This module contains old legacy interfaces
  TODO: clean up and eventually (re)moved

  ******************************************************************************
  */
#ifndef BASE_OS_DEFS_H_
#define BASE_OS_DEFS_H_

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **                     OPERATING SYSTEM INCLUDE FILES                      **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/msg.h>            /* Added for message-queues : PM : 28/10/03 */
#include <syslog.h>
#ifndef __USE_GNU
struct msgbuf {
  long int mtype;
  char mtext[1];
};
#endif   /* else defined in <sys/msg.h> */
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <signal.h>
#include <pthread.h>

#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <termios.h>

#include <assert.h>
#include <sys/wait.h>

#ifdef  __cplusplus
extern "C" {
#endif

extern void ncs_os_atomic_init(void);
extern void ncs_os_atomic_destroy(void);

extern void ncs_os_atomic_inc(uint32_t *puns32);
#define m_NCS_OS_ATOMIC_INC(p_uns32) ncs_os_atomic_inc(p_uns32);

extern void ncs_os_atomic_dec(uint32_t *p_uns32);
#define m_NCS_OS_ATOMIC_DEC(p_uns32) ncs_os_atomic_dec(p_uns32);

/*****************************************************************************
 **                                                                         **
 **             Message queue primitive defintions                          **
 **                                                                         **
 ****************************************************************************/
#define NCS_OS_MQ_KEY         key_t
#define NCS_OS_MQ_HDL         int
#define NCS_OS_MQ_MSG_LL_HDR  long      /* The "header" part of <struct msgbuf> */
#define NCS_OS_MQ_MAX_PAYLOAD 4500      /* This is actually dependant on
                                           configuration. Usual defaults in LINUX
                                           are about 8192. : Phani:28/10/03 */
/*****************************************************************************
 **                                                                         **
 **             POSIX Message queue primitive defintions                          **
 **                                                                         **
 ****************************************************************************/
#define NCS_OS_POSIX_MQD      uint32_t
#define NCS_OS_POSIX_TIMESPEC struct timespec

#define __NCSINC_LINUX__

/*****************************************************************************
 **                                                                         **
 **                                                                         **
 **                   System Timestamp Interface Macros                     **
 **                                                                         **
 **                                                                         **
 ****************************************************************************/
int64_t ncs_os_time_ms(void);
#define m_NCS_GET_TIME_MS   ncs_os_time_ms()

uint64_t ncs_os_time_ns(void);
#define m_NCS_GET_TIME_NS   ncs_os_time_ns()

/****************************************************************************
 *
 * POSIX Locks - override OS_PRIMS default
 *
 ***************************************************************************/

typedef struct posixlock {
  pthread_mutex_t lock;
  unsigned long count;
  pthread_t tid;
} POSIXLOCK;

#define NCS_OS_LOCK POSIXLOCK

extern NCS_OS_LOCK *get_cloexec_lock(void);

extern FILE *ncs_os_fopen(const char *fpath, const char *fmode);

#define m_NCS_OS_NTOHL_P(p8) (uint32_t)((*(uint8_t*)p8<<24)|(*(uint8_t*)(p8+1)<<16)| \
                                        (*(uint8_t*)(p8+2)<<8)|(*(uint8_t*)(p8+3)))
#define m_NCS_OS_NTOHS_P(p8) (uint16_t)((*(uint8_t*)p8<<8)|*((uint8_t*)(p8+1)))

#define m_NCS_OS_HTONL_P(p8,v32) {              \
    *p8     = (uint8_t)(v32>>24);               \
    *(p8+1) = (uint8_t)(v32>>16);               \
    *(p8+2) = (uint8_t)(v32>>8);                \
    *(p8+3) = (uint8_t)v32; }

#define m_NCS_OS_HTONS_P(p8,v16) {              \
    *p8     = (uint8_t)(v16>>8);                \
    *(p8+1) = (uint8_t)v16; }

/*****************************************************************************
 **                                                                         **
 **             ncs_sel_obj_*  primitive definitions                        **
 **                                                                         **
 ****************************************************************************/
typedef struct ncs_sel_obj {
  int raise_obj;
  int rmv_obj;
} NCS_SEL_OBJ;

#ifdef  __cplusplus
}
#endif

#endif  // BASE_OS_DEFS_H_
