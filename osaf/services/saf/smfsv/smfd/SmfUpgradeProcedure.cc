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

#include <saAmf.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>
#include <saf_error.h>

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
    m_afterUnlock(0)
{

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

	std::vector < SmfUpgradeAction * >::iterator it;

	/* Delete procedure initialization */
	for (it = m_procInitAction.begin(); it != m_procInitAction.end(); ++it) {
		delete(*it);
	}

	/* Delete procedure wrapup */
	for (it = m_procWrapupAction.begin(); it != m_procWrapupAction.end(); ++it) {
		delete(*it);
	}

	std::vector < SmfUpgradeStep * >::iterator stepit;

	/* Delete upgrade steps */
	for (stepit = m_procSteps.begin(); stepit != m_procSteps.end(); ++stepit) {
		delete(*stepit);
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
	std::multimap<std::string, objectInst> objInstances;

	if (!getImmComponentInfo(objInstances)) {
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
			if ( !calculateRollingSteps((SmfRollingUpgrade *)upgradeMethod, objInstances)) {
                                LOG_NO("SmfUpgradeProcedure::calculateSteps:calculateRollingSteps failed");
                                return false;
                        }
			break;
		}

	case SA_SMF_SINGLE_STEP:
		{
			if ( !calculateSingleStep((SmfSinglestepUpgrade*)upgradeMethod, objInstances)) {
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

			SmfUpgradeStep *newStep = new SmfUpgradeStep();
			newStep->setRdn(rdnStr);
			newStep->setDn(newStep->getRdn() + "," + getDn());
			newStep->addActivationUnit(*it);
			newStep->addDeactivationUnit(*it);
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

                        SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep();
                        osafassert(newStep != NULL);
                        newStep->setRdn(rdnStr);
                        newStep->setDn(newStep->getRdn() + "," + getDn());
                        newStep->addActivationUnit(*itActDeact);
                        newStep->addDeactivationUnit(*itActDeact);
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

			SmfUpgradeStep *newStep = new SmfUpgradeStep();
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
	SmfUpgradeStep *newStep = new SmfUpgradeStep();
	const SmfUpgradeScope* scope = i_upgrade->getUpgradeScope();
	const SmfForAddRemove* forAddRemove = 
		dynamic_cast<const SmfForAddRemove*>(scope);

	newStep->setRdn("safSmfStep=1");
	newStep->setDn(newStep->getRdn() + "," + getDn());
	newStep->setMaxRetry(i_upgrade->getStepMaxRetryCount());
	newStep->setRestartOption(i_upgrade->getStepRestartOption());

	getCallbackList(i_upgrade);

	if (forAddRemove != NULL) {

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
					newStep->addActivationUnit(*a);
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
			newStep->addActivationUnit(*entity);
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
					newStep->addDeactivationUnit(*a);
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
			newStep->addDeactivationUnit(*entity);
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
			newStep->addDeactivationUnit(*a);
			newStep->addActivationUnit(*a);
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
					amfNodeDn.append((char *)amfNode->value, amfNode->length);
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
			bool removeDuplicates = false;
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
									removeDuplicates = true;  //Duplicates must be removed from list when the loop is finished
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
							removeDuplicates = true;  //Duplicates must be removed from list when the loop is finished
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

			if (removeDuplicates == true){
				TRACE("Sort the act/deact unit list");
				o_actDeactUnits.sort();
				o_actDeactUnits.unique();
			}
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

		//Since the list is on component level there may have been several hits resulting in the same SU and node
		//Make the SU list unique
		o_actDeactUnits.sort();
		o_actDeactUnits.unique();

		//Make the Node list unique
		if (o_nodeList != NULL) {
			o_nodeList->sort();
			o_nodeList->unique();
		}
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
	const std::string & auNodeName = i_newStep->getActivationUnitList().front();
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
		std::list < std::string >::const_iterator auEntityIt;
		const std::list < std::string > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
		for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
			if (!addStepModificationList(i_newStep, *auEntityIt, i_modificationList)) {
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
        std::list < std::string >::const_iterator auEntityIt;
        const std::list < std::string > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
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
				if((*auEntityIt).find("safAmfNode=") == 0) { //Node as AU/DU
					if((*objit).second.nodeDN == (*auEntityIt)) {
						TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
						matchingSU.push_back((*objit).second.suDN);
						break;
					}
				}

				if((*objit).second.suDN == (*auEntityIt)) {
					TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
					//The i_objects map contain all instances on component level.
					//One SU may give multiple matches since one SU may contain several components.
					//Save the matching enties and remove duplicates when all matching is finished.
					matchingSU.push_back((*objit).second.suDN);
					break;
				} else {
					TRACE("Instance %s is NOT within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
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
        std::list < std::string >::const_iterator auEntityIt;
        const std::list < std::string > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
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
				if((*auEntityIt).find("safAmfNode=") == 0) { //Node as AU/DU
					if((*objit).second.nodeDN == (*auEntityIt)) {
						TRACE("Instance %s is within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).c_str());
						if (!addStepModificationList(i_newStep, (*objit).second.compDN, i_modificationList)) {
							LOG_NO("SmfUpgradeProcedure::addStepModificationsComp: addStepModificationList fails");
							TRACE_LEAVE();
							return false;
						}
					} else {
						TRACE("Instance %s is NOT within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).c_str());
					}
				}

				//If the AU/DU DN is found within the Component DN, the component is within the scope
				if(((*objit).second.compDN).find(*auEntityIt) != std::string::npos) {
					TRACE("Instance %s is within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).c_str());
					if (!addStepModificationList(i_newStep, (*objit).second.compDN, i_modificationList)) {
						LOG_NO("SmfUpgradeProcedure::addStepModificationsComp: addStepModificationList fails");
						TRACE_LEAVE();
						return false;
					}
				} else {
					TRACE("Instance %s is NOT within scope of %s", (*objit).second.compDN.c_str(), (*auEntityIt).c_str());
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
        std::list < std::string >::const_iterator auEntityIt;
        const std::list < std::string > &auEntityList = i_newStep->getActivationUnitList(); //Single step may have many activation units
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
			if((*auEntityIt).find("safAmfNode=") == 0) { //Node as AU/DU
				if((*objit).second.nodeDN == (*auEntityIt)) {
					TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
					//The i_objects map contain all instances on component level.
					//One SU may give multiple matches since one SU may contain several components.
					//Save the matching enties and remove duplicates when all matching is finished.
					matchingSU.push_back((*objit).second.suDN);
					break;
				}
			}

			if((*objit).second.suDN == (*auEntityIt)) {
				TRACE("Instance %s is within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
				//The i_objects map contain all instances on component level.
				//One SU may give multiple matches since one SU may contain several components.
				//Save the matching enties and remove duplicates when all matching is finished.
				matchingSU.push_back((*objit).second.suDN);
				break;
			} else {
				TRACE("Instance %s is NOT within scope of %s", (*objit).second.suDN.c_str(), (*auEntityIt).c_str());
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

	std::list < std::string >::const_iterator iter;
	std::list < std::string >::const_iterator iterE;

	/* Create the SaSmfDeactivationUnit object if there ia any object to deactivate */
	const std::list < std::string >& deactList = i_step->getDeactivationUnitList();
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
			attrsaSmfDuActedOn.addValue(*iter);
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
	const std::list < std::string >& actList = i_step->getActivationUnitList();
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
		const std::list < std::string > actList = i_step->getActivationUnitList();
		iter = actList.begin();
		iterE = actList.end();
		while (iter != iterE) {
			attrsaSmfAuActedOn.addValue(*iter);
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
		TRACE("Single step upgrade");
		rc = getImmStepsSingleStep();
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

		SmfUpgradeStep *newStep = new SmfUpgradeStep();

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
                
                TRACE("Step %s found in state %d", getDn().c_str(), newStep->getState());

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
			LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Fail to read campaign IMM model");
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

	SmfUpgradeStep *newStep = new SmfUpgradeStep();

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
	const SmfUpgradeScope* scope        = upgradeMethod->getUpgradeScope(); ;
	const SmfForAddRemove* forAddRemove = dynamic_cast<const SmfForAddRemove*>(scope);
	const SmfForModify*    forModify    = dynamic_cast<const SmfForModify*>(scope);

	if (forAddRemove != NULL) {
		const SmfActivationUnitType* aunit;
		aunit = forAddRemove->getActivationUnit();
		newStep->addSwAdd(aunit->getSwAdd());
		aunit = forAddRemove->getDeactivationUnit();
		newStep->addSwRemove(aunit->getSwRemove());
	} else if (forModify != NULL) {
		const SmfActivationUnitType* aunit = forModify->getActivationUnit();
		newStep->addSwAdd(aunit->getSwAdd());
		newStep->addSwRemove(aunit->getSwRemove());
	} else {
		LOG_NO("SmfUpgradeProcedure::getImmStepsSingleStep: Procedure scope not found (SmfForAddRemove/forModify)");
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

	TRACE("Adding procedure step %s from IMM", newStep->getDn().c_str());
	addProcStep(newStep);

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
				TRACE("addActivationUnit %s", (char *)au->value);
				std::string str = (char *)au->value;
				if(str != "") {
					i_newStep->addActivationUnit((char *)au->value);
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
						TRACE("Rolling saSmfINNode->value = %s", (char*)saSmfINNode->value);
						i_newStep->setSwNode((char*)saSmfINNode->value);
					}  else {  //SA_SMF_SINGLE_STEP
						const SaNameT * saSmfINNode;
						for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
											       "saSmfINNode", ix)) != NULL; ix++) {
							TRACE("Single step saSmfINNode->value = %s (%u)", (char*)saSmfINNode->value, ix);
							i_newStep->addSwNode((char*)saSmfINNode->value);
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
				TRACE("addDeactivationUnit %s", (char *)du->value);
				std::string str = (char *)du->value;
				if(str != "") {
					i_newStep->addDeactivationUnit((char *)du->value);
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
						TRACE("Rolling saSmfINNode->value = %s", (char*)saSmfINNode->value);
						i_newStep->setSwNode((char*)saSmfINNode->value);
					}  else {  //SA_SMF_SINGLE_STEP
						const SaNameT * saSmfINNode;
						for(ix = 0; (saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
											       "saSmfINNode", ix)) != NULL; ix++) {
							TRACE("Single step saSmfINNode->value = %s (%u)", (char*)saSmfINNode->value, ix);
							i_newStep->addSwNode((char*)saSmfINNode->value);
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

		if (immUtil.getObject((char *)saAmfCompType->value, &attributes) == false) {
			LOG_NO("SmfUpgradeProcedure::isCompRestartable: Can not find object %s", (char *)saAmfCompType->value);
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
// execute()
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
		std::string comp((char *)objectName.value);

		typeRef = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfCompType", 0);
		if (typeRef == NULL) {
			LOG_NO("SmfUpgradeProcedure::getImmComponentInfo: Could not get attr name saAmfCompType");
			rc = false;
			goto done;	
		}

		std::string compType((char *)typeRef->value);
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

                std::string node((char *)hostedByNode->value);
                typeRef = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfSUType", 0);
                std::string suType((char *)typeRef->value);

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
	SmfAdminOperationAction admOp(1);
	std::string si_name = smfd_cb->smfSiSwapSiName;
	admOp.setDoDn(si_name);
	admOp.setDoId(SA_AMF_ADMIN_SI_SWAP);
	int max_swap_retry = smfd_cb->smfSiSwapMaxRetry;
	int retryCnt = 0;
	int rc;
	while((rc = admOp.execute(0)) != SA_AIS_OK) {
		retryCnt++;
		if(retryCnt > max_swap_retry) {
			SmfProcStateExecFailed::instance()->changeState(m_proc, SmfProcStateExecFailed::instance());

			LOG_NO("SmfSwapThread::main: SA_AMF_ADMIN_SI_SWAP giving up after %d retries", retryCnt);
			CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
			evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
			evt->event.procResult.rc = SMF_PROC_FAILED;
			evt->event.procResult.procedure = m_proc;
			SmfCampaignThread::instance()->send(evt);
			break;
		}
		TRACE("SI_AMF_ADMIN_SI_SWAP, rc=%d, wait 2 seconds and retry", rc);
		sleep(2);
	}

	TRACE_LEAVE();
}
