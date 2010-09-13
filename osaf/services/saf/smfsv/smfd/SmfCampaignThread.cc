/*       OpenSAF
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

#include <poll.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>

#include <ncssysf_def.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tsk.h>
#include <logtrace.h>
#include <rda_papi.h>

#include "SmfCampaignThread.hh"
#include "SmfCampaign.hh"
#include "SmfUpgradeCampaign.hh"
#include "SmfUpgradeProcedure.hh"

SmfCampaignThread *SmfCampaignThread::s_instance = NULL;

#define SMF_RDA_RETRY_COUNT 25 /* This is taken as the csi's are assigned in parallel */

/*====================================================================*/
/*  Data Declarations                                                 */
/*====================================================================*/
extern struct ImmutilWrapperProfile immutilWrapperProfile;

/*====================================================================*/
/*  Class SmfCampaignThread                                           */
/*====================================================================*/

/*====================================================================*/
/*  Static methods                                                    */
/*====================================================================*/

/** 
 * SmfCampaignThread::instance
 * returns the only instance of CampaignThread.
 */
SmfCampaignThread *SmfCampaignThread::instance(void)
{
	return s_instance;
}

/** 
 * SmfCampaignThread::terminate
 * terminates (if started) the only instance of CampaignThread.
 */
void SmfCampaignThread::terminate(void)
{
	if (s_instance != NULL) {
		s_instance->stop();
		/* main will delete the CampaignThread object when terminating */
	}

	return;
}

/** 
 * SmfCampaignThread::start
 * Create the object and start the thread.
 */
int SmfCampaignThread::start(SmfCampaign * campaign)
{
	if (s_instance != NULL) {
		LOG_ER("Campaign thread already exists");
		return -1;
	}

	s_instance = new SmfCampaignThread(campaign);

	if (s_instance->start() != 0) {
		delete s_instance;
		s_instance = NULL;
		return -1;
	}
	return 0;
}

/** 
 * SmfCampaignThread::main
 * static main for the thread
 */
void SmfCampaignThread::main(NCSCONTEXT info)
{
	TRACE_ENTER();
	SmfCampaignThread *self = (SmfCampaignThread *) info;
	self->main();
	TRACE("Campaign thread exits");
	delete self;
	s_instance = NULL;
	TRACE_LEAVE();
}

/*====================================================================*/
/*  Methods                                                           */
/*====================================================================*/

/** 
 * Constructor
 */
 SmfCampaignThread::SmfCampaignThread(SmfCampaign * campaign):
	 m_task_hdl(0),
	 m_mbx(0),
	 m_running(true), 
	 m_campaign(campaign),
	 m_ntfHandle(0),
	 m_tmpSmfUpgradeCampaign()
{
	sem_init(&m_semaphore, 0, 0);
}

/** 
 * Destructor
 */
SmfCampaignThread::~SmfCampaignThread()
{
	TRACE_ENTER();
	SaAisErrorT rc = saNtfFinalize(m_ntfHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("Failed to finalize NTF handle %u", rc);
	}

        //The temporary m_tmpSmfUpgradeCampaign is used because the Campaign
        //object may be deleted (the m_campaign variable become empty), by the campaignThread 
	//cleanup function before the SmfCampaignThread destructor is executed.

	SmfUpgradeCampaign *upgradeCampaign = m_tmpSmfUpgradeCampaign;

	if (upgradeCampaign != NULL) {
		m_campaign->setUpgradeCampaign(NULL);
		delete upgradeCampaign;
	}

	//Delete the IMM handler
	deleteImmHandle();

	TRACE_LEAVE();
}

/** 
 * SmfCampaignThread::start
 * Start the CampaignThread.
 */
