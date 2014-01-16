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
 * Author(s): Ericsson AB
 *
 */


#ifndef tet_imm_h
#define tet_imm_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <assert.h>
#include <utest.h>
#include <util.h>

/* Common globals */
extern const SaVersionT constImmVersion;
extern SaVersionT immVersion;
extern SaAisErrorT rc;
extern SaSelectionObjectT selectionObject;

/* Management globals */
extern SaImmHandleT immOmHandle;
extern SaImmCallbacksT immOmCallbacks;
extern SaImmCallbacksT_o2 immOmA2bCallbacks;
extern const SaImmClassNameT configClassName;
extern const SaImmClassNameT runtimeClassName;
extern const SaImmClassNameT nodanglingClassName;

/* Implementer globals */
extern SaImmHandleT immOiHandle;
extern SaImmOiCallbacksT_2 immOiCallbacks;

/* Common functions */
SaAisErrorT config_class_create(SaImmHandleT immHandle);
SaAisErrorT config_class_delete(SaImmHandleT immHandle);
SaAisErrorT runtime_class_create(SaImmHandleT immHandle);
SaAisErrorT runtime_class_delete(SaImmHandleT immHandle);
SaAisErrorT nodangling_class_create(SaImmHandleT immHandle);
SaAisErrorT nodangling_class_delete(SaImmHandleT immHandle);

SaAisErrorT object_create(SaImmHandleT immHandle,
		SaImmAdminOwnerHandleT ownerHandle,
        const SaImmClassNameT className,
        const SaNameT *rdnObj,
        const SaNameT *parentName,
        const SaImmAttrValuesT_2 *value);
SaAisErrorT object_delete(SaImmAdminOwnerHandleT ownerHandle,
        const SaNameT *dnObj,
        int strict);
SaAisErrorT object_create_2(SaImmHandleT immHandle,
        SaImmCcbHandleT ccbHandle,
        const SaImmClassNameT className,
        const SaNameT *rdnObj,
        const SaNameT *parentName,
        const SaImmAttrValuesT_2 *value);
SaAisErrorT object_delete_2(SaImmCcbHandleT ccbHandle,
        const SaNameT *dnObj,
        int strict);


/* Setup and cleanup functions */
extern void (*test_setup)(void);
extern void (*test_cleanup)(void);

#endif
