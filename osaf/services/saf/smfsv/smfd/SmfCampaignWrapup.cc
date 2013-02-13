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
#include <immutil.h>
#include <saf_error.h>
#include "logtrace.h"
#include "SmfCampaign.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaignWrapup.hh"
#include "SmfImmOperation.hh"
#include "SmfRollback.hh"
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
// Class SmfCampaignWrapup
// Purpose:
// Comments:
//================================================================================
SmfCampaignWrapup::SmfCampaignWrapup():
   m_removeFromImm(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfTargetNodeTemplate()
// ------------------------------------------------------------------------------
SmfCampaignWrapup::~SmfCampaignWrapup()
{
	std::list < SmfImmOperation * >::iterator iter;
	std::list < SmfImmOperation * >::iterator iterE;

	std::list < SmfCallback * >::iterator cbkIter;
	std::list < SmfCallback * >::iterator cbkIterE;

	iter = SmfCampaignWrapup::m_removeFromImm.begin();
	iterE = SmfCampaignWrapup::m_removeFromImm.end();

	while (iter != iterE) {
		delete((*iter));
		iter++;
	}

	cbkIter = SmfCampaignWrapup::m_callbackAtCommit.begin();
	cbkIterE = SmfCampaignWrapup::m_callbackAtCommit.end();

	while (cbkIter != cbkIterE) {
		delete((*cbkIter));
		cbkIter++;
	}
}

//------------------------------------------------------------------------------
// addRemoveFromImm()
//------------------------------------------------------------------------------
void
SmfCampaignWrapup::addRemoveFromImm(SmfImmOperation * i_operation)
{
	m_removeFromImm.push_back(i_operation);
}

//------------------------------------------------------------------------------
// addCallbackAtCommit()
//------------------------------------------------------------------------------
void 
SmfCampaignWrapup::addCallbackAtCommit(SmfCallback* i_cbk)
{
	m_callbackAtCommit.push_back(i_cbk);
}

//------------------------------------------------------------------------------
// addCampCompleteAction()
//------------------------------------------------------------------------------
void 
SmfCampaignWrapup::addCampCompleteAction(SmfUpgradeAction * i_action)
{
	m_campCompleteAction.push_back(i_action);
}

//------------------------------------------------------------------------------
// addCampWrapupAction()
//------------------------------------------------------------------------------
void 
SmfCampaignWrapup::addCampWrapupAction(SmfUpgradeAction * i_action)
{
	m_campWrapupAction.push_back(i_action);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfCampaignWrapup::executeCampWrapup()
{
	TRACE_ENTER();
        bool rc = true;

	///////////////////////
	//Callback at commit
	///////////////////////
	std::list < SmfCallback * >:: iterator cbkiter;
	std::string dn;
	cbkiter = m_callbackAtCommit.begin();
	while (cbkiter != m_callbackAtCommit.end()) {
		SaAisErrorT rc = (*cbkiter)->execute(dn);
		if (rc == SA_AIS_ERR_FAILED_OPERATION) {
			LOG_NO("SmfCampaignCommit callback %s failed, rc=%s", (*cbkiter)->getCallbackLabel().c_str(), saf_error(rc));
		}
		cbkiter++;
	}

	// The actions below are trigged by a campaign commit operation.
	// The campaign will enter state "commited" even if some actions fails.
	// Just log errors and try to execute as many operations as possible.

	LOG_NO("CAMP: Campaign wrapup, start wrapup actions (%zu)", m_campWrapupAction.size());
	std::list < SmfUpgradeAction * >::iterator iter;
	for (iter = m_campWrapupAction.begin(); iter != m_campWrapupAction.end(); ++iter) {
		if ((*iter)->execute(0) != SA_AIS_OK) {
			LOG_NO("SmfCampaignWrapup campWrapupActions %d failed", (*iter)->getId());
		}
	}

	LOG_NO("CAMP: Campaign wrapup, start remove from IMM (%zu)", m_removeFromImm.size());
	if (m_removeFromImm.size() > 0) {
		SmfImmUtils immUtil;
		if (immUtil.doImmOperations(m_removeFromImm) != SA_AIS_OK) {
			LOG_NO("SmfCampaignWrapup remove from IMM failed");
		}
	}

	LOG_NO("CAMP: Campaign wrapup actions completed");

	TRACE_LEAVE();

	return rc;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfCampaignWrapup::rollbackCampWrapup()
{
	TRACE_ENTER();
        bool rc = true;

	///////////////////////
	//Callback at commit
	///////////////////////
        
	LOG_NO("CAMP: Campaign wrapup, rollback campaign commit callbacks");
	std::list < SmfCallback * >:: iterator cbkiter;
	std::string dn;
	cbkiter = m_callbackAtCommit.begin();
	while (cbkiter != m_callbackAtCommit.end()) {
		SaAisErrorT rc = (*cbkiter)->rollback(dn);
		if (rc == SA_AIS_ERR_FAILED_OPERATION) {
			LOG_NO("SmfCampaignCommit rollback of callback %s failed (rc=%s), ignoring", 
                               (*cbkiter)->getCallbackLabel().c_str(), saf_error(rc));
		}
		cbkiter++;
	}

	// The actions below are trigged by a campaign commit operation after rollback.
	// The campaign will enter state "rollback_commited" even if some actions fails.
	// Just log errors and try to execute as many operations as possible.
        // 

	LOG_NO("CAMP: Campaign wrapup, rollback wrapup actions (%zu)", m_campWrapupAction.size());
	std::list < SmfUpgradeAction * >::iterator iter;
	for (iter = m_campWrapupAction.begin(); iter != m_campWrapupAction.end(); ++iter) {
                SmfImmCcbAction* immCcb = NULL;
                if ((immCcb = dynamic_cast<SmfImmCcbAction*>(*iter)) != NULL) {
                        /* Since noone of these IMM CCB has been executed it's no point
                           in trying to roll them back */
			TRACE("SmfCampaignWrapup skipping immCcb rollback %d", 
                               (*iter)->getId()); 
                        continue;
                }
		if ((*iter)->rollback(dn) != SA_AIS_OK) {
			LOG_NO("SmfCampaignWrapup rollback campWrapupAction %d failed, ignoring", 
                               (*iter)->getId());
		}
	}

        /* Since the removeFromImm is made for the upgrade case there is 
           no point in trying them at rollback */

	LOG_NO("CAMP: Campaign wrapup rollback actions completed");

	TRACE_LEAVE();

	return rc;
}

//------------------------------------------------------------------------------
// executeComplete()
//------------------------------------------------------------------------------
bool 
SmfCampaignWrapup::executeCampComplete()
{
	TRACE_ENTER();

	//Campaign wrapup complete actions
	LOG_NO("CAMP: Start campaign complete actions (%zu)", m_campCompleteAction.size());
        SaAisErrorT result;
        std::string completeRollbackDn;
        completeRollbackDn = "smfRollbackElement=CampComplete,";
        completeRollbackDn += SmfCampaignThread::instance()->campaign()->getDn();

        if ((result = smfCreateRollbackElement(completeRollbackDn,
                                               SmfCampaignThread::instance()->getImmHandle())) != SA_AIS_OK) {
                LOG_ER("SmfCampaignWrapup failed to create campaign complete rollback element %s, rc=%s", 
                       completeRollbackDn.c_str(), saf_error(result));
                return false;
        }

	std::list < SmfUpgradeAction * >::iterator iter;
	iter = m_campCompleteAction.begin();
	while (iter != m_campCompleteAction.end()) {
		if ((result = (*iter)->execute(SmfCampaignThread::instance()->getImmHandle(),
                                               &completeRollbackDn)) != SA_AIS_OK) {
			LOG_ER("SmfCampaignWrapup campCompleteAction %d failed, rc=%s", (*iter)->getId(), saf_error(result));
			return false;
		}
		iter++;
	}

	LOG_NO("CAMP: Campaign complete actions completed");

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// rollbackCampComplete()
//------------------------------------------------------------------------------
bool 
SmfCampaignWrapup::rollbackCampComplete()
{
	LOG_NO("CAMP: Start rollback campaign complete actions (%zu)", m_campCompleteAction.size());

        SaAisErrorT rc;
        std::string completeRollbackDn;

        TRACE("Start rollback of all complete actions ");

        completeRollbackDn = "smfRollbackElement=CampComplete,";
        completeRollbackDn += SmfCampaignThread::instance()->campaign()->getDn();

	std::list < SmfUpgradeAction * >::reverse_iterator upActiter;

        TRACE("Start rollback of all complete actions (in reverse order)");
        /* For each action (in reverse order) call rollback */
	for (upActiter = m_campCompleteAction.rbegin(); upActiter != m_campCompleteAction.rend(); upActiter++) {
		rc = (*upActiter)->rollback(completeRollbackDn);
		if (rc != SA_AIS_OK) {
			LOG_ER("SmfCampaignWrapup rollback of complete action %d failed, rc=%s", (*upActiter)->getId(), saf_error(rc));
			return false;
		}
		
	}

	LOG_NO("CAMP: Rollback of campaign complete actions completed");
	return true;
}

