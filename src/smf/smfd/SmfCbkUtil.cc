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

#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <poll.h>
#include "osaf/saf/saAis.h"
#include "smf/saf/saSmf.h"

#include "base/ncssysf_def.h"
#include "base/ncssysf_ipc.h"
#include "base/logtrace.h"
#include "base/saf_error.h"
#include "base/osaf_extended_name.h"

#include "smf/smfd/SmfCbkUtil.h"
#include "smf/smfd/SmfUtils.h"

SmfCbkUtilThread *SmfCbkUtilThread::s_instance = NULL;

/** 
 * smf_cbk_util
 * handles the filtered callbacks triggered by a campaign 
 */
void smf_cbk_util (SaSmfHandleT smfHandle, SaInvocationT invocation, SaSmfCallbackScopeIdT scopeId,
		   const SaNameT *objectName, SaSmfPhaseT phase, const SaSmfCallbackLabelT *callbackLabel,
		   const SaStringT params)
{
	SaAisErrorT rc;
	SaAisErrorT cmdResult = SA_AIS_OK;
	char* label = (char*) malloc(callbackLabel->labelSize + 1);
        size_t objectNameLength = strlen(osaf_extended_name_borrow(objectName));
	char* obj = (char*) malloc(objectNameLength + 1);
	int result;

	memcpy(label, callbackLabel->label, callbackLabel->labelSize);
	label[callbackLabel->labelSize] = 0;

	memcpy(obj, osaf_extended_name_borrow(objectName), objectNameLength);
	obj[objectNameLength] = 0;

	TRACE("smf_cbk_util Cbk received.hdl: %llu, inv_id: %llu, scope_id: %d, obj_name: %s, phase: %d, label: %s, params: %s\n",
	       smfHandle, invocation, scopeId, obj, phase, label, params);

	setenv("CBK_OBJECT_NAME", obj, 1 /*overwrite*/);
	if (phase == SA_SMF_UPGRADE) {
		setenv("CBK_PHASE", "UPGRADE", 1 /*overwrite*/);
	}
	else {
		setenv("CBK_PHASE", "ROLLBACK", 1 /*overwrite*/);
	}

	if (!strcmp(label, SMF_UTIL_LABEL "Cmd")) {
		/* Execute Cmd in both upgrade and rollback phase */
		LOG_NO("smf_cbk_util executing cmd: '%s'", params);
		if ((result = smf_system(params)) != 0) {
			LOG_ER("smf_cbk_util cmd '%s' failed %d", params, result);
			cmdResult = SA_AIS_ERR_FAILED_OPERATION;
		}
	}
	else if (!strcmp(label, SMF_UTIL_LABEL "UpgradeCmd")) {
		/* Execute Cmd only in upgrade phase */
		TRACE("smf_cbk_util Upgrade Cmd requested: '%s'\n", params);
		if (phase == SA_SMF_UPGRADE) {
			LOG_NO("smf_cbk_util executing upgrade cmd: '%s'", params);
			if ((result = smf_system(params)) != 0) {
				LOG_ER("smf_cbk_util upgrade cmd '%s' failed %d", params, result);
				cmdResult = SA_AIS_ERR_FAILED_OPERATION;
			}
		}
		else {
			LOG_NO("smf_cbk_util wrong campaign phase (%d), skipping upgrade cmd : '%s'\n", phase, params);
		}
	}
	else if (!strcmp(label, SMF_UTIL_LABEL "RollbackCmd")) {
		/* Execute Cmd only in rollback phase */
		TRACE("smf_cbk_util Rollback Cmd requested: '%s'\n", params);
		if (phase == SA_SMF_ROLLBACK) {
			LOG_NO("smf_cbk_util executing rollback cmd: '%s'", params);
			if ((result = smf_system(params)) != 0) {
				LOG_ER("smf_cbk_util rollback cmd '%s' failed %d", params, result);
				cmdResult = SA_AIS_ERR_FAILED_OPERATION;
			}
		}
		else {
			LOG_NO("smf_cbk_util wrong campaign phase (%d), skipping rollback cmd : '%s'\n", phase, params);
		}
	}
	else {
		LOG_ER("smf_cbk_util unknown callback label requested: '%s'\n", label);
	}

	rc = saSmfResponse(smfHandle, invocation, cmdResult);

	free(label);
	free(obj);

	if (SA_AIS_OK != rc){
		LOG_ER("smf_cbk_util saSmfResponse failed, rc=%s", saf_error(rc));
		return;
	}
	TRACE("smf_cbk_util saSmfResponse successful.\n");
	return;
}

