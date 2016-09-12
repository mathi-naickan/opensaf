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
#include "smfsv_defs.h"
#include <immutil.h>
#include <sstream>
#include "SmfUpgradeAction.hh"
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
// rollbackSingleMergeProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampState::rollbackSingleMergeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::rollbackSingleMergeProc default implementation, should NEVER be executed.");
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
// verify()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCampState::verify(SmfUpgradeCampaign * i_camp, std::string & error)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::verify default implementation, should NEVER be executed.");
	TRACE_LEAVE();
	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// prerequsitescheck()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCampState::prerequsitescheck(SmfUpgradeCampaign * i_camp, std::string &error)
{
	TRACE_ENTER();
	LOG_ER("SmfCampState::prerequsitescheck default implementation, should NEVER be executed.");
	TRACE_LEAVE();
	return SA_AIS_OK;
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
	SmfCampResultT initResult;
        auto exechdl = i_camp->getExecControlHdl();

	TRACE("SmfCampStateInitial::execute implementation");

	LOG_NO("CAMP: Start upgrade campaign %s", i_camp->getCampaignName().c_str());

	//Reset error string in case the previous prerequsites check failed
        //In such case an error string is present in initial state
	SmfCampaignThread::instance()->campaign()->setError("");

	if (prerequsitescheck(i_camp, error) != SA_AIS_OK) {
		goto exit_error;
	}
        if (exechdl->smfProtectExecControlDuringInit(NULL)) {
                exechdl->install();
        }

	//Verify pre-check
	if (smfd_cb->smfVerifyEnable == SA_TRUE) {
		if (verify(i_camp, error) != SA_AIS_OK) {
			goto exit_error;
		}
	}

	//Prerequisite  check 11 "All neccessary backup is created"
	if (i_camp->m_campInit.executeCallbackAtBackup() != SA_AIS_OK) {
		error = "Campaign init backup callback failed";
		goto exit_error;
	}
	LOG_NO("CAMP: executed callbackAtBackup successfully in the campaign %s", i_camp->getCampaignName().c_str());

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

	//Disable IMM PBE
	if (i_camp->disablePbe() != SA_AIS_OK) {
		error = "Fails to disable IMM PBE";
		goto exit_error;
	}
	//Preparation is ready, change state and execute campaign initialization
	changeState(i_camp, SmfCampStateExecuting::instance());

        initResult = executeInit(i_camp);
        if (initResult != SMF_CAMP_FAILED) {
		//If the campaign init portion contain changes to SMF config there may be
		//a race condition between the smfd main process (OI) which set the smfd_cb
		//and this thread executing right here (the campaign) when reading the same smfd_cb.
		//To be sure the smfd_cb is updated also for this thread, read the config again.
		read_config_and_set_control_block(smfd_cb);

                if (!exechdl->smfProtectExecControlDuringInit(NULL)) {
                        exechdl->install();
                }

		LOG_NO("SmfCampStateInitial::execute: startProcedureThreads()");
                if (SmfCampaignThread::instance()->campaign()->startProcedureThreads() != SA_AIS_OK) {
                        LOG_NO("Fail to start procedure threads");
                        initResult = SMF_CAMP_FAILED;
                }
        }
	TRACE_LEAVE();
	return initResult;

exit_error:
	LOG_NO("%s", error.c_str());
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
// Verify_campaign()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCampStateInitial::verify(SmfUpgradeCampaign * i_camp, std::string &error)
{
	TRACE_ENTER();
	SmfCallback    cbk;
	std::string    initDnStr = "";					// not used in the initial state

	cbk.m_atAction      = SmfCallback::atCampVerify;
	cbk.m_callbackLabel = "OsafSmfCbkVerify";
	cbk.m_stringToPass  = "";
	cbk.m_time          = smfd_cb->smfVerifyTimeout;

	SaAisErrorT rc = cbk.execute(initDnStr);

	if (SA_AIS_OK != rc) {
		error = smf_valueToString(&rc, SA_IMM_ATTR_SAINT32T);
		error = "Verify Failed, application error code = " + error;
	}

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// prerequsitescheck()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCampStateInitial::prerequsitescheck(SmfUpgradeCampaign * i_camp, std::string &error)
{
	TRACE_ENTER();
	std::string s;
	std::stringstream out;
	std::vector < SmfUpgradeProcedure * >::iterator procIter;
	std::list < std::string > bundleDnCamp;   //DNs of all bundles to be installed and removed found in campaign.xml
	std::list < std::string >::iterator dnIter;
        std::list < std::string > addToImmBundleDn;
        std::list < SmfImmOperation * > immOper;
        std::list < SmfImmOperation * >::iterator operIter;

        bool bundleCheckErrorFound = false;
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

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
	//
	//1) The campaign is searched for bundles to add and remove. They are all stored in the same list.
	//2) If bundles are created in the <addToImm> portion of the campaign they are excluded from the bundle list
        //3) The configured smfBundleCheck command is invoked for each bundle in the bundle list
        //4) The existance of bundle IMM instances matching the bundle list are checked

	LOG_NO("CAMP: Check bundles to install and remove.");
        //Find bundles in the campaign xml
	procIter = i_camp->m_procedure.begin();
	while (procIter != i_camp->m_procedure.end()) {
		//Fetch the list of SW to add from each procedure
		SaSmfUpgrMethodT upType = (*procIter)->getUpgradeMethod()->getUpgradeMethod();
		if (upType == SA_SMF_ROLLING) {
			TRACE("SA_SMF_ROLLING procedure detected");
			const SmfByTemplate *byTemplate = (SmfByTemplate*)(*procIter)->getUpgradeMethod()->getUpgradeScope();
			if (byTemplate == NULL) {
				TRACE("No upgrade scope found");
				error = "CAMP: Procedure upgrade scope not found";
				goto exit_error;
			}

			std::list < SmfBundleRef * > b = byTemplate->getTargetNodeTemplate()->getSwInstallList();

			std::list< SmfBundleRef* >::const_iterator bIter;
			bIter = b.begin();
			while (bIter != b.end()) {
				bundleDnCamp.push_back((*bIter)->getBundleDn());
				bIter++;
			}

                        b = byTemplate->getTargetNodeTemplate()->getSwRemoveList();
			bIter = b.begin();
			while (bIter != b.end()) {
				bundleDnCamp.push_back((*bIter)->getBundleDn());
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
				std::list < SmfBundleRef > b = modify->getActivationUnit()->getSwAdd();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}
                                b = modify->getActivationUnit()->getSwRemove();
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}

			}else if(addRemove != 0) { //Check if the upgradeScope is SmfForAddRemove
				TRACE("SA_SMF_SINGLE_STEP procedure with scope forAddRemove detected");
				std::list < SmfBundleRef > b = addRemove->getActivationUnit()->getSwAdd();
				std::list< SmfBundleRef >::const_iterator bIter;
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleDnCamp.push_back((*bIter).getBundleDn());
					bIter++;
				}

                                b = addRemove->getDeactivationUnit()->getSwRemove();
				bIter = b.begin();
				while (bIter != b.end()) {
					bundleDnCamp.push_back((*bIter).getBundleDn());
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

	bundleDnCamp.sort();
	bundleDnCamp.unique();

	TRACE("Total number of bundles in the campaign = %zd", bundleDnCamp.size());

        //Find the bundles which will be created in the <addToImm> portion of the campaign.xml
        TRACE("Find the bundles which will be created by the campaign.xml");
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
                                                TRACE("SW Bundle %s will be created by campaign", val.c_str());
                                                addToImmBundleDn.push_back(val);
                                                break;
                                        }
                                        attrIter++;
                                }
                        }
                }
                operIter++;
        }
        addToImmBundleDn.sort();
        addToImmBundleDn.unique();

        //Remove bundles to be created by campaign <addToImm> from the list of bundles
        dnIter = addToImmBundleDn.begin();
        while (dnIter != addToImmBundleDn.end()) {
                TRACE("Remove addToImm bundle %s from list of bundles", (*dnIter).c_str());
                bundleDnCamp.remove(*dnIter);
                dnIter++;
        }

        //Call the configured bundleCheckCmd for all bundles listed in <swAdd> and <swRemove> portion of the campaign.xml.
        //Bundles which does not yet exist in IMM, but will be created by the campaign are not checked.
        LOG_NO("CAMP: Calling configured smfBundleCheckCmd for each bundle existing in IMM, to be installed or removed by the campaign");
        dnIter = bundleDnCamp.begin();
        while (dnIter != bundleDnCamp.end()) {
                std::string cmd = smfd_cb->bundleCheckCmd;
                cmd = cmd + " " + *dnIter;
                int rc = smf_system(cmd);
                if (rc != 0) {
                        bundleCheckErrorFound = true;
                        LOG_NO("CAMP: bundleCheckCmd fail [%s] [rc=%d]", cmd.c_str(), rc);
                        dnIter++;
                        continue;
                }

		if (immUtil.getObject((*dnIter), &attributes) == true) { //found
			TRACE("SW Bundle [%s] found in IMM", (*dnIter).c_str());
		} else {
                        bundleCheckErrorFound = true;
			LOG_NO("CAMP: SW bundle [%s] was not found in system or campaign", (*dnIter).c_str());
		}

                dnIter++;
        }

        if(bundleCheckErrorFound == true) {
                LOG_NO("CAMP: Bundle check error");
                error = "Bundle check error";
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
	if (i_camp->m_campInit.executeCallbackAtInit() != SA_AIS_OK) {
		error = "Campaign callback at init failed";
		goto exit_error;
	}

	// Check if parent/type has incorrect object DNs. This is an extra  prerequisite check not given in SMF specification
	procIter = i_camp->m_procedure.begin();
	while (procIter != i_camp->m_procedure.end()) {
		SaSmfUpgrMethodT upType = (*procIter)->getUpgradeMethod()->getUpgradeMethod();
		if (upType != SA_SMF_ROLLING) {
			procIter++; /* go to the next procedure */
			continue;
		}
		SmfRollingUpgrade *i_rollingUpgrade = (SmfRollingUpgrade *) (*procIter)->getUpgradeMethod();
		const SmfByTemplate *byTemplate = (const SmfByTemplate *)i_rollingUpgrade->getUpgradeScope();
		if (byTemplate == NULL) {
			LOG_ER("SmfCampStateInitial::execute: no upgrade scope by template found");
			error = "CAMP: No upgrade scope by template found";
			goto exit_error;
		}

		const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();
		const std::list < SmfParentType * >&actUnitTemplates = nodeTemplate->getActivationUnitTemplateList();
		if (actUnitTemplates.size() == 0) {
			procIter++;
			continue;
		}
		std::list < SmfParentType * >::const_iterator it;
		for (it = actUnitTemplates.begin(); it != actUnitTemplates.end(); ++it) { 
			if (((*it)->getParentDn().size() != 0) && 
				(strncmp((*it)->getParentDn().c_str(), "safSg=", strlen("safSg=")) != 0)) {
				LOG_ER("SmfCampStateInitial::execute: given DN in parent %s is not SG's DN", (*it)->getParentDn().c_str());
				error = "CAMP: parent object DN is not SG DN";
				goto exit_error;
			}

			if ((*it)->getTypeDn().size() == 0) 
				continue; /* type is optional */

			if (strncmp((*it)->getTypeDn().c_str(), "safVersion=", strlen("safVersion=")) != 0) {
				LOG_ER("SmfCampStateInitial::execute: given DN in type %s is not versioned CompType/SUType DN", (*it)->getTypeDn().c_str());
				error = "CAMP: type object DN is not versioned type of CompType/SuType";
				goto exit_error;
			} else {
				std::string temp = (*it)->getTypeDn().c_str();
				std::string parentDn = (temp).substr((temp).find(',') + 1, std::string::npos);
				if ((strncmp(parentDn.c_str(), "safCompType=", strlen("safCompType=")) != 0) 
					&& (strncmp(parentDn.c_str(), "safSuType=", strlen("safSuType=")) != 0)) {
					LOG_ER("SmfCampStateInitial::execute: given DN in type %s is not CompType/SUType DN", (*it)->getTypeDn().c_str());
					error = "CAMP: type object DN is not CompType/SuType DN";
					goto exit_error;
				}
			}
		}
		procIter++;
	}


	TRACE_LEAVE();
	return SA_AIS_OK;

exit_error:
	TRACE_LEAVE();
	return SA_AIS_ERR_FAILED_OPERATION;
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

        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

        std::vector < SmfUpgradeProcedure * >::iterator iter;
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

        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();
	std::vector < SmfUpgradeProcedure * >::iterator iter;
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

	//Activate IMM BPE if active when campaign was started.
	i_camp->restorePbe();

	// Wait for IMM to become writable again by keep on writing until it
	// succeeds. Write the same value, just for synchronization purposes.
	changeState(i_camp, SmfCampStateExecuting::instance());

	// IMM is writeable again so let's execute campCompleteAction
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

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
                std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

        	std::vector < SmfUpgradeProcedure * >::iterator iter;
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

        //Disable IMM PBE
        if (i_camp->disablePbe() != SA_AIS_OK) {
                LOG_NO("CAMP: Fail to disable IMM PBE at rollback, continue");
        }

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
        for (auto proc: i_camp->getProcedures()) {
            proc->commit();
        }

        i_camp->removeRunTimeObjects(); // No action if it fails
	i_camp->removeConfigObjects();  // No action if it fails

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
// execute()
//------------------------------------------------------------------------------
SmfCampResultT
SmfCampStateSuspendingExec::execute(SmfUpgradeCampaign * i_camp)
{
	/* This method handles a special case:
	 *
	 *    This method called only in case of continuing a campaign after cluster reboot,
	 *    where the campaign state is "SuspendingExec".
	 *    That special case only happens when the campaign suspended during execution of the cluster reboot step.
	 *    Since that step is not able to set the "ExecSuspended" state after execution,
	 *    the campaign remains in "SuspendingExec" state.
	 *
	 *    This method executes one of the following:
	 *
	 *       -if procedures left to execute:
	 *          change camp state to "ExecSuspended",
	 *          and change the status of procedures which have "Executing" to "Suspended"
	 *
	 *       -else (this was the last procedure, no more left):
	 *          change camp state to "Executing"
	 */
	TRACE_ENTER();
	TRACE("SmfCampStateSuspendingExec::execute implementation");

        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;
	bool initialFound = false;

	// Searching if any procedure is in initial status.
	for (iter = procedures.begin(); iter != procedures.end(); iter++) {
		if((*iter)->getState() == SA_SMF_PROC_INITIAL)	{
			TRACE("SmfCampStateSuspendingExec::execute SA_SMF_PROC_INITIAL found");
			initialFound = true;
			break;
		}
	}

	if(initialFound) {
		TRACE("SmfCampStateSuspendingExec::execute this is not the last procedure, changing camp state to suspended");
		changeState(i_camp, SmfCampStateExecSuspended::instance());

		// Searching for all procedures which has executing status.
		for (iter = procedures.begin(); iter != procedures.end(); iter++) {
			if((*iter)->getState() == SA_SMF_PROC_EXECUTING) {
				// The procedure did not change its status to ExecSuspended before cluster reboot.
				// It can be done now, because no procedure is running at this point.
				TRACE("SmfCampStateSuspendingExec::execute changing proc state to ExecSuspended");
				(*iter)->changeState(SmfProcStateExecSuspended::instance());
			}
		}
	}
	else {
		TRACE("SmfCampStateSuspendingExec::execute this is the last procedure, changing camp state to executing");
		changeState(i_camp, SmfCampStateExecuting::instance());
	}
	TRACE_LEAVE();
	return SMF_CAMP_DONE;
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
                        if (i_camp->getProcExecutionMode() == SMF_MERGE_TO_SINGLE_STEP) {
                                i_camp->m_noOfProcResponses = 1;
                        } else {
                                i_camp->m_noOfProcResponses = i_camp->getProcedures().size();
                        }
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	changeState(i_camp, SmfCampStateExecuting::instance());
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	changeState(i_camp, SmfCampStateExecuting::instance());
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

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

        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::reverse_iterator iter;

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

        SmfCampResultT procResult;
        if (i_camp->getProcExecutionMode() == SMF_MERGE_TO_SINGLE_STEP) {
                procResult = this->rollbackSingleMergeProc(i_camp);
        } else {
                /* No running procedures, continue with next procedure */
                procResult = rollbackProc(i_camp);
        }
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
        //Activate IMM BPE if active when campaign was started.
	i_camp->restorePbe();

	// Wait for IMM to become writable again by keep on writing until it
	// succeeds. Write the same value, just for synchronization purposes.
	changeState(i_camp, SmfCampRollingBack::instance());

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

        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::reverse_iterator iter;

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
// rollbackSingleMergeProc()
//------------------------------------------------------------------------------
SmfCampResultT 
SmfCampRollingBack::rollbackSingleMergeProc(SmfUpgradeCampaign * i_camp)
{
	TRACE_ENTER();
	LOG_NO("CAMP:: Rollback merged single step procedure only");
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();
        SmfUpgradeProcedure * mergedProc = procedures.at(0);
        if (mergedProc->getState() == SA_SMF_PROC_COMPLETED) {
                SmfProcedureThread *procThread = mergedProc->getProcThread();
                PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                evt->type = PROCEDURE_EVT_ROLLBACK;
                procThread->send(evt);

                TRACE("SmfCampRollingBack::rollbackSingleMergeProc, Wait for procedure result");
		TRACE_LEAVE();
		return SMF_CAMP_DONE;
        }

	LOG_NO("CAMP: The single step merge procedure is rolled back, start rollback of init");
        SmfCampResultT result = this->rollbackInit(i_camp);

        TRACE_LEAVE();
        return result;
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

        changeState(i_camp, SmfCampSuspendingRollback::instance());
        for (iter = procedures.begin(); iter != procedures.end(); ++iter) {
                TRACE("SmfCampRollingBack::Procedure %s, send suspend",
                      (*iter)->getProcName().c_str());
                SmfProcedureThread *procThread = (*iter)->getProcThread();
                PROCEDURE_EVT *evt = new PROCEDURE_EVT();
                evt->type = PROCEDURE_EVT_SUSPEND;
                procThread->send(evt);
	}

        i_camp->m_noOfProcResponses = 0;
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

        SmfCampResultT result;
        if (i_camp->getProcExecutionMode() == SMF_MERGE_TO_SINGLE_STEP) {
                result = this->rollbackSingleMergeProc(i_camp);
        } else {
                /* Find next procedure to be rolled back */
                result = this->rollbackProc(i_camp);
        }
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
        std::vector<SmfUpgradeProcedure*> procedures = i_camp->getProcedures();

	std::vector < SmfUpgradeProcedure * >::iterator iter;

	changeState(i_camp, SmfCampRollingBack::instance());
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

	i_camp->m_campWrapup.rollbackCampWrapup(); // No action if wrapup is faulty

        i_camp->resetMaintenanceState(); // No action if it fails

        //Remove the procedure runtime objects
        for (auto proc: i_camp->getProcedures()) {
            proc->commit();
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
