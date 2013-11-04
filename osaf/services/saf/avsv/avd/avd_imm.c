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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>
#include <logtrace.h>
#include <saflog.h>

#include <avd_evt.h>
#include <avd_util.h>
#include <avd_cb.h>
#include <avsv_defs.h>
#include <avd_imm.h>
#include <avd_cluster.h>
#include <avd_app.h>
#include <avd_sg.h>
#include <avd_su.h>
#include <avd_sutype.h>
#include <avd_comp.h>
#include <avd_si.h>
#include <avd_csi.h>
#include <avd_si_dep.h>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

enum job_type {
	JOB_IMM_OBJCREATE = 1,
	JOB_IMM_OBJUPDATE,
	JOB_IMM_OBJDELETE
};

struct job_imm_objcreate {
	enum job_type type;
	SaImmClassNameT className;
	SaNameT parentName;
	const SaImmAttrValuesT_2 **attrValues;
};

struct job_imm_objupdate {
	enum job_type type;
	SaNameT dn;
	SaImmAttrNameT attributeName;
	SaImmValueTypeT attrValueType;
	void *value;
};

struct job_imm_objdelete {
	enum job_type type;
	SaNameT dn;
};

union job {
	enum job_type type;
	struct job_imm_objcreate objcreate;
	struct job_imm_objupdate objupdate;
	struct job_imm_objdelete objdelete;
};

struct fifo_node {
	struct fifo_node *next;
	void *value;
};

static struct fifo_node *imm_update_fifo_tail, *imm_update_fifo_head;

extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

static const SaImmOiImplementerNameT implementerName =
	(SaImmOiImplementerNameT)"safAmfService";
static SaVersionT immVersion = { 'A', 2, 11 };

/* This string array must match the AVSV_AMF_CLASS_ID enum */
static char *avd_class_names[] = {
	"Invalid",

	"SaAmfCompBaseType",
	"SaAmfSUBaseType",
	"SaAmfSGBaseType",
	"SaAmfAppBaseType",
	"SaAmfSvcBaseType",
	"SaAmfCSBaseType",
	"SaAmfCompGlobalAttributes",

	"SaAmfCompType",
	"SaAmfCSType",
	"SaAmfCtCsType",
	"SaAmfHealthcheckType",
	"SaAmfSvcType",
	"SaAmfSvcTypeCSTypes",
	"SaAmfSUType",
	"SaAmfSutCompType",
	"SaAmfSGType",
	"SaAmfAppType", 

	"SaAmfCluster",
	"SaAmfNode",
	"SaAmfNodeGroup",
	"SaAmfNodeSwBundle",

	"SaAmfApplication",
	"SaAmfSG",
	"SaAmfSI",
	"SaAmfCSI",
	"SaAmfCSIAttribute",
	"SaAmfSU",
	"SaAmfComp",
	"SaAmfHealthcheck",
	"SaAmfCompCsType",
	"SaAmfSIDependency",
	"SaAmfSIRankedSU",

	"SaAmfCSIAssignment",
	"SaAmfSIAssignment"
};

static AvdImmOiCcbApplyCallbackT ccb_apply_callback[AVSV_SA_AMF_CLASS_MAX];
static AvdImmOiCcbCompletedCallbackT ccb_completed_callback[AVSV_SA_AMF_CLASS_MAX];
static SaImmOiAdminOperationCallbackT_2 admin_op_callback[AVSV_SA_AMF_CLASS_MAX];
static SaImmOiRtAttrUpdateCallbackT rtattr_update_callback[AVSV_SA_AMF_CLASS_MAX];

typedef struct avd_ccb_apply_ordered_list {
	AvdImmOiCcbApplyCallbackT ccb_apply_cb;
	CcbUtilOperationData_t *opdata;
	AVSV_AMF_CLASS_ID class_type; 
	struct avd_ccb_apply_ordered_list *next_ccb_to_apply;
} AvdCcbApplyOrderedListT;

static AvdCcbApplyOrderedListT *ccb_apply_list;

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

/**
 * Queue value at tail of FIFO
 * 
 * @param value
 * 
 * @return void*
 */
static void fifo_queue(void *value)
{
	struct fifo_node *tmp = malloc(sizeof(*tmp));

	tmp->next = NULL;
	tmp->value = value;

	if (imm_update_fifo_tail == NULL)
		imm_update_fifo_tail = imm_update_fifo_head = tmp;
	else  {
		imm_update_fifo_tail->next = tmp;
		imm_update_fifo_tail = tmp;
	}
}

/**
 * Remove and return head FIFO node
 * 
 * @return void*
 */
static void *fifo_dequeue(void)
{
	struct fifo_node *tmp;
	void *value = NULL;

	if (imm_update_fifo_head != NULL) {
		if (imm_update_fifo_head == imm_update_fifo_tail)
			imm_update_fifo_tail = NULL;

		tmp = imm_update_fifo_head;
		value = tmp->value;
		imm_update_fifo_head = imm_update_fifo_head->next;
		free(tmp);
	}

	return value;
}

/**
 * Return ptr to head FIFO node without removing it.
 * 
 * @return void*
 */
static void *fifo_peek(void)
{
	struct fifo_node *tmp = imm_update_fifo_head;
	void *value = NULL;

	if (tmp != NULL)
		value = tmp->value;

	return value;
}

static size_t value_size(SaImmValueTypeT attrValueType)
{
	size_t valueSize = 0;

	switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		valueSize = sizeof(SaInt32T);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		valueSize = sizeof(SaUint32T);
		break;
	case SA_IMM_ATTR_SAINT64T:
		valueSize = sizeof(SaInt64T);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		valueSize = sizeof(SaUint64T);
		break;
	case SA_IMM_ATTR_SATIMET:
		valueSize = sizeof(SaTimeT);
		break;
	case SA_IMM_ATTR_SANAMET:
		valueSize = sizeof(SaNameT);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		valueSize = sizeof(SaFloatT);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		valueSize = sizeof(SaDoubleT);
		break;
	case SA_IMM_ATTR_SASTRINGT:
		valueSize = sizeof(SaStringT);
		break;
	case SA_IMM_ATTR_SAANYT:
		osafassert(0);
		break;
	}

	return valueSize;
}

