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

#ifndef SMFUPGRADEACTION_HH
#define SMFUPGRADEACTION_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>

#include <saAis.h>

#include <saImmOm.h>
#include "SmfImmOperation.hh"
#include "SmfCallback.hh"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

class SmfPlmExecEnv;
class SmfCallback;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

///
/// Purpose: Base class for all actions.
///

class SmfUpgradeAction {
 public:

///
/// The constructor
/// @param    i_id The id of this action
/// @return   None
///
	SmfUpgradeAction(int i_id);

///
/// The dDestructor
/// @param    None
/// @return   None
///
	 virtual ~ SmfUpgradeAction();

///
/// Purpose: Execute the upgrade action (to be specialized).
/// @param   None
/// @return  0 on success, otherwise failure.
///
	virtual SaAisErrorT execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn = NULL);

///
/// Purpose: Rollback the upgrade action (to be specialized).
/// @param   None
/// @return  0 on success, otherwise failure.
///
	virtual SaAisErrorT rollback(const std::string& i_rollbackDn);

///
/// Purpose: Get action Id (to be specialized).
/// @param   None
/// @return  An int containing the action Id.
///
	virtual int getId();

 private:

///
/// Purpose: Disables copy constructor.
///
	 SmfUpgradeAction(const SmfUpgradeAction &);

///
/// Purpose: Disables  assignment operator.
///
	 SmfUpgradeAction & operator=(const SmfUpgradeAction &);

 protected:
	int m_id;		// The Id of this action
};

///
/// Purpose: Class for Cli Command action
///

class SmfCliCommandAction:public SmfUpgradeAction {
 public:

///
/// Purpose: The constructor
/// @param    i_id Th Id of the action
/// @return   None
///
	SmfCliCommandAction(int i_id);

///
/// Purpose: The dDestructor
/// @param    None
/// @return   None
///
	~SmfCliCommandAction();

///
/// Purpose: Execure the CLI command action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///

	SaAisErrorT execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn = NULL);

///
/// Purpose: Rollback the CLI command action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT rollback(const std::string& i_rollbackDn);

///
/// Purpose: Set the Do command (executed in execute() method).
/// @param   i_cmd A string specifying the do cmd
/// @return  None.
///
	void setDoCmd(const std::string & i_cmd);

///
/// Purpose: Set the Do command arguments (used in execute() method).
/// @param   i_cmdArgs A string specifying the do cmd arguments
/// @return  None.
///
	void setDoCmdArgs(const std::string & i_cmdArgs);

///
/// Purpose: Set the Undo command (executed in rollback() method).
/// @param   i_cmd A string specifying the undo cmd
/// @return  None.
///
	void setUndoCmd(const std::string & i_cmd);

///
/// Purpose: Set the Undo command arguments (used in rollback() method).
/// @param   i_cmdArgs A string specifying the undo cmd arguments
/// @return  None.
///
	void setUndoCmdArgs(const std::string & i_cmdArgs);


///
/// Purpose: Get the list of PlmExecEnv objects.
/// @param   None
/// @return  The list.
///
	inline const std::list<SmfPlmExecEnv>& getPlmExecEnvList() const;

 private:
///
/// Purpose: Disables copy constructor.
///
	 SmfCliCommandAction(const SmfCliCommandAction &);

///
/// Purpose: Disables  assignment operator.
///
	 SmfCliCommandAction & operator=(const SmfCliCommandAction &);

	friend class SmfCampaignXmlParser;
	 std::string m_doCmd;	// The do command to be executed
	 std::string m_doCmdArgs;	// The arguments to the do command
	 std::string m_undoCmd;	// The undo command to be executed
	 std::string m_undoCmdArgs;	// The arguments to the undo command
	std::list<SmfPlmExecEnv> m_plmExecEnvList;
};

///
/// Purpose: Class for admin operation action
///

