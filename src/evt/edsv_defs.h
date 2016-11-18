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

#ifndef EVT_EDSV_DEFS_H_
#define EVT_EDSV_DEFS_H_

#include "base/ncs_util.h"

#define EDS_TASK_PRIORITY     5
#define EDS_STACKSIZE         NCS_STACKSIZE_HUGE

#define EDA_POOL_ID           1

#define EDSV_RELEASE_CODE 'B'
#define EDSV_MAJOR_VERSION 0x03
#define EDSV_MINOR_VERSION 0x01
#define EDSV_BASE_MAJOR_VERSION 0x01
#define EDSV_BASE_MINOR_VERSION 0x01

/* IMMSv Defs */
#define EDSV_IMM_RELEASE_CODE 'A'
#define EDSV_IMM_MAJOR_VERSION 0x02
#define EDSV_IMM_MINOR_VERSION 0x01

#define EDSV_IMM_IMPLEMENTER_NAME (SaImmOiImplementerNameT) "safEvtService"

/* Macro to validate the EVT version */
#define m_EDA_VER_IS_VALID(ver)   \
       (ver->releaseCode  == EDSV_RELEASE_CODE) && \
       ((ver->majorVersion == EDSV_MAJOR_VERSION) || \
        (ver->majorVersion == EDSV_BASE_MAJOR_VERSION))

#define m_IS_B03_CLIENT(version)  \
   (version->releaseCode == 'B') && (version->majorVersion >= 0x03)

#define m_EDA_FILL_VERSION(ver) \
         ver->releaseCode = EDSV_RELEASE_CODE;\
         ver->majorVersion = EDSV_MAJOR_VERSION;\
         ver->minorVersion = EDSV_MINOR_VERSION;

/* Macro to validate the dispatch flags */
#define m_EDA_DISPATCH_FLAG_IS_VALID(flag) \
   ( (SA_DISPATCH_ONE == flag) || \
     (SA_DISPATCH_ALL == flag) || \
     (SA_DISPATCH_BLOCKING == flag) )

#define m_GET_MY_VERSION(ver) \
     ver.releaseCode = 'B';  \
     ver.majorVersion = 0x03;  \
     ver.minorVersion = 0x01;

#define m_GET_MY_VENDOR(vendor) \
     vendor.length=13; \
     memset(&vendor.value,'\0',SA_MAX_NAME_LENGTH); \
     memcpy(&vendor.value,(uint8_t *)"OpenSAF",7);

/** Macro to validate the channel open flags **/
#define m_EDA_CHAN_OPEN_FLAG_IS_VALID(flag) \
   ( (SA_EVT_CHANNEL_CREATE     & flag) ||  \
     (SA_EVT_CHANNEL_PUBLISHER  & flag) ||  \
     (SA_EVT_CHANNEL_SUBSCRIBER & flag) )

/**** Verbose macro definitions for EDSv ****/
#define EDSV_NANOSEC_TO_LEAPTM 10000000

#define m_EDSV_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;

#define m_EDSV_GET_CLM_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x01;

/* Define our limits */
/* EDSv maximum channels for this implementation */
#define EDSV_MAX_CHANNELS      1024

/* EDSv maximum length of data of an event(in bytes) */
#define SA_EVENT_DATA_MAX_SIZE   65536

/* EDSv maximum length of a pattern size */
#define EDSV_MAX_PATTERN_SIZE    255

/* EDSv maximum number of patterns */
#define EDSV_MAX_PATTERNS       10

/* EDSv maximum event retention time */
#define EDSV_MAX_RETENTION_TIME  100000000000000.0	/* Revisit This */

#endif  // EVT_EDSV_DEFS_H_
