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

#ifndef SMF_SMFD_SMFPROCSTATE_H_
#define SMF_SMFD_SMFPROCSTATE_H_

#include "smf/saf/saSmf.h"
#include "smf/smfd/SmfCampaignThread.h"

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

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
class SmfUpgradeProcedure;
class SmfSwapThread;

//================================================================================
// Class SmfProcState
//================================================================================
///
/// Purpose: Base class for all upgrade procedure states.
///
class SmfProcState {
 public:

	virtual ~ SmfProcState() {
	};

	virtual std::string getClassName() const;

	virtual SmfProcResultT execute(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT executeStep(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT rollbackStep(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT suspend(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT commit(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const = 0;

	friend class SmfSwapThread;
 protected:

	void changeState(SmfUpgradeProcedure * i_proc, SmfProcState * i_state);
};

//================================================================================
// Class SmfProcStateInitial
//================================================================================
///
/// Purpose: The initial state of the upgrade procedure.
///
class SmfProcStateInitial:public SmfProcState {
 public:

	static SmfProcState *instance(void);

	virtual std::string getClassName() const;

	virtual SmfProcResultT execute(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT executeInit(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_INITIAL;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateExecuting
//================================================================================
///
/// Purpose: The executing state of the upgrade procedure.
///
class SmfProcStateExecuting:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT execute(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT executeStep(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT executeWrapup(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT suspend(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_EXECUTING;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateExecutionCompleted
//================================================================================
///
/// Purpose: The execution completed state of the upgrade procedure.
///
class SmfProcStateExecutionCompleted:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT rollbackWrapup(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_COMPLETED;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateExecSuspended
//================================================================================
///
/// Purpose: The execution suspended state of the upgrade procedure.
///
class SmfProcStateExecSuspended:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT execute(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT executeStep(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_SUSPENDED;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateExecFailed
//================================================================================
///
/// Purpose: The execution failed state of the upgrade procedure.
///
class SmfProcStateExecFailed:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_FAILED;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateStepUndone
//================================================================================
///
/// Purpose: The step undone state of the upgrade procedure.
///
class SmfProcStateStepUndone:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT execute(SmfUpgradeProcedure * i_proc);

	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_STEP_UNDONE;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateRollingBack
//================================================================================
///
/// Purpose: The rolling back state of the upgrade procedure.
///
class SmfProcStateRollingBack:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT rollbackStep(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT rollbackInit(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT suspend(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_ROLLING_BACK;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateRollbackSuspended
//================================================================================
///
/// Purpose: The rollback suspended state of the upgrade procedure.
///
class SmfProcStateRollbackSuspended:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SmfProcResultT rollbackStep(SmfUpgradeProcedure * i_proc);
	virtual SmfProcResultT rollback(SmfUpgradeProcedure * i_proc);

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_ROLLBACK_SUSPENDED;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateRolledBack
//================================================================================
///
/// Purpose: The rolled back state of the upgrade procedure.
///
class SmfProcStateRolledBack:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

        virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_ROLLED_BACK;
 } private:

	static SmfProcState *s_instance;
};

//================================================================================
// Class SmfProcStateRollbackFailed
//================================================================================
///
/// Purpose: The rollback failed state of the upgrade procedure.
///
class SmfProcStateRollbackFailed:public SmfProcState {
 public:

	static SmfProcState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfProcStateT getState() const {
		return SA_SMF_PROC_ROLLBACK_FAILED;
 } private:

	static SmfProcState *s_instance;
};

#endif  // SMF_SMFD_SMFPROCSTATE_H_
