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

  DESCRIPTION:  MDS layer initialization and destruction entry points

******************************************************************************
*/

#ifndef _MDS_DL_API_H_
#define _MDS_DL_API_H_

#include "ncs_lib.h"

uint32_t mds_lib_req(NCS_LIB_REQ_INFO *req);
int mds_auth_server_connect(const char *name, MDS_DEST mds_dest,
		int svc_id, int timeout);
int mds_auth_server_disconnect(const char *name, MDS_DEST mds_dest,
		int svc_id, int timeout);
int mds_auth_server_create(const char *name);

#endif
