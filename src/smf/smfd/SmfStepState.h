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

#ifndef SMF_SMFD_SMFSTEPSTATE_H_
#define SMF_SMFD_SMFSTEPSTATE_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

typedef enum {
	SMF_STEP_NULL = 1,     /* Nothing done */
	SMF_STEP_CONTINUE = 2, /* Continue step in new state */
	SMF_STEP_SWITCHOVER = 3,
	SMF_STEP_COMPLETED = 4,
	SMF_STEP_FAILED = 5,
	SMF_STEP_UNDONE = 6,
	SMF_STEP_ROLLEDBACK = 7,
	SMF_STEP_ROLLBACKFAILED = 8,
	SMF_STEP_ROLLBACKUNDONE = 9
} SmfStepResultT;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

class SmfUpgradeStep;

//================================================================================
// Class SmfStepState
//================================================================================
///
/// Purpose: Base class for all procedure steps.
///
class SmfStepState {
public:

        virtual ~ SmfStepState() {};

        virtual std::string getClassName() const;

	virtual SmfStepResultT execute(SmfUpgradeStep * i_step);
	virtual SmfStepResultT rollback(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const = 0;

protected:

	void changeState(SmfUpgradeStep * i_step, SmfStepState * i_state);
};

//================================================================================
// Class SmfStepStateInitial
//================================================================================
///
/// Purpose: Initial state for the procedure step..
///
class SmfStepStateInitial : public SmfStepState {
public:

	static SmfStepState *instance(void);

	virtual std::string getClassName() const;

	virtual SmfStepResultT execute(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_INITIAL; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateExecuting
//================================================================================
///
/// Purpose: Executing state for the procedure step..
///
class SmfStepStateExecuting : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT execute(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_EXECUTING; } 
private:

        static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateCompleted
//================================================================================
///
/// Purpose: Execution completed state for the procedure step..
///
class SmfStepStateCompleted : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT rollback(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_COMPLETED; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateFailed
//================================================================================
///
/// Purpose: Execution failed state for the procedure step..
///
class SmfStepStateFailed : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_FAILED; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateUndoing
//================================================================================
///
/// Purpose: Execution undoing state for the procedure step..
///
class SmfStepStateUndoing : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT execute(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_UNDOING; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateUndone
//================================================================================
///
/// Purpose: Execution undone state for the procedure step..
///
class SmfStepStateUndone : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT execute(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_UNDONE; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateRollingBack
//================================================================================
///
/// Purpose: Execution rolling back state for the procedure step..
///
class SmfStepStateRollingBack : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT rollback(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_ROLLING_BACK; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateRolledBack
//================================================================================
///
/// Purpose: Execution rolled back state for the procedure step..
///
class SmfStepStateRolledBack : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_ROLLED_BACK; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateUndoingRollback
//================================================================================
///
/// Purpose: Execution undoing rollback state for the procedure step..
///
class SmfStepStateUndoingRollback : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SmfStepResultT rollback(SmfUpgradeStep * i_step);

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_UNDOING_ROLLBACK; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateRollbackUndone
//================================================================================
///
/// Purpose: Execution rollback undone state for the procedure step..
///
class SmfStepStateRollbackUndone : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_ROLLBACK_UNDONE; } 
private:

	static SmfStepState *s_instance;
};

//================================================================================
// Class SmfStepStateRollbackFailed
//================================================================================
///
/// Purpose: Execution rollback failed state for the procedure step..
///
class SmfStepStateRollbackFailed : public SmfStepState {
public:

	static SmfStepState *instance();

	virtual std::string getClassName() const;

	virtual SaSmfStepStateT getState() const 
                { return SA_SMF_STEP_ROLLBACK_FAILED; } 
private:

	static SmfStepState *s_instance;
};

#endif  // SMF_SMFD_SMFSTEPSTATE_H_
