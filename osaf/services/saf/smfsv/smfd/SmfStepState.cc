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
#include "saSmf.h"
#include "SmfUpgradeStep.hh"
#include "SmfStepState.hh"
#include "SmfUtils.hh"
#include "immutil.h"
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
bool 
SmfStepState::execute(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	LOG_ER("SmfStepState::execute default implementation, should NEVER be executed.");
	TRACE_LEAVE();
	return false;
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
bool 
SmfStepStateInitial::execute(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();

	TRACE("Start executing upgrade step %s", i_step->getDn().c_str());

	/* Check if any bundle requires node reboot, in such case only AU = node is accepted */
	TRACE("SmfStepStateInitial::execute:Check if any software bundle requires reboot");

	SmfImmUtils immutil;
	bool rebootNeeded = false;
	SaImmAttrValuesT_2 ** attributes;

	//Check if any bundle to install needs reboot
	const std::list < SmfBundleRef > &removeList = i_step->getSwRemoveList();
	std::list< SmfBundleRef >::const_iterator bundleIter;
	bundleIter = removeList.begin();
	while (bundleIter != removeList.end()) {
		/* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
		if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
			LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			TRACE_LEAVE();
			return false;
		}
		const SaUint32T* scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
							       "saSmfBundleRemoveOfflineScope",
							       0);

		if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
			TRACE("SmfStepStateInitial::execute:The SW bundle %s requires reboot to remove", 
			      (*bundleIter).getBundleDn().c_str());

			rebootNeeded = true;
			break;
		}
		bundleIter++;
	}

	//If restart was not needed for installation, also check the bundle removal otherwise not needed
	if (rebootNeeded == false) {
		const std::list < SmfBundleRef > &addList = i_step->getSwAddList();
		bundleIter = addList.begin();
		while (bundleIter != addList.end()) {
			/* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
				changeState(i_step, SmfStepStateFailed::instance());
				TRACE_LEAVE();
				return false;
			}
			const SaUint32T* scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
								       "saSmfBundleInstallOfflineScope",
								       0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfStepStateInitial::execute:The SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				rebootNeeded = true;
				break;
			}

			bundleIter++;
		}
	}

	//Find out type of step
	//If single step upgrade, determin the type from first AU or first DU in the step
	//If rolling upgrade check the first AU of the step
	std::string className;
	std::string firstAuDu;
	if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		//Single step
		//Try the activation unit list, if empy try the deactivation unit list
		if (!i_step->getActivationUnitList().empty()) {
			firstAuDu = i_step->getActivationUnitList().front();
		} else if (!i_step->getDeactivationUnitList().empty()) {
			firstAuDu = i_step->getDeactivationUnitList().front();
		} else {
			//No activation/deactivation, just SW installation
			className = "SaAmfNode";
		}

	} else { 
		//Rolling
		if (!i_step->getActivationUnitList().empty()) {
			firstAuDu = i_step->getActivationUnitList().front();
		} else {
			//No activation/deactivation, just SW installation
			className = "SwInstallNode"; //Fake name for SW installation only
		}
	}

	//If a AU/DU was found, check the DN for key words to figure out the class name
	if (!firstAuDu.empty()) {
		if (firstAuDu.find("safComp") == 0) {
			className = "SaAmfComp";
		} else if (firstAuDu.find("safSu") == 0) {
			className = "SaAmfSU";
		} else if (firstAuDu.find("safAmfNode") == 0) {
			className = "SaAmfNode";
		} else {
			LOG_ER("Could not find class for AU/DU DN %s", firstAuDu.c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			TRACE_LEAVE();
			return false;
		}
	}
	if (className == "SwInstallNode") {
		i_step->setStepType(SMF_STEP_SW_INSTALL);
	}
	else if (className == "SaAmfNode") {
		/* AU is AMF node */

		if (rebootNeeded) {
			/* -If rolling upgrade: Check if the step will lock/reboot our own node and if so
			    move our campaign execution to the other controller using e.g. 
			    admin operation SWAP on the SI we belong to. Then the other
			    controller will continue with this step and do the lock/reboot.
			   
			   -If single step  upgrade: No switchover is needed. A cluster reboot will be 
                            ordered within the step */

			if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				if (i_step->isCurrentNode(firstAuDu) == true) {
					i_step->setSwitchOver(true);
					return true; 
				}
			}

			i_step->setStepType(SMF_STEP_NODE_REBOOT);
		} else {
			i_step->setStepType(SMF_STEP_AU_LOCK);
		}

	} else if (className == "SaAmfSU") {
		/* AU is SU */
		if (rebootNeeded) {
			LOG_ER("A software bundle requires reboot but the AU is a SU (%s)", firstAuDu.c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			TRACE_LEAVE();
			return false;
		}

		i_step->setStepType(SMF_STEP_AU_LOCK);
	} else if (className == "SaAmfComp") {
		/* AU is Component */
		if (rebootNeeded) {
			LOG_ER("A software bundle requires reboot but the AU is a Component (%s)", firstAuDu.c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			TRACE_LEAVE();
			return false;
		}

		i_step->setStepType(SMF_STEP_AU_RESTART);
	} else {
		LOG_ER("class name %s for %s unknown as AU", className.c_str(), firstAuDu.c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		TRACE_LEAVE();
		return false;
	}

	changeState(i_step, SmfStepStateExecuting::instance());
	if (i_step->execute() == false) {
		TRACE_LEAVE();
		return false;
	}

	TRACE_LEAVE();
	return true;
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
bool 
SmfStepStateExecuting::execute(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	TRACE("Executing step %s", i_step->getDn().c_str());
	bool rc = true;

	// Two sets of step actions are available
	// The first set is according to the SMF specification SMF A.01.02
	// The second set is an OpeSAF proprietary set.
	// This set is triggerd by the nodeBundleActCmd attribute in the SmfConfig class.
	// If the nodeBundleActCmd attribute is set, the value shall point out a command
	// which shall be executed to to activate the software installed/removed in the 
	// sw bundle installation/removal scripts. The activation is done once for the step.

	if((smfd_cb->nodeBundleActCmd == NULL) || (strcmp(smfd_cb->nodeBundleActCmd,"") == 0)) {
		switch (i_step->getStepType()) {
		TRACE("SmfStepStateExecuting::execute: entering SMF standard  action set");
		case SMF_STEP_SW_INSTALL:
                {
                        if (executeSwInstall(i_step) == false){
				LOG_ER("executeSwInstall failed");
				rc = false;
			}
                        break;
                }
		case SMF_STEP_AU_LOCK:
                {
                        if (executeAuLock(i_step) == false){
				LOG_ER("executeAuLock failed");
				rc = false;
			}
                        break;
                }
		case SMF_STEP_AU_RESTART:
                {
                        if (executeAuRestart(i_step) == false){
				LOG_ER("executeAuRestart failed");
				rc = false;
			}
                        break;
                }
		case SMF_STEP_NODE_REBOOT:
                {
                        if (executeNodeReboot(i_step) == false){
				LOG_ER("executeNodeReboot failed");
				rc = false;
			}
                        break;
                }
		default:
                {
                        LOG_ER("Unknown step type %d", i_step->getStepType());
                        changeState(i_step, SmfStepStateFailed::instance());
			rc = false;
                        break;
                }
		}
	} else {
		TRACE("SmfStepStateExecuting::execute: entering Activation action set");
		rc = false;

		switch (i_step->getStepType()) {
		case SMF_STEP_SW_INSTALL:
		{
			if (executeSwInstallAct(i_step) == false){
				LOG_ER("executeSwInstall failed");
				rc = false;
			}
			break;
		}
		case SMF_STEP_AU_LOCK:
                {
                        if (executeAuLockAct(i_step) == false){
				LOG_ER("executeAuLock failed");
				rc = false;
			}
                        break;
                }
		case SMF_STEP_AU_RESTART:
                {
                        if (executeAuRestartAct(i_step) == false){
				LOG_ER("executeAuRestart failed");
				rc = false;
			}
                        break;
                }
		case SMF_STEP_NODE_REBOOT:
                {
                        if (executeNodeRebootAct(i_step) == false){
				LOG_ER("executeNodeReboot failed");
				rc = false;
			}
                        break;
                }
		default:
                {
                        LOG_ER("Unknown step type %d", i_step->getStepType());
                        changeState(i_step, SmfStepStateFailed::instance());
			rc = false;
                        break;
                }
		}
	}

	TRACE_LEAVE();
	return rc;
}

//------------------------------------------------------------------------------
// executeSwInstall()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeSwInstall(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	LOG_NO("STEP: Executing SW install step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline remove bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline install bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

#if 0
//Moved to the procedure
	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online remove bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }
#endif

	LOG_NO("STEP: Upgrade SW install step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeSwInstallAct()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeSwInstallAct(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	LOG_NO("STEP: Executing SW install activation step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline remove bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline install bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online remove bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",i_step->getSwNode().c_str());
	if (i_step->callActivationCmd(i_step->getSwNode()) == false){
                LOG_ER("Failed to execute SW activate command");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

	LOG_NO("STEP: Upgrade SW install activation step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}


//------------------------------------------------------------------------------
// executeAuLock()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeAuLock(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	LOG_NO("STEP: Executing AU lock step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install new bundles in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (i_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (i_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline remove old bundles in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (i_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

	if (i_step->setMaintenanceState() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline install new software in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (i_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (i_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }
#if 0
//Moved to the procedure
	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online remove bundles in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }
#endif

	LOG_NO("STEP: Upgrade AU lock step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeAuLockAct()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeAuLockAct(SmfUpgradeStep * i_step)
{

	TRACE_ENTER();
	LOG_NO("STEP: Executing AU lock activation step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install new bundles in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (i_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (i_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline remove old bundles in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (i_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

	if (i_step->setMaintenanceState() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to offline install new software in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

	/* Note that the Online uninstallation is made before SW activation and AU instantiate/unlock */
	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online remove bundles in step=%s",i_step->getRdn().c_str());
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",i_step->getSwNode().c_str());
	if (i_step->callActivationCmd(i_step->getSwNode()) == false){
                LOG_ER("Failed to execute SW activate command");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (i_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (i_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",i_step->getRdn().c_str());
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }


	LOG_NO("STEP: Upgrade AU lock activation step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeAuRestart()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeAuRestart(SmfUpgradeStep * i_step)
{
        //The step actions below are executed for steps containing
        //restartable activation units i.e. restartable components.
	TRACE_ENTER();
	LOG_NO("STEP: Executing AU restart step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (i_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}
//TODO: Shall maintenance status be set here for restartable units ??

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (i_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

#if 0
//Moved to the procedure
        /* Online uninstallation of old software */
        LOG_NO("STEP: Online uninstallation of old software");
        if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to online remove bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }
#endif

	LOG_NO("STEP: Upgrade AU restart step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeAuRestartAct()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeAuRestartAct(SmfUpgradeStep * i_step)
{
        //The step actions below are executed for steps containing
        //restartable activation units i.e. restartable components.
	TRACE_ENTER();
	LOG_NO("STEP: Executing AU restart activation step %s", i_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
		LOG_ER("Failed to online install bundles");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create SaAmfNodeSwBundle object");
        if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (i_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model");
		changeState(i_step, SmfStepStateFailed::instance());
		return false;
	}

	/* Note that the Online uninstallation is made before SW activation and AU restart */
        /* Online uninstallation of old software */
        LOG_NO("STEP: Online uninstallation of old software");
        if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to online remove bundles");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Delete SaAmfNodeSwBundle object */
        LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
        if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
                LOG_ER("Failed to delete SaAmfNodeSwBundle object");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",i_step->getSwNode().c_str());
	if (i_step->callActivationCmd(i_step->getSwNode()) == false){
                LOG_ER("Failed to execute SW activate command");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
	}

//TODO: Shall maintenance status be set here for restartable units ??

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (i_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                changeState(i_step, SmfStepStateFailed::instance());
                return false;
        }

	LOG_NO("STEP: Upgrade AU restart activation step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeNodeReboot()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeNodeReboot(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	LOG_NO("STEP: Executing node reboot step %s", i_step->getDn().c_str());

	SmfImmUtils immutil;
	SaImmAttrValuesT_2 ** attributes;
	const SaUint32T* scope;

	std::list< SmfBundleRef >::const_iterator bundleIter;
	std::list < SmfBundleRef > addList;
	bool installationRebootNeeded;
	
	std::list < SmfBundleRef > restartBundles;
        std::list < SmfBundleRef > removeList;
	bool removalRebootNeeded;

	int singleStepRebootInfo = SMF_NO_CLUSTER_REBOOT;
	if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		//Read the reboot info object. If the object does not exist is it assumed that
		//this is the first time the step is executed i.e. the cluster has not yet been rebooted
		SaAisErrorT rc = i_step->getSingleStepRebootInfo(&singleStepRebootInfo);
		if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST)){
			LOG_ER("Failed to get single step reboot info");
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}
	}

	//singleStepRebootInfo is always equal 0 for a rolling type upgrade, i.e. the sequence is unbroken
	//In case of a single step upgrade the singleStepRebootInfo is equal 0 the first time the sequence
	//is entered. Before each possible cluster reboot the singleStepRebootInfo is updated.
	switch(singleStepRebootInfo) { 
	case SMF_NO_CLUSTER_REBOOT:

		/* Online installation of all new software */
		/* All online scripts could be installed here, also for those bundles which requires restart */
		LOG_NO("STEP: Online installation of new software");
		if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to online install bundles");
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Lock deactivation units */
		LOG_NO("STEP: Lock deactivation units");
		if (i_step->lockDeactivationUnits() == false) {
			LOG_ER("Failed to Lock deactivation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Terminate deactivation units */
		LOG_NO("STEP: Terminate deactivation units");
		if (i_step->terminateDeactivationUnits() == false) {
			LOG_ER("Failed to Terminate deactivation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Offline uninstallation of old software */
		LOG_NO("STEP: Offline uninstallation of old software");
		if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to offline remove bundles in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Modify information model and set maintenance status */
		LOG_NO("STEP: Modify information model and set maintenance status");
		if (i_step->modifyInformationModel() != SA_AIS_OK) {
			LOG_ER("Failed to Modify information model in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		if (i_step->setMaintenanceState() == false) {
			LOG_ER("Failed to set maintenance state in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* The action below is an add on to SMF.---------------------------------------*/
		/* See if any of the software bundles installed at online installation has the */
		/* saSmfBundleInstallOfflineScope attribute set to SA_SMF_CMD_SCOPE_PLM_EE     */
		/* If true one or several of the bundles need a reboot to install              */
		/* After the reboot these bundles are considered installed                     */
		/* This is an OpenSAF extension of the SMF specification. This behaviour make  */
		/* it possible to install e.g. new OS which needs a restart to be taken into   */
		/* operation. These kind of software bundles shall install/remove the software */
		/* in the online portion only.                                                 */

		/* Find out if any bundle to install which bundles requires restart to be installed */
		addList = i_step->getSwAddList();
		bundleIter = addList.begin();
		installationRebootNeeded = false;
		while (bundleIter != addList.end()) {
			/* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
				changeState(i_step, SmfStepStateFailed::instance());
				TRACE_LEAVE();
				return false;
			}
			scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
								       "saSmfBundleInstallOfflineScope",
								       0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfStepStateInitial::execute:The SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				installationRebootNeeded = true;
				break;
			}

			bundleIter++;
		}

		if(installationRebootNeeded == true) {
			/* If rolling upgrade, reboot node */
			if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				LOG_NO("STEP: Reboot node %s", i_step->getSwNode().c_str());
				if (i_step->nodeReboot(i_step->getSwNode()) == false){
					LOG_ER("Fails to reboot node %s", i_step->getSwNode().c_str());
					changeState(i_step, SmfStepStateFailed::instance());
					TRACE_LEAVE();
					return false;
				}
				// Here the rebooted node is up and running

			/* If single step, reboot cluster */
			} else if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
				SaAisErrorT rc;
				//Create smfSingleStepInfo object
				if ((rc = i_step->setSingleStepRebootInfo(SMF_INSTALLATION_REBOOT)) != SA_AIS_OK){
					LOG_ER("Creation of SmfSingleStepInfo object fails, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}

				//Save IMM content
				if ((rc = i_step->saveImmContent()) != SA_AIS_OK){
					LOG_ER("Fails to save imm content, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}

				//Reboot cluster
				LOG_NO("STEP: Order cluster reboot");
				if ((rc = i_step->clusterReboot()) != SA_AIS_OK){
					LOG_ER("Fails to save imm content, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}
				
				//Wait for cluster to reboot
				LOG_NO("STEP: Waiting for cluster to reboot");
				sleep(100000);
				//This should never happend !!
				LOG_ER("Cluster refuses to reboot");
				changeState(i_step, SmfStepStateFailed::instance());
				return false;
			}
		}

	case SMF_INSTALLATION_REBOOT:

		/* Find out which bundles requires restart to be removed */
		removeList = i_step->getSwRemoveList();
		bundleIter = removeList.begin();
		removalRebootNeeded = false;
		while (bundleIter != removeList.end()) {
			/* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
				changeState(i_step, SmfStepStateFailed::instance());
				TRACE_LEAVE();
				return false;
			}
			scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
						      "saSmfBundleRemoveOfflineScope",
						      0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfStepStateInitial::execute:The SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				restartBundles.push_back((*bundleIter));
				removalRebootNeeded = true;
			}

			bundleIter++;
		}

		/* Online ununstallation of old software for bundles where */
		/* saSmfBundleRemoveOfflineScope=SA_SMF_CMD_SCOPE_PLM_EE   */
		if( removalRebootNeeded == true ) {
			LOG_NO("STEP: Online uninstallation of old software where reboot is needed to complete the removal");
			if (i_step->onlineRemoveBundlesUserList(i_step->getSwNode(), restartBundles) == false) {
				LOG_ER("Failed to online remove bundles to be rebooted in step=%s",i_step->getRdn().c_str());
				changeState(i_step, SmfStepStateFailed::instance());
				return false;
			}

			/* If rolling upgrade, reboot node */
			if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {
				if (i_step->nodeReboot(i_step->getSwNode()) == false){
					LOG_ER("Fails to reboot node %s", i_step->getSwNode().c_str());
					changeState(i_step, SmfStepStateFailed::instance());
					TRACE_LEAVE();
					return false;
				}
				// Here the rebooted node is up and running

			/* If single step, reboot cluster */
			} else if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
				i_step->setSingleStepRebootInfo(SMF_REMOVAL_REBOOT); //Create smfSingleStepInfo object
				i_step->saveImmContent(); //Save IMM content
				i_step->clusterReboot();  //Reboot cluster
				return true;


				SaAisErrorT rc;
				//Create smfSingleStepInfo object
				if ((rc = i_step->setSingleStepRebootInfo(SMF_REMOVAL_REBOOT)) != SA_AIS_OK){
					LOG_ER("Creation of SmfSingleStepInfo object fails, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}

				//Save IMM content
				if ((rc = i_step->saveImmContent()) != SA_AIS_OK){
					LOG_ER("Fails to save imm content, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}

				//Reboot cluster
				LOG_NO("STEP: Order cluster reboot");
				if ((rc = i_step->clusterReboot()) != SA_AIS_OK){
					LOG_ER("Fails to save imm content, rc=%d", rc);
					changeState(i_step, SmfStepStateFailed::instance());
					return false;
				}
				
				//Wait for cluster to reboot
				LOG_NO("STEP: Waiting for cluster to reboot");
				sleep(100000);
				//This should never happend !!
				LOG_ER("Cluster refuses to reboot");
				changeState(i_step, SmfStepStateFailed::instance());
				return false;

			}
		}

	case SMF_REMOVAL_REBOOT:

		/* --------------------------------------------------------------------------------------*/
		/* End of add on action -----------------------------------------------------------------*/
		/* --------------------------------------------------------------------------------------*/

		/* Offline installation of new software */
		LOG_NO("STEP: Offline installation of new software");
		if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to offline install new software in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Create SaAmfNodeSwBundle object for all bundles to be adde */
		LOG_NO("STEP: Create SaAmfNodeSwBundle object");
		if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to create SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Instantiate activation units */
		LOG_NO("STEP: Instantiate activation units");
		if (i_step->instantiateActivationUnits() == false) {
			LOG_ER("Failed to Instantiate activation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Unlock activation units */
		LOG_NO("STEP: Unlock activation units");
		if (i_step->unlockActivationUnits() == false) {
			LOG_ER("Failed to Unlock activation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		break;

#if 0
//Moved to the procedure

		/* Online uninstallation of old software (no reboot required) */
		LOG_NO("STEP: Online uninstallation of old software");
		if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to online remove bundles in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Delete SaAmfNodeSwBundle object */
		LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
		if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to delete SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}
#endif
	default:
                LOG_ER("Unknown reboot info %d", singleStepRebootInfo);
                assert(0);
                return false;
        }		//End switch

	LOG_NO("STEP: Upgrade node reboot step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
}

//------------------------------------------------------------------------------
// executeNodeRebootAct()
//------------------------------------------------------------------------------
bool 
SmfStepStateExecuting::executeNodeRebootAct(SmfUpgradeStep * i_step)
{
	TRACE_ENTER();
	LOG_NO("STEP: Executing node reboot activate step %s", i_step->getDn().c_str());

	int singleStepRebootInfo = SMF_NO_CLUSTER_REBOOT;
	if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
		//Read the reboot info object. If the object does not exist is it assumed that
		//this is the first time the step is executed i.e. the cluster has not yet been rebooted
		SaAisErrorT rc = i_step->getSingleStepRebootInfo(&singleStepRebootInfo);
		if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST)){
			LOG_ER("Failed to get single step reboot info");
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}
	}

	//singleStepRebootInfo is always equal 0 for a rolling type upgrade, i.e. the sequence is unbroken
	//In case of a single step upgrade the singleStepRebootInfo is equal 0 the first time the sequence
	//is entered. Before each possible cluster reboot the singleStepRebootInfo is updated.
	switch(singleStepRebootInfo) { 
	case SMF_NO_CLUSTER_REBOOT:

		/* Online installation of all new software */
		/* All online scripts could be installed here, also for those bundles which requires restart */
		LOG_NO("STEP: Online installation of new software");
		if (i_step->onlineInstallBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to online install bundles");
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Lock deactivation units */
		LOG_NO("STEP: Lock deactivation units");
		if (i_step->lockDeactivationUnits() == false) {
			LOG_ER("Failed to Lock deactivation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Terminate deactivation units */
		LOG_NO("STEP: Terminate deactivation units");
		if (i_step->terminateDeactivationUnits() == false) {
			LOG_ER("Failed to Terminate deactivation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Offline uninstallation of old software */
		LOG_NO("STEP: Offline uninstallation of old software");
		if (i_step->offlineRemoveBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to offline remove bundles in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Modify information model and set maintenance status */
		LOG_NO("STEP: Modify information model and set maintenance status");
		if (i_step->modifyInformationModel() != SA_AIS_OK) {
			LOG_ER("Failed to Modify information model in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		if (i_step->setMaintenanceState() == false) {
			LOG_ER("Failed to set maintenance state in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Offline installation of new software */
		LOG_NO("STEP: Offline installation of new software");
		if (i_step->offlineInstallBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to offline install new software in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Create SaAmfNodeSwBundle object for all bundles to be adde */
		LOG_NO("STEP: Create SaAmfNodeSwBundle object");
		if (i_step->createSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to create SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Online uninstallation of old software (no reboot required) */
		LOG_NO("STEP: Online uninstallation of old software");
		if (i_step->onlineRemoveBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to online remove bundles in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Delete SaAmfNodeSwBundle object */
		LOG_NO("STEP: Delete SaAmfNodeSwBundle object");
		if (i_step->deleteSaAmfNodeSwBundles(i_step->getSwNode()) == false) {
			LOG_ER("Failed to delete SaAmfNodeSwBundle object in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* The reboot will include an activation of the software */

		/* If rolling upgrade, reboot node */
		if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_ROLLING) {

			/* Reboot node */
			LOG_NO("STEP: Reboot node %s", i_step->getSwNode().c_str());
			if (i_step->nodeReboot(i_step->getSwNode()) == false){
				LOG_ER("Fails to reboot node %s", i_step->getSwNode().c_str());
				changeState(i_step, SmfStepStateFailed::instance());
				TRACE_LEAVE();
				return false;
			}
			// Here the rebooted node is up and running

			/* If single step, reboot cluster */
		} else if (i_step->getProcedure()->getUpgradeMethod()->getUpgradeMethod() == SA_SMF_SINGLE_STEP) {
			SaAisErrorT rc;
			//Create smfSingleStepInfo object
			if ((rc = i_step->setSingleStepRebootInfo(SMF_INSTALLATION_REBOOT)) != SA_AIS_OK) {
				LOG_ER("Creation of SmfSingleStepInfo object fails, rc=%d", rc);
				changeState(i_step, SmfStepStateFailed::instance());
				return false;
			}

			//Save IMM content
			if ((rc = i_step->saveImmContent()) != SA_AIS_OK){
				LOG_ER("Fails to save imm content, rc=%d", rc);
				changeState(i_step, SmfStepStateFailed::instance());
				return false;
			}

			//Reboot cluster
			LOG_NO("STEP: Order cluster reboot");
			if ((rc = i_step->clusterReboot()) != SA_AIS_OK){
				LOG_ER("Fails to save imm content, rc=%d", rc);
				changeState(i_step, SmfStepStateFailed::instance());
				return false;
			}
				
			//Wait for cluster to reboot
			LOG_NO("STEP: Waiting for cluster to reboot");
			sleep(100000);
			//This should never happend !!
			LOG_ER("Cluster refuses to reboot");
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

	case SMF_INSTALLATION_REBOOT:

		/* Instantiate activation units */
		LOG_NO("STEP: Instantiate activation units");
		if (i_step->instantiateActivationUnits() == false) {
			LOG_ER("Failed to Instantiate activation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		/* Unlock activation units */
		LOG_NO("STEP: Unlock activation units");
		if (i_step->unlockActivationUnits() == false) {
			LOG_ER("Failed to Unlock activation units in step=%s",i_step->getRdn().c_str());
			changeState(i_step, SmfStepStateFailed::instance());
			return false;
		}

		break;

	default:
                LOG_ER("Unknown reboot info %d", singleStepRebootInfo);
                assert(0);
                return false;
        }		//End switch

	LOG_NO("STEP: Upgrade node reboot step completed %s", i_step->getDn().c_str());
	changeState(i_step, SmfStepStateCompleted::instance());
	TRACE_LEAVE();

	return true;
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