static void copySaImmAttrValuesT(SaImmAttrValuesT_2 *copy, const SaImmAttrValuesT_2 *original)
{
	size_t valueSize = 0;
	unsigned int i, valueCount = original->attrValuesNumber;
	char *databuffer;

	copy->attrName = strdup(original->attrName);
	osafassert(copy->attrName != NULL);

	copy->attrValuesNumber = valueCount;
	copy->attrValueType = original->attrValueType;
	if (valueCount == 0)
		return;		/* (just in case...) */

	copy->attrValues = malloc(valueCount * sizeof(SaImmAttrValueT));
	osafassert(copy->attrValues != NULL);

	valueSize = value_size(original->attrValueType);

	// alloc blob shared by all values
	databuffer = (char *)malloc(valueCount * valueSize);
	osafassert(databuffer != NULL);
	for (i = 0; i < valueCount; i++) {
		copy->attrValues[i] = databuffer;
		if (original->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			char *cporig = *((char **)original->attrValues[i]);
			char **cpp = (char **)databuffer;
			*cpp = strdup(cporig);
			osafassert(*cpp != NULL);
		} else {
			memcpy(databuffer, original->attrValues[i], valueSize);
		}
		databuffer += valueSize;
	}
}

static const SaImmAttrValuesT_2 *dupSaImmAttrValuesT(const SaImmAttrValuesT_2 *original)
{
	SaImmAttrValuesT_2 *copy = malloc(sizeof(SaImmAttrValuesT_2));
	osafassert(copy != NULL);
	copySaImmAttrValuesT(copy, original);
	return copy;
}

static const SaImmAttrValuesT_2 **dupSaImmAttrValuesT_array(const SaImmAttrValuesT_2 **original)
{
	const SaImmAttrValuesT_2 **copy;
	unsigned int i, alen = 0;

	if (original == NULL)
		return NULL;

	while (original[alen] != NULL)
		alen++;

	copy = calloc(1, ((alen + 1) * sizeof(SaImmAttrValuesT_2 *)));
	osafassert(copy != NULL);

	for (i = 0; i < alen; i++)
		copy[i] = dupSaImmAttrValuesT(original[i]);

	return copy;
}

static AVSV_AMF_CLASS_ID class_name_to_class_type(const SaImmClassNameT className)
{
	int i;

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if (strcmp(className, avd_class_names[i]) == 0)
			return i;
	}

	osafassert(0);
	return AVSV_SA_AMF_CLASS_INVALID;
}

/*****************************************************************************
 * Function: avd_class_type_find
 *
 * Purpose: This function returns class enum corresponding to Object name.
 *
 * Input: cb  - Object Name, Class type.
 *
 * Returns: OK/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static AVSV_AMF_CLASS_ID object_name_to_class_type(const SaNameT *obj_name)
{
	AVSV_AMF_CLASS_ID class_type = AVSV_SA_AMF_CLASS_INVALID;

	/* Cluster and Node Class Related */
	if (strncmp((char *)&obj_name->value, "safAmfCluster=", 14) == 0) {
		class_type = AVSV_SA_AMF_CLUSTER;
	} else if (strncmp((char *)&obj_name->value, "safAmfNode=", 11) == 0) {
		class_type = AVSV_SA_AMF_NODE;
	} else if (strncmp((char *)&obj_name->value, "safAmfNodeGroup=", 16) == 0) {
		class_type = AVSV_SA_AMF_NODE_GROUP;
	} else if (strncmp((char *)&obj_name->value, "safInstalledSwBundle=", 21) == 0) {
		class_type = AVSV_SA_AMF_NODE_SW_BUNDLE;
	}

	/* Application Class Related */
	else if (strncmp((char *)&obj_name->value, "safApp=", 7) == 0) {
		class_type = AVSV_SA_AMF_APP;
	} else if (strncmp((char *)&obj_name->value, "safAppType=", 11) == 0) {
		class_type = AVSV_SA_AMF_APP_BASE_TYPE;
	}

	/* Service Group Class Related */
	else if (strncmp((char *)&obj_name->value, "safSg=", 6) == 0) {
		class_type = AVSV_SA_AMF_SG;
	} else if (strncmp((char *)&obj_name->value, "safSgType=", 10) == 0) {
		class_type = AVSV_SA_AMF_SG_BASE_TYPE;
	}

	/* Service Unit Class Related */
	else if (strncmp((char *)&obj_name->value, "safSu=", 6) == 0) {
		class_type = AVSV_SA_AMF_SU;
	} else if (strncmp((char *)&obj_name->value, "safSuType=", 10) == 0) {
		class_type = AVSV_SA_AMF_SU_BASE_TYPE;
	} else if (strncmp((char *)&obj_name->value, "safMemberCompType=", 18) == 0) {
		class_type = AVSV_SA_AMF_SUT_COMP_TYPE;
	}

	/* Service Instance Class Related */
	else if (strncmp((char *)&obj_name->value, "safSi=", 6) == 0) {
		class_type = AVSV_SA_AMF_SI;
	} else if (strncmp((char *)&obj_name->value, "safSvcType=", 11) == 0) {
		class_type = AVSV_SA_AMF_SVC_BASE_TYPE;
	} else if (strncmp((char *)&obj_name->value, "safDepend=", 10) == 0) {
		class_type = AVSV_SA_AMF_SI_DEPENDENCY;
	} else if (strncmp((char *)&obj_name->value, "safRankedSu=", 12) == 0) {
		class_type = AVSV_SA_AMF_SI_RANKED_SU;
	} else if (strncmp((char *)&obj_name->value, "safSISU=", 8) == 0) {
		class_type = AVSV_SA_AMF_SI_ASSIGNMENT;
	} else if (strncmp((char *)&obj_name->value, "safMemberCSType=", 16) == 0) {
		class_type = AVSV_SA_AMF_SVC_TYPE_CS_TYPES;
	}

	/* Component Service Instance Class Related */
	else if (strncmp((char *)&obj_name->value, "safCsi=", 7) == 0) {
		class_type = AVSV_SA_AMF_CSI;
	} else if (strncmp((char *)&obj_name->value, "safCSType=", 10) == 0) {
		class_type = AVSV_SA_AMF_CS_BASE_TYPE;
	} else if (strncmp((char *)&obj_name->value, "safCsiAttr=", 11) == 0) {
		class_type = AVSV_SA_AMF_CSI_ATTRIBUTE;
	} else if (strncmp((char *)&obj_name->value, "safCSIComp=", 11) == 0) {
		class_type = AVSV_SA_AMF_CSI_ASSIGNMENT;
	}

	/* Component and component types Related */
	else if (strncmp((char *)&obj_name->value, "safCompType=", 12) == 0) {
		class_type = AVSV_SA_AMF_COMP_BASE_TYPE;
	} else if (strncmp((char *)&obj_name->value, "safSupportedCsType=", 19) == 0) {
		if (strstr((char *)&obj_name->value, "safCompType=") != 0) {
			class_type = AVSV_SA_AMF_CT_CS_TYPE;
		} else if (strstr((char *)&obj_name->value, "safComp=") != 0) {
			class_type = AVSV_SA_AMF_COMP_CS_TYPE;
		}
	} else if (strncmp((char *)&obj_name->value, "safComp=", 8) == 0) {
		class_type = AVSV_SA_AMF_COMP;
	}

	/* Global Component Attributes and Health Check Related */
	else if (strncmp((char *)&obj_name->value, "safRdn=", 7) == 0) {
		class_type = AVSV_SA_AMF_COMP_GLOBAL_ATTR;
	} else if (strncmp((char *)&obj_name->value, "safHealthcheckKey=", 18) == 0) {
		if (strstr((char *)&obj_name->value, "safVersion=") != 0) {
			class_type = AVSV_SA_AMF_HEALTH_CHECK_TYPE;
		} else if (strstr((char *)&obj_name->value, "safComp=") != 0) {
			class_type = AVSV_SA_AMF_HEALTH_CHECK;
		}
	}

	/* Common Version Related */
	else if (strncmp((char *)&obj_name->value, "safVersion=", 11) == 0) {
		if (strstr((char *)&obj_name->value, "safAppType=") != 0) {
			class_type = AVSV_SA_AMF_APP_TYPE;
		} else if (strstr((char *)&obj_name->value, "safSgType=") != 0) {
			class_type = AVSV_SA_AMF_SG_TYPE;
		} else if (strstr((char *)&obj_name->value, "safSuType=") != 0) {
			class_type = AVSV_SA_AMF_SU_TYPE;
		} else if (strstr((char *)&obj_name->value, "safSvcType=") != 0) {
			class_type = AVSV_SA_AMF_SVC_TYPE;
		} else if (strstr((char *)&obj_name->value, "safCSType=") != 0) {
			class_type = AVSV_SA_AMF_CS_TYPE;
		} else if (strstr((char *)&obj_name->value, "safCompType=") != 0) {
			class_type = AVSV_SA_AMF_COMP_TYPE;
		}
	}

	return class_type;
}