/*====================================================================*/
/*  Data Declarations                                                 */
/*====================================================================*/

/*====================================================================*/
/*  Class SmfCbkUtilThread                                           */
/*====================================================================*/

/*====================================================================*/
/*  Static methods                                                    */
/*====================================================================*/

/** 
 * SmfCbkUtilThread::instance
 * returns the only instance of SmfCbkUtilThread.
 */
SmfCbkUtilThread *SmfCbkUtilThread::instance(void)
{
	return s_instance;
}

/** 
 * SmfCbkUtilThread::terminate
 * terminates (if started) the only instance of SmfCbkUtilThread.
 */
void SmfCbkUtilThread::terminate(void)
{
	if (s_instance != NULL) {
		s_instance->stop();
		/* main will delete the SmfCbkUtilThread object when terminating */
	}

	return;
}

/** 
 * SmfCbkUtilThread::start
 * Create the object and start the thread.
 */
int SmfCbkUtilThread::start()
{
	if (s_instance != NULL) {
		LOG_ER("SmfCbkUtilThread already exists");
		return -1;
	}

	s_instance = new SmfCbkUtilThread();

	if (s_instance->startThread() != 0) {
		delete s_instance;
		s_instance = NULL;
		return -1;
	}
	return 0;
}

/** 
 * SmfCbkUtilThread::main
 * static main for the thread
 */
void* SmfCbkUtilThread::main(NCSCONTEXT info)
{
	TRACE_ENTER();
	SmfCbkUtilThread *utilObject = (SmfCbkUtilThread *) info;
	utilObject->mainThread();
	TRACE("SmfCbkUtilThread exits");
	delete utilObject;
	s_instance = NULL;
	TRACE_LEAVE();
	return 0;
}

/*====================================================================*/
/*  Methods                                                           */
/*====================================================================*/

/** 
 * Constructor
 */
 SmfCbkUtilThread::SmfCbkUtilThread():
	 m_mbx(0),
	 m_running(true), 
	 m_semaphore(NULL),
	 m_smfHandle(0)
{
}

/** 
 * Destructor
 */
SmfCbkUtilThread::~SmfCbkUtilThread()
{
}

/** 
 * SmfCbkUtilThread::startThread
 * Start the SmfCbkUtilThread.
 */
