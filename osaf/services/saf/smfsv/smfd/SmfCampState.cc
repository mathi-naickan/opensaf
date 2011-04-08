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
#include "smfd.h"
#include "SmfUtils.hh"
#include "SmfUpgradeCampaign.hh"
#include "SmfCampState.hh"
#include "SmfProcState.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfUpgradeMethod.hh"
#include "SmfTargetTemplate.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaign.hh"
#include "SmfProcedureThread.hh"
#include <immutil.h>
#include <sstream>
#include <iostream>
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
// ------Base class SmfCampState------------------------------------------------
//
// SmfCampState default implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampState::getClassName(std::string & io_str) const
{
	io_str = "SmfCampState";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampState::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::execute default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// executeProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::executeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::executeProc default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// rollbackProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::rollbackProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::rollbackProc default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::rollback(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::rollback default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::suspend(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::suspend default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::commit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::commit default implementation, should NEVER be executed.");
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::procResult(SmfUpgradeCampaign *  i_camp,
                         SmfUpgradeProcedure * i_procedure,
                         SmfProcResultT        i_result)
{
	TRACE_ENTER();
	LOG_NO("SmfCampState::procResult default implementation. Received response %d from procedure %s",
              i_result, i_procedure->getProcName().c_str());
	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// changeState()
//------------------------------------------------------------------------------
void 
SmfCampState::changeState(SmfUpgradeCampaign * i_camp, SmfCampState * i_state)
{
	TRACE_ENTER();
	TRACE("SmfCampState::changeState");

	std::string newState;
	i_state->getClassName(newState);

	std::string oldState;
	i_camp->m_state->getClassName(oldState);

	TRACE("SmfCampState::changeState old state=%s , new state=%s", oldState.c_str(), newState.c_str());
	i_camp->changeState(i_state);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateInitial implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SmfCampState *SmfCampStateInitial::s_instance = NULL;

SmfCampState *SmfCampStateInitial::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateInitial;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateInitial::getClassName(std::string & io_str) const {
	io_str = "SmfCampStateInitial";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateInitial::toString(std::string & io_str) const {
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateInitial::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	std::string error;
	std::string s;
	std::stringstream out;
	std::vector < SmfUpgradeProcedure * >::iterator procIter;
        SmfCampResultT initResult;

	std::list < std::string > bundleInstallDnCamp;
	std::list < std::string > notFoundInstallDn;
	std::list < std::string > bundleRemoveDnCamp;
	std::list < std::string > notFoundRemoveDn;
	std::list < std::string >::iterator dnIter;

        SaImmAttrValuesT_2 **attributes;
        SmfImmUtils immUtil;

	TRACE("SmfCampStateInitial::execute implementation");

	LOG_NO("CAMP: Start upgrade campaign %s", i_camp->getCampaignName().c_str());

	//Reset error string in case the previous prerequsites check failed
        //In such case an error string is present in initial state
	SmfCampaignThread::instance()->campaign()->setError("");

	//Prerequisite  check 1 "The Software Management Framework is operational"
	//If executing here, it is operational

	//Prerequisite  check 2 "The software repository is accessable"
	LOG_NO("CAMP: Check SMF repository %s", i_camp->getCampaignName().c_str());
	if (smfd_cb->repositoryCheckCmd != NULL) {
		TRACE("Executing SMF repository check cmd %s", smfd_cb->repositoryCheckCmd);
		int rc = smf_system(smfd_cb->repositoryCheckCmd);
		if (rc != 0) {
			error = "CAMP: SMF repository check command ";
			error += smfd_cb->repositoryCheckCmd;
			error += " failed ";
			out << rc;
			s = out.str();
			error += s;
			goto exit_error;
		}
	} else {
		error = "CAMP: No SMF repository check command found";
		goto exit_error;
	}

	//Prerequisite  check 3 "There is no other upgrade campaign in progress"
	//If executing here, this was checked before this campaign was started


	//Prerequisite  check 5 "The specifics of the upgrade campaign have been provided, and the campaign is still applicable"
	//TBD

	//Prerequisite  check 4 "The current running version of the software is available in the software repository"
	//Prerequisite  check 6 "The desired version of the software is available in the software repository, 
        //                       and all the dependencies of the required packages have been checked and are satisfied"

	//Below is the current implementation of prerequisite check 4 and 6. 
	//For bundles included in any <swAdd> portion of the campaign:
	//1) The IMM is searched for existing SW bundle objects
	//2) If not found in IMM, the campaign xml <softwareBundle> portion is searched to see if the swBundle
        //   objects are going to be created by the campaign.

	//For bundles included in any <swRemove> portion of the campaign:
	//1) The IMM is searched for existing SW bundle objects
	
	//Check that bundles to install is available in the software repository
	LOG_NO("CAMP: Check bundles to install and remove.");
	//Find out which SW Bundles to install
	TRACE("Find out which SW Bundles to install");
	procIter = i_camp->m_procedure.begin();
	while (procIter != i_camp->m_procedure.end()) {
		//Fetch the list of SW to add from each procedure
		SaSmfUpgrMethodT upType = (*procIter)->getUpgradeMethod()->getUpgradeMethod();
		if (upType == SA_SMF_ROLLING) {
			TRACE("SA_SMF_ROLLING procedure detected");
			const std::list < SmfBundleRef * > b = ((SmfByTemplate*)(*procIter)->getUpgradeMethod()->
								getUpgradeScope())->getTargetNodeTemplate()->getSwInstallList();

			std::list< SmfBundleRef* >::const_iterator bIter;
			bIter = b.begin();
			while (bIter != b.end()) {
				bundleInstallDnCamp.push_back((*bIter)->getBundleDn());
				bIter++;
			}
		} else if (upType == SA_SMF_SINGLE_STEP) {
			TRACE("SA_SMF_SINGLE_STEP procedure detected");
			const SmfUpgradeScope * scope = (*procIter)->getUpgradeMethod()->getUpgradeScope();

			//Cast to valid upgradeScope
			const SmfForModify* modify = dynamic_cast<const SmfForModify*>(scope);
			const SmfForAddRemove* addRemove = dynamic_cast<const SmfForAddRemove*>(scope);
			
			if(modify != 0) { //Check if the upgradeScope is SmfForModify
				TRACE("SA_SMF_SINGLE_STEP procedure with scope forModify detected");
				const std::list < SmfBundleRef > b = modify->getActivationUnit()->getSwAdd();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleInstallDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}
			}else if(addRemove != 0) { //Check if the upgradeScope is SmfForAddRemove
				TRACE("SA_SMF_SINGLE_STEP procedure with scope forAddRemove detected");
				const std::list < SmfBundleRef > b = addRemove->getActivationUnit()->getSwAdd();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleInstallDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}
			} else {
				TRACE("Unknown procedure scope");
				error = "CAMP: Unknown procedure scope";
				goto exit_error;
			}
		} else {
			TRACE("SA_SMF_SINGLE_STEP unknown procedure type");
			error = "CAMP: Unknown procedure type";
			goto exit_error;
		}


		procIter++;
	}

	bundleInstallDnCamp.sort();
	bundleInstallDnCamp.unique();

	//Find out which SW Bundles to remove
	TRACE("Find out which SW Bundles to remove");
	procIter = i_camp->m_procedure.begin();
	while (procIter != i_camp->m_procedure.end()) {
		//Fetch the list of SW to add from each procedure
		SaSmfUpgrMethodT upType = (*procIter)->getUpgradeMethod()->getUpgradeMethod();
		if (upType == SA_SMF_ROLLING) {
			TRACE("SA_SMF_ROLLING procedure detected");
			const std::list < SmfBundleRef * > b = ((SmfByTemplate*)(*procIter)->getUpgradeMethod()->
								getUpgradeScope())->getTargetNodeTemplate()->getSwRemoveList();

			std::list< SmfBundleRef* >::const_iterator bIter;
			bIter = b.begin();
			while (bIter != b.end()) {
				bundleRemoveDnCamp.push_back((*bIter)->getBundleDn());
				bIter++;
			}
		} else if (upType == SA_SMF_SINGLE_STEP) {
			TRACE("SA_SMF_SINGLE_STEP procedure detected");
			const SmfUpgradeScope * scope = (*procIter)->getUpgradeMethod()->getUpgradeScope();

			//Cast to valid upgradeScope
			const SmfForModify* modify = dynamic_cast<const SmfForModify*>(scope);
			const SmfForAddRemove* addRemove = dynamic_cast<const SmfForAddRemove*>(scope);
			
			if(modify != 0) { //Check if the upgradeScope is SmfForModify
				TRACE("SA_SMF_SINGLE_STEP procedure with scope forModify detected");
				const std::list < SmfBundleRef > b = modify->getActivationUnit()->getSwRemove();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleRemoveDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}
			}else if(addRemove != 0) { //Check if the upgradeScope is SmfForAddRemove
				TRACE("SA_SMF_SINGLE_STEP procedure with scope forAddRemove detected");
				const std::list < SmfBundleRef > b = addRemove->getDeactivationUnit()->getSwRemove();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleRemoveDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}
			} else {
				TRACE("Unknown procedure scope");
				error = "CAMP: Unknown procedure scope";
				goto exit_error;
			}
		} else {
			error = "CAMP: Unknown procedure type";
			goto exit_error;
		}

		procIter++;
	}

	bundleRemoveDnCamp.sort();
	bundleRemoveDnCamp.unique();

	TRACE("Total number of bundles to install in the campaign = %zd", bundleInstallDnCamp.size());
	TRACE("Total number of bundles to remove in the campaign = %zd", bundleRemoveDnCamp.size());

	//Check if SW Bundles to install can be found in IMM
	TRACE("Check if SW Bundles to install can be found in IMM");
	for (dnIter=bundleInstallDnCamp.begin(); dnIter != bundleInstallDnCamp.end(); ++dnIter) {
		if (immUtil.getObject((*dnIter), &attributes) == true) { //found
			TRACE("SW Bundle to install %s found in IMM", (*dnIter).c_str());
		} else {
			TRACE("SW Bundle to install %s NOT found in IMM", (*dnIter).c_str());
			notFoundInstallDn.push_back(*dnIter);
		}
	}

	//If bundles are not found in IMM, check if the budles are specified to be installed in 
	//the <addToImm> portion of the campaign
	if (notFoundInstallDn.size() > 0) {
		TRACE("All bundles was not found in IMM, check if they will be created by the campaign");
		//Find all bundles to be created my the campaign
		std::list < std::string > addToImmBundleDn;
		std::list < SmfImmOperation * > immOper;
		std::list < SmfImmOperation * >::iterator operIter;

		immOper = i_camp->m_campInit.getAddToImm();
		operIter = immOper.begin();
		while (operIter != immOper.end()) {
			SmfImmCreateOperation* ico = dynamic_cast<SmfImmCreateOperation*>((*operIter));
			if (ico != NULL) {
				if (ico->getClassName() == "SaSmfSwBundle") {                //This is sw bundle
					std::list <SmfImmAttribute> attr = ico->getValues(); //Get all instance attributes
					std::list <SmfImmAttribute>::iterator attrIter;
					attrIter = attr.begin();
					while (attrIter != attr.end()) {                     //Search for safSmfBundle attribute
						if( (*attrIter).getName() == "safSmfBundle") {
							std::string  val = (*attrIter).getValues().front(); //Only one value
							if (ico->getParentDn().size() > 0) {
								val += "," + ico->getParentDn();
							}
							TRACE("SW Bundle to add %s will be created by campaign", val.c_str());
							addToImmBundleDn.push_back(val);
						}
						attrIter++;
					}
				}
			}
			operIter++;
		}
		addToImmBundleDn.sort();
		addToImmBundleDn.unique();

		//Match found bundles to be created with the remaining list of bundles to install
		std::list < std::string >::iterator strIter;
		strIter = addToImmBundleDn.begin();
		while (strIter != addToImmBundleDn.end()) {
			TRACE("Remove SW Bundle %s from list of missing SW Bundles", (*strIter).c_str());
			notFoundInstallDn.remove(*strIter);
			strIter++;
		}
	}

	TRACE("Check if SW Bundles to remove are found in IMM");
	//Check if SW Bundles to remove are found in IMM
	for (dnIter=bundleRemoveDnCamp.begin(); dnIter != bundleRemoveDnCamp.end(); ++dnIter) {
		if (immUtil.getObject((*dnIter), &attributes) == true) { //found
			TRACE("SW Bundle to remove %s found in IMM", (*dnIter).c_str());
		} else {
			TRACE("SW Bundle to remove %s NOT found in IMM", (*dnIter).c_str());
			notFoundRemoveDn.push_back(*dnIter);
		}
	}

	//If bundles are not found in IMM, check if the budles to remove are specified to be added in 
	//the <addToImm> portion of the campaign
	if (notFoundRemoveDn.size() > 0) {
		TRACE("All bundles was not found in IMM, check if they will be created by the campaign");
		//Find all bundles to be created my the campaign
		std::list < std::string > addToImmBundleDn;
		std::list < SmfImmOperation * > immOper;
		std::list < SmfImmOperation * >::iterator operIter;

		immOper = i_camp->m_campInit.getAddToImm();
		operIter = immOper.begin();
		while (operIter != immOper.end()) {
			SmfImmCreateOperation* ico = dynamic_cast<SmfImmCreateOperation*>((*operIter));
			if (ico != NULL) {
				if (ico->getClassName() == "SaSmfSwBundle") {                //This is sw bundle
					std::list <SmfImmAttribute> attr = ico->getValues(); //Get all instance attributes
					std::list <SmfImmAttribute>::iterator attrIter;
					attrIter = attr.begin();
					while (attrIter != attr.end()) {                     //Search for safSmfBundle attribute
						if( (*attrIter).getName() == "safSmfBundle") {
							std::string  val = (*attrIter).getValues().front(); //Only one value
							if (ico->getParentDn().size() > 0) {
								val += "," + ico->getParentDn();
							}
							TRACE("SW Bundle to remove %s will be created by campaign", val.c_str());
							addToImmBundleDn.push_back(val);
						}
						attrIter++;
					}
				}
			}
			operIter++;
		}
		addToImmBundleDn.sort();
		addToImmBundleDn.unique();

		//Match found bundles to be created with the remaining list of bundles to install
		std::list < std::string >::iterator strIter;
		strIter = addToImmBundleDn.begin();
		while (strIter != addToImmBundleDn.end()) {
			TRACE("Remove SW Bundle %s from list of missing SW Bundles", (*strIter).c_str());
			notFoundRemoveDn.remove(*strIter);
			strIter++;
		}
	}

	//Here are the lists of bundles to install and remove. Shall be empty if all bundles was found 
	//in Imm or was prepared to be created in <addToImm> portion of the campaigm
	if ((notFoundInstallDn.size() > 0) || (notFoundRemoveDn.size() > 0)) {
		std::list < std::string >::iterator strIter;
		if (notFoundInstallDn.size() > 0) {
			LOG_ER("CAMP: The following SW bundles to add was not found in system or campaign:");
			strIter = notFoundInstallDn.begin();
			while (strIter != notFoundInstallDn.end()) {
                                LOG_ER("CAMP: %s", (*strIter).c_str());
				strIter++;
			}
		}

		if (notFoundRemoveDn.size() > 0) {
			LOG_ER("CAMP: The following SW bundles to remove was not found in system:");
			strIter = notFoundRemoveDn.begin();
			while (strIter != notFoundRemoveDn.end()) {
                                LOG_ER("CAMP: %s", (*strIter).c_str());
				strIter++;
			}

		}

		error = "CAMP: Bundles to add/remove was not found in system or campaign";
		goto exit_error;
	}

	//Prerequisite  check 7 "All affected nodes are able to provide the resources (for instance, sufficient disk space 
	//                       and proper access rights) needed to perform the upgrade campaign"
	//TBD

	//Prerequisite  check 8 "The Software Management Framework is able to obtain the administrative ownership 
	//                       for all necessary information model objects"
	//TBD

	//Prerequisite  check 9 "The target system is in a state such that the expected service outage does not exceed 
	//                       the acceptable service outage defined for the campaig"
	//TBD

	//Prerequisite  check 10 "Upgrade-aware entities are ready for an upgrade campaign"
	//TBD

	//Prerequisite  check 11 "All neccessary backup is created"
	LOG_NO("CAMP: Create system backup %s", i_camp->getCampaignName().c_str());
	if (smfd_cb->backupCreateCmd != NULL) {
		std::string backupCmd = smfd_cb->backupCreateCmd;
		backupCmd += " ";
		backupCmd += i_camp->getCampaignName();

		TRACE("Executing backup create cmd %s", backupCmd.c_str());
		int rc = smf_system(backupCmd.c_str());
		if (rc != 0) {
			error = "CAMP: Backup create command ";
			error += smfd_cb->backupCreateCmd;
			error += " failed ";
			out << rc;
			s = out.str();
			error += s;
			goto exit_error;
		}
	} else {
		error = "CAMP: No backup create command  found";
		goto exit_error;
	}

	if (i_camp->m_campInit.executeBackup() != SA_AIS_OK) {
		std::string error = "Campaign init backup callback failed";
		LOG_ER("%s", error.c_str());
		goto exit_error;
	}
	LOG_NO("CAMP: executed callbackAtBackup successfully in the campaign %s", i_camp->getCampaignName().c_str());

	LOG_NO("CAMP: Start executing campaign %s", i_camp->getCampaignName().c_str());

	TRACE("Create the SmfCampRestartInfo object");
	i_camp->createCampRestartInfo();

	//Preparation is ready, change state and execute campaign initialization
	changeState(i_camp, SmfCampStateExecuting::instance());

        initResult = executeInit(i_camp);
	TRACE_LEAVE();
	return initResult;

exit_error:
	LOG_ER("%s", error.c_str());
	SmfCampaignThread::instance()->campaign()->setError(error);

	//Remain in state initial if prerequsites check or SMF backup fails

	/* Terminate campaign thread */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
	return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateInitial::executeInit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecuting::executeInit, Running campaign init actions");

	if (i_camp->m_campInit.execute() != SA_AIS_OK) {
		std::string error = "Campaign init failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		return SMF_CAMP_FAILED;
	}

	TRACE("SmfCampStateExecuting::executeInit, campaign init actions completed");
        return SMF_CAMP_CONTINUE; /* Continue in next state */
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateExecuting implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateExecuting::s_instance = NULL;

SmfCampState *SmfCampStateExecuting::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateExecuting;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateExecuting";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecuting::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	TRACE("SmfCampStateExecuting::execute, Do some checking");

	//If a running campaign was restarted on another node, the procedures in executing state
	//must be restarted. The execution shall continue at step execution phase. The procedure initialization
	//and step calculation was performed before the move of control.
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
        std::vector < SmfUpgradeProcedure * >::const_iterator iter;
	bool execProcFound = false;

	iter = procedures.begin();
	i_camp->m_noOfExecutingProc = 0; //The no of answers which must be wait for, could be more than 1 if parallel procedures
	while (iter != procedures.end()) {
		if ((*iter)->getState() == SA_SMF_PROC_EXECUTING) {
			TRACE("SmfCampStateExecuting::execute, restarted procedure found, send PROCEDURE_EVT_EXECUTE_STEP event to thread");
			SmfProcedureThread *procThread = (*iter)->getProcThread();
			PROCEDURE_EVT *evt = new PROCEDURE_EVT();
			evt->type = PROCEDURE_EVT_EXECUTE_STEP;
			procThread->send(evt);
			execProcFound = true;
			i_camp->m_noOfExecutingProc++; 
		}

		iter++;
	}

	if (execProcFound == true) {
		//Execution will continue at SmfCampStateExecuting::executeProc() when proc result is received.
		TRACE("SmfCampStateExecuting::execute, Wait for restarted procedures to respond.");
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	}

	/* No executing procedures, start executing next procedure */
        SmfCampResultT result = this->executeProc(i_camp);
        TRACE_LEAVE();
        return result;
}

