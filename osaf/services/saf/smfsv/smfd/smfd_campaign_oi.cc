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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <saImmOm.h>
#include <saImmOi.h>

#include <configmake.h>
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
#include "SmfCbkUtil.hh"

static SaVersionT immVersion = { 'A', 2, 1 };
static const SaImmOiImplementerNameT implementerName = (SaImmOiImplementerNameT) "safSmfService";

static const SaImmClassNameT campaignClassName = (SaImmClassNameT) "SaSmfCampaign";
static const SaImmClassNameT smfConfigClassName = (SaImmClassNameT) "OpenSafSmfConfig";
static const SaImmClassNameT smfSwBundleClassName = (SaImmClassNameT) "SaSmfSwBundle";

typedef enum {
        SMF_CLASS_UNKNOWN  = 0,
        SMF_CLASS_CAMPAIGN = 1,
        SMF_CLASS_BUNDLE   = 2,
        SMF_CLASS_CONFIG   = 3
} smf_classes_t;

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
		LOG_NO("Campaign %s not found", objectName->value);
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
			LOG_NO("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the creation request" */
	if (ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attrMods) == 0) {
		LOG_NO("saImmOiCcbObjectCreateCallback: Fail to add create operation for instance of %s to parent %s",
		       className, parentName->value);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

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
			LOG_NO("Failed to get CCB objectfor %llu", ccbId);
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
			LOG_NO("Failed to get CCB objectfor %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the modification request" */
	if (ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods) != 0) {
		LOG_NO("saImmOiCcbObjectModifyCallback: Fail to add modify operation for %s",
		       objectName->value);
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;	
	}

 done:
	TRACE_LEAVE();
	return rc;
}

