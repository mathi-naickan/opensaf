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
#include <string>
#include <vector>
#include <sstream>

#include <poll.h>
#include <sched.h>

#include <ncssysf_def.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tsk.h>

#include "saAis.h"
#include <saAmf.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>
#include <saf_error.h>
#include "osaf_extended_name.h"
#include "osaf_time.h"

#include "stdio.h"
#include "logtrace.h"
#include "SmfUpgradeCampaign.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfProcedureThread.hh"
#include "SmfCampaignThread.hh"
#include "SmfProcState.hh"
#include "SmfUpgradeStep.hh"
#include "SmfUpgradeMethod.hh"
#include "SmfUpgradeAction.hh"
#include "SmfUtils.hh"
#include "smfd.h"

// This static member variable is the object counter for upgrade procedures
unsigned long SmfUpgradeProcedure::s_procCounter = 1;

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
// Class SmfUpgradeProcedureg71
// Purpose:
// Comments:
//================================================================================

 SmfUpgradeProcedure::SmfUpgradeProcedure():
    m_state(SmfProcStateInitial::instance()),
    m_procState(SA_SMF_PROC_INITIAL), 
    m_procedureThread(NULL), 
    m_name(""),
    m_oiName(""),
    m_time(0),
    m_execLevel(0), 
    m_dn(""),
    m_upgradeMethod(0),
    m_procInitAction(0), 
    m_procWrapupAction(0),
    m_procSteps(0),
    m_beforeLock(0),
    m_beforeTerm(0),
    m_afterImmModify(0),
    m_afterInstantiate(0),
    m_afterUnlock(0),
    m_isMergedProcedure(false)
{
    // create and set the OI name of the procedure
    std::stringstream ss;
    ss << s_procCounter;
    setProcOiName(SMF_PROC_OI_NAME_PREFIX + ss.str());

    // Increment object counter.
    // Note: it is not needed to decrement this counter in the destructor
    //       since the value of this counter is only used for creating OI names.
    s_procCounter++;
}