int SmfCbkUtilThread::startThread(void)
{
	pthread_t thread;
	pthread_attr_t attr;

	sem_t localSemaphore;
	sem_init(&localSemaphore, 0, 0);
	m_semaphore = &localSemaphore;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_create(&thread, &attr, SmfCbkUtilThread::main, (void*)this) != 0) {
		LOG_ER("pthread_create failed: %s", strerror(errno));
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
 * SmfCbkUtilThread::stop
 * Stop the SmfCbkUtilThread.
 */
int SmfCbkUtilThread::stop(void)
{
	TRACE_ENTER();
	TRACE("Stopping SmfCbkUtilThread");

	sem_t localSemaphore;
	sem_init(&localSemaphore, 0, 0);
	m_semaphore = &localSemaphore;

	/* send a message to the thread to make it terminate */
	SMFCBKUTIL_EVT *evt = new SMFCBKUTIL_EVT();
	evt->type = SMFCBKUTIL_EVT_TERMINATE;
	this->send(evt);

	/* Wait for the thread to terminate */
	while((sem_wait(&localSemaphore) == -1) && (errno == EINTR))
               continue;       /* Restart if interrupted by handler */

        //m_semaphore can not be written here since the thread may have already deleted the SmfCbkUtilThread object
	//m_semaphore = NULL;
	sem_destroy(&localSemaphore);

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCbkUtilThread::init
 * init the thread.
 */
int SmfCbkUtilThread::init(void)
{
	TRACE_ENTER();
	uint32_t rc;

	/* Create the mailbox used for communication with this thread */
	if ((rc = m_NCS_IPC_CREATE(&m_mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE failed %d", rc);
		return -1;
	}

	/* Attach mailbox to this thread */
	if ((rc = m_NCS_IPC_ATTACH(&m_mbx) != NCSCC_RC_SUCCESS)) {
		LOG_ER("m_NCS_IPC_ATTACH failed %d", rc);
		return -1;
	}

	/* Initialize the SMF callback API */
	if (this->initSmfCbkApi() != 0) {
		LOG_ER("initSmfCbkApi failed");
		return -1;
	}

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCbkUtilThread::initSmfCbkApi
 * initialize the SMF callback API.
 */
int SmfCbkUtilThread::initSmfCbkApi(void)
{
	TRACE_ENTER();
	SaVersionT ver;
	SaSmfCallbacksT cbk;
	SaAisErrorT rc;
	SaSmfCallbackScopeIdT scope_id = 1;
	SaSmfLabelFilterArrayT filter_arr;
	SaSmfLabelFilterT filter_label[1];
	char filter_str[] = SMF_UTIL_LABEL;

	ver.releaseCode = 'A';
	ver.majorVersion = 1;
	ver.minorVersion = 1;
	
	cbk.saSmfCampaignCallback = smf_cbk_util;

	rc = saSmfInitialize(&m_smfHandle, &cbk, &ver);
	if (SA_AIS_OK != rc){
		LOG_ER("SmfCbkUtilThread::initSmfCbkApi saSmfInitialize failed, rc=%s", saf_error(rc));
		return -1;
	}
	TRACE("SmfCbkUtilThread::initSmfCbkApi saSmfInitialize successful. Hdl: %llu.\n", m_smfHandle);
	
	/* Create a filter so we only receive our specific callbacks */
	filter_label[0].filterType = SA_SMF_PREFIX_FILTER;
	filter_label[0].filter.labelSize = strlen(filter_str);
	filter_label[0].filter.label = (SaUint8T *)filter_str; 
	
	filter_arr.filtersNumber = 1;
	filter_arr.filters = filter_label; 

	rc = saSmfCallbackScopeRegister(m_smfHandle, scope_id, &filter_arr);
	if (SA_AIS_OK != rc){
		LOG_ER("SmfCbkUtilThread::initSmfCbkApi saSmfCallbackScopeRegister failed, rc=%s", saf_error(rc));
		return -1;
	}
	TRACE("SmfCbkUtilThread::initSmfCbkApi saSmfCallbackScopeRegister successful. scope_id: %d, filter: %s\n",
	       scope_id, filter_str);

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCbkUtilThread::finalizeSmfCbkApi
 * finalize the SMF callback API.
 */
int SmfCbkUtilThread::finalizeSmfCbkApi(void)
{
	TRACE_ENTER();

	if (m_smfHandle != 0) {
		SaAisErrorT rc;
		rc = saSmfFinalize(m_smfHandle);
		if (SA_AIS_OK != rc){
			LOG_ER("SmfCbkUtilThread::finalizeSmfCbkApi saSmfFinalize failed, rc=%s", saf_error(rc));
		}
		m_smfHandle = 0;
	}

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCbkUtilThread::send
 * send a mailbox event to this thread.
 */
int SmfCbkUtilThread::send(SMFCBKUTIL_EVT * evt)
{
	uint32_t rc;

	//Save the mailbox pointer since SmfCbkUtilThread object may be deleted 
	//when SmfCbkUtilThread member variable m_mbx is used in m_NCS_IPC_SEND
	SYSF_MBX tmp_mbx = m_mbx;

	TRACE("SmfCbkUtilThread send event type %d", evt->type);
	rc = m_NCS_IPC_SEND(&tmp_mbx, (NCSCONTEXT) evt, NCS_IPC_PRIORITY_HIGH);

	return rc;
}

/** 
 * SmfCbkUtilThread::processEvt
 * process events in the mailbox.
 */
void SmfCbkUtilThread::processEvt(void)
{
	SMFCBKUTIL_EVT *evt;

	evt = (SMFCBKUTIL_EVT *) m_NCS_IPC_NON_BLK_RECEIVE(&m_mbx, evt);
	if (evt != NULL) {
		TRACE("SmfCbkUtilThread received event type %d", evt->type);

		switch (evt->type) {
		case SMFCBKUTIL_EVT_TERMINATE:
			{
				/* */
				m_running = false;
				break;
			}

		default:
			{
				LOG_ER("SmfCbkUtilThread::processEvt unknown event received %d", evt->type);
			}
		}

		delete(evt);
	}
}

/** 
 * SmfCbkUtilThread::handleEvents
 * handle incoming events to the thread.
 */
int SmfCbkUtilThread::handleEvents(void)
{
	TRACE_ENTER();
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[2];
	SaAisErrorT rc;
	SaSelectionObjectT smf_sel_obj;

	mbx_fd = ncs_ipc_get_sel_obj(&m_mbx);

	/* Set up all file descriptors to listen to */

	/* Mailbox fd */
	fds[0].fd = mbx_fd.rmv_obj;
	fds[0].events = POLLIN;
	fds[0].revents = 0; /* Coverity */

	/* SMF callback API fd */
	rc = saSmfSelectionObjectGet(m_smfHandle, &smf_sel_obj);
	if (SA_AIS_OK != rc){
		LOG_ER("saSmfSelectionObjectGet failed, rc=%s", saf_error(rc));
		return 0;
	}
	fds[1].fd = smf_sel_obj;
	fds[1].events = POLLIN;

	while (m_running) {
		
		/* Wait for something to arrive */
                int ret = poll(fds, 2, -1);

		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

                /* Process any mailbox events */
		if (fds[0].revents & POLLIN) {
			/* dispatch MBX events */
			processEvt();
		}

                /* Process any SMF callback events */
		if (fds[1].revents & POLLIN) {
			/* dispatch SMF callback events */
			rc = saSmfDispatch(m_smfHandle, SA_DISPATCH_ALL);	
			if (SA_AIS_OK != rc){
				LOG_ER("SmfCbkUtilThread saSmfDispatch failed, rc=%s", saf_error(rc));
			}
		}
	}

	TRACE_LEAVE();
	return 0;
}

/** 
 * SmfCbkUtilThread::mainThread
 * main for the thread.
 */
void SmfCbkUtilThread::mainThread(void)
{
	TRACE_ENTER();
	if (this->init() == 0) {
		/* Mark the thread started */
		if(m_semaphore != NULL) {
			sem_post(m_semaphore);
		}

	} else {
	       	LOG_ER("SmfCbkUtilThread init failed, terminating thread");
		if(m_semaphore != NULL) {
			sem_post(m_semaphore);
		}
		TRACE_LEAVE();
		return;
	}

	this->handleEvents();	/* runs forever until stopped */

	this->finalizeSmfCbkApi(); 

	/* Mark the thread terminated */
	if(m_semaphore != NULL) {
		sem_post(m_semaphore);
	}

	TRACE_LEAVE();
}
