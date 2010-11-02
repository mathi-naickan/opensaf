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
#include <assert.h>

#include <poll.h>

#include <ncssysf_def.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tsk.h>

#include <saAmf.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>

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
    m_procedureThread(0), 
    m_name(""),
    m_time(0),
    m_execLevel(0), 
    m_dn(""),
    m_upgradeMethod(0),
    m_procInitAction(0), 
    m_procWrapupAction(0)
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
			LOG_ER("invalid number of values %u for %s", (*attribute)->attrValuesNumber,
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
				LOG_ER("invalid proc state %u", state);
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

	case SA_SMF_PROC_FAILED:
		{
			m_state = SmfProcStateExecFailed::instance();
			break;
		}

#if 0
		SA_SMF_PROC_STEP_UNDONE = 5, SA_SMF_PROC_ROLLING_BACK = 7, SA_SMF_PROC_ROLLBACK_SUSPENDED =
		    8, SA_SMF_PROC_ROLLED_BACK = 9, SA_SMF_PROC_ROLLBACK_FAILED = 10
#endif
	default:
		{
			LOG_ER("unknown state");
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
	m_procState = i_state;
	getProcThread()->updateImmAttr(this->getDn().c_str(), (char*)"saSmfProcState", SA_IMM_ATTR_SAUINT32T, &m_procState);
	SmfCampaignThread::instance()->sendStateNotification(m_dn, MINOR_ID_PROCEDURE, SA_NTF_MANAGEMENT_OPERATION,
							     SA_SMF_PROCEDURE_STATE, i_state);
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
	SmfUpgradeMethod *upgradeMethod = NULL;

	TRACE_ENTER();
	//TBD, this is just for testing purposes

	upgradeMethod = getUpgradeMethod();

	if (upgradeMethod == NULL) {
		LOG_ER("calculateSteps no upgrade method found");
		return false;
	}

	switch (upgradeMethod->getUpgradeMethod()) {
	case SA_SMF_ROLLING:
		{
			SmfRollingUpgrade *rollingUpgrade = (SmfRollingUpgrade *) upgradeMethod;
			if ( !calculateRollingSteps(rollingUpgrade)) {
                                LOG_ER("SmfUpgradeProcedure::calculateSteps:calculateRollingSteps failed");
                                return false;
                        }
			break;
		}

	case SA_SMF_SINGLE_STEP:
		{
			if ( !calculateSingleStep((SmfSinglestepUpgrade*)upgradeMethod)) {
                                LOG_ER("SmfUpgradeProcedure::calculateSteps:calculateSingleStep failed");
                                return false;
                        }
			break;
		}

	default:
		{
			LOG_ER("calculateSteps unknown upgrade method found %d", upgradeMethod->getUpgradeMethod());
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
SmfUpgradeProcedure::calculateRollingSteps(SmfRollingUpgrade * i_rollingUpgrade)
{
	TRACE_ENTER();
	const SmfByTemplate *byTemplate = (const SmfByTemplate *)i_rollingUpgrade->getUpgradeScope();

	if (byTemplate == NULL) {
		LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps no upgrade scope by template found");
		return false;
	}

	const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();

	if (nodeTemplate == NULL) {
		LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps no node template found");
		return false;
	}

	const std::string & objectDn = nodeTemplate->getObjectDn();
	std::list < std::string > nodeList;

	if ( !calculateNodeList(objectDn, nodeList) ){
		LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps no nodes found");
                return false;
        }

	if (nodeList.size() == 0) {
		LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps no nodes found");
		return false;
	}

	const std::list < SmfParentType * >&actUnitTemplates = nodeTemplate->getActivationUnitTemplateList();
	std::list < std::string > activationUnitList;

	if (actUnitTemplates.size() == 0) {
		/* No activation unit templates, use node list as activation/deactivation units */
		std::list < std::string >::const_iterator it;
		int stepCntr = 0;

		for (it = nodeList.begin(); it != nodeList.end(); ++it) {
			std::ostringstream ost;
			stepCntr++;
			ost << stepCntr;

			SmfUpgradeStep *newStep = new SmfUpgradeStep();
			newStep->setRdn("safSmfStep=" + ost.str());
			newStep->setDn(newStep->getRdn() + "," + getDn());
			newStep->addActivationUnit(*it);
			newStep->addDeactivationUnit(*it);
			newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
			newStep->setRestartOption(i_rollingUpgrade->getStepRestartOption());
			newStep->addSwRemove(nodeTemplate->getSwRemoveList());
			newStep->addSwAdd(nodeTemplate->getSwInstallList());
			newStep->setSwNode(*it);

			TRACE("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps new step added %s with activation/deactivation unit %s",
			      newStep->getRdn().c_str(), (*it).c_str());

			/* TODO: Update objects to be modified by step */
			if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_AMF_NODE)){
                                LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:addStepModifications failed");
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
                        if(calcActivationUnitsFromTemplate((*it), nodeList, actDeactUnits) != true) {
                                LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:calcActivationUnitsFromTemplate failed");
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

                        std::ostringstream ost;
                        stepCntr++;
                        ost << stepCntr;

                        SmfUpgradeStep *newStep = new(std::nothrow) SmfUpgradeStep();
                        assert(newStep != NULL);
                        newStep->setRdn("safSmfStep=" + ost.str());
                        newStep->setDn(newStep->getRdn() + "," + getDn());
                        newStep->addActivationUnit(*itActDeact);
                        newStep->addDeactivationUnit(*itActDeact);
                        newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
                        newStep->setRestartOption(i_rollingUpgrade->getStepRestartOption());
                        newStep->addSwRemove(nodeTemplate->getSwRemoveList());
                        newStep->addSwAdd(nodeTemplate->getSwInstallList());
                        std::string node = getNodeForCompSu(*itActDeact);
                        if (node.length() == 0) {
                                LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:getNodeForCompSu fails");
                                return false;
                        }
                        newStep->setSwNode(node);
			actDeactNodes.push_back(node);  //Used to detect uncovered nodes below

                        TRACE("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps new step added %s with activation/deactivation unit %s",
                              newStep->getRdn().c_str(), (*itActDeact).c_str());

			if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_SU_COMP)){
                                LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:addStepModifications failed");
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
			std::ostringstream ost;
			stepCntr++;
			ost << stepCntr;

			SmfUpgradeStep *newStep = new SmfUpgradeStep();
			newStep->setRdn("safSmfStep=" + ost.str());
			newStep->setDn(newStep->getRdn() + "," + getDn());
			newStep->setMaxRetry(i_rollingUpgrade->getStepMaxRetryCount());
			newStep->addSwRemove(nodeTemplate->getSwRemoveList());
			newStep->addSwAdd(nodeTemplate->getSwInstallList());
			newStep->setSwNode(*nodeIt);

			TRACE("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps new SW install step added %s (with no act/deact unit) for node %s",
			      newStep->getRdn().c_str(), (*nodeIt).c_str());

			/* TODO: Update objects to be modified by step */
			if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_AMF_NODE)){
                                LOG_ER("SmfUpgradeProcedure::calculateRollingSteps:addStepModifications failed");
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
SmfUpgradeProcedure::getNodeForCompSu(const std::string & i_objectDn)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;
        SaImmAttrValuesT_2 **attributes;
	const char *className;

        /* First find out if it is a SU or components version type */
        if (immUtil.getObject(i_objectDn, &attributes) == false) {
                LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:failed to get imm object %s", i_objectDn.c_str());
                TRACE_LEAVE();
                return "";
        }

        className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
                                          attributes, SA_IMM_ATTR_CLASS_NAME, 0);
        if (className == NULL) {
                LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:class name not found for version type %s", i_objectDn.c_str());
                TRACE_LEAVE();
                return "";
        }

        if (strcmp(className, "SaAmfSU") == 0) {
                const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                  "saAmfSUHostedByNode",
                                                                  0);
                if (hostedByNode != NULL) {
                        return (char *)hostedByNode->value;
                } else {
                        LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:No hostedByNode attr set for %s", i_objectDn.c_str());  
                        TRACE_LEAVE();
                        return "";
                }

        } else if (strcmp(className, "SaAmfComp") == 0) {
                /* Find the parent SU to this component */
                if (immUtil.getParentObject((i_objectDn), &attributes) == true) {
                        const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                          "saAmfSUHostedByNode", 
                                                                          0);
                        if (hostedByNode != NULL) {
                                TRACE_LEAVE();
                                return (char *)hostedByNode->value;
                        } else {
                                LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:No hostedByNode attr set for %s", i_objectDn .c_str());  
                                TRACE_LEAVE();
                                return "";
                        }
                } else {
                        LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:Fails to get parent to %s", i_objectDn.c_str());  
                        TRACE_LEAVE();
                        return "";
                }
        }

        LOG_ER("SmfUpgradeProcedure::getNodeForCompSu:The DN does not refere to a  SaAmfSUT or SaAmfComp class, DN=%s", i_objectDn.c_str());  
        TRACE_LEAVE();
        return "";
}

//------------------------------------------------------------------------------
// calculateSingleStep()
//------------------------------------------------------------------------------
bool SmfUpgradeProcedure::calculateSingleStep(SmfSinglestepUpgrade* i_upgrade)
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

	if (forAddRemove != NULL) {

		TRACE("SmfUpgradeProcedure::calculateSingleStep:calculateSingleStep: SmfForAddRemove");

		/*
		 * Handle the activation-unit.
		 */
		const SmfActivationUnitType* aunit = forAddRemove->getActivationUnit();
		assert(aunit != NULL);
		assert(aunit->getRemoved().size() == 0);
		assert(aunit->getSwRemove().size() == 0);
		std::list<std::string> nodeList;
		std::list<std::string> entityList;
		std::list<SmfEntity>::const_iterator e;
		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				std::list<std::string> actUnits;
				if (!calcActivationUnitsFromTemplateSingleStep(*e, actUnits, nodeList)) {
					delete newStep;
					return false;
				}
				std::list<std::string>::const_iterator a;
				for (a = actUnits.begin(); a != actUnits.end(); a++) {
					newStep->addActivationUnit(*a);
				}
			} else {
				assert(e->getName().length() > 0);
#if 0
				std::string node = getNodeForCompSu(e->getName());
				if (node.length() > 0) nodeList.push_back(node);
				newStep->addActivationUnit(e->getName());
#endif
				std::string actUnit;
				std::string node;
				if(getActDeactUnitsAndNodes(e->getName(), actUnit, node) == false) {
					LOG_ER("getActDeactUnitsAndNodes failes for DN=%s",e->getName().c_str());
					TRACE_LEAVE();
					return false;
				}

//TODO check the node list, should it not be different lists for add/remove???
				if (node.length() > 0) {
					nodeList.push_back(node);
				}

				entityList.push_back(actUnit);
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
		assert(aunit != NULL);
		assert(aunit->getAdded().size() == 0);
		assert(aunit->getSwAdd().size() == 0);
		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				std::list<std::string> deactUnits;
				if (!calcActivationUnitsFromTemplateSingleStep(*e, deactUnits, nodeList)) {
					delete newStep;
					return false;
				}
				std::list<std::string>::const_iterator a;
				for (a = deactUnits.begin(); a != deactUnits.end(); a++) {
					newStep->addDeactivationUnit(*a);
				}
			} else {
				assert(e->getName().length() > 0);

				std::string deactUnit;
				std::string node;
				if(getActDeactUnitsAndNodes(e->getName(), deactUnit, node) == false) {
					LOG_ER("getActDeactUnitsAndNodes failes for DN=%s",e->getName().c_str());
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
				LOG_ER("SmfUpgradeProcedure::calculateSingleStep: (forAddRemove/removed) byTemplate not implemented");
                                delete newStep;
				return false;
			}
			assert(e->getName().length() > 0);
			SmfImmDeleteOperation* deleteop = new SmfImmDeleteOperation;
			deleteop->setDn(e->getName());
			newStep->addImmOperation(deleteop);
		}

		std::list<std::string>::const_iterator n;
		for (n = nodeList.begin(); n != nodeList.end(); n++) {
			newStep->addSwNode(*n);
		}

		addProcStep(newStep);
		return true;		// <--------- RETURN HERE
	}

	const SmfForModify* forModify = dynamic_cast<const SmfForModify*>(scope);
	if (forModify != NULL) {

		TRACE("SmfUpgradeProcedure::calculateSingleStep: SmfForModify");

		/*
		 * (The main difference between "forAddRemove" and
		 * "forModify" is that the acivation/deactivation
		 * units are separated in addremove but symmetric in
		 * modify.)
		 */

		bool removeDuplicates = false;
		const SmfActivationUnitType* aunit = forModify->getActivationUnit();
		assert(aunit != NULL);
		std::list<std::string> nodeList;
		std::list<SmfEntity>::const_iterator e;
		std::list<std::string> actDeactUnits;
		for (e = aunit->getActedOn().begin(); e != aunit->getActedOn().end(); e++) {
			if (e->getParent().length() > 0 || e->getType().length() > 0) {
				if (!calcActivationUnitsFromTemplateSingleStep(*e, actDeactUnits, nodeList)) {
					delete newStep;
					return false;
				}
			} else {
				assert(e->getName().length() > 0);
				std::string unit;
				std::string node;
				if(getActDeactUnitsAndNodes(e->getName(), unit, node) == false) {
					LOG_ER("getActDeactUnitsAndNodes failes for DN=%s",e->getName().c_str());
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
		if (!addStepModifications(newStep, forModify->getTargetEntityTemplate(), SMF_AU_SU_COMP)) {
			delete newStep;
			return false;
		}

		addProcStep(newStep);
                return true;		// <--------- RETURN HERE
	}

	delete newStep;
	LOG_ER("SmfUpgradeProcedure::calculateSingleStep:Unknown upgradeScope");
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
		LOG_ER("SmfUpgradeProcedure::calculateNodeList:failed to get imm object %s", i_objectDn.c_str());
		return false;
	}

	className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes, SA_IMM_ATTR_CLASS_NAME, 0);
	if (className == NULL) {
		LOG_ER("SmfUpgradeProcedure::calculateNodeList:class name not found for %s", i_objectDn.c_str());
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
		LOG_ER("SmfUpgradeProcedure::calculateNodeList:class name %s for %s unknown as node template", className, i_objectDn.c_str());
		return false;
	}

        return true;
}

