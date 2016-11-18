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

  MODULE NAME: NCS_SAF.H

..............................................................................

  DESCRIPTION:

  Contains substitutes for SAF definitions. These definitions apply when
  NCS_SAF flag is off (which prevents real SAF headers files to be included).

  ******************************************************************************
  */
#ifndef BASE_NCS_SAF_H_
#define BASE_NCS_SAF_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "osaf/saf/saAis.h"
#include "amf/saf/saAmf.h"

/* Macro for offset of value in the SaNameT structure.*/
#define m_OFFSET_VAL_SANAMET ((long)((uint8_t *)(((SaNameT *)0)->value)))

/*********************************************************************\

  Macros for manipulation of SaNameT structures.

      Motivation for these macros is the need to keep the "length" field
      in big-endian form. This ensures lexicographic ordering of PATRICIA
      get-next operations on trees that use SaNameT as keys.

      The following macros have been defined to manipulate on SaNameT
      structures. The  "length" of SaNameT should only be compared or
      modified using the following macros.

      The following macros should be used at subsystem entry and exit
      points to convert to and from internal storage format:
           m_HTON_SANAMET_LEN (snamet_len)
           m_NTOH_SANAMET_LEN (snamet_len)

      The following macros should be used internally to compare
      SaNameT structures.
           m_CMP_NORDER_SANAMET_LEN(sanamet_len1, sanamet_len2)
           m_CMP_HORDER_SANAMET_LEN(sanamet_len1, sanamet_len2)

           m_CMP_NORDER_SANAMET(sanamet1, sanamet2)
           m_CMP_HORDER_SANAMET(sanamet1, sanamet2)

      The examples below demonstrate the usage of these macros.

  Example usages:
      ========================================
      SaNameT  snt1, snt2;
      snt2.length = m_HTON_SANAMET_LEN(snt1.length);
      snt1.length = m_NTOH_SANAMET_LEN(snt2.length);

      ========================================
      greater_or_equal_nlen =
         (m_CMP_NORDER_SANAMET_LEN(  snt1.length,   snt2.length) >= 0);

      greater_or_equal_hlen =
         (m_CMP_HORDER_SANAMET_LEN(  snt1.length,   256) >= 0);

      ========================================
      greater_or_equal =
         (m_CMP_NORDER_SANAMET_LEN(  snt1,    snt2) >= 0);

      greater_or_equal =
         (m_CMP_HORDER_SANAMET_LEN(  snt1,    snt2) >= 0);

\********************************************************************/

#define m_HTON_SANAMET_LEN(snamet_len) (htons(snamet_len))
#define m_NTOH_SANAMET_LEN(snamet_len) (ntohs(snamet_len))

#define m_CMP_NORDER_SANAMET_LEN(sanamet_len1, sanamet_len2)    \
  (memcmp(&(sanamet_len1), &(sanamet_len2), 2))

/* memcmp avoided to allow  m_CMP_HORDER_SANAMET_LEN(x, 256) */
#define m_CMP_HORDER_SANAMET_LEN(sanamet_len1, sanamet_len2)    \
  (((sanamet_len1) > (sanamet_len2))? 1:                        \
   (((sanamet_len1) < (sanamet_len2))?-1:0))

/* Note: The following macro assumes that bytes beyond the name-length
   are all set to zero (PATRICIA will expect the same for get-next
   operations).
*/
#define m_CMP_NORDER_SANAMET(sanamet1, sanamet2)                        \
  (memcmp(&(sanamet1), &(sanamet2), (ntohs((sanamet1).length) + m_OFFSET_VAL_SANAMET)))

#define m_CMP_HORDER_SANAMET(sanamet1, sanamet2)                        \
  (                                                                     \
      ((sanamet1).length > (sanamet2).length)? 1:                       \
      (                                                                 \
          ((sanamet1).length < (sanamet2).length)?-1:                   \
          memcmp(&(sanamet1).value, &(sanamet2).value, (sanamet1).length) \
       )                                                                \
   )

#define m_CMP_HORDER_SAHCKEY(hckey1, hckey2)                    \
  (                                                             \
      ((hckey1).keyLen > (hckey2).keyLen) ? 1:                  \
      (                                                         \
          ((hckey1).keyLen < (hckey2).keyLen) ? -1:             \
          memcmp(&(hckey1).key, &(hckey2).key, (hckey1).keyLen) \
       )                                                        \
   )

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_SAF_H_
