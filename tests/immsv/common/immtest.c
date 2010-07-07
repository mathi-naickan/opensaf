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

#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "immtest.h"

SaVersionT immVersion = {'A', 0x02, 0x01}; 
SaAisErrorT rc;
SaImmHandleT immOmHandle;
SaImmHandleT immOiHandle;
SaImmOiCallbacksT_2 immOiCallbacks;
SaImmCallbacksT immOmCallbacks = {NULL};
SaSelectionObjectT selectionObject;

const SaImmClassNameT configClassName = (SaImmClassNameT) "TestClassConfig";
const SaImmClassNameT runtimeClassName = "TestClassRuntime";

int main(int argc, char **argv) 
{
    int suite = ALL_SUITES, tcase = ALL_TESTS;

    srandom(getpid());

    if (argc > 1)
    {
        suite = atoi(argv[1]);
    }

    if (argc > 2)
    {
        tcase = atoi(argv[2]);
    }

    if (suite == 0)
    {
        test_list();
        return 0;
    }

    return test_run(suite, tcase);
}  

SaAisErrorT config_class_create(SaImmHandleT immHandle)
{
    SaAisErrorT err=SA_AIS_OK;
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE,
	NULL};

    SaImmAttrDefinitionT_2 attr2 = {
        "attr2", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, &attr2, NULL};

    err = saImmOmClassCreate_2(immHandle, configClassName, SA_IMM_CLASS_CONFIG,
        attributes);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_EXIST)) {
        return SA_AIS_OK;
    }

    return rc;
}

SaAisErrorT config_class_delete(SaImmHandleT immHandle)
{
    SaAisErrorT err = saImmOmClassDelete(immHandle, configClassName);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_NOT_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}


SaAisErrorT runtime_class_create(SaImmHandleT immHandle)
{
    SaImmAttrDefinitionT_2 rdn = {
        "rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_RUNTIME | SA_IMM_ATTR_RDN | SA_IMM_ATTR_CACHED,
        NULL
    };

    SaImmAttrDefinitionT_2 attr1 = {
        "attr1", SA_IMM_ATTR_SAUINT32T, SA_IMM_ATTR_RUNTIME, NULL};

    const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr1, NULL};

    return  saImmOmClassCreate_2(immHandle, runtimeClassName, SA_IMM_CLASS_RUNTIME,
        attributes);
}


SaAisErrorT runtime_class_delete(SaImmHandleT immHandle)
{
    SaAisErrorT err = saImmOmClassDelete(immHandle, runtimeClassName);
    if((err == SA_AIS_OK) || (err == SA_AIS_ERR_NOT_EXIST)) {
        return SA_AIS_OK;
    }

    return err;
}