int
 SmfCampaignThread::start(void)
{
	uns32 rc;

	TRACE("Starting campaign thread %s", m_campaign->getDn().c_str());

	/* Create the task */
	if ((rc =
	     m_NCS_TASK_CREATE((NCS_OS_CB) SmfCampaignThread::main, (NCSCONTEXT) this, (char*)m_CAMPAIGN_TASKNAME,
			       m_CAMPAIGN_TASK_PRI, m_CAMPAIGN_STACKSIZE, &m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_CREATE_FAILED");
		return -1;
	}

	if ((rc =m_NCS_TASK_DETACH(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_DETACH\n");
		return -1;
	}

	if ((rc = m_NCS_TASK_START(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_FAILED\n");
		return -1;
	}

	/* Wait for the thread to start */
	sem_wait(&m_semaphore);
	return 0;
}

/** 
 * SmfCampaignThread::stop
 * Stop the SmfCampaignThread.
 */
int SmfCampaignThread::stop(void)
{
	TRACE_ENTER();
	TRACE("Stopping campaign thread %s", m_campaign->getDn().c_str());

	/* send a message to the thread to make it terminate */
	CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
	evt->type = CAMPAIGN_EVT_TERMINATE;
	this->send(evt);

	/* Wait for the thread to terminate */
	sem_wait(&m_semaphore);
	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCampaignThread::campaign
 * Returns the Campaign object.
 */
SmfCampaign *SmfCampaignThread::campaign(void)
{
	return m_campaign;
}

/** 
 * SmfCampaignThread::init
 * init the thread.
 */
int SmfCampaignThread::init(void)
{
	TRACE_ENTER();
	uns32 rc;

	/* Create the mailbox used for communication with this thread */
	if ((rc = m_NCS_IPC_CREATE(&m_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		return -1;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&m_mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
		return -1;
	}

	/* Create Imm handle for our runtime objects */
	if ((rc = createImmHandle(m_campaign)) != NCSCC_RC_SUCCESS) {
		LOG_ER("createImmHandle FAILED %d", rc);
		return -1;
	}

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCampaignThread::initNtf
 * init the NTF state notification.
 */
int SmfCampaignThread::initNtf(void)
{
	SaAisErrorT rc = SA_AIS_ERR_TRY_AGAIN;
	SaVersionT ntfVersion = { 'A', 1, 1 };
	unsigned int numOfTries = 50;

	while (rc == SA_AIS_ERR_TRY_AGAIN && numOfTries > 0) {
		rc = saNtfInitialize(&m_ntfHandle, NULL, &ntfVersion);
		if (rc != SA_AIS_ERR_TRY_AGAIN) {
			break;
		}
		usleep(200000);
		numOfTries--;
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("saNtfInitialize FAILED %d", rc);
		return -1;
	}

	return 0;
}

/** 
 * SmfCampaignThread::send
 * send event to the thread.
 */
int SmfCampaignThread::send(CAMPAIGN_EVT * evt)
{
	uns32 rc;

	TRACE("Campaign thread send event type %d", evt->type);
	rc = m_NCS_IPC_SEND(&m_mbx, (NCSCONTEXT) evt, NCS_IPC_PRIORITY_HIGH);

	return rc;
}

/** 
 * SmfCampaignThread::createImmHandle
 * Creates Imm handle for our runtime objects.
 */
SaAisErrorT 
SmfCampaignThread::createImmHandle(SmfCampaign *i_campaign)
{
	SaAisErrorT rc = SA_AIS_OK;
	int existCnt = 0;

	SaVersionT immVersion = { 'A', 2, 1 };
        const char *campDn = i_campaign->getDn().c_str();

	TRACE_ENTER();

	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOiInitialize_2(&m_campOiHandle, NULL, &immVersion);
	while (rc == SA_AIS_ERR_TRY_AGAIN) {
		sleep(1);
		rc = immutil_saImmOiInitialize_2(&m_campOiHandle, NULL, &immVersion);	
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiInitialize_2 fails rc=%d", rc);
		goto done;
	}

	TRACE("saImmOiImplementerSet DN=%s", campDn);

	//SA_AIS_ERR_TRY_AGAIN can proceed forever
	//SA_AIS_ERR_EXIST is limited to 20 seconds (for the other side to release the handle)
	rc = immutil_saImmOiImplementerSet(m_campOiHandle, (char *)campDn);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) || (rc == SA_AIS_ERR_EXIST)) {
		if(rc == SA_AIS_ERR_EXIST) 
			existCnt++;
		if(existCnt > 20) {
			TRACE("immutil_saImmOiImplementerSet rc = SA_AIS_ERR_EXIST for 20 sec, giving up ");
			goto done;
		}

		TRACE("immutil_saImmOiImplementerSet rc = %d, wait 1 sec and retry", rc);
		sleep(1);
		rc = immutil_saImmOiImplementerSet(m_campOiHandle, (char *)campDn);	
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet for DN=%s fails rc=%d", campDn, rc);
		goto done;
	}

	done:
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
	TRACE_LEAVE();
	return rc;
}

/** 
 * SmfCampaignThread::deleteImmHandle
 * Deletes Imm handle for our runtime objects.
 */
SaAisErrorT 
SmfCampaignThread::deleteImmHandle()
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOiImplementerClear(m_campOiHandle);
	while (rc == SA_AIS_ERR_TRY_AGAIN) {
		sleep(1);
		rc = immutil_saImmOiImplementerClear(m_campOiHandle);	
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("SmfCampaignThread::deleteImmHandle:saImmOiImplementerClear fails rc=%d", rc);
		goto done;
	}

	rc = immutil_saImmOiFinalize(m_campOiHandle);
	while (rc == SA_AIS_ERR_TRY_AGAIN) {
		sleep(1);
		rc = immutil_saImmOiFinalize(m_campOiHandle);	
	}

	if (rc != SA_AIS_OK) {
		LOG_ER("SmfCampaignThread::deleteImmHandle:saImmOiFinalize fails rc=%d", rc);
	}

	done:
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
	TRACE_LEAVE();
	return rc;
}

/** 
 * SmfCampaignThread::getImmHandle
 * Get the Imm handle for campaign runtime objects.
 */
SaImmOiHandleT 
SmfCampaignThread::getImmHandle()
{
	return m_campOiHandle;
}

/** 
 * SmfCampaignThread::sendStateNotification
 * send state change notification to NTF.
 */
int SmfCampaignThread::sendStateNotification(const std::string & dn, uns32 classId, SaNtfSourceIndicatorT sourceInd,
					     uns32 stateId, uns32 newState)
{
	SaAisErrorT rc;
	int result = 0;
	SaNtfStateChangeNotificationT ntfStateNot;

	rc = saNtfStateChangeNotificationAllocate(m_ntfHandle, &ntfStateNot, 1,	/* numCorrelated Notifications */
						  0,	/* length addition text */
						  0,	/* num additional info */
						  1,	/* num of state changes */
						  0);	/* variable data size */
	if (rc != SA_AIS_OK) {
		LOG_ER("saNtfStateChangeNotificationAllocate FAILED %d", rc);
		return -1;
	}

	/* Notifying object */
	SaUint16T length = sizeof(SMF_NOTIFYING_OBJECT);
	if (length >= SA_MAX_NAME_LENGTH) {
		TRACE("notifyingObject length was %d, truncated to 256", length);
		length = 255;
	}
	ntfStateNot.notificationHeader.notifyingObject->length = length - 1;
	strncpy((char *)ntfStateNot.notificationHeader.notifyingObject->value, SMF_NOTIFYING_OBJECT, length);

	/* Notification object */
	length = dn.length();
	if (length >= SA_MAX_NAME_LENGTH) {
		TRACE("notificationHeader length was %d, truncated to 256", length);
		length = 255;
	}
	ntfStateNot.notificationHeader.notificationObject->length = length;
	strncpy((char *)ntfStateNot.notificationHeader.notificationObject->value, dn.c_str(), length);

	/* Event type */
	*(ntfStateNot.notificationHeader.eventType) = SA_NTF_OBJECT_STATE_CHANGE;

	/* event time to be set automatically to current
	   time by saNtfNotificationSend */
	*(ntfStateNot.notificationHeader.eventTime) = (SaTimeT) SA_TIME_UNKNOWN;

	/* set Notification Class Identifier */
	ntfStateNot.notificationHeader.notificationClassId->vendorId = SA_NTF_VENDOR_ID_SAF;
	ntfStateNot.notificationHeader.notificationClassId->majorId = SA_SVC_SMF;
	ntfStateNot.notificationHeader.notificationClassId->minorId = classId;

	/* Set source indicator */
	*(ntfStateNot.sourceIndicator) = sourceInd;

	/* Set state changed */
	ntfStateNot.changedStates[0].stateId = stateId;
	ntfStateNot.changedStates[0].newState = newState;
	ntfStateNot.changedStates[0].oldStatePresent = SA_FALSE;

	/* Send the notification */
	rc = saNtfNotificationSend(ntfStateNot.notificationHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("saNtfNotificationSend FAILED %d", rc);
		result = -1;
		goto done;
	}

 done:
	rc = saNtfNotificationFree(ntfStateNot.notificationHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("saNtfNotificationFree FAILED %d", rc);
	}
	return result;
}

/** 
 * SmfCampaignThread::processEvt
 * process events in the mailbox.
 */
void SmfCampaignThread::processEvt(void)
{
	CAMPAIGN_EVT *evt;

	evt = (CAMPAIGN_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&m_mbx, evt);
	if (evt != NULL) {
		TRACE("Campaign thread received event type %d", evt->type);

		switch (evt->type) {
		case CAMPAIGN_EVT_TERMINATE:
			{
				// Save a pointer to the SmfUpgradeCampaign since
				// this pointer will be removed by the main process
				// after ordering campaign thread terminate
				m_tmpSmfUpgradeCampaign = m_campaign->getUpgradeCampaign();
				
				/* */
				m_running = false;
				break;
			}

		case CAMPAIGN_EVT_EXECUTE:
			{
				m_campaign->adminOpExecute();
				break;
			}

		case CAMPAIGN_EVT_SUSPEND:
			{
				m_campaign->adminOpSuspend();
				break;
			}

		case CAMPAIGN_EVT_COMMIT:
			{
				m_campaign->adminOpCommit();
				break;
			}

		case CAMPAIGN_EVT_ROLLBACK:
			{
				m_campaign->adminOpRollback();
				break;
			}

		case CAMPAIGN_EVT_EXECUTE_INIT:
			{
				m_campaign->getUpgradeCampaign()->executeInit();
				break;
			}

		case CAMPAIGN_EVT_EXECUTE_PROC:
			{
				m_campaign->getUpgradeCampaign()->executeProc();
				break;
			}

		case CAMPAIGN_EVT_EXECUTE_WRAPUP:
			{
				m_campaign->getUpgradeCampaign()->executeWrapup();
				break;
			}

		case CAMPAIGN_EVT_PROCEDURE_RC:
			{
				TRACE("Procedure result from %s = %u",
				      evt->event.procResult.procedure->getProcName().c_str(), evt->event.procResult.rc);
                                /* TODO We need to send the procedure result to the state machine
                                   so we can see what response this is. This is needed for suspend 
                                   where we can receive an execute response in the middle of the suspend
                                   responses if a procedure just finished when we sent the suspend to it. */
				m_campaign->getUpgradeCampaign()->executeProc();
				break;
			}

		default:
			{
				LOG_ER("unknown event received %d", evt->type);
			}
		}

		delete(evt);
	}
}

/** 
 * SmfCampaignThread::handleEvents
 * handle incoming events to the thread.
 */
int SmfCampaignThread::handleEvents(void)
{
	TRACE_ENTER();
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[1];

	mbx_fd = ncs_ipc_get_sel_obj(&m_mbx);

	/* Set up all file descriptors to listen to */
	fds[0].fd = mbx_fd.rmv_obj;
	fds[0].events = POLLIN;

	TRACE("Campaign thread %s waiting for events", m_campaign->getDn().c_str());

	while (m_running) {
		int ret = poll(fds, 1, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		/* Process the Mail box events */
		if (fds[0].revents & POLLIN) {
			/* dispatch MBX events */
			processEvt();
		}
	}
	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCampaignThread::main
 * main for the thread.
 */
void SmfCampaignThread::main(void)
{
	TRACE_ENTER();
	if (this->init() == 0) {
		uns32 rc = NCSCC_RC_SUCCESS; 
		unsigned int numOfTries = SMF_RDA_RETRY_COUNT; 
		SaAmfHAStateT smfd_rda_role = SA_AMF_HA_STANDBY;

		/* Mark the thread started */
		sem_post(&m_semaphore);

		if (initNtf() != 0) {
			LOG_ER("initNtf failed");
		}
	
		while (numOfTries > 0) {
			/* Rda get Role */
			if ((rc = rda_get_role(&smfd_rda_role)) != NCSCC_RC_SUCCESS) {
				LOG_ER("rda_get_role FAILED in SmfCampaignThread::main ,retrying to get the RDA role");
			} 

			if (smfd_rda_role == SA_AMF_HA_ACTIVE) {
				LOG_NO("rda_get_role: Active");
				break;
			}
			sleep(1);
			numOfTries--;
		}
		if ( (numOfTries == 0) && (smfd_rda_role != SA_AMF_HA_ACTIVE)) {
			LOG_ER("Failed to get Active RDA role");
		}

	} else {
		sem_post(&m_semaphore);
		LOG_ER("init failed");
		return;
	}
	this->handleEvents();	/* runs forever until stopped */
	/* Mark the thread terminated */
	sem_post(&m_semaphore);

	TRACE_LEAVE();
}