//------------------------------------------------------------------------------
// calcActivationUnitsFromTemplateSingleStep()
//------------------------------------------------------------------------------
bool SmfUpgradeProcedure::calcActivationUnitsFromTemplateSingleStep(
	SmfEntity const& i_entity,
	std::list<std::string>& o_actDeactUnits,
	std::list<std::string>& o_nodeList)
{
	SmfParentType parent;
	parent.setParentDn(i_entity.getParent());
	parent.setTypeDn(i_entity.getType());
	std::list<std::string> dummy_nodeList;
	return calcActivationUnitsFromTemplate(
		&parent, dummy_nodeList, o_actDeactUnits, &o_nodeList);
}

//------------------------------------------------------------------------------
// calcActivationUnitsFromTemplate()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::calcActivationUnitsFromTemplate(SmfParentType * i_parentType, 
                                                     const std::list < std::string >&i_nodeList,
                                                     std::list < std::string > &o_actDeactUnits,
						     std::list<std::string>* o_nodeList)
{
        TRACE_ENTER();
        SmfImmUtils immUtil;
        const char *className;
        SaImmAttrValuesT_2 **attributes;

//TODO: If the components are of non restartable type the SU shall be the act/deact unit. 
//      If the restart flag is set in the campaign and the components are non restartable
//      it is considered an error and the campaign fails. The oposit way if restartable
//      components are found and the restart flag is not set it is considered an error and 
//      the campaign shall fail.

        if ( (i_parentType->getTypeDn().size() == 0) && (i_parentType->getParentDn().size() == 0))
        {
                LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No value for parent nor type found in activationUnitTemplate SmfParentType");
                return false;
        }

        if (i_parentType->getTypeDn().size() > 0) { /* Type is set, if parent is set it will just */
                /* narrow the search for SUs or Components   */
                                   
                /* First find out if it is a SU or components version type */
                if (immUtil.getObject(i_parentType->getTypeDn(), &attributes) == false) {
                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:failed to get version type imm object %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
                                                  attributes, SA_IMM_ATTR_CLASS_NAME, 0);
                if (className == NULL) {
                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:class name not found for version type %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                if (strcmp(className, "SaAmfSUType") == 0) {
                        TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Check SU type %s for modifications", i_parentType->getTypeDn().c_str());
                        std::list < std::string > foundObjs; //All found objects of type SaAmfSUType in the system

                        /* type DN is of class SaAmfSUType, find all SU's of this version type */
                        if(!immUtil.getChildren(i_parentType->getParentDn(), foundObjs, SA_IMM_SUBTREE, "SaAmfSU")){
				TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No SaAmfSU type=%s children found to parent=%s",
				       i_parentType->getTypeDn().c_str(), i_parentType->getParentDn().c_str());
				return true;
			}
                                               
                        std::list < std::string >::const_iterator objit;
                        //For each found object check the versioned type
                        for (objit = foundObjs.begin(); objit != foundObjs.end(); ++objit) {
                                TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Check SU %s for modifications", (*objit).c_str());
                                if (immUtil.getObject((*objit), &attributes) == true) {
                                        const SaNameT *typeRef = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
                                                                                     "saAmfSUType",
                                                                                     0);
                                        if ((typeRef != NULL) && 
                                            (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
                                                                      
                                                /* This SU is of the correct version type. Check if it is hosted by any node in the node list */
                                                const SaNameT *hostedByNode =
                                                        immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                            "saAmfSUHostedByNode",
                                                                            0);
						if (hostedByNode == NULL) {
                                                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No hostedByNode attr set for %s", (*objit).c_str());  
                                                        return false;
						}
						TRACE("SU %s hosted by %s, add to list", (*objit).c_str(), (char *)hostedByNode->value);
                                                if (o_nodeList == NULL) { //No output nodelist given, use the input nodelist to check the scope
                                                        std::list < std::string >::const_iterator it;
                                                        for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {
                                                                if (strcmp((*it).c_str(), (char *)hostedByNode->value) == 0) {
                                                                        /* The SU is hosted by the node */
									TRACE("SU %s is hosted on node within the targetNodeTemplate, add to list", (*objit).c_str());
                                                                        o_actDeactUnits.push_back(*objit);
                                                                        break;
                                                                } 
                                                        }
                                                } else { //Scope (of nodes) unknown, add the hosting node to the output node list
							o_actDeactUnits.push_back(*objit);
							o_nodeList->push_back((const char*)hostedByNode->value);
                                                }
                                        }
                                } else {
                                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Fails to get object %s", (*objit).c_str());  
                                        return false;
                                }
                        }
                } else if (strcmp(className, "SaAmfCompType") == 0) {
                        TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Check Comp type %s for modifications", i_parentType->getTypeDn().c_str());
			std::list < std::string > foundObjs; //All found objects of type SaAmfCompType in the system

                        /* type DN is of class SaAmfCompType, find all components's of this version type */
                        if(!immUtil.getChildren(i_parentType->getParentDn(), foundObjs, SA_IMM_SUBTREE, "SaAmfComp")){
				TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No SaAmfComp type=%s children found to parent=%s",
				       i_parentType->getTypeDn().c_str(), i_parentType->getParentDn().c_str());
				return true;
			}

			bool removeDuplicates = false;
                        std::list < std::string >::const_iterator objit;
                        for (objit = foundObjs.begin(); objit != foundObjs.end(); ++objit) {
                                TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Check Comp %s for modifications", (*objit).c_str());
                                if (immUtil.getObject((*objit), &attributes) == true) {
                                        const SaNameT *typeRef =
                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                    "saAmfCompType", 0); 

                                        if ((typeRef != NULL)
                                            && (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
                                                /* This component is of the correct version type. */

						/* Check if it is hosted by any node in the node list */
                                                /* Find the parent SU to this component */
                                                if (immUtil.getParentObject((*objit), &attributes) == true) {
                                                        const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                                                          "saAmfSUHostedByNode", 
                                                                                                          0);
                                                        if (hostedByNode == NULL) {
								LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No hostedByNode attr set for %s", (*objit).c_str());  
								return false;
							}

							TRACE("Component %s hosted by %s", (*objit).c_str(), (char *)hostedByNode->value);
							if (o_nodeList == NULL) {//No output nodelist given, use the input nodelist to check the scope
                                                                std::list < std::string >::const_iterator it;
                                                                for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {
                                                                        if (strcmp((*it).c_str(), (char *)hostedByNode->value) == 0) {
                                                                                /* The SU is hosted by the node */
										if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
											std::string parentDn = (*objit).substr((*objit).find(',') + 1, std::string::npos);
											TRACE("Component %s is hosted on node within the targetNodeTemplate", (*objit).c_str());
											TRACE("The stepRestartOption was set to FALSE(0), use parent %s, as act/deactComponent", parentDn.c_str());
											o_actDeactUnits.push_back(parentDn);
											removeDuplicates = true;  //Duplicates must be removed from list when the loop is finished
										} else { // saSmfStepRestartOption is set to true
											TRACE("Component %s is hosted on node within the targetNodeTemplate, add to list", (*objit).c_str());
											//Check if component is restartable
											if (isCompRestartable((*objit)) == false) {
												LOG_ER("Component %s is not restartable", (*objit).c_str());
												return false;
											}
											o_actDeactUnits.push_back(*objit);
										}

                                                                                break;
                                                                        }
                                                                }
                                                        } else { //Scope (of nodes) unknown, add the hosting node to the output node list
								if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
									std::string parentDn = (*objit).substr((*objit).find(',') + 1, std::string::npos);
									TRACE("The stepRestartOption was set to FALSE(0), use parent %s, as act/deactComponent", parentDn.c_str());
									o_actDeactUnits.push_back(parentDn);
									removeDuplicates = true;  //Duplicates must be removed from list when the loop is finished
								} else { // saSmfStepRestartOption is set to true
									//Check if component is restartable
									if (isCompRestartable((*objit)) == false) {
										LOG_ER("Component %s is not restartable", (*objit).c_str());
										return false;
									}
									o_actDeactUnits.push_back(*objit);
								}

								o_nodeList->push_back((const char*)hostedByNode->value);
                                                        }
                                                }
                                        }
                                } else {
                                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Fails to get object %s", (*objit).c_str());  
                                        return false;
                                }
                        } //End for (objit = foundObjs.begin(); objit != foundObjs.end(); ++objit)

			if (removeDuplicates == true){
				TRACE("Sort the act/deact unit list");
				o_actDeactUnits.sort();
				o_actDeactUnits.unique();
			}
                }
        }
        if (i_parentType->getTypeDn().size() == 0) {
                /* Only parent is set */
                TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Find SUs for parent SG %s for modifications", i_parentType->getParentDn().c_str());
                std::list < std::string > foundObjs; //All found SUs which are children to the parent SG

                /* type DN is of class SaAmfSUType, find all SU's of this version type */
                if(!immUtil.getChildren(i_parentType->getParentDn(), foundObjs, SA_IMM_SUBTREE, "SaAmfSU")){
			TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No SaAmfSU children found to parent=%s", i_parentType->getParentDn().c_str());
			return true;
		}
                       
                std::list < std::string >::const_iterator objit;
                //For all found SUs
                for (objit = foundObjs.begin(); objit != foundObjs.end(); ++objit) {
                        TRACE("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Check SU %s for modifications", (*objit).c_str());
                        if (immUtil.getObject((*objit), &attributes) == true) {
                                /* Type does not matter. Check if the SU is hosted by any node in the node list */
                                const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                                  "saAmfSUHostedByNode",
                                                                                  0);
				if (hostedByNode == NULL) {
                                        LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No hostedByNode attr set for %s", (*objit).c_str());  
                                        return false;
				}

				TRACE("SU %s is hosted by %s", (*objit).c_str(), (char *)hostedByNode->value);
                                if (o_nodeList == NULL) {
                                        std::list < std::string >::const_iterator it;
					TRACE("Check if node is within the the targetNodeTemplate");
                                        for (it = i_nodeList.begin(); it != i_nodeList.end(); ++it) {                                                
						if (strcmp((*it).c_str(), (char *)hostedByNode->value) == 0) {
                                                        /* The SU is hosted by the node */
                                                        TRACE("SU %s is hosted on node within the targetNodeTemplate, add to list", (*objit).c_str());
                                                        o_actDeactUnits.push_back(*objit);
                                                        break;
                                                }
                                        }
                                } else {
					o_actDeactUnits.push_back(*objit);
					o_nodeList->push_back((const char*)hostedByNode->value);
                                }
                        } else {
                                LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:Fails to get object %s", (*objit).c_str());  
                                return false;
                        }
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
                                          SmfAuT i_auType)
{
	std::list < SmfTargetEntityTemplate * >::const_iterator it;

	for (it = i_targetEntityTemplate.begin(); it != i_targetEntityTemplate.end(); ++it) {
		SmfTargetEntityTemplate *targetEntity = *it;
		const SmfParentType *parentType = targetEntity->getEntityTemplate();
		const std::list < SmfImmModifyOperation * >&modificationList = targetEntity->getModifyOperationList();

		switch (i_auType) {
		case SMF_AU_AMF_NODE: /* Activation unit is Node */
			{
				if (!addStepModificationsNode(i_newStep, parentType, modificationList)) {
                                        LOG_ER("SmfUpgradeProcedure::addStepModifications: addStepModificationsNode failed");
                                        return false;
                                }
				break;
			}
		case SMF_AU_SU_COMP: /* Activation unit is SU or Component */
			{
				if ( !addStepModificationsSuComp(i_newStep, parentType, modificationList)) {
                                        LOG_ER("SmfUpgradeProcedure::addStepModifications: addStepModificationsSuComp failed");
                                        return false;
                                }
				break;
			}
		default:
                        
                        LOG_ER("SmfUpgradeProcedure::addStepModifications: unknown auType");
                        return false;
			break;
		}
	}

        return true;
}

