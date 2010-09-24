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

extern struct ImmutilWrapperProfile immutilWrapperProfile;

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
void 
SmfCampState::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::execute default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfCampState::executeInit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::executeInit default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeProc()
//------------------------------------------------------------------------------
void 
SmfCampState::executeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::executeProc default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfCampState::executeWrapup(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::executeWrapup default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
void 
SmfCampState::rollback(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::rollback default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfCampState::suspend(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::suspend default implementation, should NEVER be executed.");
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfCampState::commit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::commit default implementation, should NEVER be executed.");
	TRACE_LEAVE();
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
void 
SmfCampStateInitial::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	std::string error;
	std::string s;
	std::stringstream out;
	std::vector < SmfUpgradeProcedure * >::iterator procIter;

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

	LOG_NO("CAMP: Start executing campaign %s", i_camp->getCampaignName().c_str());

	TRACE("Create the SmfCampRestartInfo object");
	i_camp->createCampRestartInfo();

	//Preparation is ready, change state and continue the campaign execution
	changeState(i_camp, SmfCampStateExecuting::instance());
	i_camp->execute();

	TRACE_LEAVE();
	return;

exit_error:
	LOG_ER("%s", error.c_str());
	SmfCampaignThread::instance()->campaign()->setError(error);

	//Remain in state initial if prerequsites check or SMF backup fails
//	changeState(i_camp, SmfCampStateExecFailed::instance());

	/* Terminate campaign thread */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
	return;
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
void 
SmfCampStateExecuting::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	TRACE("SmfCampStateExecuting::execute, Do some checking");

	/* Check if the campaign have been restarted to many times */
	bool o_result;
	if (i_camp->tooManyRestarts(&o_result) == SA_AIS_OK){
		if (o_result == true) {
			LOG_ER("The campaign have been restarted to many times");
			std::string cnt = getenv("CAMP_MAX_RESTART");
			std::string error = "To many campaign restarts, max " + cnt;
			SmfCampaignThread::instance()->campaign()->setError(error);
			changeState(i_camp, SmfCampStateExecFailed::instance());
			TRACE_LEAVE();
			return;
		}
	} else {
		LOG_ER("toManyRestarts() fails");
		std::string error = "Restart number check fails";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return;
	}

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	//Set the DN of the procedures
	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {
		//Set the DN of the procedure
		std::string dn = (*iter)->getProcName() + "," + SmfCampaignThread::instance()->campaign()->getDn();
		(*iter)->setDn(dn);

		iter++;
	}

	//Start procedure threads
	TRACE("SmfCampStateExecuting::execute, start procedure threads and set initial state");
	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {
		SmfProcedureThread *procThread = new SmfProcedureThread(*iter);
		(*iter)->setProcThread(procThread);

		TRACE("SmfCampStateExecuting::executeProc, Starting procedure %s", (*iter)->getProcName().c_str());
		procThread->start();

		iter++;
	}

	//If a running campaign was restarted on an other node, the procedures in executing state
	//must be restarted. The execution shall continue at step execution phase. The procedure initiation
	//and step calculation was performed before the move of control.
	bool execProcFound = false;
	iter = i_camp->m_procedure.begin();
	i_camp->m_noOfExecutingProc = 0; //The no of answers which must be wait for, could be more than 1 if parallel procedures
	while (iter != i_camp->m_procedure.end()) {
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
		//Execution will continue at SmfCampStateExecuting::executeProc() when first proc result is received.
		TRACE("SmfCampStateExecuting::execute, Wait for restarted procedures to respond.");
		TRACE_LEAVE();
		return;
	}

	/* No executing procedures, this must be a campaign start so execute campaign inititialization */
        this->executeInit(i_camp);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeInit()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::executeInit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecuting::executeInit, Running campaign init actions");

	if (i_camp->m_campInit.execute() != SA_AIS_OK) {
		std::string error = "Campaign init failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		return;
	}

	TRACE("SmfCampStateExecuting::executeInit, campaign init actions completed");
        this->executeProc(i_camp);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeProc()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::executeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecuting::executeProc, Starting procedure threads");

	if (i_camp->m_noOfExecutingProc > 1) {
		i_camp->m_noOfExecutingProc--;
		TRACE("Still noProc=%d procedures executing, continue waiting for result.", i_camp->m_noOfExecutingProc);
		TRACE_LEAVE();
		return;
	} else if (i_camp->m_noOfExecutingProc == 1) {
		//Decrement counter for the last procedure on the current exec level 
		i_camp->m_noOfExecutingProc--;
	}

	TRACE("All procedures started on the same execlevel have answered.");
	//Check that the counter is 0, otherwise fail
	if ( i_camp->m_noOfExecutingProc != 0)
	{
		LOG_ER("Procedure counter is supposed to be 0, was %d", i_camp->m_noOfExecutingProc);
		std::string error = "Wrong procedure counter " + i_camp->m_noOfExecutingProc;
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return;
	}
	
	//All procedures started on the execution level has send results
	//Find out if some procedure is still executing, should never happend here
	//Find out if any of the procedures failed

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {
		switch ((*iter)->getState()) {
		case SA_SMF_PROC_EXECUTING: //Shold never happends since all procedures are assumed to have answered
			{
				LOG_ER("No procdedures are supposed to execute but procedure %s still in SA_SMF_PROC_EXECUTING state", (*iter)->getProcName().c_str());
				std::string error = "Procedure in wrong state (SA_SMF_PROC_EXECUTING)";
				SmfCampaignThread::instance()->campaign()->setError(error);
				changeState(i_camp, SmfCampStateExecFailed::instance());
				TRACE_LEAVE();
				return;
			}
		case SA_SMF_PROC_FAILED:
			{
				std::string error = "Procedure " + (*iter)->getProcName() + " failed";
				LOG_ER("%s", error.c_str());
				SmfCampaignThread::instance()->campaign()->setError(error);
				changeState(i_camp, SmfCampStateExecFailed::instance());
				TRACE_LEAVE();
				return;
			}
		default:
			break;
		}
		iter++;
	}

	TRACE("No procedures in executing state was found, proceed with next execution level procedures");

	//The procedure vector is sorted in execution level order (low -> high)
	//Lowest number shall be executed first.

	int execLevel = -1;
	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {

		TRACE("Start procedures, try procedure %s, state=%u, execLevel=%d",
		      (*iter)->getProcName().c_str(), (*iter)->getState(), (*iter)->getExecLevel());

		//If the state is initial and the execution level is new or the same as 
		//the procedure just started.
		if (((*iter)->getState() == SA_SMF_PROC_INITIAL)
		    && ((execLevel == -1) || (execLevel == (*iter)->getExecLevel()))) {
			if (execLevel == (*iter)->getExecLevel()) {
				TRACE("A procedure already run at execLevel=%d, start parallel procedure", execLevel);
			} else if (execLevel == -1) {
				TRACE("Start first  procedure on  execLevel=%d", (*iter)->getExecLevel());
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
		return;
	}

	TRACE("SmfCampStateExecuting::executeProc, All procedures executed, start wrapup");
        this->executeWrapup(i_camp);

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// executeWrapup()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::executeWrapup(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_NO("CAMP: Running campaign wrapup complete actions");

	if (i_camp->m_campWrapup.executeCampComplete() == false) {
		std::string error = "Campaign wrapup.executeCampComplete failed";
		LOG_ER("%s", error.c_str());
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		return;
	}
	// TODO Start wait to complete timer
	LOG_NO("CAMP: Start wait to complete timer (not implemented yet)");

	/* TODO State completed should be set when waitToComplete times out */
	changeState(i_camp, SmfCampStateExecCompleted::instance());

	LOG_NO("CAMP: Upgrade campaign completed %s", i_camp->getCampaignName().c_str());
	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// suspend()
//------------------------------------------------------------------------------
void 
SmfCampStateExecuting::suspend(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecuting::suspend implementation");

	/* Send suspend message to all procedures */
	std::vector < SmfUpgradeProcedure * >::iterator iter;

	for (iter = i_camp->m_procedure.begin(); iter != i_camp->m_procedure.end(); ++iter) {
                TRACE("SmfCampStateExecuting::Procedure %s executing, send suspend",
                      (*iter)->getProcName().c_str());
                SmfProcedureThread *procThread = (*iter)->getProcThread();
                PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                evt->type = PROCEDURE_EVT_SUSPEND;
                procThread->send(evt);
	}

        changeState(i_camp, SmfCampStateSuspendingExec::instance());
        /* Wait for responses from all procedures (SmfCampStateSuspendingExec::execProc) */

	TRACE_LEAVE();
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
// execute()
//------------------------------------------------------------------------------
void 
SmfCampStateExecCompleted::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	TRACE("SmfCampStateExecuting::execute, Do some checking");

	/* Check if the campaign have been restarted to many times */
	bool o_result;
	if (i_camp->tooManyRestarts(&o_result) == SA_AIS_OK){
		if (o_result == true) {
			LOG_ER("The campaign have been restarted to many times");
			std::string cnt = getenv("CAMP_MAX_RESTART");
			std::string error = "To many campaign restarts, max " + cnt;
			SmfCampaignThread::instance()->campaign()->setError(error);
			changeState(i_camp, SmfCampStateExecFailed::instance());
			TRACE_LEAVE();
			return;
		}
	} else {
		LOG_ER("toManyRestarts() fails");
		std::string error = "Restart number check fails";
		SmfCampaignThread::instance()->campaign()->setError(error);
		changeState(i_camp, SmfCampStateExecFailed::instance());
		TRACE_LEAVE();
		return;
	}

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	//Set the DN of the procedures
	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {
		//Set the DN of the procedure
		std::string dn = (*iter)->getProcName() + "," + SmfCampaignThread::instance()->campaign()->getDn();
		(*iter)->setDn(dn);

		iter++;
	}

	//Start procedure threads
	TRACE("SmfCampStateExecuting::execute, start procedure threads and set initial state");
	iter = i_camp->m_procedure.begin();
	while (iter != i_camp->m_procedure.end()) {
		SmfProcedureThread *procThread = new SmfProcedureThread(*iter);
		(*iter)->setProcThread(procThread);

		TRACE("SmfCampStateExecuting::executeProc, Starting procedure %s", (*iter)->getProcName().c_str());
		procThread->start();

		iter++;
	}

	TRACE_LEAVE();
}

//------------------------------------------------------------------------------
// commit()
//------------------------------------------------------------------------------
void 
SmfCampStateExecCompleted::commit(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();

	i_camp->m_campWrapup.executeCampWrapup(); // No action if wrapup is faulty

	//Remove the procedure runtime objects
	std::vector < SmfUpgradeProcedure * >::iterator iter;
	for (iter = i_camp->m_procedure.begin(); iter != i_camp->m_procedure.end(); ++iter) {
		(*iter)->commit();
	}

	/* Remove smfRestartInfo runtime object */
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;

	SaNameT objectName;
	std::string dn = "smfRestartInfo=info," + SmfCampaignThread::instance()->campaign()->getDn(); 
	objectName.length = dn.length();
	strncpy((char *)objectName.value, dn.c_str(), objectName.length);
	objectName.value[objectName.length] = 0;

	SaAisErrorT rc = immutil_saImmOiRtObjectDelete(SmfCampaignThread::instance()->getImmHandle(),	//The OI handle
					   &objectName);

	if (rc != SA_AIS_OK) {
		LOG_ER("immutil_saImmOiRtObjectDelete returned %u for %s, continuing", rc, dn.c_str());
	}

	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	changeState(i_camp, SmfCampStateCommitted::instance());
	LOG_NO("CAMP: Upgrade campaign committed %s", i_camp->getCampaignName().c_str());

	// TODO Start wait to allow new campaign timer
	LOG_NO("CAMP: Start wait to allow new campaign timer (not implemented yet)");

	/* Terminate campaign thread */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	SmfCampaignThread::instance()->send(evt);

	TRACE_LEAVE();
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
// SmfCampStateSuspendingExec()
//------------------------------------------------------------------------------
SmfCampStateSuspendingExec::SmfCampStateSuspendingExec(void):
        m_numOfProcResponses(0)
{
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
// suspend()
//------------------------------------------------------------------------------
void 
SmfCampStateSuspendingExec::executeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateSuspendingExec::executeProc implementation");

        /* If first response, set number of expected responses */
        if (m_numOfProcResponses == 0) {
                m_numOfProcResponses = i_camp->m_procedure.size();
        }

        /* Decrease the response counter */
        m_numOfProcResponses--;

        /* If last response, change state to suspended */
        if (m_numOfProcResponses == 0) {
		changeState(i_camp, SmfCampStateExecSuspended::instance());
	}

	TRACE_LEAVE();
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
void 
SmfCampStateExecSuspended::execute(SmfUpgradeCampaign * i_camp)
{
        int numOfSuspendedProc = 0;
	TRACE_ENTER();
	TRACE("SmfCampStateExecSuspended::execute implementation");

	/* Send execute to all suspended procedures */
	std::vector < SmfUpgradeProcedure * >::iterator iter;

	for (iter = i_camp->m_procedure.begin(); iter != i_camp->m_procedure.end(); ++iter) {
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

        /* If no suspended procedures existed, execute next procedure in state initial.
           I.e. we were suspended in between procedures */
        if (numOfSuspendedProc == 0) {
                /* Execute next procedure */
                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_EXECUTE_PROC;
                SmfCampaignThread::instance()->send(evt);
        }

	TRACE_LEAVE();
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
// execute()
//------------------------------------------------------------------------------
void 
SmfCampStateCommitted::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateCommitted::execute implementation");
	TRACE_LEAVE();
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
// execute()
//------------------------------------------------------------------------------
void 
SmfCampStateErrorDetected::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateErrorDetected::execute implementation");
	TRACE_LEAVE();
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
// execute()
//------------------------------------------------------------------------------
void 
SmfCampStateExecFailed::execute(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	TRACE("SmfCampStateExecFailed::execute implementation");
	TRACE_LEAVE();
}
