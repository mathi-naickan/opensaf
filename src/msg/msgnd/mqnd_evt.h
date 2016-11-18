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

#ifndef MSG_MSGND_MQND_EVT_H_
#define MSG_MSGND_MQND_EVT_H_

void mqnd_process_evt(MQSV_EVT *evt);
void mqnd_process_dsend_evt(MQSV_DSEND_EVT *evt);

#endif  // MSG_MSGND_MQND_EVT_H_
