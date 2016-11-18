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

#ifndef MSG_MSGND_MQND_IMM_H_
#define MSG_MSGND_MQND_IMM_H_

#include "osaf/immutil/immutil.h"
#include "imm/saf/saImm.h"

extern void mqnd_imm_declare_implementer(MQND_CB *cb);
extern void mqnd_imm_reinit_bg(MQND_CB * cb);
extern SaAisErrorT mqnd_imm_initialize(MQND_CB *cb);
extern SaAisErrorT mqnd_create_runtime_MsgQobject(char *rname, SaTimeT time, MQND_QUEUE_NODE *qnode,
					   SaImmOiHandleT immOiHandle);
extern SaAisErrorT mqnd_create_runtime_MsgQPriorityobject(char *rname, MQND_QUEUE_NODE *qnode, SaImmOiHandleT immOiHandle);

#endif  // MSG_MSGND_MQND_IMM_H_
