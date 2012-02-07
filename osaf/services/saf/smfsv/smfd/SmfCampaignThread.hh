/*      -- OpenSAF  --
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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

#ifndef _SmfCampaignThread_hh_
#define _SmfCampaignThread_hh_

#include <semaphore.h>
#include <ncsgl_defs.h>
#include <ncssysf_ipc.h>
#include <saNtf.h>
#include <string>
#include <saImmOi.h>


class SmfUpgradeProcedure;

/* TODO: This needs to be handled somewhere else */
#define SMF_NOTIFYING_OBJECT "safApp=safSmfService"
#define MINOR_ID_CAMPAIGN 0x65
#define MINOR_ID_PROCEDURE 0x66
#define MINOR_ID_STEP 0x67

/**** Macro for CAMPAIGN task name ****/
#define m_CAMPAIGN_TASKNAME "CAMPAIGN"

/**** Macro for CAMPAIGN task stack size ****/
#define m_CAMPAIGN_STACKSIZE NCS_STACKSIZE_HUGE

/* Procedure result enums */
typedef enum {
	SMF_PROC_DONE = 1,
	SMF_PROC_CONTINUE = 2,
	SMF_PROC_FAILED = 3,
	SMF_PROC_STEPUNDONE = 4,
	SMF_PROC_SUSPENDED = 5,
	SMF_PROC_COMPLETED = 6,
	SMF_PROC_ROLLBACKFAILED = 7,
	SMF_PROC_ROLLEDBACK = 8,
	SMF_PROC_MAX
} SmfProcResultT;

/* CampaignThread event enums */
typedef enum {
	CAMPAIGN_EVT_TERMINATE = 1,
	CAMPAIGN_EVT_CONTINUE = 2,
	CAMPAIGN_EVT_EXECUTE = 3,
	CAMPAIGN_EVT_SUSPEND = 4,
	CAMPAIGN_EVT_COMMIT = 5,
	CAMPAIGN_EVT_ROLLBACK = 6,
	CAMPAIGN_EVT_EXECUTE_PROC = 7,
	CAMPAIGN_EVT_ROLLBACK_PROC = 8,
	CAMPAIGN_EVT_PROCEDURE_RC = 9,
	CAMPAIGN_EVT_MAX
} CAMPAIGN_EVT_TYPE;

/*** CAMPAIGN event definitions ***/
typedef struct {
	uint32_t dummy;
} campaign_evt_terminate;

typedef struct {
	uint32_t dummy;
} campaign_evt_execute;

typedef struct {
	uint32_t dummy;
} campaign_evt_execute_init;

typedef struct {
	uint32_t dummy;
} campaign_evt_execute_proc;

typedef struct {
	uint32_t dummy;
} campaign_evt_execute_wrapup;

typedef struct {
	uint32_t dummy;
} campaign_evt_suspend;

typedef struct {
	uint32_t dummy;
} campaign_evt_commit;

typedef struct {
	uint32_t dummy;
} campaign_evt_rollback;

typedef struct {
	SmfUpgradeProcedure *procedure;
	SmfProcResultT rc;
} campaign_evt_procedure_result;

typedef struct {
	void *next;		/* needed by mailbox send/receive */
	CAMPAIGN_EVT_TYPE type;	/* evt type */
	union {
		campaign_evt_terminate terminate;
		campaign_evt_execute execute;
		campaign_evt_execute_init executeInit;
		campaign_evt_execute_proc executeProc;
		campaign_evt_execute_wrapup executeWrapup;
		campaign_evt_suspend suspend;
		campaign_evt_commit commit;
		campaign_evt_rollback rollback;
		campaign_evt_procedure_result procResult;
	} event;
} CAMPAIGN_EVT;

class SmfCampaign;
class SmfUpgradeCampaign;

///
/// Class for the SMF campaign thread. This is a singleton.
///

class SmfCampaignThread {
 public:
	static SmfCampaignThread *instance(void);

///
/// Purpose: Starts the campaign thread
/// @param   campaign A pointer to the campaign object to run..
/// @return  An interger returning 0 on success.
///
	static int start(SmfCampaign * campaign);

///
/// Purpose: Terminate the campaign thread
/// @param   None.
/// @return  None.
///
	static void terminate(void);

///
/// Purpose: Send an event to the campaign thread
/// @param   evt A pointer to a campaign event.
/// @return  An interger returning 0 on success.
///
	int send(CAMPAIGN_EVT * evt);

///
/// Purpose: Get the pointer to the campaing currently run by the thread.
/// @param   None.
/// @return  A pointer to a SmfCampaign object.
///
	SmfCampaign *campaign(void);

///
/// Purpose: Send a state change notification
/// @param   dn The DN of the object which state is changed.
/// @param   classId The notification class Id.
/// @param   sourceInd The of the notification.
/// @param   stateId The identfication id of the state (SA_SMF_PROCEDURE_STATE).
/// @param   newState The new state (SaSmfProcStateT).
/// @return  An interger returning 0 on success.
///
	int sendStateNotification(const std::string & dn, uint32_t classId, SaNtfSourceIndicatorT sourceInd, uint32_t stateId,
				  uint32_t newState, uint32_t oldState);

	SaImmOiHandleT getImmHandle();
	SYSF_MBX & getCbkMbx();

 private:
	 SmfCampaignThread(SmfCampaign * campaign);
	~SmfCampaignThread();

	int start(void);
	int stop(void);
	void main(void);
	int init(void);
	int initNtf(void);
	int handleEvents(void);
	void processEvt(void);

	SaAisErrorT createImmHandle(SmfCampaign * i_campaign);
	SaAisErrorT deleteImmHandle();

	static void main(NCSCONTEXT info);
	static SmfCampaignThread *s_instance;

	NCSCONTEXT m_task_hdl;
	SYSF_MBX m_mbx;		/* mailbox */
	SYSF_MBX m_cbkMbx;	/* mailbox for send/receive callback/response */
	bool m_running;
	SmfCampaign *m_campaign;
	sem_t* m_semaphore;
	SaNtfHandleT m_ntfHandle;
	SaImmOiHandleT m_campOiHandle;	/* IMM OI handle */
	SmfUpgradeCampaign * m_tmpSmfUpgradeCampaign; /*Used at campaign termination */
};

#endif
