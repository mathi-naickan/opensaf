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

  DESCRIPTION:  This file declares utility macros.

  ******************************************************************************
  */
#ifndef BASE_NCS_UTIL_H_
#define BASE_NCS_UTIL_H_

#define m_NCS_ENC_MSG_FMT_GET(rem_ver,rem_ver_min,rem_ver_max,msg_fmt_array) \
  (((rem_ver) < (rem_ver_min))?0:                                       \
   (((rem_ver) >= (rem_ver_max))?((msg_fmt_array)[(rem_ver_max) - (rem_ver_min)]): \
    ((msg_fmt_array)[(rem_ver) - (rem_ver_min)])))

#define m_NCS_MSG_FORMAT_IS_VALID(                                      \
    msg_fmt_version,rem_ver_min,rem_ver_max,msg_fmt_array)              \
  (((msg_fmt_version) > (msg_fmt_array)[(rem_ver_max) - (rem_ver_min)])? \
   0:(((msg_fmt_version) < (msg_fmt_array)[0])?0:1))

#define m_NCS_MBCSV_FMT_GET(peer_mbcsv_ver,mbcsv_ver,mbcsv_ver_min)     \
  (((peer_mbcsv_ver) < (mbcsv_ver_min))?0:                              \
   (((peer_mbcsv_ver) >=(mbcsv_ver))?(mbcsv_ver):(peer_mbcsv_ver)))

#endif  // BASE_NCS_UTIL_H_
