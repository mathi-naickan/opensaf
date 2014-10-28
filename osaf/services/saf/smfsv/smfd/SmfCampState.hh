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

#ifndef SMFCAMPSTATE_HH
#define SMFCAMPSTATE_HH

#include <saSmf.h>

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

class SmfUpgradeCampaign;

///
/// Purpose: Base class for all campaign states.
///

class SmfCampState {
 public:

	virtual ~ SmfCampState() {};

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT executeProc(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT rollbackProc(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);

	virtual SaAisErrorT verify(SmfUpgradeCampaign * i_camp, std::string & error);

	virtual SaAisErrorT prerequsitescheck(SmfUpgradeCampaign * i_camp, std::string & error);

	virtual SmfCampResultT suspend(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT commit(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const = 0;

 protected:

	void changeState(SmfUpgradeCampaign * i_camp, SmfCampState * i_state);
};

///
/// Purpose: The initial state of the upgrade campaign.
///

class SmfCampStateInitial:public SmfCampState {
 public:

	static SmfCampState *instance(void);

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT executeInit(SmfUpgradeCampaign * i_camp);

	virtual SaAisErrorT verify(SmfUpgradeCampaign * i_camp, std::string & error);

	virtual SaAisErrorT prerequsitescheck(SmfUpgradeCampaign * i_camp, std::string & error);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_INITIAL;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The executing state of the upgrade campaign.
///

class SmfCampStateExecuting:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT executeProc(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT executeWrapup(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT suspend(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_EXECUTING;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution completed state of the upgrade campaign.
///

class SmfCampStateExecCompleted:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT rollbackWrapup(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT commit(SmfUpgradeCampaign * i_camp);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_EXECUTION_COMPLETED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution suspending state of the upgrade campaign.
///

class SmfCampStateSuspendingExec:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_SUSPENDING_EXECUTION;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution suspended(completed) state of the upgrade campaign.
///

class SmfCampStateExecSuspended:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_EXECUTION_SUSPENDED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution committed state of the upgrade campaign.
///

class SmfCampStateCommitted:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_CAMPAIGN_COMMITTED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution failed state of the upgrade campaign.
///

class SmfCampStateExecFailed:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_EXECUTION_FAILED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The execution error detected state of the upgrade campaign.
///

class SmfCampStateErrorDetected:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ERROR_DETECTED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_ERROR_DETECTED_IN_SUSPENDING state of the upgrade campaign.
///

class SmfCampStateErrorDetectedInSuspending:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ERROR_DETECTED_IN_SUSPENDING;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED state of the upgrade campaign.
///

class SmfCampStateSuspendedByErrorDetected:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT execute(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_SUSPENDED_BY_ERROR_DETECTED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_ROLLING_BACK state of the upgrade campaign.
///

class SmfCampRollingBack:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT rollbackProc(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT rollbackInit(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT suspend(SmfUpgradeCampaign * i_camp);
	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ROLLING_BACK;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_ROLLBACK_SUSPENDED state of the upgrade campaign.
///

class SmfCampRollbackSuspended:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT rollback(SmfUpgradeCampaign * i_camp);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ROLLBACK_SUSPENDED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_SUSPENDING_ROLLBACK state of the upgrade campaign.
///

class SmfCampSuspendingRollback:public SmfCampState {
public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT procResult(SmfUpgradeCampaign *  i_camp,
                                          SmfUpgradeProcedure * i_procedure,
                                          SmfProcResultT        i_result);

	virtual SaSmfCmpgStateT getState() const 
                { return SA_SMF_CMPG_SUSPENDING_ROLLBACK; } 
private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_ROLLBACK_COMPLETED state of the upgrade campaign.
///

class SmfCampRollbackCompleted:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SmfCampResultT commit(SmfUpgradeCampaign * i_camp);

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ROLLBACK_COMPLETED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: The SA_SMF_CMPG_ROLLBACK_COMMITTED state of the upgrade campaign.
///

class SmfCampRollbackCommitted:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ROLLBACK_COMMITTED;
 } private:

	static SmfCampState *s_instance;
};

///
/// Purpose: SA_SMF_CMPG_ROLLBACK_FAILED state of the upgrade campaign.
///

class SmfCampRollbackFailed:public SmfCampState {
 public:

	static SmfCampState *instance();

	virtual void getClassName(std::string & io_str) const;

	virtual void toString(std::string & io_str) const;

	virtual SaSmfCmpgStateT getState() const {
		return SA_SMF_CMPG_ROLLBACK_FAILED;
 } private:

	static SmfCampState *s_instance;
};

#endif				// SMFCAMPSTATE_HH