// ------------------------------------------------------------------------------
// ~SmfUpgradeProcedure()
// ------------------------------------------------------------------------------
SmfUpgradeProcedure::~SmfUpgradeProcedure()
{
	TRACE_ENTER();
	if (m_procedureThread != NULL) {
                m_procedureThread->stop();
		/* The thread deletes it's own object when terminating */
		m_procedureThread = NULL;
	}

	delete m_upgradeMethod;

        //No delete if merged procedure, deletions are made by original procedures
        if (m_isMergedProcedure == false) {
                std::vector < SmfUpgradeAction * >::iterator it;

                /* Delete procedure initialization */
                for (it = m_procInitAction.begin(); it != m_procInitAction.end(); ++it) {
                        delete(*it);
                }

               /* Delete procedure wrapup */
                for (it = m_procWrapupAction.begin(); it != m_procWrapupAction.end(); ++it) {
                        delete(*it);
                }

                /* Delete upgrade steps */
                std::vector < SmfUpgradeStep * >::iterator stepit;
                for (stepit = m_procSteps.begin(); stepit != m_procSteps.end(); ++stepit) {
                        delete(*stepit);
                }
        }

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// init()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::init(const SaImmAttrValuesT_2 ** attrValues)
{
	TRACE_ENTER();
	const SaImmAttrValuesT_2 **attribute;

	for (attribute = attrValues; *attribute != NULL; attribute++) {
		void *value;

		if ((*attribute)->attrValuesNumber != 1) {
			TRACE("invalid number of values %u for %s", (*attribute)->attrValuesNumber,
			       (*attribute)->attrName);
			continue;
		}

		value = (*attribute)->attrValues[0];

		if (strcmp((*attribute)->attrName, "safSmfProcedure") == 0) {
			char *rdn = *((char **)value);
			TRACE("init safSmfProcedure = %s", rdn);
		} else if (strcmp((*attribute)->attrName, "saSmfProcState") == 0) {
			unsigned int state = *((unsigned int *)value);

			if ((state >= SA_SMF_PROC_INITIAL) && (state <= SA_SMF_PROC_ROLLBACK_FAILED)) {
				setProcState((SaSmfProcStateT) state);
			} else {
				LOG_NO("SmfUpgradeProcedure::init: invalid proc state %u", state);
				setProcState(SA_SMF_PROC_INITIAL);
			}
			TRACE("init saSmfProcState = %u", (int)m_procState);
		} else {
			TRACE("init unhandled attribute = %s", (*attribute)->attrName);
		}
	}

	TRACE_LEAVE();
	return SA_AIS_OK;
}

// ------------------------------------------------------------------------------
// changeState()
// ------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::changeState(const SmfProcState * i_state)
{
	TRACE_ENTER();
	//Change state class pointer
	m_state = const_cast < SmfProcState * >(i_state);

	//Set stat in IMM procedure object and send state change notification
	setImmStateAndSendNotification(m_state->getState());

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// getState()
//------------------------------------------------------------------------------
SaSmfProcStateT SmfUpgradeProcedure::getState() const
{
	return m_state->getState();
}

//------------------------------------------------------------------------------
// setProcState()
//------------------------------------------------------------------------------ 
void 
SmfUpgradeProcedure::setProcState(SaSmfProcStateT i_state)
{
	TRACE_ENTER();

	switch (i_state) {
	case SA_SMF_PROC_INITIAL:
		{
			m_state = SmfProcStateInitial::instance();
			break;
		}

	case SA_SMF_PROC_EXECUTING:
		{
			m_state = SmfProcStateExecuting::instance();
			break;
		}

	case SA_SMF_PROC_SUSPENDED:
		{
			m_state = SmfProcStateExecSuspended::instance();
			break;
		}

	case SA_SMF_PROC_COMPLETED:
		{
			m_state = SmfProcStateExecutionCompleted::instance();
			break;
		}

	case SA_SMF_PROC_STEP_UNDONE:
		{
			m_state = SmfProcStateStepUndone::instance();
			break;
		}

	case SA_SMF_PROC_FAILED:
		{
			m_state = SmfProcStateExecFailed::instance();
			break;
		}

	case SA_SMF_PROC_ROLLING_BACK:
		{
			m_state = SmfProcStateRollingBack::instance();
			break;
		}

	case SA_SMF_PROC_ROLLBACK_SUSPENDED:
		{
			m_state = SmfProcStateRollbackSuspended::instance();
			break;
		}

	case SA_SMF_PROC_ROLLED_BACK:
		{
			m_state = SmfProcStateRolledBack::instance();
			break;
		}

	case SA_SMF_PROC_ROLLBACK_FAILED:
		{
			m_state = SmfProcStateRollbackFailed::instance();
			break;
		}
	default:
		{
			LOG_NO("SmfUpgradeProcedure::setProcState: unknown state %d", i_state);
		}
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// setProcName()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setProcName(std::string i_name)
{
	m_name = i_name;
}

//------------------------------------------------------------------------------
// getProcName()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeProcedure::getProcName()
{
	return m_name;
}

//------------------------------------------------------------------------------
// setProcOiName()
//------------------------------------------------------------------------------
void
SmfUpgradeProcedure::setProcOiName(const std::string &i_oiName)
{
	m_oiName = i_oiName;
}

//------------------------------------------------------------------------------
// getProcOiName()
//------------------------------------------------------------------------------
const std::string &
SmfUpgradeProcedure::getProcOiName()
{
	return m_oiName;
}

//------------------------------------------------------------------------------
// setExecLevel()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setExecLevel(std::string i_level)
{
	m_execLevel = atoi(i_level.c_str());
}

//------------------------------------------------------------------------------
// getExecLevel()
//------------------------------------------------------------------------------
const int &
SmfUpgradeProcedure::getExecLevel()
{
	return m_execLevel;
}

//------------------------------------------------------------------------------
// setProcedurePeriod()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setProcedurePeriod(SaTimeT i_time)
{
	m_time = i_time;
}

//------------------------------------------------------------------------------
// getProcedurePeriod()
//------------------------------------------------------------------------------
const SaTimeT &
SmfUpgradeProcedure::getProcedurePeriod()
{
	return m_time;
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// getDn()
//------------------------------------------------------------------------------
const std::string & 
SmfUpgradeProcedure::getDn()
{
	return m_dn;
}

//------------------------------------------------------------------------------
// setImmStateAndSendNotification()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setImmStateAndSendNotification(SaSmfProcStateT i_state)
{
	TRACE_ENTER();

	TRACE("Set procedure state = %u", i_state);
	SaSmfProcStateT oldState = m_procState;
	m_procState = i_state;
	getProcThread()->updateImmAttr(this->getDn().c_str(), (char*)"saSmfProcState", SA_IMM_ATTR_SAUINT32T, &m_procState);
	SmfCampaignThread::instance()->sendStateNotification(m_dn, MINOR_ID_PROCEDURE, SA_NTF_MANAGEMENT_OPERATION,
							     SA_SMF_PROCEDURE_STATE, i_state , oldState);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// setUpgradeMethod()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setUpgradeMethod(SmfUpgradeMethod * i_method)
{
	m_upgradeMethod = i_method;
}

//------------------------------------------------------------------------------
// getUpgradeMethod()
//------------------------------------------------------------------------------
SmfUpgradeMethod *
SmfUpgradeProcedure::getUpgradeMethod(void)
{
	return m_upgradeMethod;
}

//------------------------------------------------------------------------------
// addProcInitAction()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::addProcInitAction(SmfUpgradeAction * i_action)
{
	m_procInitAction.push_back(i_action);
}

//------------------------------------------------------------------------------
// addProcWrapupAction()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::addProcWrapupAction(SmfUpgradeAction * i_action)
{
	m_procWrapupAction.push_back(i_action);
}

//------------------------------------------------------------------------------
// setProcThread()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::setProcThread(SmfProcedureThread * i_procThread)
{
	m_procedureThread = i_procThread;
}

//------------------------------------------------------------------------------
// getProcThread()
//------------------------------------------------------------------------------
SmfProcedureThread *
SmfUpgradeProcedure::getProcThread()
{
	return m_procedureThread;
}

//------------------------------------------------------------------------------
// switchOver()
//------------------------------------------------------------------------------
void
SmfUpgradeProcedure::switchOver()
{
	TRACE_ENTER();
	TRACE("SmfUpgradeProcedure::switchOver: Create the restart indicator");
	SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->createSmfRestartIndicator();

	SmfSwapThread *swapThread = new SmfSwapThread(this);
	TRACE("SmfUpgradeProcedure::switchOver, Starting SI_SWAP thread");
	swapThread->start();

	TRACE_LEAVE();
}
//------------------------------------------------------------------------------
// calculateSteps()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calculateSteps()
{
	TRACE_ENTER();
	SmfUpgradeMethod *upgradeMethod = NULL;
        std::multimap<std::string, objectInst> objects;
        if (!getImmComponentInfo(objects)) {
                LOG_NO("SmfUpgradeProcedure::calculateSteps: Config info from IMM could not be read");
                return false;
        }

	upgradeMethod = getUpgradeMethod();

	if (upgradeMethod == NULL) {
		LOG_NO("SmfUpgradeProcedure::calculateSteps: calculateSteps no upgrade method found");
		return false;
	}

	switch (upgradeMethod->getUpgradeMethod()) {
	case SA_SMF_ROLLING:
		{
			if ( !calculateRollingSteps((SmfRollingUpgrade *)upgradeMethod, objects)) {
                                LOG_NO("SmfUpgradeProcedure::calculateSteps:calculateRollingSteps failed");
                                return false;
                        }
			break;
		}

	case SA_SMF_SINGLE_STEP:
		{
			if ( !calculateSingleStep((SmfSinglestepUpgrade*)upgradeMethod, objects)) {
                                LOG_NO("SmfUpgradeProcedure::calculateSteps:calculateSingleStep failed");
                                return false;
                        }
			break;
		}

	default:
		{
			LOG_NO("SmfUpgradeProcedure::calculateSteps unknown upgrade method found %d", upgradeMethod->getUpgradeMethod());
			return false;
		}
	}

	TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// calculateSteps()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calculateSteps(std::multimap<std::string, objectInst>& i_objects)
{
	TRACE_ENTER();
	SmfUpgradeMethod *upgradeMethod = getUpgradeMethod();

	if (upgradeMethod == NULL) {
		LOG_NO("SmfUpgradeProcedure::calculateSteps: calculateSteps no upgrade method found");
		return false;
	}

	switch (upgradeMethod->getUpgradeMethod()) {
	case SA_SMF_ROLLING:
		{
			if ( !calculateRollingSteps((SmfRollingUpgrade *)upgradeMethod, i_objects)) {
                                LOG_NO("SmfUpgradeProcedure::calculateSteps:calculateRollingSteps failed");
                                return false;
                        }
			break;
		}

	case SA_SMF_SINGLE_STEP:
		{
			if ( !calculateSingleStep((SmfSinglestepUpgrade*)upgradeMethod, i_objects)) {
                                LOG_NO("SmfUpgradeProcedure::calculateSteps:calculateSingleStep failed");
                                return false;
                        }
			break;
		}

	default:
		{
			LOG_NO("SmfUpgradeProcedure::calculateSteps unknown upgrade method found %d", upgradeMethod->getUpgradeMethod());
			return false;
		}
	}

	TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// calculateRollingSteps()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calculateRollingSteps(SmfRollingUpgrade * i_rollingUpgrade,
					   std::multimap<std::string, objectInst> &i_objects)
{
	TRACE_ENTER();
	const SmfByTemplate *byTemplate = (const SmfByTemplate *)i_rollingUpgrade->getUpgradeScope();

	if (byTemplate == NULL) {
		LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: no upgrade scope by template found");
		return false;
	}

	const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();

	if (nodeTemplate == NULL) {
		LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: no node template found");
		return false;
	}

	const std::string & objectDn = nodeTemplate->getObjectDn();
	std::list < std::string > nodeList;

	if ( !calculateNodeList(objectDn, nodeList) ){
		LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: no nodes found");
                return false;
        }

	if (nodeList.size() == 0) {
		LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: no nodes found");
		return false;
	}

	getCallbackList(i_rollingUpgrade);

	const std::list < SmfParentType * >&actUnitTemplates = nodeTemplate->getActivationUnitTemplateList();
	std::list < std::string > activationUnitList;

	if (actUnitTemplates.size() == 0) {
		/* No activation unit templates, use node list as activation/deactivation units */
		std::list < std::string >::const_iterator it;
		int stepCntr = 0;

		for (it = nodeList.begin(); it != nodeList.end(); ++it) {
			int rdnStrSize = 32;
                        char rdnStr[rdnStrSize];
			stepCntr++;
                        snprintf(rdnStr, rdnStrSize, "safSmfStep=%04u", stepCntr);

			SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
                        osafassert(newStep != NULL);
			newStep->setRdn(rdnStr);
			newStep->setDn(newStep->getRdn() + "," + getDn());
			unitNameAndState tmp;
			tmp.name = *it;
                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
			newStep->addActivationUnit(tmp);
			newStep->addDeactivationUnit(tmp);
			newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
			newStep->setRestartOption(i_rollingUpgrade->getStepRestartOption());
			newStep->addSwRemove(nodeTemplate->getSwRemoveList());
			newStep->addSwAdd(nodeTemplate->getSwInstallList());
                        newStep->setSwNode(*it);

			TRACE("SmfUpgradeProcedure::calculateRollingSteps: new step added %s with activation/deactivation unit %s",
			      newStep->getRdn().c_str(), (*it).c_str());

			if ( !addStepModifications(newStep, 
						   byTemplate->getTargetEntityTemplate(), 
						   SMF_AU_AMF_NODE, 
						   i_objects)){
                                LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: addStepModifications failed");
                                return false;
                        }

			addProcStep(newStep);
		}
	} else {
		/* We have activation unit templates, calculate activation/deactivation units */
                std::list < std::string > actDeactUnits;

                //Find out all objects in the system pointed out by the templates
		std::list < SmfParentType * >::const_iterator it;
		for (it = actUnitTemplates.begin(); it != actUnitTemplates.end(); ++it) {
                        if(calcActivationUnitsFromTemplate((*it), nodeList, i_objects, actDeactUnits) != true) {
                                LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: calcActivationUnitsFromTemplate failed");
				return false;
			}
		}

//TODO: The actDeactUnits list is a straight list of singular act/deact units calculated from 
//      the templates so the number of steps may be less since 
//      each step may contain several act/deact units if located in different SGs on the same node,
//       
//      Currently only one act/deact entity is specified for each step.

                TRACE("Create steps for all found act/deac units calculated from templates");
		std::list  < std::string > actDeactNodes; //Nodes pointed out by act/deact units
		int stepCntr = 0;
		std::list < std::string >::const_iterator itActDeact;
                for (itActDeact = actDeactUnits.begin(); itActDeact != actDeactUnits.end(); ++itActDeact) {
			TRACE("Create step %d for %s", stepCntr, (*itActDeact).c_str());
			int rdnStrSize = 32;
                        char rdnStr[rdnStrSize];
			stepCntr++;
                        snprintf(rdnStr, rdnStrSize, "safSmfStep=%04u", stepCntr);

                        SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
                        osafassert(newStep != NULL);
                        newStep->setRdn(rdnStr);
                        newStep->setDn(newStep->getRdn() + "," + getDn());
                        unitNameAndState tmp;
                        tmp.name = *itActDeact;
                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
                        newStep->addActivationUnit(tmp);
                        newStep->addDeactivationUnit(tmp);
                        newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
                        newStep->setRestartOption(i_rollingUpgrade->getStepRestartOption());
                        newStep->addSwRemove(nodeTemplate->getSwRemoveList());
                        newStep->addSwAdd(nodeTemplate->getSwInstallList());
                        std::string node = getNodeForCompSu(*itActDeact, i_objects);
                        if (node.length() == 0) {
                                LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: getNodeForCompSu fails");
                                return false;
                        }
                        newStep->setSwNode(node);
			actDeactNodes.push_back(node);  //Used to detect uncovered nodes below

                        TRACE("New step added %s with activation/deactivation unit %s",
                              newStep->getRdn().c_str(), (*itActDeact).c_str());

			if ( !addStepModifications(newStep, 
						   byTemplate->getTargetEntityTemplate(), 
						   SMF_AU_SU_COMP, 
						   i_objects)){
                                LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: addStepModifications failed");
                                return false;
                        }

                        addProcStep(newStep);
                }

		//Check if there remain nodes in the nodelist which are not pointed out by the steps generated
		//from the act/deact lista above.
		//On such nodes only SW install/remove shall be executed.

		//Match found step nodes with nodelist.
		actDeactNodes.sort();
		actDeactNodes.unique();
		std::list < std::string >::iterator nodeIt;
		for (nodeIt = actDeactNodes.begin(); nodeIt != actDeactNodes.end(); ++nodeIt) {
			nodeList.remove(*nodeIt); //Reduce the nodelist with nodes found in act/deact steps
		}

		//Create SW install steps for the nodes not found in the created steps.
		for (nodeIt = nodeList.begin(); nodeIt != nodeList.end(); ++nodeIt) {
			int rdnStrSize = 32;
                        char rdnStr[rdnStrSize];
			stepCntr++;
                        snprintf(rdnStr, rdnStrSize, "safSmfStep=%04u", stepCntr);

			SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
                        osafassert(newStep != NULL);
			newStep->setRdn(rdnStr);
			newStep->setDn(newStep->getRdn() + "," + getDn());
			newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
			newStep->addSwRemove(nodeTemplate->getSwRemoveList());
			newStep->addSwAdd(nodeTemplate->getSwInstallList());
                        newStep->setSwNode(*nodeIt);

			TRACE("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps new SW install step added %s (with no act/deact unit) for node %s",
			      newStep->getRdn().c_str(), (*nodeIt).c_str());

			/* TODO: Update objects to be modified by step */
			if ( !addStepModifications(newStep, 
						   byTemplate->getTargetEntityTemplate(),
						   SMF_AU_AMF_NODE, 
						   i_objects)){
                                LOG_NO("SmfUpgradeProcedure::calculateRollingSteps: addStepModifications failed");
                                return false;
                        }

			addProcStep(newStep);
		}
	}

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// getNodeForCompSu()
//------------------------------------------------------------------------------
std::string 
SmfUpgradeProcedure::getNodeForCompSu(const std::string & i_objectDn, std::multimap<std::string, objectInst> &i_objects)
{
	TRACE_ENTER();
	if (i_objectDn.find("safSu=") == std::string::npos &&
		i_objectDn.find("safComp=") == std::string::npos) {
		// not a SU or Comp, return an empty string
		TRACE_LEAVE();
		return "";
	}

	std::multimap<std::string, objectInst>::iterator objit;
	for( objit = i_objects.begin(); objit != i_objects.end(); ++objit) { //For all component instances in the system
		if ((i_objectDn == (*objit).second.suDN) || (i_objectDn == (*objit).second.compDN)) {
			return (*objit).second.nodeDN;
		}
	}

	LOG_NO("SmfUpgradeProcedure::getNodeForCompSu: No node found for %s", i_objectDn.c_str());
        TRACE_LEAVE();
	return "";
}

//------------------------------------------------------------------------------
// getCallbackList()
//------------------------------------------------------------------------------
void SmfUpgradeProcedure::getCallbackList(const SmfUpgradeMethod* i_upgradeMethod)
{
	TRACE_ENTER();

	/* Get the list of callbacks from upgrade method and set pointer to Upgrade Procedure */
	std::list <SmfCallback *>::const_iterator cbkiter;
	const std::list <SmfCallback *>& cbklist = const_cast<SmfUpgradeMethod*>(i_upgradeMethod)->getCallbackList();
	cbkiter = cbklist.begin();
	while (cbkiter != cbklist.end()) {
		(*cbkiter)->setProcedure(this);
		TRACE_2("cbk atAction %d, onStep %d, label= %s", (*cbkiter)->getAtAction(), 
			(*cbkiter)->getStepCount(), (*cbkiter)->getCallbackLabel().c_str());
		switch((*cbkiter)->getAtAction()) {
			case SmfCallback::beforeLock:
			{
				/*Add this callback ptr to the beforeLock list */
				m_beforeLock.push_back(*cbkiter);
				break;
			}
			case SmfCallback::beforeTermination:
			{
				m_beforeTerm.push_back(*cbkiter);
				break;
			}
			case SmfCallback::afterImmModification:
			{
				m_afterImmModify.push_back(*cbkiter);
				break;
			}
			case SmfCallback::afterInstantiation:
			{
				m_afterInstantiate.push_back(*cbkiter);
				break;
			}
			case SmfCallback::afterUnlock:
			{
				m_afterUnlock.push_back(*cbkiter);
				break;
			}
			default:
				TRACE_2 ("default case, atAction val = %d", (*cbkiter)->getAtAction());
				break;
		}
		cbkiter++;
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// calculateSingleStep()
//------------------------------------------------------------------------------
bool SmfUpgradeProcedure::calculateSingleStep(SmfSinglestepUpgrade* i_upgrade,
					      std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();
	SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
        osafassert(newStep != NULL);
	const SmfUpgradeScope* scope = i_upgrade->getUpgradeScope();
	const SmfForAddRemove* forAddRemove = 
		dynamic_cast<const SmfForAddRemove*>(scope);

	newStep->setRdn("safSmfStep=0001");
	newStep->setDn(newStep->getRdn() + "," + getDn());
	newStep->setMaxRetry(i_upgrade->getStepMaxRetryCount());
	newStep->setRestartOption(i_upgrade->getStepRestartOption());

	getCallbackList(i_upgrade);

	if (forAddRemove != NULL) {
                //TODO:
                //forAddRemove/removed by Template is not implemented (same as getImmStepsSingleStep())
		TRACE("CalculateSingleStep: SmfForAddRemove");

		/*
		 * Handle the activation-unit.
		 */
		const SmfActivationUnitType* aunit = forAddRemove->getActivationUnit();
		if (aunit == NULL){
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No activation unit in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}
		if (aunit->getRemoved().size() != 0) {
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: Removed not empty in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}
		if (aunit->getSwRemove().size() != 0) {
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: SwRemove not empty in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}

		std::list<std::string> nodeList;
		std::list<std::string> entityList;
		std::list<SmfEntity>::const_iterator e;
		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				std::list<std::string> actUnits;
				if (!calcActivationUnitsFromTemplateSingleStep(*e, i_objects, actUnits, nodeList)) {
					delete newStep;
					TRACE_LEAVE();
					return false;
				}
				std::list<std::string>::const_iterator a;
				for (a = actUnits.begin(); a != actUnits.end(); a++) {
					unitNameAndState tmp;
					tmp.name = *a;
                                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
					newStep->addActivationUnit(tmp);
				}
			} else {
				if (e->getName().length() == 0) {
					LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No DN given in single step actedOn");
					delete newStep;
					TRACE_LEAVE();
					return false;
				}
				//This a single step acted on list without template
				std::string node = getNodeForCompSu(e->getName(), i_objects);
				if (node.length() > 0) nodeList.push_back(node);
				entityList.push_back(e->getName());
			}
		}

		//Make unique to avoid multiple SU in case non restartable components was found and converted to SU's.
		entityList.sort();
		entityList.unique();
		std::list<std::string>::iterator entity;
		for (entity = entityList.begin(); entity != entityList.end(); entity++) {
			unitNameAndState tmp;
			tmp.name = *entity;
                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
			newStep->addActivationUnit(tmp);
		}
		entityList.clear();

		newStep->addSwAdd(aunit->getSwAdd());

		std::list<SmfImmCreateOperation>::const_iterator o;
		for (o = aunit->getAdded().begin(); o != aunit->getAdded().end(); o++) {
			newStep->addImmOperation(new SmfImmCreateOperation(*o));
		}

		/*
		 * Handle the deactivation-unit.
		 */
		aunit = forAddRemove->getDeactivationUnit();
		if (aunit == NULL){
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No deActivation unit in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}
		if (aunit->getAdded().size() != 0) {
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: Added not empty in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}
		if (aunit->getSwAdd().size() != 0) {
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: SwAdd not empty in forAddRemove");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}

		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				std::list<std::string> deactUnits;
				if (!calcActivationUnitsFromTemplateSingleStep(*e, i_objects, deactUnits, nodeList)) {
					delete newStep;
					TRACE_LEAVE();
					return false;
				}
				std::list<std::string>::const_iterator a;
				for (a = deactUnits.begin(); a != deactUnits.end(); a++) {
					unitNameAndState tmp;
					tmp.name = *a;
                                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
					newStep->addDeactivationUnit(tmp);
				}
			} else {
				if (e->getName().length() == 0){
					LOG_NO("ActedOn contain no name");
					delete newStep;
					TRACE_LEAVE();
					return false;
				}

				std::string deactUnit;
				std::string node;
				if(getActDeactUnitsAndNodes(e->getName(), deactUnit, node, i_objects) == false) {
					LOG_NO("SmfUpgradeProcedure::calculateSingleStep: getActDeactUnitsAndNodes failes for DN=%s",e->getName().c_str());
					delete newStep;
					TRACE_LEAVE();
					return false;
				}

				if (node.length() > 0) {
					nodeList.push_back(node);
				}

				entityList.push_back(deactUnit);
			}
		}

		//Make unique to avoid multiple SU in case non restartable components was found and converted to SU's.
		entityList.sort();
		entityList.unique();
		for (entity = entityList.begin(); entity != entityList.end(); entity++) {
			unitNameAndState tmp;
			tmp.name = *entity;
                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
			newStep->addDeactivationUnit(tmp);
		}
		entityList.clear();

		newStep->addSwRemove(aunit->getSwRemove());

		for (e = aunit->getRemoved().begin(); e != aunit->getRemoved().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				LOG_NO("SmfUpgradeProcedure::calculateSingleStep: (forAddRemove/removed) byTemplate not implemented");
                                delete newStep;
				TRACE_LEAVE();
				return false;
			}
			if (e->getName().length() == 0) {
				LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No bundle DN given in single step swRemove");
				delete newStep;
				TRACE_LEAVE();
				return false;
			}
			SmfImmDeleteOperation* deleteop = new SmfImmDeleteOperation;
			deleteop->setDn(e->getName());
			newStep->addImmOperation(deleteop);
		}

		std::list<std::string>::const_iterator n;
		for (n = nodeList.begin(); n != nodeList.end(); n++) {
			newStep->addSwNode(*n);
		}

		addProcStep(newStep);
		TRACE_LEAVE();
		return true;		// <--------- RETURN HERE
	}

	const SmfForModify* forModify = dynamic_cast<const SmfForModify*>(scope);
	if (forModify != NULL) {

		TRACE("calculateSingleStep SmfForModify");

		/*
		 * (The main difference between "forAddRemove" and
		 * "forModify" is that the acivation/deactivation
		 * units are separated in addremove but symmetric in
		 * modify.)
		 */

		bool removeDuplicates = false;
		const SmfActivationUnitType* aunit = forModify->getActivationUnit();
		if (aunit == NULL){
			LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No activation unit in single step forModify");
			delete newStep;
			TRACE_LEAVE();
			return false;
		}
		std::list<std::string> nodeList;
		std::list<SmfEntity>::const_iterator e;
		std::list<std::string> actDeactUnits;
		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				if (!calcActivationUnitsFromTemplateSingleStep(*e, i_objects,  actDeactUnits, nodeList)) {
					delete newStep;
					TRACE_LEAVE();
					return false;
				}
			} else {
				if (e->getName().length() == 0){
					LOG_NO("SmfUpgradeProcedure::calculateSingleStep: No actedOn in single step forModify");
					delete newStep;
					TRACE_LEAVE();
					return false;
				}
				std::string unit;
				std::string node;
				if(getActDeactUnitsAndNodes(e->getName(), unit, node, i_objects) == false) {
					LOG_NO("SmfUpgradeProcedure::calculateSingleStep: getActDeactUnitsAndNodes failes for DN=%s",e->getName().c_str());
					delete newStep;
					TRACE_LEAVE();
					return false;
				}

				removeDuplicates = true;
				actDeactUnits.push_back(unit);
				if (node.length() > 0) {
					nodeList.push_back(node);
				}
			}
		}

		if (removeDuplicates == true) {
			TRACE("Sort the act/deact unit list");
			actDeactUnits.sort();
			actDeactUnits.unique();
		}

		std::list<std::string>::const_iterator a;
		for (a = actDeactUnits.begin(); a != actDeactUnits.end(); a++) {
			unitNameAndState tmp;
			tmp.name = *a;
                        tmp.initState = SA_AMF_ADMIN_UNLOCKED;
			newStep->addDeactivationUnit(tmp);
			newStep->addActivationUnit(tmp);
		}

		std::list<std::string>::const_iterator n;
		for (n = nodeList.begin(); n != nodeList.end(); n++) {
			newStep->addSwNode(*n);
		}
		newStep->addSwAdd(aunit->getSwAdd());
		newStep->addSwRemove(aunit->getSwRemove());

		/*
		 * Handle TargetEntityTemplate's
		 */
		if (!addStepModifications(newStep, 
					  forModify->getTargetEntityTemplate(), 
					  SMF_AU_SU_COMP, 
					  i_objects)) {
			delete newStep;
			TRACE_LEAVE();
			return false;
		}

		addProcStep(newStep);
		TRACE_LEAVE();
		return true;		// <--------- RETURN HERE
	}

	delete newStep;
	LOG_NO("SmfUpgradeProcedure::calculateSingleStep: Unknown upgradeScope");
	TRACE_LEAVE();
        return false;
}

//------------------------------------------------------------------------------
// mergeStepIntoSingleStep()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::mergeStepIntoSingleStep(SmfUpgradeProcedure * i_proc, SmfUpgradeStep *i_newStep)
{
        TRACE_ENTER();
	std::multimap<std::string, objectInst> objInstances;
        SmfUpgradeStep *newStep;

	if (!getImmComponentInfo(objInstances)) {
		LOG_NO("SmfUpgradeProcedure::calculateSteps: Config info from IMM could not be read");
		return false;
	}

        if (i_newStep == 0) {
                newStep = new(std::nothrow) SmfUpgradeStep;
                osafassert(newStep != NULL);
        } else {
                newStep = i_newStep;
        }

	newStep->setRdn("safSmfStep=0001");
	newStep->setDn(newStep->getRdn() + "," + getDn());
	newStep->setMaxRetry(0);
	newStep->setRestartOption(0);

	int initActionId = 1;
	int wrapupActionId = 1;

        std::list < unitNameAndState > forAddRemoveAU;
        std::list < unitNameAndState > forAddRemoveDU;
	std::list < unitNameAndState > tmpDU;
        SmfUpgradeCampaign * camp = SmfCampaignThread::instance()->campaign()->getUpgradeCampaign();
        const std::vector < SmfUpgradeProcedure * >& procedures = camp->getProcedures();
        std::vector < SmfUpgradeProcedure * >::const_iterator proc_iter;
        for (proc_iter = procedures.begin(); proc_iter != procedures.end(); proc_iter++) {
                LOG_NO("Merging [%s] into a single step procedure", (*proc_iter)->getName().c_str());
                //Calculate the steps of the campaign.xml procedures
                LOG_NO("Calculate the procedure steps");
                if( !(*proc_iter)->calculateSteps(objInstances) ) {
                        LOG_NO("SmfProcStateExecuting::executeInit:Step calculation failes");
                        TRACE_LEAVE();
                        return false;
                }

                //Append the information in the new single step with info from the calculated steps above.
                //For all steps in the procedure
                std::vector < SmfUpgradeStep * >::const_iterator step_iter;
                const std::vector < SmfUpgradeStep * >& steps = (*proc_iter)->getProcSteps();
                for (step_iter = steps.begin(); step_iter != steps.end(); step_iter++) {
                        LOG_NO("Step = %s", (*step_iter)->getDn().c_str());
                        LOG_NO("Copy activation/deactivation units");

                        if ((*proc_iter)->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) { //SA_SMF_ROLLING
                                //Add the DU list, AU list will be created later as a copy of DU list.
				tmpDU.insert(tmpDU.end(),
					     (*step_iter)->getDeactivationUnitList().begin(),
					     (*step_iter)->getDeactivationUnitList().end());
                                //Merge (rolling) step bundle ref into the new single step
                                mergeBundleRefRollingToSingleStep(newStep, (*step_iter));
                        } else { //SA_SMF_SINGLE_STEP
                                if (dynamic_cast<const SmfForAddRemove*>((*proc_iter)->getUpgradeMethod()->getUpgradeScope()) != NULL) {
                                        //The scope of the single step is forAddRemove
                                        //Add the AU/DU lists The lists must be added "as is" e.g. a SU unlock must be run
                                        //even if the hosting node is unlocked.

                                        //Copy AU/DU to local temp list
                                        forAddRemoveAU.insert(forAddRemoveAU.end(),
                                                              (*step_iter)->getActivationUnitList().begin(),
                                                              (*step_iter)->getActivationUnitList().end());
                                        forAddRemoveDU.insert(forAddRemoveDU.end(),
                                                              (*step_iter)->getDeactivationUnitList().begin(),
                                                              (*step_iter)->getDeactivationUnitList().end());

                                        //Merge (single for add remove) step bundle ref into the new single step
                                        mergeBundleRefSingleStepToSingleStep(newStep, (*step_iter));
                                } else if (dynamic_cast<const SmfForModify*>((*proc_iter)->getUpgradeMethod()->getUpgradeScope()) != NULL) {
                                        //The scope of the single step is forModify
                                        //Add the DU list, AU list will be created later as a copy of DU list.
                                        //newStep->addDeactivationUnits((*step_iter)->getDeactivationUnitList());
					tmpDU.insert(tmpDU.end(),
						     (*step_iter)->getDeactivationUnitList().begin(),
						     (*step_iter)->getDeactivationUnitList().end());
                                        //Merge (single for modify) step bundle ref into the new single step
                                        mergeBundleRefSingleStepToSingleStep(newStep, (*step_iter));
                                } else {
                                        LOG_NO("SmfUpgradeProcedure::mergeStepIntoSingleStep: Procedure scope not found (forAddRemove/forModify)");
                                        delete newStep;
                                        TRACE_LEAVE();
                                        return false;
                                }
                        }

                        LOG_NO("Add modifications");
                        std::list < SmfImmOperation * >& qq = (*step_iter)->getModifications();
                        TRACE("old step modifications size() = %zu", qq.size());

                        newStep->addModifications((*step_iter)->getModifications());
                        std::list < SmfImmOperation * >& rr = newStep->getModifications();
                        TRACE("newStep merged modifications size() = %zu", rr.size());
                }

		//The init actions
		LOG_NO("Copy the procedure init actions");
		i_proc->addInitActions((*proc_iter)->getInitActions());
		std::vector <SmfUpgradeAction*>::const_iterator actioniter;
		actioniter = i_proc->getInitActions().begin();
		while (actioniter != i_proc->getInitActions().end()) {
			const SmfCallbackAction* cbkAction =
				dynamic_cast<const SmfCallbackAction*>(*actioniter);
			if (cbkAction != NULL) {
				const_cast<SmfCallbackAction*>(cbkAction)->setCallbackProcedure(this);
			}
			//Renumber to action id aviod DN name collision in the merged procedure
			(*actioniter)->changeId(initActionId);
			initActionId++;

			actioniter++;
		}

		//The wrapup actions
		LOG_NO("Copy the procedure wrapup actions");
		i_proc->addWrapupActions((*proc_iter)->getWrapupActions());
		actioniter = i_proc->getWrapupActions().begin();
		while (actioniter != i_proc->getWrapupActions().end()) {
			const SmfCallbackAction* cbkAction =
				dynamic_cast<const SmfCallbackAction*>(*actioniter);
			if (cbkAction != NULL) {
				const_cast<SmfCallbackAction*>(cbkAction)->setCallbackProcedure(this);
			}
			//Renumber to action id aviod DN name collision in the merged procedure
			(*actioniter)->changeId(wrapupActionId);
			wrapupActionId++;

			actioniter++;
		}

		//The step callbacks
		getCallbackList((*proc_iter)->getUpgradeMethod());
        }

        //Remove DU duplicates
        LOG_NO("Remove duplicates from the merged DU list");
        tmpDU.sort(compare_du_part);
        tmpDU.unique(unique_du_part);

        //Reduce the DU list, check if smaller scope is within bigger scope. 
        LOG_NO("Optimize AU/DU");
	std::pair<std::multimap<std::string, objectInst>::iterator, 
		  std::multimap<std::string, objectInst>::iterator> nodeName_mm;
	std::multimap<std::string, objectInst>::iterator iter;

	std::list < unitNameAndState > nodeLevelDU;
        std::list < unitNameAndState >::iterator unit_iter;
	//Find DU on node level and save them in a separate list
        for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
                if ((*unit_iter).name.find("safAmfNode=") == 0) {        //DU is a node node
			nodeLevelDU.push_back(*unit_iter);               //A node will never be optimized away, save it
			unit_iter = tmpDU.erase(unit_iter);              //Remove the node and update iterator
		} else {
			unit_iter++;
		}
	}

	//For all found nodes, look if some other DU (comp/SU) is within scope
	//tmpDU contain all DU except the node level ones which was removed above and saved in nodeLevelDU list
	std::list < unitNameAndState >::iterator node_iter;
        for (node_iter = nodeLevelDU.begin(); node_iter != nodeLevelDU.end(); node_iter++) {
		//For all comp/SU found in the scope of the node.
		//Find out if any remaining DU is within it
		nodeName_mm = objInstances.equal_range((*node_iter).name); //Get all components/SU within the node
		for (iter = nodeName_mm.first;  iter != nodeName_mm.second;  ++iter) {
			//For all comp/SU sound in the scope of the node.
			//Find out if any remaininf DU is within it
			for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
				if ((*unit_iter).name == (*iter).second.suDN) {  //Check SU
					LOG_NO("[%s] is in scope of [%s], remove it from DU list",
					       (*unit_iter).name.c_str(), (*node_iter).name.c_str());
					unit_iter = tmpDU.erase(unit_iter); //Remove the node and update iterator
				} else if ((*unit_iter).name == (*iter).second.compDN) { //Check comp
					LOG_NO("[%s] is in scope of [%s], remove it from DU list",
					       (*unit_iter).name.c_str(), (*node_iter).name.c_str());
					unit_iter = tmpDU.erase(unit_iter); //Remove the node and update iterator
				} else {
					unit_iter++;
				}
			}
		}
	}

	//tmpDU contain all DU which was not in the scope of an included node
	//Find DU on SU level and save them in a separate list. Remove SU from tmpDU list
	std::list < unitNameAndState > suLevelDU;
        for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
                if ((*unit_iter).name.find("safSu=") == 0) {             //DU is a SU
			suLevelDU.push_back(*unit_iter);                 //A node will never be optimized away, save it
			unit_iter = tmpDU.erase(unit_iter);              //Remove the SU and update iterator
		} else {
			unit_iter++;
		}
	}

	//For all SU in the suLevelDU list, look if remaining DU in tmpDU is within scope of the SU
 	std::list < unitNameAndState >::iterator su_iter;
	for (su_iter = suLevelDU.begin(); su_iter != suLevelDU.end(); su_iter++) {
		for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
			if ((*unit_iter).name.find((*su_iter).name) != std::string::npos) {
				//The component was in the scope of the SU
				LOG_NO("[%s] is in scope of [%s], remove it from DU list",
						       (*unit_iter).name.c_str(), (*su_iter).name.c_str());
				unit_iter = tmpDU.erase(unit_iter); //Remove the Component and update iterator
			} else {
				unit_iter++;
			}
		}
	}

        newStep->addDeactivationUnits(nodeLevelDU);  //Add the node level DU
	newStep->addDeactivationUnits(suLevelDU);    //Add the SU level DU
	newStep->addDeactivationUnits(tmpDU);        //Add the comp level DU
	newStep->addActivationUnits(nodeLevelDU);    //Rolling and forModify are symetric, add the node level DU
        newStep->addActivationUnits(suLevelDU);      //Rolling and forModify are symetric, Add the SU level DU
        newStep->addActivationUnits(tmpDU);          //Rolling and forModify are symetric, Add the comp level DU

        //Copy the forAddRemove AU/DU into the lists as is. They must be run as specified in the campaign.
        newStep->addDeactivationUnits(forAddRemoveDU);
        newStep->addActivationUnits(forAddRemoveAU);

        //Add the merged single step to the procedure if allocated in this method
        if (i_newStep == 0) {
                i_proc->addProcStep(newStep);
        }

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// mergeBundleRefSingleStepToSingleStep()
//------------------------------------------------------------------------------
bool
SmfUpgradeProcedure::mergeBundleRefSingleStepToSingleStep(SmfUpgradeStep * io_newStep,
                                                          SmfUpgradeStep * i_oldStep)
{
        //Set the node from the step into bundle plmExecEnvList
        std::list < SmfBundleRef >::iterator oldStepBundleIter;
        std::list < SmfBundleRef >::iterator newStepBundleIter;
        //Add the old steps AMF node to the plmExecEnv of the bundle ref to make it
        //to install on the correct node/nodes.

        LOG_NO("Merge SwAddLists from the single step into a single step bundle list");
        //Read the bundles to add from the old step
        std::list < SmfBundleRef > & bundlesOldStep = i_oldStep->getSwAddList();
        LOG_NO("SwAddLists from old step contain [%zu] elements", bundlesOldStep.size());
        for (oldStepBundleIter = bundlesOldStep.begin(); oldStepBundleIter != bundlesOldStep.end(); oldStepBundleIter++) {
                std::list < SmfBundleRef >::iterator newStepBundleIter;

                //Read the list of already saved bundles in the new step, if already exist only add the
                //swNode's to the existing bundle
                std::list < SmfBundleRef > & bundleNewStep = io_newStep->getSwAddList();
                bool bundle_exists = false;

                //Check if bundle from the old step already exist in the new step
                for (newStepBundleIter = bundleNewStep.begin(); newStepBundleIter != bundleNewStep.end(); newStepBundleIter++) {
                        TRACE("swAdd: (*newStepBundleIter).getBundleDn() = %s, (*oldStepBundleIter).getBundleDn() = %s",
                               (*newStepBundleIter).getBundleDn().c_str(),
                               (*oldStepBundleIter).getBundleDn().c_str());
                        if ((*newStepBundleIter).getBundleDn() == (*oldStepBundleIter).getBundleDn()) {
                                //Bundle already exist in the new single step
                                //-If the bundle to add contain a plmExecEnv list, add that list of nodes to
                                // the existing SmfBundleRef.
                                //-If the bundle to add does not contain a plmExecEnv list, add the step nodes
                                // to the existing SmfBundleRef.
                                const std::list<SmfPlmExecEnv>& tmpPlmExecEnvList = (*oldStepBundleIter).getPlmExecEnvList();
                                if (tmpPlmExecEnvList.size() != 0) {
                                        (*newStepBundleIter).addPlmExecEnvList(tmpPlmExecEnvList);
                                } else {
                                        const std::list<std::string> nodelist = i_oldStep->getSwNodeList();
                                        std::list<std::string>::const_iterator str_iter;
                                        for (str_iter = nodelist.begin(); str_iter != nodelist.end(); str_iter++) {
                                                SmfPlmExecEnv plm;
                                                plm.setAmfNode(const_cast<std::string &>((*str_iter)));
                                                //Copy the new SmfPlmExecEnv into the bundle plvExecEnv list
                                               (*newStepBundleIter).addPlmExecEnv(plm);
                                        }
                                }

                                bundle_exists = true;
                                TRACE("Existing Bundle = %s", (*oldStepBundleIter).getBundleDn().c_str());
                        }
                }

                if (bundle_exists == false) {
                        //New bundle, add the AMF nodes and save in the single step
                        //If the new bundle does not contain a plmExecEnv list, add the step nodes to the plmExecEnv list
                        if ((*oldStepBundleIter).getPlmExecEnvList().size() == 0) {
                                const std::list<std::string> nodelist = i_oldStep->getSwNodeList();
                                std::list<std::string>::const_iterator str_iter;
                                for (str_iter = nodelist.begin(); str_iter != nodelist.end(); str_iter++) {
                                        SmfPlmExecEnv plm;
                                        plm.setAmfNode(const_cast<std::string &>((*str_iter)));
                                        //Copy the new SmfPlmExecEnv into the bundle plvExecEnv list
                                        (*oldStepBundleIter).addPlmExecEnv(plm);
                                }
                        }

                        LOG_NO("New Bundle = %s added", (*oldStepBundleIter).getBundleDn().c_str());
                        io_newStep->addSwAdd((*oldStepBundleIter));
                }
        }

        LOG_NO("Merge SwRemoveLists from the single step into a single step bundle list");
        //Read the bundles to remove from the old step swRemove list
        bundlesOldStep = i_oldStep->getSwRemoveList();
        for (oldStepBundleIter = bundlesOldStep.begin(); oldStepBundleIter != bundlesOldStep.end(); oldStepBundleIter++) {
                std::list < SmfBundleRef >::iterator newStepBundleIter;

                //Read the list of already saved bundles, if already exist add the new
                //swNode's to the existing bundle
                std::list < SmfBundleRef > & bundleNewStep = io_newStep->getSwRemoveList();
                bool bundle_exists = false;

                //Check if bundle from the old step already exist in the new step
                for (newStepBundleIter = bundleNewStep.begin(); newStepBundleIter != bundleNewStep.end(); newStepBundleIter++) {
                        TRACE("swAdd: (*newStepBundleIter).getBundleDn() = %s, (*oldStepBundleIter).getBundleDn() = %s",
                               (*newStepBundleIter).getBundleDn().c_str(),
                               (*oldStepBundleIter).getBundleDn().c_str());
                        if ((*newStepBundleIter).getBundleDn() == (*oldStepBundleIter).getBundleDn()) {
                                //Bundle already exist in the new single step
                                //-If the bundle to add contain a plmExecEnv list, add that list of nodes to
                                // the existing SmfBundleRef.
                                //-If the bundle to add does not contain a plmExecEnv list, add the step nodes
                                // to the existing SmfBundleRef.
                                const std::list<SmfPlmExecEnv>& tmpPlmExecEnvList = (*oldStepBundleIter).getPlmExecEnvList();
                                if (tmpPlmExecEnvList.size() != 0) {
                                        (*newStepBundleIter).addPlmExecEnvList(tmpPlmExecEnvList);
                                } else {
                                        const std::list<std::string> nodelist = i_oldStep->getSwNodeList();
                                        std::list<std::string>::const_iterator str_iter;
                                        for (str_iter = nodelist.begin(); str_iter != nodelist.end(); str_iter++) {
                                                SmfPlmExecEnv plm;
                                                plm.setAmfNode(const_cast<std::string &>((*str_iter)));
                                                //Copy the new SmfPlmExecEnv into the bundle plvExecEnv list
                                                (*newStepBundleIter).addPlmExecEnv(plm);
                                        }
                                }

                                bundle_exists = true;
                                TRACE("Existing Bundle = %s", (*oldStepBundleIter).getBundleDn().c_str());
                        }
                }

                if (bundle_exists == false) {
                        //New bundle, add the AMF nodes and save in the single step
                        //If the new bundle does not contain a plmExecEnv list, add the step nodes to the plmExecEnv list
                        if ((*oldStepBundleIter).getPlmExecEnvList().size() == 0) {
                                const std::list<std::string> nodelist = i_oldStep->getSwNodeList();
                                std::list<std::string>::const_iterator str_iter;
                                for (str_iter = nodelist.begin(); str_iter != nodelist.end(); str_iter++) {
                                        SmfPlmExecEnv plm;
                                        plm.setAmfNode(const_cast<std::string &>((*str_iter)));
                                        //Copy the new SmfPlmExecEnv into the bundle plvExecEnv list
                                        (*oldStepBundleIter).addPlmExecEnv(plm);
                                }
                        }

                        LOG_NO("New Bundle = %s added", (*oldStepBundleIter).getBundleDn().c_str());
                        io_newStep->addSwRemove((*oldStepBundleIter));
                }
        }

        return true;
}

//------------------------------------------------------------------------------
// mergeBundleRefRollingToSingleStep()
//------------------------------------------------------------------------------
bool
SmfUpgradeProcedure::mergeBundleRefRollingToSingleStep(SmfUpgradeStep * io_newStep,
                                                       SmfUpgradeStep * i_oldStep)
{
	TRACE_ENTER();
        //Set the node from the step into bundle plmExecEnvList
        std::list < SmfBundleRef >::iterator oldStepBundleIter;
        std::list < SmfBundleRef >::iterator newStepBundleIter;

        //Add the old steps AMF node to the plmExecEnv of the bundle ref to make it
        //install on the right node/nodes.

        std::list < SmfBundleRef > & bundlesOldStep = i_oldStep->getSwAddList();
        for (oldStepBundleIter = bundlesOldStep.begin(); oldStepBundleIter != bundlesOldStep.end(); oldStepBundleIter++) {
                //Read the list of already saved bundles, if already exist only add the new
                //swNode to the existing bundle
                std::list < SmfBundleRef >::iterator newStepBundleIter;
                std::list < SmfBundleRef > & bundlesNewStep = io_newStep->getSwAddList();
                //Check if bundle already saved
                bool bundle_exists = false;
                for (newStepBundleIter = bundlesNewStep.begin(); newStepBundleIter != bundlesNewStep.end(); newStepBundleIter++) {
                        TRACE("(*newStepBundleIter).getBundleDn() = %s, (*oldStepBundleIter).getBundleDn() = %s",
			      (*newStepBundleIter).getBundleDn().c_str(),
			      (*oldStepBundleIter).getBundleDn().c_str());
                        if ((*newStepBundleIter).getBundleDn() == (*oldStepBundleIter).getBundleDn()) {
                                //Bundle already saved in the single step, just add the AMF the node
                                SmfPlmExecEnv plm;
                                plm.setAmfNode(const_cast<std::string &>(i_oldStep->getSwNode()));
                                (*newStepBundleIter).addPlmExecEnv(plm);
                                bundle_exists = true;
                                TRACE("Existing Bundle = %s, AmfNode = %s", (*oldStepBundleIter).getBundleDn().c_str()  ,plm.getAmfNode().c_str());
                        }
                }

                if (bundle_exists == false) {
                        //New bundle, add the AMF the node and save in the single step
                        SmfPlmExecEnv plm;
                        plm.setAmfNode(const_cast<std::string &>(i_oldStep->getSwNode()));
                        (*oldStepBundleIter).addPlmExecEnv(plm);
                        LOG_NO("New Bundle = %s added, AmfNode = %s", (*oldStepBundleIter).getBundleDn().c_str()  ,plm.getAmfNode().c_str());
                        io_newStep->addSwAdd((*oldStepBundleIter));
                }
        }

        LOG_NO("Merge SwRemoveLists from the rolling steps into a single step bundle list");
        bundlesOldStep = i_oldStep->getSwRemoveList();
        for (oldStepBundleIter = bundlesOldStep.begin(); oldStepBundleIter != bundlesOldStep.end(); oldStepBundleIter++) {
                //Read the list of already saved bundles, if already exist only add the new
                //swNode to the existing bundle
                std::list < SmfBundleRef >::iterator newStepBundleIter;
                std::list < SmfBundleRef > & bundlesNewStep = io_newStep->getSwRemoveList();
                //Check if bundle already saved
                bool bundle_exists = false;
                for (newStepBundleIter = bundlesNewStep.begin(); newStepBundleIter != bundlesNewStep.end(); newStepBundleIter++) {
                        if ((*newStepBundleIter).getBundleDn() == (*oldStepBundleIter).getBundleDn()) {
                                //Bundle already saved in the single step, just add the AMF the node
                                SmfPlmExecEnv plm;
                                plm.setAmfNode(const_cast<std::string &>(i_oldStep->getSwNode()));
                                (*newStepBundleIter).addPlmExecEnv(plm);
                                bundle_exists = true;
                                TRACE("Existing Bundle = %s, AmfNode = %s", (*oldStepBundleIter).getBundleDn().c_str()  ,plm.getAmfNode().c_str());
                        } 
                }

                if (bundle_exists == false) {
                        //New bundle, add the AMF the node and save in the single step
                        SmfPlmExecEnv plm;
                        plm.setAmfNode(const_cast<std::string &>(i_oldStep->getSwNode()));
                        (*oldStepBundleIter).addPlmExecEnv(plm);
                        LOG_NO("New Bundle = %s added, AmfNode = %s", (*oldStepBundleIter).getBundleDn().c_str()  ,plm.getAmfNode().c_str());
                        io_newStep->addSwRemove((*oldStepBundleIter));
                }
        }
	TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// calculateNodeList()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calculateNodeList(const std::string & i_objectDn, std::list < std::string > &o_nodeList)
{
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	const char *className;

	if (immUtil.getObject(i_objectDn, &attributes) == false) {
		LOG_NO("SmfUpgradeProcedure::calculateNodeList: failed to get imm object %s", i_objectDn.c_str());
		return false;
	}

	className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes, SA_IMM_ATTR_CLASS_NAME, 0);
	if (className == NULL) {
		LOG_NO("SmfUpgradeProcedure::calculateNodeList: class name not found for %s", i_objectDn.c_str());
		return false;
	}

	if (strcmp(className, "SaAmfCluster") == 0) {
		/* object DN is of class SaAmfCluster, find all SaAmfNode children */
		(void)immUtil.getChildren(i_objectDn, o_nodeList, SA_IMM_SUBLEVEL, "SaAmfNode");
	} else if (strcmp(className, "SaAmfNodeGroup") == 0) {
		/* object DN is of class SaAmfNodeGroup, get saAmfNGNodeList attribute */
		unsigned int i;

		for (i = 0; attributes[i] != NULL; i++) {
			if (strcmp(attributes[i]->attrName, "saAmfNGNodeList")
			    == 0) {
				unsigned int j;
				for (j = 0; j < attributes[i]->attrValuesNumber; j++) {
					SaNameT *amfNode = (SaNameT *) attributes[i]->attrValues[j];
					std::string amfNodeDn;
					amfNodeDn.append(osaf_extended_name_borrow(amfNode));
					TRACE("calculateNodeList adding amf group node %s to node list",
					      amfNodeDn.c_str());
					o_nodeList.push_back(amfNodeDn);
				}
			}
		}
	} else {
		LOG_NO("SmfUpgradeProcedure::calculateNodeList: class name %s for %s unknown as node template", className, i_objectDn.c_str());
		return false;
	}

        return true;
}

//------------------------------------------------------------------------------
// calcActivationUnitsFromTemplateSingleStep()
//------------------------------------------------------------------------------
bool SmfUpgradeProcedure::calcActivationUnitsFromTemplateSingleStep(
	SmfEntity const& i_entity,
	std::multimap<std::string, objectInst> &i_objects,
	std::list<std::string>& o_actDeactUnits,
	std::list<std::string>& o_nodeList)
{
	SmfParentType parent;
	parent.setParentDn(i_entity.getParent());
	parent.setTypeDn(i_entity.getType());
	std::list<std::string> dummy_nodeList;
	return calcActivationUnitsFromTemplate(
		&parent, dummy_nodeList, i_objects, o_actDeactUnits, &o_nodeList);
}

//------------------------------------------------------------------------------
// calcActivationUnitsFromTemplate()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calcActivationUnitsFromTemplate(SmfParentType * i_parentType, 
                                                     const std::list < std::string >&i_nodeList,
						     std::multimap<std::string, objectInst> &i_objects,
                                                     std::list < std::string > &o_actDeactUnits,
						     std::list<std::string>* o_nodeList)
{
        TRACE_ENTER();
        SmfImmUtils immUtil;
	std::string className;
        SaImmAttrValuesT_2 **attributes;

        if ( (i_parentType->getTypeDn().size() == 0) && (i_parentType->getParentDn().size() == 0))
        {
                LOG_NO("SmfUpgradeProcedure::calcActivationUnitsFromTemplate: No value for parent nor type found in activationUnitTemplate SmfParentType");
                return false;
        }

        if (i_parentType->getTypeDn().size() > 0) { /* Type is set, if parent is set it will just */
                /* narrow the search for SUs or Components   */
                                   
                /* First find out if it is a SU or components version type */
                if (immUtil.getObject(i_parentType->getTypeDn(), &attributes) == false) {
                        LOG_NO("SmfUpgradeProcedure::calcActivationUnitsFromTemplate: failed to get version type imm object %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
                                                  attributes, SA_IMM_ATTR_CLASS_NAME, 0);
                if (className.size() == 0) {
                        LOG_NO("SmfUpgradeProcedure::calcActivationUnitsFromTemplate: class name not found for version type %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                if (className == "SaAmfSUType") {
                        TRACE("Check SU type %s", i_parentType->getTypeDn().c_str());
                        std::list < std::string > foundObjs; //All found objects of type SaAmfSUType in the system
			std::multimap<std::string, objectInst>::iterator objit;
                        //For each found object check the versioned type
                        for (objit = i_objects.begin(); objit != i_objects.end(); ++objit) {
                                TRACE("Check SU %s", (*objit).second.suDN.c_str());
				if ((i_parentType->getParentDn().size() > 0) && (i_parentType->getParentDn() != (*objit).second.sgDN)){
					//The parent is set but it is no parent to this SU
					TRACE("%s is not a parent to SU %s, continue", i_parentType->getParentDn().c_str(), (*objit).second.suDN.c_str());
					continue;
				}

				//For each object check the versioned type
				if (i_parentType->getTypeDn() == (*objit).second.suTypeDN) {
					TRACE("Matching type SU %s hosted by %s", (*objit).second.suDN.c_str(), (*objit).second.nodeDN.c_str());
					if (o_nodeList == NULL) { //No output nodelist given, use the input nodelist to check the scope
                                                        std::list < std::string >::const_iterator it;
                                                        for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {
                                                                if ((*it) ==  (*objit).second.nodeDN) {
                                                                        /* The SU is hosted by the node */
									TRACE("Matching type SU %s is hosted on node within the targetNodeTemplate, add to list", (*objit).second.suDN.c_str());
                                                                        o_actDeactUnits.push_back((*objit).second.suDN);
                                                                        break;
                                                                } 
                                                        }
                                                } else { //Scope (of nodes) unknown, add the hosting node to the output node list
							o_actDeactUnits.push_back((*objit).second.suDN);
							o_nodeList->push_back((*objit).second.nodeDN);
                                                }
				}
                        }

                } else if (className == "SaAmfCompType") {
                        TRACE("Check Comp type %s", i_parentType->getTypeDn().c_str());
			std::multimap<std::string, objectInst>::iterator objit;
                        for (objit = i_objects.begin(); objit != i_objects.end(); ++objit) {
                                TRACE("Check Comp %s", (*objit).second.compDN.c_str());
				if ((i_parentType->getParentDn().size() > 0) && (i_parentType->getParentDn() != (*objit).second.sgDN)){
					//The parent is set but it is no parent to this SU
					TRACE("%s is not a parent to Comp %s, continue", i_parentType->getParentDn().c_str(), (*objit).second.compDN.c_str());
					continue;
				}
				//For each object check the versioned type
				if (i_parentType->getTypeDn() == (*objit).second.compTypeDN) {
					TRACE("Matching type Comp %s hosted by %s", (*objit).second.compDN.c_str(), (*objit).second.nodeDN.c_str());

					if (o_nodeList == NULL) {//No output nodelist given, use the input nodelist to check the scope
						std::list < std::string >::const_iterator it;
						for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {
							if ((*it) == (*objit).second.nodeDN) {
								/* The SU is hosted by the node */
								if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
									std::string comp = (*objit).second.compDN;
									std::string parentDn = comp.substr(comp.find(',') + 1, std::string::npos);
									TRACE("Component %s is hosted on node within the targetNodeTemplate", comp.c_str());
									TRACE("The stepRestartOption was set to false(0), use parent %s, as act/deactComponent", parentDn.c_str());
									o_actDeactUnits.push_back(parentDn);
								} else { // saSmfStepRestartOption is set to true
									TRACE("Component %s is hosted on node within the targetNodeTemplate, add to list", (*objit).second.compDN.c_str());
									//Check if component is restartable
									if (isCompRestartable((*objit).second.compDN) == false) {
										LOG_NO("SmfUpgradeProcedure::calcActivationUnitsFromTemplate: Component %s is not restartable", (*objit).second.compDN.c_str());
										return false;
									}
									o_actDeactUnits.push_back((*objit).second.compDN);
								}

								break;
							}
						}
					} else { //Scope (of nodes) unknown, add the hosting node to the output node list
						if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
							std::string comp = (*objit).second.compDN;
							std::string parentDn = comp.substr(comp.find(',') + 1, std::string::npos);
							TRACE("The stepRestartOption was set to false(0), use parent %s, as act/deactComponent", parentDn.c_str());
							o_actDeactUnits.push_back(parentDn);
						} else { // saSmfStepRestartOption is set to true
							//Check if component is restartable
							if (isCompRestartable((*objit).second.compDN) == false) {
								LOG_NO("SmfUpgradeProcedure::calcActivationUnitsFromTemplate: Component %s is not restartable", (*objit).second.compDN.c_str());
								return false;
							}
							o_actDeactUnits.push_back((*objit).second.compDN);
						}

						o_nodeList->push_back((*objit).second.nodeDN);
					}
				}

                        } //End for (objit = foundObjs.begin(); objit != foundObjs.end(); ++objit)
                }
        } else {
                /* Only parent is set and contain a value */
                TRACE("Find SUs for parent SG %s", i_parentType->getParentDn().c_str());

		//Search the component list for matching SG
		std::multimap<std::string, objectInst>::iterator objit;
		for (objit = i_objects.begin(); objit != i_objects.end(); ++objit) {
			TRACE("Check Comp %s", (*objit).second.compDN.c_str());
			if (i_parentType->getParentDn() == (*objit).second.sgDN) {
				TRACE("SU %s is a child to SG parent %s and hosted by %s", 
				      (*objit).second.suDN.c_str(), 
				      (*objit).second.sgDN.c_str(),
				      (*objit).second.nodeDN.c_str());
                                if (o_nodeList == NULL) {
                                        std::list < std::string >::const_iterator it;
					TRACE("Check if node is within the the targetNodeTemplate");
                                        for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {                                                
						if ((*it) == (*objit).second.nodeDN) {
                                                        /* The SU is hosted by the node */
                                                        TRACE("SU %s is hosted on node %s, within the targetNodeTemplate, add to list", 
							      (*objit).second.suDN.c_str(),
							      (*objit).second.nodeDN.c_str());
                                                        o_actDeactUnits.push_back((*objit).second.suDN);
                                                        break;
                                                }
                                        }
                                } else {
					TRACE("SU %s is added to output actDeactUnits list", (*objit).second.suDN.c_str());
					o_actDeactUnits.push_back((*objit).second.suDN);
					TRACE("Node %s is added to output nodeList list", (*objit).second.nodeDN.c_str());
					o_nodeList->push_back((*objit).second.nodeDN);
                                }
			}
		}
        }

        //Always remove duplicates from o_actDeactUnits and o_nodeList output lists
        o_actDeactUnits.sort();
        o_actDeactUnits.unique();

        if (o_nodeList != NULL) {
                o_nodeList->sort();
                o_nodeList->unique();
        }

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// addProcStep()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::addProcStep(SmfUpgradeStep * i_step)
{
	std::vector < SmfUpgradeStep * >::iterator iter;
	iter = m_procSteps.begin();

	i_step->setProcedure(this);

	while (iter != m_procSteps.end()) {
		if ((*iter)->getRdn() > i_step->getRdn()) {
			/* Insert new step before this one */
			m_procSteps.insert(iter, i_step);
			return;
		}
		iter++;
	}

	m_procSteps.push_back(i_step);
}

//------------------------------------------------------------------------------
// addStepModifications()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModifications(SmfUpgradeStep * i_newStep,
                                          const std::list < SmfTargetEntityTemplate * >&i_targetEntityTemplate,
                                          SmfAuT i_auType,
					  std::multimap<std::string, objectInst> &i_objects)
{
	//This method is called for each calculated step. The purpose is to find out and add the modifications 
	//which should be carried out for this step. The targetEntityTemplate parent/type part of the procedure (in the campaign) 
	//is used to match the steps activation/deactivation units. 
	//If a match is found the modifications associated with this parent/type shall be added to the step.
        TRACE_ENTER();

        //Skip this for procedures in state completed, modifications will not be needed if completed.
        //This can happend if the cluster is rebooted and will fail if the reboot is performed when the 
        //versioned types are removed i.e. during test traffic, if the types was removed in campaign wrapup/complete section.
        if (getState() == SA_SMF_PROC_COMPLETED) {
                TRACE_LEAVE();
                return true;
        }

	std::list < SmfTargetEntityTemplate * >::const_iterator it;

	//For each targetEntityTemplate in the procedure
	for (it = i_targetEntityTemplate.begin(); it != i_targetEntityTemplate.end(); ++it) {
		SmfTargetEntityTemplate *targetEntity = *it;
		const SmfParentType *parentType = targetEntity->getEntityTemplate();
		const std::list < SmfImmModifyOperation * >&modificationList = targetEntity->getModifyOperationList();

		switch (i_auType) {
		case SMF_AU_AMF_NODE: /* Activation unit is Node */
			{
				if (!addStepModificationsNode(i_newStep, parentType, modificationList, i_objects)) {
                                        LOG_NO("SmfUpgradeProcedure::addStepModifications: addStepModificationsNode failed");
                                        return false;
                                }
				break;
			}
		case SMF_AU_SU_COMP: /* Activation unit is SU or Component */
			{
				if ( !addStepModificationsSuOrComp(i_newStep, parentType, modificationList, i_objects)) {
                                        LOG_NO("SmfUpgradeProcedure::addStepModifications: addStepModificationsSuOrComp failed");
                                        return false;
                                }
				break;
			}
		default:
                        
                        LOG_NO("SmfUpgradeProcedure::addStepModifications: unknown auType (%i)", i_auType);
                        return false;
			break;
		}
	}

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// addStepModificationsNode()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsNode(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
                                              const std::list < SmfImmModifyOperation * >&i_modificationList,
					      std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	std::list < std::string > objectList;
	std::list < std::string >::const_iterator objit;
	const std::string & auNodeName = i_newStep->getActivationUnitList().front().name;
	std::multimap<std::string, objectInst>::iterator iter;
	std::pair<std::multimap<std::string, objectInst>::iterator, std::multimap<std::string, objectInst>::iterator> nodeName_mm;

	nodeName_mm = i_objects.equal_range(auNodeName);

	std::string typeDn   = i_parentType->getTypeDn();
	std::string parentDn = i_parentType->getParentDn();

	//Find instances matching the parent/type pair in template
	if (typeDn.size() > 0) {  //Type and possibly also parent is set in parent/type pair
		TRACE("Check type %s for modifications", typeDn.c_str());

		//Check if the parent exist (if given)
		if (parentDn.size() > 0) {
			if (immUtil.getObject(typeDn, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: A none existing parent (%s) in <targetEntityTemplate>.", parentDn.c_str());
				TRACE_LEAVE();				
				return false;
			}
		}

		//Check if typeDN is a valid type
		if ((typeDn.find(",safSuType=") == std::string::npos) && 
		    (typeDn.find(",safCompType=") == std::string::npos)){
			LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: Unknown type %s in <targetEntityTemplate>, only safSuType and safCompType allowed.", typeDn.c_str());
			return false;	
		}

		//Check if the versioned comp or su type type pointed out exist
		if (immUtil.getObject(typeDn, &attributes) == false) {
			LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: A none existing version type (%s) in <targetEntityTemplate>.", typeDn.c_str());
			TRACE_LEAVE();
			return false;
		}

		//Check for SU or Comp type match
		if (typeDn.find(",safSuType=") != std::string::npos) { //Check for SU versioned type
			TRACE("Check SU type %s for modifications", typeDn.c_str());
			// Find objects hosted on this node
			// Loop through range of maps of key auNodeName
			std::list<std::string> matchingSU;
			for (iter = nodeName_mm.first;  iter != nodeName_mm.second;  ++iter)  
			{
				//This is an entity hosted by this node
				//Check if matching SG (parent)
				if ((parentDn.size() > 0) && (parentDn != (*iter).second.sgDN)){
					TRACE("Instance %s is NOT matching parent", (*iter).second.sgDN.c_str());
					continue; //Take next if parent DN is given but does not match
				}
				//Check if matching SU or component versioned trype
				if (typeDn != (*iter).second.suTypeDN) { 
					TRACE("Instance %s is NOT matching versioned type", (*iter).second.suDN.c_str());
					continue; //Take next if versioned type DN is given but does not match
				}

				TRACE("Instance %s is matching modification template", (*iter).second.suDN.c_str());
				//The i_objects map contain all instances on component level.
				//One SU may give multiple matches since one SU may contain several components.
				//Save the matching enties and remove duplicates when all matching is finished.
				matchingSU.push_back((*iter).second.suDN);
			}

			//Remove duplicates
			matchingSU.sort();
			matchingSU.unique();

			//Add object modifications
			std::list < std::string >::const_iterator strIter;
			for (strIter = matchingSU.begin(); strIter != matchingSU.end(); ++strIter) {
				if (!addStepModificationList(i_newStep, *strIter, i_modificationList)) {
					LOG_NO("SmfUpgradeProcedure::addStepModificationsSu: addStepModificationList fails");
					TRACE_LEAVE();
					return false;
				}
			}
		} else if (typeDn.find(",safCompType=") != std::string::npos) { //Check for Comp versioned type
			TRACE("Check Comp type %s for modifications", typeDn.c_str());
			// Find objects hosted on this node
			// Loop through range of maps of key auNodeName
			for (iter = nodeName_mm.first;  iter != nodeName_mm.second;  ++iter)  
			{
				//This is an entity hosted by this node
				//Check if matching SG
				if ((parentDn.size() > 0) && (parentDn != (*iter).second.sgDN)){
					TRACE("Instance %s is NOT matching parent", (*iter).second.sgDN.c_str());
					continue; //Take next if parent DN is given but does not match
				}
				//Check if matching Component versioned trype
				if (typeDn != (*iter).second.compTypeDN) { 
					TRACE("Instance %s is NOT matching versioned type", (*iter).second.compDN.c_str());
					continue; //Take next if versioned type DN is given but does not match
				}

				TRACE("Instance %s is matching modification template", (*iter).second.compDN.c_str());

				if (!addStepModificationList(i_newStep, (*iter).second.compDN, i_modificationList)) {
					LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: addStepModificationList fails");
					TRACE_LEAVE();
					return false;
				}
			}
		} else {
			//The typeDn was not Comp or SU type
			LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: Unknown type %s in <targetEntityTemplate>, only safSuType and safCompType allowed.", typeDn.c_str());
			TRACE_LEAVE();
			return false;
		}
	} else if (parentDn.size() > 0) { //Parent only is set in parent/type pair
                TRACE("Check SU children to parent %s for modifications", parentDn.c_str());
			// Find objects hosted on this node
			// Loop through range of maps of key auNodeName
			for (iter = nodeName_mm.first;  iter != nodeName_mm.second;  ++iter)  
			{
				//This is an entity hosted by this node
				//Check if matching SG
				if (parentDn != (*iter).second.sgDN){
					TRACE("Instance %s is NOT matching parent", (*iter).second.sgDN.c_str());
					continue; //Take next if parent DN is given but does not match
				}

				TRACE("Instance %s is matching modification template", (*iter).second.suDN.c_str());

				if (!addStepModificationList(i_newStep, (*iter).second.suDN, i_modificationList)) {
					LOG_NO("SmfUpgradeProcedure::addStepModificationsNode: addStepModificationList fails");
					TRACE_LEAVE();
					return false;
				}
			}
        } else {
		//No parent type was set. 
		//Since this AU/DU is on node level the DN shall not be inherited from the AU/DU and the
		//modifications shall not be added.
		//Modifications can only be added if SU or Component is addressed.
		TRACE("addStepModificationsNode called without parent/dn in target entity template, must be SW installation step.");
        }

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// addStepModificationsSuOrComp()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsSuOrComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
						const std::list < SmfImmModifyOperation * >&i_modificationList,
						std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	std::string typeDn   = i_parentType->getTypeDn();
	std::string parentDn = i_parentType->getParentDn();

        if (typeDn.size() > 0) { //Type is set, parent may be set to limit the scope
                /* Type was specified, find all objects of this version type */
                TRACE("Check type %s for modifications", typeDn.c_str());

		//Check if the parent exist (if given)
		if (parentDn.size() > 0) {
			if (immUtil.getObject(typeDn, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: A none existing parent (%s) in <targetEntityTemplate>.", parentDn.c_str());
				return false;
			}
		}

		//Check if typeDN is a valid type
		if ((typeDn.find(",safSuType=") == std::string::npos) && 
		    (typeDn.find(",safCompType=") == std::string::npos)){
			LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: Unknown type %s in <targetEntityTemplate>, only safSuType and safCompType allowed.", typeDn.c_str());
			return false;	
		}

		//Check if the versioned comp or su type type pointed out exist
		if (immUtil.getObject(typeDn, &attributes) == false) {
			LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: A none existing version type (%s) is specified in a <targetEntityTemplate>.", typeDn.c_str());
			return false;
		}

		if (typeDn.find(",safSuType=") != std::string::npos) {
			//Find instances of a SU Type
			if (addStepModificationsSu(i_newStep, i_parentType, i_modificationList, i_objects) == false){
				LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: addStepModificationsSu fail");
				return false;
			}
		} else if (typeDn.find(",safCompType=") != std::string::npos) {
			//Find instances of a Comp Type
			if (addStepModificationsComp(i_newStep, i_parentType, i_modificationList, i_objects)  == false){
				LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: addStepModificationsComp fail");
				return false;
			}
		}
	} else if (parentDn.size() > 0) { //Parent only is set, no type in template parent/type
		if (addStepModificationsParentOnly(i_newStep, i_parentType, i_modificationList, i_objects)  == false){
			LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: addStepModificationsParentOnly fail");
			return false;
		}

	} else {
		//No parent type was set, apply the modifications on the entity pointed out by the activation units.
		//The optional modifyOperation RDN attribute if added when applying the modification
		std::list < unitNameAndState >::const_iterator auEntityIt;
		const std::list < unitNameAndState > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
		for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
			if (!addStepModificationList(i_newStep, (*auEntityIt).name, i_modificationList)) {
				LOG_NO("SmfUpgradeProcedure::addStepModificationsSuOrComp: addStepModificationList fails");
				return false;
			}
		}
	}

        TRACE_LEAVE();
        return true;
}


//------------------------------------------------------------------------------
// addStepModificationList()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationList(SmfUpgradeStep * i_newStep, const std::string & i_dn,
						  const std::list < SmfImmModifyOperation * >&i_modificationList)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;

        //The modifications must be copied, since several steps can use the same modification list
        //but with different DNs. Deep copy not neccessary since only DN is changed.

        //If RND is set, modifications shall be applied to entities matching DN + RDN. This is used to
        //apply changes to a certain Component within a SU, where DN is the SU and RDN is the component.

	std::list < SmfImmModifyOperation * >::const_iterator modit;
	for (modit = i_modificationList.begin(); modit != i_modificationList.end(); ++modit) {
		if ((*modit)->getRdn().size() > 0) {
			std::string dn = (*modit)->getRdn(); 
			dn += "," + i_dn;
                        
                        //Check if object exist
                        SaImmAttrValuesT_2 **attributes;
                        if (immUtil.getObject(dn, &attributes) == true) {
                                SmfImmModifyOperation *modOp = new SmfImmModifyOperation(*(*modit));
                                modOp->setDn(dn);
                                i_newStep->addModification(modOp);
                        }
                } else {
                        SmfImmModifyOperation *modOp = new SmfImmModifyOperation(*(*modit));
			modOp->setDn(i_dn);
                        i_newStep->addModification(modOp);
                }
	}

        TRACE_LEAVE();
        return true;
}
//------------------------------------------------------------------------------
// addStepModificationsSu()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsSu(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
					    const std::list < SmfImmModifyOperation * >&i_modificationList,
					    std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();

	std::list < std::string >::const_iterator strIter;
        std::list < unitNameAndState >::const_iterator auEntityIt;
        const std::list < unitNameAndState > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
	std::string typeDn   = i_parentType->getTypeDn();
	std::string parentDn = i_parentType->getParentDn();

	TRACE("Find instances matching SU type %s", typeDn.c_str());

	// Type DN given in the targetEntityTemplate parent/type pair is of class SaAmfSUType,
	// find all SU's in the system matching this version type
	// The i_objects map contain information about all Comp, SU and SG and which Nodes are hosting
	// For all objects (instances) in the map, match the parent and SU type given in the template.

	std::multimap<std::string, objectInst>::iterator objit;
	for( objit = i_objects.begin(); objit != i_objects.end(); ++objit) { //For all component instances in the system
		//Filter out instances not within SG (parent) scope, if given.
		if ((parentDn.size() > 0) && (parentDn != (*objit).second.sgDN)){
			TRACE("Instance %s is NOT matching parent", (*objit).second.sgDN.c_str());
			continue; //Take next if parent DN is given but does not match
		}

		if (typeDn == (*objit).second.suTypeDN) {
			// This SU is of the correct SU version type
			// In single step procedures several activation/deactivation unit can be in the single step. 
			// Check if the matching SU is within the scope (DN) of the activation/deactivation unit.
			// In this (addStepModificationsSu) method the AU/DU is always a SU, never a Node?????????
			std::list<std::string> matchingSU;
			for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
				//This test is because in single step upgrades the SU/Comp step mod routines are called 
				//also when the AU/DU is a node.
				if((*auEntityIt).name.find("safAmfNode=") == 0) { //Node as AU/DU
					if((*objit).second.nodeDN == (*auEntityIt).name) {
						TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
						matchingSU.push_back((*objit).second.suDN);
						break;
					}
				}

				if((*objit).second.suDN == (*auEntityIt).name) {
					TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
					//The i_objects map contain all instances on component level.
					//One SU may give multiple matches since one SU may contain several components.
					//Save the matching enties and remove duplicates when all matching is finished.
					matchingSU.push_back((*objit).second.suDN);
					break;
				} else {
					TRACE("Instance %s is NOT within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
				}
			}
			//Remove duplicates
			matchingSU.sort();
			matchingSU.unique();

			//Add object modifications
			for (strIter = matchingSU.begin(); strIter != matchingSU.end(); ++strIter) {
				if (!addStepModificationList(i_newStep, *strIter, i_modificationList)) {
					LOG_NO("SmfUpgradeProcedure::addStepModificationsSu: addStepModificationList fails");
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
// addStepModificationsComp()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
					      const std::list < SmfImmModifyOperation * >&i_modificationList,
					      std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();

	std::list < std::string >::const_iterator strIter;
        std::list < unitNameAndState >::const_iterator auEntityIt;
        const std::list < unitNameAndState > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
	std::string typeDn   = i_parentType->getTypeDn();
	std::string parentDn = i_parentType->getParentDn();

	TRACE("Find instances matching Comp type %s", typeDn.c_str());

	// Type DN given in the targetEntityTemplate parent/type pair is of class SaAmfCompType,
	// find all Components in the system matching this version type
	// The i_objects map contain information about all Comp, SU and SG and which Nodes are hosting
	// For all objects (instances) in the map, match the parent and Comp type given in the template.

	std::multimap<std::string, objectInst>::iterator objit;
	for( objit = i_objects.begin(); objit != i_objects.end(); ++objit) { //For all component instances in the system
		//Filter out instances not within SG (parent) scope, if given.
		if ((parentDn.size() > 0) && (parentDn != (*objit).second.sgDN)){
			TRACE("Instance %s is NOT matching parent", (*objit).second.sgDN.c_str());
			continue; //Take next if parent DN is given but does not match
		}

		if (typeDn == (*objit).second.compTypeDN) {
			// This Component is of the correct Component version type
			// In single step procedures several activation/deactivation unit can be in the single step. 
			// Check if the matching Component is within the scope (DN) of the activation/deactivation unit.
			for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
				//This test is because in single step upgrades the SU/Comp step mod routines are called 
				//also when the AU/DU is a node.
				if((*auEntityIt).name.find("safAmfNode=") == 0) { //Node as AU/DU
					if((*objit).second.nodeDN == (*auEntityIt).name) {
						TRACE("Instance %s is within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).name.c_str());
						if (!addStepModificationList(i_newStep, (*objit).second.compDN, i_modificationList)) {
							LOG_NO("SmfUpgradeProcedure::addStepModificationsComp: addStepModificationList fails");
							TRACE_LEAVE();
							return false;
						}
					} else {
						TRACE("Instance %s is NOT within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).name.c_str());
					}
				}

				//If the AU/DU DN is found within the Component DN, the component is within the scope
				if(((*objit).second.compDN).find((*auEntityIt).name) != std::string::npos) {
					TRACE("Instance %s is within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).name.c_str());
					if (!addStepModificationList(i_newStep, (*objit).second.compDN, i_modificationList)) {
						LOG_NO("SmfUpgradeProcedure::addStepModificationsComp: addStepModificationList fails");
						TRACE_LEAVE();
						return false;
					}
				} else {
					TRACE("Instance %s is NOT within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).name.c_str());
				}
			}
		}
	}

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// addStepModificationsParentOnly()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsParentOnly(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
						    const std::list < SmfImmModifyOperation * >&i_modificationList,
						    std::multimap<std::string, objectInst> &i_objects)
{
        TRACE_ENTER();

	std::list < std::string >::const_iterator strIter;
        std::list < unitNameAndState >::const_iterator auEntityIt;
        const std::list < unitNameAndState > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
	std::string typeDn   = i_parentType->getTypeDn();
	std::string parentDn = i_parentType->getParentDn();

	TRACE("Find instances (SU's) which are children to parent (SG) %s, given in template parent/type", parentDn.c_str());
	// Loop through range of maps of key auNodeName
	std::multimap<std::string, objectInst>::iterator objit;
	for( objit = i_objects.begin(); objit != i_objects.end(); ++objit) { //For all component instances in the system
		//Check if matching SG
		if (parentDn != (*objit).second.sgDN){
			TRACE("Instance %s is NOT matching parent", (*objit).second.sgDN.c_str());
			continue; //Take next if parent DN is given but does not match
		}

		TRACE("Instance %s is matching modification template", (*objit).second.suDN.c_str());

		// This SU is a children to the given SG
		// In single step procedures several activation/deactivation unit can be in the single step. 
		// Check if the matching SU is within the scope (DN) of the activation/deactivation unit.
		std::list<std::string> matchingSU;
		for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
			//This test is because in single step upgrades the SU/Comp step mod routines are called 
			//also when the AU/DU is a node.
			if((*auEntityIt).name.find("safAmfNode=") == 0) { //Node as AU/DU
				if((*objit).second.nodeDN == (*auEntityIt).name) {
					TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
					//The i_objects map contain all instances on component level.
					//One SU may give multiple matches since one SU may contain several components.
					//Save the matching enties and remove duplicates when all matching is finished.
					matchingSU.push_back((*objit).second.suDN);
					break;
				}
			}

			if((*objit).second.suDN == (*auEntityIt).name) {
				TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
				//The i_objects map contain all instances on component level.
				//One SU may give multiple matches since one SU may contain several components.
				//Save the matching enties and remove duplicates when all matching is finished.
				matchingSU.push_back((*objit).second.suDN);
				break;
			} else {
				TRACE("Instance %s is NOT within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).name.c_str());
			}
		}
		//Remove duplicates
		matchingSU.sort();
		matchingSU.unique();

		//Add object modifications
		for (strIter = matchingSU.begin(); strIter != matchingSU.end(); ++strIter) {
			if (!addStepModificationList(i_newStep, *strIter, i_modificationList)) {
				LOG_NO("SmfUpgradeProcedure::addStepModificationsParentOnly: addStepModificationList fails");
				TRACE_LEAVE();
				return false;
			}
		}
	}

	TRACE_LEAVE();
	return true;
}
//------------------------------------------------------------------------------
// createImmSteps()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::createImmSteps()
{
	TRACE_ENTER();
	SaAisErrorT rc;

	std::vector < SmfUpgradeStep * >::iterator iter;
	iter = m_procSteps.begin();
	while (iter != m_procSteps.end()) {
		rc = createImmStep(*iter);
                if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
                        LOG_NO("SmfUpgradeProcedure::createImmSteps: creation of SaSmfStep object structure fails, rc=%s", saf_error(rc));
                        return false;
                }
		iter++;
	}

	TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// createImmStep()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::createImmStep(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	SaAisErrorT rc = SA_AIS_OK;
        std::string dnDeactUnit = "safSmfDu=smfDeactivationUnit";
        std::string dnActUnit   = "safSmfAu=smfActivationUnit";
	int strSize = 64;
        char str[strSize];

        /* Create the SaSmfStep object */
        SmfImmRTCreateOperation icoSaSmfStep;
        icoSaSmfStep.setClassName("SaSmfStep");
        icoSaSmfStep.setParentDn(getDn());
        icoSaSmfStep.setImmHandle(getProcThread()->getImmHandle());

        SmfImmAttribute attrSafSmfStep;
        attrSafSmfStep.setName("safSmfStep");
        attrSafSmfStep.setType("SA_IMM_ATTR_SASTRINGT");
        attrSafSmfStep.addValue(i_step->getRdn());
        icoSaSmfStep.addValue(attrSafSmfStep);

        SmfImmAttribute attrsaSmfStepMaxRetry;
        attrsaSmfStepMaxRetry.setName("saSmfStepMaxRetry");
        attrsaSmfStepMaxRetry.setType("SA_IMM_ATTR_SAUINT32T");
        snprintf(str,strSize, "%d",i_step->getMaxRetry());
        attrsaSmfStepMaxRetry.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepMaxRetry);

        SmfImmAttribute attrsaSmfStepRetryCount;
        attrsaSmfStepRetryCount.setName("saSmfStepRetryCount");
        attrsaSmfStepRetryCount.setType("SA_IMM_ATTR_SAUINT32T");
        snprintf(str, strSize,"%d",i_step->getRetryCount());
        attrsaSmfStepRetryCount.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepRetryCount);
 
        SmfImmAttribute attrsaSmfStepRestartOption;
        attrsaSmfStepRestartOption.setName("saSmfStepRestartOption");
        attrsaSmfStepRestartOption.setType("SA_IMM_ATTR_SAUINT32T");
        snprintf(str, strSize, "%d",i_step->getRestartOption());
        attrsaSmfStepRestartOption.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepRestartOption);

        SmfImmAttribute attrsaSmfStepState;
        attrsaSmfStepState.setName("saSmfStepState");
        attrsaSmfStepState.setType("SA_IMM_ATTR_SAUINT32T");
        snprintf(str, strSize, "%d",i_step->getState());
        attrsaSmfStepState.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepState);

        SmfImmAttribute attrsaSmfStepError;
        attrsaSmfStepError.setName("saSmfStepError");
        attrsaSmfStepError.setType("SA_IMM_ATTR_SASTRINGT");
        attrsaSmfStepError.addValue("");
        icoSaSmfStep.addValue(attrsaSmfStepError);

	// Accept rc = SA_AIS_ERR_EXIST. This could happend if the execution 
	// is taken over by the other controller.
        rc = icoSaSmfStep.execute(); //Create the object
	if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
                LOG_NO("SmfUpgradeProcedure::createImmStep: Creation of SaSmfStep object fails, rc=%s, [dn=%s]", 
		       saf_error(rc), 
		       (i_step->getRdn() + "," + getDn()).c_str());
                TRACE_LEAVE();
                return rc;
        }

	std::list < unitNameAndState >::const_iterator iter;
	std::list < unitNameAndState >::const_iterator iterE;

	/* Create the SaSmfDeactivationUnit object if there ia any object to deactivate */
	const std::list < unitNameAndState >& deactList = i_step->getDeactivationUnitList();
	if (deactList.size() != 0) {
		SmfImmRTCreateOperation icoSaSmfDeactivationUnit;
		icoSaSmfDeactivationUnit.setClassName("SaSmfDeactivationUnit");
		icoSaSmfDeactivationUnit.setParentDn(i_step->getRdn() + "," + getDn());
		icoSaSmfDeactivationUnit.setImmHandle(getProcThread()->getImmHandle());

		SmfImmAttribute attrsafSmfDu;
		attrsafSmfDu.setName("safSmfDu");
		attrsafSmfDu.setType("SA_IMM_ATTR_SASTRINGT");
		attrsafSmfDu.addValue("safSmfDu=smfDeactivationUnit");
		icoSaSmfDeactivationUnit.addValue(attrsafSmfDu);

		SmfImmAttribute attrsaSmfDuActedOn;
		attrsaSmfDuActedOn.setName("saSmfDuActedOn");
		attrsaSmfDuActedOn.setType("SA_IMM_ATTR_SANAMET");

		iter = deactList.begin();
		iterE = deactList.end();
		while (iter != iterE) {
			attrsaSmfDuActedOn.addValue((*iter).name);
			iter++;
		}

		icoSaSmfDeactivationUnit.addValue(attrsaSmfDuActedOn);

		SmfImmAttribute attrsaSmfDuEntityToRemove;
		attrsaSmfDuEntityToRemove.setName("saSmfDuEntityToRemove");
		attrsaSmfDuEntityToRemove.setType("SA_IMM_ATTR_SANAMET");

		if (!setEntitiesToAddRemMod(i_step, &attrsaSmfDuEntityToRemove)) {
			rc = SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED;
		}
		icoSaSmfDeactivationUnit.addValue(attrsaSmfDuEntityToRemove);

		rc = icoSaSmfDeactivationUnit.execute(); //Create the object
		if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
			LOG_NO("SmfUpgradeProcedure::createImmStep: Creation of SaSmfDeactivationUnit object fails, rc=%s, [dn=%s]",
			       saf_error(rc),
			       ("safSmfDu=smfDeactivationUnit," + i_step->getRdn() + "," + getDn()).c_str());
			TRACE_LEAVE();
			return rc;
		}

		/* Create the SaSmfImageNodes objects as childrens to the SaSmfDeactivationUnit instance */
		/* One SaSmfImageNodes object for each removed software bundle                           */

		std::list < SmfBundleRef >::const_iterator bundleRefiter;
		std::list < SmfBundleRef >::const_iterator bundleRefiterE;

		const std::list < SmfBundleRef > removedBundles = i_step->getSwRemoveList();  //Get the list of bundles to remove in this step
		bundleRefiter = removedBundles.begin();
		bundleRefiterE = removedBundles.end();

		//For each sw bundle, create a SaSmfImageNodes object as child to the SaSmfDeactivationUnit
		while (bundleRefiter != bundleRefiterE) {
			SmfImmRTCreateOperation icoSaSmfImageNodes;
			icoSaSmfImageNodes.setClassName("SaSmfImageNodes");
			icoSaSmfImageNodes.setParentDn(dnDeactUnit + "," + i_step->getRdn() + "," + getDn());
			icoSaSmfImageNodes.setImmHandle(getProcThread()->getImmHandle());

			SmfImmAttribute safIMageNode;
			safIMageNode.setName("safImageNode");
			safIMageNode.setType("SA_IMM_ATTR_SASTRINGT");
			//Extract the bundle name from the DN
			std::string imageNode = "safImageNode=";
			std::string bundleName = (*bundleRefiter).getBundleDn();
			std::string::size_type pos = bundleName.find("=");
			if (pos == std::string::npos) {
				LOG_NO("SmfUpgradeProcedure::createImmStep: No \"=\" separator found in bundle name [dn=%s]", bundleName.c_str());
				TRACE_LEAVE();
				return SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED;
			}
			pos++;
			imageNode += bundleName.substr(pos, bundleName.find(",") - pos);
			safIMageNode.addValue(imageNode);
			icoSaSmfImageNodes.addValue(safIMageNode);

			SmfImmAttribute saSmfINSwBundle;
			saSmfINSwBundle.setName("saSmfINSwBundle");
			saSmfINSwBundle.setType("SA_IMM_ATTR_SANAMET");
			saSmfINSwBundle.addValue((*bundleRefiter).getBundleDn());
			icoSaSmfImageNodes.addValue(saSmfINSwBundle);

			SmfImmAttribute saSmfINNode;
			saSmfINNode.setName("saSmfINNode");
			saSmfINNode.setType("SA_IMM_ATTR_SANAMET");
               
			//Rolling steps contain only one node
			//Single step may contain several nodes
			if (getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				saSmfINNode.addValue(i_step->getSwNode());
			} else {  //SA_SMF_SINGLE_STEP
				std::list<std::string> swNodeList;
				if (i_step->calculateSingleStepNodes(bundleRefiter->getPlmExecEnvList(), swNodeList)) {
					std::list < std::string >::const_iterator it;
					for (it = swNodeList.begin(); it != swNodeList.end(); ++it) {
						saSmfINNode.addValue(*it);
					}
				} else {
					LOG_NO("SmfUpgradeProcedure::createImmStep: Can not calculate nodes for SaSmfINNode attribute, continue");
				}
			}

			//Fail the campaign if no node is given for the bundle.
			if(saSmfINNode.getValues().size() == 0) {
				LOG_NO("SmfUpgradeProcedure::createImmStep: No node given for bundle %s", bundleName.c_str());
				return SA_AIS_ERR_NOT_EXIST;
			}

			icoSaSmfImageNodes.addValue(saSmfINNode);
			rc = icoSaSmfImageNodes.execute(); //Create the object
			if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
				LOG_NO("SmfUpgradeProcedure::createImmStep: Creation of SaSmfImageNodes object fails, rc=%s, [dn=%s]",
				       saf_error(rc),
				       (imageNode + "," + dnDeactUnit + "," + i_step->getRdn() + "," + getDn()).c_str());
				TRACE_LEAVE();
				return rc;
			}

			bundleRefiter++;
		}
	}

	/* Create the SaSmfActivationUnit object if there is any object to activate */
	const std::list < unitNameAndState >& actList = i_step->getActivationUnitList();
	if(actList.size() != 0) {
		SmfImmRTCreateOperation icoSaSmfActivationUnit;
		icoSaSmfActivationUnit.setClassName("SaSmfActivationUnit");
		icoSaSmfActivationUnit.setParentDn(i_step->getRdn() + "," + getDn());
		icoSaSmfActivationUnit.setImmHandle(getProcThread()->getImmHandle());

		SmfImmAttribute attrsafSmfAu;
		attrsafSmfAu.setName("safSmfAu");
		attrsafSmfAu.setType("SA_IMM_ATTR_SASTRINGT");
		attrsafSmfAu.addValue("safSmfAu=smfActivationUnit");
		icoSaSmfActivationUnit.addValue(attrsafSmfAu);

		SmfImmAttribute attrsaSmfAuActedOn;
		attrsaSmfAuActedOn.setName("saSmfAuActedOn");
		attrsaSmfAuActedOn.setType("SA_IMM_ATTR_SANAMET");
		const std::list < unitNameAndState > actList = i_step->getActivationUnitList();
		iter = actList.begin();
		iterE = actList.end();
		while (iter != iterE) {
			attrsaSmfAuActedOn.addValue((*iter).name);
			iter++;
		}

		icoSaSmfActivationUnit.addValue(attrsaSmfAuActedOn);

		SmfImmAttribute attrsaSmfAuEntityToAdd;
		attrsaSmfAuEntityToAdd.setName("saSmfAuEntityToAdd");
		attrsaSmfAuEntityToAdd.setType("SA_IMM_ATTR_SANAMET");

		if (!setEntitiesToAddRemMod(i_step, &attrsaSmfAuEntityToAdd)) {
			rc = SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED;
		}
		icoSaSmfActivationUnit.addValue(attrsaSmfAuEntityToAdd);

		rc = icoSaSmfActivationUnit.execute(); //Create the object
		if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
			LOG_NO("SmfUpgradeProcedure::createImmStep: Creation of SaSmfActivationUnit object fails [rdn=%s], rc=%s",
			       saf_error(rc),
			       ("safSmfAu=smfActivationUnit," + i_step->getRdn() + "," + getDn()).c_str());
			TRACE_LEAVE();
			return rc;
		}

		/* Create the SaSmfImageNodes objects as childrens to the SaSmfActivationUnit instance */
		/* One SaSmfImageNodes object for each added software bundle                           */

		std::list < SmfBundleRef >::const_iterator bundleRefiter;
		std::list < SmfBundleRef >::const_iterator bundleRefiterE;

		const std::list < SmfBundleRef > addBundles = i_step->getSwAddList();  //Get the list of bundles to add in this step
		bundleRefiter = addBundles.begin();
		bundleRefiterE = addBundles.end();

		//For each sw bundle, create a SaSmfImageNodes object as child to the SaSmfActivationUnit
		while (bundleRefiter != bundleRefiterE) {
			SmfImmRTCreateOperation icoSaSmfImageNodes;
			icoSaSmfImageNodes.setClassName("SaSmfImageNodes");
			icoSaSmfImageNodes.setParentDn(dnActUnit + "," + i_step->getRdn() + "," + getDn());
			icoSaSmfImageNodes.setImmHandle(getProcThread()->getImmHandle());

			SmfImmAttribute safIMageNode;
			safIMageNode.setName("safImageNode");
			safIMageNode.setType("SA_IMM_ATTR_SASTRINGT");
			//Extract the bundle name from the DN
			std::string imageNode = "safImageNode=";
			std::string bundleName = (*bundleRefiter).getBundleDn();
			std::string::size_type pos = bundleName.find("=");
			//Check if the "=" for some reason is not found
			//This is only a recording object, dont fail just coninue.
			if(pos == std::string::npos) 
				break;
			pos++;
			imageNode += bundleName.substr(pos, bundleName.find(",") - pos);
			safIMageNode.addValue(imageNode);
			icoSaSmfImageNodes.addValue(safIMageNode);

			SmfImmAttribute saSmfINSwBundle;
			saSmfINSwBundle.setName("saSmfINSwBundle");
			saSmfINSwBundle.setType("SA_IMM_ATTR_SANAMET");
			saSmfINSwBundle.addValue((*bundleRefiter).getBundleDn());
			icoSaSmfImageNodes.addValue(saSmfINSwBundle);

			SmfImmAttribute saSmfINNode;
			saSmfINNode.setName("saSmfINNode");
			saSmfINNode.setType("SA_IMM_ATTR_SANAMET");

			//Rolling steps contain only one node
			//Single step may contain several nodes
			if (getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				saSmfINNode.addValue(i_step->getSwNode());
			} else {  //SA_SMF_SINGLE_STEP
				std::list<std::string> swNodeList;
				if (i_step->calculateSingleStepNodes(bundleRefiter->getPlmExecEnvList(), swNodeList)) {
					std::list < std::string >::const_iterator it;
					for (it = swNodeList.begin(); it != swNodeList.end(); ++it) {
						saSmfINNode.addValue(*it);
					}
				} else {
					LOG_NO("SmfUpgradeProcedure::createImmStep: Can not calculate nodes for SaSmfINNode attribute, continue");
				}
			}

			//Fail the campaign if no node is given for the bundle.
			if(saSmfINNode.getValues().size() == 0) {
				LOG_NO("SmfUpgradeProcedure::createImmStep: No node given for bundle %s", bundleName.c_str());
				return SA_AIS_ERR_NOT_EXIST;
			}

			icoSaSmfImageNodes.addValue(saSmfINNode);
			rc = icoSaSmfImageNodes.execute(); //Create the object
			if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
				LOG_NO("SmfUpgradeProcedure::createImmStep: Creation of SaSmfImageNodes object fails, rc=%s, [dn=%s]", 
				       saf_error(rc),  
				       (imageNode + dnActUnit + "," + i_step->getRdn() + "," + getDn()).c_str());
				TRACE_LEAVE();
				return rc;
			}

			bundleRefiter++;
		}
	}
	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// getImmSteps()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::getImmSteps()
{
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();
	SmfUpgradeMethod *upgradeMethod = getUpgradeMethod();
	if (upgradeMethod == NULL) {
		LOG_NO("SmfUpgradeProcedure::getImmSteps: no upgrade method found");
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	if (upgradeMethod->getUpgradeMethod() == SA_SMF_ROLLING) {
		TRACE("Rolling upgrade");
		rc = getImmStepsRolling();
	} else if (upgradeMethod->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
                if (SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->getProcExecutionMode()
		    == SMF_MERGE_TO_SINGLE_STEP) {  //This is a merged single step
                        TRACE("Merged single step upgrade");
                        rc = getImmStepsMergedSingleStep();
                } else { //This is a written normal single step
                        TRACE("Single step upgrade");
                        rc = getImmStepsSingleStep();
                }
	} else {
		LOG_NO("SmfUpgradeProcedure::getImmSteps: no upgrade method type found");
		rc =  SA_AIS_ERR_NOT_EXIST;
	}

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// getImmStepsRolling()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::getImmStepsRolling()
{
	TRACE_ENTER();
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;
	SaAisErrorT rc = SA_AIS_OK;
	std::list < std::string > stepList;
	std::multimap<std::string, objectInst> objInstances;

	if (!getImmComponentInfo(objInstances)) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: config info from IMM could not be read");
		return SA_AIS_ERR_INIT;
	}

	SmfRollingUpgrade * upgradeMethod = (SmfRollingUpgrade*)getUpgradeMethod();
	if (upgradeMethod == NULL) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: no upgrade method found");
		return SA_AIS_ERR_INIT;
	}

	getCallbackList(upgradeMethod);

	// Read the steps from IMM
	if (immutil.getChildren(getDn(), stepList, SA_IMM_SUBLEVEL, "SaSmfStep") == false) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: Failed to get steps for procedure %s", getDn().c_str());
		return SA_AIS_ERR_NOT_EXIST;
	}

	// We need to sort the list of steps here since the order from IMM is not guaranteed
	stepList.sort();

	std::list < std::string >::iterator stepit;
	TRACE("Fetch IMM data for our upgrade procedure steps");

	/* Fetch IMM data for our upgrade procedure steps */
	for (stepit = stepList.begin(); stepit != stepList.end(); ++stepit) {
		if (immutil.getObject((*stepit), &attributes) == false) {
			LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: IMM data for step %s not found", (*stepit).c_str());
			rc = SA_AIS_ERR_NOT_EXIST;
			goto done;
		}

		TRACE("Fetch IMM data for item in stepList");

		SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
                osafassert(newStep != NULL);

		if (newStep->init((const SaImmAttrValuesT_2 **)attributes) != SA_AIS_OK) {
			LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: Initialization failed for step %s", (*stepit).c_str());
			delete newStep;
			rc = SA_AIS_ERR_INIT;
			goto done;
		}

		newStep->setDn((*stepit));
		newStep->setProcedure(this);

		// After a switchover the new SMFD must:
		// For all steps in status Initial and Executing, 
		// recreate the SMFD internal steps as it was after initial calculation
		// This is needed because the steps are calculated "on the fly"
		// on the active node and the result is partly saved in IMM and partly
		// stored in local memory when a new procedure is started.
		// The AU/DU and software bundles can be fetched from IMM.
		// The modification must be recalculated.
                
                TRACE("SmfUpgradeProcedure::getImmStepsRolling: Step %s found in state %d", getDn().c_str(), newStep->getState());

                newStep->setDn(newStep->getRdn() + "," + getDn());

		TRACE("Rolling upgrade method found");

                newStep->setMaxRetry(upgradeMethod->getStepMaxRetryCount());
                newStep->setRestartOption(upgradeMethod->getStepRestartOption());

                const SmfByTemplate *byTemplate = (const SmfByTemplate *)upgradeMethod->getUpgradeScope();
                const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();

                // The SW list can not be fetched from the SaSmfImageNodes objects since it lack information
                // about e.g. pathNamePrefix.
                // Fetch it from the campaign XML data.
                newStep->addSwRemove(nodeTemplate->getSwRemoveList());
                newStep->addSwAdd(nodeTemplate->getSwInstallList());

		rc = readCampaignImmModel(newStep);
		if (rc != SA_AIS_OK) {
			LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: Fail to read campaign IMM model");
			TRACE_LEAVE();
			return rc;
		}

                const std::list < SmfParentType * >&actUnitTemplates = nodeTemplate->getActivationUnitTemplateList();

                if (actUnitTemplates.size() == 0) {
                        if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_AMF_NODE, objInstances)){
                                LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: addStepModifications failed");
                                rc = SA_AIS_ERR_CAMPAIGN_PROC_FAILED;
                                goto done;
                        }
                } else {
                        if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_SU_COMP, objInstances)){
                                LOG_NO("SmfUpgradeProcedure::getImmStepsRolling: addStepModifications failed");
                                rc = SA_AIS_ERR_CAMPAIGN_PROC_FAILED;
                                goto done;
                        }
                }

		TRACE("Adding procedure step %s from IMM", newStep->getDn().c_str());
		addProcStep(newStep);
	}

 done:
	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// getImmStepsSingleStep()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::getImmStepsSingleStep()
{
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;
	std::list < std::string > stepList;

	TRACE_ENTER();

	SmfSinglestepUpgrade * upgradeMethod = (SmfSinglestepUpgrade *)getUpgradeMethod();
	if (upgradeMethod == NULL) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: no upgrade method found");
		TRACE_LEAVE();
		return SA_AIS_ERR_INIT;
	}

	getCallbackList(upgradeMethod);

	// Read the single step from IMM
	if (immutil.getChildren(getDn(), stepList, SA_IMM_SUBLEVEL, "SaSmfStep") == false) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Failed to get steps for procedure %s", getDn().c_str());
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	std::list < std::string >::iterator stepit;
	TRACE("Fetch IMM data for our upgrade procedure steps");

	/* Fetch IMM data for our upgrade procedure single step, only just one */
	stepit = stepList.begin();
	if (immutil.getObject((*stepit), &attributes) == false) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: IMM data for step %s not found", (*stepit).c_str());
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	TRACE("Fetch IMM data for item in stepList");

	SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
        osafassert(newStep != NULL);

	if (newStep->init((const SaImmAttrValuesT_2 **)attributes) != SA_AIS_OK) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Initialization failed for step %s", (*stepit).c_str());
		delete newStep;
		TRACE_LEAVE();
		return SA_AIS_ERR_INIT;
	}

	newStep->setDn((*stepit));
	newStep->setProcedure(this);
               
	TRACE("Step %s found in state %d", getDn().c_str(), newStep->getState());
	newStep->setDn(newStep->getRdn() + "," + getDn());

	TRACE("Single step upgrade method found");

	newStep->setMaxRetry(upgradeMethod->getStepMaxRetryCount());
	newStep->setRestartOption(upgradeMethod->getStepRestartOption());

	//---------------------------------------------
	// Read the software to install and remove
	// The SW list can not be fetched from the SaSmfImageNodes objects since it lack information
	// about e.g. pathNamePrefix.
	// Fetch it from the campaign XML data.
	//---------------------------------------------
	const SmfUpgradeScope* scope        = upgradeMethod->getUpgradeScope();
	const SmfForAddRemove* forAddRemove = dynamic_cast<const SmfForAddRemove*>(scope);
	const SmfForModify*    forModify    = dynamic_cast<const SmfForModify*>(scope);

        if ((forAddRemove == NULL)&&(forModify == NULL)) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Procedure scope not found (SmfForAddRemove/forModify)");
                delete newStep;
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
        }

	//----------------------------------------------------------
	// The step was calculated before the campaign was restarted
	// Do not regenerate, read the previously generated step 
	// informationfrom IMM.
	//----------------------------------------------------------
	SaAisErrorT rc = readCampaignImmModel(newStep);
	if (rc != SA_AIS_OK) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Fail to read campaign IMM model");
		TRACE_LEAVE();
		return rc;
	}

	if (forAddRemove != NULL) {
                //TODO:
                //forAddRemove/removed by Template is not implemented (same as calculateSingleStep())
		const SmfActivationUnitType* aunit;
		aunit = forAddRemove->getActivationUnit();
		newStep->addSwAdd(aunit->getSwAdd());
		aunit = forAddRemove->getDeactivationUnit();
		newStep->addSwRemove(aunit->getSwRemove());
	} else if (forModify != NULL) {
		const SmfActivationUnitType* aunit = forModify->getActivationUnit();
		newStep->addSwAdd(aunit->getSwAdd());
		newStep->addSwRemove(aunit->getSwRemove());
                //Add step modifications.
                //For simplicity reasons this is always made.
                //Needed only if the single step procedure swap the controllers before executing the step.
                //This is made if the affected nodes to reboot only affect one of the two controllers.
                std::multimap<std::string, objectInst> objInstances;
                if (!getImmComponentInfo(objInstances)) {
                        LOG_NO("SmfUpgradeProcedure::calculateSteps: Config info from IMM could not be read");
                        delete newStep;
                        TRACE_LEAVE();
                        return SA_AIS_ERR_INIT;
                }

                //Find out modifications from TargetEntityTemplates
                if (!addStepModifications(newStep, 
                                          forModify->getTargetEntityTemplate(),
                                          SMF_AU_SU_COMP,
                                          objInstances)) {
                        delete newStep;
                        TRACE_LEAVE();
                        return SA_AIS_ERR_INIT;
                }
	}

	TRACE("Adding procedure step %s from IMM", newStep->getDn().c_str());
	addProcStep(newStep);

	TRACE_LEAVE();
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// getImmStepsMergedSingleStep()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::getImmStepsMergedSingleStep()
{
        // This routine will never be executed unless the campaign procedures is merged into
        // a single step procedure i.e. the merge is configured in SMF.
        // If above is valid, there is only in two cases this routine will be executed
        // 1) after a si-swap, step state is SA_SMF_STEP_INITIAL
        // 2) after a cluster reboot, step state is SA_SMF_STEP_EXECUTING
        //
        // The IMM modifications are always made before the reboot. The only thing left to
        // do after the reboot is unlock DU, online remove of bundles and callbacks.
        // This infon is fetched from IMM and campaign without recalculation of steps.
        //
        // After a si-swap, the procedure was never started before the swap. Just do the merge
        // again on this new active SC.

	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;
	std::list < std::string > stepList;

	TRACE_ENTER();
 	SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep;
        osafassert(newStep != NULL);

	// Read the single step from IMM
	if (immutil.getChildren(getDn(), stepList, SA_IMM_SUBLEVEL, "SaSmfStep") == false) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Failed to get steps for procedure %s", getDn().c_str());
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	TRACE("Fetch IMM data for merged upgrade procedure step");
	std::list < std::string >::iterator stepit;
	/* Fetch IMM data for our upgrade procedure single step, only just one */
	stepit = stepList.begin();
	if (immutil.getObject((*stepit), &attributes) == false) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: IMM data for step %s not found", (*stepit).c_str());
		TRACE_LEAVE();
		return SA_AIS_ERR_NOT_EXIST;
	}

	TRACE("Copy step basic data from IMM into the new merged step.");
	if (newStep->init((const SaImmAttrValuesT_2 **)attributes) != SA_AIS_OK) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Initialization failed for step %s", (*stepit).c_str());
		delete newStep;
		TRACE_LEAVE();
		return SA_AIS_ERR_INIT;
	}

	if ((newStep->getState() != SA_SMF_STEP_INITIAL) && (newStep->getState() != SA_SMF_STEP_EXECUTING)) {
		LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Invalid state %d", newStep->getState());
                delete newStep;
                TRACE_LEAVE();
                return SA_AIS_ERR_INIT;
	}

        newStep->setDn((*stepit));
	newStep->setProcedure(this);

        if(newStep->getState() == SA_SMF_STEP_INITIAL) {
                mergeStepIntoSingleStep(this, newStep); //Just merge again, as before si-swap
		addProcStep(newStep);
        } else if(newStep->getState() == SA_SMF_STEP_EXECUTING) {
                //Fetch AU/DU and step swNode from IMM steps
                SaAisErrorT rc = readCampaignImmModel(newStep);
                if (rc != SA_AIS_OK) {
                        LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Fail to read campaign IMM model");
                        delete newStep;
                        TRACE_LEAVE();
                        return rc;
                }

		//Fetch bundle info from IMM steps, create SmfBundleRef and add pathnamePrefix to step
		rc = bundleRefFromSsCampaignImmModel(newStep);
                if (rc != SA_AIS_OK) {
                        LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Fail to create SmfBundleRef");
                        delete newStep;
                        TRACE_LEAVE();
                        return rc;
                }

		//Add the recreated step to the procedure
		addProcStep(newStep);

		//Fetch callbacks and procedure init/wraup actions.
		int initActionId = 1;
		int wrapupActionId = 1;
		SmfUpgradeCampaign * camp = SmfCampaignThread::instance()->campaign()->getUpgradeCampaign();
		const std::vector < SmfUpgradeProcedure * >& procedures = camp->getProcedures();
		std::vector < SmfUpgradeProcedure * >::const_iterator proc_iter;
		for (proc_iter = procedures.begin(); proc_iter != procedures.end(); proc_iter++) {
			LOG_NO("SmfUpgradeProcedure::getImmStepsMergedSingleStep: Fetch callbacks and wrapup actions from [%s]",
			       (*proc_iter)->getName().c_str());

			//The procedure init actions
			LOG_NO("Copy the procedure init actions");
			addInitActions((*proc_iter)->getInitActions());
			std::vector <SmfUpgradeAction*>::const_iterator actioniter;
			actioniter = getInitActions().begin();
			while (actioniter != getInitActions().end()) {
				//For the callback actions, set new calback procedure pointer
				const SmfCallbackAction* cbkAction =
					dynamic_cast<const SmfCallbackAction*>(*actioniter);
				if (cbkAction != NULL) {
					const_cast<SmfCallbackAction*>(cbkAction)->setCallbackProcedure(this);
				}

				//Renumber action id aviod DN name collision in the merged procedure rollback data
				(*actioniter)->changeId(initActionId);
				initActionId++;

				actioniter++;
			}

			//The procedure wrapup actions
			LOG_NO("Copy the procedure wrapup actions");
			addWrapupActions((*proc_iter)->getWrapupActions());
			actioniter = getWrapupActions().begin();
			while (actioniter != getWrapupActions().end()) {
				//For the callback actions, set new calback procedure pointer
				const SmfCallbackAction* cbkAction =
					dynamic_cast<const SmfCallbackAction*>(*actioniter);
				if (cbkAction != NULL) {
					const_cast<SmfCallbackAction*>(cbkAction)->setCallbackProcedure(this);
				}

				//Renumber action id aviod DN name collision in the merged procedure rollback data
				(*actioniter)->changeId(wrapupActionId);
				wrapupActionId++;

				actioniter++;
			}

			//The procedure step callbacks
			getCallbackList((*proc_iter)->getUpgradeMethod());
		}
        }

	TRACE_LEAVE();
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// readCampaignImmModel()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeProcedure::readCampaignImmModel(SmfUpgradeStep *i_newStep)
{
        TRACE_ENTER();
	std::list < std::string > auList;
	std::list < std::string > duList;
	std::list < std::string >::iterator stringit;
	unsigned int ix;
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;

	//---------------------------------------------
	// Read the SaSmfActivationUnit object from IMM
	//---------------------------------------------
	TRACE("Read the SaSmfActivationUnit object from IMM parent=%s", i_newStep->getDn().c_str());
	if (immutil.getChildren(i_newStep->getDn(), auList, SA_IMM_SUBLEVEL, "SaSmfActivationUnit") != false) {
		TRACE("SaSmfActivationUnit:Resulting list size=%zu", auList.size());

		//Continue only if there really is any SaSmfActivationUnit.
		//If there was no match for the types to operate on e.g. component or SU type, 
		//when the step was calculated, no SaSmfActivationUnit was created.
		if (auList.size() != 0) {

			/* Fetch IMM data for SaSmfAactivationUnit (should be max one)*/
			std::string activationUnit = (*auList.begin());
			if (immutil.getObject(activationUnit, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: IMM data for step activationUnit %s not found", activationUnit.c_str());
				TRACE_LEAVE();
				return SA_AIS_ERR_NOT_EXIST;
			}

			//For the SaAmfActivationUnit read the attribute saSmfAuActedOn and store it in step
			TRACE("For the SaAmfActivationUnit read the saSmfAuActedOn and store it in step");
			const SaNameT * au;
			for(ix = 0; (au = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
							      "saSmfAuActedOn", ix)) != NULL; ix++) {
				TRACE("addActivationUnit %s", osaf_extended_name_borrow(au));
				unitNameAndState tmp;
				tmp.name = osaf_extended_name_borrow(au);
                                tmp.initState = SA_AMF_ADMIN_UNLOCKED;
				if(tmp.name != "") {
					i_newStep->addActivationUnit(tmp);
				} else {
					TRACE("No activation unit, must be SW install");
				}
			}

			//For the SaAmfActivationUnit fetch the SaSmfImageNodes objects
			TRACE("For the SaAmfActivationUnit fetch the SaSmfImageNodes objects");
			std::list < std::string > imageNodesList;
			if (immutil.getChildren(activationUnit, imageNodesList, SA_IMM_SUBLEVEL, "SaSmfImageNodes") != false) {

				TRACE("Nr of SaSmfImageNodes found = %zu", imageNodesList.size());
				if (imageNodesList.size() > 0) {

					//Read the first SaSmfImageNodes. All bundles in the step are installed on the same node
					std::string imageNodes = (*imageNodesList.begin());
					TRACE("std::string imageNodes = %s", imageNodes.c_str());
					if (immutil.getObject(imageNodes, &attributes) == false) {
						LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: IMM data for ImageNodes %s not found", imageNodes.c_str());
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					// Read the saSmfINSwNode attribute

					//Rolling steps contain only one node
					//Single step may contain several nodes
					if (getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
						const SaNameT *saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINNode", 0);
						if (saSmfINNode == NULL) {
							LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: saSmfINNode does not exist");  
							TRACE_LEAVE();
							return SA_AIS_ERR_NOT_EXIST;
						}
						TRACE("Rolling saSmfINNode->value = %s", osaf_extended_name_borrow(saSmfINNode));
						i_newStep->setSwNode(osaf_extended_name_borrow(saSmfINNode));
					}  else {  //SA_SMF_SINGLE_STEP
						const SaNameT * saSmfINNode;
						for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
											       "saSmfINNode", ix)) != NULL; ix++) {
							TRACE("Single step saSmfINNode->value = %s (%u)",
                                                              osaf_extended_name_borrow(saSmfINNode), ix);

                                                        i_newStep->addSwNode(osaf_extended_name_borrow(saSmfINNode));
						}
						if ( ix == 0 ) {
							LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: saSmfINNode does not exist");  
							TRACE_LEAVE();
							return SA_AIS_ERR_NOT_EXIST;
						}
					}
				}
			}
		} //if (auList.size() != 0)
	}

	//---------------------------------------------
	// Read the SaSmfDeactivationUnit object from IMM
	//---------------------------------------------
	TRACE("Read the SaSmfDeactivationUnit object from IMM parent=%s", i_newStep->getDn().c_str());
	if (immutil.getChildren(i_newStep->getDn(), duList, SA_IMM_SUBLEVEL, "SaSmfDeactivationUnit") != false) {
		TRACE("SaSmfDeactivationUnit:Resulting list size=%zu", duList.size());

		//Continue only if there really is any SaSmfDeactivationUnit.
		//If there was no match for the types to operate on e.g. component or SU type, 
		//when the step was calculated, no SaSmfDeactivationUnit was created.
		if (duList.size() != 0) {

			/* Fetch IMM data for SaSmfDeactivationUnit (should be max one)*/
			std::string deactivationUnit = (*duList.begin());
			if (immutil.getObject(deactivationUnit, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: IMM data for step deactivationUnit %s not found", deactivationUnit.c_str());
				TRACE_LEAVE();
				return SA_AIS_ERR_NOT_EXIST;
			}

			//For the SaAmfDeactivationUnit read the attribute saSmfDuActedOn and store it in step
			TRACE("For the SaAmfDeactivationUnit read the saSmfDuActedOnand store it in step");
			const SaNameT * du;
			for(ix = 0; (du = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
							      "saSmfDuActedOn", ix)) != NULL; ix++) {
				TRACE("addDeactivationUnit %s", osaf_extended_name_borrow(du));
				unitNameAndState tmp;
				tmp.name = osaf_extended_name_borrow(du);
                                tmp.initState = SA_AMF_ADMIN_UNLOCKED;
				if(tmp.name != "") {
					i_newStep->addDeactivationUnit(tmp);
				} else {
					TRACE("No deactivation unit, must be SW remove");
				}
			}

			//For the SaAmfDeactivationUnit fetch the SaSmfImageNodes objects
			TRACE("For the SaAmfDeactivationUnit fetch the SaSmfImageNodes objects");
			std::list < std::string > imageNodesList;

			if (immutil.getChildren(deactivationUnit, imageNodesList, SA_IMM_SUBLEVEL, "SaSmfImageNodes") != false) {

				TRACE("Nr of SaSmfImageNodes found = %zu", imageNodesList.size());
				if (imageNodesList.size() > 0) {

					//Read the first SaSmfImageNodes. All bundles in the step are installed on the same node
					std::string imageNodes = (*imageNodesList.begin());
					TRACE("std::string imageNodes = %s", imageNodes.c_str());
					if (immutil.getObject(imageNodes, &attributes) == false) {
						LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: IMM data for ImageNodes %s not found", imageNodes.c_str());
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					// Read the saSmfINSwNode attribute
				
					//Rolling steps contain only one node
					//Single step may contain several nodes
					if (getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
						const SaNameT *saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINNode", 0);
						if (saSmfINNode == NULL) {
							LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: saSmfINNode does not exist");  
							TRACE_LEAVE();
							return SA_AIS_ERR_NOT_EXIST;
						}
						TRACE("Rolling saSmfINNode->value = %s", osaf_extended_name_borrow(saSmfINNode));
						i_newStep->setSwNode(osaf_extended_name_borrow(saSmfINNode));
					}  else {  //SA_SMF_SINGLE_STEP
						const SaNameT * saSmfINNode;
						for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
											       "saSmfINNode", ix)) != NULL; ix++) {
							TRACE("Single step saSmfINNode->value = %s (%u)", osaf_extended_name_borrow(saSmfINNode), ix);
							i_newStep->addSwNode(osaf_extended_name_borrow(saSmfINNode));
						}
						if ( ix == 0 ) {
							LOG_NO("SmfUpgradeProcedure::readCampaignImmModel: saSmfINNode does not exist");  
							TRACE_LEAVE();
							return SA_AIS_ERR_NOT_EXIST;
						}
                                        }
				}
			}
		} //if (duList.size() != 0)
	}

        //If single step procedure, the same node may have occured several times in different saSmfINNode
        //in the routine above. Nodes was added to the list by "addSwNode" call.
        //Remove duplicates from list
        if (getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
                i_newStep->removeSwNodeListDuplicates();
        }

	TRACE_LEAVE();
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// bundleRefFromSsCampaignImmModel()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel(SmfUpgradeStep *i_newStep)
{
        TRACE_ENTER();
	std::list < std::string > auList;
	std::list < std::string > duList;
	std::list < std::string >::iterator stringIt;
	unsigned int ix;
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;

        //This method create lists of SmfBundleRef from a single step campaign IMM model.
        //-From activationUnit the bundles to install are fetched.
        //-From deactivationUnit the bundles to remove are fetched.
        //-plmExecEnv are filled in from nodes found in the resp. SaSmfImageNodes
        //-pathNamePrefix is fetched from the parsed camaign.

	//----------------------------------------------------------------
	// Read the pathNamePrefix (SmfBundleRef) from the parsed campaign
	//----------------------------------------------------------------
	//Read all the bundles to add/remove from the parsed camaign
        SmfUpgradeCampaign * camp = SmfCampaignThread::instance()->campaign()->getUpgradeCampaign();
	const std::vector < SmfUpgradeProcedure * >& procedures = camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator proc_iter;
	std::list < SmfBundleRef > bundlesOldProcSS;
	std::list < SmfBundleRef *> bundlesOldProcRO;
	for (proc_iter = procedures.begin(); proc_iter != procedures.end(); proc_iter++) {
		if ((*proc_iter)->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) { //SA_SMF_ROLLING
			const SmfByTemplate *byTemplate = (SmfByTemplate*)(*proc_iter)->getUpgradeMethod()->getUpgradeScope();
			if (byTemplate != NULL) {
				bundlesOldProcRO.insert(bundlesOldProcRO.end(),
							byTemplate->getTargetNodeTemplate()->getSwInstallList().begin(),
							byTemplate->getTargetNodeTemplate()->getSwInstallList().end());

				bundlesOldProcRO.insert(bundlesOldProcRO.end(),
							byTemplate->getTargetNodeTemplate()->getSwRemoveList().begin(),
							byTemplate->getTargetNodeTemplate()->getSwRemoveList().end());
			} else {
				LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: Procedure scope not found (byTemplate)");
			}
		} else { //SA_SMF_SINGLE_STEP
			const SmfForAddRemove* addRemove = dynamic_cast<const SmfForAddRemove*>((*proc_iter)->getUpgradeMethod()->getUpgradeScope());
			const SmfForModify* modify = dynamic_cast<const SmfForModify*>((*proc_iter)->getUpgradeMethod()->getUpgradeScope());
			if (addRemove != NULL) {
				bundlesOldProcSS.insert(bundlesOldProcSS.end(),
							addRemove->getActivationUnit()->getSwAdd().begin(),
							addRemove->getActivationUnit()->getSwAdd().end());

				bundlesOldProcSS.insert(bundlesOldProcSS.end(),
							addRemove->getDeactivationUnit()->getSwRemove().begin(),
							addRemove->getDeactivationUnit()->getSwRemove().end());
			} else if (modify != NULL) {
				bundlesOldProcSS.insert(bundlesOldProcSS.end(),
							modify->getActivationUnit()->getSwAdd().begin(),
							modify->getActivationUnit()->getSwAdd().end());

				bundlesOldProcSS.insert(bundlesOldProcSS.end(),
							modify->getActivationUnit()->getSwRemove().begin(),
							modify->getActivationUnit()->getSwRemove().end());
			} else {
				LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: Procedure scope not found (forAddRemove/forModify)");
			}
		}
	}

	//---------------------------------------------
	// Read the SaSmfActivationUnit object from IMM
	//---------------------------------------------
	TRACE("Read the SaSmfActivationUnit object from IMM parent=%s", i_newStep->getDn().c_str());
	if (immutil.getChildren(i_newStep->getDn(), auList, SA_IMM_SUBLEVEL, "SaSmfActivationUnit") != false) {
                TRACE("SaSmfActivationUnit:Resulting list size=%zu", auList.size());

		//Continue if a SaSmfActivationUnit exist.
		if (auList.size() != 0) {

                        // Fetch IMM data for SaSmfAactivationUnit (should be max one)
                        std::string activationUnit = (*auList.begin());
                        if (immutil.getObject(activationUnit, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: IMM data for step activationUnit %s not found",
                                       activationUnit.c_str());
				TRACE_LEAVE();
				return SA_AIS_ERR_NOT_EXIST;
			}

			//For the SaAmfActivationUnit fetch the SaSmfImageNodes objects
			TRACE("For the SaAmfActivationUnit fetch the SaSmfImageNodes objects");
			std::list < std::string > imageNodesList;
			if (immutil.getChildren(activationUnit, imageNodesList, SA_IMM_SUBLEVEL, "SaSmfImageNodes") != false) {
				TRACE("Nr of SaSmfImageNodes found = %zu", imageNodesList.size());

				//For all SaSmfImageNodes.(Bundles may be installed on different nodes)
				for (stringIt = imageNodesList.begin(); stringIt != imageNodesList.end(); stringIt++) {
					//TRACE("std::string imageNodes = %s", (*stringIt).c_str());
					if (immutil.getObject((*stringIt), &attributes) == false) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: IMM data for ImageNodes %s not found",
						       (*stringIt).c_str());
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					// Read the saSmfINSwNode attribute, may contain several nodes
					// SaSmfImageNodes Single step may contain several nodes
					const SaNameT * saSmfINNode;
					const SaNameT * saSmfINSwBundle;
					SmfBundleRef tmpBundleRef; //addPlmExecEnv setBundleDn setPathNamePrefix
					saSmfINSwBundle = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINSwBundle", 0);
					tmpBundleRef.setBundleDn(osaf_extended_name_borrow(saSmfINSwBundle));

					for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
										       "saSmfINNode", ix)) != NULL; ix++) {
						TRACE("Single step saSmfINNode->value = %s (%u)",
						      osaf_extended_name_borrow(saSmfINNode), ix);

						SmfPlmExecEnv plm;
						std::string amfNode(osaf_extended_name_borrow(saSmfINNode));
						plm.setAmfNode(amfNode);
						//Add the new SmfPlmExecEnv into the bundle plvExecEnv list
						tmpBundleRef.addPlmExecEnv(plm);
					}

					if ( ix == 0 ) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: saSmfINNode does not exist");
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					//Search for pathnamePrefix search all the procedures in the parsed campaign
					//It is assumed the pathnamePrefix is common on all nodes for a spcific bundle name.
					std::list < SmfBundleRef >::iterator bundle_iter;        //For SS procedures
					std::list < SmfBundleRef * >::iterator bundlePtr_iter;   //For RO procedures

					bool pathNamePrefixFound = false;
					//Look in rolling procedures
					for(bundlePtr_iter = bundlesOldProcRO.begin(); bundlePtr_iter != bundlesOldProcRO.end(); bundlePtr_iter++) {
			                        if((*bundlePtr_iter)->getBundleDn() == tmpBundleRef.getBundleDn()) {
							tmpBundleRef.setPathNamePrefix((*bundlePtr_iter)->getPathNamePrefix());
							pathNamePrefixFound = true;
							LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: pathnamePrefix [%s] for bundle [%s] found", 
							       tmpBundleRef.getPathNamePrefix().c_str(), 
							       tmpBundleRef.getBundleDn().c_str());
							break;
		                                }
					}

					if (pathNamePrefixFound == false) {
						//If not found also look in single step procedures
						for(bundle_iter = bundlesOldProcSS.begin(); bundle_iter != bundlesOldProcSS.end(); bundle_iter++) {
							if((*bundle_iter).getBundleDn() == tmpBundleRef.getBundleDn()) {
								tmpBundleRef.setPathNamePrefix((*bundle_iter).getPathNamePrefix());
								pathNamePrefixFound = true;
								LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: pathnamePrefix [%s] for bundle [%s] found", 
								       tmpBundleRef.getPathNamePrefix().c_str(), 
								       tmpBundleRef.getBundleDn().c_str());
								break;
							}
						}
					}

					if ( pathNamePrefixFound == false ) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: pathNamePrefixFound not found");
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					//Add the new bundle to add to the step
					i_newStep->addSwAdd(tmpBundleRef);
				} //for
                        } //!= false
                } //if (auList.size() != 0)
        }

	//---------------------------------------------
	// Read the SaSmfDeactivationUnit object from IMM
	//---------------------------------------------
	TRACE("Read the SaSmfActivationUnit object from IMM parent=%s", i_newStep->getDn().c_str());
	if (immutil.getChildren(i_newStep->getDn(), duList, SA_IMM_SUBLEVEL, "SaSmfDeactivationUnit") != false) {
                TRACE("SaSmfDeactivationUnit:Resulting list size=%zu", duList.size());

		//Continue if a SaSmfDeactivationUnit exist.
		if (duList.size() != 0) {

                        // Fetch IMM data for SaSmfDeactivationUnit (should be max one)
                        std::string activationUnit = (*duList.begin());
                        if (immutil.getObject(activationUnit, &attributes) == false) {
				LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: IMM data for step activationUnit %s not found",
                                       activationUnit.c_str());
				TRACE_LEAVE();
				return SA_AIS_ERR_NOT_EXIST;
			}

			//For the SaSmfDeactivationUnit fetch the SaSmfImageNodes objects
			TRACE("For the SaSmfDeactivationUnit fetch the SaSmfImageNodes objects");
			std::list < std::string > imageNodesList;
			if (immutil.getChildren(activationUnit, imageNodesList, SA_IMM_SUBLEVEL, "SaSmfImageNodes") != false) {
				TRACE("Nr of SaSmfImageNodes found = %zu", imageNodesList.size());

				//For all SaSmfImageNodes.(Bundles may be installed on different nodes)
				for (stringIt = imageNodesList.begin(); stringIt != imageNodesList.end(); stringIt++) {
					//TRACE("std::string imageNodes = %s", (*stringIt).c_str());
					if (immutil.getObject((*stringIt), &attributes) == false) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: IMM data for ImageNodes %s not found",
						       (*stringIt).c_str());
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					//saSmfINSwBundle
					// Read the saSmfINSwNode attribute, may contain several nodes
					// SaSmfImageNodes Single step may contain several nodes
					const SaNameT * saSmfINNode;
					const SaNameT * saSmfINSwBundle;
					SmfBundleRef tmpBundleRef; //addPlmExecEnv setBundleDn setPathNamePrefix
					saSmfINSwBundle = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINSwBundle", 0);
					tmpBundleRef.setBundleDn(osaf_extended_name_borrow(saSmfINSwBundle));

					for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
										       "saSmfINNode", ix)) != NULL; ix++) {
						TRACE("Single step saSmfINNode->value = %s (%u)",
						      osaf_extended_name_borrow(saSmfINNode), ix);

						SmfPlmExecEnv plm;
						std::string amfNode(osaf_extended_name_borrow(saSmfINNode));
						plm.setAmfNode(amfNode);
						//Add the new SmfPlmExecEnv into the bundle plvExecEnv list
						tmpBundleRef.addPlmExecEnv(plm);
					}

					if ( ix == 0 ) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: saSmfINNode does not exist");
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					//Search for pathnamePrefix search all the procedures in the parsed campaign
					//It is assumed the pathnamePrefix is common on all nodes for a spcific bundle name.
					std::list < SmfBundleRef >::iterator bundle_iter;        //For SS procedures
					std::list < SmfBundleRef * >::iterator bundlePtr_iter;   //For RO procedures

					bool pathNamePrefixFound = false;
					//Look in rolling procedures
					for(bundlePtr_iter = bundlesOldProcRO.begin(); bundlePtr_iter != bundlesOldProcRO.end(); bundlePtr_iter++) {
			                        if((*bundlePtr_iter)->getBundleDn() == tmpBundleRef.getBundleDn()) {
							tmpBundleRef.setPathNamePrefix((*bundlePtr_iter)->getPathNamePrefix());
							pathNamePrefixFound = true;
		                                }
					}

					if (pathNamePrefixFound == false) {
						//If not found also look in single step procedures
						for(bundle_iter = bundlesOldProcSS.begin(); bundle_iter != bundlesOldProcSS.end(); bundle_iter++) {
							if((*bundle_iter).getBundleDn() == tmpBundleRef.getBundleDn()) {
								tmpBundleRef.setPathNamePrefix((*bundle_iter).getPathNamePrefix());
								pathNamePrefixFound = true;
							}
						}
					}

					if ( pathNamePrefixFound == false ) {
						LOG_NO("SmfUpgradeProcedure::bundleRefFromSsCampaignImmModel: pathNamePrefixFound not found");
						TRACE_LEAVE();
						return SA_AIS_ERR_NOT_EXIST;
					}

					//Add the new bundle to add to the step
					i_newStep->addSwRemove(tmpBundleRef);
				} //for
                        } //!= false
                } //if (duList.size() != 0)
        }

	TRACE_LEAVE();
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// setEntitiesToAddRemMod()
//------------------------------------------------------------------------------
bool
SmfUpgradeProcedure::setEntitiesToAddRemMod(SmfUpgradeStep * i_step, SmfImmAttribute* io_smfEntityToAddRemove)
{
        TRACE_ENTER();

        //Modifications shall be visible in both attrsaSmfDuEntityToRemove and attrsaSmfDuEntityToAdd
        std::list < SmfImmOperation * > modList;
        modList = i_step->getModifications();

        SmfImmCreateOperation* createOper;
        SmfImmDeleteOperation* deleteOper;
        SmfImmModifyOperation* modifyOper;

	if (modList.size() == 0) {
		/* A value must always be supplied to a cached runtime attribute */
		io_smfEntityToAddRemove->addValue("");
		TRACE_LEAVE();
		return true;
	}

        std::list < SmfImmOperation * >::iterator it;
        for (it = modList.begin(); it != modList.end(); ++it) {
                if ((createOper = dynamic_cast<SmfImmCreateOperation*>(*it)) != 0) {
                        SmfImmUtils immUtil;
                        //Get class decription
                        SaImmAttrDefinitionT_2** attrDefinitionsOut = NULL;
                        if (!immUtil.getClassDescription(createOper->getClassName(), &attrDefinitionsOut)) {
				LOG_NO("SmfUpgradeProcedure::setEntitiesToAddRemMod: getClassDescription FAILED for [%s]", createOper->getClassName().c_str());
				return false;
			}
			if (attrDefinitionsOut == NULL) {
				LOG_NO("SmfUpgradeProcedure::setEntitiesToAddRemMod: Could not get attributes for class [%s]", createOper->getClassName().c_str());
				return false;
			}

                        //Look in SaImmAttrDefinitionT_2 for an attribute with the SA_IMM_ATTR_RDN flag set
                        std::string rdnAttr;
                        for (int i = 0; attrDefinitionsOut[i] != 0; i++) {
                                SaImmAttrFlagsT flags = attrDefinitionsOut[i]->attrFlags;
                                if ((flags & SA_IMM_ATTR_RDN) == SA_IMM_ATTR_RDN) {
                                        rdnAttr = (char*)attrDefinitionsOut[i]->attrName;
                                        break;
                                }
                        }

                        immUtil.classDescriptionMemoryFree(attrDefinitionsOut);

                        //Find attribute name in SmfImmCreateOperation attributes
                        const std::list < SmfImmAttribute > values = createOper->getValues();
                        if ( values.size() != 0 ) {
                                std::list < SmfImmAttribute >::const_iterator it;
                                for (it = values.begin(); it != values.end(); ++it) {
                                        if (((SmfImmAttribute)(*it)).getName() == rdnAttr) {
                                                std::string rdn = ((SmfImmAttribute)(*it)).getValues().front(); //RDN always one value
                                                std::string str = rdn + "," + createOper->getParentDn();
                                                io_smfEntityToAddRemove->addValue(rdn + "," + createOper->getParentDn());
                                                break;
                                        }
                                }
                        } else {
                                LOG_NO("SmfUpgradeProcedure::setEntitiesToAddRemMod: no values set in create operation for instance of class %s",createOper->getClassName().c_str());
                                return false;
                        }
                } else if ((deleteOper = dynamic_cast<SmfImmDeleteOperation*>(*it)) != 0) {
                        io_smfEntityToAddRemove->addValue(deleteOper->getDn());
                } else if ((modifyOper = dynamic_cast<SmfImmModifyOperation*>(*it)) != 0) {
                        io_smfEntityToAddRemove->addValue(modifyOper->getDn());
                } else {
                        LOG_NO("SmfUpgradeProcedure::setEntitiesToAddRemMod: unknown operation");
                        return false;
                }
        }

        TRACE_LEAVE();

        return true;
}

