/*
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

#include <algorithm>
#include "saAis.h"
#include "logtrace.h"
#include <immutil.h>
#include <saf_error.h>
#include "SmfUpgradeCampaign.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaign.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfProcedureThread.hh"
#include "SmfCampState.hh"
#include "SmfUtils.hh"
#include "smfd.h"

#include <saf_error.h>
#include "osaf_extended_name.h"

class SmfUpgradeProcedure;

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

// ------------------------------------------------------------------------------
// SmfUpgradeCampaign()
// ------------------------------------------------------------------------------
 SmfUpgradeCampaign::SmfUpgradeCampaign():
    m_state(SmfCampStateInitial::instance()), 
    m_campPackageName(""), m_campaignName(""), 
    m_noNameSpaceSchemaLocation(""),
    m_xsi(""), 
    m_campaignPeriod(""), 
    m_configurationBase(""), 
    m_campInit(),
    m_waitToCommit(0),
    m_waitToAllowNewCampaign(0),
    m_noOfExecutingProc(0),
    m_noOfProcResponses(0)
{

}

// ------------------------------------------------------------------------------
// ~SmfUpgradeCampaign()
// ------------------------------------------------------------------------------
SmfUpgradeCampaign::~SmfUpgradeCampaign()
{
	TRACE_ENTER();
	//Delete procedures
	std::vector < SmfUpgradeProcedure * >::iterator iter;

	for (iter = m_procedure.begin(); iter != m_procedure.end(); ++iter) {
		delete(*iter);
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// changeState()
// ------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::changeState(const SmfCampState * i_state)
{
	m_state = const_cast < SmfCampState * >(i_state);
	SmfCampaignThread::instance()->campaign()->setState(m_state->getState());
}

// ------------------------------------------------------------------------------
// getClassName(std::string& io_name)
// ------------------------------------------------------------------------------
void  
SmfUpgradeCampaign::getClassName(std::string & io_str) const
{
	io_str = "SmfUpgradeCampaign";
}

// ------------------------------------------------------------------------------
// toString()
// ------------------------------------------------------------------------------
void  
SmfUpgradeCampaign::toString(std::string & io_str) const
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// setCampPackageName()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setCampPackageName(const std::string & i_name)
{
	m_campPackageName = i_name;
}

//------------------------------------------------------------------------------
// getCampPackageName()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeCampaign::getCampPackageName(void)
{
	return m_campPackageName;
}

//------------------------------------------------------------------------------
// setCampaignName()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setCampaignName(const std::string & i_name)
{
	m_campaignName = i_name;
}

//------------------------------------------------------------------------------
// getCampaignName()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeCampaign::getCampaignName(void)
{
	return m_campaignName;
}

//------------------------------------------------------------------------------
// setXsi()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setXsi(const std::string & i_name)
{
	m_xsi = i_name;
}

//------------------------------------------------------------------------------
// setNameSpaceSchemaLocation()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setNameSpaceSchemaLocation(const std::string & i_name)
{
	m_noNameSpaceSchemaLocation = i_name;
}

//------------------------------------------------------------------------------
// setCampaignPeriod()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setCampaignPeriod(const std::string & i_period)
{
	m_campaignPeriod = i_period;
}

//------------------------------------------------------------------------------
// setDependencyDescription()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setConfigurationBase(const std::string & i_confbase)
{
	m_configurationBase = i_confbase;
}

//------------------------------------------------------------------------------
// setCampState()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setCampState(SaSmfCmpgStateT i_state)
{
	TRACE_ENTER();

	switch (i_state) {
	case SA_SMF_CMPG_INITIAL:
		{
			m_state = SmfCampStateInitial::instance();
			break;
		}

	case SA_SMF_CMPG_EXECUTING:
		{
			m_state = SmfCampStateExecuting::instance();
			break;
		}

	case SA_SMF_CMPG_SUSPENDING_EXECUTION:
		{
			m_state = SmfCampStateSuspendingExec::instance();
			break;
		}

	case SA_SMF_CMPG_EXECUTION_SUSPENDED:
		{
			m_state = SmfCampStateExecSuspended::instance();
			break;
		}

	case SA_SMF_CMPG_EXECUTION_COMPLETED:
		{
			m_state = SmfCampStateExecCompleted::instance();
			break;
		}

	case SA_SMF_CMPG_CAMPAIGN_COMMITTED:
		{
			m_state = SmfCampStateCommitted::instance();
			break;
		}

	case SA_SMF_CMPG_ERROR_DETECTED:
		{
			m_state = SmfCampStateErrorDetected::instance();
			break;
		}

	case SA_SMF_CMPG_EXECUTION_FAILED:
		{
			m_state = SmfCampStateExecFailed::instance();
			break;
		}
	case SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED:
		{
			m_state = SmfCampStateSuspendedByErrorDetected::instance();
			break;
		}
	case SA_SMF_CMPG_ERROR_DETECTED_IN_SUSPENDING:
		{
			m_state = SmfCampStateErrorDetectedInSuspending::instance();
			break;
		}
	case SA_SMF_CMPG_ROLLING_BACK:
		{
			m_state = SmfCampRollingBack::instance();
			break;
		}
	case SA_SMF_CMPG_SUSPENDING_ROLLBACK:
		{
			m_state = SmfCampSuspendingRollback::instance();
			break;
		}
	case SA_SMF_CMPG_ROLLBACK_SUSPENDED:
		{
			m_state = SmfCampRollbackSuspended::instance();
			break;
		}
	case SA_SMF_CMPG_ROLLBACK_COMPLETED:
		{
			m_state = SmfCampRollbackCompleted::instance();
			break;
		}
	case SA_SMF_CMPG_ROLLBACK_COMMITTED:
		{
			m_state = SmfCampRollbackCommitted::instance();
			break;
		}
	case SA_SMF_CMPG_ROLLBACK_FAILED:
		{
			m_state = SmfCampRollbackFailed::instance();
			break;
		}
	default:
		{
			LOG_NO("SmfUpgradeCampaign: Trying to set unknown state %d", i_state);
		}
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addUpgradeProcedure()
//------------------------------------------------------------------------------
bool 
SmfUpgradeCampaign::addUpgradeProcedure(SmfUpgradeProcedure * i_procedure)
{
	std::vector < SmfUpgradeProcedure * >::iterator iter;
 
        iter = this->m_procedure.begin();
        while (iter != this->m_procedure.end()) {
                if (!strcmp((*iter)->getProcName().c_str(), i_procedure->getProcName().c_str())){
                        LOG_NO("CAMP: Procedure %s is already present, invalid upgrade campaign", i_procedure->getProcName().c_str());
                        return false;
                }
                iter++;
        }
	m_procedure.push_back(i_procedure);
	return true;
}

//------------------------------------------------------------------------------
// getUpgradeProcedures()
//------------------------------------------------------------------------------
const std::vector < SmfUpgradeProcedure * >&
SmfUpgradeCampaign::getUpgradeProcedures()
{
	return m_procedure;
}

//------------------------------------------------------------------------------
// sortProceduresInExecLevelOrder()
//------------------------------------------------------------------------------
void SmfUpgradeCampaign::sortProceduresInExecLevelOrder()
{
	TRACE_ENTER();

	std::vector < SmfUpgradeProcedure * >::iterator iter;
	std::list < int >levels;

	//Find out used procedure execution levels, just one entry for each level
	for (iter = m_procedure.begin();iter != m_procedure.end(); iter++) {
		levels.push_back((*iter)->getExecLevel());
	}

	//Sort the execution levels and remove duplicates
	levels.sort();
	levels.unique();

	//Create a sorted procedure vector
	std::vector < SmfUpgradeProcedure * >sortedProc;

	std::list < int >::iterator levelIter;
	for (levelIter = levels.begin(); levelIter != levels.end(); levelIter++) {
		iter = m_procedure.begin();
		while (iter != m_procedure.end()) {
			if ((*levelIter) == (*iter)->getExecLevel())
				sortedProc.push_back(*iter);
			iter++;
		}
	}

	//Copy the sorted procedure vector to class member
	m_procedure = sortedProc;

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addAddToImm()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::addCampInitAddToImm(SmfImmOperation * i_operation)
{
	TRACE_ENTER();
	m_campInit.addAddToImm(i_operation);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addCampInitAction()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::addCampInitAction(SmfUpgradeAction * i_action)
{
	TRACE_ENTER();
	m_campInit.addCampInitAction(i_action);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addCampWrapupRemoveFromImm()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::addCampWrapupRemoveFromImm(SmfImmOperation * i_operation)
{
	TRACE_ENTER();
	m_campWrapup.addRemoveFromImm(i_operation);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addCampCommitAction()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::addCampCompleteAction(SmfUpgradeAction * i_action)
{
	TRACE_ENTER();
	m_campWrapup.addCampCompleteAction(i_action);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addCampWrapupAction()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::addCampWrapupAction(SmfUpgradeAction * i_action)
{
	TRACE_ENTER();
	m_campWrapup.addCampWrapupAction(i_action);
	TRACE_LEAVE();
}
//------------------------------------------------------------------------------
// setWaitToCommit()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setWaitToCommit(SaTimeT i_time)
{
	TRACE_ENTER();
	m_waitToCommit = i_time;
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// getWaitToCommit()
//------------------------------------------------------------------------------
SaTimeT
SmfUpgradeCampaign::getWaitToCommit()
{
	return m_waitToCommit;
}

//------------------------------------------------------------------------------
// setWaitToAllowNewCampaign()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::setWaitToAllowNewCampaign(SaTimeT i_time)
{
	TRACE_ENTER();
	m_waitToAllowNewCampaign = i_time;
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// getWaitToAllowNewCampaign()
//------------------------------------------------------------------------------
SaTimeT
SmfUpgradeCampaign::getWaitToAllowNewCampaign()
{
	return m_waitToAllowNewCampaign ;
}

//------------------------------------------------------------------------------
// createCampRestartInfo()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeCampaign::createCampRestartInfo()
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;

	SmfImmRTCreateOperation icoCampRestartInfo;

	icoCampRestartInfo.setClassName("OpenSafSmfCampRestartInfo");
	icoCampRestartInfo.setParentDn(SmfCampaignThread::instance()->campaign()->getDn());
	icoCampRestartInfo.setImmHandle(SmfCampaignThread::instance()->getImmHandle());

	SmfImmAttribute attrsmfRestartInfo;
        attrsmfRestartInfo.setName("smfRestartInfo");
        attrsmfRestartInfo.setType("SA_IMM_ATTR_SASTRINGT");
        attrsmfRestartInfo.addValue("smfRestartInfo=info");
        icoCampRestartInfo.addValue(attrsmfRestartInfo);

	SmfImmAttribute attrsmfCampRestartCnt;
        attrsmfCampRestartCnt.setName("smfCampRestartCnt");
        attrsmfCampRestartCnt.setType("SA_IMM_ATTR_SAUINT32T");
        attrsmfCampRestartCnt.addValue("0");
        icoCampRestartInfo.addValue(attrsmfCampRestartCnt);

        rc = icoCampRestartInfo.execute(); //Create the object
	if (rc != SA_AIS_OK){
                LOG_NO("Fail to create smfRestartInfo object, rc=%s, dn=[%s]", 
		       saf_error(rc), 
		       ("smfRestartInfo=info," + SmfCampaignThread::instance()->campaign()->getDn()).c_str());
        }

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// tooManyRestarts()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeCampaign::tooManyRestarts(bool *o_result)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrValuesT_2 **attributes;
	int curCnt = 0;

	/* Read the SmfCampRestartInfo object smfCampRestartCnt attr */
	std::string obj = "smfRestartInfo=info," + SmfCampaignThread::instance()->campaign()->getDn();
	SmfImmUtils immUtil;
	if (immUtil.getObject(obj, &attributes) == true) {
		const SaUint32T* cnt = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, "smfCampRestartCnt", 0);

		curCnt = *cnt;

		/* Increment and store counter */
		curCnt++;
		SmfImmRTUpdateOperation imoCampRestartInfo;
		std::string dn = "smfRestartInfo=info," + SmfCampaignThread::instance()->campaign()->getDn();
		imoCampRestartInfo.setDn(dn);
		imoCampRestartInfo.setImmHandle(SmfCampaignThread::instance()->getImmHandle());
		imoCampRestartInfo.setOp("SA_IMM_ATTR_VALUES_REPLACE");

		SmfImmAttribute attrsmfCampRestartCnt;
		attrsmfCampRestartCnt.setName("smfCampRestartCnt");
		attrsmfCampRestartCnt.setType("SA_IMM_ATTR_SAUINT32T");
		char buf[5];
		snprintf(buf, 4, "%d", curCnt);
		attrsmfCampRestartCnt.addValue(buf);
		imoCampRestartInfo.addValue(attrsmfCampRestartCnt);

		rc = imoCampRestartInfo.execute(); //Modify the object
		if (rc != SA_AIS_OK){
			LOG_NO("Fail to modify object, rc=%s, dn=[%s]", saf_error(rc), dn.c_str());
		}
	}

	int maxCnt = smfd_cb->smfCampMaxRestart;
	TRACE("maxCnt=%d, curCnt=%d", 	maxCnt, curCnt);	
	if (curCnt > maxCnt){
		TRACE("TRUE");
		*o_result = true;
	} else {
		TRACE("FALSE");
		*o_result = false;
	}

	TRACE_LEAVE();
	return rc;
}

