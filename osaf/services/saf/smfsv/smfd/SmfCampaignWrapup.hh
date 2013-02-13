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

#ifndef SMFCAMPAIGNWRAPUP_HH
#define SMFCAMPAIGNWRAPUP_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>

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

class SmfImmOperation;
class SmfUpgradeAction;
class SmfCallback;

//================================================================================
// Class SmfCampaignWrapup
//================================================================================
///
/// Purpose: Class for execution of campaign wrapup actions.
///
class SmfCampaignWrapup {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfCampaignWrapup();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfCampaignWrapup();

///
/// Purpose: Add operations from the campaign xml removeFromImm section.
/// @param   i_operation A pointer to a SmfImmOperation.
/// @return  None.
///
	void addRemoveFromImm(SmfImmOperation * i_operation);

///
/// Purpose: Add a callback to be issued at commit (from callbackAtCommit section).
/// @param   i_option A pointer to a SmfCallbackOptions.
/// @return  None.
///
	void addCallbackAtCommit(SmfCallback* i_cbk);


///
/// Purpose: Add an action to be executed at campaign commit (campCommitAction in campaign xml)..
/// @param   i_action A pointer to a SmfUpgradeAction.
/// @return  None.
///
	void addCampCompleteAction(SmfUpgradeAction * i_action);

///
/// Purpose: Add an action to be executed at campaign wrapup (campWrapupAction in campaign xml)..
/// @param   i_action A pointer to a SmfUpgradeAction.
/// @return  None.
///
	void addCampWrapupAction(SmfUpgradeAction * i_action);

///
/// Purpose: Execute campaing wrapup actions.
/// @param   None.
/// @return  True if sucessful otherwise false.
///
	bool executeCampWrapup();

///
/// Purpose: Rollback campaing wrapup actions.
/// @param   None.
/// @return  True if sucessful otherwise false.
///
	bool rollbackCampWrapup();

///
/// Purpose: Execute wrapup complete actions.
/// @param   None.
/// @return  True if sucessful otherwise false.
///
	bool executeCampComplete();

///
/// Purpose: Rollback campaign complete actions.
/// @param   None.
/// @return  True if sucessful otherwise false.
///
	bool rollbackCampComplete();

 private:
	 std::list < SmfImmOperation * >m_removeFromImm;
	 std::list < SmfUpgradeAction * >m_campCompleteAction;
	 std::list < SmfUpgradeAction * >m_campWrapupAction;

	 std::list < SmfCallback* >m_callbackAtCommit;
};

#endif				// SMFCAMPAIGNWRAPUP_HH
