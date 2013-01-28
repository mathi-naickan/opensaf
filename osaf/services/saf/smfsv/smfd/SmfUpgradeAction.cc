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
#include <immutil.h>
#include "stdio.h"
#include "logtrace.h"
#include "SmfUpgradeAction.hh"
#include "SmfRollback.hh"
#include "SmfUtils.hh"
#include "smfd.h"
#include <SmfTargetTemplate.hh>

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

//================================================================================
// Class SmfUpgradeAction
// Purpose:
// Comments:
//================================================================================

SmfUpgradeAction::SmfUpgradeAction(int i_id):
   m_id(i_id)
{

}

// ------------------------------------------------------------------------------
// ~SmfUpgradeAction()
// ------------------------------------------------------------------------------
SmfUpgradeAction::~SmfUpgradeAction()
{
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT
SmfUpgradeAction::execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn)
{
	LOG_ER("execute must be specialised");
	return SA_AIS_ERR_NOT_EXIST;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfUpgradeAction::rollback(const std::string& i_rollbackDn)
{
	LOG_ER("rollback must be specialised");
	return SA_AIS_ERR_FAILED_OPERATION;
}

//------------------------------------------------------------------------------
// getId()
//------------------------------------------------------------------------------
int 
SmfUpgradeAction::getId()
{
	return m_id;
}

//================================================================================
// Class SmfCliCommandAction
// Purpose:
// Comments:
//================================================================================

SmfCliCommandAction::SmfCliCommandAction(int i_id):
   SmfUpgradeAction(i_id)
{

}

// ------------------------------------------------------------------------------
// ~SmfUpgradeAction()
// ------------------------------------------------------------------------------
SmfCliCommandAction::~SmfCliCommandAction()
{
}

//------------------------------------------------------------------------------
// setDoCmd()
//------------------------------------------------------------------------------
void
SmfCliCommandAction::setDoCmd(const std::string & i_cmd)
{
	m_doCmd = i_cmd;
}

//------------------------------------------------------------------------------
// setDoCmdArgs()
//------------------------------------------------------------------------------
void 
SmfCliCommandAction::setDoCmdArgs(const std::string & i_cmdArgs)
{
	m_doCmdArgs = i_cmdArgs;
}

//------------------------------------------------------------------------------
// setUndoCmd()
//------------------------------------------------------------------------------
void 
SmfCliCommandAction::setUndoCmd(const std::string & i_cmd)
{
	m_undoCmd = i_cmd;
}

//------------------------------------------------------------------------------
// setUndoCmdArgs()
//------------------------------------------------------------------------------
void 
SmfCliCommandAction::setUndoCmdArgs(const std::string & i_cmdArgs)
{
	m_undoCmdArgs = i_cmdArgs;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfCliCommandAction::execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn)
{
	SaAisErrorT result = SA_AIS_OK;
	SaTimeT timeout = smfd_cb->cliTimeout;	/* Default timeout */
	std::string command;
	std::list <SmfPlmExecEnv>::iterator it;

	TRACE_ENTER();

	command = m_doCmd;
	if (m_doCmdArgs.length() > 0) {
		command += " ";
		command += m_doCmdArgs;
	}

	for (it = m_plmExecEnvList.begin(); it != m_plmExecEnvList.end(); ++it) {
		const std::string& n = it->getPrefered();
		MDS_DEST nodeDest = getNodeDestination(n);
		if (nodeDest == 0) {
			LOG_ER("SmfCliCommandAction no node destination found for node %s", n.c_str());
			result = SA_AIS_ERR_NOT_EXIST;
			goto done;
		}

		TRACE("executing command '%s' on node '%s'", command.c_str(), n.c_str());

		/* Execute the script remote on node */
		int rc = smfnd_remote_cmd(command.c_str(), nodeDest, timeout / 10000000);	/* convert ns to 10 ms timeout */
		if (rc != 0) {
			LOG_ER("executing command '%s' on node '%s' failed with rc %d", command.c_str(), n.c_str(),
			       rc);
			result = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
	}

 done:
	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfCliCommandAction::rollback(const std::string& i_rollbackDn)
{
	SaAisErrorT result = SA_AIS_OK;
	SaTimeT timeout = smfd_cb->cliTimeout;	/* Default timeout */
	std::string command;
	std::list <SmfPlmExecEnv>::reverse_iterator it;

	TRACE_ENTER();

        TRACE("Start rollback of Cli command action");
	command = m_undoCmd;
	if (m_undoCmdArgs.length() > 0) {
		command += " ";
		command += m_undoCmdArgs;
	}

        /* Execute the undo command on nodes in reverse order */
	for (it = m_plmExecEnvList.rbegin(); it != m_plmExecEnvList.rend(); it++) {
		const std::string& n = it->getPrefered();
		MDS_DEST nodeDest = getNodeDestination(n);
		if (nodeDest == 0) {
			LOG_ER("SmfCliCommandAction no node destination found for node %s", n.c_str());
			result = SA_AIS_ERR_NOT_EXIST;
			goto done;
		}

		TRACE("executing undo command '%s' on node '%s'", command.c_str(), n.c_str());

		/* Execute the script remote on node */
		int rc = smfnd_remote_cmd(command.c_str(), nodeDest, timeout / 10000000);	/* convert ns to 10 ms timeout */
		if (rc != 0) {
			LOG_ER("executing undo command '%s' on node '%s' failed with rc %d", 
                               command.c_str(), n.c_str(), rc);

			result = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
	}

 done:
	TRACE_LEAVE();
	return result;
}

//================================================================================
// Class SmfAdminOperationAction
// Purpose:
// Comments:
//================================================================================

SmfAdminOperationAction::SmfAdminOperationAction(int i_id):
	SmfUpgradeAction(i_id),
	m_doOpId(-1),
	m_undoOpId(-1)
{

}

// ------------------------------------------------------------------------------
// ~SmfAdminOperationAction()
// ------------------------------------------------------------------------------
SmfAdminOperationAction::~SmfAdminOperationAction()
{

}

// ------------------------------------------------------------------------------
// createAdmOperParams()
// ------------------------------------------------------------------------------
const SaImmAdminOperationParamsT_2 ** 
SmfAdminOperationAction::createAdmOperParams(std::list < SmfAdminOperationParameter > i_parameters)
{
	//Create space for param pointers
	const SaImmAdminOperationParamsT_2 **params = 
                (const SaImmAdminOperationParamsT_2 **) new SaImmAdminOperationParamsT_2 *[i_parameters.size() + 1];

	std::list < SmfAdminOperationParameter >::iterator iter;
	std::list < SmfAdminOperationParameter >::iterator iterE;
	iter  = i_parameters.begin();
	iterE = i_parameters.end();

	int i = 0;		//Index to a SaImmAdminOperationParamsT_2 pointer in the params array

	//For all parameters
	while (iter != iterE) {
		//Create structure for one parameter
		SaImmAdminOperationParamsT_2 *par = new(std::nothrow)  SaImmAdminOperationParamsT_2();
		osafassert(par != 0);

		par->paramName   = (SaStringT)(*iter).m_name.c_str();
		par->paramType   = (SaImmValueTypeT)smf_stringToImmType((char *)(*iter).m_type.c_str());
                smf_stringToValue(par->paramType, &par->paramBuffer, (*iter).m_value.c_str());

		//Add the pointer to the SaImmAdminOperationParamsT_2 structure to the parameter list
		params[i++] = par;

		iter++;
	}

	params[i] = NULL;	//Null terminate the list of parameter pointers

        return params;
}

//------------------------------------------------------------------------------
// setDoDn()
//------------------------------------------------------------------------------
void
SmfAdminOperationAction::setDoDn(const std::string & i_dn)
{
	m_doDn = i_dn;
}

//------------------------------------------------------------------------------
// setDoId()
//------------------------------------------------------------------------------
void 
SmfAdminOperationAction::setDoId(int i_id)
{
	m_doOpId = i_id;
}

//------------------------------------------------------------------------------
// addDoParameter()
//------------------------------------------------------------------------------
void 
SmfAdminOperationAction::addDoParameter(const std::string & i_name, const std::string & i_type,
					     const std::string & i_value)
{
	SmfAdminOperationParameter parameter;
	parameter.m_name = i_name;
	parameter.m_type = i_type;
	parameter.m_value = i_value;

	m_doParameters.push_back(parameter);
}

//------------------------------------------------------------------------------
// setUndoDn()
//------------------------------------------------------------------------------
void 
SmfAdminOperationAction::setUndoDn(const std::string & i_dn)
{
	m_undoDn = i_dn;
}

//------------------------------------------------------------------------------
// setUndoId()
//------------------------------------------------------------------------------
void 
SmfAdminOperationAction::setUndoId(int i_id)
{
	m_undoOpId = i_id;
}

//------------------------------------------------------------------------------
// addUndoParameter()
//------------------------------------------------------------------------------
void 
SmfAdminOperationAction::addUndoParameter(const std::string & i_name, const std::string & i_type,
					       const std::string & i_value)
{
	SmfAdminOperationParameter parameter;
	parameter.m_name = i_name;
	parameter.m_type = i_type;
	parameter.m_value = i_value;

	m_undoParameters.push_back(parameter);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT
SmfAdminOperationAction::execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn)
{
	TRACE_ENTER();

	TRACE("execute admin op, do objectDN = %s, operationID = %d",m_doDn.c_str(), m_doOpId);

	std::list < SmfAdminOperationParameter >::iterator iter = m_doParameters.begin();
	std::list < SmfAdminOperationParameter >::iterator iterE = m_doParameters.end();

	while (iter != iterE) {
		TRACE("parameter name = %s, type = %s, value = '%s'",
                      (*iter).m_name.c_str(),(*iter).m_type.c_str(),(*iter).m_value.c_str());
                iter++;
	}

        const SaImmAdminOperationParamsT_2 **params = createAdmOperParams(m_doParameters);

	SmfImmUtils siu;
        SaAisErrorT rc = siu.callAdminOperation(m_doDn, m_doOpId, params, smfd_cb->adminOpTimeout);

	TRACE_LEAVE();

	return rc;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfAdminOperationAction::rollback(const std::string& i_rollbackDn)
{
	TRACE_ENTER();

	TRACE("rollback admin op, undo objectDN = %s, operationID = %d",m_undoDn.c_str(), m_undoOpId);

	std::list < SmfAdminOperationParameter >::iterator iter = m_undoParameters.begin();
	std::list < SmfAdminOperationParameter >::iterator iterE = m_undoParameters.end();

	while (iter != iterE) {
		TRACE("parameter name = %s, type = %s, value = '%s'",
                      (*iter).m_name.c_str(),(*iter).m_type.c_str(),(*iter).m_value.c_str());
                iter++;
	}

        const SaImmAdminOperationParamsT_2 **params = createAdmOperParams(m_undoParameters);

	SmfImmUtils siu;
        SaAisErrorT rc = siu.callAdminOperation(m_undoDn, m_undoOpId, params, smfd_cb->adminOpTimeout);

	TRACE_LEAVE();

	return rc;
}

//================================================================================
// Class SmfImmCcbAction
// Purpose:
// Comments:
//================================================================================

SmfImmCcbAction::SmfImmCcbAction(int i_id):
   SmfUpgradeAction(i_id)
{

}

// ------------------------------------------------------------------------------
// ~SmfImmCcbAction()
// ------------------------------------------------------------------------------
SmfImmCcbAction::~SmfImmCcbAction()
{
	std::list < SmfImmOperation * >::iterator it;

	/* Delete Imm operations */
	for (it = m_operations.begin(); it != m_operations.end(); ++it) {
		delete(*it);
	}
}

//------------------------------------------------------------------------------
// setCcbFlag()
//------------------------------------------------------------------------------
void
SmfImmCcbAction::setCcbFlag(const std::string & i_flag)
{
	m_ccbFlags = i_flag;
}

//------------------------------------------------------------------------------
// addOperation()
//------------------------------------------------------------------------------
void 
SmfImmCcbAction::addOperation(SmfImmOperation * i_op)
{
	m_operations.push_back(i_op);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmCcbAction::execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn)
{
	SaAisErrorT result = SA_AIS_OK;
        SmfRollbackCcb* rollbackCcb = NULL;

        TRACE_ENTER();

	TRACE("Imm ccb actions id %d, size %zu", m_id, m_operations.size());
        if (i_rollbackDn != NULL) {
                std::string immRollbackCcbDn;
                char idStr[16];
                sprintf(idStr, "%08d", m_id);
                immRollbackCcbDn = "smfRollbackElement=ccb_";
                immRollbackCcbDn += idStr;
                immRollbackCcbDn += ",";
                immRollbackCcbDn += *i_rollbackDn;
        
                if ((result = smfCreateRollbackElement(immRollbackCcbDn,
                                                       i_oiHandle)) != SA_AIS_OK) {
                        LOG_ER("SmfImmCcbAction::execute failed to create rollback element %s, rc = %d", 
                               immRollbackCcbDn.c_str(), result);
                        return result;
                }

                rollbackCcb = new (std::nothrow) SmfRollbackCcb (immRollbackCcbDn, i_oiHandle);
                if (rollbackCcb == NULL) {
                        LOG_ER("SmfImmCcbAction::execute failed to create SmfRollbackCcb");
                        return SA_AIS_ERR_NO_MEMORY;
                }
        }

	if (m_operations.size() > 0) {
		SmfImmUtils immUtil;
		if ((result = immUtil.doImmOperations(m_operations, rollbackCcb)) != SA_AIS_OK) {
                        delete rollbackCcb;
                        rollbackCcb = NULL;
		}
	}

        if (rollbackCcb != NULL) {
                if ((result = rollbackCcb->execute()) != SA_AIS_OK) {
			LOG_ER("SmfImmCcbAction::execute failed to store rollback CCB %d", result);
                }
                delete rollbackCcb;
        }

	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmCcbAction::rollback(const std::string& i_rollbackDn)
{
	SaAisErrorT result = SA_AIS_OK;
        SmfImmUtils immUtil;

        TRACE_ENTER();

        std::string immRollbackCcbDn;
        char idStr[16];
        sprintf(idStr, "%08d", m_id);
        immRollbackCcbDn = "smfRollbackElement=ccb_";
        immRollbackCcbDn += idStr;
        immRollbackCcbDn += ",";
        immRollbackCcbDn += i_rollbackDn;

        SmfRollbackCcb rollbackCcb(immRollbackCcbDn, 0);

	TRACE("Rollback IMM ccb actions id %d, dn %s", m_id, immRollbackCcbDn.c_str());

        if ((result = rollbackCcb.rollback()) != SA_AIS_OK) {
                LOG_ER("SmfImmCcbAction::rollback failed to rollback CCB %s, %d", immRollbackCcbDn.c_str(), result);
        }

	return result;
}


//================================================================================
// Class SmfCallbackAction
// Purpose:
// Comments:
//================================================================================

SmfCallbackAction::SmfCallbackAction(int i_id):
   SmfUpgradeAction(i_id)
{

}

// ------------------------------------------------------------------------------
// ~SmfCallbackAction()
// ------------------------------------------------------------------------------
SmfCallbackAction::~SmfCallbackAction()
{
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCallbackAction::execute(SaImmOiHandleT i_oiHandle, const std::string* i_rollbackDn)
{
	SaAisErrorT result;
	std::string dn;
	TRACE_ENTER();

	TRACE("execute callbackAction");

	result = m_callback.execute(dn);

	TRACE_LEAVE();

	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT
SmfCallbackAction::rollback(const std::string& i_rollbackDn)
{
	SaAisErrorT result;
	std::string dn;
	TRACE_ENTER();

	TRACE("execute callbackAction");

	result = m_callback.rollback(dn);

	TRACE_LEAVE();

	return result;
}