//------------------------------------------------------------------------------
// isCompRestartable()
//------------------------------------------------------------------------------
bool SmfUpgradeProcedure::isCompRestartable(const std::string &i_compDN)
{
	TRACE_ENTER();

	bool rc = true;
	SaImmAttrValuesT_2 **attributes;
	bool instanceCompDisableRestartIsSet = false;
	SaBoolT instanceCompDisableRestart;
	bool instanceCtDefDisableRestartIsSet = false;
	SaBoolT instanceCtDefDisableRestart;

	SmfImmUtils immUtil;

	//Read the saAmfCompDisableRestart attribute of the component instance
	if (immUtil.getObject(i_compDN.c_str(), &attributes) != true) {
		LOG_NO("SmfUpgradeProcedure::isCompRestartable: Fails to get object %s", i_compDN.c_str());  
		TRACE_LEAVE();
		return false; //Must return here because goto can not cross the variable def below
	}

	const SaBoolT *compDisableRestart =
		(SaBoolT*)immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
						"saAmfCompDisableRestart", 0);
	if(compDisableRestart != NULL) {
		instanceCompDisableRestart = *compDisableRestart;
		instanceCompDisableRestartIsSet = true;
	} else {
		//No info in the instance
		//Read the saAmfCtDefDisableRestart attribute of the component versioned type
		const SaNameT *saAmfCompType =
			immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
					    "saAmfCompType", 0);
		if(saAmfCompType == NULL){
			LOG_NO("SmfUpgradeProcedure::isCompRestartable: Can not read attr saAmfCompType in object %s", i_compDN.c_str());
			rc = false;
			goto done;
		}

		if (immUtil.getObject(osaf_extended_name_borrow(saAmfCompType), &attributes) == false) {
			LOG_NO("SmfUpgradeProcedure::isCompRestartable: Can not find object %s", osaf_extended_name_borrow(saAmfCompType));
			rc = false;
			goto done;
		}

		//Read the saAmfCtDefDisableRestart attribute from type object
		const SaBoolT *ctDefDisableRestart =
			(SaBoolT *)immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes,
							 "saAmfCtDefDisableRestart", 0);

		if(ctDefDisableRestart != NULL) {
			instanceCtDefDisableRestart      = *ctDefDisableRestart;
			instanceCtDefDisableRestartIsSet = true;
		}
	}

	//Evaluate the component restart information found above
	if (instanceCompDisableRestartIsSet == false){
		//No info in instance, check if component type saAmfCtDefDisableRestart is set in base class
		if ((instanceCtDefDisableRestartIsSet == true) &&
		    (instanceCtDefDisableRestart == SA_TRUE)){ //Types says non restartable
			TRACE("saSmfStepRestartOption is set to true(1), but the component %s is not restartable according to base type information", i_compDN.c_str()); 
			rc = false;
			goto done;
		}
	} else if (instanceCompDisableRestart == SA_TRUE) {     //Instance says non restartable
		TRACE("saSmfStepRestartOption is set to true(1), but the component %s is not restartable according to instance information", i_compDN.c_str()); 
		rc = false;
		goto done;
	}