/*****************************************************************************
 * Function: avd_oi_admin_operation_cb
 *
 * Purpose: This function handles all admin operations.
 *
 * Input: cb  - Oi Handle, Invocation, Object Name, Operation Id, Parameters 
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void admin_operation_cb(SaImmOiHandleT immoi_handle,
	SaInvocationT invocation, const SaNameT *object_name,
	SaImmAdminOperationIdT op_id, const SaImmAdminOperationParamsT_2 **params)
{
	AVSV_AMF_CLASS_ID type = object_name_to_class_type(object_name);

 	TRACE_ENTER2("'%s', invocation: %llu, op: %llu", object_name->value, invocation, op_id);
	saflog(LOG_NOTICE, amfSvcUsrName, "Admin op \"%s\" initiated for '%s', invocation: %llu",
		   admin_op_name(op_id), object_name->value, invocation);

	if (admin_op_callback[type] != NULL) {
		admin_op_callback[type](immoi_handle, invocation, object_name, op_id, params);
	} else {
		LOG_ER("Admin operation not supported for %s (%u)", object_name->value, type);
		avd_saImmOiAdminOperationResult(immoi_handle, invocation, SA_AIS_ERR_INVALID_PARAM);
	}
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_oi_rt_attr_update_cb
 *
 * Purpose: This function handles all Runtime attr update callbacks.
 *
 * Input: cb  - Oi Handle, Invocation, Object Name, Operation Id, Parameters 
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT rt_attr_update_cb(SaImmOiHandleT immoi_handle,
	const SaNameT *object_name, const SaImmAttrNameT *attribute_names)
{
	SaAisErrorT error;
	AVSV_AMF_CLASS_ID type = object_name_to_class_type(object_name);

	TRACE_ENTER2("%s", object_name->value);
	osafassert(rtattr_update_callback[type] != NULL);
	error = rtattr_update_callback[type](immoi_handle, object_name, attribute_names);
	TRACE_LEAVE2("%u", error);
	return error;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_create_cb
 *
 * Purpose: This function handles object create callback for all config classes.
 *          Its purpose is to memorize the request until the completed callback.
 *
 * Input: cb  - Oi Handle, Ccb Id, Class Name, Parent Name, Attributes. 
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_create_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaImmClassNameT class_name,
	const SaNameT *parent_name, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	CcbUtilCcbData_t *ccb_util_ccb_data;
	CcbUtilOperationData_t *operation;
	int i = 0;
	const SaImmAttrValuesT_2 *attrValue;
	AVSV_AMF_CLASS_ID id_from_class_name, id_from_dn;

	TRACE_ENTER2("CCB ID %llu, class %s, parent '%s'", ccb_id, class_name, parent_name->value);

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) == NULL) {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	operation = ccbutil_ccbAddCreateOperation(ccb_util_ccb_data, class_name, parent_name, attr);

	if (operation == NULL) {
		LOG_ER("Failed to get CCB operation object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
		goto done;
	}

	/* Find the RDN attribute and store the object DN */
	while ((attrValue = attr[i++]) != NULL) {
		if (!strncmp(attrValue->attrName, "saf", 3)) {
			if (attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
				SaStringT rdnVal = *((SaStringT *)attrValue->attrValues[0]);
				if ((parent_name != NULL) && (parent_name->length > 0)) {
					operation->objectName.length = sprintf((char *)operation->objectName.value,
						"%s,%s", rdnVal, parent_name->value);
				} else {
					operation->objectName.length = sprintf((char *)operation->objectName.value,
						"%s", rdnVal);
				}
			} else {
				SaNameT *rdnVal = ((SaNameT *)attrValue->attrValues[0]);
				operation->objectName.length = sprintf((char *)operation->objectName.value,
					"%s,%s", rdnVal->value, parent_name->value);
			}
			
			TRACE("%s(%u)", operation->objectName.value, operation->objectName.length);
		}
	}
	
	if (operation->objectName.length == 0) {
		LOG_ER("Malformed DN %llu", ccb_id);
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

	/* Verify that DN is valid for class */
	id_from_class_name = class_name_to_class_type(class_name);
	id_from_dn = object_name_to_class_type(&operation->objectName);
	if (id_from_class_name != id_from_dn) {
		LOG_ER("Illegal DN '%s' for class '%s'", operation->objectName.value, class_name);
		rc = SA_AIS_ERR_INVALID_PARAM;
	}

done:
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_delete_cb
 *
 * Purpose: This function handles object delete callback for all config classes
 *
 * Input: cb  - Oi Handle, Ccb Id, object name.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_delete_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaNameT *object_name)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccb_util_ccb_data;

	TRACE_ENTER2("CCB ID %llu, %s", ccb_id, object_name->value);

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != NULL) {
		/* "memorize the request" */
		ccbutil_ccbAddDeleteOperation(ccb_util_ccb_data, object_name);
	} else {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_object_modify_cb
 *
 * Purpose: This function handles object modify callback for config classes.
 *
 * Input: cb  - Oi Handle, Ccb Id, Class Name, Parent Name, Attributes.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_object_modify_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id, const SaNameT *object_name,
	const SaImmAttrModificationT_2 **attr_mods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccb_util_ccb_data;

	TRACE_ENTER2("CCB ID %llu, %s", ccb_id, object_name->value);

	if ((ccb_util_ccb_data = ccbutil_getCcbData(ccb_id)) != NULL) {
		/* "memorize the request" */
		if (ccbutil_ccbAddModifyOperation(ccb_util_ccb_data, object_name, attr_mods) != 0) {
			LOG_ER("Failed '%s'", object_name->value);
			rc = SA_AIS_ERR_BAD_OPERATION;
		}
	} else {
		LOG_ER("Failed to get CCB object for %llu", ccb_id);
		rc = SA_AIS_ERR_NO_MEMORY;
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_oi_ccb_completed_cb
 *
 * Purpose: This function handles completed callback for the corresponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: Ok/Error.
 *
 * NOTES: None.
 *
 **************************************************************************/
static SaAisErrorT ccb_completed_cb(SaImmOiHandleT immoi_handle,
	SaImmOiCcbIdT ccb_id)
{
	SaAisErrorT rc = SA_AIS_OK;
	CcbUtilOperationData_t *opdata = NULL;
	AVSV_AMF_CLASS_ID type;

	TRACE_ENTER2("CCB ID %llu", ccb_id);

	/* "check that the sequence of change requests contained in the CCB is
	   valid and that no errors will be generated when these changes
	   are applied." */

	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		type = object_name_to_class_type(&opdata->objectName);
		if (ccb_completed_callback[type] == NULL) {
			/* this can happen for malformed DNs */
			LOG_ER("Class implementer for '%s' not found", opdata->objectName.value);
			goto done;
		}
		rc = ccb_completed_callback[type](opdata);

		/* Get out at first error */
		if (rc != SA_AIS_OK)
			break;
	}
done:
	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: ccb_abort_cb
 *
 * Purpose: This function handles abort callback for the corresponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_abort_cb(SaImmOiHandleT immoi_handle, SaImmOiCcbIdT ccb_id)
{
	CcbUtilCcbData_t *ccb_util_ccb_data;

	TRACE_ENTER2("CCB ID %llu", ccb_id);

	/* Return CCB container memory */
	ccb_util_ccb_data = ccbutil_findCcbData(ccb_id);
	osafassert(ccb_util_ccb_data);
	ccbutil_deleteCcbData(ccb_util_ccb_data);

	TRACE_LEAVE();
}
/*****************************************************************************
 * Function: ccb_insert_ordered_list
 *
 * Purpose: This function sorts the Ccb apply callbacks for the correponding 
 *          CCB operation in order of dependency/heirarchy
 *
 * Input: ccb_apply_cb - apply callback for specific object
 * 	  opdata - Ccb Operation data
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_insert_ordered_list(AvdImmOiCcbApplyCallbackT ccb_apply_cb,
		CcbUtilOperationData_t *opdata, AVSV_AMF_CLASS_ID type)
{
	AvdCcbApplyOrderedListT *temp = NULL;
	AvdCcbApplyOrderedListT *prev = NULL;
	AvdCcbApplyOrderedListT *next = NULL;

	/* allocate memory */

	temp = malloc(sizeof(AvdCcbApplyOrderedListT));

	temp->ccb_apply_cb = ccb_apply_cb;
	temp->opdata = opdata;
	temp->class_type = type;
	temp->next_ccb_to_apply = NULL;

	/* ccbs are sorted in top-down order in create/modify operations and
	 * sorted in bottom-up order for delete operation. All the ccbs are 
	 * appended to a single list in the order of create operations first
	 * then modify operations and lastly delete operations
	 */

	switch (opdata->operationType) {
	case CCBUTIL_CREATE:
		next = ccb_apply_list;
		while (next != NULL) {
			if((next->opdata->operationType != CCBUTIL_CREATE) ||
					(next->class_type > temp->class_type)) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != NULL) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	case CCBUTIL_MODIFY:
		next = ccb_apply_list;

		/* traverse to the end of all the create CCBs */
		while (next && next->opdata->operationType == CCBUTIL_CREATE) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		while (next != NULL) {
			if((next->opdata->operationType != CCBUTIL_MODIFY) ||
					(next->class_type > temp->class_type)) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != NULL) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	case CCBUTIL_DELETE:
		next = ccb_apply_list;

		/* traverse to the end of all the create CCBs */
		while (next && next->opdata->operationType == CCBUTIL_CREATE) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		/* traverse to the end of all the modify CCBs */
		while (next && next->opdata->operationType == CCBUTIL_MODIFY) {
			prev = next;
			next = next->next_ccb_to_apply;
		}

		while (next != NULL) {
			if(next->class_type < temp->class_type) 
				break;
			prev = next;
			next = next->next_ccb_to_apply;
		}
		/* insert the ccb */
		if (prev != NULL) 
			prev->next_ccb_to_apply = temp;
		else 
			ccb_apply_list = temp;

		temp->next_ccb_to_apply = next;
		break;

	default:
		osafassert(0);
			break;
	}
}

/*****************************************************************************
 * Function: ccb_apply_cb
 *
 * Purpose: This function handles apply callback for the correponding 
 *          CCB operation.
 *
 * Input: cb  - Oi Handle and Ccb Id.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void ccb_apply_cb(SaImmOiHandleT immoi_handle, SaImmOiCcbIdT ccb_id)
{
	CcbUtilCcbData_t *ccb_util_ccb_data;
	CcbUtilOperationData_t *opdata = NULL;
	AVSV_AMF_CLASS_ID type;
	AvdCcbApplyOrderedListT *next = NULL;
	AvdCcbApplyOrderedListT *temp = NULL;

	TRACE_ENTER2("CCB ID %llu", ccb_id);

	while ((opdata = ccbutil_getNextCcbOp(ccb_id, opdata)) != NULL) {
		type = object_name_to_class_type(&opdata->objectName);
		/* Base types will not have an apply callback, skip empty ones */
		if (ccb_apply_callback[type] != NULL) {
			/* insert the apply callback into the sorted list
			 * to be applied later, after all the ccb apply 
			 * callback are sorted as required by the internal
			 * AMFD's information model
			 */
			ccb_insert_ordered_list(ccb_apply_callback[type], opdata, type);
		}

		if (avd_cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
			switch (opdata->operationType) {
			case CCBUTIL_CREATE:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Created %s", ccb_id, opdata->objectName.value);
				break;
			case CCBUTIL_MODIFY:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Modified %s", ccb_id, opdata->objectName.value);
				break;
			case CCBUTIL_DELETE:
				saflog(LOG_NOTICE, amfSvcUsrName, "CCB %llu Deleted %s", ccb_id, opdata->objectName.value);
				break;
			default:
				osafassert(0);
			}
		}
	}

	/* First pass: apply all the CCBs in the sorted order */
	next = ccb_apply_list;
	while (next != NULL) {
		next->ccb_apply_cb(next->opdata);
		temp = next;
		next = next->next_ccb_to_apply;
	}

	/* Second pass: adjust configuration after all model changes has been applied */
	next = ccb_apply_list;
	while (next != NULL) {
		// TODO: would be more elegant with yet another function pointer
		type = object_name_to_class_type(&next->opdata->objectName);
		if ((type == AVSV_SA_AMF_SG) && (next->opdata->operationType == CCBUTIL_CREATE)) {
			AVD_SG *sg = avd_sg_get(&next->opdata->objectName);
			avd_sg_adjust_config(sg);
		}
		next = next->next_ccb_to_apply;
	}

	/* Third pass: free allocated memory */
	next = ccb_apply_list;
	while (next != NULL) {
		temp = next;
		next = next->next_ccb_to_apply;
		free(temp);
	}

	ccb_apply_list = NULL;

	/* Return CCB container memory */
	ccb_util_ccb_data = ccbutil_findCcbData(ccb_id);
	osafassert(ccb_util_ccb_data);
	ccbutil_deleteCcbData(ccb_util_ccb_data);
	TRACE_LEAVE();
}

