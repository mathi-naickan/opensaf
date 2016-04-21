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
#include <poll.h>
#include <sched.h>

#include <ncssysf_def.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tsk.h>

#include <saAis.h>
#include <saSmf.h>
#include <stdio.h>
#include <logtrace.h>
#include <sstream>
#include <saf_error.h>
#include "osaf_time.h"
#include "osaf_extended_name.h"

#include "SmfUpgradeStep.hh"
#include "SmfCampaign.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfProcedureThread.hh"
#include "SmfUpgradeMethod.hh"
#include "SmfProcedureThread.hh"
#include "SmfStepState.hh"
#include "SmfStepTypes.hh"
#include "SmfCampaignThread.hh"
#include "SmfTargetTemplate.hh"
#include "SmfRollback.hh"
#include "SmfUtils.hh"
#include "immutil.h"
#include "smfd_smfnd.h"
#include "smfd.h"
#include "osaf_time.h"

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

//================================================================================
// Class SmfUpgradeStep
// Purpose:
// Comments:
//================================================================================

SmfUpgradeStep::SmfUpgradeStep():
   m_state(SmfStepStateInitial::instance()),
   m_stepState(SA_SMF_STEP_INITIAL),
   m_maxRetry(0), 
   m_retryCount(0), 
   m_restartOption(1), //True
   m_procedure(NULL),
   m_stepType(NULL),
   m_switchOver(false)
{
}

// ------------------------------------------------------------------------------
// ~SmfUpgradeStep()
// ------------------------------------------------------------------------------
SmfUpgradeStep::~SmfUpgradeStep()
{
	std::list < SmfImmOperation * >::iterator it;

	/* Delete Imm operations */
	for (it = m_modificationList.begin(); it != m_modificationList.end(); ++it) {
		delete(*it);
	}

        delete m_stepType;
}

// ------------------------------------------------------------------------------
// init()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::init(const SaImmAttrValuesT_2 ** attrValues)
{
	const SaImmAttrValuesT_2 **attribute;

	for (attribute = attrValues; *attribute != NULL; attribute++) {
		void *value;

		if ((*attribute)->attrValuesNumber != 1) {
			TRACE("invalid number of values %u for %s", (*attribute)->attrValuesNumber,
			       (*attribute)->attrName);
			continue;
		}

		value = (*attribute)->attrValues[0];

		if (strcmp((*attribute)->attrName, "safSmfStep") == 0) {
			char *rdn = *((char **)value);
			m_rdn = rdn;
			TRACE("init safSmfStep = %s", rdn);
		} else if (strcmp((*attribute)->attrName, "saSmfStepMaxRetry") == 0) {
			unsigned int maxRetry = *((unsigned int *)value);
			m_maxRetry = maxRetry;
			TRACE("init saSmfStepMaxRetry = %u", m_maxRetry);
		} else if (strcmp((*attribute)->attrName, "saSmfStepRetryCount") == 0) {
			unsigned int retryCount = *((unsigned int *)value);
			m_retryCount = retryCount;
			TRACE("init saSmfStepRetryCount = %u", m_retryCount);
		} else if (strcmp((*attribute)->attrName, "saSmfStepRestartOption") == 0) {
			unsigned int restartOption = *((unsigned int *)value);
			m_restartOption = restartOption;
			TRACE("init saSmfStepRestartOption = %u", m_restartOption);
		} else if (strcmp((*attribute)->attrName, "saSmfStepState") == 0) {
			unsigned int state = *((unsigned int *)value);

			if ((state >= SA_SMF_STEP_INITIAL) && (state <= SA_SMF_STEP_ROLLBACK_FAILED)) {
				setStepState((SaSmfStepStateT) state);
			} else {
				LOG_NO("SmfUpgradeStep: invalid step state %u", state);
				setStepState(SA_SMF_STEP_INITIAL);
			}
			TRACE("init saSmfStepState = %u", (int)state);
		} else {
			TRACE("init unhandled attribute = %s", (*attribute)->attrName);
		}
	}

	return SA_AIS_OK;
}