done:
	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// getActDeactUnitsAndNodes()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::getActDeactUnitsAndNodes(const std::string &i_dn, std::string& io_unit,
					      std::string& io_node,
					      std::multimap<std::string, objectInst> &i_objects)
{
	TRACE_ENTER();
	//Find out what type of object the actedOn points to
	bool rc = true;

	if (i_dn.find("safSu=") == 0) {
		io_node = getNodeForCompSu(i_dn, i_objects);
		io_unit = i_dn;
	} else if (i_dn.find("safComp=") == 0) {
		io_node = getNodeForCompSu(i_dn, i_objects);
		if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
			io_unit = i_dn.substr(i_dn.find(',') + 1, std::string::npos);
			TRACE("The stepRestartOption was set to false(0), use parent %s, as act/deactComponent", io_unit.c_str());
		} else { // saSmfStepRestartOption is set to true
			//Check if component is restartable
			if (isCompRestartable(i_dn) == false) {
				LOG_NO("SmfUpgradeProcedure::getActDeactUnitsAndNodes: Component %s is not restartable", i_dn.c_str());
				rc =  false;
				goto done;
			}
			io_unit = i_dn;
		}
	} else if (i_dn.find("safAmfNode=") == 0) {
		io_node = i_dn;
		io_unit = i_dn;
	} else {
		LOG_NO("SmfUpgradeProcedure::getActDeactUnitsAndNodes: Wrong class (DN=%s), can only handle SaAmfSU,SaAmfComp and SaAmfNode", i_dn.c_str());
		rc =  false;
		goto done;
	}
