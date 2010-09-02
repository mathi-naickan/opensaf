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

#ifndef SMFCAMPAIGNINIT_HH
#define SMFCAMPAIGNINIT_HH

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

///
/// Purpose: Class for execution of campaign initiation actions.
///
class SmfCampaignInit {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfCampaignInit();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfCampaignInit();

///
/// Purpose: Add operations from the campaign xml addToImm section.
/// @param   i_operation A pointer to a SmfImmOperation.
/// @return  None.
///
	void addAddToImm(SmfImmOperation * i_operation);

///
/// Purpose: Get operations from the campaign xml addToImm section.
/// @param   None
/// @return  A list of SmfImmOperation*
///
	const std::list < SmfImmOperation * >& getAddToImm();

#if 0
///
/// Purpose: Add a callback to be issued at init.
/// @param   i_operation A pointer to a SmfCallbackOptions.
/// @return  None.
///
	void addCallbackAtInit(SmfCallbackOptions * i_option);

///
/// Purpose: Add a callback to be issued at backup.
/// @param   i_operation A pointer to a SmfCallbackOptions.
/// @return  None.
///
	void addCallbackAtBackup(SmfCallbackOptions * i_option);

///
/// Purpose: Add a callback to be issued at rollback.
/// @param   i_operation A pointer to a SmfCallbackOptions.
/// @return  None.
///
	void addCallbackAtRollback(SmfCallbackOptions * i_option);
#endif

///
/// Purpose: Add an action to be performed. Actions read from campaign campInitAction (adminOp/IMM CCB/CLI). 
/// @param   i_action A pointer to a SmfUpgradeAction.
/// @return  None.
///
	void addCampInitAction(SmfUpgradeAction * i_action);

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	bool execute();

///
/// Purpose: Perform backup actions.
/// @param   None.
/// @return  None.
///
	bool executeBackup();

///
/// Purpose: Perform rollback actions.
/// @param   None.
/// @return  None.
///
	bool executeRollback();

 private:

	 std::list < SmfImmOperation * >m_addToImm;
	 std::list < SmfUpgradeAction * >m_campInitAction;

#if 0
	 std::list < SmfCallbackOptions * >m_callbackAtInit;
	 std::list < SmfCallbackOptions * >m_callbackAtBackup;
	 std::list < SmfCallbackOptions * >m_callbackAtRollback;
#endif
};

#endif				// SMFCAMPAIGNINIT_HH
