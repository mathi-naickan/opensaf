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

/*****************************************************************************/

#ifndef MSG_MSGD_MQD_IMM_H_
#define MSG_MSGD_MQD_IMM_H_

#include "osaf/immutil/immutil.h"
#include "imm/saf/saImm.h"
#include "msg/msgd/mqd.h"

SaAisErrorT mqd_imm_initialize(MQD_CB *cb);
SaAisErrorT mqd_create_runtime_MqGrpObj(MQD_OBJ_NODE *pNode, SaImmOiHandleT immOiHandle);
void mqd_runtime_update_grpmembers_attr(MQD_CB *pMqd, MQD_OBJ_NODE *pObjNode);

#endif  // MSG_MSGD_MQD_IMM_H_