// ------------------------------------------------------------------------------
// disablePbe()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeCampaign::disablePbe()
{
	TRACE_ENTER();
	if (smfd_cb->smfInactivatePbeDuringUpgrade == 0){ //False
		TRACE_LEAVE();
		return SA_AIS_OK;
	}

	//Disable IMM PBE
	//Read the attr saImmRepositoryInit in object "safRdn=immManagement,safApp=safImmService" 
	//Create a OpenSafSmfPbeIndicator object with DN "safRdn=smfPbeIndicator,safApp=safSmfService"
	//if PBE is activated i.e. saImmRepositoryInit=1

	SaAisErrorT rc = SA_AIS_OK;
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	if (immUtil.getObject("safRdn=immManagement,safApp=safImmService", &attributes) == false) {
		LOG_NO("Failed to get imm object safRdn=immManagement,safApp=safImmService");
		TRACE_LEAVE();
		return SA_AIS_ERR_ACCESS;
	}

	const SaUint32T* immRepositoryInit = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
						  "saImmRepositoryInit", 0);
	if(immRepositoryInit == NULL) {
		LOG_NO("Failed to read attribute saImmRepositoryInit in object SaImmMngt");
		TRACE_LEAVE();
		return SA_AIS_ERR_ACCESS;
	}

	if (*immRepositoryInit != 1) { //PBE not active)
		LOG_NO("IMM PBE is not active, do nothing");
		TRACE_LEAVE();
		return SA_AIS_OK;
	}

	//PBE is active
	//Disable PBE, set immRepositoryInit=2
	std::list < SmfImmOperation * > operations;
	SmfImmModifyOperation modifyOper;
	modifyOper.setDn("safRdn=immManagement,safApp=safImmService");
	modifyOper.setOp("SA_IMM_ATTR_VALUES_REPLACE");
	SmfImmAttribute saImmRepositoryInit;
	saImmRepositoryInit.setName("saImmRepositoryInit");
	saImmRepositoryInit.setType("SA_IMM_ATTR_SAUINT32T");
	saImmRepositoryInit.addValue("2");  //PBE disabled
	modifyOper.addValue(saImmRepositoryInit);
	operations.push_back(&modifyOper);
	if ((rc = immUtil.doImmOperations(operations)) != SA_AIS_OK) {
 		LOG_NO("Can not disable IMM PBE, rc=%s", saf_error(rc));
		TRACE_LEAVE();
		return rc;
        }

	//Create the OpenSafSmfPbeIndicator object
	SmfImmRTCreateOperation icoSmfPbeIndicator;

	icoSmfPbeIndicator.setClassName("OpenSafSmfPbeIndicator");
	icoSmfPbeIndicator.setParentDn(SMF_SAF_APP_DN);
	icoSmfPbeIndicator.setImmHandle(SmfCampaignThread::instance()->getImmHandle());

	SmfImmAttribute attrsafRdn;
	attrsafRdn.setName("safRdn");
	attrsafRdn.setType("SA_IMM_ATTR_SASTRINGT");
	attrsafRdn.addValue("safRdn=smfPbeIndicator");
	icoSmfPbeIndicator.addValue(attrsafRdn);

	rc = icoSmfPbeIndicator.execute(); //Create the object
	if (rc != SA_AIS_OK){
		LOG_NO("Fail to create object OpenSafSmfPbeIndicator, rc=%s, dn=[%s,%s]", saf_error(rc), "safRdn=smfPbeIndicator", SMF_SAF_APP_DN);
	}

	TRACE_LEAVE();
	return rc;
}