static const SaImmOiCallbacksT_2 avd_callbacks = {
	.saImmOiAdminOperationCallback = admin_operation_cb,
	.saImmOiCcbAbortCallback = ccb_abort_cb,
	.saImmOiCcbApplyCallback = ccb_apply_cb,
	.saImmOiCcbCompletedCallback = ccb_completed_cb,
	.saImmOiCcbObjectCreateCallback = ccb_object_create_cb,
	.saImmOiCcbObjectDeleteCallback = ccb_object_delete_cb,
	.saImmOiCcbObjectModifyCallback = ccb_object_modify_cb,
	.saImmOiRtAttrUpdateCallback = rt_attr_update_cb
};

/*****************************************************************************
 * Function: avd_imm_init
 *
 * Purpose: This function Initialize the OI interface and get a selection 
 *          object.
 *
 * Input: cb  - AVD control block
 * 
 * Returns: Void pointer.
 * 
 * NOTES: None.
 * 
 **************************************************************************/

SaAisErrorT avd_imm_init(void *avd_cb)
{
	SaAisErrorT error = SA_AIS_OK;
	AVD_CL_CB *cb = (AVD_CL_CB *)avd_cb;

	TRACE_ENTER();

	/* Setup immutils profile once and for all */
	immutilWrapperProfile.errorsAreFatal = false;
	immutilWrapperProfile.retryInterval = 1000;
	immutilWrapperProfile.nTries = 180;


	if ((error = immutil_saImmOiInitialize_2(&cb->immOiHandle, &avd_callbacks, &immVersion)) != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize failed %u", error);
		goto done;
	}

	if ((error = immutil_saImmOmInitialize(&cb->immOmHandle, NULL, &immVersion)) != SA_AIS_OK) {
		LOG_ER("saImmOmInitialize failed %u", error);
		goto done;
	}

	if ((error = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj)) != SA_AIS_OK) {
		LOG_ER("saImmOiSelectionObjectGet failed %u", error);
		goto done;
	}

	TRACE("Successfully initialized IMM");