//------------------------------------------------------------------------------
// executeProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecuting::executeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	TRACE("SmfCampStateExecuting::executeProc Proceed with next execution level procedures");

	//The procedure vector is sorted in execution level order (low -> high)
	//Lowest number shall be executed first.

        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;
	int execLevel = -1;

	iter = procedures.begin();
	while (iter != procedures.end()) {

		TRACE("Start procedures, try procedure %s, state=%u, execLevel=%d",
		      (*iter)->getProcName().c_str(), (*iter)->getState(), (*iter)->getExecLevel());

		// If the state is initial and the execution level is new or the same as 
		// the procedure just started.
		if (((*iter)->getState() == SA_SMF_PROC_INITIAL)
		    && ((execLevel == -1) || (execLevel == (*iter)->getExecLevel()))) {
			if (execLevel == (*iter)->getExecLevel()) {
				TRACE("A procedure already run at execLevel=%d, start parallel procedure", execLevel);
			} else if (execLevel == -1) {
				TRACE("Start first procedure on execLevel=%d", (*iter)->getExecLevel());
			}

			SmfProcedureThread *procThread = (*iter)->getProcThread();
			PROCEDURE_EVT *evt = new PROCEDURE_EVT();
			evt->type = PROCEDURE_EVT_EXECUTE;
			procThread->send(evt);
			execLevel = (*iter)->getExecLevel();
			i_camp->m_noOfExecutingProc++;
		}
		iter++;
	}

	if (execLevel != -1) {
		TRACE("SmfCampStateExecuting::executeProc, Wait for procedure results");
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	}

	TRACE("SmfCampStateExecuting::executeProc, All procedures executed, start wrapup");

	LOG_NO("CAMP: All procedures executed, start wrapup");
        SmfCampResultT result = this->executeWrapup(i_camp);
        TRACE_LEAVE();
        return result;
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecuting::executeWrapup(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	if (i_camp->m_campWrapup.executeCampComplete() == false) {
		std::string error = "Campaign wrapup executing campaign complete actions failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		return SMF_CAMP_FAILED;
	}
	// TODO Start wait to complete timer
	LOG_NO("CAMP: Start wait to complete timer (not implemented yet)");

	/* TODO State completed should be set when waitToComplete times out */
	changeState(i_camp, SmfCampStateExecCompleted::instance());

	LOG_NO("CAMP: Upgrade campaign completed %s", i_camp->getCampaignName().c_str());
	TRACE_LEAVE();
        return SMF_CAMP_COMPLETED;
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecuting::suspend(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecuting::suspend implementation");

	/* Send suspend message to all procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
                TRACE("SmfCampStateExecuting::Procedure %s, send suspend",
                      (*iter)->getProcName().c_str());
                SmfProcedureThread *procThread = (*iter)->getProcThread();
                PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                evt->type = PROCEDURE_EVT_SUSPEND;
                procThread->send(evt);
	}

        i_camp->m_noOfProcResponses = 0;
        changeState(i_camp, SmfCampStateSuspendingExec::instance());
        /* Wait for suspend responses from all procedures (SmfCampStateSuspendingExec::procResult) */

	TRACE_LEAVE();
        return SMF_CAMP_SUSPENDING; 
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecuting::procResult(SmfUpgradeCampaign *  i_camp,
                                  SmfUpgradeProcedure * i_procedure,
                                  SmfProcResultT        i_result)
{
	TRACE_ENTER();

        switch (i_result) {
        case SMF_PROC_COMPLETED: {
                LOG_NO("CAMP: Procedure %s returned COMPLETED", i_procedure->getProcName().c_str());
                break;
        }
        case SMF_PROC_FAILED: {
                LOG_NO("CAMP: Procedure %s returned FAILED", i_procedure->getProcName().c_str());
                std::string error = "Procedure " + i_procedure->getProcName() + " failed";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return SMF_CAMP_FAILED;
                break;
        }
        case SMF_PROC_STEPUNDONE: {
                LOG_NO("CAMP: Procedure %s returned STEPUNDONE", i_procedure->getProcName().c_str());

                /* Send suspend message to all procedures */
                const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
        	std::vector < SmfUpgradeProcedure * >::const_iterator iter;
        
        	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
                        TRACE("SmfCampStateExecuting:: Step undone, send suspend to procedure %s",
                              (*iter)->getProcName().c_str());
                        SmfProcedureThread *procThread = (*iter)->getProcThread();
                        PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                        evt->type = PROCEDURE_EVT_SUSPEND;
                        procThread->send(evt);
        	}
                i_camp->m_noOfProcResponses = 0;

                /* Receive all suspend responses in new state */
		changeState(i_camp, SmfCampStateErrorDetected::instance());
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
                break;
        }
        default: {
                LOG_NO("SmfCampStateExecuting::procResult received unhandled response %d from procedure %s", 
                       i_result, i_procedure->getDn().c_str());                
                break;
        }
        }

	if (i_camp->m_noOfExecutingProc > 1) {
		i_camp->m_noOfExecutingProc--;
		TRACE("Still noProc=%d procedures executing, continue waiting for result.", i_camp->m_noOfExecutingProc);
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	} else if (i_camp->m_noOfExecutingProc == 1) {
		TRACE("Response from last procedure received.");
		//Decrement counter for the last procedure on the current exec level 
		i_camp->m_noOfExecutingProc--;
	}

	TRACE("All procedures started on the same execlevel have answered.");

        /* Find next procedure to be executed */
        SmfCampResultT result = this->executeProc(i_camp);
        TRACE_LEAVE();
        return result;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateExecutionCompleted implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateExecCompleted::s_instance = NULL;

SmfCampState *SmfCampStateExecCompleted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateExecCompleted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateExecCompleted::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateExecutionCompleted";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void SmfCampStateExecCompleted::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecCompleted::rollback(SmfUpgradeCampaign * i_camp)
{
        TRACE_ENTER();
	LOG_NO("CAMP: Start rolling back completed campaign %s", i_camp->getCampaignName().c_str());
        changeState(i_camp, SmfCampRollingBack::instance());
	if (i_camp->m_campInit.executeCallbackAtRollback() == false) {
		std::string error = "Campaign rollback callback failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		return SMF_CAMP_FAILED;
	}
	
        SmfCampResultT wrapupResult = rollbackWrapup(i_camp);
	TRACE_LEAVE();
        return wrapupResult;
}

//------------------------------------------------------------------------------
// rollbackWrapup()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecCompleted::rollbackWrapup(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	if (i_camp->m_campWrapup.rollbackCampComplete() == false) {
		std::string error = "Campaign rollback complete actions failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		return SMF_CAMP_FAILED;
	}

	LOG_NO("CAMP: Start rolling back the procedures");
	TRACE_LEAVE();
        return SMF_CAMP_CONTINUE; /* Continue in next state */
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecCompleted::commit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

        LOG_NO("CAMP: Commit upgrade campaign %s", i_camp->getCampaignName().c_str());

	i_camp->m_campWrapup.executeCampWrapup(); // No action if wrapup is faulty

        i_camp->resetMaintenanceState(); // No action if it fails

	//Remove the procedure runtime objects
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

        for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		(*iter)->commit();
	}

        i_camp->removeRunTimeObjects(); // No action if it fails

	changeState(i_camp, SmfCampStateCommitted::instance());
	LOG_NO("CAMP: Upgrade campaign committed %s", i_camp->getCampaignName().c_str());

	// TODO Start wait to allow new campaign timer
	LOG_NO("CAMP: Start wait to allow new campaign timer (not implemented yet)");

	/* Terminate campaign thread */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
        return SMF_CAMP_COMMITED;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateSuspendingExec implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateSuspendingExec::s_instance = NULL;

