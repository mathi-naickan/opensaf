/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

#include "imm/apitest/immtest.h"
#include "base/osaf_extended_name.h"


static char *longDn = "longdn=0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
		"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";


static void setupLongDn(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaNameT dn;
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	SaImmAttrValueT attrValues[1] = { &dn };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE,
			{ "attr8", SA_IMM_ATTR_SANAMET, 1, attrValues }};
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immHandle);
	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(object_create(immHandle, ownerHandle, configClassName, &dn, NULL, NULL), SA_AIS_OK);

	// Change SaNameT attribute for testing
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbObjectModify_2(ccbHandle, &dn, attrMods), SA_AIS_OK);
	safassert(saImmOmCcbApply(ccbHandle), SA_AIS_OK);
	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);
}

static void cleanLongDn(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaNameT dn;
	const SaNameT *objects[2] = { &dn, NULL };
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE), SA_AIS_OK);

	safassert(object_delete(ownerHandle, &dn, 0), SA_AIS_OK);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);
}

static void saImmOmLongDn_01(void) {
	SaImmHandleT immHandle;
	SaAisErrorT error;
	SaNameT defaultValue;
	SaImmAttrDefinitionT_2 rdn = {
			"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL
	};
	SaImmAttrDefinitionT_2 attr = {
			"attr", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE, &defaultValue
	};
	const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr, NULL};

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	osaf_extended_name_lend(longDn, &defaultValue);
	error = saImmOmClassCreate_2(immHandle, "LongDnClass", SA_IMM_CLASS_CONFIG, attributes);
	safassert(saImmOmClassDelete(immHandle, "LongDnClass"), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(error, SA_AIS_OK);
}

static void saImmOmLongDn_02(void) {
	SaImmHandleT immHandle;
	SaAisErrorT error;
	SaNameT defaultValue;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 rdn = {
			"rdn", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_RDN, NULL
	};
	SaImmAttrDefinitionT_2 attr = {
			"attr", SA_IMM_ATTR_SANAMET, SA_IMM_ATTR_CONFIG | SA_IMM_ATTR_WRITABLE, &defaultValue
	};
	const SaImmAttrDefinitionT_2* attributes[] = {&rdn, &attr, NULL};
	SaImmAttrDefinitionT_2 **attrDef;

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	osaf_extended_name_lend(longDn, &defaultValue);
	safassert(saImmOmClassCreate_2(immHandle, "LongDnClass", SA_IMM_CLASS_CONFIG, attributes), SA_AIS_OK);

	error = saImmOmClassDescriptionGet_2(immHandle, "LongDnClass", &classCategory, &attrDef);
	if(error == SA_AIS_OK) {
		int i = 0;
		while(attrDef[i]) {
			if(!strcmp("attr", attrDef[i]->attrName)) {
				SaNameT *testDn = (SaNameT *)attrDef[i]->attrDefaultValue;
				assert(testDn != NULL);
				assert(strcmp(longDn, osaf_extended_name_borrow(testDn)) == 0);
				break;
			}
			i++;
		}
		assert(attrDef[i] != NULL);

		safassert(saImmOmClassDescriptionMemoryFree_2(immHandle, attrDef), SA_AIS_OK);
	}

	safassert(saImmOmClassDelete(immHandle, "LongDnClass"), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(error, SA_AIS_OK);
}

static void saImmOmLongDn_03(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaAisErrorT rc;
	SaNameT dn;
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	SaImmAttrValueT rdnVal[1] = { (SaImmAttrValueT)&dn };
	SaImmAttrValuesT_2 attrValue = { "rdn", SA_IMM_ATTR_SANAMET, 1, rdnVal };
	const SaImmAttrValuesT_2 *attrValues[2] = { &attrValue, NULL };

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immHandle);
	osaf_extended_name_lend(longDn, &dn);

	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);
	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectCreate_2(ccbHandle, configClassName, NULL, attrValues);
	saImmOmCcbApply(ccbHandle);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(object_delete(ownerHandle, &dn, 1), SA_AIS_OK);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOmLongDn_04(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaAisErrorT rc;
	SaNameT dn;
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	SaImmAttrValueT attrValues[1] = { &dn };
	SaImmAttrModificationT_2 attrMod = { SA_IMM_ATTR_VALUES_REPLACE,
			{ "attr8", SA_IMM_ATTR_SANAMET, 1, attrValues }};
	const SaImmAttrModificationT_2 *attrMods[2] = { &attrMod, NULL };

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immHandle);

	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

	osaf_extended_name_lend(longDn, &dn);
	safassert(object_create(immHandle, ownerHandle, configClassName, &dn, NULL, NULL), SA_AIS_OK);

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectModify_2(ccbHandle, &dn, attrMods);
	saImmOmCcbApply(ccbHandle);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	safassert(object_delete(ownerHandle, &dn, 1), SA_AIS_OK);
	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOmLongDn_05(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT ccbHandle;
	SaAisErrorT rc;
	SaNameT dn;
	const SaImmAdminOwnerNameT ownerName = (const SaImmAdminOwnerNameT)__FUNCTION__;

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	config_class_create(immHandle);

	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

	osaf_extended_name_lend(longDn, &dn);
	safassert(object_create(immHandle, ownerHandle, configClassName, &dn, NULL, NULL), SA_AIS_OK);

	safassert(saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle), SA_AIS_OK);

	rc = saImmOmCcbObjectDelete(ccbHandle, &dn);
	saImmOmCcbApply(ccbHandle);

	safassert(saImmOmCcbFinalize(ccbHandle), SA_AIS_OK);

	config_class_delete(immHandle);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);
}

