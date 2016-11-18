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

  This file consists of:
    1) Definations not defined in SAF pub_inc files
    2) Validation macors
    3) Manipulation macros
  used by all SAF Services

  ******************************************************************************
  */

#ifndef BASE_SAF_DEF_H_
#define BASE_SAF_DEF_H_

/* Macro to Validate the timeout value passed in SAF APIs (TRUE/FALSE) */

#define m_NCS_SA_IS_VALID_TIME_DURATION(time)                   \
  (((time) >= SA_TIME_BEGIN) && ((time) <= SA_TIME_MAX))

#define NCS_SAF_MIN_ACCEPT_TIME  10     /* In Milli Seconds */

/* Macro to convert the nano sec to 10 milli seconds */

#define m_NCS_CONVERT_SATIME_TO_TEN_MILLI_SEC(t)      (t)/(10000000)    /* 10^7 */

#endif  // BASE_SAF_DEF_H_
