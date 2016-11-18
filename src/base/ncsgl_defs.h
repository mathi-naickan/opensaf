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

  This module contains data type defs usable throughout the system.

******************************************************************************/

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSGL_DEFS_H_
#define BASE_NCSGL_DEFS_H_

#include <inttypes.h>
#include <stdbool.h>

#ifdef  __cplusplus
extern "C" {
#endif

/*****************************************************************************

                             BASIC DECLARATIONS

****************************************************************************/

typedef void* NCSCONTEXT;       /* opaque context between svc-usr/svc-provider */

#define NCS_PTR_TO_INT32_CAST(x)   ((int32_t)(long)(x))
#define NCS_PTR_TO_UNS64_CAST(x)   ((uint64_t)(long)(x))
#define NCS_PTR_TO_UNS32_CAST(x)   ((uint32_t)(long)(x))
#define NCS_INT32_TO_PTR_CAST(x)   ((void*)(long)(x))
#define NCS_INT64_TO_PTR_CAST(x)   ((void*)(long)(x))
#define NCS_UNS32_TO_PTR_CAST(x)   ((void*)(long)(x))


/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  Generic Function Return Codes

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define NCSCC_RC_SUCCESS               1
#define NCSCC_RC_FAILURE               2
#define NCSCC_RC_BAD_ATTR              7
#define NCSCC_RC_INV_VAL              16
#define NCSCC_RC_NO_OBJECT            18
#define NCSCC_RC_OUT_OF_MEM           21
#define NCSCC_RC_DISABLED            127
#define NCSCC_RC_TMR_STOPPED         128
#define NCSCC_RC_REQ_TIMOUT          131
#define NCSCC_RC_NO_CREATION         134
#define NCSCC_RC_INVALID_INPUT       137
#define NCSCC_RC_CONTINUE           1023
#define NCSCC_RC_DUPLICATE_ENTRY    2011

/*************************************************
 * Maximum Slots (Including sub slots) supported
 * 256 slots x 16 subslots
 *************************************************/
enum {
  kMaxSlotsSubslots = 4096
};

typedef uint64_t MDS_DEST;
typedef uint32_t NCS_NODE_ID;
typedef uint8_t NCS_CHASSIS_ID;
typedef uint32_t SlotSubslotId;

/* m_NCS_NODE_ID_FROM_MDS_DEST: Returns node-id if the MDS_DEST provided
   is an absolute destination. Returns 0
   if the MDS_DEST provided is a virtual
   destination.
*/
#define m_NCS_NODE_ID_FROM_MDS_DEST(mdsdest) ((uint32_t) (((uint64_t)(mdsdest))>>32))

#ifdef  __cplusplus
}
#endif

#ifdef  __cplusplus
extern "C" {
extern void __osafassert_fail(const char *__file, int __line, const char* __func, const char *__assertion);
#define __OSAFASSERT_VOID_CAST static_cast<void>
}
#else
extern void __osafassert_fail(const char *__file, int __line, const char* __func, const char *__assertion);
#define __OSAFASSERT_VOID_CAST (void)
#endif

#define osafassert(expr)                                                \
  ((expr) ? __OSAFASSERT_VOID_CAST (0) : __osafassert_fail (__FILE__, __LINE__, __FUNCTION__, #expr))

#endif  // BASE_NCSGL_DEFS_H_
