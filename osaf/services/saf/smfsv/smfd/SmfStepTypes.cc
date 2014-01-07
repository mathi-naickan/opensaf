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
#include "SmfUpgradeCampaign.hh"
#include "SmfUpgradeMethod.hh"
#include "SmfUpgradeProcedure.hh"
#include "saSmf.h"
#include "SmfUpgradeStep.hh"
#include "SmfStepTypes.hh"
#include "SmfUtils.hh"
#include "immutil.h"

#include <saf_error.h>

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
// ------Base class SmfStepType------------------------------------------------
//
// SmfStepType implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SmfStepType::SmfStepType(SmfUpgradeStep * i_step) : 
  m_step(i_step) 
{

}

SmfStepType::~SmfStepType()
{

}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeSwInstall implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeSwInstall::execute()
{
	TRACE_ENTER();
	LOG_NO("STEP: Executing SW install step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
	        LOG_ER("Failed to online install bundles");
		return false;
	}

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove bundles");
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install bundles");
                return false;
        }

        /* Create SaAmfNodeSwBundle object */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects");
                return false;
        }

        //The following actions are executed in the procedure:
        //-Online uninstallation of old software
        //-Delete SaAmfNodeSwBundle objects

	LOG_NO("STEP: Upgrade SW install step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeSwInstall::rollback()
{
	TRACE_ENTER();
	LOG_NO("STEP: Rolling back SW install step %s", m_step->getDn().c_str());

	/* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
	        LOG_ER("Failed to online install old bundles");
		return false;
	}

	/* Create SaAmfNodeSwBundle objects for old bundles */
	LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
	if (m_step->createSaAmfNodeSwBundlesOld() == false) {
	        LOG_ER("Failed to create SaAmfNodeSwBundle objects");
	        return false;
	}

	/* Offline uninstallation of new software */
	LOG_NO("STEP: Offline uninstallation of new software");
	if (m_step->offlineRemoveNewBundles() == false) {
	        LOG_ER("Failed to offline uninstall new bundles");
		return false;
	}

	/* Offline installation of old software */
	LOG_NO("STEP: Offline installation of old software");
	if (m_step->offlineInstallOldBundles() == false) {
	        LOG_ER("Failed to offline install old bundles");
		return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of new software
        //-Delete new SaAmfNodeSwBundle objects

	LOG_NO("STEP: Rolling back SW install step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeSwInstallAct implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeSwInstallAct::execute()
{
	TRACE_ENTER();
	LOG_NO("STEP: Executing SW install activation step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
		LOG_ER("Failed to online install bundles");
		return false;
	}

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove bundles");
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install bundles");
                return false;
        }

        /* Create new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects");
                return false;
        }

	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (m_step->onlineRemoveOldBundles() == false) {
		LOG_ER("Failed to online remove bundles");
		return false;
	}

        /* Delete old SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete old SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to delete old SaAmfNodeSwBundle objects");
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

	LOG_NO("STEP: Upgrade SW install activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeSwInstallAct::rollback()
{
	TRACE_ENTER();
	LOG_NO("STEP: Rolling back SW install activation step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

        /* Offline uninstallation of new software */
        LOG_NO("STEP: Offline uninstallation of new software");
        if (m_step->offlineRemoveNewBundles() == false) {
                LOG_ER("Failed to offline uninstall new bundles");
                return false;
        }

        /* Offline installation of old software */
        LOG_NO("STEP: Offline installation of old software");
        if (m_step->offlineInstallOldBundles() == false) {
                LOG_ER("Failed to offline install old bundles");
                return false;
	}

	/* Online uninstallation of new software */
	LOG_NO("STEP: Online uninstallation of new software");
	if (m_step->onlineRemoveNewBundles() == false) {
		LOG_ER("Failed to online uninstall new bundles");
		return false;
	}

        /* Delete SaAmfNodeSwBundle objects for new bundles */
        LOG_NO("STEP: Delete new SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to delete new SaAmfNodeSwBundle objects");
                return false;
        }

        /* Activate the SW changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

	LOG_NO("STEP: Rolling back SW install activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeAuLock implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuLock::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Executing AU lock step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
		LOG_ER("Failed to online install new bundles in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if ((m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE)) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (m_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if ((m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE)) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (m_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove old bundles in step=%s",m_step->getRdn().c_str());
                return false;
	}

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (m_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if ((m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE)) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Create new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (m_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if ((m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE)) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (m_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if ((m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE)) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of old software
        //-Delete old SaAmfNodeSwBundle objects

	LOG_NO("STEP: Upgrade AU lock step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuLock::rollback()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Rolling back SW AU lock step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock activation units */
        LOG_NO("STEP: Lock activation units");
        if (m_step->lockActivationUnits() == false) {
                LOG_ER("Failed to Lock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}
        /* Terminate activation units */
        LOG_NO("STEP: Terminate activation units");
        if (m_step->terminateActivationUnits() == false) {
                LOG_ER("Failed to Terminate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of new software */
        LOG_NO("STEP: Offline uninstallation of new software");
        if (m_step->offlineRemoveNewBundles() == false) {
                LOG_ER("Failed to offline uninstall new bundles");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateDeactUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Offline installation of old software */
        LOG_NO("STEP: Offline installation of old software");
        if (m_step->offlineInstallOldBundles() == false) {
                LOG_ER("Failed to offline install old bundles");
                return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

        /* Instantiate deactivation units */
        LOG_NO("STEP: Instantiate deactivation units");
        if (m_step->instantiateDeactivationUnits() == false) {
                LOG_ER("Failed to Instantiate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock deactivation units */
        LOG_NO("STEP: Unlock deactivation units");
        if (m_step->unlockDeactivationUnits() == false) {
                LOG_ER("Failed to Unlock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of new software
        //-Delete new SaAmfNodeSwBundle objects

	LOG_NO("STEP: Rolling back SW AU lock step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeAuLockAct implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuLockAct::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Executing AU lock activation step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
		LOG_ER("Failed to online install new bundles in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (m_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (m_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove old bundles in step=%s",m_step->getRdn().c_str());
                return false;
	}

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (m_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Create new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Note that the Online uninstallation is made before SW activation and AU instantiate/unlock */
	/* Online uninstallation of old software */
	LOG_NO("STEP: Online uninstallation of old software");
	if (m_step->onlineRemoveOldBundles() == false) {
		LOG_ER("Failed to online remove bundles in step=%s",m_step->getRdn().c_str());
		return false;
	}

        /* Delete old SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete old SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to delete old SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (m_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (m_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}


	LOG_NO("STEP: Upgrade AU lock activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuLockAct::rollback()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Rolling back SW AU lock activation step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock activation units */
        LOG_NO("STEP: Lock activation units");
        if (m_step->lockActivationUnits() == false) {
                LOG_ER("Failed to Lock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate activation units */
        LOG_NO("STEP: Terminate activation units");
        if (m_step->terminateActivationUnits() == false) {
                LOG_ER("Failed to Terminate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of new software */
        LOG_NO("STEP: Offline uninstallation of new software");
        if (m_step->offlineRemoveNewBundles() == false) {
                LOG_ER("Failed to offline uninstall new bundles");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateDeactUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Offline installation of old software */
        LOG_NO("STEP: Offline installation of old software");
        if (m_step->offlineInstallOldBundles() == false) {
                LOG_ER("Failed to offline install old bundles");
                return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

	/* Online uninstallation of new software */
	LOG_NO("STEP: Online uninstallation of new software");
	if (m_step->onlineRemoveNewBundles() == false) {
		LOG_ER("Failed to online uninstall new bundles");
		return false;
	}

        /* Delete new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete new SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to delete new SaAmfNodeSwBundle objects");
                return false;
        }

        /* Activate the SW changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

        /* Instantiate deactivation units */
        LOG_NO("STEP: Instantiate deactivation units");
        if (m_step->instantiateDeactivationUnits() == false) {
                LOG_ER("Failed to Instantiate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock deactivation units */
        LOG_NO("STEP: Unlock deactivation units");
        if (m_step->unlockDeactivationUnits() == false) {
                LOG_ER("Failed to Unlock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

	LOG_NO("STEP: Rolling back SW AU lock activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeAuRestart implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuRestart::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Executing AU restart step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
		LOG_ER("Failed to online install bundles");
		return false;
	}

        /* Create new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects");
                return false;
        }

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (m_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Set maintenance status */
	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (m_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of old software
        //-Delete old SaAmfNodeSwBundle objects

	LOG_NO("STEP: Upgrade AU restart step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuRestart::rollback()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Rolling back AU restart step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Set maintenance status */
	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (m_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of new software
        //-Delete new SaAmfNodeSwBundle objects

	LOG_NO("STEP: Rolling back AU restart step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeAuRestartAct implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuRestartAct::execute()
{
        //The step actions below are executed for steps containing
        //restartable activation units i.e. restartable components.
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();

	LOG_NO("STEP: Executing AU restart activation step %s", m_step->getDn().c_str());

	/* Online installation of new software */
	LOG_NO("STEP: Online installation of new software");
	if (m_step->onlineInstallNewBundles() == false) {
		LOG_ER("Failed to online install bundles");
		return false;
	}

        /* Create new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects");
                return false;
        }

	/* Modify information model and set maintenance status */
	LOG_NO("STEP: Modify information model and set maintenance status");
	if (m_step->modifyInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Modify information model");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	/* Note that the Online uninstallation is made before SW activation and AU restart */
        /* Online uninstallation of old software */
        LOG_NO("STEP: Online uninstallation of old software");
        if (m_step->onlineRemoveOldBundles() == false) {
                LOG_ER("Failed to online remove bundles");
                return false;
        }

        /* Delete old SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete old SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to delete old SaAmfNodeSwBundle objects");
                return false;
        }

        /* Activate the changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

        /* Set maintenance status */
	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (m_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

	LOG_NO("STEP: Upgrade AU restart activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeAuRestartAct::rollback()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();

	LOG_NO("STEP: Rolling back AU restart activation step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	/* Online uninstallation of new software */
	LOG_NO("STEP: Online uninstallation of new software");
	if (m_step->onlineRemoveNewBundles() == false) {
		LOG_ER("Failed to online uninstall new bundles");
		return false;
	}

        /* Delete new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete new SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to delete new SaAmfNodeSwBundle objects");
                return false;
        }

        /* Activate the SW changes on the node */
        LOG_NO("STEP: Activate installed software on node %s",m_step->getSwNode().c_str());
	if (m_step->callActivationCmd() == false){
                LOG_ER("Failed to execute SW activate command");
                return false;
	}

        /* Set maintenance status */
	if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Restartable activation units, restart them */
        LOG_NO("STEP: Restart activation units");
        if (m_step->restartActivationUnits() == false) {
                LOG_ER("Failed to Restart activation units");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

	LOG_NO("STEP: Rolling back AU restart activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeNodeReboot implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeNodeReboot::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();

	LOG_NO("STEP: Executing node reboot step %s", m_step->getDn().c_str());

	SmfImmUtils immutil;
	SaImmAttrValuesT_2 ** attributes;
	const SaUint32T* scope;

	std::list< SmfBundleRef >::const_iterator bundleIter;
	const std::list < SmfBundleRef >& addList = m_step->getSwAddList();
	bool installationRebootNeeded;
	
	std::list < SmfBundleRef > restartBundles;
        const std::list < SmfBundleRef >& removeList = m_step->getSwRemoveList();;
	bool removalRebootNeeded;

        /* Online installation of all new software */
        /* All online scripts could be installed here, also for those bundles which requires restart */
        LOG_NO("STEP: Online installation of new software");
        if (m_step->onlineInstallNewBundles() == false) {
                LOG_ER("Failed to online install bundles");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (m_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (m_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove bundles in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Modify information model and set maintenance status */
        LOG_NO("STEP: Modify information model and set maintenance status");
        if (m_step->modifyInformationModel() != SA_AIS_OK) {
                LOG_ER("Failed to Modify information model in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

        if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
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
        bundleIter = addList.begin();
        installationRebootNeeded = false;
        while (bundleIter != addList.end()) {
                /* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
                if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
                        LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
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
                LOG_NO("STEP: Reboot node for installation %s", m_step->getSwNode().c_str());
                if (m_step->nodeReboot() == false){
                        LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                        TRACE_LEAVE();
                        return false;
                }
        }
        // Here the rebooted node is up and running

        /* Find out which bundles requires restart to be removed */
        bundleIter = removeList.begin();
        removalRebootNeeded = false;
        while (bundleIter != removeList.end()) {
                /* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
                if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
                        LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
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

        /* Online uninstallation of old software for bundles where */
        /* saSmfBundleRemoveOfflineScope=SA_SMF_CMD_SCOPE_PLM_EE   */
        if( removalRebootNeeded == true ) {
                LOG_NO("STEP: Online uninstallation of old software where reboot is needed to complete the removal");
                if (m_step->onlineRemoveBundlesUserList(m_step->getSwNode(), restartBundles) == false) {
                        LOG_ER("Failed to online remove bundles to be rebooted in step=%s",m_step->getRdn().c_str());
                        return false;
                }

                LOG_NO("STEP: Reboot node for removal %s", m_step->getSwNode().c_str());
                if (m_step->nodeReboot() == false){
                        LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                        TRACE_LEAVE();
                        return false;
                }
        }
        // Here the rebooted node is up and running

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Create new SaAmfNodeSwBundle object for all bundles to be added */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (m_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (m_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of old software (no reboot required)
        //-Delete old SaAmfNodeSwBundle objects

	LOG_NO("STEP: Upgrade node reboot step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeNodeReboot::rollback()
{
	std::list < SmfCallback * > cbkList;
        TRACE_ENTER();

        SmfImmUtils immutil;
	SaImmAttrValuesT_2 ** attributes;
	const SaUint32T* scope;

	std::list< SmfBundleRef >::const_iterator bundleIter;
	const std::list < SmfBundleRef >& addList = m_step->getSwAddList();

	bool installationRebootNeeded;
	
	std::list < SmfBundleRef > restartBundles;
        const std::list < SmfBundleRef >& removeList = m_step->getSwRemoveList();

	bool removalRebootNeeded;

        LOG_NO("STEP: Rolling back node reboot step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK);
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock activation units */
        LOG_NO("STEP: Lock activation units");
        if (m_step->lockActivationUnits() == false) {
                LOG_ER("Failed to Lock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate activation units */
        LOG_NO("STEP: Terminate activation units");
        if (m_step->terminateActivationUnits() == false) {
                LOG_ER("Failed to Terminate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of new software */
        LOG_NO("STEP: Offline uninstallation of new software");
        if (m_step->offlineRemoveNewBundles() == false) {
                LOG_ER("Failed to offline uninstall new bundles");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateDeactUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Find out if any old bundle we have reinstalled requires restart to be installed */
        bundleIter = removeList.begin();
        installationRebootNeeded = false;
        while (bundleIter != removeList.end()) {
                /* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
                if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
                        LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
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
                LOG_NO("STEP: Reboot node for installation %s", m_step->getSwNode().c_str());
                if (m_step->nodeReboot() == false){
                        LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                        TRACE_LEAVE();
                        return false;
                }
        }
        // Here the rebooted node is up and running

        /* Find out which of the new bundles we have removed requires restart to be removed */
        bundleIter = addList.begin();
        removalRebootNeeded = false;
        while (bundleIter != addList.end()) {
                /* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
                if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
                        LOG_ER("Could not find software bundle  %s", (*bundleIter).getBundleDn().c_str());
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

        /* Online uninstallation of new software for bundles where */
        /* saSmfBundleRemoveOfflineScope=SA_SMF_CMD_SCOPE_PLM_EE   */
        if( removalRebootNeeded == true ) {
                LOG_NO("STEP: Online uninstallation of new software where reboot is needed to complete the removal");
                if (m_step->onlineRemoveBundlesUserList(m_step->getSwNode(), restartBundles) == false) {
                        LOG_ER("Failed to online remove bundles to be rebooted in step=%s",m_step->getRdn().c_str());
                        return false;
                }

                LOG_NO("STEP: Reboot node for removal %s", m_step->getSwNode().c_str());
                if (m_step->nodeReboot() == false){
                        LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                        TRACE_LEAVE();
                        return false;
                }
        }
        // Here the rebooted node is up and running

        /* Offline installation of old software */
        LOG_NO("STEP: Offline installation of old software");
        if (m_step->offlineInstallOldBundles() == false) {
                LOG_ER("Failed to offline install old bundles");
                return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

        /* Instantiate deactivation units */
        LOG_NO("STEP: Instantiate deactivation units");
        if (m_step->instantiateDeactivationUnits() == false) {
                LOG_ER("Failed to Instantiate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock deactivation units */
        LOG_NO("STEP: Unlock deactivation units");
        if (m_step->unlockDeactivationUnits() == false) {
                LOG_ER("Failed to Unlock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        //The following actions are executed in the procedure:
        //-Online uninstallation of new software
        //-Delete new SaAmfNodeSwBundle objects

	LOG_NO("STEP: Rolling back node reboot step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeNodeRebootAct implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeNodeRebootAct::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Executing node reboot activate step %s", m_step->getDn().c_str());

        /* Online installation of all new software */
        /* All online scripts could be installed here, also for those bundles which requires restart */
        LOG_NO("STEP: Online installation of new software");
        if (m_step->onlineInstallNewBundles() == false) {
                LOG_ER("Failed to online install bundles");
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock deactivation units */
        LOG_NO("STEP: Lock deactivation units");
        if (m_step->lockDeactivationUnits() == false) {
                LOG_ER("Failed to Lock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate deactivation units */
        LOG_NO("STEP: Terminate deactivation units");
        if (m_step->terminateDeactivationUnits() == false) {
                LOG_ER("Failed to Terminate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of old software */
        LOG_NO("STEP: Offline uninstallation of old software");
        if (m_step->offlineRemoveOldBundles() == false) {
                LOG_ER("Failed to offline remove bundles in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Modify information model and set maintenance status */
        LOG_NO("STEP: Modify information model and set maintenance status");
        if (m_step->modifyInformationModel() != SA_AIS_OK) {
                LOG_ER("Failed to Modify information model in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

        if (m_step->setMaintenanceStateActUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline installation of new software */
        LOG_NO("STEP: Offline installation of new software");
        if (m_step->offlineInstallNewBundles() == false) {
                LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Create new SaAmfNodeSwBundle objects for all bundles to be added */
        LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Online uninstallation of old software (no reboot required) */
        LOG_NO("STEP: Online uninstallation of old software");
        if (m_step->onlineRemoveOldBundles() == false) {
                LOG_ER("Failed to online remove bundles in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Delete old SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete old SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to delete old SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* The reboot will include an activation of the software */

        /* Reboot node */
        LOG_NO("STEP: Reboot node %s", m_step->getSwNode().c_str());
        if (m_step->nodeReboot() == false){
                LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                return false;
        }
        // Here the rebooted node is up and running

        /* Instantiate activation units */
        LOG_NO("STEP: Instantiate activation units");
        if (m_step->instantiateActivationUnits() == false) {
                LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock activation units */
        LOG_NO("STEP: Unlock activation units");
        if (m_step->unlockActivationUnits() == false) {
                LOG_ER("Failed to Unlock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

	LOG_NO("STEP: Upgrade node reboot activate step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeNodeRebootAct::rollback()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();

	LOG_NO("STEP: Rolling back node reboot activation step %s", m_step->getDn().c_str());

        /* Online installation of old software */
	LOG_NO("STEP: Online installation of old software");
	if (m_step->onlineInstallOldBundles() == false) {
		LOG_ER("Failed to online install old bundles");
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeLock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Lock activation units */
        LOG_NO("STEP: Lock activation units");
        if (m_step->lockActivationUnits() == false) {
                LOG_ER("Failed to Lock activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksBeforeTerm();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Terminate activation units */
        LOG_NO("STEP: Terminate activation units");
        if (m_step->terminateActivationUnits() == false) {
                LOG_ER("Failed to Terminate activation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

        /* Offline uninstallation of new software */
        LOG_NO("STEP: Offline uninstallation of new software");
        if (m_step->offlineRemoveNewBundles() == false) {
                LOG_ER("Failed to offline uninstall new bundles");
                return false;
        }

	/* Reverse information model and set maintenance status */
	LOG_NO("STEP: Reverse information model and set maintenance status for deactivation units");
	if (m_step->reverseInformationModel() != SA_AIS_OK) {
		LOG_ER("Failed to Reverse information model in step=%s",m_step->getRdn().c_str());
		return false;
	}

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterImmModify();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
                return false;
	}

	if (m_step->setMaintenanceStateDeactUnits() == false) {
                LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Offline installation of old software */
        LOG_NO("STEP: Offline installation of old software");
        if (m_step->offlineInstallOldBundles() == false) {
                LOG_ER("Failed to offline install old bundles");
                return false;
	}

        /* Create SaAmfNodeSwBundle objects for old bundles */
        LOG_NO("STEP: Create old SaAmfNodeSwBundle objects");
        if (m_step->createSaAmfNodeSwBundlesOld() == false) {
                LOG_ER("Failed to create SaAmfNodeSwBundle objects");
                return false;
        }

	/* Online uninstallation of new software */
	LOG_NO("STEP: Online uninstallation of new software");
	if (m_step->onlineRemoveNewBundles() == false) {
		LOG_ER("Failed to online uninstall new bundles");
		return false;
	}

        /* Delete new SaAmfNodeSwBundle objects */
        LOG_NO("STEP: Delete new SaAmfNodeSwBundle objects");
        if (m_step->deleteSaAmfNodeSwBundlesNew() == false) {
                LOG_ER("Failed to delete new SaAmfNodeSwBundle objects");
                return false;
        }

        /* The reboot will include an activation of the software */

        /* Reboot node */
        LOG_NO("STEP: Reboot node %s", m_step->getSwNode().c_str());
        if (m_step->nodeReboot() == false){
                LOG_ER("Fails to reboot node %s", m_step->getSwNode().c_str());
                return false;
        }
        // Here the rebooted node is up and running

        /* Instantiate deactivation units */
        LOG_NO("STEP: Instantiate deactivation units");
        if (m_step->instantiateDeactivationUnits() == false) {
                LOG_ER("Failed to Instantiate deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
                return false;
	}

        /* Unlock deactivation units */
        LOG_NO("STEP: Unlock deactivation units");
        if (m_step->unlockDeactivationUnits() == false) {
                LOG_ER("Failed to Unlock deactivation units in step=%s",m_step->getRdn().c_str());
                return false;
        }

	/* Check if callback is required to be invoked.*/
	cbkList = m_step->getProcedure()->getCbksAfterUnlock();
	if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_ROLLBACK) == false) {
                LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
                return false;
	}

	LOG_NO("STEP: Rolling back node reboot activation step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeClusterReboot implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeClusterReboot::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();

	LOG_NO("STEP: Executing cluster reboot step %s", m_step->getDn().c_str());

	SmfImmUtils immutil;
	SaImmAttrValuesT_2 ** attributes;
	const SaUint32T* scope;

	std::list< SmfBundleRef >::const_iterator bundleIter;
	const std::list < SmfBundleRef >& addList = m_step->getSwAddList();

	bool installationRebootNeeded;
	
	std::list < SmfBundleRef > restartBundles;
        const std::list < SmfBundleRef >& removeList = m_step->getSwRemoveList() ;

	bool removalRebootNeeded;

	int singleStepRebootInfo = SMF_NO_CLUSTER_REBOOT;
        //Read the reboot info object. If the object does not exist is it assumed that
        //this is the first time the step is executed i.e. the cluster has not yet been rebooted
        SaAisErrorT rc = m_step->getSingleStepRebootInfo(&singleStepRebootInfo);
        if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST)){
                LOG_ER("SmfStepTypeClusterReboot::execute, fail to get single step reboot info");
                return false;
	}

	//In case of a single step upgrade the singleStepRebootInfo is equal 0 the first time the sequence
	//is entered. Before each possible cluster reboot the singleStepRebootInfo is updated.
	switch(singleStepRebootInfo) { 
	case SMF_NO_CLUSTER_REBOOT:

		/* Online installation of all new software */
		/* All online scripts could be installed here, also for those bundles which requires restart */
		LOG_NO("STEP: Online installation of new software");
		if (m_step->onlineInstallNewBundles() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to online install bundles");
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksBeforeLock();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Lock deactivation units */
		LOG_NO("STEP: Lock deactivation units");
		if (m_step->lockDeactivationUnits() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to Lock deact units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksBeforeTerm();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Terminate deactivation units */
		LOG_NO("STEP: Terminate deactivation units");
		if (m_step->terminateDeactivationUnits() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to term deact units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Offline uninstallation of old software */
		LOG_NO("STEP: Offline uninstallation of old software");
		if (m_step->offlineRemoveOldBundles() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to offline remove bundles in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Modify information model and set maintenance status */
		LOG_NO("STEP: Modify information model and set maintenance status");
		if (m_step->modifyInformationModel() != SA_AIS_OK) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to Modify information model in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterImmModify();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
			return false;
		}

		if (m_step->setMaintenanceStateActUnits() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to set maintenance state in step=%s",m_step->getRdn().c_str());
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
		bundleIter = addList.begin();
		installationRebootNeeded = false;
		while (bundleIter != addList.end()) {
			/* Read the saSmfBundleInstallOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_ER("SmfStepTypeClusterReboot::execute, sw bundle not found dn=[%s]", (*bundleIter).getBundleDn().c_str());
				TRACE_LEAVE();
				return false;
			}
			scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
								       "saSmfBundleInstallOfflineScope",
								       0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfStepTypeClusterReboot::execute, SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				installationRebootNeeded = true;
				break;
			}

			bundleIter++;
		}

		if(installationRebootNeeded == true) {
                        SaAisErrorT rc;
                        //Create smfSingleStepInfo object
                        if ((rc = m_step->setSingleStepRebootInfo(SMF_INSTALLATION_REBOOT)) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, creation of SmfSingleStepInfo object fails, rc=%s", saf_error(rc));
                                return false;
                        }

			//Create SMF restart indicator. This is used to decide if the campaign restart was init by SMF.
			SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->createSmfRestartIndicator();

                        //Save IMM content
                        if ((rc = m_step->saveImmContent()) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, fail to save imm content, rc=%s", saf_error(rc));
                                return false;
                        }

                        //Reboot cluster
                        LOG_NO("STEP: Order cluster reboot");
                        if ((rc = m_step->clusterReboot()) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, fail to reboot cluster, rc=%s", saf_error(rc));
                                return false;
                        }
				
                        //Wait for cluster to reboot
                        LOG_NO("STEP: Waiting for cluster to reboot");
                        sleep(100000);
                        //This should never happend !!
                        LOG_ER("Cluster refuses to reboot");
                        return false;
		}
                /* Fall through */

	case SMF_INSTALLATION_REBOOT:

		/* Find out which bundles requires restart to be removed */
		bundleIter = removeList.begin();
		removalRebootNeeded = false;
		while (bundleIter != removeList.end()) {
			/* Read the saSmfBundleRemoveOfflineScope to detect if the bundle requires reboot */
			if (immutil.getObject((*bundleIter).getBundleDn(), &attributes) == false) {
				LOG_ER("SmfStepTypeClusterReboot::execute, sw bundle not found dn=[%s]", (*bundleIter).getBundleDn().c_str());
				TRACE_LEAVE();
				return false;
			}
			scope = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, 
						      "saSmfBundleRemoveOfflineScope",
						      0);

			if ((scope != NULL) && (*scope == SA_SMF_CMD_SCOPE_PLM_EE)) {
				TRACE("SmfStepTypeClusterReboot::execute, the SW bundle %s requires reboot to install", 
				      (*bundleIter).getBundleDn().c_str());

				restartBundles.push_back((*bundleIter));
				removalRebootNeeded = true;
			}

			bundleIter++;
		}

		/* Online uninstallation of old software for bundles where */
		/* saSmfBundleRemoveOfflineScope=SA_SMF_CMD_SCOPE_PLM_EE   */
		if( removalRebootNeeded == true ) {
			LOG_NO("STEP: Online uninstallation of old software where reboot is needed to complete the removal");
			if (m_step->onlineRemoveBundlesUserList(m_step->getSwNode(), restartBundles) == false) {
				LOG_ER("Failed to online remove bundles to be rebooted in step=%s",m_step->getRdn().c_str());
				return false;
			}

                        SaAisErrorT rc;
                        //Create smfSingleStepInfo object
                        if ((rc = m_step->setSingleStepRebootInfo(SMF_REMOVAL_REBOOT)) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, creation of SmfSingleStepInfo obj fail, rc=%s", saf_error(rc));
                                return false;
                        }

			//Create SMF restart indicator. This is used to decide if the campaign restart was init by SMF.
			SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->createSmfRestartIndicator();

                        //Save IMM content
                        if ((rc = m_step->saveImmContent()) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, fails to save imm content, rc=%s", saf_error(rc));
                                return false;
                        }

                        //Reboot cluster
                        LOG_NO("STEP: Order cluster reboot");
                        if ((rc = m_step->clusterReboot()) != SA_AIS_OK){
                                LOG_ER("SmfStepTypeClusterReboot::execute, fail to reboot cluster, rc=%s", saf_error(rc));
                                return false;
                        }
				
                        //Wait for cluster to reboot
                        LOG_NO("STEP: Waiting for cluster to reboot");
                        sleep(100000);
                        //This should never happend !!
                        LOG_ER("SmfStepTypeClusterReboot::execute, cluster refuses to reboot");
                        return false;
		}
                /* Fall through */

	case SMF_REMOVAL_REBOOT:

		/* --------------------------------------------------------------------------------------*/
		/* End of add on action -----------------------------------------------------------------*/
		/* --------------------------------------------------------------------------------------*/

		/* Offline installation of new software */
		LOG_NO("STEP: Offline installation of new software");
		if (m_step->offlineInstallNewBundles() == false) {
			LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Create new SaAmfNodeSwBundle object for all bundles to be added */
		LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
		if (m_step->createSaAmfNodeSwBundlesNew() == false) {
			LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Instantiate activation units */
		LOG_NO("STEP: Instantiate activation units");
		if (m_step->instantiateActivationUnits() == false) {
			LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Unlock activation units */
		LOG_NO("STEP: Unlock activation units");
		if (m_step->unlockActivationUnits() == false) {
			LOG_ER("SmfStepTypeClusterReboot::execute, fail to Unlock activation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterUnlock();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
			return false;
		}

		break;

                //The following actions are executed in the procedure:
                //-Online uninstallation of old software (no reboot required)
                //-Delete old SaAmfNodeSwBundle objects

	default:
                LOG_ER("SmfStepTypeClusterReboot::execute, unknown reboot info %d", singleStepRebootInfo);
                return false;
        }		//End switch

	//Mark the campaign not possible to rollback (since it contain a cluster reboot procedure)
	if (SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->
	    disableCampRollback("Single step procedure reboot the cluster, campaign can not be rolled back") != SA_AIS_OK) {
		LOG_NO("Fail to disable rollback, continue.");
	}

	LOG_NO("STEP: Upgrade cluster reboot step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeClusterReboot::rollback()
{
	TRACE_ENTER();

	LOG_ER("SmfStepTypeClusterReboot::rollback, rollback of cluster reboot step is not implemented");

        TRACE_LEAVE();
	return false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SmfStepTypeClusterRebootAct implementations
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
bool 
SmfStepTypeClusterRebootAct::execute()
{
	std::list < SmfCallback * > cbkList;
	TRACE_ENTER();
	LOG_NO("STEP: Executing cluster reboot activate step %s", m_step->getDn().c_str());

	int singleStepRebootInfo = SMF_NO_CLUSTER_REBOOT;
        SaAisErrorT rc = m_step->getSingleStepRebootInfo(&singleStepRebootInfo);
        if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST)){
                LOG_ER("Failed to get single step reboot info");
                return false;
	}

	//In case of a single step upgrade the singleStepRebootInfo is equal 0 the first time the sequence
	//is entered. Before each possible cluster reboot the singleStepRebootInfo is updated.
	switch(singleStepRebootInfo) { 
	case SMF_NO_CLUSTER_REBOOT:

		/* Online installation of all new software */
		/* All online scripts could be installed here, also for those bundles which requires restart */
		LOG_NO("STEP: Online installation of new software");
		if (m_step->onlineInstallNewBundles() == false) {
			LOG_ER("Failed to online install bundles");
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksBeforeLock();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeLock, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Lock deactivation units */
		LOG_NO("STEP: Lock deactivation units");
		if (m_step->lockDeactivationUnits() == false) {
			LOG_ER("Failed to Lock deactivation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksBeforeTerm();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksBeforeTerm, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Terminate deactivation units */
		LOG_NO("STEP: Terminate deactivation units");
		if (m_step->terminateDeactivationUnits() == false) {
			LOG_ER("Failed to Terminate deactivation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Offline uninstallation of old software */
		LOG_NO("STEP: Offline uninstallation of old software");
		if (m_step->offlineRemoveOldBundles() == false) {
			LOG_ER("Failed to offline remove bundles in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Modify information model and set maintenance status */
		LOG_NO("STEP: Modify information model and set maintenance status");
		if (m_step->modifyInformationModel() != SA_AIS_OK) {
			LOG_ER("Failed to Modify information model in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterImmModify();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterImmModify, step=%s",m_step->getRdn().c_str());
			return false;
		}

		if (m_step->setMaintenanceStateActUnits() == false) {
			LOG_ER("Failed to set maintenance state in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Offline installation of new software */
		LOG_NO("STEP: Offline installation of new software");
		if (m_step->offlineInstallNewBundles() == false) {
			LOG_ER("Failed to offline install new software in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Create new SaAmfNodeSwBundle objects for all bundles to be added */
		LOG_NO("STEP: Create new SaAmfNodeSwBundle objects");
		if (m_step->createSaAmfNodeSwBundlesNew() == false) {
			LOG_ER("Failed to create new SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Online uninstallation of old software (no reboot required) */
		LOG_NO("STEP: Online uninstallation of old software");
		if (m_step->onlineRemoveOldBundles() == false) {
			LOG_ER("Failed to online remove bundles in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Delete old SaAmfNodeSwBundle objects */
		LOG_NO("STEP: Delete old SaAmfNodeSwBundle objects");
		if (m_step->deleteSaAmfNodeSwBundlesOld() == false) {
			LOG_ER("Failed to delete old SaAmfNodeSwBundle objects in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* The reboot will include an activation of the software */

                SaAisErrorT rc;
                //Create smfSingleStepInfo object
                if ((rc = m_step->setSingleStepRebootInfo(SMF_INSTALLATION_REBOOT)) != SA_AIS_OK) {
                        LOG_ER("Creation of SmfSingleStepInfo object fails, rc=%d", rc);
                        return false;
                }

		//Create SMF restart indicator. This is used to decide if the campaign restart was init by SMF.
		SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->createSmfRestartIndicator();

                //Save IMM content
                if ((rc = m_step->saveImmContent()) != SA_AIS_OK){
                        LOG_ER("Fails to save imm content, rc=%d", rc);
                        return false;
                }

                //Reboot cluster
                LOG_NO("STEP: Order cluster reboot");
                if ((rc = m_step->clusterReboot()) != SA_AIS_OK){
                        LOG_ER("Failed to reboot cluster, rc=%d", rc);
                        return false;
                }
				
                //Wait for cluster to reboot
                LOG_NO("STEP: Waiting for cluster to reboot");
                sleep(100000);
                //This should never happend !!
                LOG_ER("Cluster refuses to reboot");
                return false;

	case SMF_INSTALLATION_REBOOT:

		/* Instantiate activation units */
		LOG_NO("STEP: Instantiate activation units");
		if (m_step->instantiateActivationUnits() == false) {
			LOG_ER("Failed to Instantiate activation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterInstantiate();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterInstantiate, step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Unlock activation units */
		LOG_NO("STEP: Unlock activation units");
		if (m_step->unlockActivationUnits() == false) {
			LOG_ER("Failed to Unlock activation units in step=%s",m_step->getRdn().c_str());
			return false;
		}

		/* Check if callback is required to be invoked.*/
		cbkList = m_step->getProcedure()->getCbksAfterUnlock();
		if (m_step->checkAndInvokeCallback(cbkList, SA_SMF_UPGRADE) == false) {
			LOG_ER("checkAndInvokeCallback returned false for list cbksAfterUnlock, step=%s",m_step->getRdn().c_str());
			return false;
		}

		break;

	default:
                LOG_ER("Unknown reboot info %d", singleStepRebootInfo);
                return false;
        }		//End switch

	//Mark the campaign not possible to rollback (since it contain a cluster reboot procedure)
	if (SmfCampaignThread::instance()->campaign()->getUpgradeCampaign()->
	    disableCampRollback("Single step procedure reboot the cluster, campaign can not be rolled back") != SA_AIS_OK) {
		LOG_NO("Fail to disable rollback, continue.");
	}
	
	LOG_NO("STEP: Upgrade cluster reboot activate step completed %s", m_step->getDn().c_str());

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
bool 
SmfStepTypeClusterRebootAct::rollback()
{
	TRACE_ENTER();

	LOG_ER("Rollback of cluster reboot activate step is not implemented");

	TRACE_LEAVE();
	return false;
}