done:
	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// getImmComponentInfo()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::getImmComponentInfo(std::multimap<std::string, objectInst> &i_objects)
{
	TRACE_ENTER();
	bool rc = true;
	SmfImmUtils immUtil;
	std::multimap<std::string, objectInst> objInstances;
	SaImmAttrValuesT_2 **attributes;
	SaImmSearchHandleT immSearchHandle;
	SaNameT objectName;

	SaImmAttrNameT attributeNames[] = {
		(char*)"saAmfCompType",
                NULL
        };

	if (immUtil.getChildrenAndAttrBySearchHandle("", immSearchHandle, SA_IMM_SUBTREE, (SaImmAttrNameT*)attributeNames, "SaAmfComp") == false) {
		LOG_NO("SmfUpgradeProcedure::getImmComponentInfo: getChildrenAndAttrBySearchHandle fail");
		rc = false;
		goto done_without_finalize;   //The immSearchHandle is already finalized by getChildrenAndAttrBySearchHandle method
	}

	while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
		const SaNameT *typeRef;
		std::string comp(osaf_extended_name_borrow(&objectName));

		typeRef = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfCompType", 0);
		if (typeRef == NULL) {
			LOG_NO("SmfUpgradeProcedure::getImmComponentInfo: Could not get attr name saAmfCompType");
			rc = false;
			goto done;	
		}

		std::string compType(osaf_extended_name_borrow(typeRef));
		std::string su(comp.substr(comp.find(',') + 1, std::string::npos));
		std::string sg(su.substr(su.find(',') + 1, std::string::npos));

                //The attribute hostedByNode may not be set by AMF yet
                //SMFD tries to read the attribute every 5 seconds until set
                //Times out after time configured as reboot timeout
                int interval = 5; //seconds
                int timeout = smfd_cb->rebootTimeout/1000000000; //seconds
                SaNameT *hostedByNode = 0;
                bool nodeEmpty = false;  //Used to control log printout
                while(true) {
                        if (immUtil.getObject(su, &attributes) == false) {
                                LOG_NO("SmfUpgradeProcedure::getImmComponentInfo: Could not get object %s", su.c_str());
                                rc = false;
                                goto done;
                        }

                        hostedByNode = (SaNameT *)immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                      "saAmfSUHostedByNode",
                                                                      0);
                        if (hostedByNode != NULL) {
                                if (nodeEmpty == true) {
                                        LOG_NO("SmfUpgradeProcedure::getImmComponentInfo:saAmfSUHostedByNode attribute in %s is now set, continue", su.c_str());
                                }
                                break;
                        }

                        if(timeout <= 0) { //Timeout
                                LOG_NO("SmfUpgradeProcedure::getImmComponentInfo: No saAmfSUHostedByNode attribute was set for %s, timeout", su.c_str());
                                rc = false;
                                goto done;
                        }

                        if (nodeEmpty == false) {
                                LOG_NO("SmfUpgradeProcedure::getImmComponentInfo:saAmfSUHostedByNode attribute in %s is empty, waiting for it to be set", su.c_str());
                                nodeEmpty = true;
                        }

                        sleep(interval);
                        timeout -= interval;
                        //No hostedByNode was set, read the same object again
                } //End  while(true)

                std::string node(osaf_extended_name_borrow(hostedByNode));
                typeRef = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfSUType", 0);
                std::string suType(osaf_extended_name_borrow(typeRef));

                //Save result in a multimap
                //Node as key
                i_objects.insert(std::pair<std::string, objectInst>(node, objectInst(node, 
                                                                                     sg, 
                                                                                     su, 
                                                                                     suType,
                                                                                     comp,
                                                                                     compType)));
	}