class SmfAdminOperationAction:public SmfUpgradeAction {
 public:

///
/// The constructor
/// @param    i_id The id of this action
/// @return   None
///
	SmfAdminOperationAction(int i_id);

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfAdminOperationAction();

///
/// Purpose: Execure the admin operation action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn = NULL);

///
/// Purpose: Rollback the admin operation action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT rollback(const std::string& i_rollbackDn);

///
/// Purpose: Set the dn to the do object (executed in execute() method).
/// @param   i_dn A std::string containing the DN of the object where to apply the operation.
/// @return  None.
///
	void setDoDn(const std::string & i_dn);

///
/// Purpose: Set the id of the do admin operation (an admin operation is a number)
/// @param   i_dn An int containing the admin operation number.
/// @return  None.
///
	void setDoId(int i_id);

///
/// Purpose: Add a parameter to be set in the do admin operation
/// @param   i_name The name of the parameter.
/// @param   i_type The type of the parameter.
/// @param   i_value The value of the parameter.
/// @return  None.
///
	void addDoParameter(const std::string & i_name, const std::string & i_type, const std::string & i_value);

///
/// Purpose: Set the dn to the undo object (executed in rollback() method).
/// @param   i_dn A std::string containing the DN of the object where to apply the operation.
/// @return  None.
///
	void setUndoDn(const std::string & i_dn);

///
/// Purpose: Set the id of the undo admin operation (an admin operation is a number)
/// @param   i_dn An int containing the admin operation number.
/// @return  None.
///
	void setUndoId(int i_id);

///
/// Purpose: Add a parameter to be set in the undo admin operation
/// @param   i_name The name of the parameter.
/// @param   i_type The type of the parameter.
/// @param   i_value The value of the parameter.
/// @return  None.
///
	void addUndoParameter(const std::string & i_name, const std::string & i_type, const std::string & i_value);

 private:

	class SmfAdminOperationParameter {
 public:
		std::string m_name;	// The parameter name 
		std::string m_type;	// The parameter type 
		std::string m_value;	// The parameter value 
	};

///
/// Purpose: Disables copy constructor.
///
	 SmfAdminOperationAction(const SmfAdminOperationAction &);

///
/// Purpose: Disables  assignment operator.
///
	 SmfAdminOperationAction & operator=(const SmfAdminOperationAction &);

///
/// Purpose: Create a SaImmAdminOperationParamsT_2 ** from strings parsed from campaign xml
/// @param   A std::list < SmfAdminOperationParameter >.
/// @return  A SaImmAdminOperationParamsT_2 ** pointer if parameters exist, otherwise 0.
///
         const SaImmAdminOperationParamsT_2 ** createAdmOperParams(std::list < SmfAdminOperationParameter > i_parameters);

	 std::string m_doDn;	// The object to do admin operation on 
	 int m_doOpId;		// The id of the admin operation
	 std::list < SmfAdminOperationParameter > m_doParameters;	// A list of parameters to the admin operation 

	 std::string m_undoDn;	// The object to do admin operation on 
	 int m_undoOpId;	// The id of the admin operation
	 std::list < SmfAdminOperationParameter > m_undoParameters;	// A list of parameters to the admin operation 
};

///
/// Purpose: Class for Imm CCB action
///

class SmfImmCcbAction:public SmfUpgradeAction {
 public:

///
/// The constructor
/// @param    i_id Th Id of the action
/// @return   None
///
	SmfImmCcbAction(int i_id);

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfImmCcbAction();

///
/// Purpose: Execute the IMM CCB action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn = NULL);

///
/// Purpose: Rollback the IMM CCB action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT rollback(const std::string& i_rollbackDn);

///
/// Purpose: Set the CCB flag.
/// @param   i_flag A.std::string representation of the flag
/// @return  None.
///
	void setCcbFlag(const std::string & i_flag);

///
/// Purpose: Add an IMM operation to the CCB.
/// @param   i_op A.pointer to a SmfImmOperation.
/// @return  None.
///
	void addOperation(SmfImmOperation * i_op);

 private:
///
/// Purpose: Disables copy constructor.
///
	 SmfImmCcbAction(const SmfImmCcbAction &);

///
/// Purpose: Disables  assignment operator.
///
	 SmfImmCcbAction & operator=(const SmfImmCcbAction &);

	 std::string m_ccbFlags;	/* CCB flags */
	 std::list < SmfImmOperation * >m_operations;	/* A list of Imm operations within this ccb */
};

class SmfCallbackAction:public SmfUpgradeAction {
 public:

///
/// The constructor
/// @param    i_id Th Id of the action
/// @return   None
///
	SmfCallbackAction(int i_id);

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfCallbackAction();

///
/// Purpose: Execute the callback action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn = NULL);

///
/// Purpose: Rollback the callback action.
/// @param   None.
/// @return  0 on success, otherwise failure.
///
	SaAisErrorT rollback(const std::string& i_rollbackDn);

	SmfCallback & getCallback(void) { return m_callback; }

 private:
///
/// Purpose: Disables copy constructor.
///
	 SmfCallbackAction(const SmfCallbackAction &);

///
/// Purpose: Disables  assignment operator.
///
	 SmfCallbackAction & operator=(const SmfCallbackAction &);

	 SmfCallback m_callback;	/* SmfCallback attribute */
};

#endif				// SMFUPGRADEACTION_HH
