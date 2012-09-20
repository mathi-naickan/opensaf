/*      - OpenSAF  -
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

#include "SmfProcedureThread.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfUpgradeStep.hh"
#include "SmfCampaignThread.hh"
#include "SmfCampaign.hh"
#include "SmfUtils.hh"

#include <poll.h>
#include <sched.h>

#include <ncssysf_def.h>
#include <ncssysf_ipc.h>
#include <ncssysf_tsk.h>
#include <logtrace.h>
#include <saImmOm.h>
#include <saImmOi.h>
#include <immutil.h>

/*====================================================================*/
/*  Data Declarations                                                 */
/*====================================================================*/

/*====================================================================*/
/*  Class SmfProcedureThread                                          */
/*====================================================================*/

/*====================================================================*/
/*  Static methods                                                    */
/*====================================================================*/

/** 
 * SmfProcedureThread::main
 * static main for the thread
 */
void
SmfProcedureThread::main(NCSCONTEXT info)
{
	SmfProcedureThread *self = (SmfProcedureThread *) info;
	self->main();
	TRACE("Procedure thread exits");
	delete self; 
}

/*====================================================================*/
/*  Methods                                                           */
/*====================================================================*/

/** 
 * Constructor
 */
SmfProcedureThread::SmfProcedureThread(SmfUpgradeProcedure * procedure):
	m_task_hdl(0),
	m_mbx(0),
	m_running(true), 
	m_procedure(procedure),
	m_semaphore(NULL)
{
}

/** 
 * Destructor
 */
SmfProcedureThread::~SmfProcedureThread()
{
	TRACE_ENTER();

	/* IPC cleanup */
	m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
	m_NCS_IPC_RELEASE(&m_mbx, NULL);
	m_NCS_IPC_DETACH(&m_cbk_mbx, NULL, NULL);
	m_NCS_IPC_RELEASE(&m_cbk_mbx, NULL);

	TRACE_LEAVE();
}

/**
 * SmfProcedureThread::start
 * Start the SmfProcedureThread.
 */