//------------------------------------------------------------------------------
// addStepModificationsNode()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsNode(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
                                              const std::list < SmfImmModifyOperation * >&i_modificationList)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	const char *className;
	std::list < std::string > objectList;
	std::list < std::string >::const_iterator objit;
	const std::string & auNodeName = i_newStep->getActivationUnitList().front();

	if (i_parentType->getTypeDn().size() > 0) {
		/* Type was specified, find all objects of this version type */
		TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check type %s for modifications", i_parentType->getTypeDn().c_str());

		/* First find out if it's a SU or components version type */
		if (immUtil.getObject(i_parentType->getTypeDn(), &attributes) == false) {
			LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:failed to get version type imm object %s", i_parentType->getTypeDn().c_str());
			return false;
		}

		className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
						  attributes, SA_IMM_ATTR_CLASS_NAME, 0);
		if (className == NULL) {
			LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:class name not found for version type %s", i_parentType->getTypeDn().c_str());
			return false;
		}

		if (strcmp(className, "SaAmfSUType") == 0) {
			TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check SU type %s for modifications", i_parentType->getTypeDn().c_str());

			/* type DN is of class SaAmfSUType, find all SU's of this version type */
			(void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBTREE, "SaAmfSU");

			for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
				TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check SU %s for modifications", (*objit).c_str());
				if (immUtil.getObject((*objit), &attributes) == true) {
					const SaNameT *typeRef =
                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfSUType",
                                                                    0);

					TRACE("typeRef = %s", (char*)typeRef->value);

					if ((typeRef != NULL)
					    && (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
						/* This SU is of the correct version type */
						TRACE("This SU is of the correct version type");

						/* Check if the found SU is hosted by the activation unit (node) */
						TRACE("Check if the found SU is hosted by the activation unit (node)");
						const SaNameT *hostedByNode =
                                                        immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                            "saAmfSUHostedByNode",
                                                                            0);
						if (hostedByNode == NULL) {
                                                        LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:No hostedByNode attr set for %s", (*objit).c_str());  
                                                        return false;
						}
                               
						TRACE("hostedByNode = %s", (char*)hostedByNode->value);
						if ((hostedByNode != NULL)
						    && (strcmp(auNodeName.c_str(), (char *)hostedByNode->value) == 0)) {
							/* The SU is hosted by the AU node */
							TRACE("SmfUpgradeProcedure::addStepModificationsNode:SU %s hosted by %s, add modifications", (*objit).c_str(),
							      auNodeName.c_str());
							if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
                                                                LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:addStepModificationList fails");
                                                                return false;
                                                        }
						}
					}
				}
			}
		} else if (strcmp(className, "SaAmfCompType") == 0) {
			TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check Comp type %s for modifications", i_parentType->getTypeDn().c_str());

			/* type DN is of class SaAmfCompType, find all components's of this version type */
			(void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBTREE, "SaAmfComp");

			for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
				TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check Comp %s for modifications", (*objit).c_str());
				if (immUtil.getObject((*objit), &attributes) == true) {
					const SaNameT *typeRef =
                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                    "saAmfCompType", 0);

					if ((typeRef != NULL)
					    && (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
						/* This component is of the correct version type */

						/* Check if the found component is hosted by the activation unit (node) */

						/* Find the parent SU to this component */
						if (immUtil.getParentObject((*objit), &attributes) == true) {
							const SaNameT *hostedByNode =
                                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                                    "saAmfSUHostedByNode", 0);
							if ((hostedByNode != NULL)
							    && (strcmp(auNodeName.c_str(), (char *)hostedByNode->value)
								== 0)) {
								/* The component is hosted by the AU node */
								TRACE("Component %s hosted by %s, add modifications",
								      (*objit).c_str(), auNodeName.c_str());
                                                                if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
                                                                        LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:addStepModificationList fails");
                                                                        return false;
                                                                }
							}
						}
					}
				}
			}
		} else {
			LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:class name %s for type %s not valid as type", className,
			       i_parentType->getTypeDn().c_str());
			return false;
		}
	} else if (i_parentType->getParentDn().size() > 0) {	
                /* Parent was specified */
                TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check SU children to parent %s for modifications", i_parentType->getParentDn().c_str());

                /* Parent SG but no type specified, find all child SU's to the parent SG */
                (void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBLEVEL, "SaAmfSU");

                for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
                        TRACE("SmfUpgradeProcedure::addStepModificationsNode:Check child SU %s for modifications", (*objit).c_str());
                        if (immUtil.getObject((*objit), &attributes) == true) {
                                /* Check if the found SU is hosted by the activation unit (node) */
                                const SaNameT *hostedByNode =
                                        immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                            "saAmfSUHostedByNode",
                                                            0);
                                if ((hostedByNode != NULL)
                                    && (strcmp(auNodeName.c_str(), (char *)hostedByNode->value) == 0)) {
                                        /* The SU is hosted by the AU node */
                                        TRACE("SmfUpgradeProcedure::addStepModificationsNode:SU %s hosted by %s, add modifications", (*objit).c_str(),
                                              auNodeName.c_str());
                                        if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
                                                LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:addStepModificationList fails");
                                                return false;
                                        }
                                }
                        }
                }
        } else {                          
                //No parent type was set, apply the modifications on the entity pointed out by the activation unit.
                //The optional modifyOperation RDN attribute if added when applying the modification
 
                if (!addStepModificationList(i_newStep, auNodeName, i_modificationList)) {
                        LOG_ER("SmfUpgradeProcedure::addStepModificationsNode:addStepModificationList fails");
                        return false;
                }
        }

        TRACE_LEAVE();
        return true;
}