done:
	TRACE_LEAVE();
	return error;
}

/*****************************************************************************
 * Function: avd_imm_impl_set
 *
 * Purpose: This task makes class implementer.
 *
 * Input: -
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
SaAisErrorT avd_imm_impl_set(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t i;

	TRACE_ENTER();

	if ((rc = immutil_saImmOiImplementerSet(avd_cb->immOiHandle, implementerName)) != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		return rc;
	}

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if ((NULL != ccb_completed_callback[i]) &&
		    (rc = immutil_saImmOiClassImplementerSet(avd_cb->immOiHandle, avd_class_names[i])) != SA_AIS_OK) {

			LOG_ER("Impl Set Failed for %s, returned %d",	avd_class_names[i], rc);
			break;
		}
	}

	avd_cb->is_implementer = true;

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * @brief		This function becomes applier and sets the applier name
 *			for all AMF objects
 *
 * @param[in] 		Nothing
 *
 * @return 		SA_AIS_OK or error
 *
 **************************************************************************/
SaAisErrorT avd_imm_applier_set(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t i;
	char applier_name[SA_MAX_NAME_LENGTH] = {0};

	TRACE_ENTER();
	snprintf(applier_name, SA_MAX_NAME_LENGTH, "@safAmfService%x", avd_cb->node_id_avd);

	if ((rc = immutil_saImmOiImplementerSet(avd_cb->immOiHandle, applier_name)) != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed %u", rc);
		return rc;
	}

	for (i = 0; i < AVSV_SA_AMF_CLASS_MAX; i++) {
		if ((NULL != ccb_completed_callback[i]) &&
		    (rc = immutil_saImmOiClassImplementerSet(avd_cb->immOiHandle, avd_class_names[i])) != SA_AIS_OK) {

			LOG_ER("Impl Set Failed for %s, returned %d",	avd_class_names[i], rc);
			break;
		}
	}

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/*****************************************************************************
 * Function: avd_imm_impl_set_task
 *
 * Purpose: This task makes object and class implementer.
 *
 * Input: cb  - AVD control block
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
static void *avd_imm_impl_set_task(void *_cb)
{
	if (avd_imm_impl_set() != SA_AIS_OK) {
		LOG_ER("exiting since avd_imm_impl_set failed");
		exit(EXIT_FAILURE);	// TODO ncs_reboot?
	}

	return NULL;
}

/*****************************************************************************
 * Function: avd_imm_impl_set_task_create
 *
 * Purpose: This function spawns thread for setting object and class 
 *          implementer, non-blocking.
 *
 * Input: -
 *
 * Returns: Void pointer.
 *
 * NOTES: None.
 *
 **************************************************************************/
void avd_imm_impl_set_task_create(void)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avd_imm_impl_set_task, avd_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/*****************************************************************************
 * @brief 		This function calls the routines to become applier for
 *			all AMF objects and to read the configuration from IMM.
 *
 * @param[in]		cb  - Pointer to AVD control block
 *
 * @return		void pointer.
 *
 **************************************************************************/
static void *avd_imm_applier_set_task(void *_cb)
{
	if (avd_imm_applier_set() != SA_AIS_OK) {
		LOG_ER("exiting since avd_imm_applier_set failed");
		exit(EXIT_FAILURE);
	}

	if (avd_imm_config_get() != NCSCC_RC_SUCCESS) {
		LOG_ER("avd_imm_config_get FAILED");
		exit(EXIT_FAILURE);
	}

	return NULL;
}

/*****************************************************************************
 * @brief		This function spawns thread for setting object and class 
 *          		applier, non-blocking.
 *
 * @param[in]		Nothing
 *
 * @return		Nothing
 *
 **************************************************************************/
void avd_imm_applier_set_task_create(void)
{
	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avd_imm_applier_set_task, avd_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void avd_class_impl_set(const SaImmClassNameT className,
	SaImmOiRtAttrUpdateCallbackT rtattr_cb, SaImmOiAdminOperationCallbackT_2 adminop_cb,
	AvdImmOiCcbCompletedCallbackT ccb_compl_cb, AvdImmOiCcbApplyCallbackT ccb_apply_cb)
{
	AVSV_AMF_CLASS_ID type = class_name_to_class_type(className);

	rtattr_update_callback[type] = rtattr_cb;
	admin_op_callback[type] = adminop_cb;
	osafassert(ccb_completed_callback[type] == NULL);
	ccb_completed_callback[type] = ccb_compl_cb;
	ccb_apply_callback[type] = ccb_apply_cb;
}

SaAisErrorT avd_imm_default_OK_completed_cb(CcbUtilOperationData_t *opdata)
{
	TRACE_ENTER2("'%s'", opdata->objectName.value);

	/* Only create and delete operations are OK */
	if (opdata->operationType != CCBUTIL_MODIFY) {
		return SA_AIS_OK;
	} else {
		return SA_AIS_ERR_BAD_OPERATION;
	}
}

unsigned int avd_imm_config_get(void)
{
	uint32_t rc = NCSCC_RC_FAILURE;

	TRACE_ENTER();

	/*
	** Get types first since instances are dependent of them.
	**
	** Important to get the types in the right order since
	** certain validation will be done dependent on the
	** existence of another type.
	**
	** Name space validation is done per type, for instances it is done
	** by parsing the model top down. Objects "outside" the name space
	** will not be found and neglected... Downside is they will not be 
	** reported either. Off line tools can do this better.
	*/

	/* SaAmfCompType indirectly needed by SaAmfSUType */
	if (avd_comptype_config_get() != SA_AIS_OK)
		goto done;

	/* SaAmfSUType needed by SaAmfSGType */
	if (avd_sutype_config_get() != SA_AIS_OK)
		goto done;

	/* SaAmfSGType needed by SaAmfAppType */
	if (avd_sgtype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_apptype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_svctype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_cstype_config_get() != SA_AIS_OK)
		goto done;

	if (avd_compglobalattrs_config_get() != SA_AIS_OK)
		goto done;

	if (avd_cluster_config_get() != SA_AIS_OK)
		goto done;

	if (avd_node_config_get() != SA_AIS_OK)
		goto done;

	if (avd_ng_config_get() != SA_AIS_OK)
		goto done;

	if (avd_app_config_get() != SA_AIS_OK)
		goto done;

	if (avd_sidep_config_get() != SA_AIS_OK)
		goto done;

	// SGs needs to adjust configuration once all instances have been added
	{
		AVD_SG *sg;
		SaNameT dn = {.length = 0};

		for (sg = avd_sg_getnext(&dn); sg; sg = avd_sg_getnext(&dn)) {
			avd_sg_adjust_config(sg);
			dn = sg->name;
		}
	}

	rc = NCSCC_RC_SUCCESS;

done:
	if (rc == NCSCC_RC_SUCCESS)
		TRACE("AMF Configuration successfully read from IMM");
	else
		LOG_ER("Failed to read configuration, AMF will not start");

	TRACE_LEAVE2("%u", rc);
	return rc;
}

/**
 * IM object update, BLOCKING
 * @param dn
 * @param attributeName
 * @param attrValueType
 * @param value
 */
void avd_saImmOiRtObjectUpdate_sync(const SaNameT *dn, SaImmAttrNameT attributeName,
	SaImmValueTypeT attrValueType, void *value)
{
	SaAisErrorT rc;
	SaImmAttrModificationT_2 attrMod;
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
	SaImmAttrValueT attrValues[] = {value};

	TRACE_ENTER2("'%s' %s", dn->value, attributeName);

	attrMod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	attrMod.modAttr.attrName = attributeName;
	attrMod.modAttr.attrValuesNumber = 1;
	attrMod.modAttr.attrValueType = attrValueType;
	attrMod.modAttr.attrValues = attrValues;

	rc = saImmOiRtObjectUpdate_2(avd_cb->immOiHandle, dn, attrMods);
	if (rc != SA_AIS_OK) {
		LOG_WA("saImmOiRtObjectUpdate of '%s' %s failed with %u", 
			dn->value, attributeName, rc);
	}
}

/**
 * Queue an IM object update to be executed later, NON BLOCKING
 * @param dn
 * @param attributeName
 * @param attrValueType
 * @param value
 */
void avd_saImmOiRtObjectUpdate(const SaNameT *dn, SaImmAttrNameT attributeName,
	SaImmValueTypeT attrValueType, void *value)
{
	union job *ajob;
	size_t sz;

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		return;

	ajob = malloc(sizeof(*ajob));
	osafassert(ajob != NULL);
	sz = value_size(attrValueType);
	ajob->type = JOB_IMM_OBJUPDATE;
	ajob->objupdate.dn = *dn;
	ajob->objupdate.attributeName = strdup(attributeName);
	osafassert(ajob->objupdate.attributeName != NULL);
	ajob->objupdate.attrValueType = attrValueType;
	ajob->objupdate.value = malloc(sz);
	osafassert(ajob->objupdate.value != NULL);
	memcpy(ajob->objupdate.value, value, sz);
	fifo_queue(ajob);
}

/**
 * Queue an IM object create to be executed later, non blocking
 * @param className
 * @param parentName
 * @param attrValues
 */
void avd_saImmOiRtObjectCreate(const SaImmClassNameT className,
	const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	union job *ajob;

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		return;

	ajob = malloc(sizeof(*ajob));
	osafassert(ajob != NULL);
	ajob->type = JOB_IMM_OBJCREATE;
	ajob->objcreate.className = strdup(className);
	osafassert(ajob->objcreate.className != NULL);
	ajob->objcreate.parentName = *parentName;
	ajob->objcreate.attrValues = dupSaImmAttrValuesT_array(attrValues);
	fifo_queue(ajob);
}

/**
 * Queue an IM object delete to be executed later, non blocking
 * @param dn
 */
void avd_saImmOiRtObjectDelete(const SaNameT* dn)
{
	union job *ajob;

	if (avd_cb->avail_state_avd != SA_AMF_HA_ACTIVE)
		return;

	ajob = malloc(sizeof(*ajob));
	osafassert(ajob != NULL);
	ajob->type = JOB_IMM_OBJDELETE;
	ajob->objdelete.dn = *dn;
	fifo_queue(ajob);
}

/**
 * Update cached runtime attributes in IMM
 */
void avd_imm_update_runtime_attrs(void)
{
	SaNameT su_name = {0};
	AVD_SU  *su;
	SaNameT comp_name ={0};
	AVD_COMP *comp;
	SaNameT node_name = {0};
	AVD_AVND *node;
	SaNameT si_name = {0};
	AVD_SI  *si;

	/* Update SU Class runtime cached attributes. */
	su = avd_su_getnext(&su_name);
	while (su != NULL) {
		avd_saImmOiRtObjectUpdate(&su->name,
					  "saAmfSUPreInstantiable", SA_IMM_ATTR_SAUINT32T, 
					  &su->saAmfSUPreInstantiable);

		avd_saImmOiRtObjectUpdate(&su->name,
					  "saAmfSUHostedByNode", SA_IMM_ATTR_SANAMET, 
					  &su->saAmfSUHostedByNode);

		avd_saImmOiRtObjectUpdate(&su->name,
					  "saAmfSUPresenceState", SA_IMM_ATTR_SAUINT32T, 
					  &su->saAmfSUPresenceState);

		avd_saImmOiRtObjectUpdate(&su->name,
					  "saAmfSUOperState", SA_IMM_ATTR_SAUINT32T, 
					  &su->saAmfSUOperState);

		avd_saImmOiRtObjectUpdate(&su->name,
					  "saAmfSUReadinessState", SA_IMM_ATTR_SAUINT32T, 
					  &su->saAmfSuReadinessState);

		su = avd_su_getnext(&su->name);
	}

	/* Update Component Class runtime cached attributes. */
	comp = avd_comp_getnext(&comp_name);
	while (comp != NULL) {
		avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
					  "saAmfCompReadinessState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompReadinessState);

		avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
					  "saAmfCompOperState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompOperState);

		avd_saImmOiRtObjectUpdate(&comp->comp_info.name,
					  "saAmfCompPresenceState", SA_IMM_ATTR_SAUINT32T, &comp->saAmfCompPresenceState);

		comp = avd_comp_getnext(&comp->comp_info.name);
	}

	/* Update Node Class runtime cached attributes. */
	node = avd_node_getnext(&node_name);
	while (node != NULL) {
		avd_saImmOiRtObjectUpdate(&node->name, "saAmfNodeOperState",
					  SA_IMM_ATTR_SAUINT32T, &node->saAmfNodeOperState);

		node = avd_node_getnext(&node->name);
	}

	/* Update Node Class runtime cached attributes. */
	si = avd_si_getnext(&si_name);
	while (si != NULL) {
		avd_saImmOiRtObjectUpdate(&si->name, "saAmfSIAssignmentState",
					  SA_IMM_ATTR_SAUINT32T, &si->saAmfSIAssignmentState);

		si = avd_si_getnext(&si->name);
	}
}

static void free_objcreate(struct job_imm_objcreate *objcreate)
{
	int i, j;

	for (i = 0; objcreate->attrValues[i] != NULL; i++) {
		SaImmAttrValuesT_2 *attrValue =
			(SaImmAttrValuesT_2 *)objcreate->attrValues[i];
		
		if (attrValue->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			for (j = 0; j < attrValue->attrValuesNumber; j++) {
				char *p = *((char **)attrValue->attrValues[j]);
				free(p);
			}
		}
		free(attrValue->attrName);
		free(attrValue->attrValues[0]); // free blob shared by all values
		free(attrValue->attrValues);
		free(attrValue);
	}

	free(objcreate->className);
	free(objcreate->attrValues);
}

static AvdJobDequeueResultT job_exec_imm_objcreate(SaImmOiHandleT immOiHandle,
	struct job_imm_objcreate *objcreate)
{
	SaAisErrorT rc;

	TRACE_ENTER();

	rc = saImmOiRtObjectCreate_2(immOiHandle, objcreate->className,
		&objcreate->parentName, objcreate->attrValues);

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_EXIST)) {
		free_objcreate(objcreate);
		free(fifo_dequeue());
		return JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		return JOB_ETRYAGAIN;
	} else {
		free_objcreate(objcreate);
		free(fifo_dequeue());
		LOG_ER("%s: create FAILED %u", __FUNCTION__, rc);
		return JOB_ERR;
	}
}

