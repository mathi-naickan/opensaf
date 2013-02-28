/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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
 * Author(s): Ericsson AB
 *
 */

#ifndef NTFIMCN_NOTIFIER_H
#define	NTFIMCN_NOTIFIER_H

#include "saAis.h"
#include "immutil.h"
#include "ntfimcn_main.h"

#ifdef	__cplusplus
extern "C" {
#endif

int ntfimcn_ntf_init(ntfimcn_cb_t *cb);
int ntfimcn_send_object_create_notification(
		CcbUtilOperationData_t *CcbUtilOperationData, SaStringT rdn_attr_name,
		SaBoolT ccbLast);
int ntfimcn_send_object_delete_notification(CcbUtilOperationData_t *CcbUtilOperationData,
		const SaNameT *invoke_name,
		SaBoolT ccbLast);
int ntfimcn_send_object_modify_notification(CcbUtilOperationData_t *CcbUtilOperationData,
		SaNameT *invoker_name,
		SaBoolT ccbLast);
int ntfimcn_send_lost_cm_notification(void);

#ifdef	__cplusplus
}
#endif

#endif	/* NTFIMCN_NOTIFIER_H */