//------------------------------------------------------------------------------
// addStepModificationsSuComp()
//------------------------------------------------------------------------------
bool 
SmfUpgradeProcedure::addStepModificationsSuComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
                                            const std::list < SmfImmModifyOperation * >&i_modificationList)
{
        TRACE_ENTER();
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	const char *className;
	std::list < std::string > objectList;
	std::list < std::string >::const_iterator objit;
        std::list < std::string >::const_iterator auEntityIt;

//	const std::string & auEntity = i_newStep->getActivationUnitList().front();
        const std::list < std::string > auEntityList = i_newStep->getActivationUnitList();

        if (i_parentType->getTypeDn().size() > 0) { //Type is set, parent may be set to limit the scope
                /* Type was specified, find all objects of this version type */
                TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check type %s for modifications", i_parentType->getTypeDn().c_str());

                /* First find out if it's a SU or components version type */
                if (immUtil.getObject(i_parentType->getTypeDn(), &attributes) == false) {
                        LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:failed to get version type imm object %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
                                                  attributes, SA_IMM_ATTR_CLASS_NAME, 0);
                if (className == NULL) {
                        LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:class name not found for version type %s", i_parentType->getTypeDn().c_str());
                        return false;
                }

                if (strcmp(className, "SaAmfSUType") == 0) {
                        TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check SU type %s for modifications", i_parentType->getTypeDn().c_str());

                        /* type DN is of class SaAmfSUType, find all SU's of this version type */
                        (void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBTREE, "SaAmfSU");

			for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
				TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check SU %s for modifications", (*objit).c_str());
				if (immUtil.getObject((*objit), &attributes) == true) {
					const SaNameT *typeRef =
						immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfSUType", 0);


					if ((typeRef != NULL) && (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
						/* This SU is of the correct version type */
                                                //For all AU/DU in the step, check if the SU DN is within the AU/DU DN
                                                for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
							//If the auEntity is an amf node it must be find out if the SU is hosted on the node
							SaImmAttrValuesT_2 **auAttributes;
							if (immUtil.getObject(*auEntityIt, &auAttributes) == false) {
								LOG_ER("addStepModificationsSuComp:Failed to get actedOn object %s in IMM", (*auEntityIt).c_str());
								TRACE_LEAVE();
								return false;
							}

							className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
											  auAttributes, SA_IMM_ATTR_CLASS_NAME, 0);
							if (className == NULL) {
								LOG_ER("addStepModificationsSuComp:class name not found for actedOn object %s", (*auEntityIt).c_str());
								TRACE_LEAVE();
								return false;
							}

							if (strcmp(className, "SaAmfNode") == 0) {  
								//The auEntity is a node.
								//Find out if the instance of the SaAmfSUType is located on the node
								if (immUtil.getObject((*objit), &attributes) == false) {
									LOG_ER("addStepModificationsSuComp: Can not read object %s", (*objit).c_str());
									return false;
								}

							       	const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
														  "saAmfSUHostedByNode",
														  0);
								
								if (hostedByNode == NULL) {
									LOG_ER("addStepModificationsSuComp:No hostedByNode attribute set for %s", (*objit).c_str());  
									return false;
								}

								//Match "hostedByNode" with the auEntity
								if (strcmp((*auEntityIt).c_str(), (char *)hostedByNode->value) == 0) {
								       	TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
									      (*objit).c_str(),
									      (*auEntityIt).c_str());

									if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
										LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
										return false;
									}
									break;
								} else {
									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:NO MATCH Modification DN=%s, AU/DU=%s", (*objit).c_str(),(*auEntityIt).c_str());
								}
							} else {
								//The auEntity is NOT a node.
								//If the auEntity is a SU, string matching can be used
								if((*objit).find(*auEntityIt) != std::string::npos) {
									//The auEntityIt DN is a substring of objit i.e. the object 
									//to modify within the AU/DU domain
									//Add object modifications

									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
									      (*objit).c_str(),
									      (*auEntityIt).c_str());
                                                   
									if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
										LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
										return false;
									}
									break;
								} else {
									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:NO MATCH Modification DN=%s, AU/DU=%s", (*objit).c_str(),(*auEntityIt).c_str());
								}
							} 
						}
					}
				}
			}
                } else if (strcmp(className, "SaAmfCompType") == 0) {
                        TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check Comp type %s for modifications", i_parentType->getTypeDn().c_str());

                        /* type DN is of class SaAmfCompType, find all components's of this version type */
                        (void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBTREE, "SaAmfComp");

                        for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
				TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check Comp %s for modifications", (*objit).c_str());
				if (immUtil.getObject((*objit), &attributes) == true) {
					const SaNameT *typeRef =
                                                immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfCompType", 0);

					if ((typeRef != NULL)
					    && (strcmp(i_parentType->getTypeDn().c_str(), (char *)typeRef->value) == 0)) {
						/* This component is of the correct version type */
                                                //For all objects found of this type, match DNs to see if it is within the act/deact unit
						for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {

							//If the auEntity is an amf node it must be found out if the SU is hosted on the node
							SaImmAttrValuesT_2 **auAttributes;
							if (immUtil.getObject(*auEntityIt, &auAttributes) == false) {
								LOG_ER("addStepModificationsSuComp:Failed to get actedOn object %s in IMM", (*auEntityIt).c_str());
								TRACE_LEAVE();
								return false;
							}

							className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
											  auAttributes, SA_IMM_ATTR_CLASS_NAME, 0);
							if (className == NULL) {
								LOG_ER("addStepModificationsSuComp:class name not found for actedOn object %s", (*auEntityIt).c_str());
								TRACE_LEAVE();
								return false;
							}

							if (strcmp(className, "SaAmfNode") == 0) {  
								//The auEntity is a node.
								//Find out if the instance of the SaAmfCompType is located on the node

								//Find the parent SU to this component
								if (immUtil.getParentObject((*objit), &attributes) == false) {
									LOG_ER("addStepModificationsSuComp:No parent found to %s", (*objit).c_str());
									return false;
								}

								const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
														  "saAmfSUHostedByNode", 
														  0);
								if (hostedByNode == NULL) {
									LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No hostedByNode attr set for %s", (*objit).c_str());  
									return false;
								}

								//Match "hostedByNode" with the auEntity
								if (strcmp((*auEntityIt).c_str(), (char *)hostedByNode->value) == 0) {
									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
									      (*objit).c_str(),
									      (*auEntityIt).c_str());

									if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
										LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
										return false;
									}
									break;
								} else {
									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:NO MATCH Modification DN=%s, AU/DU=%s", 
									      (*objit).c_str(),
									      (*auEntityIt).c_str());
								}
								
							} else {
								//The auEntity is NOT a node.
								//If the auEntity is a SU, string matching can be used
								if((*objit).find(*auEntityIt) != std::string::npos) {
									//The auEntityIt DN is a substring of objit i.e. the object 
									//to modify within the AU/DU domain
									//Add object modifications

									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
									      (*objit).c_str(),
									      (*auEntityIt).c_str());
                                                   
									if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
										LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
										return false;
									}
									break;

								} else {
									TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:NO MATCH Modification DN=%s, AU/DU=%s", (*objit).c_str(),(*auEntityIt).c_str());
								}
							}
						}//END for (auEntityIt = auEntityList.begin()
					}
				} //END for (objit = objectList.begin();
			}
		 } else {
			   LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:class name %s for type %s not valid as type",
					     className,
					      i_parentType->getTypeDn().c_str());
			    return false;
		   }
		} else if (i_parentType->getParentDn().size() > 0) { //Parent only is set, no type
			/* Parent was specified */
			TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Check SU children to parent %s for modifications", i_parentType->getParentDn().c_str());

			/* Parent SG but no type specified, find all child SU's to the parent SG */
			(void)immUtil.getChildren(i_parentType->getParentDn(), objectList, SA_IMM_SUBLEVEL, "SaAmfSU");

			for (objit = objectList.begin(); objit != objectList.end(); ++objit) {
				for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
					//If the auEntity is an amf node it must be found out if the SU is hosted on the node
					SaImmAttrValuesT_2 **auAttributes;
					if (immUtil.getObject(*auEntityIt, &auAttributes) == false) {
						LOG_ER("addStepModificationsSuComp:Failed to get actedOn object %s in IMM", (*auEntityIt).c_str());
						TRACE_LEAVE();
						return false;
					}

					className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
									  auAttributes, SA_IMM_ATTR_CLASS_NAME, 0);
					if (className == NULL) {
						LOG_ER("addStepModificationsSuComp:class name not found for actedOn object %s", (*auEntityIt).c_str());
						TRACE_LEAVE();
						return false;
					}

					if (strcmp(className, "SaAmfNode") == 0) {  
						//The auEntity is a node.
						//Find out if the instance of the SaAmfCompType is located on the node

						//Find the parent SU to this component
						if (immUtil.getParentObject((*objit), &attributes) == false) {
							LOG_ER("addStepModificationsSuComp:No parent found to %s", (*objit).c_str());
							return false;
						}

						const SaNameT *hostedByNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
												  "saAmfSUHostedByNode", 
												  0);
						if (hostedByNode == NULL) {
							LOG_ER("SmfUpgradeProcedure::calcActivationUnitsFromTemplate:No hostedByNode attr set for %s", (*objit).c_str());  
							return false;
						}

						//Match "hostedByNode" with the auEntity
						if (strcmp((*auEntityIt).c_str(), (char *)hostedByNode->value) == 0) {
							TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
							      (*objit).c_str(),
							      (*auEntityIt).c_str());

							if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
								LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
								return false;
							}
							break;
						}
					} else {
						//The auEntity is NOT a node.
						//If the auEntity is a SU, string matching can be used
						if((*objit).find(*auEntityIt) != std::string::npos) {
							//The auEntityIt DN is a substring of objit i.e. the object 
							//to modify within the AU/DU domain
							//Add object modifications

							TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:Modification DN=%s, is within AU/DU=%s, add modifications", 
							      (*objit).c_str(),
							      (*auEntityIt).c_str());
                                                   
							if (!addStepModificationList(i_newStep, *objit, i_modificationList)) {
								LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
								return false;
							}
							break;
						} else {
							TRACE("SmfUpgradeProcedure::addStepModificationsSuComp:NO MATCH Modification DN=%s, AU/DU=%s", 
							      (*objit).c_str(),
							      (*auEntityIt).c_str());
						}
					} 
				} //END for (auEntityIt = auEntityList.begin()
			} //END for (objit = objectList.begin()
		} else {

			//No parent type was set, apply the modifications on the entity pointed out by the activation units.
			//The optional modifyOperation RDN attribute if added when applying the modification
			std::list < std::string >::const_iterator auEntityIt;
			for (auEntityIt = auEntityList.begin(); auEntityIt != auEntityList.end(); ++auEntityIt) {
				if (!addStepModificationList(i_newStep, *auEntityIt, i_modificationList)) {
					LOG_ER("SmfUpgradeProcedure::addStepModificationsSuComp:addStepModificationList fails");
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
                        LOG_ER("SmfUpgradeProcedure::createImmSteps: createImmStep returns SaAisErrorT=%d", rc);
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
        char str[64];

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
        sprintf(str,"%d",i_step->getMaxRetry());
        attrsaSmfStepMaxRetry.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepMaxRetry);

        SmfImmAttribute attrsaSmfStepRetryCount;
        attrsaSmfStepRetryCount.setName("saSmfStepRetryCount");
        attrsaSmfStepRetryCount.setType("SA_IMM_ATTR_SAUINT32T");
        sprintf(str,"%d",i_step->getRetryCount());
        attrsaSmfStepRetryCount.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepRetryCount);
 
        SmfImmAttribute attrsaSmfStepRestartOption;
        attrsaSmfStepRestartOption.setName("saSmfStepRestartOption");
        attrsaSmfStepRestartOption.setType("SA_IMM_ATTR_SAUINT32T");
        sprintf(str,"%d",i_step->getRestartOption());
        attrsaSmfStepRestartOption.addValue(str);
        icoSaSmfStep.addValue(attrsaSmfStepRestartOption);

        SmfImmAttribute attrsaSmfStepState;
        attrsaSmfStepState.setName("saSmfStepState");
        attrsaSmfStepState.setType("SA_IMM_ATTR_SAUINT32T");
        sprintf(str,"%d",i_step->getState());
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
                LOG_ER("SmfUpgradeProcedure::createImmStep: icoSaSmfStep.execute() returned %d", rc);
                TRACE_LEAVE();
                return rc;
        }

	std::list < std::string >::const_iterator iter;
	std::list < std::string >::const_iterator iterE;

	/* Create the SaSmfDeactivationUnit object */
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
	const std::list < std::string > deactList = i_step->getDeactivationUnitList();
	if(deactList.size() != 0) {
		iter = deactList.begin();
		iterE = deactList.end();
		while (iter != iterE) {
			attrsaSmfDuActedOn.addValue(*iter);
			iter++;
		}
	} else {
		/* A value must always be supplied to a cached runtime attribute */
		attrsaSmfDuActedOn.addValue("");
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
		LOG_ER("SmfUpgradeProcedure::createImmStep: icoSaSmfDeactivationUnit.execute() returned %d", rc);
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
                saSmfINNode.addValue(i_step->getSwNode());
                icoSaSmfImageNodes.addValue(saSmfINNode);

                rc = icoSaSmfImageNodes.execute(); //Create the object
                if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
                        LOG_ER("SmfUpgradeProcedure::createImmStep: icoSaSmfImageNodes.execute() returned %d", rc);
                        TRACE_LEAVE();
                        return rc;
                }

		bundleRefiter++;
	}

	/* Create the SaSmfActivationUnit object */
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
	if(actList.size() != 0) {
		iter = actList.begin();
		iterE = actList.end();
		while (iter != iterE) {
			attrsaSmfAuActedOn.addValue(*iter);
			iter++;
		}
	} else {
		/* A value must always be supplied to a cached runtime attribute */
		attrsaSmfAuActedOn.addValue("");
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
		LOG_ER("SmfUpgradeProcedure::createImmStep: icoSaSmfActivationUnit.execute() returned %d", rc);
		TRACE_LEAVE();
		return rc;
	}

        /* Create the SaSmfImageNodes objects as childrens to the SaSmfActivationUnit instance */
        /* One SaSmfImageNodes object for each added software bundle                           */

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
                saSmfINNode.addValue(i_step->getSwNode());
                icoSaSmfImageNodes.addValue(saSmfINNode);

                rc = icoSaSmfImageNodes.execute(); //Create the object
                if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_EXIST)){
                       LOG_ER("SmfUpgradeProcedure::createImmStep: icoSaSmfImageNodes.execute() returned %d", rc);
                        TRACE_LEAVE();
                        return rc;
                }

		bundleRefiter++;
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
		LOG_ER("SmfUpgradeProcedure::getImmSteps: no upgrade method found");
		rc = SA_AIS_ERR_NOT_EXIST;
	}
			
	if (upgradeMethod->getUpgradeMethod() == SA_SMF_ROLLING) {
		TRACE("Rolling upgrade");
		rc = getImmStepsRolling();

	} else if (upgradeMethod->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		TRACE("Single step upgrade");
		rc = getImmStepsSingleStep();
	} else {
		LOG_ER("SmfUpgradeProcedure::getImmSteps: no upgrade method type found");
		rc =  SA_AIS_ERR_NOT_EXIST;
	}

	return rc;
}

