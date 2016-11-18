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

#ifndef SMF_SMFD_SMFCAMPAIGNINIT_H_
#define SMF_SMFD_SMFCAMPAIGNINIT_H_

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

///
/// Purpose: Add a callback to be issued at init.
/// @param   i_operation A pointer to a SmfCallback.
/// @return  None.
///
	void addCallbackAtInit(SmfCallback* i_cbk);

///
/// Purpose: Add a callback to be issued at backup.
/// @param   i_operation A pointer to a SmfCallback.
/// @return  None.
///
	void addCallbackAtBackup(SmfCallback* i_cbk);

///
/// Purpose: Add a callback to be issued at rollback.
/// @param   i_operation A pointer to a SmfCallback.
/// @return  None.
///
	void addCallbackAtRollback(SmfCallback* i_cbk);

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
/// Purpose: Execute callback at campaign init.
/// @param   None.
/// @return  SaAisErrorT SA_AIS_OK if ok, otherwise failure.
///
	SaAisErrorT executeCallbackAtInit();

///
/// Purpose: Execute specified backup callbacks.
/// @param   None.
/// @return  SaAisErrorT SA_AIS_OK if ok, otherwise failure.
///
	SaAisErrorT executeCallbackAtBackup();

///
/// Purpose: Perform rollback actions.
/// @param   None.
/// @return  None.
///
	bool rollback();

///
/// Purpose: Execute callback at rollback.
/// @param   None.
/// @return  None.
///
	bool executeCallbackAtRollback();

 private:

	 std::list < SmfImmOperation * >m_addToImm;
	 std::list < SmfUpgradeAction * >m_campInitAction;

	 std::list < SmfCallback * >m_callbackAtInit;
	 std::list < SmfCallback * >m_callbackAtBackup;
	 std::list < SmfCallback * >m_callbackAtRollback;
};

#endif  // SMF_SMFD_SMFCAMPAIGNINIT_H_
