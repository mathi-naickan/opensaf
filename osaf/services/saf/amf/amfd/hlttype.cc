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

#include <string.h>

#include <logtrace.h>
#include <immutil.h>
#include <ncsgl_defs.h>
#include <imm.h>

/**
 * Validates proposed change in comptype
 */
static SaAisErrorT ccb_completed_modify_hdlr(const CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2 *mod;
	int i = 0;
	const char *dn = (char*)opdata->objectName.value;

	while ((mod = opdata->param.modify.attrMods[i++]) != NULL) {
		if (strcmp(mod->modAttr.attrName, "saAmfHctDefPeriod") == 0) {
			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < SA_TIME_ONE_SECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfHctDefPeriod for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		} else if (strcmp(mod->modAttr.attrName, "saAmfHctDefMaxDuration") == 0) {
			SaTimeT value = *((SaTimeT *)mod->modAttr.attrValues[0]);
			if (value < 100 * SA_TIME_ONE_MILLISECOND) {
				report_ccb_validation_error(opdata,
					"Invalid saAmfHctDefMaxDuration for '%s'", dn);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
		}
	}

done:
	return rc;
}

static SaAisErrorT hct_ccb_completed_cb(CcbUtilOperationData_t *opdata)
{
	SaAisErrorT rc = SA_AIS_ERR_BAD_OPERATION;

	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		rc = SA_AIS_OK;
		break;
	case CCBUTIL_MODIFY:
		rc = ccb_completed_modify_hdlr(opdata);
		break;
	case CCBUTIL_DELETE:
		rc = SA_AIS_OK;
		break;
	default:
		osafassert(0);
		break;
	}

	return rc;
}

static void hct_ccb_apply_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("CCB ID %llu, '%s'", opdata->ccbId, opdata->objectName.value);

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		break;
	case CCBUTIL_DELETE:
		break;
	case CCBUTIL_MODIFY:
		// values not used, no need to reinitialize type
		break;
	default:
		osafassert(0);
		break;
	}
}

void avd_hctype_constructor(void)
{
	avd_class_impl_set(const_cast<SaImmClassNameT>("SaAmfHealthcheckType"),
			NULL, NULL, hct_ccb_completed_cb, hct_ccb_apply_cb);
}

