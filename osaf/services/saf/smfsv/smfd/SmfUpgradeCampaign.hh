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

#ifndef __SMFUPGRADECAMPAIGN_H
#define __SMFUPGRADECAMPAIGN_H

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <string>
#include <vector>
#include <saSmf.h>
#include "SmfCampaignInit.hh"
#include "SmfCampaignWrapup.hh"
#include "SmfCampaignThread.hh"
#include "SmfExecControlHdl.h"

class SmfUpgradeProcedure;
class SmfCampState;
class SmfUpgradeAction;

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* Campaign result enums */
typedef enum {
	SMF_CAMP_DONE = 1,
	SMF_CAMP_CONTINUE = 2,
	SMF_CAMP_FAILED = 3,
	SMF_CAMP_COMPLETED = 4,
	SMF_CAMP_SUSPENDING = 5,
	SMF_CAMP_SUSPENDED = 6,
	SMF_CAMP_COMMITED = 6,
	SMF_CAMP_MAX
} SmfCampResultT;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

///
/// Purpose: Contains all campaign data extracted from the campaign xml file.
///
class SmfUpgradeCampaign {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfUpgradeCampaign();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfUpgradeCampaign();

///
/// Purpose: Get the class name.
/// @param   io_str A reference to a std::string for the method to fill in.
/// @return  None.
///
	void getClassName(std::string & io_str) const;

///
/// Purpose: Get a string representation of the upgrade campaign.
/// @param   io_str A reference to a std::string for the method to fill in.
/// @return  None.
///
	void toString(std::string & io_str) const;

///
/// Purpose: Set the name of the upgrade campaign package.
/// @param   A std::string containing the package name.
/// @return  None.
///
	void setCampPackageName(const std::string & i_name);

///
/// Purpose: Get the name of the upgrade campaign package.
/// @param   None.
/// @return  A std::string containing the package name.
///
	const std::string & getCampPackageName(void);

///
/// Purpose: Set the campaign name.
/// @param   i_name A reference to a std::string containing the package name.
/// @return  None.
///
	void setCampaignName(const std::string & i_name);

///
/// Purpose: Get the campaign name.
/// @param   None.
/// @return  A std::string containing the campaign name.
///
	const std::string & getCampaignName(void);

///
/// Purpose: Set the xsi.
/// @param   i_name A reference to a std::string containing the xsi name.
/// @return  None.
///
	void setXsi(const std::string & i_name);

///
/// Purpose: Set the name space schema location.
/// @param   i_name A reference to a std::string containing the name space.
/// @return  None.
///
	void setNameSpaceSchemaLocation(const std::string & i_name);

///
/// Purpose: Set the campaign period.
/// @param   i_period A reference to a std::string containing the campaign period (in nano seconds).
/// @return  None.
///
	void setCampaignPeriod(const std::string & i_period);

///
/// Purpose: Get the campaign period.
/// @param   None
/// @return  A reference to a std::string containing the campaign period (in nano seconds).
///
	const std::string& getCampaignPeriod() { return m_campaignPeriod; }

///
/// Purpose: Set the configuration base.
/// @param   i_confbase A reference to a std::string containing the configuration base.
/// @return  None.
///
	void setConfigurationBase(const std::string & i_confbase);

///
/// Purpose: Set current campaign state.
/// @param   i_state A SaSmfCmpgStateT object.
/// @return  None.
///
	void setCampState(SaSmfCmpgStateT i_state);

///
/// Purpose: Add one SmfUpgradeProcedure to the campaign.
/// @param   i_procedure A pointer to a (filled in) SmfUpgradeProcedure object that should be added.
/// @return  None.
///
	bool addUpgradeProcedure(SmfUpgradeProcedure * i_procedure);

///
/// Purpose: Get pointers to the upgrade procedures.
/// @param   None.
/// @return  A vector containing pointers to SmfUpgradeProcedure objecs.
///
	const std::vector < SmfUpgradeProcedure * >& getUpgradeProcedures();

///
/// Purpose: Sort the added SmfUpgradeProcedure in execution level order.
/// @param   None.
/// @return  None.
///
	void sortProceduresInExecLevelOrder();

///
/// Purpose: Add one "add to IMM" campaign init operation to the campaign.
/// @param   i_operation A pointer to a (filled in) SmfImmOperation object that should be added.
/// @return  None.
///
	void addCampInitAddToImm(SmfImmOperation * i_operation);

///
/// Purpose: Add one campaign init action to the campaign.
/// @param   i_action A pointer to a (filled in) SmfUpgradeAction object that should be added.
/// @return  None.
///
	void addCampInitAction(SmfUpgradeAction * i_action);

///
/// Purpose: Add one campaign wrapup operation to the campaign.
/// @param   i_operation A pointer to a (filled in) SmfImmOperation object that should be added.
/// @return  None.
///
	void addCampWrapupRemoveFromImm(SmfImmOperation * i_operation);

///
/// Purpose: Add one campaign complete action to the campaign.
/// @param   i_action A pointer to a (filled in) SmfUpgradeAction object that should be added.
/// @return  None.
///
	void addCampCompleteAction(SmfUpgradeAction * i_action);

///
/// Purpose: Add one campaign wrapup action to the campaign.
/// @param   i_action A pointer to a (filled in) SmfUpgradeAction object that should be added.
/// @return  None.
///
	void addCampWrapupAction(SmfUpgradeAction * i_action);

///
/// Purpose: Set time for waitToCommit.
/// @param   i_time The time before commit is permitted.
/// @return  None.
///
	void setWaitToCommit(SaTimeT i_time);

///
/// Purpose: Get time for waitToCommit.
/// @param   i_time The time before commit is permitted. In nano sec.
/// @return  SaTimeT
///
	SaTimeT getWaitToCommit();

///
/// Purpose: Set time for waitToAllowNewCampaign.
/// @param   i_time The time before commit is permitted.
/// @return  None.
///
	void setWaitToAllowNewCampaign(SaTimeT i_time);

///
/// Purpose: Get time for waitToAllowNewCampaign.
/// @param   i_time The time before commit is permitted. In nano sec.
/// @return  SaTimeT
///
	SaTimeT getWaitToAllowNewCampaign();

///
/// Purpose: Create the SmfCampRestartInfo object
/// @param   None.
/// @return  SaAisErrorT
///
	SaAisErrorT createCampRestartInfo();

///
/// Purpose: Check if number of campaign restarts are exceeded
/// @param   bool o_result, .true if level exceeded otherwise false
/// @return  SaAisErrorT
///
	SaAisErrorT tooManyRestarts(bool *o_result);

///
/// Purpose: Disable IMM persistent backend (PBE)
/// @return  SaAisErrorT
///
	SaAisErrorT disablePbe();

///
/// Purpose: Activate IMM persistent backend (PBE) if activated when campaign was started
/// @return  SaAisErrorT
///
	SaAisErrorT restorePbe();

///
/// Purpose: Mark the campaign not possible to rollback
/// @param   std::string& i_reason, reason information
/// @return  SaAisErrorT
///
	SaAisErrorT disableCampRollback(std::string i_reason);

///
/// Purpose: Check if campaign is possible to rollback
/// @param   bool o_result, true if disabled otherwise false
/// @param   std::string& o_reason, reason information
/// @return  SaAisErrorT
///
	SaAisErrorT isCampRollbackDisabled(bool& o_result, std::string& o_reason);

///
/// Purpose: Start or continue the execution of the campaign.
/// @param   None.
/// @return  None.
///
	void execute();

///
/// Purpose: Execute the campaign initialization part.
/// @param   None.
/// @return  None.
///
	void executeInit();

///
/// Purpose: Execute the campaign upgrade procedures.
/// @param   None.
/// @return  None.
///
	void executeProc();

///
/// Purpose: Execute campaign wrapup part.
/// @param   None.
/// @return  None.
///
	void executeWrapup();

///
/// Purpose: Rollback the campaign.
/// @param   None.
/// @return  None.
///
	void rollback();

///
/// Purpose: call application registered callback functions prior to the execution of the campaign.
/// @param   None.
/// @return  None.
///
void verify();

///
/// Purpose: Rollback the campaign upgrade procedure.
/// @param   None.
/// @return  None.
///
	void rollbackProc();

///
/// Purpose: Suspend the campaign.
/// @param   None.
/// @return  None.
///
	void suspend();

///
/// Purpose: Commit the campaign.
/// @param   None.
/// @return  None.
///
	void commit();

///
/// Purpose: Procedure result.
/// @param   None.
/// @return  None.
///
	void procResult(SmfUpgradeProcedure* i_procedure, SmfProcResultT i_result);

///
/// Purpose: Continue executing the campaign
/// @param   None.
/// @return  None.
///
	void continueExec();

///
/// Purpose: Reset Maintenance State
/// @param   None.
/// @return  None.
///
	void resetMaintenanceState();

///
/// Purpose: Remove runtime objects
/// @param   None.
/// @return  None.
///
	void removeRunTimeObjects();
///
/// Purpose: Remove config objects
/// @param   None.
/// @return  None.
///
	void removeConfigObjects();
///
/// Purpose: Create SMF campaign restart indicator
/// @param   None.
/// @return  SaAisErrorT SA_AIS_OK if ok, othervise other error code
///
	SaAisErrorT createSmfRestartIndicator();

///
/// Purpose: Check SMF campaign restart indicator
/// @param   None.
/// @return  SaAisErrorT SA_AIS_OK if ok, othervise other error code
///
        SaAisErrorT checkSmfRestartIndicator();

///
/// Purpose: Get the unmodified procedures, no merged procedures included
///
	const std::vector<SmfUpgradeProcedure*>& getOriginalProcedures() { return m_originalProcedures; };

///
/// Purpose: Add the merged procedure while saving the original
///
	void addMergedProcedure(SmfUpgradeProcedure* procedure);

///
/// Purpose: Can be used with any procedure execution mode
///
	const std::vector<SmfUpgradeProcedure*>& getProcedures() { return m_procedure; }

