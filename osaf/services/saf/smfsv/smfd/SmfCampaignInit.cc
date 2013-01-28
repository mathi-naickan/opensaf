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
#include <saAis.h>
#include <immutil.h>
#include "logtrace.h"
#include "SmfCampaignInit.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaign.hh"
#include "SmfImmOperation.hh"
#include "SmfUpgradeAction.hh"
#include "SmfRollback.hh"
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
// Class SmfCampaignInit
// Purpose:
// Comments:
//================================================================================
SmfCampaignInit::SmfCampaignInit():
   m_addToImm(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfTargetNodeTemplate()
// ------------------------------------------------------------------------------
SmfCampaignInit::~SmfCampaignInit()
{
	std::list < SmfImmOperation * >::iterator addIter;
	std::list < SmfImmOperation * >::iterator addIterE;

	std::list <SmfCallback *>::iterator cbkIter;
	std::list <SmfCallback *>::iterator cbkIterE;

	addIter = SmfCampaignInit::m_addToImm.begin();
	addIterE = SmfCampaignInit::m_addToImm.end();

	while (addIter != addIterE) {
		delete((*addIter));
		addIter++;
	}

	cbkIter = SmfCampaignInit::m_callbackAtInit.begin();
	cbkIterE = SmfCampaignInit::m_callbackAtInit.end();

	while (cbkIter != cbkIterE) {
		delete((*cbkIter));
		cbkIter++;
	}

	cbkIter = SmfCampaignInit::m_callbackAtBackup.begin();
	cbkIterE = SmfCampaignInit::m_callbackAtBackup.end();

	while (cbkIter != cbkIterE) {
		delete((*cbkIter));
		cbkIter++;
	}

	cbkIter = SmfCampaignInit::m_callbackAtRollback.begin();
	cbkIterE = SmfCampaignInit::m_callbackAtRollback.end();

	while (cbkIter != cbkIterE) {
		delete((*cbkIter));
		cbkIter++;
	}
}

//------------------------------------------------------------------------------
// addAddToImm()
//------------------------------------------------------------------------------
void
SmfCampaignInit::addAddToImm(SmfImmOperation * i_operation)
{
	m_addToImm.push_back(i_operation);
}

//------------------------------------------------------------------------------
// getAddToImm()
//------------------------------------------------------------------------------
const std::list < SmfImmOperation * >&
SmfCampaignInit::getAddToImm()
{
	return m_addToImm;
}

//------------------------------------------------------------------------------
// addCallbackAtInit()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtInit(SmfCallback* i_cbk)
{
	m_callbackAtInit.push_back(i_cbk);
}

//------------------------------------------------------------------------------
// addCallbackAtBackup()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtBackup(SmfCallback* i_cbk)
{
	m_callbackAtBackup.push_back(i_cbk);
}

//------------------------------------------------------------------------------
// addCallbackAtRollback()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtRollback(SmfCallback* i_cbk)
{
	m_callbackAtRollback.push_back(i_cbk);
}

//------------------------------------------------------------------------------
// addCampInitAction()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCampInitAction(SmfUpgradeAction * i_action)
{
	m_campInitAction.push_back(i_action);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfCampaignInit::execute()
{
	TRACE_ENTER();

        SaAisErrorT result = SA_AIS_OK;

        LOG_NO("CAMP: Campaign init, start add to IMM (%zu)", m_addToImm.size());

        std::string addToImmRollbackCcbDn;
        addToImmRollbackCcbDn = "smfRollbackElement=AddToImmCcb,";
        addToImmRollbackCcbDn += SmfCampaignThread::instance()->campaign()->getDn();

        if ((result = smfCreateRollbackElement(addToImmRollbackCcbDn,
                                               SmfCampaignThread::instance()->getImmHandle())) != SA_AIS_OK) {
                LOG_ER("SmfCampaignInit failed to create addToImm rollback element %s, rc = %d", 
                       addToImmRollbackCcbDn.c_str(), result);
                return false;
        }

	if (m_addToImm.size() > 0) {
		SmfImmUtils immUtil;
                SmfRollbackCcb rollbackCcb(addToImmRollbackCcbDn,
                                           SmfCampaignThread::instance()->getImmHandle());

		if (immUtil.doImmOperations(m_addToImm, &rollbackCcb) != SA_AIS_OK) {
			LOG_ER("SmfCampaignInit add to IMM failed");
			return false;
		}

                if ((result = rollbackCcb.execute()) != SA_AIS_OK) {
			LOG_ER("SmfCampaignInit failed to store rollback CCB %d", result);
			return false;
                }
	}
	///////////////////////
	//Campaign init actions
	///////////////////////

	LOG_NO("CAMP: Campaign init, start init actions (%zu)", m_campInitAction.size());

        std::string initRollbackDn;
        initRollbackDn = "smfRollbackElement=CampInit,";
        initRollbackDn += SmfCampaignThread::instance()->campaign()->getDn();

        if ((result = smfCreateRollbackElement(initRollbackDn,
                                               SmfCampaignThread::instance()->getImmHandle())) != SA_AIS_OK) {
                LOG_ER("SmfCampaignInit failed to create campaign init rollback element %s, rc = %d", 
                       initRollbackDn.c_str(), result);
                return false;
        }

	std::list < SmfUpgradeAction * >::iterator upActiter;
	upActiter = m_campInitAction.begin();
	while (upActiter != m_campInitAction.end()) {
		SaAisErrorT rc = (*upActiter)->execute(SmfCampaignThread::instance()->getImmHandle(),
                                                       &initRollbackDn);
		if (rc != SA_AIS_OK) {
			LOG_ER("SmfCampaignInit init action %d failed, rc = %d", (*upActiter)->getId(), rc);
			return false;
		}
		upActiter++;
	}

	//////////////////
	//Callback at init
	//////////////////
	std::list < SmfCallback * >:: iterator cbkiter;
	cbkiter = m_callbackAtInit.begin();
	while (cbkiter != m_callbackAtInit.end()) {
		SaAisErrorT rc = (*cbkiter)->execute(initRollbackDn);
		if (rc == SA_AIS_ERR_FAILED_OPERATION) {
			LOG_ER("SmfCampaignInit callback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
			TRACE_LEAVE();
			return false;
		}
		cbkiter++;
	}


#if 0
	std::list < SmfCallbackOptions * >m_callbackAtInit;
#endif

	LOG_NO("CAMP: Campaign init completed");

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeBackup()
//------------------------------------------------------------------------------
bool 
SmfCampaignInit::executeBackup()
{
#if 0
	std::list < SmfCallbackOptions * >m_callbackAtBackup;
#endif
	///////////////////////
	//Callback at backup
	///////////////////////

	std::list < SmfCallback * >:: iterator cbkiter;
	std::string dn;
	cbkiter = m_callbackAtBackup.begin();
	while (cbkiter != m_callbackAtBackup.end()) {
		SaAisErrorT rc = (*cbkiter)->execute(dn);
		if (rc == SA_AIS_ERR_FAILED_OPERATION) {
			LOG_ER("SmfCampaignInit callbackAtBackup %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
			TRACE_LEAVE();
			return false;
		}
		cbkiter++;
	}
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfCampaignInit::rollback()
{
	LOG_NO("CAMP: Start rollback of campaign init actions (%zu)", m_campInitAction.size());

        SaAisErrorT rc;
        std::string initRollbackDn;
        initRollbackDn = "smfRollbackElement=CampInit,";
        initRollbackDn += SmfCampaignThread::instance()->campaign()->getDn();

	std::list < SmfUpgradeAction * >::reverse_iterator upActiter;

        /* For each action (in reverse order) call rollback */
	for (upActiter = m_campInitAction.rbegin(); upActiter != m_campInitAction.rend(); upActiter++) {
		rc = (*upActiter)->rollback(initRollbackDn);
		if (rc != SA_AIS_OK) {
			LOG_ER("SmfCampaignInit rollback of init action %d failed, rc = %d", (*upActiter)->getId(), rc);
			return false;
		}
		
	}

        LOG_NO("CAMP: Campaign init, rollback add to IMM (%zu)", m_addToImm.size());

        SmfImmUtils immUtil;
        std::string addToImmRollbackCcbDn;
        addToImmRollbackCcbDn = "smfRollbackElement=AddToImmCcb,";
        addToImmRollbackCcbDn += SmfCampaignThread::instance()->campaign()->getDn();

        SmfRollbackCcb rollbackCcb(addToImmRollbackCcbDn,
                                   SmfCampaignThread::instance()->getImmHandle());

        if ((rc = rollbackCcb.rollback()) != SA_AIS_OK) {
                LOG_ER("SmfCampaignInit failed to rollback add to IMM CCB %d", rc);
                return false;
	}

#if 0
	std::list < SmfCallbackOptions * >m_callbackAtRollback;
#endif

	LOG_NO("CAMP: Rollback of campaign init actions completed");
	return true;
}
//------------------------------------------------------------------------------
// executeCallbackAtRollback()
//------------------------------------------------------------------------------
bool
SmfCampaignInit::executeCallbackAtRollback()
{
	std::string dn;
	////////////////////////
	//Callback at rollback
	////////////////////////

	TRACE_ENTER();
	std::list < SmfCallback * >:: iterator cbkiter;
	cbkiter = m_callbackAtRollback.begin();
	while (cbkiter != m_callbackAtRollback.end()) {
		SaAisErrorT rc = (*cbkiter)->rollback(dn);
		LOG_NO("SmfCampaignInit callbackAtRollback returned %d", rc);
		if (rc == SA_AIS_ERR_FAILED_OPERATION) {
			LOG_ER("SmfCampaignInit callbackAtRollback %s failed, rc = %d", (*cbkiter)->getCallbackLabel().c_str(), rc);
			TRACE_LEAVE();
			return false;
		}
		cbkiter++;
	}
	TRACE_LEAVE();
	return true;
}
