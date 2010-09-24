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
#include "logtrace.h"
#include "SmfUpgradeProcedure.hh"
#include "SmfUpgradeStep.hh"
#include "SmfProcState.hh"
#include "SmfStepState.hh"
#include "SmfCampaignThread.hh"
#include "SmfProcedureThread.hh"
#include "SmfUpgradeAction.hh"
#include <immutil.h>
#include "SmfUtils.hh"
#include "smfd.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

extern struct ImmutilWrapperProfile immutilWrapperProfile;

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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// ------Base class SmfProcState------------------------------------------------
//
// SmfProcState default implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcState::getClassName()const
{
	return "SmfProcState";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
void 
SmfProcState::execute(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState::execute default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfProcState::executeInit(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState::executeInit default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeStep()
//------------------------------------------------------------------------------
void 
SmfProcState::executeStep(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState:: executeStep default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfProcState::executeWrapup(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState::executeWrapup default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
void 
SmfProcState::rollback(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState::rollback default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfProcState::suspend(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	TRACE("SmfProcState::suspend default implementation, send response.");

        CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
	evt->event.procResult.rc = PROCEDURE_SUSPENDED;
	evt->event.procResult.procedure = i_proc;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfProcState::commit(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_ER("SmfProcState::commit default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// changeState()
//------------------------------------------------------------------------------
void 
SmfProcState::changeState(SmfUpgradeProcedure * i_proc, SmfProcState * i_state)
{
	TRACE_ENTER();
	TRACE("SmfProcState::changeState");

	std::string newState = i_state->getClassName();
	std::string oldState = i_proc->m_state->getClassName();

	TRACE("SmfProcState::changeState old state=%s , new state=%s", oldState.c_str(), newState.c_str());
	i_proc->changeState(i_state);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfProcStateInitial implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SmfProcState *SmfProcStateInitial::s_instance = NULL;

SmfProcState *
SmfProcStateInitial::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfProcStateInitial;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcStateInitial::getClassName()const
{
	return "SmfProcStateInitial";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
void 
SmfProcStateInitial::execute(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();

	TRACE("SmfProcStateInitial::execute implementation");

	LOG_NO("PROC: Start upgrade procedure %s", i_proc->getDn().c_str());

	changeState(i_proc, SmfProcStateExecuting::instance());
	i_proc->executeInit();

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfProcStateExecuting implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfProcState *SmfProcStateExecuting::s_instance = NULL;

SmfProcState *
SmfProcStateExecuting::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfProcStateExecuting;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcStateExecuting::getClassName()const
{
	return "SmfProcStateExecuting";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
void 
SmfProcStateExecuting::execute(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();

	TRACE("SmfProcStateExecuting::execute, Do some checking");

        /* Execute init actions */
        this->executeInit(i_proc);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfProcStateExecuting::executeInit(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	LOG_NO("PROC: Running procedure init actions");

	TRACE("SmfProcStateExecuting::executeInit, Calculate steps");
        if( !i_proc->calculateSteps() ) {
                changeState(i_proc, SmfProcStateExecFailed::instance());
                LOG_ER("SmfProcStateExecuting::executeInit:Step calculation failes");
                CAMPAIGN_EVT *errevt = new CAMPAIGN_EVT();
                errevt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                errevt->event.procResult.rc = PROCEDURE_FAILED;
                errevt->event.procResult.procedure = i_proc;
                SmfCampaignThread::instance()->send(errevt);
                TRACE_LEAVE();
                return;
        }

	TRACE("SmfProcStateExecuting::executeInit, Create step objects in IMM");
        if( !i_proc->createImmSteps() ) {
                changeState(i_proc, SmfProcStateExecFailed::instance());
                LOG_ER("SmfProcStateExecuting::executeInit:createImmSteps in IMM failes");
                CAMPAIGN_EVT *errevt = new CAMPAIGN_EVT();
                errevt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                errevt->event.procResult.rc = PROCEDURE_FAILED;
                errevt->event.procResult.procedure = i_proc;
                SmfCampaignThread::instance()->send(errevt);
                TRACE_LEAVE();
                return;
        }

	TRACE("SmfProcStateExecuting::executeInit: Execute init actions");
	std::vector < SmfUpgradeAction * >::iterator iter;
	for (iter = i_proc->m_procInitAction.begin(); iter != i_proc->m_procInitAction.end(); ++iter) {
                if ((*iter)->execute() != SA_AIS_OK) {
			changeState(i_proc, SmfProcStateExecFailed::instance());
                        LOG_ER("SmfProcStateExecuting::executeInit:init action %d failed", (*iter)->getId());
                        CAMPAIGN_EVT *errevt = new CAMPAIGN_EVT();
                        errevt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                        errevt->event.procResult.rc = PROCEDURE_FAILED;
                        errevt->event.procResult.procedure = i_proc;
                        SmfCampaignThread::instance()->send(errevt);
                        TRACE_LEAVE();
                        return;
                }
	}

	TRACE("SmfProcStateExecuting::executeInit, Procedure init actions completed");
        this->executeStep(i_proc);

	TRACE_LEAVE();
        return;
}

//------------------------------------------------------------------------------
// executeStep()
//------------------------------------------------------------------------------
void 
SmfProcStateExecuting::executeStep(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	TRACE("SmfProcStateExecuting::executeStep: Procedure=%s", i_proc->getProcName().c_str());

	/* Find and execute first step in state Initial. */
	std::vector < SmfUpgradeStep * >::iterator iter;
	iter = i_proc->m_procSteps.begin();
	while (iter != i_proc->m_procSteps.end()) {
		if ((*iter)->getState() == SA_SMF_STEP_INITIAL) {
			SmfStepResultT stepResult;

			LOG_NO("PROC: Start step %s", (*iter)->getDn().c_str());

			/* Executing the step */
			stepResult = (*iter)->execute();

			/* Check step result */
			if (stepResult == SMF_STEP_COMPLETED) {
				TRACE ("Step %s completed, sending PROCEDURE_EVT_EXECUTE_STEP",
                                       (*iter)->getRdn().c_str());

				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_EXECUTE_STEP;
				i_proc->getProcThread()->send(evt);
				TRACE_LEAVE();
				return;
			} else if (stepResult == SMF_STEP_SWITCHOVER) {
				LOG_NO ("PROC: Step %s needs switchover, let other controller take over",
                                       (*iter)->getRdn().c_str());

				i_proc->switchOver();

				TRACE_LEAVE();
				return;
			} else {
				changeState(i_proc, SmfProcStateExecFailed::instance());

				LOG_ER("Step %s in procedure %s failed, send procedure failed event",
				      (*iter)->getRdn().c_str(), i_proc->getProcName().c_str());

				CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
				evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
				evt->event.procResult.rc = PROCEDURE_FAILED;
				evt->event.procResult.procedure = i_proc;
				SmfCampaignThread::instance()->send(evt);
				TRACE_LEAVE();
				return;
			}
		}
		iter++;
	}

	// Execute the online remove commands.
	// If the OpenSAF proprietary step actions was selected (by the nodeBundleActCmd attribute in the SmfConfig class)
	// the offline remove scripts was run within the steps. Check if the proprietary step actions was choosen.

	if((smfd_cb->nodeBundleActCmd == NULL) || (strcmp(smfd_cb->nodeBundleActCmd,"") == 0)) {
		//Run all online remove scripts for all bundles (which does not require restart) listed in the upgrade steps
		iter = i_proc->m_procSteps.begin();
		while (iter != i_proc->m_procSteps.end()) {

			TRACE("SmfProcStateExecuting::executeStep: Execute OnlineRemove for the bundles to remove");
			/* Online uninstallation of old software */
			LOG_NO("PROC: Online uninstallation of old software");

			/* Run only remove scripts for those bundles which does NOT require reboot to online uninstall */
			/* Find out which bundles to be removed here. Bundles which requires reboot have already has   */
			/* their online remove scripts executed in the step.                                           */

			/* Create the list of bundles NOT restarted */
			SmfImmUtils immutil;
			SaImmAttrValuesT_2 ** attributes;
			std::list < SmfBundleRef > nonRestartBundles;
			const std::list < SmfBundleRef > &removeList = (*iter)->getSwRemoveList();
			std::list< SmfBundleRef >::const_iterator bundleIter = removeList.begin();
			while (bundleIter != removeList.end()) {
				/* Read the saSmfBundleRemoveOfflineScope to detect if the bundle shall be included */
				if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
					LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
					CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
					evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
					evt->event.procResult.rc = PROCEDURE_FAILED;
					evt->event.procResult.procedure = i_proc;
					SmfCampaignThread::instance()->send(evt);
					TRACE_LEAVE();
					return;
				}
				const SaUint32T* scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
									       "saSmfBundleRemoveOfflineScope",
									       0);
				/* Include only bundles not need reboot */
				if ((scope != NULL) && (*scope != SA_SMF_CMD_SCOPE_PLM_EE)) {
					TRACE("SmfProcStateExecuting::executeStep:Include the SW bundle %s to remove list", 
					      (*bundleIter).getBundleDn().c_str());

					nonRestartBundles.push_back((*bundleIter));
				}

				bundleIter++;
			}

			/* Run the online remove scripts for the bundles NOT restarted */
			if ((*iter)->onlineRemoveBundlesUserList((*iter)->getSwNode(), nonRestartBundles) == false) {
				changeState(i_proc, SmfProcStateExecFailed::instance());
				LOG_ER("SmfProcStateExecuting::executeStep:Failed to online remove bundles");
				CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
				evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
				evt->event.procResult.rc = PROCEDURE_FAILED;
				evt->event.procResult.procedure = i_proc;
				SmfCampaignThread::instance()->send(evt);
				TRACE_LEAVE();
				return;
			}

			/* Delete SaAmfNodeSwBundle objects for ALL bundles in the step*/
			LOG_NO("PROC: Delete SaAmfNodeSwBundle objects");
			if ((*iter)->deleteSaAmfNodeSwBundles((*iter)->getSwNode()) == false) {
				changeState(i_proc, SmfProcStateExecFailed::instance());
				LOG_ER("SmfProcStateExecuting::executeStep:Failed to delete SaAmfNodeSwBundle object");
				CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
				evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
				evt->event.procResult.rc = PROCEDURE_FAILED;
				evt->event.procResult.procedure = i_proc;
				SmfCampaignThread::instance()->send(evt);
				TRACE_LEAVE();
				return;
			}
		
			iter++;
		}
	}

	/* All steps are executed, continue the procedure execution */
	TRACE("SmfProcStateExecuting::executeStep, All steps in procedure %s executed", i_proc->getProcName().c_str());

        this->executeWrapup(i_proc);
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfProcStateExecuting::executeWrapup(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	TRACE("SmfProcStateExecuting::executeWrapup actions");

	LOG_NO("PROC: Running procedure wrapup actions");

	std::vector < SmfUpgradeAction * >::iterator iter;
	for (iter = i_proc->m_procWrapupAction.begin(); iter != i_proc->m_procWrapupAction.end(); ++iter) {
		if ((*iter)->execute() != SA_AIS_OK) {
			changeState(i_proc, SmfProcStateExecFailed::instance());
			LOG_ER("wrapup action %d failed", (*iter)->getId());
			TRACE("SmfProcStateExecuting::executeWrapup, Sending CAMPAIGN_EVT_PROCEDURE_RC event to campaign thread. RC=PROCEDURE_FAILED");
                        CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                        evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                        evt->event.procResult.rc = PROCEDURE_FAILED;
                        evt->event.procResult.procedure = i_proc;
                        SmfCampaignThread::instance()->send(evt);
                        TRACE_LEAVE();
                        return;
		}
	}

	TRACE("SmfProcStateExecuting::executeWrapup, Wrapup actions finished for procedure %s ",
	      i_proc->getProcName().c_str());
	TRACE("SmfProcStateExecuting::executeWrapup, Sending CAMPAIGN_EVT_PROCEDURE_RC event to campaign thread. RC=PROCEDURE_COMPLETED");

	LOG_NO("PROC: Upgrade procedure completed %s", i_proc->getProcName().c_str());
	changeState(i_proc, SmfProcStateExecutionCompleted::instance());
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
	evt->event.procResult.rc = PROCEDURE_COMPLETED;
	evt->event.procResult.procedure = i_proc;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfProcStateExecuting::suspend(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	TRACE("SmfProcStateExecuting::suspend implementation");

	changeState(i_proc, SmfProcStateExecSuspended::instance());
	LOG_NO("PROC: Suspended procedure %s", i_proc->getDn().c_str());

	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
	evt->event.procResult.rc = PROCEDURE_SUSPENDED;
	evt->event.procResult.procedure = i_proc;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfProcStateExecutionCompleted implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfProcState *SmfProcStateExecutionCompleted::s_instance = NULL;

SmfProcState *
SmfProcStateExecutionCompleted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfProcStateExecutionCompleted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcStateExecutionCompleted::getClassName()const
{
	return "SmfProcStateExecutionCompleted";
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfProcStateExecutionCompleted::commit(SmfUpgradeProcedure * i_proc)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT objectName;
        SmfImmUtils immUtil;

	TRACE_ENTER();
	TRACE("SmfProcStateExecutionCompleted::commit implementation");

	/* Remove upgrade procedure object (and the whole subtree) from IMM 
	   The children objects in the subtree are:
	   -SaSmfStep
	   -SaSmfActivationUnit, 
	   -SaSmfDeactivationUnit 
	   -SaSmfImageNodes 
	   -OpenSafSmfSingleStepInfo
	*/

	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;

	objectName.length = i_proc->getDn().length();
	strncpy((char *)objectName.value, i_proc->getDn().c_str(), objectName.length);
	objectName.value[objectName.length] = 0;

	rc = immutil_saImmOiRtObjectDelete(i_proc->getProcThread()->getImmHandle(),	//The OI handle
					   &objectName);

	if (rc != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiRtObjectDelete returned %u for %s", rc, i_proc->getDn().c_str());
	}

	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfProcStateExecSuspended implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfProcState *SmfProcStateExecSuspended::s_instance = NULL;

SmfProcState *
SmfProcStateExecSuspended::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfProcStateExecSuspended;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcStateExecSuspended::getClassName()const
{
	return "SmfProcStateExecSuspended";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
void 
SmfProcStateExecSuspended::execute(SmfUpgradeProcedure * i_proc)
{
	TRACE_ENTER();
	TRACE("SmfProcStateExecSuspended::execute implementation");
	changeState(i_proc, SmfProcStateExecuting::instance());

	LOG_NO("PROC: Continue executing suspended procedure %s", i_proc->getDn().c_str());

	PROCEDURE_EVT *evt = new PROCEDURE_EVT();
	evt->type = PROCEDURE_EVT_EXECUTE_STEP;
	i_proc->getProcThread()->send(evt);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeStep()
//------------------------------------------------------------------------------
void 
SmfProcStateExecSuspended::executeStep(SmfUpgradeProcedure * i_proc)
{
	/* We will get executeStep at suspend so just ignore it */
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfProcStateExecFailed implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfProcState *SmfProcStateExecFailed::s_instance = NULL;

SmfProcState *
SmfProcStateExecFailed::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfProcStateExecFailed;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfProcStateExecFailed::getClassName()const
{
	return "SmfProcStateExecFailed";
}
