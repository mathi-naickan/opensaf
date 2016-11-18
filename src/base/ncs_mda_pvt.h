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

  DESCRIPTION:  The VDEST-ID range is made of 3 ranges

                    (1) UNNAMED(fixed), INTERNAL USE : Defined in this file.
                    (2) UNNAMED(fixed), EXTERNAL USE : Defined in ncs_mda_papi.h
                    (3) NAMED, dynamically allocated : All other values.



                    ******************************************************************************
                    */
#ifndef BASE_NCS_MDA_PVT_H_
#define BASE_NCS_MDA_PVT_H_

/***********************************************************************\
         Range of unnamed VDEST values reserved for
         services external NCS implementation.
\***********************************************************************/
#define NCSVDA_INTERNAL_UNNAMED_MIN             1
#define NCSVDA_INTERNAL_UNNAMED_MAX          1000

/* External range is defined in "ncs_mda_papi.h" */

/* RESERVED VDEST values */
typedef enum {
  AVSV_VDEST_ID = NCSVDA_INTERNAL_UNNAMED_MIN,
  VDS_VDEST_ID,
  DTS_VDEST_ID_DEPRECATE,
  EDS_VDEST_ID,
  SPSV_VDEST_ID_UNUSED,

  GLD_VDEST_ID,

  HISV_VDEST_ID,
  SWITCH_VDEST_ID,
  CPD_VDEST_ID,
  MQD_VDEST_ID,
  LGS_VDEST_ID,
  AVND_VDEST_ID,
  IMMD_VDEST_ID,
  NTFS_VDEST_ID,
  SMFD_VDEST_ID,
  CLMS_VDEST_ID,
  PLMS_VDEST_ID,
  MAX_INTERNAL_FIXED_VDEST = NCSVDA_INTERNAL_UNNAMED_MAX
} NCS_INTERNAL_FIXED_VDEST;

#endif  // BASE_NCS_MDA_PVT_H_
