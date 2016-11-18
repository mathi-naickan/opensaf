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

  This module contains definitions for H&J Subsystem and Service ID's, and
  the NCS_SERVICE_DESCRIPTOR structure.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */

#ifndef BASE_NCS_SVD_H_
#define BASE_NCS_SVD_H_

#include "base/ncsgl_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

/************************************************************************

  H & J   S u b s y s t e m   or   S e r v i c e

                  I d e n t i f i e r s

*************************************************************************/

typedef enum ncs_service_id {
  /** The H & J Subsystem and Service Identifiers
   **/
  NCS_SERVICE_ID_BASE = 0,
  NCS_SERVICE_ID_DUMMY = NCS_SERVICE_ID_BASE,
  NCS_SERVICE_ID_LEAP_TMR,
  NCS_SERVICE_ID_MDS,
  NCS_SERVICE_ID_COMMON,
  NCS_SERVICE_ID_OS_SVCS,
  NCS_SERVICE_ID_TARGSVCS,
  NCS_SERVICE_ID_SOCKET,
  NCS_SERVICE_ID_L2SOCKET,
  NCS_SERVICE_ID_DTSV_DEPRECATE,
  NCS_SERVICE_ID_AVSV,
  NCS_SERVICE_ID_AVD,
  NCS_SERVICE_ID_AVND,
  NCS_SERVICE_ID_AVA,
  NCS_SERVICE_ID_CLA,
  NCS_SERVICE_ID_GLD,
  NCS_SERVICE_ID_GLND,
  NCS_SERVICE_ID_GLA,
  NCS_SERVICE_ID_EDA,
  NCS_SERVICE_ID_EDS,
  NCS_SERVICE_ID_MQA,
  NCS_SERVICE_ID_MQND,
  NCS_SERVICE_ID_MQD,
  NCS_SERVICE_ID_CPA,
  NCS_SERVICE_ID_CPND,
  NCS_SERVICE_ID_CPD,
  NCS_SERVICE_ID_SAF_COMMON,
  NCS_SERVICE_ID_HCD,
  NCS_SERVICE_ID_HPL,
  NCS_SERVICE_ID_MBCSV,

  UD_SERVICE_ID_STUB1,
  UD_SERVICE_ID_STUB2,
  NCS_SERVICE_ID_VDS,
  NCS_SERVICE_ID_RDE,

  UD_DTSV_DEMO_SERVICE1,
  UD_DTSV_DEMO_SERVICE2,

  NCS_SERVICE_ID_LGA,     /* SAF LOG Service */
  NCS_SERVICE_ID_LGS,

  NCS_SERVICE_ID_FMA_GFM,
  NCS_SERVICE_ID_FMA,
  NCS_SERVICE_ID_GFM,

  NCS_SERVICE_ID_IMMA,
  NCS_SERVICE_ID_IMMND,
  NCS_SERVICE_ID_IMMD,

  NCS_SERVICE_ID_NTFA,
  NCS_SERVICE_ID_NTFS,

  NCS_SERVICE_ID_PLMA,
  NCS_SERVICE_ID_PLMS,

  NCS_SERVICE_ID_CLMA,
  NCS_SERVICE_ID_CLMS,

  NCS_SERVICE_ID_MAX,
  NCS_SERVICE_ID_END = 1000,

  NCS_SERVICE_ID_INTERNAL_BASE,
  /* All Motorola internal non-NCS service-ids go in this range */
  NCS_SERVICE_ID_INTERNAL_END = 2000,

  UD_SERVICE_ID_BASE,
  /* All Motorola external service-ids should come from this range */
  UD_SERVICE_ID_MAX,
  UD_SERVICE_ID_END = 3000
} NCS_SERVICE_ID;

typedef NCS_SERVICE_ID SS_SVC_ID;       /* Subsystem Service ID     */

/************************************************************************

  H & J   K e y

 An abstraction used to identify 'instance'. Its 'val' content can be
 attributed to a particular object (type) in a particular subsystem
 (svc). The value (val) serves as the qualifier that identifies 'which
 instance' of this <svc>.<type> object we are referring to.

*************************************************************************/

#define SYSF_MAX_KEY_LEN  42    /* picked as 'big enough' so far */

typedef struct ncs_octstr {
  uint8_t len;
  uint8_t data[SYSF_MAX_KEY_LEN];
} NCS_OCTSTR;

typedef struct ncs_key {
  NCS_SERVICE_ID svc;     /* an object based in this service */
  uint8_t type;   /* type or component of service   */
  uint8_t fmat;   /* value format NUM|HDL|STR|OCT   */

  union {
    uint32_t num;
    NCSCONTEXT hdl;
    NCS_OCTSTR oct;
    uint8_t str[SYSF_MAX_KEY_LEN];  /* null terminated string       */
  } val;

} NCS_KEY;

/* NCS_KEY::fmat values */

#define NCS_FMT_EMPTY 0
#define NCS_FMT_NUM   1
#define NCS_FMT_HDL   2
#define NCS_FMT_STR   3
#define NCS_FMT_OCT   4


#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_SVD_H_
