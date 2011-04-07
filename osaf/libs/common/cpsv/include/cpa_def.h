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

#ifndef CPA_DEF_H
#define CPA_DEF_H

/* Macros for Validating Version */
#define CPA_RELEASE_CODE 'B'
#define CPA_MAJOR_VERSION 0x02
#define CPA_MINOR_VERSION 0x02
#define CPA_BASE_MAJOR_VERSION 0x01
#define CPA_BASE_MINOR_VERSION 0x01

#define m_CPA_VER_IS_VALID(ver) \
   ((ver->releaseCode == CPA_RELEASE_CODE) && \
     (ver->majorVersion == CPA_MAJOR_VERSION || ver->majorVersion == 0xff) && \
     (ver->minorVersion == CPA_MINOR_VERSION || ver->minorVersion == 0xff) )

#define CPSV_WAIT_TIME  1000	/* MDS wait time in case of syncronous call */

#define CPSV_MIN_DATA_SIZE  10000
#define CPSV_AVG_DATA_SIZE  1000000
#define CPA_WAIT_TIME(datasize) ((datasize<CPSV_MIN_DATA_SIZE)?400:600+((datasize/CPSV_AVG_DATA_SIZE)*400))

#define m_CPSV_CONVERT_SATIME_TEN_MILLI_SEC(t)      (t)/(10000000)	/* 10^7 */

#endif
