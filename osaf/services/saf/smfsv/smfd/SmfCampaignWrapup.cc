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
#include "SmfCampaign.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaignWrapup.hh"
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

	iter = SmfCampaignWrapup::m_removeFromImm.begin();
	iterE = SmfCampaignWrapup::m_removeFromImm.end();

	while (iter != iterE) {
		delete((*iter));
		iter++;
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

#if 0
//------------------------------------------------------------------------------
// addCallbackAtCommit()
//------------------------------------------------------------------------------
void 
SmfCampaignWrapup::addCallbackAtCommit(SmfCallbackOptions * i_option)
{
	m_callbackAtCommit.push_back(i_option);
}
#endif

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

	//Callback at commit
#if 0
	std::list < SmfCallbackOptions * >m_callbackAtCommit;
#endif
	// The actions below are trigged by a campaign commit operation.
	// The campaign will enter state "commited" even if some actions fails.
	// Just log errors and try to execute as many operations as possible.

	LOG_NO("CAMP: Campaign wrapup, start campWrapupActions (%zu)", m_campWrapupAction.size());
	std::list < SmfUpgradeAction * >::iterator iter;
	for (iter = m_campWrapupAction.begin(); iter != m_campWrapupAction.end(); ++iter) {
		if ((*iter)->execute() != SA_AIS_OK) {
			LOG_ER("SmfCampaignWrapup campWrapupActions %d failed", (*iter)->getId());
		}
	}

	LOG_NO("CAMP: Campaign wrapup , start remove from IMM (%zu)", m_removeFromImm.size());
	if (m_removeFromImm.size() > 0) {
		SmfImmUtils immUtil;
		if (immUtil.doImmOperations(m_removeFromImm) != SA_AIS_OK) {
			LOG_ER("SmfCampaignWrapup remove from IMM failed");
		}
	}

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

			if ((maintCamp != NULL) && (maintCamp->length > 0)) {
				SmfImmModifyOperation *modop = new (std::nothrow) SmfImmModifyOperation;
				assert(modop != 0);
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
                LOG_ER("SmfUpgradeStep::setMaintenanceState(), fails to reset all saAmfSUMaintenanceCampaign attr");
        }

        //Delete the created SmfImmModifyOperation instances
        std::list < SmfImmOperation * > ::iterator operIter;
        for (operIter = operations.begin(); operIter != operations.end(); ++operIter) {
                delete (*operIter);
        }

	LOG_NO("CAMP: campWrapupActions completed");

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
	LOG_NO("CAMP: Campaign complete, start executeCampComplete (%zu)", m_campCompleteAction.size());
	std::list < SmfUpgradeAction * >::iterator iter;
	iter = m_campCompleteAction.begin();
	while (iter != m_campCompleteAction.end()) {
		if ((*iter)->execute() != SA_AIS_OK) {
			LOG_ER("SmfCampaignWrapup campCompleteAction %d failed", (*iter)->getId());
			return false;
		}
		iter++;
	}

	LOG_NO("CAMP: executeCampComplete completed");

	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeRollback()
//------------------------------------------------------------------------------
bool 
SmfCampaignWrapup::executeRollback()
{
#if 0
	std::list < SmfCallbackOptions * >m_callbackAtRollback;
	std::list < SmfAction * >m_campInitAction;
#endif

	return true;
}
