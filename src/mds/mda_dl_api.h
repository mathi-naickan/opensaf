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

  ******************************************************************************
  */
#ifndef MDS_MDA_DL_API_H_
#define MDS_MDA_DL_API_H_

/***********************************************************************\
    mda_lib_req :  This API initializes MDA (MDS Destination agent code)
                   The MDA module implements the ADA/VDA APIs defined
                   in ncs_mda_papi.h
\***********************************************************************/
uint32_t mda_lib_req(NCS_LIB_REQ_INFO *req);

#endif  // MDS_MDA_DL_API_H_
