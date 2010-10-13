/*       OpenSAF
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

#include <string>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <saImmOm.h>
#include <saImmOi.h>

#include <logtrace.h>

#include "immutil.h"
#include "smfd.h"
#include "smfsv_defs.h"
#include "SmfCampaign.hh"
#include "SmfUpgradeCampaign.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfCampaignThread.hh"
#include "SmfProcedureThread.hh"
#include "SmfUtils.hh"
#include "SmfCampState.hh"

static SaVersionT immVersion = { 'A', 2, 1 };
static const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) "safSmfService";

static const SaImmClassNameT campaignClassName = (SaImmClassNameT) "SaSmfCampaign";
static const SaImmClassNameT smfConfigClassName = (SaImmClassNameT) "OpenSafSmfConfig";

/**
 * Campaign Admin operation handling. This function is executed as an
 * IMM callback. 
 * 
 * @param immOiHandle
 * @param invocation
 * @param objectName
 * @param opId
 * @param params
 */
static void saImmOiAdminOperationCallback(SaImmOiHandleT immOiHandle, SaInvocationT invocation,
					  const SaNameT * objectName, SaImmAdminOperationIdT opId,
					  const SaImmAdminOperationParamsT_2 ** params)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();

	/* Find Campaign object out of objectName */
	SmfCampaign *campaign = SmfCampaignList::instance()->get(objectName);
	if (campaign == NULL) {
		LOG_ER("Campaign %s not found", objectName->value);
		(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, SA_AIS_ERR_INVALID_PARAM);
		goto done;
	}

	/* Call admin operation and return result */
	rc = campaign->adminOperation(opId, params);

	(void)immutil_saImmOiAdminOperationResult(immOiHandle, invocation, rc);

 done:
	TRACE_LEAVE();
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						  const SaImmClassNameT className, const SaNameT * parentName,
						  const SaImmAttrValuesT_2 ** attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the creation request" */
	ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attrMods);

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						  const SaNameT * objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the deletion request" */
	ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
						  const SaNameT * objectName,
						  const SaImmAttrModificationT_2 ** attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			LOG_ER("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the modification request" */
	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("Failed to find CCB object for %llu", ccbId);
		rc = SA_AIS_ERR_BAD_OPERATION;
		goto done;
	}

	/*
	 ** "check that the sequence of change requests contained in the CCB is 
	 ** valid and that no errors will be generated when these changes
	 ** are applied."
	 */
	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL) {
		switch (ccbUtilOperationData->operationType) {
		case CCBUTIL_CREATE:
			{
				if (strcmp(ccbUtilOperationData->param.create.className, "SaSmfCampaign") == 0) {
					SmfCampaign test(ccbUtilOperationData->param.create.parentName,
							 ccbUtilOperationData->param.create.attrValues);

					TRACE("Create campaign %s", test.getDn().c_str());
					/* Creation always allowed */
				} else {
					LOG_ER("Objects of class %s, can't be created",
					       ccbUtilOperationData->param.create.className);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
				break;
			}

		case CCBUTIL_DELETE:
		{
			//Handle the campaign object
			if (strncmp((char *)ccbUtilOperationData->param.modify.objectName->value, "safSmfCampaign=", 15) == 0) {
				TRACE("Delete campaign %s", ccbUtilOperationData->param.deleteOp.objectName->value);

				SmfCampaign *campaign =
					SmfCampaignList::instance()->get(ccbUtilOperationData->param.deleteOp.objectName);
				if (campaign == NULL) {
					LOG_ER("Campaign %s doesn't exists, can't be deleted",
					       ccbUtilOperationData->param.deleteOp.objectName->value);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				if (campaign->executing() == true) {
					LOG_ER("Campaign %s is executing, can't be deleted",
					       ccbUtilOperationData->param.deleteOp.objectName->value);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}
			//Handle the OpenSAFSmfConfig object
			}else if (strncmp((char*)ccbUtilOperationData->param.modify.objectName->value, "smfConfig=", 10) == 0) {
				LOG_ER("Deletion of SMF configuration object %s, not allowed",
				       ccbUtilOperationData->param.deleteOp.objectName->value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			//Handle any unknown object
			} else {
				LOG_ER("Unknown object %s, can't be deleted",
				       ccbUtilOperationData->param.deleteOp.objectName->value);
				rc = SA_AIS_ERR_BAD_OPERATION;
				goto done;
			}
			break;
		}

		case CCBUTIL_MODIFY:
			{
				//Handle the campaign object
				if (strncmp((char *)ccbUtilOperationData->param.modify.objectName->value, "safSmfCampaign=", 15) == 0) {
					TRACE("Modify campaign %s", ccbUtilOperationData->param.modify.objectName->value);

					/* Find Campaign object */
					SmfCampaign *campaign =
						SmfCampaignList::instance()->get(ccbUtilOperationData->param.modify.objectName);
					if (campaign == NULL) {
						LOG_ER("Campaign %s not found, can't be modified",
						       ccbUtilOperationData->param.modify.objectName->value);
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done;
					}

					rc = campaign->verify(ccbUtilOperationData->param.modify.attrMods);
					if (rc != SA_AIS_OK) {
						LOG_ER("Campaign %s attribute modification fail, wrong state or parameter content",
						       ccbUtilOperationData->param.modify.objectName->value);
						goto done;
					}
				//Handle the OpenSAFSmfConfig object
				} else if (strncmp((char*)ccbUtilOperationData->param.modify.objectName->value, "smfConfig=", 10) == 0) {
					TRACE("Modification of object %s", ccbUtilOperationData->param.modify.objectName->value);
					//Check if any campaign is executing
					if (SmfCampaignThread::instance() != NULL) {
						LOG_ER("Modification not allowed, campaign %s is executing, ",
						       SmfCampaignThread::instance()->campaign()->getDn().c_str());
						rc = SA_AIS_ERR_BAD_OPERATION;
						goto done;
					}

				//Handle any unknown object
				} else {
					LOG_ER("Unknown object %s, can't be modified",
					       ccbUtilOperationData->param.modify.objectName->value);
					rc = SA_AIS_ERR_BAD_OPERATION;
					goto done;
				}

				break;
			}
		}
		ccbUtilOperationData = ccbUtilOperationData->next;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_ER("Failed to find CCB object for %llu", ccbId);
		goto done;
	}

	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL) {
		switch (ccbUtilOperationData->operationType) {
		case CCBUTIL_CREATE:
		{
			SmfCampaign *newCampaign =
				new SmfCampaign(ccbUtilOperationData->param.create.parentName,
						ccbUtilOperationData->param.create.attrValues);

			TRACE("Adding campaign %s", newCampaign->getDn().c_str());
			SmfCampaignList::instance()->add(newCampaign);
			break;
		}

		case CCBUTIL_DELETE:
		{
			TRACE("Deleting campaign %s", ccbUtilOperationData->param.deleteOp.objectName->value);

			SmfCampaignList::instance()->del(ccbUtilOperationData->param.deleteOp.objectName);
			break;
		}

		case CCBUTIL_MODIFY:
		{
			//Handle the campaign object
			if (strncmp((char *)ccbUtilOperationData->param.modify.objectName->value, "safSmfCampaign=", 15) == 0) {
				TRACE("Modifying campaign %s", ccbUtilOperationData->param.modify.objectName->value);

				/* Find Campaign object */
				SmfCampaign *campaign =
					SmfCampaignList::instance()->get(ccbUtilOperationData->param.modify.objectName);
				if (campaign == NULL) {
					LOG_ER("Campaign %s not found",
					       ccbUtilOperationData->param.modify.objectName->value);
					goto done;
				}

				campaign->modify(ccbUtilOperationData->param.modify.attrMods);

				//Handle the OpenSAFSmfConfig object
			}else if (strncmp((char*)ccbUtilOperationData->param.modify.objectName->value, "smfConfig=", 10) == 0) {
				TRACE("Modifying configuration object %s", ccbUtilOperationData->param.modify.objectName->value);
				//Reread the SMF config object
			        read_config_and_set_control_block(smfd_cb);

				//Handle any unknown object
			} else {
				LOG_ER("Unknown object %s, can't be modified",
				       ccbUtilOperationData->param.modify.objectName->value);
				goto done;
			}
			break;
		}
		}
		ccbUtilOperationData = ccbUtilOperationData->next;
	}

 done:
	ccbutil_deleteCcbData(ccbUtilCcbData);
	TRACE_LEAVE();
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		LOG_ER("Failed to find CCB object for %llu", ccbId);

	TRACE_LEAVE();
}

/**
 * IMM requests us to update a non cached runtime attribute. 
 * @param immOiHandle
 * @param objectName
 * @param attributeNames
 * 
 * @return SaAisErrorT
 */
static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle, const SaNameT * objectName,
					       const SaImmAttrNameT * attributeNames)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrNameT attributeName;
	int i = 0;

	TRACE_ENTER();

	/* Find Campaign out of objectName */
	SmfCampaign *campaign = SmfCampaignList::instance()->get(objectName);
	if (campaign == NULL) {
		LOG_ER("saImmOiRtAttrUpdateCallback, campaign %s not found", objectName->value);
		rc = SA_AIS_ERR_FAILED_OPERATION;	/* not really covered in spec */
		goto done;
	}

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		/* We don't have any runtime attributes needing updates */
		{
			LOG_ER("saImmOiRtAttrUpdateCallback, unknown attribute %s", attributeName);
			rc = SA_AIS_ERR_FAILED_OPERATION;	/* not really covered in spec */
			goto done;
		}
	}

 done:
	TRACE_LEAVE();
	return rc;
}

