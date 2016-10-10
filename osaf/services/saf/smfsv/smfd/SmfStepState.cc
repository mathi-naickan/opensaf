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
#include "SmfUpgradeMethod.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfUpgradeCampaign.hh"
#include "saSmf.h"
#include "SmfUpgradeStep.hh"
#include "SmfStepState.hh"
#include "SmfStepTypes.hh"
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// ------Base class SmfStepState------------------------------------------------
//
// SmfStepState default implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepState::getClassName()const
{
	return "SmfStepState";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepState::execute(SmfUpgradeStep * i_step)
{
	return SMF_STEP_NULL; /* Nothing done */
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepState::rollback(SmfUpgradeStep * i_step)
{
	return SMF_STEP_NULL; /* Nothing done */
}

//------------------------------------------------------------------------------
// changeState()
//------------------------------------------------------------------------------
void 
SmfStepState::changeState(SmfUpgradeStep * i_step, SmfStepState * i_state)
{
	TRACE_ENTER();
	TRACE("SmfStepState::changeState");

	std::string newState = i_state->getClassName();
	std::string oldState = i_step->m_state->getClassName();

	TRACE("SmfStepState::changeState old state=%s , new state=%s", oldState.c_str(), newState.c_str());
	i_step->changeState(i_state);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateInitial implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SmfStepState *SmfStepStateInitial::s_instance = NULL;

SmfStepState *
SmfStepStateInitial::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateInitial;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateInitial::getClassName()const
{
	return "SmfStepStateInitial";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateInitial::execute(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();

	TRACE("Start executing upgrade step %s", i_step->getDn().c_str());

        if (i_step->calculateStepType() != SA_AIS_OK) {
                LOG_ER("SmfStepStateInitial: Failed to calculate step type");
                changeState(i_step, SmfStepStateFailed::instance());
                TRACE_LEAVE();
                return SMF_STEP_FAILED;
        }

        if (i_step->getSwitchOver() == true) {
                TRACE("Switch over is needed in this step");
                TRACE_LEAVE();
                return SMF_STEP_SWITCHOVER;
        }

        i_step->setRetryCount(0);
	changeState(i_step, SmfStepStateExecuting::instance());

	TRACE_LEAVE();
	return SMF_STEP_CONTINUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateExecuting implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateExecuting::s_instance = NULL;

SmfStepState *
SmfStepStateExecuting::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateExecuting;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateExecuting::getClassName()const
{
	return "SmfStepStateExecuting";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateExecuting::execute(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	TRACE("Executing step %s", i_step->getDn().c_str());

	// Two sets of step actions are available
	// The first set is according to the SMF specification SMF A.01.02
	// The second set is an OpeSAF proprietary set.
	// This set is triggerd by the nodeBundleActCmd attribute in the SmfConfig class.
	// If the nodeBundleActCmd attribute is set, the value shall point out a command
	// which shall be executed to to activate the software installed/removed in the 
	// sw bundle installation/removal scripts. The activation is done once for the step.

        SmfStepType* stepType = i_step->getStepType();
        if (stepType == NULL) {
                /* We could have been restarted in this state e.g. at cluster reboot */
                if (SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->getProcExecutionMode()
		    == SMF_MERGE_TO_SINGLE_STEP) {
                        // If SMF_MERGE_TO_SINGLE_STEP is configured we can only come here after a cluster reboot
                        if (i_step->calculateStepTypeForMergedSingle() != SA_AIS_OK) {
                                LOG_ER("Failed to recalculate step type for merged single step when \
                                        trying to continue step %s", i_step->getDn().c_str());
                                changeState(i_step, SmfStepStateFailed::instance());
                                return SMF_STEP_FAILED;
                        }
                } else {
                        if (i_step->calculateStepType() != SA_AIS_OK) {
                                LOG_ER("Failed to recalculate step type when trying to continue step %s", i_step->getDn().c_str());
                                changeState(i_step, SmfStepStateFailed::instance());
                                return SMF_STEP_FAILED;
                        }
                }
                stepType = i_step->getStepType();
        }

        if (stepType == NULL) {
                LOG_ER("Unknown step type when trying to execute step %s", i_step->getDn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return SMF_STEP_FAILED;
        }

        if (stepType->execute() == false) {
                LOG_ER("Step execution failed, Try undoing the step");
                changeState(i_step, SmfStepStateUndoing::instance());
                return SMF_STEP_CONTINUE; /* Continue in new state */
        }
        else {
                changeState(i_step, SmfStepStateCompleted::instance());
                return SMF_STEP_COMPLETED;
        }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateCompleted implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateCompleted::s_instance = NULL;

SmfStepState *
SmfStepStateCompleted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateCompleted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateCompleted::getClassName()const
{
	return "SmfStepStateCompleted";
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateCompleted::rollback(SmfUpgradeStep * i_step)
{
        TRACE_ENTER();

	TRACE("Start rollback upgrade step %s", i_step->getDn().c_str());

        /* We could have switched over at previous steps so recalculate step type just in case*/
        if (i_step->calculateStepType() != SA_AIS_OK) {
                LOG_ER("Failed to calculate step type at rollback");
                changeState(i_step, SmfStepStateRollbackFailed::instance());
                TRACE_LEAVE();
                return SMF_STEP_ROLLBACKFAILED;
        }

        if (i_step->getSwitchOver() == true) {
                TRACE("Switch over is needed when rolling back this step");
                TRACE_LEAVE();
                return SMF_STEP_SWITCHOVER;
        }

        i_step->setRetryCount(0);
	changeState(i_step, SmfStepStateRollingBack::instance());

	TRACE_LEAVE();
	return SMF_STEP_CONTINUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateFailed implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateFailed::s_instance = NULL;

SmfStepState *
SmfStepStateFailed::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateFailed;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateFailed::getClassName()const
{
	return "SmfStepStateFailed";
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateUndoing implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateUndoing::s_instance = NULL;

SmfStepState *
SmfStepStateUndoing::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateUndoing;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateUndoing::getClassName()const
{
	return "SmfStepStateUndoing";
}


//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateUndoing::execute(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	LOG_NO("SmfStepStateUndoing::execute start undoing step.");

        SmfStepType* stepType = i_step->getStepType();
        if (stepType == NULL) {
                LOG_ER("Unknown step type when trying to undo step %s", i_step->getDn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return SMF_STEP_FAILED;
        }

        if (stepType->rollback() == false) {
                LOG_ER("Step undoing failed");
                changeState(i_step, SmfStepStateFailed::instance());
                return SMF_STEP_FAILED;
        }

        i_step->setRetryCount(i_step->getRetryCount() + 1);
        if (i_step->getRetryCount() > i_step->getMaxRetry()) {
        	changeState(i_step, SmfStepStateUndone::instance());
                LOG_NO("SmfStepStateUndoing::execute step undone.");
                return SMF_STEP_UNDONE;
        }
        else {
        	changeState(i_step, SmfStepStateExecuting::instance());
                LOG_NO("SmfStepStateUndoing::execute step undone OK, try step again.");
                return SMF_STEP_CONTINUE; /* Continue in next state */
        }

	TRACE_LEAVE();
	return SMF_STEP_FAILED;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateUndone implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateUndone::s_instance = NULL;

SmfStepState *
SmfStepStateUndone::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateUndone;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateUndone::getClassName()const
{
	return "SmfStepStateUndone";
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateUndone::execute(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();

        if (i_step->calculateStepType() != SA_AIS_OK) {
                LOG_ER("SmfStepStateUndone: Failed to calculate step type");
                changeState(i_step, SmfStepStateFailed::instance());
                TRACE_LEAVE();
                return SMF_STEP_FAILED;
        }

        if (i_step->getSwitchOver() == true) {
                TRACE("Switch over is needed in this step");
                TRACE_LEAVE();
                return SMF_STEP_SWITCHOVER;
        }

        i_step->setRetryCount(0); /* Reset the retry counter */
	changeState(i_step, SmfStepStateExecuting::instance());

        TRACE_LEAVE();
	return SMF_STEP_CONTINUE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateRollingBack implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateRollingBack::s_instance = NULL;

SmfStepState *
SmfStepStateRollingBack::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateRollingBack;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateRollingBack::getClassName()const
{
	return "SmfStepStateRollingBack";
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateRollingBack::rollback(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
        TRACE("Rolling back step %s", i_step->getDn().c_str());

	// Two sets of step actions are available
	// The first set is according to the SMF specification SMF A.01.02
	// The second set is an OpenSAF proprietary set.
	// This set is triggerd by the nodeBundleActCmd attribute in the SmfConfig class.
	// If the nodeBundleActCmd attribute is set, the value shall point out a command
	// which shall be executed to activate the software installed/removed in the 
	// sw bundle installation/removal scripts. The activation is done once for the step.

        SmfStepType* stepType = i_step->getStepType();
        if (stepType == NULL) {
                /* We could have been restarted in this state e.g. at cluster reboot */
                if (i_step->calculateStepType() != SA_AIS_OK) {
                        LOG_ER("Failed to recalculate step type when trying to continue step %s", i_step->getDn().c_str());
                        changeState(i_step, SmfStepStateFailed::instance());
                        return SMF_STEP_FAILED;
                }
                stepType = i_step->getStepType();
        }

        if (stepType == NULL) {
                LOG_ER("Unknown step type when trying to rollback step %s", i_step->getDn().c_str());
                changeState(i_step, SmfStepStateRollbackFailed::instance());
                return SMF_STEP_ROLLBACKFAILED;
        }

        if (stepType->rollback() == false) {
                LOG_ER("Step rollback failed, Try redoing the step");
                changeState(i_step, SmfStepStateUndoingRollback::instance());
                return SMF_STEP_CONTINUE; /* Continue in next state */
        }
        else {
                changeState(i_step, SmfStepStateRolledBack::instance());
                return SMF_STEP_ROLLEDBACK; 
        }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateRolledBack implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateRolledBack::s_instance = NULL;

SmfStepState *
SmfStepStateRolledBack::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateRolledBack;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateRolledBack::getClassName()const
{
	return "SmfStepStateRolledBack";
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateUndoingRollback implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateUndoingRollback::s_instance = NULL;

SmfStepState *
SmfStepStateUndoingRollback::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateUndoingRollback;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateUndoingRollback::getClassName()const
{
	return "SmfStepStateUndoingRollback";
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfStepResultT 
SmfStepStateUndoingRollback::rollback(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	LOG_NO("SmfStepStateUndoingRollback::rollback try to undo the rollback.");

        SmfStepType* stepType = i_step->getStepType();
        if (stepType == NULL) {
                LOG_ER("Unknown step type when trying to undo rollback step %s", i_step->getDn().c_str());
                changeState(i_step, SmfStepStateRollbackFailed::instance());
                return SMF_STEP_ROLLBACKFAILED;
        }

        if (stepType->execute() == false) {
                LOG_ER("SmfStepStateUndoingRollback::rollback Step undoing rollback failed");
                changeState(i_step, SmfStepStateRollbackFailed::instance());
                return SMF_STEP_ROLLBACKFAILED;
        }

        i_step->setRetryCount(i_step->getRetryCount() + 1);
        if (i_step->getRetryCount() > i_step->getMaxRetry()) {
        	changeState(i_step, SmfStepStateRollbackUndone::instance());
                LOG_NO("SmfStepStateUndoingRollback::rollback step undone.");
                return SMF_STEP_ROLLBACKUNDONE;
        }
        else {
        	changeState(i_step, SmfStepStateRollingBack::instance());
                LOG_NO("SmfStepStateUndoingRollback::rollback step rollback undone OK, try rollback again.");
                return SMF_STEP_CONTINUE; /* Continue in next state */
        }

	TRACE_LEAVE();
	return SMF_STEP_ROLLBACKFAILED;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateRollbackUndone implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateRollbackUndone::s_instance = NULL;

SmfStepState *
SmfStepStateRollbackUndone::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateRollbackUndone;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateRollbackUndone::getClassName()const
{
	return "SmfStepStateRollbackUndone";
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepStateRollbackFailed implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfStepState *SmfStepStateRollbackFailed::s_instance = NULL;

SmfStepState *
SmfStepStateRollbackFailed::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfStepStateRollbackFailed;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
std::string 
SmfStepStateRollbackFailed::getClassName()const
{
	return "SmfStepStateRollbackFailed";
}

