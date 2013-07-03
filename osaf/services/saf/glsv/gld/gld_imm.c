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

#include "gld.h"
#include "gld_imm.h"
#include "immutil.h"
#include "saImm.h"

#define GLSV_IMM_IMPLEMENTER_NAME (SaImmOiImplementerNameT) "safLckService"
extern struct ImmutilWrapperProfile immutilWrapperProfile;
SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = NULL,
	.saImmOiCcbApplyCallback = NULL,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = NULL,
	.saImmOiRtAttrUpdateCallback = gld_saImmOiRtAttrUpdateCallback
};

static const SaImmOiImplementerNameT implementer_name = GLSV_IMM_IMPLEMENTER_NAME;

/* IMMSv Defs */
#define GLSV_IMM_RELEASE_CODE 'A'
#define GLSV_IMM_MAJOR_VERSION 0x02
#define GLSV_IMM_MINOR_VERSION 0x01

static SaVersionT imm_version = {
	GLSV_IMM_RELEASE_CODE,
	GLSV_IMM_MAJOR_VERSION,
	GLSV_IMM_MINOR_VERSION
};

/****************************************************************************
 * Name          : gld_saImmOiRtAttrUpdateCallback
 *
 * Description   : This callback function is invoked when a OM requests for object 
 *                 information .
 *
 * Arguments     : immOiHandle      - IMM handle
 *                 objectName       - Object name (DN) 
 *                 attributeNames   - attribute names of the object to be updated
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT gld_saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
					    const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	int i = 0, attr_count = 0;
	GLSV_GLD_CB *gld_cb;
	GLSV_GLD_RSC_INFO *rsc_info;
	GLSV_GLD_RSC_MAP_INFO *map;
	SaNameT rsc_name;
	SaUint32T num_users, is_orphan, stripped_cnt;
	SaImmAttrNameT attributeName;
	SaImmAttrModificationT_2 attr_output[3];
	const SaImmAttrModificationT_2 *attrMods[4];

	SaImmAttrValueT attrUpdateValues1[] = { &num_users };
	SaImmAttrValueT attrUpdateValues2[] = { &is_orphan };
	SaImmAttrValueT attrUpdateValues3[] = { &stripped_cnt };

	/* Get GLD CB Handle. */
	gld_cb = m_GLSV_GLD_RETRIEVE_GLD_CB;
	if (gld_cb != NULL) {
		memset(&rsc_name, 0, sizeof(rsc_name));
		strncpy((char *)rsc_name.value, (char *)objectName->value, objectName->length);
		rsc_name.length = htons(objectName->length);
		map = (GLSV_GLD_RSC_MAP_INFO *)ncs_patricia_tree_get(&gld_cb->rsc_map_info, (uint8_t *)&rsc_name);
		if(map == NULL) {
			TRACE_2("ncs_patricia_tree_get returned with null");
			goto done;
		}
		rsc_info = (GLSV_GLD_RSC_INFO *)ncs_patricia_tree_get(&gld_cb->rsc_info_id, (uint8_t *)&map->rsc_id);

		if (rsc_info != NULL) {
			if (m_CMP_HORDER_SANAMET(*objectName, rsc_info->lck_name) == 0) {
				/* Walk through the attribute Name list */
				while ((attributeName = attributeNames[i]) != NULL) {

					if (strcmp(attributeName, "saLckResourceNumOpeners") == 0) {
						num_users = rsc_info->saf_rsc_no_of_users;
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValues = attrUpdateValues1;
						attrMods[attr_count] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saLckResourceIsOrphaned") == 0) {
						is_orphan = rsc_info->can_orphan;
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValues = attrUpdateValues2;
						attrMods[attr_count] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saLckResourceStrippedCount") == 0) {
						stripped_cnt = rsc_info->saf_rsc_stripped_cnt;
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValues = attrUpdateValues3;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					}

					i++;
				}	/*End while attributesNames() */

				attrMods[attr_count] = NULL;
				saImmOiRtObjectUpdate_2(gld_cb->immOiHandle, objectName, attrMods);
				return SA_AIS_OK;
			}	/* End if  m_CMP_HORDER_SANAMET */
		}		/* end of if (rsc_info != NULL) */
	}
