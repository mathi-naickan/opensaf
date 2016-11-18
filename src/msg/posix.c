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

/***************************************************************************
 *
 * uint32_t ncs_os_posix_mq(NCS_OS_POSIX_MQ_REQ_INFO *req)
 * 
 *
 * Description:
 *   This routine handles operating system primitives for message-queues.
 *
 * Synopsis:
 *
 * Call Arguments:
 *    req ............... ....action request
 *
 * Returns:
 *   Returns NCSCC_RC_SUCCESS if successful, otherwise NCSCC_RC_FAILURE.
 *
 * Notes:
 *
 ****************************************************************************/
#include "msg/mqsv.h"