done:
        (void) immutil_saImmOmSearchFinalize(immSearchHandle);

done_without_finalize:
	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::execute()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->execute(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// executeStep()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::executeStep()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->executeStep(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// rollbackStep()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::rollbackStep()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->rollbackStep(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }


	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::suspend()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->suspend(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }


	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::rollback()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->rollback(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
SmfProcResultT 
SmfUpgradeProcedure::commit()
{
	TRACE_ENTER();

        SmfProcResultT procResult;

        while (1) {
                procResult = m_state->commit(this);

                if (procResult != SMF_PROC_CONTINUE) {
                        break;
                }
        }

	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// resetProcCounter()
//------------------------------------------------------------------------------
void
SmfUpgradeProcedure::resetProcCounter()
{
	TRACE_ENTER();
	SmfUpgradeProcedure::s_procCounter = 1;
	TRACE_LEAVE();
}

/*====================================================================*/
/*  Class SmfSwapThread                                               */
/*====================================================================*/

/*====================================================================*/
/*  Static methods                                                    */
/*====================================================================*/

/** 
 * SmfSmfSwapThread::main
 * static main for the thread
 */
void
SmfSwapThread::main(NCSCONTEXT info)
{
	SmfSwapThread *self = (SmfSwapThread *) info;
	self->main();
	TRACE("Swap thread exits");
	delete self;
}

/*====================================================================*/
/*  Methods                                                           */
/*====================================================================*/

/** 
 * Constructor
 */
SmfSwapThread::SmfSwapThread(SmfUpgradeProcedure * i_proc):
	m_task_hdl(0),
	m_proc(i_proc)
{
	sem_init(&m_semaphore, 0, 0);
}

/** 
 * Destructor
 */
SmfSwapThread::~SmfSwapThread()
{
}

/**
 * SmfSwapThread::start
 * Start the SmfSwapThread.
 */
int
SmfSwapThread::start(void)
{
	TRACE_ENTER();
	uint32_t rc;

	/* Create the task */
	int policy = SCHED_OTHER; /*root defaults */
	int prio_val = sched_get_priority_min(policy);
	
	if ((rc =
	     m_NCS_TASK_CREATE((NCS_OS_CB) SmfSwapThread::main, (NCSCONTEXT) this, (char *)"OSAF_SMF_UPGRADE_PROC",
			       prio_val, policy,  m_PROCEDURE_STACKSIZE, &m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfSwapThread::start: TASK_CREATE_FAILED");
		return -1;
	}
	if ((rc =m_NCS_TASK_DETACH(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfSwapThread::start: TASK_START_DETACH\n");
		return -1;
	}

	if ((rc = m_NCS_TASK_START(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_NO("SmfSwapThread::start: TASK_START_FAILED\n");
		return -1;
	}

	/* Wait for the thread to start */
	while((sem_wait(&m_semaphore) == -1) && (errno == EINTR))
               continue;       /* Restart if interrupted by handler */

	TRACE_LEAVE();
	return 0;
}

/**
 * SmfSwapThread::main
 * main for the thread.
 */
void
SmfSwapThread::main(void)
{
	TRACE_ENTER();
	sem_post(&m_semaphore);          //Start method waits for thread to start
	std::string si_name = smfd_cb->smfSiSwapSiName;
	int max_swap_retry = smfd_cb->smfSiSwapMaxRetry; //Number of retries before giving up
	int retryCnt = 0;
        int termCnt;
	SmfAdminOperationAction admOp(1);
	admOp.setDoDn(si_name);
	admOp.setDoId(SA_AMF_ADMIN_SI_SWAP);
	int rc = admOp.execute(0);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) ||
              (rc == SA_AIS_ERR_BUSY) ||
              (rc == SA_AIS_ERR_FAILED_OPERATION)) {

                if (retryCnt > max_swap_retry) {
                        LOG_NO("SA_AMF_ADMIN_SI_SWAP giving up after %d retries", retryCnt);
                        goto exit_error;
                }

                if (rc == SA_AIS_ERR_FAILED_OPERATION) {
                        //A failed operation occur. It is undefined if the operation was successful or not.
                        //We wait for maximum two minutes to see if the campaign thread is terminated (which it is in a successful swap)
                        //If not terminated, retry the SWAP operation.
                        LOG_NO("SA_AMF_ADMIN_SI_SWAP return  SA_AIS_ERR_FAILED_OPERATION [%d]. Wait for SmfCampaignThread to die, if not retry", rc);
                        termCnt = 0;
                        while (SmfCampaignThread::instance() != NULL) {
                                if(termCnt >= 60) { //Wait for max 2 minutes (60 * 2 sec)
                                        LOG_NO("Campaign thread was not terminated within 120 seconds after SA_AMF_ADMIN_SI_SWAP, retry SWAP operation");
                                        break;
                                }
                                struct timespec sleepTime = { 2, 0 };
                                osaf_nanosleep(&sleepTime);
                                termCnt++;
                        }
                	goto exit_error;
			
                } else { //SA_AIS_ERR_TRY_AGAIN or SA_AIS_ERR_BUSY
                        LOG_NO("SA_AMF_ADMIN_SI_SWAP return SA_AIS_ERR_TRY_AGAIN or SA_AIS_ERR_BUSY [%d], wait 2 seconds and retry", rc);
                        struct timespec sleepTime = { 2, 0 };
                        osaf_nanosleep(&sleepTime);
                }

                retryCnt++;
                rc = admOp.execute(0);
        }

        if (rc != SA_AIS_OK) {
                //SA_AIS_ERR_LIBRARY, SA_AIS_ERR_BAD,_HANDLE SA_AIS_ERR_INIT, SA_AIS_ERR,_INVALID_PARAM, SA_AIS_ERR_NO_MEMORY
                //SA_AIS_ERR_NO_RESOURCES, SA_AIS_ERR_BAD_OPERATION, SA_AIS_ERR_NOT_EXIST, SA_AIS_ERR_EXIST, SA_AIS_ERR_UNAVAILABLE
		//SA_AIS_ERR_TIMEOUT
                LOG_NO("Admin op SA_AMF_ADMIN_SI_SWAP fail [rc = %d]", rc);
                goto exit_error;
        }

        LOG_NO("SA_AMF_ADMIN_SI_SWAP [rc=%d] successfully initiated", rc);

        //Wait for the campaign thread to disappear on current node after swap
        termCnt = 0;
        while (SmfCampaignThread::instance() != NULL) {
                if(termCnt >= 60) { //Wait for 2 minutes (60 * 2 sec)
                        LOG_NO("Campaign thread does not disappear within 120 seconds after SA_AMF_ADMIN_SI_SWAP, the operation was assumed failed.");
                        goto exit_error;
                }
                struct timespec sleepTime = { 2, 0 };
                osaf_nanosleep(&sleepTime);
                termCnt++;
        }

        LOG_NO("Campaign thread terminated after SA_AMF_ADMIN_SI_SWAP");
        return;
	TRACE_LEAVE();

exit_error:
        if (SmfCampaignThread::instance() != NULL) {
                SmfProcStateExecFailed::instance()->changeState(m_proc, SmfProcStateExecFailed::instance());
        }

        if (SmfCampaignThread::instance() != NULL) {
                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                evt->event.procResult.rc = SMF_PROC_FAILED;
                evt->event.procResult.procedure = m_proc;
                SmfCampaignThread::instance()->send(evt);
        }
        TRACE_LEAVE();
        return;
}