int
SmfProcedureThread::start(void)
{
	uint32_t rc;

	TRACE("Starting procedure thread %s", m_procedure->getDn().c_str());

	sem_t localSemaphore;
	sem_init(&localSemaphore, 0, 0);
	m_semaphore = &localSemaphore;

	/* Create the task */
	int policy = SCHED_OTHER; /*root defaults */
	int prio_val = sched_get_priority_min(policy);
	
	if ((rc =
	     m_NCS_TASK_CREATE((NCS_OS_CB) SmfProcedureThread::main, (NCSCONTEXT) this, (char *)"OSAF_SMF_PROC",
			       prio_val, policy, m_PROCEDURE_STACKSIZE, &m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_CREATE_FAILED");
		m_semaphore = NULL;
		sem_destroy(&localSemaphore);
		return -1;
	}

	if ((rc =m_NCS_TASK_DETACH(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_DETACH\n");
		m_semaphore = NULL;
		sem_destroy(&localSemaphore);
		return -1;
	}

	if ((rc = m_NCS_TASK_START(m_task_hdl)) != NCSCC_RC_SUCCESS) {
		LOG_ER("TASK_START_FAILED\n");
		m_semaphore = NULL;
		sem_destroy(&localSemaphore);
		return -1;
	}

	/* Wait for the thread to start */
	while((sem_wait(&localSemaphore) == -1) && (errno == EINTR))
               continue;       /* Restart if interrupted by handler */

	m_semaphore = NULL;
	sem_destroy(&localSemaphore);

	return 0;
}

/** 
 * SmfProcedureThread::stop
 * Stop the SmfProcedureThread.
 */
int 
SmfProcedureThread::stop(void)
{
	TRACE_ENTER();
	TRACE("Stopping procedure thread %s", m_procedure->getDn().c_str());

	sem_t localSemaphore;
	sem_init(&localSemaphore, 0, 0);
	m_semaphore = &localSemaphore;

	/* send a message to the thread to make it terminate */
	PROCEDURE_EVT *evt = new PROCEDURE_EVT();
	evt->type = PROCEDURE_EVT_TERMINATE;
	this->send(evt);

	/* Wait for the thread to terminate */
	while((sem_wait(&localSemaphore) == -1) && (errno == EINTR))
               continue;       /* Restart if interrupted by handler */

        //m_semaphore can not be written here since the thread may have already deleted the SmfProcedureThread object
	//m_semaphore = NULL;
	sem_destroy(&localSemaphore);

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfProcedureThread::init
 * init the thread.
 */
int 
SmfProcedureThread::init(void)
{
	uint32_t rc;
	SaAisErrorT result = SA_AIS_OK;

	/* Create the mailbox used for communication with this thread */
	if ((rc = m_NCS_IPC_CREATE(&m_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		return -1;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&m_mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
		m_NCS_IPC_RELEASE(&m_mbx, NULL);
		return -1;
	}

	/* Create the mailbox used for callback communication */
	if ((rc = m_NCS_IPC_CREATE(&m_cbk_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED %d", rc);
		m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
		m_NCS_IPC_RELEASE(&m_mbx, NULL);
		return -1;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&m_cbk_mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED %d", rc);
		m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
		m_NCS_IPC_RELEASE(&m_mbx, NULL);
		m_NCS_IPC_RELEASE(&m_cbk_mbx, NULL);
		return -1;
	}

	/* Check if our Imm runtime object already exists (switchover or restart occured) */
	result = getImmProcedure(m_procedure);
	if (result == SA_AIS_ERR_NOT_EXIST) {
		/* Create our Imm runtime object */
		if ((result = createImmProcedure(m_procedure)) != SA_AIS_OK) {
			LOG_ER("createImmProcedure FAILED %d", result);
			m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
			m_NCS_IPC_RELEASE(&m_mbx, NULL);
			m_NCS_IPC_DETACH(&m_cbk_mbx, NULL, NULL);
			m_NCS_IPC_RELEASE(&m_cbk_mbx, NULL);
			return -1;
		}
	} else if (result == SA_AIS_OK) {
                if (m_procedure->getState() != SA_SMF_PROC_INITIAL) {
        		/* Procedure exists and it has been started at some time, get step data */
        		result = m_procedure->getImmSteps();
        		if (result != SA_AIS_OK) {
        			LOG_ER("getImmSteps FAILED %d", result);
				m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
				m_NCS_IPC_RELEASE(&m_mbx, NULL);
				m_NCS_IPC_DETACH(&m_cbk_mbx, NULL, NULL);
				m_NCS_IPC_RELEASE(&m_cbk_mbx, NULL);
        			return -1;
        		}
                }
	} else {
		LOG_ER("getImmProcedure FAILED %d", result);
		m_NCS_IPC_DETACH(&m_mbx, NULL, NULL);
		m_NCS_IPC_RELEASE(&m_mbx, NULL);
		m_NCS_IPC_DETACH(&m_cbk_mbx, NULL, NULL);
		m_NCS_IPC_RELEASE(&m_cbk_mbx, NULL);
		return -1;
	}

	return 0;
}

/** 
 * SmfProcedureThread::send
 * send event to the thread.
 */
int 
SmfProcedureThread::send(PROCEDURE_EVT * evt)
{
	uint32_t rc;

	//Save the mailbox pointer since SmfProcedureThread object may be deleted 
	//when SmfProcedureThread member variable m_mbx is used in m_NCS_IPC_SEND
	SYSF_MBX tmp_mbx = m_mbx;
	
	TRACE("Procedure thread send event type %d", evt->type);
	rc = m_NCS_IPC_SEND(&tmp_mbx, (NCSCONTEXT) evt, NCS_IPC_PRIORITY_HIGH);
	return rc;
}

/** 
 * SmfProcedureThread::getImmHandle
 * Get the Imm handle for procedure runtime objects.
 */
SaImmOiHandleT 
SmfProcedureThread::getImmHandle()
{
        /* Use the same handle in all threads for rollback to work */
        return SmfCampaignThread::instance()->getImmHandle();
}

/** 
 * SmfProcedureThread::getCbkMbx
 * Get the cbk mbx.
 */
SYSF_MBX & 
SmfProcedureThread::getCbkMbx()
{
	return m_cbk_mbx;
}

/** 
 * SmfProcedureThread::getImmProcedure
 * Get our Imm procedure objects (if exists).
 */
SaAisErrorT 
SmfProcedureThread::getImmProcedure(SmfUpgradeProcedure * procedure)
{
	SaAisErrorT rc = SA_AIS_OK;
	SmfImmUtils immutil;
	SaImmAttrValuesT_2 **attributes;
	std::list < std::string > stepList;

	TRACE_ENTER();

	TRACE("Get IMM data for %s", procedure->getDn().c_str());

	if (immutil.getObject(procedure->getDn(), &attributes) == false) {
		LOG_NO("IMM data for procedure %s not found", procedure->getDn().c_str());
		rc = SA_AIS_ERR_NOT_EXIST;
		goto done;
	}

	rc = procedure->init((const SaImmAttrValuesT_2 **)attributes);
	if (rc != SA_AIS_OK) {
		LOG_ER("Initialization failed for procedure %s", procedure->getDn().c_str());
		rc = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/** 
 * SmfProcedureThread::createImmProcedure
 * Create our Imm runtime object.
 */
SaAisErrorT 
SmfProcedureThread::createImmProcedure(SmfUpgradeProcedure * procedure)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT parentName;
	SmfCampaign *campaign = SmfCampaignThread::instance()->campaign();

	const char *safSmfProcedure = procedure->getProcName().c_str();
	uint32_t saSmfProcExecLevel = procedure->getExecLevel();
	SaNameT saSmfProcMustKeepSIs = { 0 };
	SaNameT saSmfProcAcceptSIOutage = { 0 };
	uint32_t saSmfProcMaxNumSIsOutage = 0;
	uint32_t saSmfProcUpgrMethod = 0;

//TODO: Parse and read the saSmfProcDisableSimultanExec attribute from the campaign.xml
//      Handle the the content i.e. do not execute in parallel 
	uint32_t saSmfProcDisableSimultanExec = 1;
	SaTimeT saSmfProcPeriod = 0;
	uint32_t saSmfProcState = SA_SMF_PROC_INITIAL;
	char *saSmfProcError = (char*)"";

	TRACE_ENTER();

	void *arr1[] = { &safSmfProcedure };
	const SaImmAttrValuesT_2 attr_safSmfProc = {
		(char*)"safSmfProcedure",
		SA_IMM_ATTR_SASTRINGT,
		1,
		arr1
	};

	void *arr2[] = { &saSmfProcExecLevel };
	const SaImmAttrValuesT_2 attr_saSmfProcExecLevel = {
		(char*)"saSmfProcExecLevel",
		SA_IMM_ATTR_SAUINT32T,
		1,
		arr2
	};

	void *arr3[] = { &saSmfProcMustKeepSIs };
	const SaImmAttrValuesT_2 attr_saSmfProcMustKeepSIs = {
		(char*)"saSmfProcMustKeepSIs",
		SA_IMM_ATTR_SANAMET,
		1,
		arr3
	};

	void *arr4[] = { &saSmfProcAcceptSIOutage };
	const SaImmAttrValuesT_2 attr_saSmfProcAcceptSIOutage = {
		(char*)"saSmfProcAcceptSIOutage",
		SA_IMM_ATTR_SANAMET,
		1,
		arr4
	};

	void *arr5[] = { &saSmfProcMaxNumSIsOutage };
	const SaImmAttrValuesT_2 attr_saSmfProcMaxNumSIsOutage = {
		(char*)"saSmfProcMaxNumSIsOutage",
		SA_IMM_ATTR_SAUINT32T,
		1,
		arr5
	};

	void *arr6[] = { &saSmfProcUpgrMethod };
	const SaImmAttrValuesT_2 attr_saSmfProcUpgrMethod = {
		(char*)"saSmfProcUpgrMethod",
		SA_IMM_ATTR_SAUINT32T,
		1,
		arr6
	};

	void *arr7[] = { & saSmfProcDisableSimultanExec};
	const SaImmAttrValuesT_2 attr_saSmfProcDisableSimultanExec = {
		(char*)"saSmfProcDisableSimultanExec",
		SA_IMM_ATTR_SAUINT32T,
		1,
		arr7
	};

	void *arr8[] = { &saSmfProcPeriod };
	const SaImmAttrValuesT_2 attr_saSmfProcPeriod = {
		(char*)"saSmfProcPeriod",
		SA_IMM_ATTR_SATIMET,
		1,
		arr8
	};

	void *arr9[] = { &saSmfProcState };
	const SaImmAttrValuesT_2 attr_saSmfProcState = {
		(char*)"saSmfProcState",
		SA_IMM_ATTR_SAUINT32T,
		1,
		arr9
	};

	void *arr10[] = { &saSmfProcError };
	const SaImmAttrValuesT_2 attr_saSmfProcError = {
		(char*)"saSmfProcError",
		SA_IMM_ATTR_SASTRINGT,
		1,
		arr10
	};

	const SaImmAttrValuesT_2 *attrValues[] = {
		&attr_safSmfProc,
		&attr_saSmfProcExecLevel,
		&attr_saSmfProcMustKeepSIs,
		&attr_saSmfProcAcceptSIOutage,
		&attr_saSmfProcMaxNumSIsOutage,
		&attr_saSmfProcUpgrMethod,
		&attr_saSmfProcDisableSimultanExec,
		&attr_saSmfProcPeriod,
		&attr_saSmfProcState,
		&attr_saSmfProcError,
		NULL
	};

	parentName.length = campaign->getDn().length();
	strncpy((char *)parentName.value, campaign->getDn().c_str(), parentName.length);
	parentName.value[parentName.length] = 0;

	rc = immutil_saImmOiRtObjectCreate_2(getImmHandle(), (char*)"SaSmfProcedure", &parentName, attrValues);

	if (rc != SA_AIS_OK) {
		TRACE("saImmOiRtObjectCreate_2 returned %u for %s, parent %s", rc, procedure->getProcName().c_str(),
		      parentName.value);
		goto done;
	}

 done:
	TRACE_LEAVE();
	return rc;
}

/**
 * SmfProcedureThread::updateImmAttr
 * Updates a runtime attribute in the IMM
 */
int 
SmfProcedureThread::updateImmAttr(const char *dn, SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType,
				      void *value)
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE_ENTER();
	rc = immutil_update_one_rattr(getImmHandle(), dn, attributeName, attrValueType, value);

	if (rc != SA_AIS_OK) {
		LOG_ER("update attribute failed %d, dn %s, attr %s", rc, dn, attributeName);
		TRACE_LEAVE();
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/** 
 * SmfProcedureThread::processEvt
 * process events in the mailbox.
 */
void 
SmfProcedureThread::processEvt(void)
{
	PROCEDURE_EVT *evt;
        SmfProcResultT procResult = SMF_PROC_DONE;

	evt = (PROCEDURE_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&m_mbx, evt);
	if (evt != NULL) {
		TRACE("Procedure thread received event type %d", evt->type);

		switch (evt->type) {
		case PROCEDURE_EVT_TERMINATE:
			{
				/* */
				m_running = false;
				break;
			}

		case PROCEDURE_EVT_EXECUTE:
			{
				TRACE("Executing procedure %s", m_procedure->getProcName().c_str());
				procResult = m_procedure->execute();
				break;
			}

		case PROCEDURE_EVT_EXECUTE_STEP:
			{
				TRACE("Executing steps %s", m_procedure->getProcName().c_str());
				procResult = m_procedure->executeStep();
				break;
			}

		case PROCEDURE_EVT_ROLLBACK_STEP:
			{
				TRACE("Rollback steps %s", m_procedure->getProcName().c_str());
				procResult = m_procedure->rollbackStep(); 
				break;
			}

		case PROCEDURE_EVT_SUSPEND:
			{
				procResult = m_procedure->suspend();
				break;
			}

		case PROCEDURE_EVT_COMMIT:
			{
				procResult = m_procedure->commit();
				break;
			}

		case PROCEDURE_EVT_ROLLBACK:
			{
				procResult = m_procedure->rollback();
				break;
			}
		default:
			{
				LOG_ER("unknown event received %d", evt->type);
			}
		}

		delete(evt);
	}

        if (procResult != SMF_PROC_DONE) {
                /* Send procedure response to campaign thread */
                TRACE("Sending procedure response %d to campaign from %s", procResult, m_procedure->getProcName().c_str());
                CAMPAIGN_EVT *evt = new CAMPAIGN_EVT();
                evt->type = CAMPAIGN_EVT_PROCEDURE_RC;
                evt->event.procResult.rc = procResult;
                evt->event.procResult.procedure = m_procedure;
                SmfCampaignThread::instance()->send(evt);
        }
}

typedef enum {
	PROC_MBX_FD,
	PROC_MAX_FD
} proc_pollfd_t;

/** 
 * SmfProcedureThread::handleEvents
 * handle incoming events to the thread.
 */
int 
SmfProcedureThread::handleEvents(void)
{
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[PROC_MAX_FD];

	mbx_fd = ncs_ipc_get_sel_obj(&m_mbx);

	/* Set up all file descriptors to listen to */
	fds[PROC_MBX_FD].fd = mbx_fd.rmv_obj;
	fds[PROC_MBX_FD].events = POLLIN;

	TRACE("Procedure thread %s waiting for events", m_procedure->getDn().c_str());

	while (m_running) {
		int ret = poll(fds, PROC_MAX_FD, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		/* Process the Mail box events */
		if (fds[PROC_MBX_FD].revents & POLLIN) {
			/* dispatch MBX events */
			processEvt();
		}
	}

	return 0;
}

/** 
 * SmfProcedureThread::main
 * main for the thread.
 */
void 
SmfProcedureThread::main(void)
{
	TRACE_ENTER();
	if (this->init() == 0) {
		/* Mark the thread correctly started */
                m_procedure->setProcThread(this);
		if(m_semaphore != NULL) {
			sem_post(m_semaphore);
		}

		this->handleEvents();	/* runs forever until stopped */

		/* Mark the thread terminated */
		if(m_semaphore != NULL) {
			sem_post(m_semaphore);
		}
	} else {
		LOG_ER("SmfProcedureThread: init failed");
                if(m_semaphore != NULL) {
			sem_post(m_semaphore);
		}
	}
	TRACE_LEAVE();
}