static AvdJobDequeueResultT job_exec_imm_objupdate(SaImmOiHandleT immOiHandle,
	struct job_imm_objupdate *objupdate)
{
	SaAisErrorT rc;
	SaImmAttrModificationT_2 attrMod;
	const SaImmAttrModificationT_2 *attrMods[] = {&attrMod, NULL};
	SaImmAttrValueT attrValues[] = {objupdate->value};

	TRACE_ENTER2("%s %s", objupdate->dn.value, objupdate->attributeName);

	attrMod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	attrMod.modAttr.attrName = objupdate->attributeName;
	attrMod.modAttr.attrValuesNumber = 1;
	attrMod.modAttr.attrValueType = objupdate->attrValueType;
	attrMod.modAttr.attrValues = attrValues;

	rc = saImmOiRtObjectUpdate_2(immOiHandle, &objupdate->dn, attrMods);

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_EXIST)) {
		free(objupdate->attributeName);
		free(objupdate->value);
		free(fifo_dequeue());
		return JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		return JOB_ETRYAGAIN;
	} else {
		free(objupdate->attributeName);
		free(objupdate->value);
		free(fifo_dequeue());
		LOG_ER("%s: update FAILED %u", __FUNCTION__, rc);
		return JOB_ERR;
	}
}

static AvdJobDequeueResultT job_exec_imm_objdelete(SaImmOiHandleT immOiHandle,
	struct job_imm_objdelete *objdelete)
{
	SaAisErrorT rc;