static const SaImmOiCallbacksT_2 callbacks = {
	saImmOiAdminOperationCallback,
	saImmOiCcbAbortCallback,
	saImmOiCcbApplyCallback,
	saImmOiCcbCompletedCallback,
	saImmOiCcbObjectCreateCallback,
	saImmOiCcbObjectDeleteCallback,
	saImmOiCcbObjectModifyCallback,
	saImmOiRtAttrUpdateCallback
};

/**
 * Find and create all campaign objects
 */
uns32 create_campaign_objects(smfd_cb_t * cb)
{
	SaImmHandleT omHandle;
	SaImmAccessorHandleT accessorHandle;
	SaImmSearchHandleT immSearchHandle;
	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes;
	SaStringT className = campaignClassName;
	SaImmSearchParametersT_2 objectSearch;
	SmfCampaign *execCampaign = NULL;

	TRACE_ENTER();

	(void)immutil_saImmOmInitialize(&omHandle, NULL, &immVersion);
	(void)immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle);

	/* Search for all objects of class Campaign */
	objectSearch.searchOneAttr.attrName = (char*)SA_IMM_ATTR_CLASS_NAME;
	objectSearch.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	objectSearch.searchOneAttr.attrValue = &className;

	(void)immutil_saImmOmSearchInitialize_2(omHandle, NULL,	/* Search whole tree */
						SA_IMM_SUBTREE, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_ALL_ATTR, &objectSearch, NULL,	/* Get all attributes */
						&immSearchHandle);

	while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
		/* Create local object with objectName and attributes */
		SmfCampaign *newCampaign = new SmfCampaign(&objectName);
		newCampaign->init((const SaImmAttrValuesT_2 **)attributes);

		TRACE("Adding campaign %s", newCampaign->getDn().c_str());
		SmfCampaignList::instance()->add(newCampaign);

		if (newCampaign->executing() == true) {
			if (execCampaign == NULL) {
				TRACE("Campaign %s is executing", newCampaign->getDn().c_str());
				execCampaign = newCampaign;
			} else {
				LOG_ER("create_campaign_objects, more than one campaign is executing");
			}
		}
	}

	(void)immutil_saImmOmSearchFinalize(immSearchHandle);
	(void)immutil_saImmOmAccessorFinalize(accessorHandle);
	(void)immutil_saImmOmFinalize(omHandle);

	TRACE("Check if any executing campaign");

	if (execCampaign != NULL) {
		/* Start executing the campaign */
		LOG_NO("Continue executing ongoing campaign %s", execCampaign->getDn().c_str());

		if (SmfCampaignThread::start(execCampaign) == 0) {
			TRACE("Sending CAMPAIGN_EVT_EXECUTE event to thread");
			CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
			evt->type = CAMPAIGN_EVT_EXECUTE;
			SmfCampaignThread::instance()->send(evt);
		} else {
			LOG_ER("create_campaign_objects, failed to start campaign");
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Updates a runtime attribute in the IMM
 */
uns32 updateImmAttr(const char *dn, SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType, void *value)
{
	(void)immutil_update_one_rattr(smfd_cb->campaignOiHandle, dn, attributeName, attrValueType, value);

	return NCSCC_RC_SUCCESS;
}

/**
 * Activate implementer for Campaign class.
 * Retrieve the SaSmfCampaign configuration from IMM using the
 * IMM-OM interface and initialize the corresponding information
 * in the Campaign list.
 */
uns32 campaign_oi_activate(smfd_cb_t * cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	rc = immutil_saImmOiImplementerSet(cb->campaignOiHandle, implementerName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiImplementerSet fail, rc = %d inplementer name=%s", rc, (char*)implementerName);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, campaignClassName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiClassImplementerSet fail, rc = %d, classname=%s", rc, (char*)campaignClassName);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, smfConfigClassName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiClassImplementerSet fail, rc = %d classname=%s", rc, (char*)smfConfigClassName);
		return NCSCC_RC_FAILURE;
	}

	/* Create all Campaign objects found in the IMM  */
	if (create_campaign_objects(cb) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Deactivate implementer for Campaign class.
 * @param cb
 */
uns32 campaign_oi_deactivate(smfd_cb_t * cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	rc = immutil_saImmOiClassImplementerRelease(cb->campaignOiHandle, campaignClassName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiClassImplementerRelease fail, rc = %d, classname=%s", rc, (char*)campaignClassName);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiClassImplementerRelease(cb->campaignOiHandle, smfConfigClassName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiClassImplementerRelease fail, rc = %d, classname=%s", rc, (char*)smfConfigClassName);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiImplementerClear(cb->campaignOiHandle);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiImplementerClear fail, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

	/* We should terminate all threads (if exists) */
	/* and remove all local Campaign objects */
	SmfCampaignThread::terminate();

	/* The thread should now be terminated, cleanup */
	SmfCampaignList::instance()->cleanup();

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * init implementer for Campaign class.
 * @param cb
 */
uns32 campaign_oi_init(smfd_cb_t * cb)
{
	SaAisErrorT rc;
	SmfImmUtils immutil;

	TRACE_ENTER();
	rc = immutil_saImmOiInitialize_2(&cb->campaignOiHandle, &callbacks, &immVersion);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiInitialize_2 fail, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiSelectionObjectGet(cb->campaignOiHandle, &cb->campaignSelectionObject);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiSelectionObjectGet fail, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

	/* Read SMF configuration data and set cb data structure */
	uns32 ret = read_config_and_set_control_block(cb);
	TRACE_LEAVE();
	return ret;
}

/**
 * read SMF configuration object and set control block data accordingly.
 * @param cb
 */
uns32 read_config_and_set_control_block(smfd_cb_t * cb)
{
	TRACE_ENTER();
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;

	if (immutil.getObject(SMF_CONFIG_OBJECT_DN, &attributes) == false) {
		LOG_ER("Could not get SMF config object from IMM %s", SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	const char *backupCreateCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							    SMF_BACKUP_CREATE_ATTR, 0);

	if (backupCreateCmd == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_BACKUP_CREATE_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Backup create cmd = %s", backupCreateCmd);

	const char *bundleCheckCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							   SMF_BUNDLE_CHECK_ATTR, 0);

	if (bundleCheckCmd == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_BUNDLE_CHECK_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Bundle check cmd = %s", bundleCheckCmd);

	const char *nodeCheckCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							 SMF_NODE_CHECK_ATTR, 0);

	if (nodeCheckCmd == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_NODE_CHECK_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Node check cmd = %s", nodeCheckCmd);

	const char *repositoryCheckCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							       SMF_REPOSITORY_CHECK_ATTR, 0);

	if (repositoryCheckCmd == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_REPOSITORY_CHECK_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("SMF repository check cmd = %s", repositoryCheckCmd);

	const char *clusterRebootCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							     SMF_CLUSTER_REBOOT_ATTR, 0);

	if (clusterRebootCmd == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_CLUSTER_REBOOT_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Cluster reboot cmd = %s", clusterRebootCmd);

	const SaTimeT *adminOpTimeout = immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
							    SMF_ADMIN_OP_TIMEOUT_ATTR, 0);

	if (adminOpTimeout == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_ADMIN_OP_TIMEOUT_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Admin Op Timeout = %llu", *adminOpTimeout);

	const SaTimeT *cliTimeout = immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
							SMF_CLI_TIMEOUT_ATTR, 0);

	if (cliTimeout == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_CLI_TIMEOUT_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Cli Timeout = %llu", *cliTimeout);

	const SaTimeT *rebootTimeout = immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
							SMF_REBOOT_TIMEOUT_ATTR, 0);

	if (rebootTimeout == NULL) {
		LOG_ER("Failed to get attr %s from %s", SMF_REBOOT_TIMEOUT_ATTR, SMF_CONFIG_OBJECT_DN);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("Reboot Timeout = %llu", *rebootTimeout);

	const char *smfNodeBundleActCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							SMF_NODE_ACT_ATTR, 0);

	if ((smfNodeBundleActCmd == NULL) || (strcmp(smfNodeBundleActCmd,"") == 0)) {
		LOG_NO("SMF will use the STEP standard set of actions.");
	}else{
		LOG_NO("SMF will use the STEP non standard Bundle Activate set of actions.");
		LOG_NO("Node bundle activation cmd = %s", smfNodeBundleActCmd);
	}

	cb->backupCreateCmd = strdup(backupCreateCmd);
	cb->bundleCheckCmd = strdup(bundleCheckCmd);
	cb->nodeCheckCmd = strdup(nodeCheckCmd);
	cb->repositoryCheckCmd = strdup(repositoryCheckCmd);
	cb->clusterRebootCmd = strdup(clusterRebootCmd);
	cb->adminOpTimeout = *adminOpTimeout;
	cb->cliTimeout = *cliTimeout;
	cb->rebootTimeout = *rebootTimeout;
	cb->nodeBundleActCmd = strdup(smfNodeBundleActCmd);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Activate implementer for Campaign class.
 * Retrieve the SaSmfCampaign configuration from IMM using the
 * IMM-OM interface and initialize the corresponding information
 * in the Campaign list.
 */
void* smfd_coi_reinit_thread(void * _cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	smfd_cb_t * cb = (smfd_cb_t *)_cb;

	rc = immutil_saImmOiInitialize_2(&cb->campaignOiHandle, &callbacks, &immVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize_2 failed %u", rc);
		exit(EXIT_FAILURE);
	}

	rc = immutil_saImmOiSelectionObjectGet(cb->campaignOiHandle, &cb->campaignSelectionObject);
	if (rc != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiSelectionObjectGet failed %u", rc);
		exit(EXIT_FAILURE);
	}

	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		rc = immutil_saImmOiImplementerSet(cb->campaignOiHandle, implementerName);
		if (rc != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiImplementerSet failed rc=%u implementer name =%s", rc, (char*)implementerName);
			exit(EXIT_FAILURE);
		}

		rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, campaignClassName);
		if (rc != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiClassImplementerSet campaignClassName failed rc=%u class name=%s", rc, (char*)campaignClassName);
			exit(EXIT_FAILURE);
		}

		rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, smfConfigClassName);
		if (rc != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiClassImplementerSet smfConfigOiHandle failed rc=%u class name=%s", rc, (char*)smfConfigClassName);
			exit(EXIT_FAILURE);
		}
	}

	TRACE_LEAVE();
	return NULL;
}

void smfd_coi_reinit_bg(smfd_cb_t *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();
	if (pthread_create(&thread, &attr, smfd_coi_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	pthread_attr_destroy(&attr);
	
	TRACE_LEAVE();
}
