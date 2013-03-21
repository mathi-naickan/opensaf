/*      -- OpenSAF  --
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s): GoAhead Software
 *
 */

#ifndef _SMF_CALLBACK_HH_
#define _SMF_CALLBACK_HH_

#include <ncsgl_defs.h>
#include <ncssysf_ipc.h>
#include <saSmf.h>

#include "SmfCampaign.hh"
//#include "SmfUpgradeStep.hh"
//#include "SmfUpgradeProcedure.hh"


class SmfCallback;
class SmfUpgradeStep;
class SmfUpgradeProcedure;
class SmfCampaignThread;
class SmfProcedureThread;

class SmfCallback {
public:
	SmfCallback(): 
		m_stepCount(onEveryStep),
		m_atAction(beforeLock),
		m_callbackLabel(),
		m_time(0),
		m_stringToPass()
		{};

	~SmfCallback(){};

	// (the stepCount is only relevant for rolling-upgrade)
	enum StepCountT {
		onEveryStep,
		onFirstStep,
		onLastStep,
		halfWay
	};

	enum AtActionT {
		beforeLock,
		beforeTermination,
		afterImmModification,
		afterInstantiation,
		afterUnlock,
		atCampInit,
		atCampVerify,
		atAdminVerify,
		atCampBackup,
		atCampRollback,
		atCampCommit,
		atCampInitAction,
		atCampWrapupAction,
		atCampCompleteAction,
		atProcInitAction,
		atProcWrapupAction
	};

	StepCountT getStepCount() const {
		return m_stepCount;
	};
	AtActionT getAtAction() const {
		return m_atAction;
	};
	std::string getCallbackLabel() const {
		return m_callbackLabel;
	};
	SaTimeT getTime() const {
		return m_time;
	};
	std::string getStringToPass() const {
		return m_stringToPass;
	};
	void setProcedure (SmfUpgradeProcedure *proc) { m_procedure = proc; }
	///
	/// Purpose: Execute the callback. It hides all the communication with smfnds, does not 
	/// return until all responses have been received or timeout occurs
	/// @param   None.
	/// @return  0 on success, otherwise failure.
	///
	SaAisErrorT execute(std::string & step_dn);

	///
	/// Purpose: Rollback the callback. It hides all the communication with smfnds, does not 
	/// return until all responses have been received or timeout occurs

	/// @param   None.
	/// @return  0 on success, otherwise failure.
	///
        SaAisErrorT rollback(std::string & step_dn);

	SaAisErrorT send_callback_msg (SaSmfPhaseT, std::string & step_dn);

private:
	friend class SmfCampaignXmlParser;
	friend class SmfCampStateInitial;
	StepCountT m_stepCount;
	AtActionT m_atAction;
	std::string m_callbackLabel;
	SaTimeT m_time;
	std::string m_stringToPass;
	SmfUpgradeProcedure *m_procedure; // pointer to the procedure object thta this is part of 
};


#endif