static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *ccbUtilOperationData;
	SmfImmUtils immUtil;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_NO("Failed to find CCB object for %llu", ccbId);
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
                        //Handle the campaign object
                        if (strcmp(ccbUtilOperationData->param.create.className, campaignClassName) == 0) {
                                SmfCampaign test(ccbUtilOperationData->param.create.parentName,
                                                 ccbUtilOperationData->param.create.attrValues);
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CAMPAIGN;
                                TRACE("Create campaign %s", test.getDn().c_str()); /* Creation always allowed */

                        //Handle the SaSmfSwBundle object
                        } else if (strcmp(ccbUtilOperationData->param.create.className, smfSwBundleClassName) == 0) {
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_BUNDLE;
                                TRACE("Create a software bundle"); /* Creation always allowed */

                        //Handle the OpenSAFSmfConfig object
                        } else if (strcmp(ccbUtilOperationData->param.create.className, smfConfigClassName) == 0) {
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CONFIG;
                                LOG_NO("OpenSafSmfConfig object must be initially created");
                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;

                        //Handle unknown
                        } else {
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_UNKNOWN;
                                LOG_NO("Unknown class name %s, can't be created",
                                       ccbUtilOperationData->param.create.className);
                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;
                        }
                        break;
                }

                case CCBUTIL_DELETE:
                {
                        //Get the DN to the object to delete and read the class name
                        const std::string objToDelete = (char *)ccbUtilOperationData->param.deleteOp.objectName->value;

                        std::string className;
                        if (immUtil.getClassNameForObject(objToDelete, className) == false) {
                                LOG_NO("Failed to get class name for object to delete %s", objToDelete.c_str());
                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;
                        }

                        //Handle the campaign object
                        if (className == campaignClassName ) {
                                TRACE("Delete campaign %s", objToDelete.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CAMPAIGN;

                                //Find Campaign object
                                SmfCampaign *campaign =
                                        SmfCampaignList::instance()->get(ccbUtilOperationData->param.deleteOp.objectName); //objToDelete
                                if (campaign == NULL) {
                                        LOG_NO("Campaign %s doesn't exists, can't be deleted", objToDelete.c_str());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }

                                if (campaign->executing() == true) {
                                        LOG_NO("Campaign %s in state %u, can't be deleted", objToDelete.c_str(), campaign->getState());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }  else if ((SmfCampaignThread::instance() != NULL) &&
                                            (SmfCampaignThread::instance()->campaign() == campaign)) {
                                        //Campaign is executing prereq tests (in state INITIAL)
                                        LOG_NO("Campaign %s is executing, can't be deleted", objToDelete.c_str());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }

                        //Handle the OpenSAFSmfConfig object
                        } else if (className == smfConfigClassName){
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CONFIG;
                                LOG_NO("Deletion of SMF configuration object %s, not allowed", objToDelete.c_str());
                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;

                        //Handle the SaSmfSwBundle object
                        } else if (className == smfSwBundleClassName){
                                TRACE("Delete software bundle %s", objToDelete.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_BUNDLE;

                                //Check if the bundle is in use.
                                //Search for installed bundles
                                std::list < std::string > objectList;
                                std::list < std::string >::const_iterator objit;
                                if(immUtil.getChildren("", objectList, SA_IMM_SUBTREE, "SaAmfNodeSwBundle") == false){
                                        LOG_NO("Fail to search for SaAmfNodeSwBundle instances.");
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }
                                //Match the names of the installed bundles with the bundle requested to be removed 
                                std::string swBundleToRemove = objToDelete;
                                std::string::size_type pos1 = sizeof("safInstalledSwBundle=") - 1;

                                for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
                                        std::string::size_type pos2 = (*objit).find(",");
                                        if (pos2 == std::string::npos) {
                                                LOG_NO("saImmOiCcbCompletedCallback:: Could not find separator \",\" in %s", (*objit).c_str());
                                                rc = SA_AIS_ERR_BAD_OPERATION;
                                                goto done;
                                        }
                                        std::string swBundleInstalled = (*objit).substr(pos1, pos2 - pos1);
                                        if (swBundleInstalled.size() == 0) {
                                                LOG_NO("saImmOiCcbCompletedCallback:: Could not find installed bundle from %s", (*objit).c_str());
                                                rc = SA_AIS_ERR_BAD_OPERATION;
                                                goto done;
                                        }

                                        if (swBundleInstalled == swBundleToRemove){
                                                LOG_NO("SwBundle %s is in use, can not be removed", swBundleToRemove.c_str());
                                                rc = SA_AIS_ERR_BAD_OPERATION;
                                                goto done;
                                        }
                                }

                                goto done;
                                //Handle any unknown object
                        } else {
                                LOG_NO("Unknown object %s, can't be deleted", objToDelete.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_UNKNOWN;

                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;
                        }
                        break;
                }

                case CCBUTIL_MODIFY:
                {
                        //Get the DN to the object to modify and read the class name
                        std::string objToModify = (char *)ccbUtilOperationData->param.modify.objectName->value;
 
                        std::string className;
                        if (immUtil.getClassNameForObject(objToModify, className) == false) {
                                LOG_NO("Failed to get class name for object to modify %s", objToModify.c_str());
                                rc = SA_AIS_ERR_BAD_OPERATION;
                                goto done;
                        }

                        //Handle the campaign object
                        if (className == campaignClassName) {
                                TRACE("Modify campaign %s", objToModify.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CAMPAIGN;

                                //Find Campaign object
                                SmfCampaign *campaign =
                                        SmfCampaignList::instance()->get(ccbUtilOperationData->param.modify.objectName); //objToModify
                                if (campaign == NULL) {
                                        LOG_NO("Campaign %s not found, can't be modified", objToModify.c_str());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }

                                if (campaign->executing() == true) {
                                        LOG_NO("Campaign %s in state %u, can't be modified",
                                               objToModify.c_str(),
                                               campaign->getState());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                } else if ((SmfCampaignThread::instance() != NULL) &&
                                           (SmfCampaignThread::instance()->campaign() == campaign)) {
                                        //Campaign is executing prereq tests (in state INITIAL)
                                        LOG_NO("Campaign %s is executing, can't be modified", objToModify.c_str());
                                        rc = SA_AIS_ERR_BAD_OPERATION;
                                        goto done;
                                }

                                rc = campaign->verify(ccbUtilOperationData->param.modify.attrMods);
                                if (rc != SA_AIS_OK) {
                                        LOG_NO("Campaign %s attribute modification fail, wrong parameter content", objToModify.c_str());
                                        goto done;
                                }

                        //Handle the OpenSAFSmfConfig object
                        } else if (className == smfConfigClassName){
                                //Modification of OpenSAFSmfConfig object is always allowed.
                                //The SMF control block structure is re-read at saImmOiCcbApplyCallback
                                TRACE("Modification of object %s", objToModify.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_CONFIG;

                        //Handle the SaSmfSwBundle object
                        } else if (className == smfSwBundleClassName){
                                //Always allow modification
                                TRACE("Modification of object %s", objToModify.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_BUNDLE;

                        //Handle any unknown object
                        } else {
                                LOG_NO("Unknown object %s, can't be modified" ,objToModify.c_str());
                                //Save the class name enum for use in later phases
                                ccbUtilOperationData->userData = (void*)SMF_CLASS_UNKNOWN;
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
        bool openSAFSmfConfigApply = false;
	SmfImmUtils immUtil;

	TRACE_ENTER();

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		LOG_NO("Failed to find CCB object for %llu", ccbId);
		goto done;
	}

	ccbUtilOperationData = ccbUtilCcbData->operationListHead;
	while (ccbUtilOperationData != NULL) {
		switch (ccbUtilOperationData->operationType) {
		case CCBUTIL_CREATE:
		{
			//Handle the campaign object
                        if ((long)ccbUtilOperationData->userData == SMF_CLASS_CAMPAIGN) {
				SmfCampaign *newCampaign =
					new SmfCampaign(ccbUtilOperationData->param.create.parentName,
							ccbUtilOperationData->param.create.attrValues);

				TRACE("Adding campaign %s", newCampaign->getDn().c_str());
				SmfCampaignList::instance()->add(newCampaign);
			}
			break;
		}

		case CCBUTIL_DELETE:
		{
			//Handle the campaign object
                        if ((long)ccbUtilOperationData->userData == SMF_CLASS_CAMPAIGN) {
				TRACE("Deleting campaign %s", (char*)ccbUtilOperationData->param.deleteOp.objectName->value);
				SmfCampaignList::instance()->del(ccbUtilOperationData->param.deleteOp.objectName);
			}
			break;
		}

		case CCBUTIL_MODIFY:
		{
			//Handle the campaign object
                        if ((long)ccbUtilOperationData->userData == SMF_CLASS_CAMPAIGN) {
				TRACE("Modifying campaign %s", (char*)ccbUtilOperationData->param.modify.objectName->value);

				/* Find Campaign object */
				SmfCampaign *campaign =
					SmfCampaignList::instance()->get(ccbUtilOperationData->param.modify.objectName);
				if (campaign == NULL) {
					LOG_NO("Campaign %s not found", (char *)ccbUtilOperationData->param.modify.objectName->value);
					goto done;
				}

				campaign->modify(ccbUtilOperationData->param.modify.attrMods);

			//Handle the OpenSAFSmfConfig object
                        } else if ((long)ccbUtilOperationData->userData == SMF_CLASS_CONFIG){
				TRACE("Modifying configuration object %s", (char *)ccbUtilOperationData->param.modify.objectName->value);
                                openSAFSmfConfigApply = true;
			}
			break;
		}
		} //End switch
		ccbUtilOperationData = ccbUtilOperationData->next;
	}

        //Reread the SMF config object once
        if(openSAFSmfConfigApply == true)
                read_config_and_set_control_block(smfd_cb);

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
		LOG_NO("Failed to find CCB object for %llu", ccbId);

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
		LOG_NO("saImmOiRtAttrUpdateCallback, campaign %s not found", objectName->value);
		rc = SA_AIS_ERR_FAILED_OPERATION;	/* not really covered in spec */
		goto done;
	}

	while ((attributeName = attributeNames[i++]) != NULL) {
		TRACE("Attribute %s", attributeName);
		/* We don't have any runtime attributes needing updates */
		{
			LOG_NO("saImmOiRtAttrUpdateCallback, unknown attribute %s", attributeName);
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
uint32_t create_campaign_objects(smfd_cb_t * cb)
{
	SaImmHandleT omHandle = 0;
	SaImmAccessorHandleT accessorHandle;
	SaImmSearchHandleT immSearchHandle = 0;
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
				LOG_NO("create_campaign_objects, more than one campaign is executing");
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
			TRACE("Sending CAMPAIGN_EVT_CONTINUE event to thread");
			CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
			evt->type = CAMPAIGN_EVT_CONTINUE;
			SmfCampaignThread::instance()->send(evt);
		} else {
			LOG_NO("create_campaign_objects, failed to start campaign");
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Updates a runtime attribute in the IMM
 */
uint32_t updateImmAttr(const char *dn, SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType, void *value)
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
uint32_t campaign_oi_activate(smfd_cb_t * cb)
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

	rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, smfSwBundleClassName);
	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiClassImplementerSet fail, rc = %d classname=%s", rc, (char*)smfSwBundleClassName);
		return NCSCC_RC_FAILURE;
	}

	/* Create all Campaign objects found in the IMM  */
	if (create_campaign_objects(cb) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}

	/* Start the callback util thread */
	/* Will make it possible to run commands using the SMF API callback */
	if (SmfCbkUtilThread::start() != 0) {
		LOG_ER("Start of SmfCbkUtilThread FAILED");
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * Deactivate implementer for Campaign class.
 * @param cb
 */
uint32_t campaign_oi_deactivate(smfd_cb_t * cb)
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	/* We should terminate all threads (if exists) */
	/* and remove all local Campaign objects */
	SmfCampaignThread::terminate();

	/* The thread should now be terminated, cleanup */
	SmfCampaignList::instance()->cleanup();

	/* Stop the callback util thread */
	SmfCbkUtilThread::terminate();

        /* 
        The SMF OI class implementer information is not released. This is to avoid unneccessary fault situation during switchover
        which may increase the possibility for SMF to cause the campaign to fail.
        The drawback is the difficulty to reorganize the class implementers, if needed in the future.
        */

        /* Finalize the OI handle. This will also clear the implementer (saImmOiImplementerClear)*/
	rc = immutil_saImmOiFinalize(cb->campaignOiHandle);
	if (rc != SA_AIS_OK) {
		LOG_NO("immutil_saImmOmFinalize fail, rc = %d, continue", rc);
	} else {
                cb->campaignOiHandle = 0;
        }

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * init implementer for Campaign class.
 * @param cb
 */
uint32_t campaign_oi_init(smfd_cb_t * cb)
{
	SaAisErrorT rc;

	TRACE_ENTER();
	rc = immutil_saImmOiInitialize_2(&cb->campaignOiHandle, &callbacks, &immVersion);
	if (rc != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiInitialize_2 fail, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

	rc = immutil_saImmOiSelectionObjectGet(cb->campaignOiHandle, &cb->campaignSelectionObject);
	if (rc != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiSelectionObjectGet fail, rc = %d", rc);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/**
 * read SMF configuration object and set control block data accordingly.
 * @param cb
 */
uint32_t read_config_and_set_control_block(smfd_cb_t * cb)
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

	////////////////////////////////////////////////////
	//The attributes below is a part of a schema upgrade
	////////////////////////////////////////////////////
	const char *smfSiSwapSiName = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							     SMF_SI_SWAP_SI_NAME_ATTR, 0);
	if (smfSiSwapSiName == NULL) {
		//Not found in config object, read from smfd.conf SI_SWAP_SI
		if (getenv("SI_SWAP_SI") == NULL) {
			LOG_ER("Attr SI_SWAP_SI is not available in SMF config object or smfd.conf file");
			return NCSCC_RC_FAILURE;
		}
		smfSiSwapSiName = getenv("SI_SWAP_SI");
	}

	LOG_NO("DN for si_swap operation = %s", smfSiSwapSiName);

	const SaUint32T *smfSiSwapMaxRetry = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
							    SMF_SI_SWAP_MAX_RETRY_ATTR, 0);
	unsigned int tmp_max_swap_retry;
	if (smfSiSwapMaxRetry == NULL) {
		//Not found in config object, read from smfd.conf SI_SWAP_MAX_RETRY
		if (getenv("SI_SWAP_MAX_RETRY") == NULL) {
			LOG_ER("Attr SI_SWAP_MAX_RETRY is not available in SMF config object or smfd.conf file");
			return NCSCC_RC_FAILURE;
		}

		tmp_max_swap_retry = atoi(getenv("SI_SWAP_MAX_RETRY"));
		smfSiSwapMaxRetry = &tmp_max_swap_retry;
	}

	LOG_NO("SI si_swap operation max retry = %u", *smfSiSwapMaxRetry);

	const SaUint32T *smfCampMaxRestart = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
							    SMF_CAMP_MAX_RESTART_ATTR, 0);
	unsigned int tmp_camp_max_restart;
	if (smfCampMaxRestart == NULL) {
		//Not found in config object, read from smfd.conf CAMP_MAX_RESTART
		if (getenv("CAMP_MAX_RESTART") == NULL) {
			LOG_ER("Attr CAMP_MAX_RESTART is not available in SMF config object or smfd.conf file");
			return NCSCC_RC_FAILURE;
		}

		tmp_camp_max_restart = atoi(getenv("CAMP_MAX_RESTART"));
		smfCampMaxRestart = &tmp_camp_max_restart;
	}

	LOG_NO("Max num of campaign restarts = %u", *smfCampMaxRestart);

	const char *smfImmPersistCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							     SMF_IMM_PERSIST_CMD_ATTR, 0);
	if (smfImmPersistCmd == NULL) {
		//Not found in config object, read from smfd.conf SMF_IMM_PERSIST_CMD
		if (getenv("SMF_IMM_PERSIST_CMD") == NULL) {
			//Not found in smfd.conf. Set hardcoded value for backward compability
			LOG_NO("Attr SMF_IMM_PERSIST_CMD is not available in SMF config object or smfd.conf file. Use default cmd: immdump /etc/opensaf/imm.xml");
			smfImmPersistCmd =  "immdump " PKGSYSCONFDIR "/imm.xml";
		} else {
			smfImmPersistCmd = getenv("SMF_IMM_PERSIST_CMD");
		}
	}

	LOG_NO("IMM persist command = %s", smfImmPersistCmd);

	const char *smfNodeRebootCmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
							     SMF_NODE_REBOOT_CMD_ATTR, 0);
	if (smfNodeRebootCmd == NULL) {
		//Not found in config object, set to default "reboot".
		smfNodeRebootCmd = "reboot";
	}

	LOG_NO("Node reboot cmd = %s", smfNodeRebootCmd);

	unsigned int tmp_pbe_off;
	const SaUint32T *smfInactivatePbeDuringUpgrade = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
									    SMF_INACTIVATE_PBE_ATTR, 0);

	if (smfInactivatePbeDuringUpgrade == NULL) {
		//Not found, set default value
		tmp_pbe_off = 0;//IMM PBE will not be touched if no configuration was found
		smfInactivatePbeDuringUpgrade = &tmp_pbe_off;
	}
	LOG_NO("Turn PBE off during upgrade = %u", *smfInactivatePbeDuringUpgrade);
	
	const SaUint32T *smfVerifyEnable = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
			SMF_VERIFY_ENABLE_ATTR, 0);
	unsigned int tmp_verify_enable = 0;

	if (smfVerifyEnable == NULL) {
		//Not found, set default value
		smfVerifyEnable = &tmp_verify_enable;
		LOG_NO("Attr %s is not available in %s, using default value %d", SMF_VERIFY_ENABLE_ATTR, SMF_CONFIG_OBJECT_DN, *smfVerifyEnable);
	} else {
		LOG_NO("Verify Enable = %d", *smfVerifyEnable);
	}

	const SaTimeT *verifyTimeout = immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
							    SMF_VERIFY_TIMEOUT_ATTR, 0);
	SaTimeT tmp_verify_timeout = 100000000000LL;
	if (verifyTimeout == NULL) {
		//Not found, set default value		
		verifyTimeout = &tmp_verify_timeout;
		LOG_NO("Attr %s is not available in %s, using default value %llu", SMF_VERIFY_TIMEOUT_ATTR, SMF_CONFIG_OBJECT_DN, *verifyTimeout);
	} else {
		LOG_NO("Verify Timeout = %llu", *verifyTimeout);
	}
	//////////////////////////////////
	//End of schema upgrade attributes
	//////////////////////////////////

	cb->backupCreateCmd = strdup(backupCreateCmd);
	cb->bundleCheckCmd = strdup(bundleCheckCmd);
	cb->nodeCheckCmd = strdup(nodeCheckCmd);
	cb->repositoryCheckCmd = strdup(repositoryCheckCmd);
	cb->clusterRebootCmd = strdup(clusterRebootCmd);
	cb->adminOpTimeout = *adminOpTimeout;
	cb->cliTimeout = *cliTimeout;
	cb->rebootTimeout = *rebootTimeout;
	cb->nodeBundleActCmd = strdup(smfNodeBundleActCmd);
	cb->smfSiSwapSiName = strdup(smfSiSwapSiName);
	cb->smfSiSwapMaxRetry = *smfSiSwapMaxRetry;
	cb->smfCampMaxRestart = *smfCampMaxRestart;
	cb->smfImmPersistCmd = strdup(smfImmPersistCmd);
	cb->smfNodeRebootCmd = strdup(smfNodeRebootCmd);
	cb->smfInactivatePbeDuringUpgrade = *smfInactivatePbeDuringUpgrade;
	cb->smfVerifyEnable = *smfVerifyEnable;
	cb->smfVerifyTimeout = *verifyTimeout;
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

		rc = immutil_saImmOiClassImplementerSet(cb->campaignOiHandle, smfSwBundleClassName);
		if (rc != SA_AIS_OK) {
			LOG_ER("immutil_saImmOiClassImplementerSet smfConfigOiHandle failed rc=%u class name=%s", rc, (char*)smfSwBundleClassName);
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