	TRACE_ENTER2("Delete %s", objdelete->dn.value);

	rc = saImmOiRtObjectDelete(immOiHandle, &objdelete->dn);

	if ((rc == SA_AIS_OK) || (rc == SA_AIS_ERR_NOT_EXIST)) {
		free(fifo_dequeue());
		return JOB_EXECUTED;
	} else if (rc == SA_AIS_ERR_TRY_AGAIN) {
		TRACE("TRY-AGAIN");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_TIMEOUT) {
		TRACE("TIMEOUT");
		return JOB_ETRYAGAIN;
	} else if (rc == SA_AIS_ERR_BAD_HANDLE) {
		TRACE("BADHANDLE");
		avd_imm_reinit_bg();
		return JOB_ETRYAGAIN;
	} else {
		free(fifo_dequeue());
		LOG_ER("%s: delete FAILED %u", __FUNCTION__, rc);
		return JOB_ERR;
	}
}

/**
 * Dequeue one job from the FIFO and execute it.
 * 
 * @param immOiHandle
 * 
 * @return AvdJobDequeueResultT
 */
AvdJobDequeueResultT avd_job_fifo_execute(SaImmOiHandleT immOiHandle)
{
	union job *ajob;
	int ret = -1;

	if (!avd_cb->is_implementer)
		return JOB_EINVH;

	if ((ajob = fifo_peek()) == NULL)
		return JOB_ENOTEXIST;

	TRACE_ENTER();

	switch (ajob->type) {
	case JOB_IMM_OBJCREATE:
		ret = job_exec_imm_objcreate(immOiHandle, &ajob->objcreate);
		break;
	case JOB_IMM_OBJUPDATE:
		ret = job_exec_imm_objupdate(immOiHandle, &ajob->objupdate);
		break;
	case JOB_IMM_OBJDELETE:
		ret = job_exec_imm_objdelete(immOiHandle, &ajob->objdelete);
		break;
	default:
		osafassert(0);
		break;
	}

	TRACE_LEAVE2("%d", ret);
	return ret;
}