//------------------------------------------------------------------------------
// getImmStepsRolling()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeProcedure::getImmStepsRolling()
{
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;
	SaAisErrorT rc = SA_AIS_OK;
	std::list < std::string > stepList;

	TRACE_ENTER();
	// Read the steps from IMM
	if (immutil.getChildren(getDn(), stepList, SA_IMM_SUBLEVEL, "SaSmfStep") == false) {
		LOG_ER("SmfUpgradeProcedure::getImmSteps: Failed to get steps for procedure %s", getDn().c_str());
		return SA_AIS_ERR_NOT_EXIST;
	}

	// We need to sort the list of steps here since the order from IMM is not guaranteed
	stepList.sort();

	std::list < std::string >::iterator stepit;
	TRACE("Fetch IMM data for our upgrade procedure steps");

	/* Fetch IMM data for our upgrade procedure steps */
	for (stepit = stepList.begin(); stepit != stepList.end(); ++stepit) {
		if (immutil.getObject((*stepit), &attributes) == false) {
			LOG_ER("SmfUpgradeProcedure::getImmSteps: IMM data for step %s not found", (*stepit).c_str());
			rc = SA_AIS_ERR_NOT_EXIST;
			goto done;
		}

		TRACE("Fetch IMM data for item in stepList");

		SmfUpgradeStep *newStep = new SmfUpgradeStep();

		// The init method will reset step in state Executing to state Init to be able to
		// re-execute the unfinished step in case of a failover
		if (newStep->init((const SaImmAttrValuesT_2 **)attributes) != SA_AIS_OK) {
			LOG_ER("SmfUpgradeProcedure::getImmSteps: Initialization failed for step %s", (*stepit).c_str());
			delete newStep;
			rc = SA_AIS_ERR_INIT;
			goto done;
		}

		newStep->setDn((*stepit));
		newStep->setProcedure(this);

		// After a switchover the new SMFD must:
		// For all steps in status Initial and Executing, 
		// recreate the SMFD internal steps as it was after initial calculation
		// This is neede because the steps are calculated "on the fly"
		// on the active node and the result is partly saved in IMM and partly
		// stored in local memory when a new procedure is started.
		// The AU/DU and software bundles can be fetched from IMM.
		// The modification must be recalculated.				
		if((newStep->getState() == SA_SMF_STEP_INITIAL) ||
                   (newStep->getState() == SA_SMF_STEP_EXECUTING)) {
			TRACE("Step found in state SA_SMF_STEP_INITIAL or SA_SMF_STEP_EXECUTING %s", getDn().c_str());

		        newStep->setDn(newStep->getRdn() + "," + getDn());

			SmfUpgradeMethod *upgradeMethod = getUpgradeMethod();
			if (upgradeMethod == NULL) {
				LOG_ER("SmfUpgradeProcedure::getImmSteps: no upgrade method found");
				rc = SA_AIS_ERR_INIT;
				goto done;
			}

			TRACE("Rolling upgrade");
			newStep->setMaxRetry(((SmfRollingUpgrade*)upgradeMethod)->getStepMaxRetryCount());
			newStep->setRestartOption(((SmfRollingUpgrade*)upgradeMethod)->getStepRestartOption());

			const SmfByTemplate *byTemplate = (const SmfByTemplate *)((SmfRollingUpgrade*)upgradeMethod)->getUpgradeScope();
			const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();

			// The SW list can not be fetched from the SaSmfImageNodes objects since it lack information
			// about e.g. pathNamePrefix.
			// Fetch it from the campaign XML data.
			newStep->addSwRemove(nodeTemplate->getSwRemoveList());
			newStep->addSwAdd(nodeTemplate->getSwInstallList());

			std::list < std::string > auList;
			std::list < std::string > duList;
			std::list < std::string >::iterator stringit;
			unsigned int ix;
			//---------------------------------------------
			// Read the SaSmfActivationUnit object from IMM
			//---------------------------------------------
			TRACE("Read the SaSmfActivationUnit object from IMM parent=%s", newStep->getDn().c_str());
			if (immutil.getChildren(newStep->getDn(), auList, SA_IMM_SUBLEVEL, "SaSmfActivationUnit") != false) {
				TRACE("SaSmfActivationUnit:Resulting list size=%zu", auList.size());

				/* Fetch IMM data for SaSmfAactivationUnit (should be max one)*/
				std::string activationUnit = (*auList.begin());
				if (immutil.getObject(activationUnit, &attributes) == false) {
					LOG_ER("SmfUpgradeProcedure::getImmSteps: IMM data for step activationUnit %s not found", activationUnit.c_str());
					rc = SA_AIS_ERR_NOT_EXIST;
					goto done;
				}

				//For the SaAmfActivationUnit read the attribute saSmfAuActedOn and store it in step
				TRACE("For the SaAmfActivationUnit read the saSmfAuActedOn and store it in step");
				const SaNameT * au;
				for(ix = 0; (au = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
								      "saSmfAuActedOn", ix)) != NULL; ix++) {
					TRACE("addActivationUnit %s", (char *)au->value);
					std::string str = (char *)au->value;
					if(str != "") {
						newStep->addActivationUnit((char *)au->value);
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
							LOG_ER("SmfUpgradeProcedure::getImmSteps: IMM data for ImageNodes %s not found", imageNodes.c_str());
							rc = SA_AIS_ERR_NOT_EXIST;
							goto done;
						}

						// Read the saSmfINSwNode attribute
						const SaNameT *saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINNode", 0);
						if (saSmfINNode == NULL) {
							LOG_ER("SmfUpgradeProcedure::getImmSteps: Could not read saSmfINNode");  
							rc = SA_AIS_ERR_NOT_EXIST;
							goto done;
						}
						TRACE("saSmfINNode->value = %s", (char*)saSmfINNode->value);
						newStep->setSwNode((char*)saSmfINNode->value);
					}
				}
			}

			//---------------------------------------------
			// Read the SaSmfDeactivationUnit object from IMM
			//---------------------------------------------
			TRACE("Read the SaSmfDeactivationUnit object from IMM parent=%s", newStep->getDn().c_str());
			if (immutil.getChildren(newStep->getDn(), duList, SA_IMM_SUBLEVEL, "SaSmfDeactivationUnit") != false) {
				TRACE("SaSmfDeactivationUnit:Resulting list size=%zu", duList.size());

				/* Fetch IMM data for SaSmfDeactivationUnit (should be max one)*/
				std::string deactivationUnit = (*duList.begin());
				if (immutil.getObject(deactivationUnit, &attributes) == false) {
					LOG_ER("SmfUpgradeProcedure::getImmSteps: IMM data for step deactivationUnit %s not found", deactivationUnit.c_str());
					rc = SA_AIS_ERR_NOT_EXIST;
					goto done;
				}

				//For the SaAmfDeactivationUnit read the attribute saSmfDuActedOn and store it in step
				TRACE("For the SaAmfDeactivationUnit read the saSmfDuActedOnand store it in step");
				const SaNameT * du;
				for(ix = 0; (du = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, 
								      "saSmfDuActedOn", ix)) != NULL; ix++) {
					TRACE("addDeactivationUnit %s", (char *)du->value);
					std::string str = (char *)du->value;
					if(str != "") {
						newStep->addDeactivationUnit((char *)du->value);
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
							LOG_ER("SmfUpgradeProcedure::getImmSteps: IMM data for ImageNodes %s not found", imageNodes.c_str());
							rc = SA_AIS_ERR_NOT_EXIST;
							goto done;
						}

						// Read the saSmfINSwNode attribute
						const SaNameT *saSmfINNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saSmfINNode", 0);
						if (saSmfINNode == NULL) {
							LOG_ER("SmfUpgradeProcedure::getImmSteps: Could not read saSmfINNode");  
							rc = SA_AIS_ERR_NOT_EXIST;
							goto done;
						}

						TRACE("saSmfINNode->value = %s", (char*)saSmfINNode->value);
						newStep->setSwNode((char*)saSmfINNode->value);
					}
				}
			}