done:
	return SA_AIS_ERR_FAILED_OPERATION;
}

/****************************************************************************
 * Name          : create_runtime_object
 *
 * Description   : This function is invoked to create a runtime object 
 *
 * Arguments     : rname            - DN of resource
 *                 create_time      - Creation time of the object 
 *                 immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT create_runtime_object(SaStringT rname, SaTimeT create_time, SaImmOiHandleT immOiHandle)
{
	SaNameT parent, *parentName = NULL;
	SaAisErrorT rc = SA_AIS_OK;
	char *dndup = strdup(rname);
	char *parent_name = strchr(rname, ',');
	char *rdnstr;
	SaImmAttrValueT arr1[1], arr2[1];
	SaImmAttrValuesT_2 attr_lckrsc, attr_LckRscCreationTimeStamp;
	const SaImmAttrValuesT_2 *attrValues[3];

	if (parent_name != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;
		parentName = &parent;
		strncpy((char *)parent.value, parent_name, sizeof(parent.value));
		parent.length = strlen((char *)parent.value);
	} else
		rdnstr = rname;

	arr1[0] = &rdnstr;
	arr2[0] = &create_time;

	attr_lckrsc.attrName = "safLock";
	attr_lckrsc.attrValueType = SA_IMM_ATTR_SASTRINGT;
	attr_lckrsc.attrValuesNumber = 1;
	attr_lckrsc.attrValues = arr1;

	attr_LckRscCreationTimeStamp.attrName = "saLckResourceCreationTimestamp";
	attr_LckRscCreationTimeStamp.attrValueType = SA_IMM_ATTR_SATIMET;
	attr_LckRscCreationTimeStamp.attrValuesNumber = 1;
	attr_LckRscCreationTimeStamp.attrValues = arr2;

	attrValues[0] = &attr_lckrsc;
	attrValues[1] = &attr_LckRscCreationTimeStamp;
	attrValues[2] = NULL;

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaLckResource", parentName, attrValues);

	free(dndup);
	return rc;

}	/* End create_runtime_object() */

/****************************************************************************
 * Name          : gld_imm_init
 *
 * Description   : Initialize the OI and get selection object  
 *
 * Arguments     : cb            
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT gld_imm_init(GLSV_GLD_CB *cb)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;
	rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK)
		immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);

	return rc;
}

/****************************************************************************
 * Name          : _gld_imm_declare_implementer
 *
 * Description   : Become a OI implementer  
 *
 * Arguments     : cb            
 *
 * Return Values : None 
 *
 * Notes         : None.
 *****************************************************************************/
void *_gld_imm_declare_implementer(void *cb)
{
	SaAisErrorT error = SA_AIS_OK;
	GLSV_GLD_CB *gld_cb = (GLSV_GLD_CB *)cb;
	error = saImmOiImplementerSet(gld_cb->immOiHandle, implementer_name);
	unsigned int nTries = 1;
	while (error == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		error = saImmOiImplementerSet(gld_cb->immOiHandle, implementer_name);
		nTries++;
	}
	if (error != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet FAILED, rc = %u", error);
		exit(EXIT_FAILURE);
	}
	gld_cb->is_impl_set = true;
	return NULL;
}

/**
 * Become object implementer, non-blocking.
 * @param cb
 */
void gld_imm_declare_implementer(GLSV_GLD_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, _gld_imm_declare_implementer, cb) != 0) {
		LOG_CR("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
}

/**
 * Initialize the OI interface and get a selection object. 
 * @param cb
 * 
 * @return SaAisErrorT
 */
static void *gld_imm_reinit_thread(void *_cb)
{
	SaAisErrorT error = SA_AIS_OK;
	GLSV_GLD_CB *cb = (GLSV_GLD_CB *)_cb;
	TRACE_ENTER();
	/* Reinitiate IMM */
	error = gld_imm_init(cb);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE) {
			_gld_imm_declare_implementer(cb);
		}
	} else {

		LOG_ER("gld_imm_initialize FAILED: %s", strerror(error));
		exit(EXIT_FAILURE);

	}
	TRACE_LEAVE();
	return NULL;
}

/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void gld_imm_reinit_bg(GLSV_GLD_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();

	if (pthread_create(&thread, &attr, gld_imm_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}