SmfCampState *SmfCampStateSuspendingExec::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateSuspendingExec;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateSuspendingExec::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateSuspendingExec";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateSuspendingExec::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateSuspendingExec::procResult(SmfUpgradeCampaign *  i_camp,
                                       SmfUpgradeProcedure * i_procedure,
                                       SmfProcResultT        i_result)
{
	TRACE_ENTER();
	TRACE("SmfCampStateSuspendingExec::procResult implementation");

        /* We could get other responses */
        switch (i_result) {
        case SMF_PROC_SUSPENDED: {
                /* If first response, set number of expected responses */
                if (i_camp->m_noOfProcResponses == 0) {
                        i_camp->m_noOfProcResponses = i_camp->getProcedures().size();
                }

                /* Decrease the response counter */
                i_camp->m_noOfProcResponses--;
        
                /* If last response, change state to suspended */
                if (i_camp->m_noOfProcResponses == 0) {
        		changeState(i_camp, SmfCampStateExecSuspended::instance());
        	}
                break;
        }
        case SMF_PROC_FAILED: {
                TRACE("SmfCampStateSuspendingExec::procResult received FAILED from procedure %s", i_procedure->getDn().c_str());
                changeState(i_camp, SmfCampStateExecFailed::instance());
                /* The rest of the procedure results will be received by new state */
                break;
        }
        case SMF_PROC_STEPUNDONE: {
                TRACE("SmfCampStateSuspendingExec::procResult received STEPUNDONE from procedure %s", i_procedure->getDn().c_str());
                changeState(i_camp, SmfCampStateErrorDetectedInSuspending::instance());
                /* Handle the rest of the procedure suspend results in new state */
                break;
        }
        default: {
                LOG_NO("SmfCampStateSuspendingExec::procResult received unhandled response %d from procedure %s", 
                       i_result, i_procedure->getDn().c_str());
                break;
        }
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateExecSuspended implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateExecSuspended::s_instance = NULL;

SmfCampState *SmfCampStateExecSuspended::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateExecSuspended;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateExecSuspended::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateExecSuspended";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateExecSuspended::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecSuspended::execute(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampStateExecSuspended::execute implementation");

	/* Send execute to all suspended procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_SUSPENDED:
			{
				TRACE("SmfCampStateExecuting::Procedure %s suspended, send execute",
				      (*iter)->getProcName().c_str());
				SmfProcedureThread *procThread = (*iter)->getProcThread();
				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_EXECUTE;
				procThread->send(evt);
                                numOfSuspendedProc++;
                                break;
			}
		default:
			break;
		}
	}

	changeState(i_camp, SmfCampStateExecuting::instance());
        i_camp->m_noOfExecutingProc = numOfSuspendedProc;

        /* If no suspended procedures existed, execute next procedure in state initial.
           I.e. we were suspended in between procedures */
        if (numOfSuspendedProc == 0) {
                TRACE("SmfCampStateExecuting::No suspended procedures, start next procedure");

                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_EXECUTE_PROC;
                SmfCampaignThread::instance()->send(evt);
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateExecSuspended::rollback(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampStateExecSuspended::rollback implementation");

	/* Invoke callbackAtRollback as rollback is being initiated*/
        changeState(i_camp, SmfCampRollingBack::instance());
	if (i_camp->m_campInit.executeCallbackAtRollback() == false) {
		std::string error = "Campaign rollback callback failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		return SMF_CAMP_FAILED;
	}
	
	/* Send rollback to all suspended procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_SUSPENDED:
			{
				TRACE("SmfCampStateExecuting::Procedure %s suspended, send rollback",
				      (*iter)->getProcName().c_str());
				SmfProcedureThread *procThread = (*iter)->getProcThread();
				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_ROLLBACK;
				procThread->send(evt);
                                numOfSuspendedProc++;
                                break;
			}
		default:
			break;
		}
	}

        i_camp->m_noOfExecutingProc = numOfSuspendedProc;

        /* If no suspended procedures existed, rollback next completed procedure.
           I.e. we were suspended in between procedures */
        if (numOfSuspendedProc == 0) {
                TRACE("SmfCampStateExecuting::No suspended procedures, start rollback next procedure");

                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_ROLLBACK_PROC;
                SmfCampaignThread::instance()->send(evt);
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateCommitted implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateCommitted::s_instance = NULL;

SmfCampState *SmfCampStateCommitted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateCommitted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateCommitted::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateCommitted";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateCommitted::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateExecFailed implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateExecFailed::s_instance = NULL;

SmfCampState *SmfCampStateExecFailed::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateExecFailed;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateExecFailed::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateExecFailed";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateExecFailed::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateErrorDetected implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateErrorDetected::s_instance = NULL;

SmfCampState *SmfCampStateErrorDetected::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateErrorDetected;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateErrorDetected::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateErrorDetected";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateErrorDetected::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateErrorDetected::procResult(SmfUpgradeCampaign *  i_camp,
                                      SmfUpgradeProcedure * i_procedure,
                                      SmfProcResultT        i_result)
{
	TRACE_ENTER();
	TRACE("SmfCampStateErrorDetected::procResult implementation");

        /* We could get other responses */
        if (i_result == SMF_PROC_SUSPENDED) {
                /* If first response, set number of expected responses */
                if (i_camp->m_noOfProcResponses == 0) {
                        i_camp->m_noOfProcResponses = i_camp->m_procedure.size();
                }

                /* Decrease the response counter */
                i_camp->m_noOfProcResponses--;
        
                /* If last response, change state to suspended by error detected */
                if (i_camp->m_noOfProcResponses == 0) {
        		changeState(i_camp, SmfCampStateSuspendedByErrorDetected::instance());
        	}
        } else if (i_result == SMF_PROC_FAILED) {
                TRACE("SmfCampStateErrorDetected::procResult received FAILED from procedure %s", i_procedure->getDn().c_str());
                changeState(i_camp, SmfCampStateExecFailed::instance());
                /* Any more suspend responses will be received by new state */
        } else {
                LOG_NO("SmfCampStateErrorDetected::procResult received unhandled response %d from procedure %s", 
                       i_result, i_procedure->getDn().c_str());                
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateErrorDetectedInSuspending implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateErrorDetectedInSuspending::s_instance = NULL;

SmfCampState *SmfCampStateErrorDetectedInSuspending::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateErrorDetectedInSuspending;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateErrorDetectedInSuspending::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateErrorDetectedInSuspending";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateErrorDetectedInSuspending::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateErrorDetectedInSuspending::procResult(SmfUpgradeCampaign *  i_camp,
                                                  SmfUpgradeProcedure * i_procedure,
                                                  SmfProcResultT        i_result)
{
	TRACE_ENTER();
	TRACE("SmfCampStateErrorDetectedInSuspending::procResult implementation");

        /* We could get other responses */
        if (i_result == SMF_PROC_SUSPENDED) {
                /* If first response, set number of expected responses */
                if (i_camp->m_noOfProcResponses == 0) {
                        i_camp->m_noOfProcResponses = i_camp->m_procedure.size();
                }

                /* Decrease the response counter */
                i_camp->m_noOfProcResponses--;
        
                /* If last response, change state to suspended by error detected */
                if (i_camp->m_noOfProcResponses == 0) {
        		changeState(i_camp, SmfCampStateSuspendedByErrorDetected::instance());
        	}
        } else if (i_result == SMF_PROC_FAILED) {
                TRACE("SmfCampStateSuspendingExec::procResult received ROLLBACKFAILED from procedure %s", i_procedure->getDn().c_str());
                changeState(i_camp, SmfCampStateExecFailed::instance());
                /* Any more responses will be received by new state */
        } else {
                LOG_NO("SmfCampStateErrorDetectedInSuspending::procResult received unhandled response %d from procedure %s", 
                       i_result, i_procedure->getDn().c_str());                
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampStateSuspendedByErrorDetected implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampStateSuspendedByErrorDetected::s_instance = NULL;

SmfCampState *SmfCampStateSuspendedByErrorDetected::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampStateSuspendedByErrorDetected;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampStateSuspendedByErrorDetected::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampStateSuspendedByErrorDetected";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampStateSuspendedByErrorDetected::toString(std::string & io_str) const 
{
	getClassName(io_str);
}


//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateSuspendedByErrorDetected::execute(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampStateSuspendedByErrorDetected::execute implementation");

	/* Send execute to all suspended/undone procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_SUSPENDED:
		case SA_SMF_PROC_STEP_UNDONE:
			{
				TRACE("SmfCampStateSuspendedByErrorDetected::Procedure %s suspended/undone, send execute",
				      (*iter)->getProcName().c_str());
				SmfProcedureThread *procThread = (*iter)->getProcThread();
				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_EXECUTE;
				procThread->send(evt);
                                numOfSuspendedProc++;
                                break;
			}
		default:
			break;
		}
	}

        /* If no suspended/undone procedures exists something is really wrong */
        if (numOfSuspendedProc == 0) {
                LOG_ER("SmfCampStateSuspendedByErrorDetected: No suspended/undone procedures found when execute");
                changeState(i_camp, SmfCampStateExecFailed::instance());
        }

	changeState(i_camp, SmfCampStateExecuting::instance());
        i_camp->m_noOfExecutingProc = numOfSuspendedProc;

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampStateSuspendedByErrorDetected::rollback(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampStateSuspendedByErrorDetected::rollback implementation");

        changeState(i_camp, SmfCampRollingBack::instance());
	if (i_camp->m_campInit.executeCallbackAtRollback() == false) {
		std::string error = "Campaign rollback callback failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		return SMF_CAMP_FAILED;
	}
	
	/* Send rollback to all suspended/undone procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_SUSPENDED:
		case SA_SMF_PROC_STEP_UNDONE:
			{
				TRACE("SmfCampStateSuspendedByErrorDetected::Procedure %s suspended/undone, send rollback",
				      (*iter)->getProcName().c_str());
				SmfProcedureThread *procThread = (*iter)->getProcThread();
				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_ROLLBACK;
				procThread->send(evt);
                                numOfSuspendedProc++;
                                break;
			}
		default:
			break;
		}
	}

        /* If no suspended/undone procedures exists something is really wrong */
        if (numOfSuspendedProc == 0) {
                LOG_ER("SmfCampStateSuspendedByErrorDetected: No suspended/undone procedures found when rollback");
		changeState(i_camp, SmfCampRollbackFailed::instance());
        }

        i_camp->m_noOfExecutingProc = numOfSuspendedProc;

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampRollingBack implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampRollingBack::s_instance = NULL;

SmfCampState *SmfCampRollingBack::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampRollingBack;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampRollingBack::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampRollingBack";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampRollingBack::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::rollback(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampRollingBack::rollback implementation");

        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_reverse_iterator iter;

	//If a running campaign was restarted on an other node, the procedures in rolling back state
	//must be restarted. The execution shall continue at step rollback phase. 
	bool execProcFound = false;
	
	i_camp->m_noOfExecutingProc = 0; //The no of answers which must be wait for, could be more than 1 if parallel procedures
	for (iter = procedures.rbegin(); iter != procedures.rend(); iter++) {
		if ((*iter)->getState() == SA_SMF_PROC_ROLLING_BACK) {
			TRACE("SmfCampRollingBack::rollback, restarted procedure found, send PROCEDURE_EVT_ROLLBACK_STEP event to %s",
                              (*iter)->getProcName().c_str());
			SmfProcedureThread *procThread = (*iter)->getProcThread();
			PROCEDURE_EVT *evt = new PROCEDURE_EVT();
			evt->type = PROCEDURE_EVT_ROLLBACK_STEP;
			procThread->send(evt);
			execProcFound = true;
			i_camp->m_noOfExecutingProc++; 
		}
	}

	if (execProcFound == true) {
		//Execution will continue at SmfCampRollingBack::rollbackProc() when proc result is received.
		TRACE("SmfCampRollingBack::rollback, Wait for restarted procedures to respond.");
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	}

        /* No running procedures, continue with next procedure */
        SmfCampResultT procResult = rollbackProc(i_camp);
	TRACE_LEAVE();
        return procResult;
}

//------------------------------------------------------------------------------
// rollbackInit()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::rollbackInit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

        TRACE("SmfCampRollingBack::rollbackInit implementation");

        if (i_camp->m_campInit.rollback() != SA_AIS_OK) {
		std::string error = "Campaign init rollback failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		return SMF_CAMP_FAILED;
	}

        changeState(i_camp, SmfCampRollbackCompleted::instance());
        LOG_NO("CAMP: Upgrade campaign rollback completed %s", i_camp->getCampaignName().c_str());
        TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
// rollbackProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::rollbackProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("Proceed rollback with next execution level procedures");

	//The procedure vector is sorted in execution level order (low -> high)
	//Highest number shall be rolled back first so start from end of list.

        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_reverse_iterator iter;

        int execLevel = -1;
	
	for (iter = procedures.rbegin(); iter != procedures.rend(); iter++) {

		TRACE("Rollback procedures, try procedure %s, state=%u, execLevel=%d",
		      (*iter)->getProcName().c_str(), (*iter)->getState(), (*iter)->getExecLevel());

		// If the state is completed and the execution level is new or the same as 
		// the procedure just started.
		if (((*iter)->getState() == SA_SMF_PROC_COMPLETED)
		    && ((execLevel == -1) || (execLevel == (*iter)->getExecLevel()))) {
			if (execLevel == (*iter)->getExecLevel()) {
				TRACE("A procedure already run at execLevel=%d, start parallel procedure", execLevel);
			} else if (execLevel == -1) {
				TRACE("Start first  procedure on  execLevel=%d", (*iter)->getExecLevel());
			}

			SmfProcedureThread *procThread = (*iter)->getProcThread();
			PROCEDURE_EVT *evt = new PROCEDURE_EVT();
			evt->type = PROCEDURE_EVT_ROLLBACK;
			procThread->send(evt);
			execLevel = (*iter)->getExecLevel();
			i_camp->m_noOfExecutingProc++;
		}
	}

	if (execLevel != -1) {
		TRACE("SmfCampRollingBack::rollbackProc, Wait for procedure results");
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	}

	LOG_NO("CAMP: All procedures rolled back");

        /* End with rollback of init */
        SmfCampResultT initResult = rollbackInit(i_camp);
	TRACE_LEAVE();
        return initResult;
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::suspend(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampRollingBack::suspend implementation");

	/* Send suspend message to all procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
                TRACE("SmfCampRollingBack::Procedure %s, send suspend",
                      (*iter)->getProcName().c_str());
                SmfProcedureThread *procThread = (*iter)->getProcThread();
                PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                evt->type = PROCEDURE_EVT_SUSPEND;
                procThread->send(evt);
	}

        i_camp->m_noOfProcResponses = 0;
        changeState(i_camp, SmfCampSuspendingRollback::instance());
        /* Wait for suspend responses from all procedures (SmfCampSuspendingRollback::procResult) */

	TRACE_LEAVE();
        return SMF_CAMP_SUSPENDING; 
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::procResult(SmfUpgradeCampaign *  i_camp,
                               SmfUpgradeProcedure * i_procedure,
                               SmfProcResultT        i_result)
{
	TRACE_ENTER();

        switch (i_result) {
        case SMF_PROC_ROLLEDBACK: {
                LOG_NO("CAMP: Procedure %s returned ROLLEDBACK", i_procedure->getProcName().c_str());
                break;
        }
        case SMF_PROC_ROLLBACKFAILED: {
                LOG_NO("CAMP: Procedure %s returned ROLLBACKFAILED", i_procedure->getProcName().c_str());
                std::string error = "Procedure " + i_procedure->getProcName() + " failed rollback";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampRollbackFailed::instance());
		TRACE_LEAVE();
		return SMF_CAMP_FAILED;
                break;
        }
        default: {
                LOG_ER("SmfCampRollingBack: Procedure %s returned unhandled response %d", 
                       i_procedure->getProcName().c_str(), i_result);
                break;
        }
        }

	if (i_camp->m_noOfExecutingProc > 1) {
		i_camp->m_noOfExecutingProc--;
		TRACE("Still noProc=%d procedures rolling back, continue waiting for result.", i_camp->m_noOfExecutingProc);
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
	} else if (i_camp->m_noOfExecutingProc == 1) {
		TRACE("Response from last procedure received.");
		//Decrement counter for the last procedure on the current exec level 
		i_camp->m_noOfExecutingProc--;
	}

	TRACE("All procedures rolled back on the same execlevel have answered.");

        /* Find next procedure to be rolled back */
        SmfCampResultT result = this->rollbackProc(i_camp);
        TRACE_LEAVE();
        return result;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampRollbackSuspended implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampRollbackSuspended::s_instance = NULL;

SmfCampState *SmfCampRollbackSuspended::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampRollbackSuspended;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampRollbackSuspended::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampRollbackSuspended";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampRollbackSuspended::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollbackSuspended::rollback(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampRollbackSuspended::rollback implementation");

	/* Send rollback to all suspended procedures */
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

	for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_ROLLBACK_SUSPENDED:
			{
				TRACE("SmfCampRollbackSuspended::Procedure %s suspended, send rollback",
				      (*iter)->getProcName().c_str());
				SmfProcedureThread *procThread = (*iter)->getProcThread();
				PROCEDURE_EVT *evt = new PROCEDURE_EVT();
				evt->type = PROCEDURE_EVT_ROLLBACK;
				procThread->send(evt);
                                numOfSuspendedProc++;
                                break;
			}
		default:
			break;
		}
	}

	changeState(i_camp, SmfCampRollingBack::instance());
        i_camp->m_noOfExecutingProc = numOfSuspendedProc;

        /* If no suspended procedures existed, rollback next completed procedure.
           I.e. we were suspended in between procedures */
        if (numOfSuspendedProc == 0) {
                TRACE("SmfCampRollbackSuspended::No suspended procedures, start rollback next procedure");

                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_ROLLBACK_PROC;
                SmfCampaignThread::instance()->send(evt);
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampSuspendingRollback implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampSuspendingRollback::s_instance = NULL;

SmfCampState *SmfCampSuspendingRollback::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampSuspendingRollback;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampSuspendingRollback::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampSuspendingRollback";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampSuspendingRollback::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// procResult()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampSuspendingRollback::procResult(SmfUpgradeCampaign *  i_camp,
                                      SmfUpgradeProcedure * i_procedure,
                                      SmfProcResultT        i_result)
{
	TRACE_ENTER();
	TRACE("SmfCampSuspendingRollback::procResult implementation");

        /* We could get other responses */
        switch (i_result) {
        case (SMF_PROC_SUSPENDED): {
                /* If first response, set number of expected responses */
                if (i_camp->m_noOfProcResponses == 0) {
                        i_camp->m_noOfProcResponses = i_camp->m_procedure.size();
                }

                /* Decrease the response counter */
                i_camp->m_noOfProcResponses--;
        
                /* If last response, change state to rollback suspended */
                if (i_camp->m_noOfProcResponses == 0) {
        		changeState(i_camp, SmfCampRollbackSuspended::instance());
        	}
                break;
        } 
        case (SMF_PROC_ROLLBACKFAILED): {
                TRACE("SmfCampSuspendingRollback::procResult received ROLLBACKFAILED from procedure %s", 
                      i_procedure->getDn().c_str());
                changeState(i_camp, SmfCampRollbackFailed::instance());
                break;
        }
        default: {
                LOG_NO("SmfCampSuspendingRollback::procResult received unhandled response %d from procedure %s", 
                       i_result, i_procedure->getDn().c_str());
                break;
        }
        }

	TRACE_LEAVE();
        return SMF_CAMP_DONE;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampRollbackCompleted implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampRollbackCompleted::s_instance = NULL;

SmfCampState *SmfCampRollbackCompleted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampRollbackCompleted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampRollbackCompleted::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampRollbackCompleted";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampRollbackCompleted::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollbackCompleted::commit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampRollbackCompleted::commit implementation");

        LOG_NO("CAMP: Start rollback commit campaign %s", i_camp->getCampaignName().c_str());

	i_camp->m_campWrapup.executeCampWrapup(); // No action if wrapup is faulty

        i_camp->resetMaintenanceState(); // No action if it fails

        //Remove the procedure runtime objects
        const std::vector < SmfUpgradeProcedure * >& procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::const_iterator iter;

        for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
		(*iter)->commit();
	}

        i_camp->removeRunTimeObjects(); // No action if it fails

	changeState(i_camp, SmfCampRollbackCommitted::instance());
	LOG_NO("CAMP: Upgrade campaign rollback committed %s", i_camp->getCampaignName().c_str());

	// TODO Start wait to allow new campaign timer
	LOG_NO("CAMP: Start wait to allow new campaign timer (not implemented yet)");

	/* Terminate campaign thread */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
        return SMF_CAMP_COMMITED;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampRollbackCommitted implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampRollbackCommitted::s_instance = NULL;

SmfCampState *SmfCampRollbackCommitted::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampRollbackCommitted;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampRollbackCommitted::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampRollbackCommitted";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampRollbackCommitted::toString(std::string & io_str) const 
{
	getClassName(io_str);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfCampRollbackFailed implementation
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SmfCampState *SmfCampRollbackFailed::s_instance = NULL;

SmfCampState *SmfCampRollbackFailed::instance(void)
{
	if (s_instance == NULL) {
		s_instance = new SmfCampRollbackFailed;
	}

	return s_instance;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
void 
SmfCampRollbackFailed::getClassName(std::string & io_str) const 
{
	io_str = "SmfCampRollbackFailed";
}

//------------------------------------------------------------------------------
// toString()
//------------------------------------------------------------------------------
void 
SmfCampRollbackFailed::toString(std::string & io_str) const 
{
	getClassName(io_str);
}