//			TRACE("SmfUpgradeProcedure::calculateRollingSteps:calculateRollingSteps new step added %s with activation/deactivation unit %s",
//			      newStep->getRdn().c_str(), (*it).c_str());
//
			const std::list < SmfParentType * >&actUnitTemplates = nodeTemplate->getActivationUnitTemplateList();

			if (actUnitTemplates.size() == 0) {
				if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_AMF_NODE)){
					LOG_ER("SmfUpgradeProcedure::getImmSteps:addStepModifications failed");
					rc = SA_AIS_ERR_CAMPAIGN_PROC_FAILED;
					goto done;
				}
			} else {
				if ( !addStepModifications(newStep, byTemplate->getTargetEntityTemplate(), SMF_AU_SU_COMP)){
					LOG_ER("SmfUpgradeProcedure::getImmSteps:addStepModifications failed");
					rc = SA_AIS_ERR_CAMPAIGN_PROC_FAILED;
					goto done;
				}
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
	SaAisErrorT rc = SA_AIS_OK;
	TRACE_ENTER();

	TRACE("SmfUpgradeProcedure::getImmStepsSingleStep");
        if( !this->calculateSteps() ) {
		rc = SA_AIS_ERR_INIT;
        }

	TRACE_LEAVE();
	return rc;
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
				LOG_ER("getClassDescription FAILED for [%s]", createOper->getClassName().c_str());
				return false;
			}
			assert(attrDefinitionsOut != NULL);

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
                                                TRACE("TEST TEST create DN = %s", str.c_str());
                                                io_smfEntityToAddRemove->addValue(rdn + "," + createOper->getParentDn());
                                                break;
                                        }
                                }
                        } else {
                                LOG_ER("SmfUpgradeProcedure::setEntitiesToAddRemMod: no values set in create operation for instance of class %s",createOper->getClassName().c_str());
                                return false;
                        }
                } else if ((deleteOper = dynamic_cast<SmfImmDeleteOperation*>(*it)) != 0) {
                        io_smfEntityToAddRemove->addValue(deleteOper->getDn());
                } else if ((modifyOper = dynamic_cast<SmfImmModifyOperation*>(*it)) != 0) {
                        io_smfEntityToAddRemove->addValue(modifyOper->getDn());
                } else {
                        LOG_ER("SmfUpgradeProcedure::setEntitiesToAddRemMod: unknown operation");
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
		LOG_ER("Fails to get object %s", i_compDN.c_str());  
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
			LOG_ER("Can not read attr saAmfCompType in object %s", i_compDN.c_str());
			rc = false;
			goto done;
		}

		if (immUtil.getObject((char *)saAmfCompType->value, &attributes) == false) {
			LOG_ER("Can not find object %s", (char *)saAmfCompType->value);
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
SmfUpgradeProcedure::getActDeactUnitsAndNodes(const std::string &i_dn, std::string& io_unit, std::string& io_node)
{
	TRACE_ENTER();
	//Find out what type of object the actedOn points to
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;
	const char *className;
	bool rc = true;

	if (immUtil.getObject(i_dn, &attributes) == false) {
		LOG_ER("SmfUpgradeProcedure::calculateSingleStep:failed to get imm object %s", i_dn.c_str());
		rc =  false;
		goto done;
	}

	className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)
					  attributes, SA_IMM_ATTR_CLASS_NAME, 0);
	if (className == NULL) {
		LOG_ER("SmfUpgradeProcedure::calculateSingleStep:class name not found for version type %s", i_dn.c_str());
		rc =  false;
		goto done;
	}

	if (strcmp(className, "SaAmfSU") == 0) {
		io_node = getNodeForCompSu(i_dn);
		io_unit = i_dn;
	} else if (strcmp(className, "SaAmfComp") == 0) {
		io_node = getNodeForCompSu(i_dn);
		if (getUpgradeMethod()->getStepRestartOption() == 0) { //saSmfStepRestartOption is set to false, use SU level
			io_unit = i_dn.substr(i_dn.find(',') + 1, std::string::npos);
			TRACE("The stepRestartOption was set to FALSE(0), use parent %s, as act/deactComponent", io_unit.c_str());
		} else { // saSmfStepRestartOption is set to true
			//Check if component is restartable
			if (isCompRestartable(i_dn) == false) {
				LOG_ER("Component %s is not restartable", i_dn.c_str());
				rc =  false;
				goto done;
			}
			io_unit = i_dn;
		}
	} else if (strcmp(className, "SaAmfNode") == 0) {
		io_node = i_dn;
		io_unit = i_dn;
	} else {
		LOG_ER("Wrong class %s, can only handle SaAmfSU,SaAmfComp and SaAmfNode", className);
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
void SmfUpgradeProcedure::execute()
{
	TRACE_ENTER();
	m_state->execute(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::executeInit()
{
	TRACE_ENTER();
	m_state->executeInit(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeStep()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::executeStep()
{
	TRACE_ENTER();
	m_state->executeStep(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::executeWrapup()
{
	TRACE_ENTER();
	m_state->executeWrapup(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::suspend()
{
	TRACE_ENTER();
	m_state->suspend(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::rollback()
{
	TRACE_ENTER();
	m_state->rollback(this);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfUpgradeProcedure::commit()
{
	TRACE_ENTER();
	m_state->commit(this);
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
	uns32 rc;

	/* Create the task */
	if ((rc =
	     m_NCS_TASK_CREATE((NCS_OS_CB) SmfSwapThread::main, (NCSCONTEXT) this, (char*) m_PROCEDURE_TASKNAME,
			       m_PROCEDURE_TASK_PRI, m_PROCEDURE_STACKSIZE, &m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_CREATE_FAILED");
		return -1;
	}
	if ((rc =m_NCS_TASK_DETACH(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_DETACH\n");
		return -1;
	}

	if ((rc = m_NCS_TASK_START(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_FAILED\n");
		return -1;
	}

	/* Wait for the thread to start */
	sem_wait(&m_semaphore);

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
	std::string si_name = getenv("SI_SWAP_SI");
	admOp.setDoDn(si_name);
	admOp.setDoId(SA_AMF_ADMIN_SI_SWAP);
	int max_swap_retry = atoi(getenv("SI_SWAP_MAX_RETRY"));
	int retryCnt = 0;
	int rc;
	while((rc = admOp.execute()) != SA_AIS_OK) {
		retryCnt++;
		if(retryCnt > max_swap_retry) {
			SmfProcStateExecFailed::instance()->changeState(m_proc, SmfProcStateExecFailed::instance());

			LOG_ER("SmfSwapThread::main: SA_AMF_ADMIN_SI_SWAP giving up after %d retries", retryCnt);
			CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
			evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
			evt->event.procResult.rc = PROCEDURE_FAILED;
			evt->event.procResult.procedure = m_proc;
			SmfCampaignThread::instance()->send(evt);
			break;
		}
		TRACE("SmfSwapThread::main: SI_AMF_ADMIN_SI_SWAP returns %d, wait 2 seconds and retry", rc);
		sleep(2);
	}

	TRACE_LEAVE();
}
