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

#ifndef NTFIMCN_MAIN_H
#define	NTFIMCN_MAIN_H

#include "saAmf.h"
#include "saNtf.h"
#include "saImmOi.h"
#include "saImmOm.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define NTFIMCN_INTERNAL_ERROR (-1)

#define SA_NTF_VENDOR_ID_OSAF 32993

#define NTFIMCN_NOTIFYING_OBJECT	"safApp=OpenSaf"
#define NTFIMCN_ADMIN_OWNER_NAME	"SaImmAttrAdminOwnerName"
#define NTFIMCN_IMPLEMENTER_NAME	"SaImmAttrImplementerName"
#define NTFIMCN_CLASS_NAME			"SaImmAttrClassName"
#define NTFIMCN_CCB_ID				"SaImmOiCcbIdT"
#define NTFIMCN_CCB_LAST			"ccbLast"
#define NTFIMCN_IMM_ATTR			"SaImmAttr"

typedef struct {
	SaImmOiHandleT immOiHandle;				/* Handle from IMM OI initialize */
	SaImmHandleT immOmHandle;				/* Handle from IMM OM initialize */
	SaSelectionObjectT immSelectionObject;	/* Selection Object to wait for IMM events */
	SaNtfHandleT ntf_handle;				/* Handle from NTF initialize */
	SaAmfHAStateT haState;					/* Current HA state */
}ntfimcn_cb_t;

#define NTFIMCN_PROC_NAME "osafntfimcnd"

void imcn_exit(int status);

#ifdef	__cplusplus
}
#endif

#endif	/* NTFIMCN_MAIN_H */