        SmfExecControlObjHandler* getExecControlHdl() { return m_execControlHdl; }

/// Purpose: Get the procedure ecxecution mode
/// @param   none
/// @return  The execution mode.
///
        SaUint32T getProcExecutionMode() { return m_execControlHdl->procExecMode(NULL); }

	SmfCampaignInit & getCampaignInit() { return  m_campInit; }
	SmfCampaignWrapup & getCampaignWrapup() { return  m_campWrapup; }

	friend class SmfCampState;
	friend class SmfCampStateInitial;
	friend class SmfCampStateExecuting;
	friend class SmfCampStateExecCompleted;
	friend class SmfCampStateSuspendingExec;
	friend class SmfCampStateExecSuspended;
	friend class SmfCampStateCommitted;
	friend class SmfCampStateExecFailed;
	friend class SmfCampStateErrorDetected;
	friend class SmfCampStateErrorDetectedInSuspending;
	friend class SmfCampStateSuspendedByErrorDetected;
	friend class SmfCampRollbackCommitted;
	friend class SmfCampRollbackCompleted;
	friend class SmfCampRollbackFailed;
	friend class SmfCampRollbackSuspended;
	friend class SmfCampRollingBack;
	friend class SmfCampSuspendingRollback;

private:

	void changeState(const SmfCampState * i_state);

//--------------------------------------------------------------------------
// Purpose:  Disables copy constructor
//--------------------------------------------------------------------------
	 SmfUpgradeCampaign(const SmfUpgradeCampaign &);

//--------------------------------------------------------------------------
// Purpose:  Disables assignment operator
//--------------------------------------------------------------------------
	 SmfUpgradeCampaign & operator=(const SmfUpgradeCampaign &);

//------------------------------------------------------------------------------
// Purpose:  Execute the procedures within the campaign
// Comments: -
// @param    None
// @return   None
//------------------------------------------------------------------------------
	void procedureExecute();

	SmfCampState *m_state;
        std::string m_campPackageName;
        std::string m_campaignName;
        std::string m_noNameSpaceSchemaLocation;
        std::string m_xsi;
        std::string m_campaignPeriod;
        std::string m_configurationBase;
	SmfCampaignInit m_campInit;
	SmfCampaignWrapup m_campWrapup;
        std::vector<SmfUpgradeProcedure*> m_procedure;
        std::vector<SmfUpgradeProcedure*> m_originalProcedures;
        SaTimeT m_waitToCommit;
        SaTimeT m_waitToAllowNewCampaign;
	int m_noOfExecutingProc;
        int m_noOfProcResponses;
        SmfExecControlObjHandler* m_execControlHdl;
};
#endif				// __SMFUPGRADECAMPAIGN_H