// ------------------------------------------------------------------------------
// changeState()
// ------------------------------------------------------------------------------
void 
SmfUpgradeStep::changeState(const SmfStepState * i_state)
{
	TRACE_ENTER();
	//Change state class pointer
	m_state = const_cast < SmfStepState * >(i_state);

	setImmStateAndSendNotification(i_state->getState());

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// getState()
//------------------------------------------------------------------------------
SaSmfStepStateT 
SmfUpgradeStep::getState() const
{
	return m_state->getState();

}

//------------------------------------------------------------------------------
// setStepState()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setStepState(SaSmfStepStateT i_state)
{
	TRACE_ENTER();

	switch (i_state) {
	case SA_SMF_STEP_INITIAL:
		{
			m_state = SmfStepStateInitial::instance();
			break;
		}

	case SA_SMF_STEP_EXECUTING:
		{
			m_state = SmfStepStateExecuting::instance();
			break;
		}

	case SA_SMF_STEP_UNDOING:
		{
			m_state = SmfStepStateUndoing::instance();
			break;
		}

	case SA_SMF_STEP_COMPLETED:
		{
			m_state = SmfStepStateCompleted::instance();
			break;
		}

	case SA_SMF_STEP_UNDONE:
		{
			m_state = SmfStepStateUndone::instance();
			break;
		}

	case SA_SMF_STEP_FAILED:
		{
			m_state = SmfStepStateFailed::instance();
			break;
		}

	case SA_SMF_STEP_ROLLING_BACK:
		{
			m_state = SmfStepStateRollingBack::instance();
			break;
		}

	case SA_SMF_STEP_UNDOING_ROLLBACK:
		{
			m_state = SmfStepStateUndoingRollback::instance();
			break;
		}

	case SA_SMF_STEP_ROLLED_BACK:
		{
			m_state = SmfStepStateRolledBack::instance();
			break;
		}

	case SA_SMF_STEP_ROLLBACK_UNDONE:
		{
			m_state = SmfStepStateRollbackUndone::instance();
			break;
		}

	case SA_SMF_STEP_ROLLBACK_FAILED:
		{
			m_state = SmfStepStateRollbackFailed::instance();
			break;
		}
	default:
		{
			LOG_NO("SmfUpgradeStep::setStepState unknown state %d", i_state);
		}
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// setRdn()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setRdn(const std::string & i_rdn)
{
	m_rdn = i_rdn;
}

//------------------------------------------------------------------------------
// getRdn()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeStep::getRdn() const
{
	return m_rdn;
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// getDn()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeStep::getDn() const
{
	return m_dn;
}

//------------------------------------------------------------------------------
// setMaxRetry()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setMaxRetry(unsigned int i_maxRetry)
{
	m_maxRetry = i_maxRetry;
}

//------------------------------------------------------------------------------
// getMaxRetry()
//------------------------------------------------------------------------------
unsigned int 
SmfUpgradeStep::getMaxRetry() const
{
	return m_maxRetry;
}

//------------------------------------------------------------------------------
// setRetryCount()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setRetryCount(unsigned int i_retryCount)
{
	m_retryCount = i_retryCount;
}

//------------------------------------------------------------------------------
// getRetryCount()
//------------------------------------------------------------------------------
unsigned int 
SmfUpgradeStep::getRetryCount() const
{
	return m_retryCount;
}

//------------------------------------------------------------------------------
// setRestartOption()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setRestartOption(unsigned int i_restartOption)
{
	m_restartOption = i_restartOption;
}

//------------------------------------------------------------------------------
// getRestartOption()
//------------------------------------------------------------------------------
unsigned int 
SmfUpgradeStep::getRestartOption() const
{
	return m_restartOption;
}

//------------------------------------------------------------------------------
// setImmStateAndSendNotification()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setImmStateAndSendNotification(SaSmfStepStateT i_state)
{
	TRACE_ENTER();

	if (m_procedure == NULL) {
		return;
	}

	SaSmfStepStateT oldState = m_stepState;
	m_stepState = i_state;

	m_procedure->getProcThread()->updateImmAttr(this->getDn().c_str(), (char*)"saSmfStepState", SA_IMM_ATTR_SAUINT32T, &i_state);

	SmfCampaignThread::instance()->sendStateNotification(m_dn, MINOR_ID_STEP, SA_NTF_MANAGEMENT_OPERATION, SA_SMF_STEP_STATE, i_state, oldState);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// addActivationUnit()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addActivationUnit(const unitNameAndState & i_activationUnit)
{
	m_activationUnit.m_actedOn.push_back(i_activationUnit);
}

//------------------------------------------------------------------------------
// getActivationUnits()
//------------------------------------------------------------------------------
const std::list < unitNameAndState > &
SmfUpgradeStep::getActivationUnitList()
{
	return m_activationUnit.m_actedOn;
}

//------------------------------------------------------------------------------
// addDeactivationUnit()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addDeactivationUnit(const unitNameAndState & i_deactivationUnit)
{
	m_deactivationUnit.m_actedOn.push_back(i_deactivationUnit);
}

//------------------------------------------------------------------------------
// getDeactivationUnits()
//------------------------------------------------------------------------------
const std::list < unitNameAndState > &
SmfUpgradeStep::getDeactivationUnitList()
{
	return m_deactivationUnit.m_actedOn;
}

//------------------------------------------------------------------------------
// addSwRemove()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addSwRemove(const std::list < SmfBundleRef * >&i_swRemove)
{
	std::list < SmfBundleRef * >::const_iterator it = i_swRemove.begin();
	while (it != i_swRemove.end()) {
		SmfBundleRef newSwRemove = *(*it);
		m_swRemoveList.push_back(newSwRemove);
		it++;
	}
}
void 
SmfUpgradeStep::addSwRemove(std::list<SmfBundleRef> const& i_swRemove)
{
	m_swRemoveList = i_swRemove;
}

//------------------------------------------------------------------------------
// getSwRemoveList()
//------------------------------------------------------------------------------
std::list < SmfBundleRef > &
SmfUpgradeStep::getSwRemoveList()
{
	return m_swRemoveList;
}

//------------------------------------------------------------------------------
// addSwAdd()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addSwAdd(const std::list < SmfBundleRef * >&i_swAdd)
{
	std::list < SmfBundleRef * >::const_iterator it = i_swAdd.begin();
	while (it != i_swAdd.end()) {
		SmfBundleRef newSwAdd = *(*it);
		m_swAddList.push_back(newSwAdd);
		it++;
	}
}
void 
SmfUpgradeStep::addSwAdd(std::list<SmfBundleRef> const& i_swAdd)
{
	m_swAddList = i_swAdd;
}

//------------------------------------------------------------------------------
// getSwAddList()
//------------------------------------------------------------------------------
std::list < SmfBundleRef > &
SmfUpgradeStep::getSwAddList()
{
	return m_swAddList;
}

//------------------------------------------------------------------------------
// addModification()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addModification(SmfImmModifyOperation * i_modification)
{
	m_modificationList.push_back(i_modification);
}

//------------------------------------------------------------------------------
// getModifications()
//------------------------------------------------------------------------------
std::list < SmfImmOperation * >&
SmfUpgradeStep::getModifications()
{
	return m_modificationList;
}

//------------------------------------------------------------------------------
// addImmOperation()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addImmOperation(SmfImmOperation* i_immoperation)
{
	m_modificationList.push_back(i_immoperation);
}

//------------------------------------------------------------------------------
// setSwNode()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setSwNode(const std::string & i_swNode)
{
	m_swNode = i_swNode;
}

//------------------------------------------------------------------------------
// getSwNode()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeStep::getSwNode()
{
	return m_swNode;
}

//------------------------------------------------------------------------------
// addSwNode()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::addSwNode(const std::string& i_swNode)
{
        m_swNodeList.push_back(i_swNode);
}

//------------------------------------------------------------------------------
// removeSwNodeListDuplicates()
//------------------------------------------------------------------------------
void
SmfUpgradeStep::removeSwNodeListDuplicates()
{
        m_swNodeList.sort();
        m_swNodeList.unique();
}

//------------------------------------------------------------------------------
// getSwNodeList()
//------------------------------------------------------------------------------
const std::list<std::string> & 
SmfUpgradeStep::getSwNodeList() 
{
	return m_swNodeList;
}

//------------------------------------------------------------------------------
// getSsAffectedNodeList()
//------------------------------------------------------------------------------
const std::list<std::string> & 
SmfUpgradeStep::getSsAffectedNodeList() 
{
	return m_ssAffectedNodeList;
}

//------------------------------------------------------------------------------
// setStepType()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setStepType(SmfStepType* i_type)
{
        delete m_stepType;
	m_stepType = i_type;
}

//------------------------------------------------------------------------------
// getStepType()
//------------------------------------------------------------------------------
SmfStepType* 
SmfUpgradeStep::getStepType()
{
	return m_stepType;
}

//------------------------------------------------------------------------------
// offlineInstallNewBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::offlineInstallNewBundles()
{
	TRACE("Offline install new bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_OFFLINE_INSTALL, m_swAddList, getSwNode());
}

//------------------------------------------------------------------------------
// onlineInstallNewBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::onlineInstallNewBundles()
{
	TRACE("Online install new bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_ONLINE_INSTALL, m_swAddList, getSwNode());
}

//------------------------------------------------------------------------------
// offlineRemoveOldBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::offlineRemoveOldBundles()
{
	TRACE("Offline remove old bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_OFFLINE_REMOVE, m_swRemoveList, getSwNode());
}

//------------------------------------------------------------------------------
// onlineRemoveOldBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::onlineRemoveOldBundles()
{
	TRACE("Online remove old bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_ONLINE_REMOVE, m_swRemoveList, getSwNode());
}

//------------------------------------------------------------------------------
// offlineInstallOldBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::offlineInstallOldBundles()
{
	TRACE("Offline install old bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_OFFLINE_INSTALL, m_swRemoveList, getSwNode());
}

//------------------------------------------------------------------------------
// onlineInstallOldBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::onlineInstallOldBundles()
{
	TRACE("Online install old bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_ONLINE_INSTALL, m_swRemoveList, getSwNode());
}

//------------------------------------------------------------------------------
// offlineRemoveNewBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::offlineRemoveNewBundles()
{
	TRACE("Offline remove new bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_OFFLINE_REMOVE, m_swAddList, getSwNode());
}

//------------------------------------------------------------------------------
// onlineRemoveNewBundles()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::onlineRemoveNewBundles()
{
	TRACE("Online remove new bundles on node %s", getSwNode().c_str());
	return callBundleScript(SMF_STEP_ONLINE_REMOVE, m_swAddList, getSwNode());
}

//------------------------------------------------------------------------------
// onlineRemoveBundlesUserList()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::onlineRemoveBundlesUserList(const std::string & i_node, const std::list < SmfBundleRef > &i_bundleList)
{
	TRACE("Online remove bundles supplied by separate list containing %zu items on node %s", i_bundleList.size(), i_node.c_str());
	return callBundleScript(SMF_STEP_ONLINE_REMOVE, i_bundleList, i_node);
}

//------------------------------------------------------------------------------
// lockDeactivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::lockDeactivationUnits()
{
	TRACE("lock deactivation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL};
	return callAdminOperation(SA_AMF_ADMIN_LOCK, params, m_deactivationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// unlockDeactivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::unlockDeactivationUnits()
{
	TRACE("unlock deactivation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_UNLOCK, params, m_deactivationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// terminateDeactivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::terminateDeactivationUnits()
{
	TRACE("terminate deactivation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL};
	return callAdminOperation(SA_AMF_ADMIN_LOCK_INSTANTIATION, params, m_deactivationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// instantiateDeactivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::instantiateDeactivationUnits()
{
	TRACE("instantiate deactivation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_UNLOCK_INSTANTIATION, params, m_deactivationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// lockActivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::lockActivationUnits()
{
	TRACE("lock activation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_LOCK, params, m_activationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// unlockActivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::unlockActivationUnits()
{
	TRACE("unlock activation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_UNLOCK, params, m_activationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// terminateActivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::terminateActivationUnits()
{
	TRACE("terminate activation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_LOCK_INSTANTIATION, params, m_activationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// instantiateActivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::instantiateActivationUnits()
{
	TRACE("instantiate activation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_UNLOCK_INSTANTIATION, params, m_activationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// restartActivationUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::restartActivationUnits()
{
	TRACE("restart activation units");
	const SaImmAdminOperationParamsT_2 *params[1] = {NULL}; 
	return callAdminOperation(SA_AMF_ADMIN_RESTART, params, m_activationUnit.m_actedOn);
}

//------------------------------------------------------------------------------
// modifyInformationModel()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::modifyInformationModel()
{
        SaAisErrorT rc;
        std::string modifyRollbackCcbDn;
        modifyRollbackCcbDn = "smfRollbackElement=ModifyCcb,";
        modifyRollbackCcbDn += this->getDn();

        SaNameT objectName;
        osaf_extended_name_lend(modifyRollbackCcbDn.c_str(), &objectName);
	uint32_t retry_count = 0;

        /* In case of a undoing the rollback could already exists, delete it and recreate a new one */
	rc = immutil_saImmOiRtObjectDelete(getProcedure()->getProcThread()->getImmHandle(), &objectName);

	if (rc != SA_AIS_OK) {
		TRACE("immutil_saImmOiRtObjectDelete returned %s for %s, continuing", saf_error(rc), modifyRollbackCcbDn.c_str());
	}

        if ((rc = smfCreateRollbackElement(modifyRollbackCcbDn,
                                           getProcedure()->getProcThread()->getImmHandle())) != SA_AIS_OK) {
                LOG_NO("SmfUpgradeStep failed to create modify rollback element %s, rc=%s", 
                       modifyRollbackCcbDn.c_str(), saf_error(rc));
                return rc;
        }

	if (m_modificationList.size() > 0) {
		TRACE("Modifying information model");
		SmfImmUtils immUtil;

	while (1) {
	int interval = 5; // seconds
                SmfRollbackCcb rollbackCcb(modifyRollbackCcbDn,
                                           getProcedure()->getProcThread()->getImmHandle());

		rc = immUtil.doImmOperations(m_modificationList, &rollbackCcb);
		if (((rc == SA_AIS_ERR_TIMEOUT) || (rc == SA_AIS_ERR_NOT_EXIST)) && (retry_count <= 6)) {
			// When IMM aborts a CCB because of synch request from a payload, then
			// the next call of CCBInitialize() will return TRY_AGAIN till the time
			// the synch is complete.
			// There is no direct information available to the OM that can indicate
			// that the CCB or the Adminownerset failed because of an abort and also
			// there is no notification that can indicate that IMM is ready now.
			// That leaves SMF with the option to correlate error codes returned.
			// 
			// Notes on treatment of SA_AIS_ERR_TIMEOUT and SA_AIS_ERR_NOT_EXIST error codes:
			//
			// 1) CCB abort when it is not the first operation(create/modify/delete) in the CCB
			//    and if there is dependency between objects in the CCB:-
			//
			// An abort of a CCB and if the objects(Create/Modify/delete) had
			// some dependency(parent-child) among them, then an API call of
			// AdminOwnerSet() or the CCBCreate/Delete/Modify() on a dependant object will return
			// SA_AIS_ERR_NOT_EXIST, because the CCB aborted.
			//
			// 2) CCB abort when it is a first operation and/or there is no intra-ccb objects-dependency:-
			//
			// When an ongoing CCB is aborted because of a synch request originating from a payload,
			// then the AdminOwnerSet() or the CCBCreate/Delete/Modify() will return timeout.

			++retry_count;
			LOG_NO("SmfUpgradeStep modify IMM failed with error: %s", saf_error(rc));
			LOG_NO("CCB was aborted!?, Retrying: %u", retry_count);
			// Total retry time of 2.5 minutes for a worst case IMM loaded with say < 300k objects. 
			// Retry every 25 seconds. i.e. (nanosleep for 5 seconds)  + (immutil_ccbInitialize will
			// worstcase wait till 20 seconds).
                        struct timespec sleepTime = { interval, 0 };
			osaf_nanosleep(&sleepTime);
			// Note: Alternatively Make rollbackCcb unique by adding a method for this to the rollbackCcb.
			continue;
		} else if (rc != SA_AIS_OK) {
			LOG_NO("Giving up, SmfUpgradeStep modify IMM failed, rc=%s", saf_error(rc));
			return rc;
		} else if (rc == SA_AIS_OK) { /* Things went fine */

                	if ((rc = rollbackCcb.execute()) != SA_AIS_OK) {
				LOG_NO("SmfUpgradeStep failed to store rollback CCB, rc=%s", saf_error(rc));
				return rc;
                	}
			break;
		}
	} /* End while (1) */
        } else {
                TRACE("Nothing to modify in information model");
	}

	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// reverseInformationModel()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::reverseInformationModel()
{
        SaAisErrorT rc;
        std::string modifyRollbackCcbDn;
        modifyRollbackCcbDn = "smfRollbackElement=ModifyCcb,";
        modifyRollbackCcbDn += this->getDn();

        SmfRollbackCcb rollbackCcb(modifyRollbackCcbDn,
                                   getProcedure()->getProcThread()->getImmHandle());

        if ((rc = rollbackCcb.rollback()) != SA_AIS_OK) {
                LOG_NO("SmfUpgradeStep failed to rollback Modify CCB, rc=%s", saf_error(rc));
                return rc;
        }

	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// setMaintenanceStateActUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::setMaintenanceStateActUnits()
{
        return setMaintenanceState(m_activationUnit);
}

//------------------------------------------------------------------------------
// setMaintenanceStateDeactUnits()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::setMaintenanceStateDeactUnits()
{
        return setMaintenanceState(m_deactivationUnit);
}

//------------------------------------------------------------------------------
// setMaintenanceState()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::setMaintenanceState(SmfActivationUnit& i_units)
{
        TRACE_ENTER();

        bool rc = true;
        SaAisErrorT result = SA_AIS_OK;
	SmfImmUtils immUtil;
        std::list < std::string > suList;
        std::list < std::string >::iterator it;
        std::list < unitNameAndState >::iterator unitIt;

        for (unitIt = i_units.m_actedOn.begin(); unitIt != i_units.m_actedOn.end(); ++unitIt) {
        	if ((*unitIt).name.find("safAmfNode") == 0) {
                        //If DN is a node, set saAmfSUMaintenanceCampaign for all SUs on the node
        		// Find all SU's on the node
        		std::list < std::string > objectList;
                SaImmAttrValuesT_2 **attributes;
                (void)immUtil.getChildren("", objectList, SA_IMM_SUBTREE, "SaAmfSU");

                        std::list < std::string >::const_iterator suit;
			for (suit = objectList.begin(); suit != objectList.end(); ++suit) {
				if (immUtil.getObject((*suit), &attributes) == true) {

                                        // Check if the found SU is hosted by the activation unit (node)
                                        const SaNameT *hostedByNode =
                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                    "saAmfSUHostedByNode",
                                                                    0);
                                        if ((hostedByNode != NULL)
                                            && (strcmp((*unitIt).name.c_str(), osaf_extended_name_borrow(hostedByNode)) == 0)) {
                                                /* The SU is hosted by the AU node */
                                                suList.push_back(*suit);
                                        }
				}
			}
                } else if ((*unitIt).name.find("safSu") == 0) {
                        //If DN is a SU, set saAmfSUMaintenanceCampaign for this SU only
                        suList.push_back((*unitIt).name);
                } else if ((*unitIt).name.find("safComp") == 0) {
                        //IF DN is a component, set saAmfSUMaintenanceCampaign for the hosting SU
                        //Extract the SU name from the DN
			std::string::size_type pos = (*unitIt).name.find(",");
			if (pos == std::string::npos) {
				LOG_NO("SmfUpgradeStep::setMaintenanceState(): Separator \",\" not found in %s", (*unitIt).name.c_str());
				TRACE_LEAVE();
				return false;
			}
                        std::string su = ((*unitIt).name.substr(pos + 1, std::string::npos));
                        suList.push_back(su);
                } else {
                        LOG_NO("SmfUpgradeStep::setMaintenanceState(): unknown activation unit type %s", (*unitIt).name.c_str());
                        TRACE_LEAVE();
                        return false;
                }
        }

        //Get the campaign DN
        const std::string campDn = SmfCampaignThread::instance()->campaign()->getDn();
        std::list < SmfImmOperation * > operations;

	//Make unique to avoid duplicate SUs.
	suList.sort();
	suList.unique();

	//AMF will only accept the saAmfSUMaintenanceCampaign attribute to be set once
	//for an SU. Since the same SU could be addressed multiple times a check must
	//be made if the attribute is already set.
        //For all SUs, set saAmfSUMaintenanceCampaign attribute (if not already set)
	SaImmAttrValuesT_2 **attributes;
        for (it = suList.begin(); it != suList.end(); ++it) {
		//Read the attribute
		if (immUtil.getObject((*it), &attributes) == false) {
			LOG_NO("SmfUpgradeStep::setMaintenanceState():failed to get imm object %s", (*it).c_str());
			rc = false;
			goto exit;
		}
                const SaNameT *saAmfSUMaintenanceCampaign = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
										"saAmfSUMaintenanceCampaign",
										0);
		//If a value is set, this shall be the current campaign DN
		if(saAmfSUMaintenanceCampaign != NULL) {
			if (strcmp(osaf_extended_name_borrow(saAmfSUMaintenanceCampaign), campDn.c_str()) != 0) { //Exist, but no match
                                   LOG_NO("saAmfSUMaintenanceCampaign already set to unknown campaign dn = %s", osaf_extended_name_borrow(saAmfSUMaintenanceCampaign));
                                   rc = false;
                                   goto exit;
                           }
		} else {
			SmfImmModifyOperation *modop = new (std::nothrow) SmfImmModifyOperation;
			osafassert(modop != 0);
			modop->setDn(*it);
			modop->setOp("SA_IMM_ATTR_VALUES_REPLACE");
			SmfImmAttribute saAmfSUMaintenanceCampaign;
			saAmfSUMaintenanceCampaign.setName("saAmfSUMaintenanceCampaign");
			saAmfSUMaintenanceCampaign.setType("SA_IMM_ATTR_SANAMET");
			saAmfSUMaintenanceCampaign.addValue(campDn);
			modop->addValue(saAmfSUMaintenanceCampaign);
			operations.push_back(modop);
		}
        }

        if ((result = immUtil.doImmOperations(operations)) != SA_AIS_OK) {
                LOG_NO("Fails to set saAmfSUMaintenanceCampaign, rc=%s", saf_error(result));
                rc = false;
                goto exit;
        }

exit:
        //Delete the created SmfImmModifyOperation instances
        std::list < SmfImmOperation * > ::iterator operIter;
        for (operIter = operations.begin(); operIter != operations.end(); ++operIter) {
                delete (*operIter);
        }

        //Note:All saAmfSUMaintenanceCampaign settings shall be removed in the campaign wrap-up.

        TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// createSaAmfNodeSwBundlesNew()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::createSaAmfNodeSwBundlesNew()
{
	TRACE_ENTER();
	std::list < SmfBundleRef >::const_iterator bundleit;
	for (bundleit = m_swAddList.begin(); bundleit != m_swAddList.end(); ++bundleit) {
		if (getSwNode().length() > 0) {
			if (createOneSaAmfNodeSwBundle(getSwNode(), *bundleit) == false) {
				TRACE_LEAVE();
				return false;
			}
		} else {
			std::list<std::string> swNodeList;
			if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
				TRACE_LEAVE();
				return false;
			}
			std::list<std::string>::const_iterator n;
			for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
				if (createOneSaAmfNodeSwBundle(*n, *bundleit) == false) {
					TRACE_LEAVE();
					return false;
				}
			}
		}
	}

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// deleteSaAmfNodeSwBundleNew()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::deleteSaAmfNodeSwBundlesNew()
{
	TRACE_ENTER();
	std::list < SmfBundleRef >::const_iterator bundleit;
 
	for (bundleit = m_swAddList.begin(); bundleit != m_swAddList.end(); ++bundleit) {
		if (getSwNode().length() > 0) {
			if (deleteOneSaAmfNodeSwBundle(getSwNode(), *bundleit) == false) {
				TRACE_LEAVE();
				return false;
			}
		} else {
			std::list<std::string> swNodeList;
			if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
				TRACE_LEAVE();
				return false;
			}
			std::list<std::string>::const_iterator n;
			for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
				if (deleteOneSaAmfNodeSwBundle(*n, *bundleit) == false) {
					TRACE_LEAVE();
					return false;
				}
			}
                }
        }

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// createOneSaAmfNodeSwBundle()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::createOneSaAmfNodeSwBundle(const std::string& i_node,
                                           const SmfBundleRef& i_bundle)
{
	TRACE_ENTER();
        SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	//Check if object alredy exist
	std::string escapedDn = replaceAllCopy(i_bundle.getBundleDn(), ",", "\\,");
	std::string object = "safInstalledSwBundle=" + escapedDn + "," + i_node;
	if (immUtil.getObject(object, &attributes)) {
		// Object already exists
		TRACE_LEAVE();
		return true;
	}

	TRACE("SaAmfNodeSwBundle %s does not exist in Imm, create it", object.c_str());
	SmfImmCreateOperation icoSaAmfNodeSwBundle;

	icoSaAmfNodeSwBundle.setClassName("SaAmfNodeSwBundle");
	icoSaAmfNodeSwBundle.setParentDn(i_node);

	SmfImmAttribute attrsafInstalledSwBundle;
	attrsafInstalledSwBundle.setName("safInstalledSwBundle");
	attrsafInstalledSwBundle.setType("SA_IMM_ATTR_SANAMET");
	attrsafInstalledSwBundle.addValue("safInstalledSwBundle=" + escapedDn);
	icoSaAmfNodeSwBundle.addValue(attrsafInstalledSwBundle);

	SmfImmAttribute attrsaAmfNodeSwBundlePathPrefix;
	attrsaAmfNodeSwBundlePathPrefix.setName("saAmfNodeSwBundlePathPrefix");
	attrsaAmfNodeSwBundlePathPrefix.setType("SA_IMM_ATTR_SASTRINGT");
	attrsaAmfNodeSwBundlePathPrefix.addValue(i_bundle.getPathNamePrefix());
	icoSaAmfNodeSwBundle.addValue(attrsaAmfNodeSwBundlePathPrefix);

	std::list < SmfImmOperation * > immOperations;
	immOperations.push_back(&icoSaAmfNodeSwBundle);
	if (immUtil.doImmOperations(immOperations) != SA_AIS_OK) {
		TRACE_LEAVE();
		return false;
	}

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// deleteOneSaAmfNodeSwBundle()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::deleteOneSaAmfNodeSwBundle(const std::string & i_node,
                                           const SmfBundleRef& i_bundle)
{
	TRACE_ENTER();

        SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	std::string bundleDn = i_bundle.getBundleDn();

	//Check if object alredy exist
	std::string escapedDn = replaceAllCopy(bundleDn, ",", "\\,");
	std::string object = "safInstalledSwBundle=" + escapedDn + "," + i_node;
	if (immUtil.getObject(object, &attributes) == true) {
		TRACE("SaAmfNodeSwBundle %s exist in Imm, delete it", object.c_str());
		SmfImmDeleteOperation idoSaAmfNodeSwBundle;

		idoSaAmfNodeSwBundle.setDn(object);
		std::list < SmfImmOperation * > immOperations;
		immOperations.push_back(&idoSaAmfNodeSwBundle);

		TRACE("immUtil.doImmOperations");
		if (immUtil.doImmOperations(immOperations) != SA_AIS_OK) {
			TRACE_LEAVE();
			return false;
		}
	}

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// deleteSaAmfNodeSwBundleOld()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::deleteSaAmfNodeSwBundlesOld()
{
	TRACE_ENTER();
	std::list < SmfBundleRef >::const_iterator bundleit;
 
	for (bundleit = m_swRemoveList.begin(); bundleit != m_swRemoveList.end(); ++bundleit) {
		if (getSwNode().length() > 0) {
			if (deleteOneSaAmfNodeSwBundle(getSwNode(), *bundleit) == false) {
				TRACE_LEAVE();
				return false;
			}
		} else {
			std::list<std::string> swNodeList;
			if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
				TRACE_LEAVE();
				return false;
			}
			std::list<std::string>::const_iterator n;
			for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
				if (deleteOneSaAmfNodeSwBundle(*n, *bundleit) == false) {
					TRACE_LEAVE();
					return false;
				}
			}
                }
        }

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// createSaAmfNodeSwBundlesOld()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::createSaAmfNodeSwBundlesOld()
{
	TRACE_ENTER();
	std::list < SmfBundleRef >::const_iterator bundleit;
	for (bundleit = m_swRemoveList.begin(); bundleit != m_swRemoveList.end(); ++bundleit) {
		if (getSwNode().length() > 0) {
			if (createOneSaAmfNodeSwBundle(getSwNode(), *bundleit) == false) {
				TRACE_LEAVE();
				return false;
			}
		} else {
			std::list<std::string> swNodeList;
			if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
				TRACE_LEAVE();
				return false;
			}
			std::list<std::string>::const_iterator n;
			for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
				if (createOneSaAmfNodeSwBundle(*n, *bundleit) == false) {
					TRACE_LEAVE();
					return false;
				}
			}
		}
	}

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// calculateSingleStepNodes()
//------------------------------------------------------------------------------
bool SmfUpgradeStep::calculateSingleStepNodes(
	const std::list<SmfPlmExecEnv> & i_plmExecEnvList,
	std::list<std::string>& o_nodelist)
{
	TRACE_ENTER();
	//If the i_plmExecEnvList is empty, the calculated list in  m_swNodeList shall be used.
	//If at least one <plmExecEnv> is specified i.e. the i_plmExecEnvList is not empty, this 
	//list shall override the calculated node list.

	if (i_plmExecEnvList.empty()) {
		TRACE("No <plmExecEnv> was specified, use  m_swNodeList");
                std::list<std::string>::iterator it;
		for (it = m_swNodeList.begin(); it != m_swNodeList.end(); it++) {
                        o_nodelist.push_back(*it);
                }
	} else {
		TRACE("<plmExecEnv> was specified, get the AMF nodes from the provided plmExecEnvList");
		std::list<SmfPlmExecEnv>::const_iterator ee;
		for (ee = i_plmExecEnvList.begin(); ee != i_plmExecEnvList.end(); ee++) {
			std::string const& amfnode = ee->getAmfNode();
			if (amfnode.length() == 0) {
				LOG_NO("Only AmfNodes can be handled in plmExecEnv");
				TRACE_LEAVE();
				return false;
			}
			o_nodelist.push_back(amfnode);
		}
	}

	//The node list may contain duplicates
	o_nodelist.sort();
	o_nodelist.unique();
	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// setSwitchOver()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setSwitchOver(bool i_switchover)
{
        m_switchOver = i_switchover;
}

//------------------------------------------------------------------------------
// getSwitchOver()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::getSwitchOver()
{
        return m_switchOver;
}

//------------------------------------------------------------------------------
// calculateStepType()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::calculateStepType()
{
        SmfImmUtils immutil;
	bool rebootNeeded = false;
	bool activateUsed = false;
	SaImmAttrValuesT_2 ** attributes;

	/* Check if any bundle requires node reboot, in such case only AU = node is accepted */
	TRACE("SmfUpgradeStep::calculateStepType: Check if any software bundle requires reboot");

	if((smfd_cb->nodeBundleActCmd == NULL) || (strcmp(smfd_cb->nodeBundleActCmd,"") == 0)) {
                activateUsed = false;
        }
        else {
                activateUsed = true;
        }
	const std::list < SmfBundleRef > &removeList = this->getSwRemoveList();
	std::list< SmfBundleRef >::const_iterator bundleIter;
	bundleIter = removeList.begin();
	while (bundleIter != removeList.end()) {
		/* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
		if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
			LOG_NO("Could not find remove software bundle  %s", (*bundleIter).getBundleDn().c_str());
			return SA_AIS_ERR_FAILED_OPERATION;
		}
		const SaUint32T* scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
							       "saSmfBundleRemoveOfflineScope",
							       0);

		if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
			TRACE("SmfUpgradeStep::calculateStepType: The SW bundle %s requires reboot to remove", 
			      (*bundleIter).getBundleDn().c_str());

			rebootNeeded = true;
			break;
		}
		bundleIter++;
	}

	//If restart was not needed for installation, also check the bundle removal otherwise not needed
	if (rebootNeeded == false) {
		const std::list < SmfBundleRef > &addList = this->getSwAddList();
		bundleIter = addList.begin();
		while (bundleIter != addList.end()) {
			/* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_NO("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
				return SA_AIS_ERR_FAILED_OPERATION;
			}
			const SaUint32T* scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
								       "saSmfBundleInstallOfflineScope",
								       0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfUpgradeStep::calculateStepType: The SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				rebootNeeded = true;
				break;
			}

			bundleIter++;
		}
	}

        //Find out type of step
	//If single step upgrade, determin the type from first AU or first DU in the step
	//If rolling upgrade check the first AU of the step
	std::string className;
	std::string firstAuDu;
	if (this->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		//Single step
		//Try the activation unit list, if empty try the deactivation unit list
		if (!this->getActivationUnitList().empty()) {
			firstAuDu = this->getActivationUnitList().front().name;
		} else if (!this->getDeactivationUnitList().empty()) {
			firstAuDu = this->getDeactivationUnitList().front().name;
		} else {
			//No activation/deactivation, just SW installation
			className = "SaAmfNode";
		}

	} else { 
		//Rolling
		if (!this->getActivationUnitList().empty()) {
			firstAuDu = this->getActivationUnitList().front().name;
		} else {
			//No activation/deactivation, just SW installation
			className = "SwInstallNode"; //Fake name for SW installation only
		}
	}

	//If a AU/DU was found, check the DN for key words to figure out the class name
	if (!firstAuDu.empty()) {
		if (firstAuDu.find("safComp") == 0) {
			className = "SaAmfComp";
		} else if (firstAuDu.find("safSu") == 0) {
			className = "SaAmfSU";
		} else if (firstAuDu.find("safAmfNode") == 0) {
			className = "SaAmfNode";
		} else {
			LOG_NO("Could not find class for AU/DU DN %s", firstAuDu.c_str());
			return SA_AIS_ERR_FAILED_OPERATION;
		}
	}
	if (className == "SwInstallNode") {
                if (activateUsed == false)
                        this->setStepType(new SmfStepTypeSwInstall(this));
                else
                        this->setStepType(new SmfStepTypeSwInstallAct(this));
	}
	else if (className == "SaAmfNode") {
		/* AU is AMF node */

		if (rebootNeeded) {
			/* -If rolling upgrade: Check if the step will lock/reboot our own node and if so
                           move our campaign execution to the other controller using e.g.
                           admin operation SWAP on the SI we belong to. Then the other
                           controller will continue with this step and do the lock/reboot.

                           -If single step  upgrade: No switchover is needed. A cluster reboot will be
                           ordered within the step */

                        SaAisErrorT rc;
                        bool isSingleNode;
                        if ((rc = this->isSingleNodeSystem(isSingleNode)) != SA_AIS_OK) {
                                LOG_NO("Fail to read if this is a single node system rc=%s", saf_error(rc));
                                return SA_AIS_ERR_FAILED_OPERATION;
                        }

			if (this->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				//In a single node system, treat a rolling upgrade with reboot as single step
				if(isSingleNode == false) { //Normal multi node system
					if (this->isCurrentNode(firstAuDu) == true) {
						this->setSwitchOver(true);
						return SA_AIS_OK;
					}

					if (activateUsed == false)
						this->setStepType(new SmfStepTypeNodeReboot(this));
					else
						this->setStepType(new SmfStepTypeNodeRebootAct(this));

				} else { //Single node system, treat as single step
					LOG_NO("This is a rolling upgrade procedure with reboot in a single node system, treated as single step upgrade");
					if (activateUsed == false)
						this->setStepType(new SmfStepTypeClusterReboot(this));
					else
						this->setStepType(new SmfStepTypeClusterRebootAct(this));
				}
			}
                        else {  // SINGLE STEP
                                //Figure out what kind of step to use.
                                //-If no controllers are configured, cluster reboot step is selected.
                                //-If controllers are not included in step nodes, node reboot step is selected.
                                //-If all controllers (single or dual controller cluster) are included in step nodes,
                                // cluster reboot step is selected.
                                //-If one controller (in dual controller cluster) is included in step nodes:
                                //  *If SMF execute on the controller included, swap controllers
                                //  *Otherwise , node reboot step is selected.

                                //Find out which nodes are affected for each bundle, plmExecEnv overrides calc nodes.
                                //This list is the total set of nodes for the single step procedure
                                //This will be used to calc if all controllers are included and as a list of nodes
                                //to reboot in case of any bundle needs reboot to install/reboot
                                std::list < SmfBundleRef >::const_iterator bundleit;
                                //Find out which nodes was addressed for removal
                                for (bundleit = m_swRemoveList.begin(); bundleit != m_swRemoveList.end(); ++bundleit) {
                                        if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), m_ssAffectedNodeList)) {
                                                LOG_NO("Fail to calculate nodes for bundle [%s]", bundleit->getBundleDn().c_str());
                                        }
                                }
                                //Find out which nodes was addressed for installation
                                for (bundleit = m_swAddList.begin(); bundleit != m_swAddList.end(); ++bundleit) {
                                        if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), m_ssAffectedNodeList)) {
                                                LOG_NO("Fail to calculate nodes for bundle [%s]", bundleit->getBundleDn().c_str());
                                        }
                                }

                                bool allControllersAffected = false; //Assume all controllers is not within the single step list of nodes
                                int noOfAffectedControllers = 0;
                                std::string matchingController;

                                //If a single node system all controllers are always affected
                                //don't care about config attr
                                if (isSingleNode == true) {
                                        LOG_NO("Single node system, always treat as all controllers included");
                                        allControllersAffected = true;
                                        goto selectStepType;
                                }

                                //Read if cluster controllers are defined.
                                //They are defined if the feature to reboot affected nodes only in a
                                //single step procedures is wanted. This is a single step procedure.
                                if (false == readSmfClusterControllers()) {
                                        //In case of failure to read, continue anyway. The only impact is that
                                        //the cluster will be rebooted instead of affected nodes only. The campaign
                                        //shall however not fail because of this.
                                        LOG_NO("Fail to read the id of the cluster controllers, continue. Cluster reboot will be choosen");
                                }

                                if (smfd_cb->smfClusterControllers[0] != NULL) {  //Controller is configured

                                        //Count the number of controllers configured
                                        int noOfConfControllers = 0;
                                        for(int ix = 0; ix <= 1; ix++) {
                                                if (smfd_cb->smfClusterControllers[ix] != NULL) {
                                                        noOfConfControllers++;
                                                }
                                        }

                                        std::string clmNode; //CLM nodes are expected in attribute smfClusterControllers.
                                        // std::list <std::string> nodeList = this->getSwNodeList();
                                        std::list <std::string> nodeList = this->getSsAffectedNodeList();
                                        std::list < std::string >::const_iterator nodeit = nodeList.begin();
                                        std::string smfClusterController;

                                        while ((nodeit != nodeList.end()) && (allControllersAffected == false)) {
                                                //Convert node given for the bundles to install, to CLM node addresses
                                                immutil.nodeToClmNode(*nodeit, clmNode);

                                                //Check if the node is equal to any of the smfClusterControllers configured
                                                for(int ix = 0; ix <= 1; ix++) {
                                                        if (smfd_cb->smfClusterControllers[ix] != NULL) {
                                                                smfClusterController = smfd_cb->smfClusterControllers[ix];
                                                                if(clmNode == smfClusterController) {
                                                                        TRACE("SmfUpgradeStep::calculateStepType:node [%s] is configured as controller", clmNode.c_str());
                                                                        noOfAffectedControllers++;
                                                                        //Save the controller name to later decide if controller switch is needed
                                                                        matchingController = smfClusterController;
                                                                        break;
                                                                }
                                                        }
                                                }

                                                if (noOfAffectedControllers == noOfConfControllers) { //Stop searching if all controllers are found
                                                        allControllersAffected = true;  //This will also break the while loop
                                                        TRACE("SmfUpgradeStep::calculateStepType: all controllers was found within the step list of nodes");
                                                }

                                                ++nodeit;
                                        } //End while
                                } else {
                                        //No smfClusterControllers are configured, use old behavior i.e. cluster reboot
                                        allControllersAffected = true;
                                        LOG_NO("No smfClusterControllers are configured, treat as all controllers included");
                                }

                        selectStepType:
                                //Select the type of step
                                if (allControllersAffected == true ) { //All controlles (dual or single) are within the step list of nodes
                                        LOG_NO("All controllers are included, cluster reboot step type is selected");
                                        if (activateUsed == false)
                                                this->setStepType(new SmfStepTypeClusterReboot(this));
                                        else
                                                this->setStepType(new SmfStepTypeClusterRebootAct(this));

                                } else if (noOfAffectedControllers == 0) { //No controller is within the step list of nodes
                                        LOG_NO("No controller is included, node reboot step type is selected");
                                        if (activateUsed == false)
                                                this->setStepType(new SmfStepTypeNodeReboot(this));
                                        else
                                                this->setStepType(new SmfStepTypeNodeRebootAct(this));

                                } else { //One of two configured controllers is within the step list of nodes
                                        LOG_NO("One of two controllers is included, check if included controller is current node");
                                        //Check if smfd is executing on the controller affected by the step
                                        if (this->isCurrentNode(matchingController) == true) {
                                                LOG_NO("Included controller is current node, switch controller and try again");
                                                this->setSwitchOver(true);
                                                return SA_AIS_OK;
                                        }

                                        LOG_NO("Included controller is not current node, node reboot step type is selected");
                                        if (activateUsed == false)
                                                this->setStepType(new SmfStepTypeNodeReboot(this));
                                        else
                                                this->setStepType(new SmfStepTypeNodeRebootAct(this));
                                }
                        }
		} //End if (rebootNeeded)
                else {
                        if (activateUsed == false)
                                this->setStepType(new SmfStepTypeAuLock(this));
                        else
                                this->setStepType(new SmfStepTypeAuLockAct(this));
		}

	} else if (className == "SaAmfSU") {
		/* AU is SU */
		if (rebootNeeded) {
			LOG_NO("A software bundle requires reboot but the AU is a SU (%s)", firstAuDu.c_str());
			return SA_AIS_ERR_FAILED_OPERATION;
		}
                if (activateUsed == false)
                        this->setStepType(new SmfStepTypeAuLock(this));
                else
                        this->setStepType(new SmfStepTypeAuLockAct(this));
	} else if (className == "SaAmfComp") {
		/* AU is Component */
		if (rebootNeeded) {
			LOG_NO("A software bundle requires reboot but the AU is a Component (%s)", firstAuDu.c_str());
			return SA_AIS_ERR_FAILED_OPERATION;
		}

                if (activateUsed == false)
                        this->setStepType(new SmfStepTypeAuRestart(this));
                else
                        this->setStepType(new SmfStepTypeAuRestartAct(this));
	} else {
		LOG_NO("class name %s for %s unknown as AU", className.c_str(), firstAuDu.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
	}

        return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// calculateStepTypeForMergedSingle()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::calculateStepTypeForMergedSingle()
{
        TRACE_ENTER();
        bool activateUsed;
        if((smfd_cb->nodeBundleActCmd == NULL) || (strcmp(smfd_cb->nodeBundleActCmd,"") == 0)) {
                activateUsed = false;
        }
        else {
                activateUsed = true;
        }

        if (activateUsed == false)
                this->setStepType(new SmfStepTypeClusterReboot(this));
        else
                this->setStepType(new SmfStepTypeClusterRebootAct(this));

        TRACE_LEAVE();
        return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// isCurrentNode()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::isCurrentNode(const std::string & i_nodeDN)
{
        TRACE_ENTER();

        SmfImmUtils immUtil;
        SaImmAttrValuesT_2 **attributes;
        bool rc = false;
        char* tmp_comp_name = getenv("SA_AMF_COMPONENT_NAME");
        if (tmp_comp_name == NULL) {
                LOG_NO("Could not get env variable SA_AMF_COMPONENT_NAME");
                TRACE_LEAVE();
                return false;
        }

        std::string comp_name(tmp_comp_name);
        TRACE("My components name is %s", comp_name.c_str());

        // Find the parent SU to this component and read which node that is hosting the SU
        if (immUtil.getParentObject(comp_name, &attributes) != true) {
                LOG_NO("No hostedByNode attr set for components hosting SU %s", comp_name.c_str());
                TRACE_LEAVE();
                return false;
        }

        const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                          "saAmfSUHostedByNode",
                                                          0);
        if (hostedByNode == NULL) {
                LOG_NO("No hostedByNode attr set for components hosting SU %s", comp_name.c_str());
                rc = false;
                goto done;
        }

        TRACE("SmfUpgradeStep::isCurrentNode:The SU is hosted by AMF node %s", osaf_extended_name_borrow(hostedByNode));

        //Check if input is a CLM or AMF node
        if (i_nodeDN.find("safAmfNode") == 0) {  //AMF node
                if (strcmp(i_nodeDN.c_str(), osaf_extended_name_borrow(hostedByNode)) == 0) {
                        /* The SU is hosted by the node */
                        TRACE("SmfUpgradeStep::isCurrentNode:MATCH, component %s hosted by %s", comp_name.c_str(), i_nodeDN.c_str());
                        rc = true;
                }
        } else if (i_nodeDN.find("safNode") == 0) {  //CLM node
                //Read the CLM node from the AMF node object
                std::string clmMNode;
                if (immUtil.nodeToClmNode(osaf_extended_name_borrow(hostedByNode), clmMNode) != true) {
                        LOG_NO("Conversion of node [%s] to CLM node fail", osaf_extended_name_borrow(hostedByNode));
                        rc = false;
                        goto done;
                }

                if (i_nodeDN == clmMNode) {
                        TRACE("SmfUpgradeStep::isCurrentNode:MATCH, component %s hosted by %s", comp_name.c_str(), i_nodeDN.c_str());
                        rc = true;
                }

        } else {
                LOG_NO("Node of unknown node type [%s]", i_nodeDN.c_str());
                rc = false;
        }

done:
        TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// isSingleNodeSystem()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeStep::isSingleNodeSystem(bool& i_result)
{
	TRACE_ENTER();

	SmfImmUtils immUtil;
	SaAisErrorT rc = SA_AIS_OK;
	std::list < std::string > o_nodeList;

	/*
	  Counting the number of steps in a node level procedure would have been possible
	  But that would not have cover the case of a single node node group. In such situation
	  the number of steps would have been one even if the cluster contain several nodes.
	  Thats the reason the IMM has to be searched.
	*/
        /* Find all SaAmfNode objects in the system */
	if (immUtil.getChildren("", o_nodeList, SA_IMM_SUBTREE, "SaAmfNode") == false) {
		LOG_NO("SmfUpgradeStep::isSingleNodeSystem, Fail to get SaAmfNode instances");
		rc = SA_AIS_ERR_BAD_OPERATION;
	} else {
	
		if (o_nodeList.size() == 1) {
			i_result = true; //Single AMF node
		} else {
			i_result = false; //Multi AMF node
		}
	}

	TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// setSingleStepRebootInfo()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::setSingleStepRebootInfo(int i_rebootInfo)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrValuesT_2 **attributes;
	std::string parent = getDn();
	std::string rdn = "smfSingleStepInfo=info";
	std::string obj = rdn + "," + parent;
	SmfImmUtils immUtil;

	//Check if the object exist
	rc = immUtil.getObjectAisRC(obj, &attributes);
	if (rc == SA_AIS_ERR_NOT_EXIST) {  //If not exist, create the object

		SmfImmRTCreateOperation icoSingleStepInfo;

		icoSingleStepInfo.setClassName("OpenSafSmfSingleStepInfo");
		icoSingleStepInfo.setParentDn(parent);
		icoSingleStepInfo.setImmHandle(getProcedure()->getProcThread()->getImmHandle());

		SmfImmAttribute attrsmfSingleStepInfo;
		attrsmfSingleStepInfo.setName("smfSingleStepInfo");
		attrsmfSingleStepInfo.setType("SA_IMM_ATTR_SASTRINGT");
		attrsmfSingleStepInfo.addValue(rdn);
		icoSingleStepInfo.addValue(attrsmfSingleStepInfo);

		SmfImmAttribute attrsmfRebootType;
		attrsmfRebootType.setName("smfRebootType");
		attrsmfRebootType.setType("SA_IMM_ATTR_SAUINT32T");
		char buf[5];
		snprintf(buf, 4, "%d", i_rebootInfo);
		attrsmfRebootType.addValue(buf);
		icoSingleStepInfo.addValue(attrsmfRebootType);

		rc = icoSingleStepInfo.execute(); //Create the object
		if (rc != SA_AIS_OK){
			LOG_NO("Creation of SingleStepInfo object fails, rc= %s, [dn=%s]", obj.c_str(), saf_error(rc));
		}

	} else { //Update the object
		SmfImmRTUpdateOperation imoSingleStepInfo;
		imoSingleStepInfo.setDn(obj);
		imoSingleStepInfo.setImmHandle(getProcedure()->getProcThread()->getImmHandle());
		imoSingleStepInfo.setOp("SA_IMM_ATTR_VALUES_REPLACE");

		SmfImmAttribute attrsmfRebootType;
		attrsmfRebootType.setName("smfRebootType");
		attrsmfRebootType.setType("SA_IMM_ATTR_SAUINT32T");
		char buf[5];
		snprintf(buf, 4, "%d", i_rebootInfo);
		attrsmfRebootType.addValue(buf);
		imoSingleStepInfo.addValue(attrsmfRebootType);

		rc = imoSingleStepInfo.execute(); //Modify the object
		if (rc != SA_AIS_OK){
			LOG_NO("Modification SingleStepInfo fails, rc=%s, dn=[%s], attr=[smfRebootType], value=[%s]", 
			       saf_error(rc), obj.c_str(), buf);
		}
	}

	TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// getSingleStepRebootInfo()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::getSingleStepRebootInfo(int *o_rebootInfo)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
	std::string parent = getDn();
	std::string rdn = "smfSingleStepInfo=info";
	std::string obj = rdn + "," + parent;

	SaImmAttrValuesT_2 **attributes;

	/* Read the SmfSingleStepInfo object smfRebootType attr */
	SmfImmUtils immUtil;
	rc = immUtil.getObjectAisRC(obj, &attributes);
	if (rc == SA_AIS_OK) {
		const SaUint32T* cnt = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, "smfRebootType", 0);
		*o_rebootInfo = *cnt;
	}

	TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// clusterReboot()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::clusterReboot()
{
	TRACE_ENTER();
	SaAisErrorT result = SA_AIS_OK;
	std::string error;
	std::string s;
	std::stringstream out;
	
	if (smfd_cb->clusterRebootCmd != NULL) {
		std::string clusterBootCmd = smfd_cb->clusterRebootCmd;

		LOG_NO("STEP: Reboot the cluster");
		TRACE("Executing cluster reboot cmd %s", clusterBootCmd.c_str());
		int rc = smf_system(clusterBootCmd.c_str());
		if (rc != 0) {
			error = "CAMP: Cluster reboot command ";
			error += clusterBootCmd;
			error += " failed ";
			out << rc;
			s = out.str();
			error += s;
			LOG_NO("%s", error.c_str());
                        result = SA_AIS_ERR_FAILED_OPERATION;
		}
	} else {
		LOG_NO("STEP: No cluster reboot command found");
                result = SA_AIS_ERR_FAILED_OPERATION;
	}

	TRACE_LEAVE();

        return result;
}

//------------------------------------------------------------------------------
// saveImmContent()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeStep::saveImmContent()
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;

	std::string cmd = smfd_cb->smfImmPersistCmd;

	int result  = smf_system(cmd);
	if (result != 0) {
		TRACE("Fail to save IMM content to file, cmd=[%s], result=[%d]", cmd.c_str(), result);
		rc = SA_AIS_ERR_ACCESS;
	}

	TRACE_LEAVE();
        return rc;
}

//------------------------------------------------------------------------------
// callActivationCmd()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::callActivationCmd()
{
	std::list < SmfBundleRef >::const_iterator bundleit;
	bool result = true;
	SaTimeT timeout = smfd_cb->cliTimeout;              /* Default timeout */
	std::string actCommand = smfd_cb->nodeBundleActCmd; /* The configured activation command */
	SmfImmUtils immUtil;

	TRACE_ENTER();
	//Single step
	if (this->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		// In the single-step upgrade the nodes for
		// the bundle is provided in the m_swNodeList
		// and the PlmExecEnv list. The step itself is
		// not bound to a particular node, so the
		// "i_node" will be empty.

                //Find out which nodes are affected for each bundle, plmExecEnv overrides calc nodes
                std::list < SmfBundleRef >::const_iterator bundleit;
                std::list<std::string> swNodeList;  //The total list of nodes

                //Find out which nodes was addressed for removal
                for (bundleit = m_swRemoveList.begin(); bundleit != m_swRemoveList.end(); ++bundleit) {
                        if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
                                LOG_NO("Fail to calculate nodes for bundle [%s]",
				       bundleit->getBundleDn().c_str());
                        }
                }
                //Find out which nodes was addressed for installation
                for (bundleit = m_swAddList.begin(); bundleit != m_swAddList.end(); ++bundleit) {
                        if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
                                LOG_NO("Fail to calculate nodes for bundle [%s]",
				       bundleit->getBundleDn().c_str());
                        }
                }

		//Start a load thread for each node
		std::list <std::string>::iterator nodeIt;
		for (nodeIt = swNodeList.begin(); nodeIt != swNodeList.end(); ++nodeIt) {
			SmfNodeSwLoadThread *swLoadThread = new SmfNodeSwLoadThread(this, *nodeIt, SMF_ACTIVATE_ALL);
			swLoadThread->start();
		}

		if((result = waitForBundleCmdResult(swNodeList)) == false) {
			LOG_NO("STEP: Fail to execute bundle command");
		}

		TRACE_LEAVE();
		return result;
	}

	//Rolling upgrade
	SmfndNodeDest nodeDest;
	TRACE("Executing  activation command '%s' on node '%s'", 
	      actCommand.c_str(), getSwNode().c_str());
	TRACE("Get node destination for %s", getSwNode().c_str());
	uint32_t rc;

	if (!getNodeDestination(getSwNode(), &nodeDest, NULL)) {
		LOG_NO("no node destination found for node %s", getSwNode().c_str());
		result = false;
		goto done;
	}

	// TODO : how to handle the case where the cmd is an URI
	// First fetch the script using e.g. wget etc

	// Execute the bundle script remote on node
	rc = smfnd_exec_remote_cmd(actCommand.c_str(), &nodeDest, timeout / 10000000, 0);
	// Convert ns to 10 ms cliTimeouttimeout
	if (rc != 0) {
		LOG_NO("executing activation command '%s' on node '%s' failed (%x)", 
		       actCommand.c_str(), getSwNode().c_str(), rc);
		result = false;
		goto done;
	}
done:
	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// callBundleScript()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::callBundleScript(SmfInstallRemoveT i_order,
                                 const std::list < SmfBundleRef > &i_bundleList,
				 const std::string & i_node)
{
	std::list < SmfBundleRef >::const_iterator bundleit;
	std::string command;
	std::string cmdAttr;
	std::string argsAttr;
	SaImmAttrValuesT_2 **attributes;
	const char *cmd = NULL;
	const char *args = NULL;
	bool result = true;
	SaTimeT timeout = smfd_cb->cliTimeout;	/* Default timeout */
	SmfImmUtils immUtil;
	std::list<std::string> swNodeList; //Total node list
	TRACE_ENTER();

	//Single step
	if (this->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		// In the single-step upgrade the nodes for
		// the bundle is provided in the m_swNodeList
		// and the PlmExecEnv list. The step itself is
		// not bound to a particular node, so the
		// "i_node" will be empty.

		//Find total affected nodes
		std::list < SmfBundleRef >::const_iterator bundleit;
		for (bundleit = i_bundleList.begin(); bundleit != i_bundleList.end(); ++bundleit) {
			if (!calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
				result = false;
				TRACE_LEAVE();
				return result;
			}
		}
		//Start a load thread for each node
		std::list <std::string>::iterator nodeIt;
		for (nodeIt = swNodeList.begin(); nodeIt != swNodeList.end(); ++nodeIt) {
			SmfNodeSwLoadThread *swLoadThread = new SmfNodeSwLoadThread(this, *nodeIt, i_order, &i_bundleList);
			swLoadThread->start();
		}

		if((result = waitForBundleCmdResult(swNodeList)) == false) {
			LOG_NO("STEP: Fail to execute bundle command");
		}

		TRACE_LEAVE();
		return result;
	}

	//Rolling upgrade
	for (bundleit = i_bundleList.begin(); bundleit != i_bundleList.end(); ++bundleit) {
		/* Get bundle object from IMM */
		if (immUtil.getObject((*bundleit).getBundleDn(), &attributes) == false) {
			return false;
		}

		//Get timeout attribute
		const SaTimeT *defaultTimeout = immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
								    "saSmfBundleDefaultCmdTimeout",
								    0);
		if (defaultTimeout != NULL) {
			timeout = *defaultTimeout;
		}

		std::string curBundleDN = (*bundleit).getBundleDn();
		TRACE("Found bundle %s in Imm", curBundleDN.c_str());

		switch (i_order) {
		case SMF_STEP_OFFLINE_INSTALL:
		{
			/* Find offline install script name and args */
			cmdAttr = "saSmfBundleInstallOfflineCmdUri";
			argsAttr = "saSmfBundleInstallOfflineCmdArgs";
			break;
		}
		case SMF_STEP_ONLINE_INSTALL:
		{
			/* Find online install script name and args */
			cmdAttr = "saSmfBundleInstallOnlineCmdUri";
			argsAttr = "saSmfBundleInstallOnlineCmdArgs";
			break;
		}
		case SMF_STEP_OFFLINE_REMOVE:
		{
			/* Find offline remove script name and args */
			cmdAttr = "saSmfBundleRemoveOfflineCmdUri";
			argsAttr = "saSmfBundleRemoveOfflineCmdArgs";
			break;
		}
		case SMF_STEP_ONLINE_REMOVE:
		{
			/* Find online remove script name and args */
			cmdAttr = "saSmfBundleRemoveOnlineCmdUri";
			argsAttr = "saSmfBundleRemoveOnlineCmdArgs";
			break;
		}
		default:
		{
			LOG_NO("Invalid order [%d]", i_order);
			result = false;
			goto done;
		}
		}

		/* Get cmd attribute */
		cmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
					    attributes, cmdAttr.c_str(), 0);
		if ((cmd == NULL) || (strlen(cmd) == 0)) {
			TRACE("STEP: Attribute %s is NULL or empty in bundle %s",
                              cmdAttr.c_str(), curBundleDN.c_str());
			continue;
		}
		command = cmd;

		/* Get args attribute */
		args = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
					     attributes, argsAttr.c_str(), 0);
		if (args != NULL) {
			command += " ";
			command += args;
		}

		SmfndNodeDest nodeDest;
		TRACE("Executing bundle script '%s' on node '%s'", 
		      command.c_str(), i_node.c_str());
		TRACE("Get node destination for %s", i_node.c_str());

		if (!waitForNodeDestination(i_node, &nodeDest)) {
			LOG_NO("no node destination found for node %s", i_node.c_str());
			result = false;
			goto done;
		}

		/* TODO : how to handle the case where the cmd is an URI */
		/* First fetch the script using e.g. wget etc */

		/* Execute the bundle script remote on node */
		uint32_t rc = smfnd_exec_remote_cmd(command.c_str(), &nodeDest, timeout / 10000000, 0);
		/* convert ns to 10 ms timeout */
		if (rc != 0) {
			LOG_NO("executing command '%s' on node '%s' failed (%x)", 
			       command.c_str(), i_node.c_str(), rc);
			result = false;
			goto done;
		}
	}
 done:

	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// waitForBundleCmdResult()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::waitForBundleCmdResult(std::list<std::string>& i_swNodeList)
{
	TRACE_ENTER();
	SYSF_MBX mbx;
	NCS_SEL_OBJ mbx_fd;
	typedef enum {
		PROC_MBX_FD,
	        PROC_MAX_FD
	} proc_pollfd_t;
	struct pollfd fds[PROC_MAX_FD];
	PROCEDURE_EVT *evt;
	int ret;
	bool rc(true);
	std::list<PROCEDURE_EVT*> evtToResend;

	//Wait for all bundle command threads to finish, then continue...
	mbx = getProcedure()->getProcThread()->getMbx();
	mbx_fd = ncs_ipc_get_sel_obj(&mbx);

	/* Set up all file descriptors to listen to */
	fds[PROC_MBX_FD].fd = mbx_fd.rmv_obj;
	fds[PROC_MBX_FD].events = POLLIN;
	fds[PROC_MBX_FD].revents = 0; //Coverity

	LOG_NO("Waiting for bundle command execution to finish on all affected nodes");
	while (!i_swNodeList.empty()) {
		ret = poll(fds, PROC_MAX_FD, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("Poll failed - %s", strerror(errno));
			break;
		}

		//Process the Mail box events
		if (fds[PROC_MBX_FD].revents & POLLIN) {
			//Dispatch MBX events
			evt = (PROCEDURE_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&mbx, evt);
			if (evt != NULL) {
				switch (evt->type) {
				case PROCEDURE_EVT_SWRESULT:
				{
					if (evt->event.swResult.rc == 0) {
						LOG_NO("Successfully invoked bundle commands for %s",
						       evt->event.swResult.nodeName);
					} else {
						LOG_NO("Fail to invoke bundle commands for node [%s], rc [%d]",
						       evt->event.swResult.nodeName,
						       evt->event.swResult.rc);
						rc = false;
					}

					i_swNodeList.remove(evt->event.swResult.nodeName);
					delete(evt->event.swResult.nodeName);
					delete(evt);
					break;
				}
				default:
				{
					//The only event handled here is PROCEDURE_EVT_SWRESULT.
					//If other event is received, save it and repost as last action.
					LOG_NO("Unhandled event [%d] receiced, save and resend later", evt->type);
					evtToResend.push_back(evt);
				}
				}
			}
		}
	}

	//Repost unhandled saved events to procedure thread
	std::list < PROCEDURE_EVT* >::iterator evtit;
	for (evtit = evtToResend.begin(); evtit != evtToResend.end(); ++evtit) {
		LOG_NO("Requeue unhandled event [%d]", (*evtit)->type);
		getProcedure()->getProcThread()->send(*evtit);
	}

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// callAdminOperation()
//------------------------------------------------------------------------------
bool
SmfUpgradeStep::callAdminOperation(unsigned int i_operation,
                                   const SaImmAdminOperationParamsT_2 ** i_params,
                                   std::list <unitNameAndState> &i_dnList)
{
        //This routine is only used from the upgrade steps
	std::list < unitNameAndState >::iterator dnit;
	SmfImmUtils immUtil;
	bool result = true;

	TRACE_ENTER();

	for (dnit = i_dnList.begin(); dnit != i_dnList.end(); ++dnit) {
                //The initial deactivation unit(DU) admin state is saved in the given DU list (i_dnList).
                //The DU admin state is not read from IMM but is known from the result from adminOp.
                //The saved DU admin state is used in the unlocking phase to avoid the DU to enter a different admin state than it
                //originally have.
                //If the admin state is "empty" the unlocking will proceed regardless of original state.

                //If AU/DU original state was SA_AMF_ADMIN_LOCKED, don't unlock
                if ((i_operation == SA_AMF_ADMIN_UNLOCK) && ((*dnit).initState == SA_AMF_ADMIN_LOCKED)) {
                        LOG_NO("AU/DU [%s] shall remain SA_AMF_ADMIN_LOCKED, continue", (*dnit).name.c_str());
                        continue;
                }

                //If AU/DU original state was SA_AMF_ADMIN_LOCKED_INSTANTIATION don't instantiate or unlock
                if (((i_operation == SA_AMF_ADMIN_UNLOCK_INSTANTIATION) || (i_operation == SA_AMF_ADMIN_UNLOCK)) &&
                    ((*dnit).initState == SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
                        LOG_NO("AU/DU [%s] shall remain SA_AMF_ADMIN_LOCKED_INSTANTIATION, continue", (*dnit).name.c_str());
                        continue;
                }

		SaAisErrorT rc = immUtil.callAdminOperation((*dnit).name, i_operation, i_params, smfd_cb->adminOpTimeout);

		//In case the upgrade step is re-executed the operation may result in the followng result codes
		//depending on the state of the entity
		// SA_AIS_ERR_NO_OP, in case the operation is already performed
		// SA_AIS_ERR_BAD_OPERATION, in case the entity is already in locked-instantiation admin state
		switch (i_operation) {
		case SA_AMF_ADMIN_LOCK:
			if(rc == SA_AIS_ERR_NO_OP) {
				TRACE("Entity %s already in state LOCKED, continue", (*dnit).name.c_str());
                                //Save the state for act/deact pointed out
                               (*dnit).initState = SA_AMF_ADMIN_LOCKED;
			} else if (rc == SA_AIS_ERR_BAD_OPERATION) {
				TRACE("Entity %s already in state LOCKED-INSTANTIATION, continue", (*dnit).name.c_str());
			} else if (rc != SA_AIS_OK) {
				LOG_NO("Failed to call admin operation %u on %s", i_operation, (*dnit).name.c_str());
				result = false;
				goto done;
			}
			break;
		case SA_AMF_ADMIN_UNLOCK:
			if(rc == SA_AIS_ERR_NO_OP) {
				TRACE("Entity %s already in state UNLOCKED, continue", (*dnit).name.c_str());
			} else if (rc != SA_AIS_OK) {
				LOG_NO("Failed to call admin operation %u on %s", i_operation, (*dnit).name.c_str());
				result = false;
				goto done;
			}
			break;
		case SA_AMF_ADMIN_LOCK_INSTANTIATION:
			if(rc == SA_AIS_ERR_NO_OP) {
				TRACE("Entity %s already in state LOCKED-INSTANTIATED, continue", (*dnit).name.c_str());
				//Save the state for act/deact pointed out
				(*dnit).initState = SA_AMF_ADMIN_LOCKED_INSTANTIATION;
			} else if (rc != SA_AIS_OK) {
				LOG_NO("Failed to call admin operation %u on %s", i_operation, (*dnit).name.c_str());
				result = false;
				goto done;
			}
			break;
		case SA_AMF_ADMIN_UNLOCK_INSTANTIATION:
			if(rc == SA_AIS_ERR_NO_OP) {
				TRACE("Entity %s already in state UNLOCKED-INSTANTIATED, continue", (*dnit).name.c_str());
			} else if (rc != SA_AIS_OK) {
				LOG_NO("Failed to call admin operation %u on %s", i_operation, (*dnit).name.c_str());
				result = false;
				goto done;
			}
			break;
		case SA_AMF_ADMIN_RESTART:
			if (rc != SA_AIS_OK) {
				LOG_NO("Failed to call admin operation %u on %s", i_operation, (*dnit).name.c_str());
				result = false;
				goto done;
			}
			break;
		default:
			LOG_NO("Unknown admin operation %u on %s", i_operation, (*dnit).name.c_str());
			result = false;
			goto done;
			break;
		}
	}

done:
	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// copyDuInitStateToAu()
//------------------------------------------------------------------------------
void
SmfUpgradeStep::copyDuInitStateToAu()
{
		TRACE_ENTER();
		if (smfd_cb->smfKeepDuState == 0) { //Classic behavior, unlock at deactivation
			TRACE_LEAVE();
			return;
		}

		std::list < unitNameAndState >::iterator i_du;
		std::list < unitNameAndState >::iterator i_au;
		// For each DN given in m_activationUnit, check if DN is found in m_deactivationUnit.
		// If DN is found copy the DU initial state.
		for (i_au = m_activationUnit.m_actedOn.begin(); i_au != m_activationUnit.m_actedOn.end(); ++i_au) {
			for (i_du = m_deactivationUnit.m_actedOn.begin(); i_du != m_deactivationUnit.m_actedOn.end(); ++i_du) {
				if ((*i_au).name == (*i_du).name) {
						(*i_au).initState = (*i_du).initState;
						break;
				}
			}
		}
		TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// nodeReboot()
//------------------------------------------------------------------------------
bool 
SmfUpgradeStep::nodeReboot()
{
	TRACE_ENTER();

	bool result = true;
	uint32_t cmdrc;
	std::string cmd;        // Command to enter
	int interval;           // Retry interval
	int timeout;            // Connection timeout
	int elapsedTime;       // time spent inside getNodeDestination() 
	int rebootTimeout = smfd_cb->rebootTimeout / 1000000000; //seconds
	int cliTimeout    = rebootTimeout/2 * 100;                // sec to cs (10 ms)
	int localTimeout  = 500;                                  // 500 * 10 ms = 5 seconds
        SmfndNodeDest nodeDest;
        std::list<std::string> nodeList;
	std::list<std::string>::iterator listIt;
        std::list<SmfNodeUpInfo> rebootedNodeList;
        std::list<SmfNodeUpInfo> cmdNodeList;
	std::list<SmfNodeUpInfo>::iterator nodeIt;

        //Copy the step node/nodelist into a local node list
	if (getSwNode().length() == 0) { //Single step procedure
                nodeList = getSsAffectedNodeList();
        } else {                         //Rolling procedure
                nodeList.push_back(getSwNode());
        }

	if (true == nodeList.empty()) {
		LOG_NO("SmfUpgradeStep::nodeReboot: No nodes to reboot.");
		TRACE_LEAVE();
		return result;
	}

	//Order smf node director to reboot the node
	cmd = smfd_cb->smfNodeRebootCmd;

	for (listIt = nodeList.begin(); listIt != nodeList.end(); ++listIt) {
                if (!getNodeDestination(*listIt, &nodeDest, NULL)) {
                        LOG_NO("SmfUpgradeStep::nodeReboot: no node destination found for node %s", (*listIt).c_str());
                        result = false;
                        goto done;
                }

                /* When executing a reboot command on a node the command will never return
                   so we want a short local timeout. Since the smfnd is handling the
                   cli timeout we want that to be much longer so that the reboot command process
                   is not killed by a cli timeout in the smfnd. The reboot will interrupt the
                   command execution anyway so it doesn't nodeReboot()matter that the timeout is really long */
                cmdrc = smfnd_exec_remote_cmd(cmd.c_str(), &nodeDest, cliTimeout, localTimeout);
                if (cmdrc != 0) {
                        LOG_NO("Reboot command [%s] on node [%s] return rc=[%x], continue", cmd.c_str(), (*listIt).c_str(), cmdrc);
                }

                /* Save the nodename and node UP counter for later use */
                SmfNodeUpInfo nodeUpInfo;
                nodeUpInfo.node_name = *listIt;
                nodeUpInfo.nd_up_cntr = nodeDest.nd_up_cntr;
                rebootedNodeList.push_back(nodeUpInfo);
        }

	//The nodes has been rebooted, wait for the nodes to come UP with stepped UP counter
	timeout  = rebootTimeout; //seconds

        // It is possible that the rebootTimeout configured via IMM interface may be less 
        // than the default 10 seconds spent inside getNodeDestination().
        // To handle this case, we overwride the userconfigured rebootTimeout
        // with the default value of 10 seconds
	if (timeout < 10*ONE_SECOND) {
		LOG_NO("User configured SMF_REBOOT_TIMEOUT_ATTR is less than 10 seconds");
		LOG_NO("using a default rebootTimeout of 10 seconds for the nodes to comeup");
		timeout = 10*ONE_SECOND;
	}

	interval = 1*ONE_SECOND;
	LOG_NO("SmfUpgradeStep::nodeReboot: Waiting to get node destination with increased UP counter");

	while (true) {
                for (nodeIt = rebootedNodeList.begin(); nodeIt != rebootedNodeList.end();) {
                        if(getNodeDestination((*nodeIt).node_name, &nodeDest, &elapsedTime)) {
                                /* Check if node UP counter have been stepped */
                                if(nodeDest.nd_up_cntr > (*nodeIt).nd_up_cntr) {
                                        cmdNodeList.push_back(*nodeIt);           //Save rebooted nodes for next step
                                        nodeIt = rebootedNodeList.erase(nodeIt);  //The node have come back
                                }
                        } else {
                                nodeIt++;
                                TRACE("SmfUpgradeStep::nodeReboot: Node not yet rebooted, check again in %d seconds", interval);
                       }
		}
                
                if(true == rebootedNodeList.empty())
                     break;

		// Adjust timeout with the time spent inside getNodeDestination().
                timeout -= elapsedTime; 

                if (timeout <= 0) {
                        LOG_NO("SmfUpgradeStep::nodeReboot: the following nodes has not been correctly rebooted");
                        for (nodeIt = rebootedNodeList.begin(); nodeIt != rebootedNodeList.end(); nodeIt++) {
                                LOG_NO("Node %s", (*nodeIt).node_name.c_str());
                        }
                        result = false;
			goto done;
                }

                struct timespec time = { interval, 0 };
                osaf_nanosleep(&time);

                timeout -= interval;
	}

	//Node is UP, wait for node to accept command "true"
	cmd      = "true";              //Command "true" should be available on all Linux systems
	timeout  = rebootTimeout / 2;   //Use half of the reboot timeout
	interval = 5;
	LOG_NO("SmfUpgradeStep::nodeReboot: Trying command 'true'");

	while (true) {
                for (nodeIt = cmdNodeList.begin(); nodeIt != cmdNodeList.end();) {
                        if(getNodeDestination((*nodeIt).node_name, &nodeDest, NULL)) {
                                if (smfnd_exec_remote_cmd(cmd.c_str(), &nodeDest, cliTimeout, 0) == 0) {
                                        nodeIt = cmdNodeList.erase(nodeIt);  //The node have accepted the command
                                }
                        } else {
                                ++nodeIt;
                        }
                }

                if(true == cmdNodeList.empty())
                      break;

                LOG_NO("SmfUpgradeStep::nodeReboot: All nodes have not yet accepted command 'true', wait %d sec and retry", interval);
                struct timespec time = { interval, 0 };
                osaf_nanosleep(&time);
                if (timeout <= 0) {
                        LOG_NO("SmfUpgradeStep::nodeReboot: the following nodes has not accept command [%s]", cmd.c_str());
                        for (nodeIt = cmdNodeList.begin(); nodeIt != cmdNodeList.end(); nodeIt++) {
                                LOG_NO("Node %s", (*nodeIt).node_name.c_str());
                        }
                        result = false;
			goto done;
                }

		timeout -= interval;
	}

done:
	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfUpgradeStep::execute()
{
	SmfStepResultT stepResult;

	TRACE_ENTER();

        while (1) {
        	stepResult = m_state->execute(this);

                TRACE("Step result after executing %u", stepResult);

                if (stepResult != SMF_STEP_CONTINUE) {
                        break;
                }

                TRACE("Step state after executing %u", m_state->getState());
        }
	TRACE_LEAVE();
	return stepResult;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfUpgradeStep::rollback()
{
	SmfStepResultT stepResult;

	TRACE_ENTER();

        while (1) {
        	stepResult = m_state->rollback(this);

                TRACE("Step result after rollback %u", stepResult);

                if (stepResult != SMF_STEP_CONTINUE) {
                        break;
                }

                TRACE("Step state after rollback %u", m_state->getState());
        }
	TRACE_LEAVE();
	return stepResult;
}

//------------------------------------------------------------------------------
// setProcedure()
//------------------------------------------------------------------------------
void 
SmfUpgradeStep::setProcedure(SmfUpgradeProcedure * i_procedure)
{
	m_procedure = i_procedure;
}

//------------------------------------------------------------------------------
// getProcedure()
//------------------------------------------------------------------------------
SmfUpgradeProcedure * 
SmfUpgradeStep::getProcedure()
{
	return m_procedure;
}

//------------------------------------------------------------------------------
// checkAndInvokeCallback()
//------------------------------------------------------------------------------
bool SmfUpgradeStep::checkAndInvokeCallback (const std::list < SmfCallback * > &callbackList, unsigned int camp_phase) 
{
	std::string stepDn = getDn();
	std::list < SmfCallback * >:: const_iterator cbkiter;
	SaAisErrorT rc = SA_AIS_OK;

	std::vector < SmfUpgradeStep * >::const_iterator iter;
	const std::vector <SmfUpgradeStep *>& procSteps = m_procedure->getProcSteps();

	cbkiter = callbackList.begin();
	while (cbkiter != callbackList.end()) {
		SmfCallback::StepCountT stepCount = (*cbkiter)->getStepCount();

		if (stepCount == SmfCallback::onFirstStep) {
			/* check if this is first step */
			iter = procSteps.begin(); 
			if (iter != procSteps.end()) {
				if (strcmp((*iter)->getDn().c_str(), stepDn.c_str()) == 0) {
					/* This is the first step, so call callback */
					if (camp_phase == SA_SMF_UPGRADE) {
						rc = (*cbkiter)->execute(stepDn);
					}
					else if (camp_phase == SA_SMF_ROLLBACK) {
						rc = (*cbkiter)->rollback(stepDn);
					}
					if (rc == SA_AIS_ERR_FAILED_OPERATION) {
						LOG_NO("SmfCampaignInit callback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
						TRACE_LEAVE();
						return false;
					}
				}
			}
		}
		else if (stepCount == SmfCallback::onLastStep) {
			/* check if this is last step */
			SmfUpgradeStep* lastStep = NULL;
			for (iter = procSteps.begin(); iter != procSteps.end(); iter++) {
				if (iter+1 == procSteps.end()) {
					lastStep = (*iter);
					break;
				}
			}
			if (lastStep == NULL) {
				LOG_NO("SmfUpgradeStep::checkAndInvokeCallback: Last step not found");
				TRACE_LEAVE();
				return false;
			}
			/* lastStep is at the last step in procSteps now */
			/* check if the current step is same as lastStep */
			if (strcmp(lastStep->getDn().c_str(), stepDn.c_str()) == 0) {
				/* This is the last step, so call callback */
				if (camp_phase == SA_SMF_UPGRADE) {
					rc = (*cbkiter)->execute(stepDn);
				}
				else if (camp_phase == SA_SMF_ROLLBACK) {
					rc = (*cbkiter)->rollback(stepDn);
				}
				if (rc == SA_AIS_ERR_FAILED_OPERATION) {
					LOG_NO("SmfCampaignInit callback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
					TRACE_LEAVE();
					return false;
				}
			}
		}
		else if (stepCount == SmfCallback::halfWay) {
			int noOfSteps = 0, halfWay;
			SmfUpgradeStep* halfwayStep= 0;
			for (iter = procSteps.begin(); iter != procSteps.end(); iter++) {
				noOfSteps++;
			}
			halfWay = noOfSteps/2+1;
			noOfSteps = 0;
			for (iter = procSteps.begin(); iter != procSteps.end(); iter++) {
				noOfSteps++;
				if (halfWay == noOfSteps) {
					halfwayStep = (*iter);
					break;
				}
			}

			if (halfwayStep == 0) {
				LOG_NO("SmfUpgradeStep::checkAndInvokeCallback: could not find halfway step");
				TRACE_LEAVE();
				return false;
			}
			/* iter is at halfWay of procSteps, check if current step is same as iter */
			if (strcmp(halfwayStep->getDn().c_str(), stepDn.c_str()) == 0) {
				/* This is the halfWay step, so call callback */
				if (camp_phase == SA_SMF_UPGRADE) {
					rc = (*cbkiter)->execute(stepDn);
				}
				else if (camp_phase == SA_SMF_ROLLBACK) {
					rc = (*cbkiter)->rollback(stepDn);
				}
				if (rc == SA_AIS_ERR_FAILED_OPERATION) {
					LOG_NO("SmfCampaignInit callback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
					TRACE_LEAVE();
					return false;
				}
			}
		}
		else if (stepCount == SmfCallback::onEveryStep) {
			if (camp_phase == SA_SMF_UPGRADE) {
				rc = (*cbkiter)->execute(stepDn);
			}
			else if (camp_phase == SA_SMF_ROLLBACK) {
				rc = (*cbkiter)->rollback(stepDn);
			}
			if (rc == SA_AIS_ERR_FAILED_OPERATION) {
				LOG_NO("SmfCampaignInit callback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
				TRACE_LEAVE();
				return false;
			}
		}
		cbkiter++;
	}
	return true;
}

//------------------------------------------------------------------------------
// readSmfClusterControllers()
//------------------------------------------------------------------------------
bool SmfUpgradeStep::readSmfClusterControllers()
{
        TRACE_ENTER();
        SmfImmUtils immutil;
        SaImmAttrValuesT_2 **attributes;

        //Read the SMF config object
        if (immutil.getObject(SMF_CONFIG_OBJECT_DN, &attributes) == false) {
		LOG_ER("Could not get SMF config object from IMM %s", SMF_CONFIG_OBJECT_DN);
		return false;
	}

        //Read attr smfSSAffectedNodesEnable. If >0 (true) the cluster controller CLM nodes with nodeId 1 and 2
        //shall override the CLM nodes configured in attribute smfClusterControllers.
	const SaUint32T *smfSSAffectedNodesEnable = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
                                                                          SMF_SS_AFFECTED_NODES_ENABLE_ATTR, 0);

        if ((NULL != smfSSAffectedNodesEnable) && (*smfSSAffectedNodesEnable > 0)) {
                //smfSSAffectedNodesEnable is set to "true".
                //This will override nodes set in SMF config class attr. smfClusterControllers
                //Find the CLM nodes with hard coded default node Id
                LOG_NO("smfSSAffectedNodesEnable is [true]. SMF handle nodeId %x and %x as controllers in SS procedures",
                       smfd_cb->smfNodeIdControllers[0], smfd_cb->smfNodeIdControllers[1]);
                SaImmSearchHandleT immSearchHandle;
                SaNameT objectName;

                SaImmAttrNameT attributeNames[] = {
                        (char*)"saClmNodeID",
                        NULL
                };

                int retryCntr = 0;
                int ix = 0;
                bool controllersFound = false;
                while (false == controllersFound) {
                        if (immutil.getChildrenAndAttrBySearchHandle("",
                                                                     immSearchHandle,
                                                                     SA_IMM_SUBTREE,
                                                                     (SaImmAttrNameT*)attributeNames,
                                                                     "SaClmNode") == false) {
                                LOG_NO("getChildrenAndAttrBySearchHandle fail");
                                //If fail, the immSearchHandle is already finalized by getChildrenAndAttrBySearchHandle method
                                TRACE_LEAVE();
                                return false;
                        }

                        //Read all CLM nodes. The important nodes are the controllers. If nodeId empty, just continue (if payload nodes
                        //the id is not important). When all CLM nodes are read, check if two controllers was found, if not retry to read
                        //the CLM nodes.
                        while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
                                const SaUint32T *nodeId = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, "saClmNodeID", 0);
                                if (nodeId == NULL) {
                                        continue;
                                }
                                if ((*nodeId == smfd_cb->smfNodeIdControllers[0]) || (*nodeId == smfd_cb->smfNodeIdControllers[1])) {
                                        smfd_cb->smfClusterControllers[ix] = strdup(osaf_extended_name_borrow(&objectName));
                                        LOG_NO("Cluster controller[%d] = %s",ix ,smfd_cb->smfClusterControllers[ix]);
                                        ix++;
                                        if (ix == 2) {
                                                controllersFound = true;
                                                break;  //Two controllers found, no need to continue reading CLM nodes.
                                        }
                                }
                        }

                        //All CLM node objects was read, check if two controllers was found
                        if (ix < 2) {
                                if (retryCntr >= 10) {  //Retry 10 times
                                        LOG_NO("Two controllers was not found, giving up");
                                        //Free memory, at most one controller could have been found here
                                        free(smfd_cb->smfClusterControllers[0]);
                                        smfd_cb->smfClusterControllers[0] = NULL;
                                        (void) immutil_saImmOmSearchFinalize(immSearchHandle);
                                        TRACE_LEAVE();
                                        return false;
                                }

                                //Free memory, at most one controller could have been found here
                                free(smfd_cb->smfClusterControllers[0]);
                                smfd_cb->smfClusterControllers[0] = NULL;
                                LOG_NO("Two controllers was not found, wait and retry reading the CLM nodeId's");
                                struct timespec sleepTime = { 5, 0 }; //Five seconds
                                osaf_nanosleep(&sleepTime);
                                retryCntr++;
                                ix = 0;
                        }

                        (void) immutil_saImmOmSearchFinalize(immSearchHandle);
               }
        } else {
                LOG_NO("smfSSAffectedNodesEnable is [false], SMF uses smfClusterControllers attr. as controllers in SS procedures");
                //Read new smfClusterControllers values
                char* controller;
                for(int ix = 0; (controller = (char*)immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                           SMF_CLUSTER_CONTROLLERS_ATTR, ix)) != NULL; ix++) {
                        if(ix > 1) {
                                LOG_NO("Maximum of two cluster controllers can be defined, controller [%s] ignored", controller);
                                break;
                        }

                        smfd_cb->smfClusterControllers[ix] = strdup(controller);
                        LOG_NO("Cluster controller[%d] = %s",ix ,controller);
                }
        }

        TRACE_LEAVE();
        return true;
}

/*====================================================================*/
/*  Class SmfNodeSwLoadThread                                               */
/*====================================================================*/

/*====================================================================*/
/*  Static methods                                                    */
/*====================================================================*/

/** 
 * SmfSmfNodeSwLoadThread::main
 * static main for the thread
 */
void
SmfNodeSwLoadThread::main(NCSCONTEXT info)
{
	SmfNodeSwLoadThread *self = (SmfNodeSwLoadThread *) info;
	self->main();
	TRACE("Swap thread exits");
	delete self;
}

/*====================================================================*/
/*  Methods                                                           */
/*====================================================================*/

/** 
 * Constructor
 *
 * This class operates in two main modes: activate and install/remove. When
 * install/remove is used a bundle list must be specified. The reason for not
 * using the bundle list directly from the step is that install and remove
 * operations are switched during rollback.
 */
SmfNodeSwLoadThread::SmfNodeSwLoadThread(SmfUpgradeStep * i_step, std::string i_nodeName, 
					 SmfUpgradeStep::SmfInstallRemoveT i_order, 
					 const std::list<SmfBundleRef> *i_bundleList):
	m_task_hdl(0),
	m_step(i_step),
	m_amfNode(i_nodeName),
        m_order(i_order),
        m_bundleList(i_bundleList)
{
	sem_init(&m_semaphore, 0, 0);
}

/** 
 * Destructor
 */
SmfNodeSwLoadThread::~SmfNodeSwLoadThread()
{
}

/**
 * SmfNodeSwLoadThread::start
 * Start the SmfNodeSwLoadThread.
 */
int
SmfNodeSwLoadThread::start(void)
{
	TRACE_ENTER();
	uint32_t rc;

	/* Create the task */
	int policy = SCHED_OTHER; /*root defaults */
	int prio_val = sched_get_priority_min(policy);
	std::string threadName = "OSAF_SMF_LOAD_THREAD " + m_amfNode;

	if ((rc = m_NCS_TASK_CREATE((NCS_OS_CB) SmfNodeSwLoadThread::main,
				    (NCSCONTEXT) this,
				    (char*)threadName.c_str(),
				    prio_val, policy,
				    m_LOAD_THREAD_STACKSIZE, &m_task_hdl))
	    != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfNodeSwLoadThread::start: TASK_CREATE_FAILED");
		return -1;
	}
	if ((rc =m_NCS_TASK_DETACH(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfNodeSwLoadThread::start: TASK_START_DETACH\n");
		return -1;
	}

	if ((rc = m_NCS_TASK_START(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfNodeSwLoadThread::start: TASK_START_FAILED\n");
		return -1;
	}

	/* Wait for the thread to start */
	while((sem_wait(&m_semaphore) == -1) && (errno == EINTR))
               continue;       /* Restart if interrupted by handler */

	TRACE_LEAVE();
	return 0;
}

/**
 * SmfNodeSwLoadThread::main
 * main for the thread.
 */
void
SmfNodeSwLoadThread::main(void)
{
	std::list < SmfBundleRef >::const_iterator bundleit;
	std::string command;
	std::string cmdAttr;
	std::string argsAttr;
	SaImmAttrValuesT_2 **attributes;
	const char *cmd = NULL;
	const char *args = NULL;
	SaTimeT timeout = smfd_cb->cliTimeout;	/* Default timeout */
	SmfImmUtils immUtil;
	std::list<std::string> swNodeList; //Total node list
	std::string curBundleDN;
	SaTimeT *defaultTimeout;

	TRACE_ENTER();
	sem_post(&m_semaphore);          //Start method waits for thread to start

	TRACE("Bundle command thread for node [%s] started", m_amfNode.c_str());
 	uint32_t rc = 0;

	if (m_order == SmfUpgradeStep::SMF_ACTIVATE_ALL) {
		command = smfd_cb->nodeBundleActCmd;
		swNodeList.push_back(m_amfNode);
		std::list<std::string>::const_iterator n;
		for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
			if( *n != m_amfNode) {
				continue;
			}
			TRACE("Executing bundle activation command '%s' on node '%s'",
			      command.c_str(), m_amfNode.c_str());

			SmfndNodeDest nodeDest;
			if (!waitForNodeDestination(m_amfNode, &nodeDest)) {
				LOG_NO("no node destination found for node [%s]", m_amfNode.c_str());
				goto done;
			}
			rc = smfnd_exec_remote_cmd(command.c_str(), &nodeDest, timeout / 10000000, 0);
			if (rc != 0) {
				LOG_NO("Executing bundle command '%s' on node '%s' failed (%x)",
				       command.c_str(), m_amfNode.c_str(), rc);
				goto done;
			}
			TRACE("Bundle activation command [%s] successfully executed on node [%s]",
			      command.c_str(),
			      m_amfNode.c_str());
		}
		goto done;
	}

	if (m_bundleList == NULL) {
		LOG_ER("Bundle list must be set for installation/removal");
		rc = 1;
		goto done;
	}

	for (bundleit = m_bundleList->begin(); bundleit != m_bundleList->end(); ++bundleit) {
		/* Get bundle object from IMM */
		if (immUtil.getObject((*bundleit).getBundleDn(), &attributes) == false) {
			LOG_NO("Fail to read bundle object for bundle DN [%s]",
			       (*bundleit).getBundleDn().c_str());
			rc = 1;
			goto done;
		}

		// Get timeout attribute
		defaultTimeout = const_cast<SaTimeT*>(immutil_getTimeAttr((const SaImmAttrValuesT_2 **)attributes,
									  "saSmfBundleDefaultCmdTimeout",
									  0));
		if (defaultTimeout != NULL) {
			timeout = *defaultTimeout;
		}

		curBundleDN = (*bundleit).getBundleDn();
		TRACE("Found bundle %s in Imm", curBundleDN.c_str());

		//For the node, call bundle command
		switch (m_order) {
		case SmfUpgradeStep::SMF_STEP_OFFLINE_INSTALL:
		{
			/* Find offline install script name and args */
			cmdAttr = "saSmfBundleInstallOfflineCmdUri";
			argsAttr = "saSmfBundleInstallOfflineCmdArgs";
			break;
		}
		case SmfUpgradeStep::SMF_STEP_ONLINE_INSTALL:
		{
			/* Find online install script name and args */
			cmdAttr = "saSmfBundleInstallOnlineCmdUri";
			argsAttr = "saSmfBundleInstallOnlineCmdArgs";
			break;
		}
		case SmfUpgradeStep::SMF_STEP_OFFLINE_REMOVE:
		{
			/* Find offline remove script name and args */
			cmdAttr = "saSmfBundleRemoveOfflineCmdUri";
			argsAttr = "saSmfBundleRemoveOfflineCmdArgs";
			break;
		}
		case SmfUpgradeStep::SMF_STEP_ONLINE_REMOVE:
		{
			/* Find online remove script name and args */
			cmdAttr = "saSmfBundleRemoveOnlineCmdUri";
			argsAttr = "saSmfBundleRemoveOnlineCmdArgs";
			break;
		}
		default:
		{
			LOG_NO("Invalid order [%d]", m_order);
			rc = 1;
			goto done;
		}
		}

		//Get cmd attribute
		cmd = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
					    attributes, cmdAttr.c_str(), 0);
		if ((cmd == NULL) || (strlen(cmd) == 0)) {
			TRACE("STEP: Attribute %s is NULL or empty in bundle %s",
			      cmdAttr.c_str(), curBundleDN.c_str());
			continue;
		}
		command = cmd;

		//Get args attribute
		args = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
					     attributes, argsAttr.c_str(), 0);
		if (args != NULL) {
			command += " ";
			command += args;
		}

		swNodeList.clear();
		if (!m_step->calculateSingleStepNodes(bundleit->getPlmExecEnvList(), swNodeList)) {
			rc = 1;
			goto done;
		}

		std::list<std::string>::const_iterator n;
		for (n = swNodeList.begin(); n != swNodeList.end(); n++) {
			if( *n != m_amfNode) {
				continue;
			}
			TRACE("Executing bundle command '%s' on node '%s'",
			      command.c_str(), m_amfNode.c_str());

			SmfndNodeDest nodeDest;
			if (!waitForNodeDestination(m_amfNode, &nodeDest)) {
				LOG_NO("no node destination found for node [%s]", m_amfNode.c_str());
				goto done;
			}
			rc = smfnd_exec_remote_cmd(command.c_str(), &nodeDest, timeout / 10000000, 0);
			if (rc != 0) {
				LOG_NO("Executing bundle command '%s' on node '%s' failed (%x)",
				       command.c_str(), m_amfNode.c_str(), rc);
				goto done;
			}
			TRACE("Bundle command [%s] successfully executed on node [%s]",
			      command.c_str(),
			      m_amfNode.c_str());
		}
	}

done:
	/* Send response to procedure thread */
	PROCEDURE_EVT *evt = new PROCEDURE_EVT();
	evt->type = PROCEDURE_EVT_SWRESULT;
	evt->event.swResult.rc = rc;

	char* node = (char*)calloc(1, m_amfNode.size() + 1);
	memcpy(node, m_amfNode.c_str(), m_amfNode.size());
	evt->event.swResult.nodeName = node;

	m_step->getProcedure()->getProcThread()->send(evt);
	TRACE("Sent PROCEDURE_EVT_SWRESULT for node [%s], rc [%d]",m_amfNode.c_str(), rc);

        TRACE_LEAVE();
        return;
}