/**
 * Empty jobs in the queue, don't execute them.
 * @param immOiHandle
 */
void avd_job_fifo_empty(void)
{
	union job *ajob;

	TRACE_ENTER();

	while ((ajob = fifo_dequeue()) != NULL) {
		switch (ajob->type) {
		case JOB_IMM_OBJCREATE:
			LOG_WA("discarding create of '%s'", ajob->objcreate.className);
			free_objcreate(&ajob->objcreate);
			free(ajob);
			break;
		case JOB_IMM_OBJUPDATE:
			LOG_WA("discarding update to '%s' '%s'",
				ajob->objupdate.dn.value, ajob->objupdate.attributeName);
			free(ajob->objupdate.attributeName);
			free(ajob->objupdate.value);
			free(ajob);
			break;
		case JOB_IMM_OBJDELETE:
			LOG_WA("discarding delete of '%s'", ajob->objdelete.dn.value);
			free(ajob);
			break;
		default:
			osafassert(0);
			break;
		}
	}

	TRACE_LEAVE();
}

/**
 * Thread main to re-initialize as IMM OI.
 * @param _cb
 * 
 * @return void*
 */
static void *avd_imm_reinit_bg_thread(void *_cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	AVD_CL_CB *cb = (AVD_CL_CB *)_cb;
	AVD_EVT *evt;
	uint32_t status;

	TRACE_ENTER();

	immutilWrapperProfile.errorsAreFatal = 0;

	if ((rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &avd_callbacks, &immVersion)) != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize failed %u", rc);
		exit(EXIT_FAILURE);
	}

	if ((rc = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj)) != SA_AIS_OK) {
		LOG_ER("saImmOiSelectionObjectGet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	/* If this is the active server, become implementer again. */
	if (cb->avail_state_avd == SA_AMF_HA_ACTIVE) {
		if (avd_imm_impl_set() != SA_AIS_OK) {
			LOG_ER("exiting since avd_imm_impl_set failed");
			exit(EXIT_FAILURE);
		}
	} else {
		/* become applier and re-read the config */
		if (avd_imm_applier_set() != SA_AIS_OK) {
			LOG_ER("exiting since avd_imm_applier_set failed");
			exit(EXIT_FAILURE);
		}

		if (avd_imm_config_get() != NCSCC_RC_SUCCESS) {
			LOG_ER("avd_imm_config_get FAILED");
			exit(EXIT_FAILURE);
		}
	}

	/* Wake up the main thread so it discovers the new IMM handle. */
	evt = calloc(1, sizeof(AVD_EVT));
	osafassert(evt != NULL);
	evt->rcv_evt = AVD_IMM_REINITIALIZED;
	status = m_NCS_IPC_SEND(&avd_cb->avd_mbx, evt, NCS_IPC_PRIORITY_VERY_HIGH);
	osafassert(status == NCSCC_RC_SUCCESS);

	LOG_NO("Finished re-initializing with IMM");
	TRACE_LEAVE();
	return NULL;
}

/**
 * Finalize the current IMM OI handle and start a thread to do re-initialization.
 */
void avd_imm_reinit_bg(void)
{
	pthread_t thread;
	pthread_attr_t attr;

	TRACE_ENTER();

	LOG_NO("Re-initializing with IMM");

	(void) saImmOiFinalize(avd_cb->immOiHandle);

	avd_cb->immOiHandle = 0;
	avd_cb->is_implementer = false;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, avd_imm_reinit_bg_thread, avd_cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);

	TRACE_LEAVE();
}

/**
 * Log to SAF LOG and respond admin op to IMM
 * @param immOiHandle
 * @param invocation
 * @param result
 */
void avd_saImmOiAdminOperationResult(SaImmOiHandleT immOiHandle,
									 SaInvocationT invocation,
									 SaAisErrorT result)
{
	SaAisErrorT error;

	TRACE_ENTER2("inv:%llu, res:%u", invocation, result);
	saflog(LOG_NOTICE, amfSvcUsrName, "Admin op done for invocation: %llu, result %u",
		   invocation, result);

	error = immutil_saImmOiAdminOperationResult(immOiHandle, invocation, result);
	if (error != SA_AIS_OK)
		LOG_NO("saImmOiAdminOperationResult for %llu failed %u", invocation, error);
}

