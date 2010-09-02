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
#include "logtrace.h"
#include "SmfCampaignInit.hh"
#include "SmfImmOperation.hh"
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

	addIter = SmfCampaignInit::m_addToImm.begin();
	addIterE = SmfCampaignInit::m_addToImm.end();

	while (addIter != addIterE) {
		delete((*addIter));
		addIter++;
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

#if 0
//------------------------------------------------------------------------------
// addCallbackAtInit()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtInit(SmfCallbackOptions * i_option)
{
	m_callbackAtInit.push_back(i_option);
}

//------------------------------------------------------------------------------
// addCallbackAtBackup()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtBackup(SmfCallbackOptions * i_option)
{
	m_callbackAtBackup.push_back(i_option);
}

//------------------------------------------------------------------------------
// addCallbackAtRollback()
//------------------------------------------------------------------------------
void 
SmfCampaignInit::addCallbackAtRollback(SmfCallbackOptions * i_option)
{
	m_callbackAtRollback.push_back(i_option);
}
#endif

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

	LOG_NO("CAMP: Campaign init, start add to IMM (%zu)", m_addToImm.size());

	if (m_addToImm.size() > 0) {
		SmfImmUtils immUtil;
		if (immUtil.doImmOperations(m_addToImm) == false) {
			LOG_ER("SmfCampaignInit add to IMM failed");
			return false;
		}
	}
	///////////////////////
	//Campaign init actions
	///////////////////////

	LOG_NO("CAMP: Campaign init, start init actions (%zu)", m_campInitAction.size());
	std::list < SmfUpgradeAction * >::iterator upActiter;
	upActiter = m_campInitAction.begin();
	while (upActiter != m_campInitAction.end()) {
		int rc = (*upActiter)->execute();
		if (rc != 0) {
			LOG_ER("SmfCampaignInit init action %d failed, rc = %d", (*upActiter)->getId(), rc);
			return false;
		}
		upActiter++;
	}

	//////////////////
	//Callback at init
	//////////////////
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
	return true;
}

//------------------------------------------------------------------------------
// executeRollback()
//------------------------------------------------------------------------------
bool 
SmfCampaignInit::executeRollback()
{
#if 0
	std::list < SmfCallbackOptions * >m_callbackAtRollback;
	std::list < SmfAction * >m_campInitAction;
#endif

	return true;
}