static void saImmOmLongDn_06(void) {
	SaImmHandleT immHandle;
	SaImmSearchHandleT searchHandle;
	SaAisErrorT rc;
	SaNameT dn;
	SaImmSearchParametersT_2 searchParam;
	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes;
	int i;

	osaf_extended_name_lend(longDn, &dn);
	searchParam.searchOneAttr.attrName = "attr8";
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SANAMET;
	searchParam.searchOneAttr.attrValue = &dn;

	setupLongDn();

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);

	safassert(saImmOmSearchInitialize_2(immHandle, NULL, SA_IMM_SUBTREE,
			SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &searchParam,
			NULL, &searchHandle), SA_AIS_OK);
	rc = saImmOmSearchNext_2(searchHandle, &objectName, &attributes);
	if(rc == SA_AIS_OK) {
		assert(strcmp(osaf_extended_name_borrow(&objectName), longDn) == 0);
		for(i=0; attributes[i]; i++) {
			if(!strcmp(attributes[i]->attrName, "attr8")) {
				// Must have at least one value
				assert(attributes[i]->attrValuesNumber > 0);
				// Check attribute value
				assert(strcmp(osaf_extended_name_borrow((SaNameT *)(attributes[i]->attrValues[0])), longDn) == 0);
				break;
			}
		}
		// SaNameT attribute is missing
		assert(attributes != NULL);
	}
	safassert(saImmOmSearchFinalize(searchHandle), SA_AIS_OK);

	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanLongDn();
}

static void saImmOmLongDn_07(void) {
	SaImmHandleT immHandle;
	SaImmAccessorHandleT accessorHandle;
	SaAisErrorT rc;
	SaNameT dn;
	SaImmAttrValuesT_2 **attributes;
	int i;
	int score = 0;

	osaf_extended_name_lend(longDn, &dn);

	setupLongDn();

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);

	safassert(saImmOmAccessorInitialize(immHandle, &accessorHandle), SA_AIS_OK);
	rc = saImmOmAccessorGet_2(accessorHandle, &dn, NULL, &attributes);
	if(rc == SA_AIS_OK) {
		for(i=0; attributes[i]; i++) {
			if(!strcmp(attributes[i]->attrName, "rdn") || !strcmp(attributes[i]->attrName, "attr8")) {
				assert(attributes[i]->attrValuesNumber > 0);
				assert(strcmp(osaf_extended_name_borrow((SaNameT *)(attributes[i]->attrValues[0])), longDn) == 0);
				score++;
			}
		}

		// Both SaNameT values are correct
		assert(score == 2);
	}
	safassert(saImmOmAccessorFinalize(accessorHandle), SA_AIS_OK);

	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanLongDn();
}

static void saImmOmLongDn_08(void) {
	SaImmHandleT immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmAdminOwnerNameT ownerName = (SaImmAdminOwnerNameT)__FUNCTION__;
	SaAisErrorT rc;
	SaNameT dn;
	const SaNameT *objects[2] = { &dn, NULL };

	osaf_extended_name_lend(longDn, &dn);

	setupLongDn();

	safassert(saImmOmInitialize(&immHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(saImmOmAdminOwnerInitialize(immHandle, ownerName, SA_TRUE, &ownerHandle), SA_AIS_OK);

	rc = saImmOmAdminOwnerSet(ownerHandle, objects, SA_IMM_ONE);

	safassert(saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(saImmOmFinalize(immHandle), SA_AIS_OK);

	test_validate(rc, SA_AIS_OK);

	cleanLongDn();
}



void addOmLongDnTestCases() {
	test_case_add(8, saImmOmLongDn_01, "SA_AIS_OK - saImmOmClassCreate_2 with long DN as default value");
	test_case_add(8, saImmOmLongDn_02, "SA_AIS_OK - saImmOmClassDescriptionGet_2 with long DN as default value");
	test_case_add(8, saImmOmLongDn_03, "SA_AIS_OK - saImmOmCcbObjectCreate_2 - long RDN");
	test_case_add(8, saImmOmLongDn_04, "SA_AIS_OK - saImmOmCcbObjectModify_2 - long DN");
	test_case_add(8, saImmOmLongDn_05, "SA_AIS_OK - saImmOmCcbObjectDelete - long DN");
	test_case_add(8, saImmOmLongDn_06, "SA_AIS_OK - saImmOmSearchInitialize_2 and saImmOmSearchNext_2");
	test_case_add(8, saImmOmLongDn_07, "SA_AIS_OK - saImmOmAccessorGet_2");
	test_case_add(8, saImmOmLongDn_08, "SA_AIS_OK - saImmOmAdminOwnerSet");
}

__attribute__ ((constructor)) static void saImmOmLongDn_constructor(void)
{
	test_suite_add(8, "Long DN");
}