// ------------------------------------------------------------------------------
// restorePbe()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeCampaign::restorePbe()
{
	TRACE_ENTER();
	if (smfd_cb->smfInactivatePbeDuringUpgrade == 0){ //False
		TRACE_LEAVE();
		return SA_AIS_OK;
	}

 	//Restore IMM PBE if it was previously activated
	//Try to read object "safRdn=smfPbeIndicator,safApp=safSmfService". If found
	//set the "safRdn=immManagement,safApp=safImmService" attribute saImmRepositoryInit=1
	//to acticate IMM PBE.

	SaAisErrorT rc = SA_AIS_OK;
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	//Get the saved state if IMM PBE, if smfPbeIndicator is not found IMM PBE shall not be touched.
	//Note: When introducing SMF PBE on/off functionality (upgrade), the smfPbeIndicator will not be found and 
	//      no action will be taken.
	if (immUtil.getObject("safRdn=smfPbeIndicator," + std::string(SMF_SAF_APP_DN), &attributes) == false) {
		LOG_NO("IMM PBE was not turned off at campaign start and was not turned on at PBE restore.");
		TRACE_LEAVE();
		return SA_AIS_ERR_ACCESS;
	}

	//The object was found i.e. the IMM PBE shall be activated.
	//Delete the "safRdn=smfPbeIndicator,safApp=safSmfService" object
	std::string dn = "safRdn=smfPbeIndicator," + std::string(SMF_SAF_APP_DN);
        SaNameT objectName;
	osaf_extended_name_lend(dn.c_str(), &objectName);

	rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),
					   &objectName);
	if (rc != SA_AIS_OK) {
		LOG_NO("Fail to delete object %s,continue. %s", dn.c_str(), saf_error(rc));
	}

 	//Enable PBE, set immRepositoryInit=1
	std::list < SmfImmOperation * > operations;
	SmfImmModifyOperation modifyOper;
	modifyOper.setDn("safRdn=immManagement,safApp=safImmService");
	modifyOper.setOp("SA_IMM_ATTR_VALUES_REPLACE");
	SmfImmAttribute saImmRepositoryInit;
	saImmRepositoryInit.setName("saImmRepositoryInit");
	saImmRepositoryInit.setType("SA_IMM_ATTR_SAUINT32T");
	saImmRepositoryInit.addValue("1");  //SA_IMM_KEP_RESPOSITORY (PBE active)
	modifyOper.addValue(saImmRepositoryInit);
	operations.push_back(&modifyOper);
	if ((rc = immUtil.doImmOperations(operations)) != SA_AIS_OK) {
		LOG_NO("Can not enable IMM PBE. Fail to set saImmRepositoryInitn to SA_IMM_KEP_RESPOSITORY rc=%s, continue", saf_error(rc));
        }

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// disableCampRollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeCampaign::disableCampRollback(std::string i_reason)
{
	TRACE_ENTER();
        SmfImmUtils immUtil;
	SaAisErrorT rc = SA_AIS_OK;

	//Check if "disable rollback" object already exist
	SaImmAttrValuesT_2 **attributes;
	std::string objDn = "openSafSmfMisc=noRollback," + SmfCampaignThread::instance()->campaign()->getDn();
	if (immUtil.getObject(objDn, &attributes) == true) {
		TRACE_LEAVE();
		return rc;
	}

	SmfImmCreateOperation icoOpenSafSmfMisc;

	icoOpenSafSmfMisc.setClassName("OpenSafSmfMisc");
	icoOpenSafSmfMisc.setParentDn(SmfCampaignThread::instance()->campaign()->getDn());

	SmfImmAttribute attropenSafSmfMisc;
	attropenSafSmfMisc.setName("openSafSmfMisc");
	attropenSafSmfMisc.setType("SA_IMM_ATTR_SASTRINGT");
	attropenSafSmfMisc.addValue("openSafSmfMisc=noRollback");
	icoOpenSafSmfMisc.addValue(attropenSafSmfMisc);

	SmfImmAttribute attrreason;
	attrreason.setName("reason");
	attrreason.setType("SA_IMM_ATTR_SASTRINGT");
	attrreason.addValue(i_reason);
	icoOpenSafSmfMisc.addValue(attrreason);

	std::list <SmfImmOperation *> immOperations;
	immOperations.push_back(&icoOpenSafSmfMisc);

	rc = immUtil.doImmOperations(immOperations);
	if (rc != SA_AIS_OK) {
		LOG_NO("Can not create \"disable rollback\" object rc=%s, dn=[%s]", 
		       saf_error(rc), SmfCampaignThread::instance()->campaign()->getDn().c_str());
	} else {
		LOG_NO("Rollback disabled, reason=%s",i_reason.c_str());
	}

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// isCampRollbackDisabled()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeCampaign::isCampRollbackDisabled(bool& o_result, std::string& o_reason)
{
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	std::string objectDn = "openSafSmfMisc=noRollback," + SmfCampaignThread::instance()->campaign()->getDn();

	if (immUtil.getObject(objectDn, &attributes) == false) {
		TRACE("Can not read object, continue, dn=[%s]", objectDn.c_str());
		o_result = false;  //Rollback is allowed
		return SA_AIS_OK;
	}

	//Object is found i.e. rollback not allowed
	o_reason = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes, "reason", 0);
	o_result = true; //Rollback is NOT allowed
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::execute()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->execute(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::executeInit()
{
	TRACE_ENTER();
	//m_state->executeInit(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeProc()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::executeProc()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->executeProc(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::executeWrapup()
{
	TRACE_ENTER();
	//m_state->executeWrapup(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::rollback()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->rollback(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// verify()
//------------------------------------------------------------------------------
void
SmfUpgradeCampaign::verify()
{
	TRACE_ENTER();
	std::string error = "Verify Passed";

	if (m_state->prerequsitescheck(this, error) == SA_AIS_OK) {
		(void)m_state->verify(this, error);
	}

	SmfCampaignThread::instance()->campaign()->setError(error);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// rollbackProc()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::rollbackProc()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->rollbackProc(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::suspend()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->suspend(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::commit()
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->commit(this);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::procResult(SmfUpgradeProcedure* i_procedure, SmfProcResultT i_result)
{
	TRACE_ENTER();
        SmfCampResultT campResult;

        while (1) {
                campResult = m_state->procResult(this, i_procedure, i_result);

                if (campResult != SMF_CAMP_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// continueExec()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::continueExec()
{
	TRACE_ENTER();
        SaSmfCmpgStateT currentState = m_state->getState();

	//Check if the campaign execution continues after a campaign restart
	//resulting from a SMF ordered cluster reboot or SI_SWAP
	
	//Try to find the IMM object showing SMF initiates the campaign restart
	//If not found the campaign restart was not initiated from SMF and the campaign shall fail
	//If found the campaign restart was initiated from SMF, just remove it and continue.
	if (checkSmfRestartIndicator() != SA_AIS_OK) {
		LOG_NO("The campaign restart was not initiated by SMF, fail the campaign");
		std::string error = "Campaign was restarted by other than SMF";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return;
	}

//TODO: The "tooManyRestarts" check below is probably no longer needed when the campaign restart 
//      indicator object above is introduced.
//      For the time being it is however kept. Should probably be removed.
        /* Check if we have restarted too many times */
        bool o_result;
	if (this->tooManyRestarts(&o_result) == SA_AIS_OK){
		if (o_result == true) {
			LOG_NO("The campaign have been restarted to many times");
			int cnt = smfd_cb->smfCampMaxRestart;
			std::string error = "To many campaign restarts, max " + cnt;
			SmfCampaignThread::instance()->campaign()->setError(error);
			changeState(SmfCampStateExecFailed::instance());
			TRACE_LEAVE();
			return;
		}
	} else {
		LOG_NO("continueExec() restart number check failed");
		std::string error = "Restart number check failed";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return;
	}

        /*
         * This case happens only when a single step cluster reboot campaign suspended at the time when the cluster reboot step was executed.
         * After the reboot this function called to continue the campaign execution.
         * The campaign state is now "SUSPENDING_EXECUTION" since:
         * before the reboot the suspending event (sent to the procedure) remained pending and was not processed by the procedure thread
         * which therefore did not change the campaign state to suspended.
         * As a result: the campaign state remained suspending and the procedure state remained executing.
         */
        if(currentState == SA_SMF_CMPG_SUSPENDING_EXECUTION) {
            // The following method changes the status of the campaign to "ExecSuspended" or "Executing".
            execute();
            // Refresh local variable.
            currentState = m_state->getState();
        }

        switch (currentState) {
        case SA_SMF_CMPG_EXECUTING:
                SmfCampaignThread::instance()->campaign()->startElapsedTime();
                execute();
                break;
        case SA_SMF_CMPG_ROLLING_BACK:
                SmfCampaignThread::instance()->campaign()->startElapsedTime();
                rollback();
                break;
        default:
                TRACE("current continue state is %u, do nothing", currentState);
                break;
        }
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// resetMaintenanceState()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::resetMaintenanceState()
{
	TRACE_ENTER();

        LOG_NO("CAMP: Campaign wrapup, reset saAmfSUMaintenanceCampaign flags");
        //Find all SUs in the system
        std::list < std::string > objectList;
	SmfImmUtils immUtil;
        (void)immUtil.getChildren("", objectList, SA_IMM_SUBTREE, "SaAmfSU");

        //Reset saAmfSUMaintenanceCampaign for all found SUs
        const std::string campDn = SmfCampaignThread::instance()->campaign()->getDn();
        std::list < SmfImmOperation * > operations;
        std::list < std::string >::const_iterator suit;
	SaImmAttrValuesT_2 **attributes;

        for (suit = objectList.begin(); suit != objectList.end(); ++suit) {

		if (immUtil.getObject((*suit), &attributes) == true) {
			const SaNameT *maintCamp =
				immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
						    "saAmfSUMaintenanceCampaign",
						    0);

			if ((maintCamp != NULL) && !osaf_is_extended_name_empty(maintCamp)) {
				SmfImmModifyOperation *modop = new (std::nothrow) SmfImmModifyOperation;
				osafassert(modop != 0);
				modop->setDn(*suit);
				modop->setOp("SA_IMM_ATTR_VALUES_DELETE");
				SmfImmAttribute saAmfSUMaintenanceCampaign;
				saAmfSUMaintenanceCampaign.setName("saAmfSUMaintenanceCampaign");
				saAmfSUMaintenanceCampaign.setType("SA_IMM_ATTR_SANAMET");
				saAmfSUMaintenanceCampaign.addValue(campDn);
				modop->addValue(saAmfSUMaintenanceCampaign);
				operations.push_back(modop);
			}
		}
        }

        if (immUtil.doImmOperations(operations) != SA_AIS_OK) {
                LOG_NO("SmfUpgradeStep::setMaintenanceState(), fails to reset all saAmfSUMaintenanceCampaign attr");
        }

        //Delete the created SmfImmModifyOperation instances
        std::list < SmfImmOperation * > ::iterator operIter;
        for (operIter = operations.begin(); operIter != operations.end(); ++operIter) {
                delete (*operIter);
        }

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// removeRunTimeObjects()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::removeRunTimeObjects()
{
	TRACE_ENTER();

	/* Remove smfRestartInfo runtime object */
        LOG_NO("CAMP: Campaign wrapup, Remove runtime objects");

        SaNameT objectName;
	std::string dn = "smfRestartInfo=info," + SmfCampaignThread::instance()->campaign()->getDn(); 
	osaf_extended_name_lend(dn.c_str(), &objectName);

	SaAisErrorT rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),	//The OI handle
					   &objectName);

	if (rc != SA_AIS_OK) {
		LOG_NO("Fail to delete runtime object, rc=%s, dn=[%s], continue", saf_error(rc), dn.c_str());
	}

	/* Remove campaign rollback element runtime objects */
        dn = "smfRollbackElement=AddToImmCcb," + SmfCampaignThread::instance()->campaign()->getDn(); 
	osaf_extended_name_lend(dn.c_str(), &objectName);

	rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),	//The OI handle
                                           &objectName);

	if (rc != SA_AIS_OK) {
		LOG_NO("Fail to delete object, rc=%s, dn=[%s], continuing",  saf_error(rc), dn.c_str());
	}

        dn = "smfRollbackElement=CampInit," + SmfCampaignThread::instance()->campaign()->getDn(); 
	osaf_extended_name_lend(dn.c_str(), &objectName);

	rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),	//The OI handle
					   &objectName);

	if (rc != SA_AIS_OK) {
		LOG_NO("Fail to delete object, rc=%s, dn=[%s], continuing",  saf_error(rc), dn.c_str());
	}

        dn = "smfRollbackElement=CampComplete," + SmfCampaignThread::instance()->campaign()->getDn(); 
	osaf_extended_name_lend(dn.c_str(), &objectName);

	rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),	//The OI handle
					   &objectName);

	if (rc != SA_AIS_OK) {
		LOG_NO("Fail to delete object, rc=%s, dn=[%s], continuing",  saf_error(rc), dn.c_str());
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// removeConfigObjects()
//------------------------------------------------------------------------------
void 
SmfUpgradeCampaign::removeConfigObjects()
{
	TRACE_ENTER();
	//This method deletes config objects which has possibly been creared during the campaign execution
	//All error codes are ignored

        LOG_NO("CAMP: Campaign wrapup, Remove config objects");
        SmfImmUtils immUtil;

	//Always try to delete the "no rollback" indicator, don't care if it fails for any reason
	SmfImmDeleteOperation doNoRollbackIndicator;
	std::string objectDn = "openSafSmfMisc=noRollback," + SmfCampaignThread::instance()->campaign()->getDn();
	doNoRollbackIndicator.setDn(objectDn);

	std::list <SmfImmOperation *> immOperations;
	immOperations.push_back(&doNoRollbackIndicator);
	SaAisErrorT rc = immUtil.doImmOperations(immOperations);
	if (rc != SA_AIS_OK) {
		TRACE("Fail to delete \"no rollback\" indicator rc=%s continue, dn=[%s]", saf_error(rc), objectDn.c_str());
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// createSmfRestartIndicator()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeCampaign::createSmfRestartIndicator()
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrValuesT_2 **attributes;
	std::string parentDn = SMF_SAF_APP_DN;
	std::string rdn      = SMF_CAMP_RESTART_INDICATOR_RDN;
	std::string objDn    = rdn + "," + parentDn;
	SmfImmUtils immUtil;

	//Check if the object exist
	rc = immUtil.getObjectAisRC(objDn, &attributes);
	if (rc == SA_AIS_ERR_NOT_EXIST) {  //If not exist, create the object

		SmfImmRTCreateOperation icoCampaignRestartInd;

		icoCampaignRestartInd.setClassName("OpenSafSmfCampRestartIndicator");
		icoCampaignRestartInd.setParentDn(parentDn);
		icoCampaignRestartInd.setImmHandle(SmfCampaignThread::instance()->getImmHandle());

		SmfImmAttribute attrRestInd;
		attrRestInd.setName("smfCampaignRestartIndicator");
		attrRestInd.setType("SA_IMM_ATTR_SASTRINGT");
		attrRestInd.addValue(rdn);
		icoCampaignRestartInd.addValue(attrRestInd);

		rc = icoCampaignRestartInd.execute(); //Create the object
		if (rc != SA_AIS_OK){
			LOG_NO("Creation of OpenSafSmfCampRestartIndicator fails, rc= %s, dn=[%s]", saf_error(rc), objDn.c_str());
		}
	} else {
		TRACE("OpenSafSmfRestart object already exist, no create. dn=[%s]", objDn.c_str());
	}

	TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// checkSmfRestartIndicator()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeCampaign::checkSmfRestartIndicator()
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrValuesT_2 **attributes;
	std::string objDn    = std::string(SMF_CAMP_RESTART_INDICATOR_RDN) + "," + std::string(SMF_SAF_APP_DN);
	SmfImmUtils immUtil;

	//This is the place to be very patient regarding object access. A restart have just occured.
	//If the result differs from the expected outcome, retry a couple of times
	//An error will cause the campaign to fail and a restore must be performed.

	//Check if the object exist
	unsigned int retries = 1;
	rc = immUtil.getObjectAisRC(objDn, &attributes);

	while (rc != SA_AIS_OK && rc != SA_AIS_ERR_NOT_EXIST && retries < 30) {
		sleep(1);
		rc = immUtil.getObjectAisRC(objDn, &attributes);
		retries++;
	}

	if (rc == SA_AIS_ERR_NOT_EXIST) {  
		//If not exist, the campaign has ben restarted by other than SMF
		//This is OK if the campaign is in a state waiting for operator operations
		SaSmfCmpgStateT currentState = m_state->getState();
		switch (currentState) {
		case SA_SMF_CMPG_EXECUTION_SUSPENDED:
		case SA_SMF_CMPG_EXECUTION_COMPLETED:
		case SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED:
		case SA_SMF_CMPG_ROLLBACK_SUSPENDED:
		case SA_SMF_CMPG_ROLLBACK_COMPLETED:
			LOG_NO("CAMP: An unexpected campaign restart occured in campaign=%s, campState=%d. OK in this state.", 
			       SmfCampaignThread::instance()->campaign()->getDn().c_str(), currentState);
			rc = SA_AIS_OK;
			break;
		default:
			//The campaign was executing while an unexpected campaign restart occured
			LOG_NO("An unexpected campaign restart occured in campaign=%s, campState=%d.", 
			       SmfCampaignThread::instance()->campaign()->getDn().c_str(), currentState);
			rc = SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED;
			break;
		}
	} else {
		TRACE("OpenSafSmfRestartIndicator object exist, the campaign restart was expected");
		//Remove the indicator object
		SaNameT objectName;
                osaf_extended_name_lend(objDn.c_str(), &objectName);

		retries = 1;
		rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),
						   &objectName);
		while (rc != SA_AIS_OK && retries < 10) {
			sleep(1);
			rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),
							   &objectName);
			retries++;
		}
		if (rc != SA_AIS_OK) {
			LOG_NO("Could not remove OpenSafSmfCampRestart object dn=%s", objDn.c_str());
		}
        }

	TRACE_LEAVE();
        return rc;
}
