/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#include "lgs_imm_gcfg.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "saf_error.h"
#include "logtrace.h"
#include "saImmOi.h"
#include "saImmOm.h"
#include "immutil.h"
#include "osaf_time.h"
#include "osaf_poll.h"

/*
 * Implements an IMM applier for the OpensafConfig class.
 * Used for detecting changes of opensafNetworkName attribute.
 * The applier runs in its own thread
 *
 * API:
 *
 * int lgs_start_gcfg_applier()
 * This function starts the applier in a separate thread
 *
 * int lgs_stop_gcfg_applier()
 * This function stops the applier and free all resources
 *
 * char* lgs_get_networkname(char **name_str)
 * Gives a pointer to a network name string
 *
 * TBC: Place a message in the mail box when network name is changed??
 *
 * USAGE:
 *
 * The applier shall be started on the ACTIVE server only.
 * Start the applier when log server is started and when changing HA state
 * to ACTIVE.
 * Stop the applier when leaving ACTIVE state (quiesced and standby state handler)
 *
 * NOTES:
 * 1. Immutil cannot be used. The immutil wrapper profile is global
 * and not thread safe.
 *
 */

/* Information given when starting thread
 */
typedef struct thread_info {
	int socket;
} lgs_thread_info_t;

/**
 * 'Private' global variables used for:
 */
static const SaImmClassNameT gcfg_class = const_cast<SaImmClassNameT>("OpensafConfig");
static const SaImmOiImplementerNameT applier_name = const_cast<SaImmOiImplementerNameT>("@safLogService_appl");
/* IMM handling */
static SaVersionT immVersion = { 'A', 2, 11 };

/* Network name handling */
static char *network_name = NULL; /* Save pointer to current name */
static pthread_mutex_t lgs_gcfg_applier_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Inter thread communication (socket pair) */
static const char* CMD_STOP = "s";
static const char* CMD_START = "a";
static const size_t CMD_LEN = 2;

typedef enum {
	AP_START,
	AP_STOP
} th_cmd_t;

static int to_thread_fd = 0; /* Socket for communication with thread */

/* Become applier cancellation handling */
static bool cancel_flg = false; /* If true, cancel at next cancellation point */

/* Thread */
static pthread_t thread_id = 0;

/* States for the applier thread. Stopping and starting the applier is handled
 * differently dependent on the thread state
 */
typedef enum {
	/* Thread not started */
	TH_NOT_STARTED = 0,
	/* Thread is started but has not yet entered the event handling poll loop */
	TH_STARTING,
	/* Thread is fully started and running as applier */
	TH_IS_APPLIER,
	/* Thread is fully started but is not running as applier. In this state
	 * it can service commands e.g. start applier command but has never
	 * become an applier or have given up the applier role
	 */
	TH_IDLE
} th_state_t;

static th_state_t th_state = TH_NOT_STARTED;

/******************************************************************************
 * Outside applier thread
 ******************************************************************************/

/*******************
 * Utility functions
 */

static int start_applier_thread(lgs_thread_info_t *start_info);
static void save_network_name(char* new_name);
static void applier_finalize(SaImmOiHandleT imm_appl_hdl);
static void *applier_thread(void *info_in);
static int read_network_name();
static bool th_do_cancel();

/**
 * The function is used indirectly by the main thread
 * to start/stop the applier thread.
 *
 * @param command: start/stop command
 * @return void
 */
static void send_command(th_cmd_t command)
{
	const char *cmd_ptr = NULL;
	int rc = 0;

	TRACE_ENTER();

	switch (command) {
	case AP_START:
		cmd_ptr = CMD_START;
		break;
	case AP_STOP:
		cmd_ptr = CMD_STOP;
		break;
	default:
		LOG_ER("Unknown command %d", command);
		osaf_abort(0);
	}

	while (1) {
		rc = write(to_thread_fd, cmd_ptr, CMD_LEN);
		if ((rc == -1) && ((errno == EINTR) || (errno == EAGAIN)))
			/* Try again */
			continue;
		else
			break;
	}

	if (rc == -1) {
		LOG_ER("Write Fail %s", strerror(errno));
		osaf_abort(0);
	}

	TRACE_LEAVE2("Command '%s', rc=%d", cmd_ptr, rc);
}

/**
 * Activate cancellation of ongoing applier initialization.
 * This function is used outside of thread to trig cancellation
 * The corresponding th_cancel_check function is used inside thread as a
 * "cancellation point".
 */
static void th_cancel_activate()
{
	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);
	cancel_flg = true;
	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);
}

/**
 * Set th_state protected by mutex. See corresponding th_state_get()
 * @param th_state_set[in]
 */
static th_state_t inline th_state_get()
{
	th_state_t rc_th_state;

	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);
	rc_th_state = th_state;
	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);

	return rc_th_state;
}

/**************************
 * Network name handler API
 */

/**
 * Start the applier
 * Create applier thread (if needed) and start the applier.
 * The thread initiates the IMM applier, reads opensafNetworkName and then waits
 * for callbacks (selection object) in a poll loop
 *
 */
void lgs_start_gcfg_applier()
{
	static lgs_thread_info_t thread_input;
	int sock_fd[2] = {0};
	int rc = 0;
	std::string tmp_name;

	TRACE_ENTER();

	switch (th_state_get()) {
	case TH_NOT_STARTED:
		TRACE("TH_NOT_STARTED");
		/* Get the network name if not already fetched and saved */
		tmp_name = lgs_get_networkname();
		if (tmp_name.empty() == true) {
			rc = read_network_name();
			if (rc == -1) {
				LOG_WA("read_network_name() Fail");
			}
		}

		/* Create socket pair for communication with applier thread
		 * Non blocking
		 */
		rc = socketpair(AF_UNIX, SOCK_STREAM, 0, sock_fd);
		if (rc == -1) {
			LOG_ER("socketpair Fail %s", strerror(errno));
			osaf_abort(0);
		}

		fcntl(sock_fd[0], F_SETFL, O_NONBLOCK);
		fcntl(sock_fd[1], F_SETFL, O_NONBLOCK);
		to_thread_fd = sock_fd[0];
		thread_input.socket = sock_fd[1];

		/* Start the thread */
		(void) start_applier_thread(&thread_input);
		break;

	case TH_IDLE:
		TRACE("TH_IDLE");
		/* Send applier start command to running thread */
		send_command(AP_START);
		break;

	case TH_STARTING:
		TRACE("TH_STARTING");
	case TH_IS_APPLIER:
		TRACE("TH_IS_APPLIER");
		/* There is nothing to start we are already applier */
	default:
		TRACE("%s: Called in wrong state %d", __FUNCTION__, th_state);
		break;
	}

	TRACE_LEAVE();
}

/**
 * Stop the applier but only if an applier is started
 *
 */
void lgs_stop_gcfg_applier()
{
	TRACE_ENTER();

	switch (th_state_get()) {
	case TH_IS_APPLIER:
		TRACE("TH_IS_APPLIER");
		send_command(AP_STOP);
		break;

	case TH_STARTING:
		TRACE("TH_STARTING");
		/* Cancel ongoing applier activation and send stop command to
		 * enter correct state
		 */
		th_cancel_activate();
		send_command(AP_STOP);
		break;

	case TH_NOT_STARTED:
		TRACE("TH_NOT_STARTED");
	case TH_IDLE:
		TRACE("TH_IDLE");
		/* Nothing to stop there is no active applier */
	default:
		TRACE("%s: Called in wrong state %d", __FUNCTION__, th_state);
	}

	TRACE_LEAVE();
}

/**
 * Get the network name.
 * If no network name exist a NULL pointer is returned
 * Saved as a dynamic variable string. In parameter name_str will be set
 * to point to the string.
 * network_name. Saving and reading is done in different threads so
 * must be protected by mutex. See also save_network_name()
 *
 * @param name_str[in]
 *        Memory for a string will be allocated and the pointer will be given
 *        to the calling function that has to free the memory.
 *
 * @return pointer to a network name string
 */
std::string lgs_get_networkname()
{
	std::string networkName = "";

	TRACE_ENTER();

	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);

	/* Thread safe handling of return value */
	if (network_name != NULL) {
		networkName = network_name;
	}

	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);

	TRACE_LEAVE2("name_str \"%s\"", networkName.empty()? "<empty>": networkName.c_str());

	return networkName;
}

/******************************************************************************
 * Used by applier thread only
 ******************************************************************************/

/*********************************
 * Applier callback functions
 * See IMM AIS SaImmOiCallbacksT_2
 * Executed in OI thread
 */

/**
 * Save ccb info if OpensafConfig object is modified
 *
 */
static SaAisErrorT ccbObjectModifyCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaNameT *objectName,
		const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu, '%s'", ccbId, objectName->value);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			TRACE("Failed to get CCB object for %llu", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Cleanup by deleting saved ccb info if aborted
 *
 */
static void ccbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		TRACE("Failed to find CCB object for %llu", ccbId);

	TRACE_LEAVE();
}

/**
 * Check if network name has been modified and save new network name
 *
 * We are an applier for the OpensafConfig class. Only one object of this class
 * can exist so we don't have iterate over objects.
 * We only save modifications so the operation type must be modify.
 * We iterate over attributes even if there is only one configuration attribute
 * but more attributes may be added in the future.
 *
 */
static void ccbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;
	struct CcbUtilOperationData *opdata;
	const SaImmAttrModificationT_2 *attrMod;
	char *value_str = NULL;
	int i = 0;

	TRACE_ENTER2("CCB ID %llu", ccbId);

	/* Verify the ccb */
	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		TRACE("Failed to find CCB object for %llu", ccbId);
		goto done;
	}

	/* We don't need to loop since the only possible opereation is MODIFY
	 * and the only possible class is OpensafConfig class
	 */
	opdata = ccbUtilCcbData->operationListHead;
	if (opdata->operationType != CCBUTIL_MODIFY) {
		TRACE("Not a modify operation");
		goto done;
	}

	if (strncmp(reinterpret_cast<char *>(opdata->objectName.value),
		"opensafConfigId", sizeof("opensafConfigId") - 1) != 0) {
		TRACE("Object \"%s\" not a OpensafConfig object",
		      reinterpret_cast<char *>(opdata->objectName.value));
		goto done;
	}

	/* Read value in opensafNetworkName
	 * Note: This is implemented as a loop in case more attributes are
	 *       added in the class in the future. For now the only
	 *       attribute of interest here is opensafNetworkName
	 */
	TRACE("Read value in attributes");
	i = 0;
	attrMod = opdata->param.modify.attrMods[i];
	for (i = 1; attrMod != NULL; i++) {
		const SaImmAttrValuesT_2 *attribute = &attrMod->modAttr;
		TRACE("Found attribute \"%s\"", attribute->attrName);
		if (strcmp(attribute->attrName, "opensafNetworkName") == 0) {
			/* Get the value for opensafNetworkName */
			void *value = NULL;
			if (attribute->attrValuesNumber != 0)
				value = attribute->attrValues[0];

			if (value == NULL) {
				TRACE("Value is NULL");
				save_network_name(value_str);
				goto done;
			}

			value_str = *(static_cast<char **>(value));
			TRACE("%s='%s'", attribute->attrName, value_str);

			/*
			 * Save the new network_name
			 */
			save_network_name(value_str);
		} else {
			/* Not opensafNetworkName */
			TRACE("Found attribute %s", attribute->attrName);
		}

		attrMod = opdata->param.modify.attrMods[i];
	}

done:
	if (ccbUtilCcbData != NULL)
		ccbutil_deleteCcbData(ccbUtilCcbData);

	TRACE_LEAVE();
}

/**
 * An object of OpensafConfig class is created. We don't have to do anything
 *
 */
static SaAisErrorT ccbCreateCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaImmClassNameT className,
		const SaNameT *parentName, const SaImmAttrValuesT_2 **attr)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/**
 * An object of OpensafConfig class is deleted. We don't have to do anything
 *
 */
static SaAisErrorT ccbDeleteCallback(SaImmOiHandleT immOiHandle,
		SaImmOiCcbIdT ccbId, const SaNameT *objectName)
{
	TRACE_ENTER();
	TRACE_LEAVE();
	return SA_AIS_OK;
}

/* Callback function list
 * We need:
 * Modify for saving modify ccb
 * Apply for knowing that the modification has been applied
 * Abort for removing saved ccb in case of an abortion of ccb
 */
static const SaImmOiCallbacksT_2 callbacks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = ccbAbortCallback,
	.saImmOiCcbApplyCallback = ccbApplyCallback,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = ccbCreateCallback,
	.saImmOiCcbObjectDeleteCallback = ccbDeleteCallback,
	.saImmOiCcbObjectModifyCallback = ccbObjectModifyCallback,
	.saImmOiRtAttrUpdateCallback = NULL
};


/***************************************
 * Internal utility function definitions
 */

/**
 * Save a new network name
 * Saved as a dynamic variable string pointed to by 'private' global pointer
 * network_name. Saving and reading is done in different threads so
 * must be protected by mutex. See also lgs_get_networkname
 *
 * @param new_name[in]
 */
static void save_network_name(char* new_name)
{
	uint32_t name_len = 0;

	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);

	/* Delete old name */
	if (network_name != NULL)
		free(network_name);

	/* Save new name */
	if (new_name == NULL) {
		network_name = NULL;
	} else {
		name_len = strlen(new_name) + 1;
		network_name = static_cast<char *>(calloc(1, name_len));
		if (network_name == NULL) {
			LOG_ER("%s: calloc Fail", __FUNCTION__);
			osaf_abort(0);
		}

		strcpy(network_name, new_name);
	}

	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);

	TRACE_LEAVE();
}

/**
 * Read network name from the OpensafConfig object
 * - Create an Object manager
 * - Search for and get opensafNetworkName value
 * - Allocate memory for and save the value. Save pointer in network_name
 *
 * If no value is found network_name will be NULL
 *
 * @return -1 on error
 */
static int read_network_name()
{
	SaAisErrorT ais_rc = SA_AIS_OK;
	SaImmHandleT om_handle = 0;
	char *tmp_name_str = NULL;
	int rc = 0;
	/* Setup value get parameters */
	SaNameT object_name;
	SaImmAttrValuesT_2 **attributes;
	SaImmAttrValuesT_2 *attribute;
	void *value = NULL;
	/* Setup search initialize parameters */
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	searchParam.searchOneAttr.attrName = const_cast<SaImmAttrNameT>(
		"opensafNetworkName");
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = NULL;

	SaImmAttrNameT attributeNames[2] = {
		const_cast<SaImmAttrNameT>("opensafNetworkName"),
		NULL
	};

	TRACE_ENTER();

	/*
	 * Initialize an IMM object manager
	 */
	ais_rc = immutil_saImmOmInitialize(&om_handle, NULL, &immVersion);
	if (ais_rc != SA_AIS_OK) {
		TRACE("immutil_saImmOmInitialize FAIL %s", saf_error(ais_rc));
		rc = -1;
		goto done;
	}

	/*
	 * Search for objects with attribute opensafNetworkName
	 */
	/* Find the opensafNetworkName attribute in the OpensafConfig object and
	 * get its value
	 */
	ais_rc = immutil_saImmOmSearchInitialize_2(
		om_handle,
		NULL,
		SA_IMM_SUBTREE,
		SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR,
		&searchParam,
		attributeNames,
		&searchHandle
		);
	if (ais_rc != SA_AIS_OK) {
		TRACE("immutil_saImmOmSearchInitialize_2 FAIL %s", saf_error(ais_rc));
		rc = -1;
		goto done;
	}

	/*
	 * Get the value for the searched attribute
	 */
	ais_rc = immutil_saImmOmSearchNext_2(searchHandle, &object_name, &attributes);
	if (ais_rc != SA_AIS_OK) {
		TRACE("immutil_saImmOmSearchNext_2 FAIL %s", saf_error(ais_rc));
		rc = -1;
		goto done;
	}

	/*
	 * Save the network name or set to NULL if no name is found
	 * Note: Saving network_name must be thread safe
	 */
	attribute = attributes[0];
	if (attribute->attrValuesNumber != 0) {
		value = attribute->attrValues[0];
		tmp_name_str = *(static_cast<char **>(value));
	} else {
		/* Set network name to an empty string if the attribute value
		 * is "empty"
		 */
		tmp_name_str = const_cast<char *>("");
	}

	save_network_name(tmp_name_str);

done:
	/* Finalize search */
	ais_rc = immutil_saImmOmFinalize(om_handle);
	if ((ais_rc != SA_AIS_OK) && (ais_rc != SA_AIS_ERR_BAD_HANDLE)) {
		/* BAD HANDLE is not a fault here since finalize is called
		 * also if initialize failed
		 */
		TRACE("immutil_saImmOmFinalize FAIL %s", saf_error(ais_rc));
		rc = -1;
	}

	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Initialize IMM API and get selection object
 * The following API sequence is used:
 * - Finalize if a handle exist
 * - Initialize (get imm OI handle)
 * - Get selection object
 *
 * Note1:TRY AGAIN loops time out after "max_waiting_time..." which can be
 *       up to 60 sec.
 *
 * Note2: Init can be canclled. See th_cancel_activate()
 *        Cancelling is done when reaching a cancellation point defined by
 *        th_cancel_OIinit()
 *
 * Is 60 sec timeout for initialize really needed?
 *
 * @param imm_appl_hdl[out]
 * @param imm_appl_selobj[out]
 *
 * @return -1 on error
 */
/**
 *
 * @param imm_appl_hdl
 * @param imm_appl_selobj
 * @param nfds
 * @return
 */
static int applier_init(SaImmOiHandleT *imm_appl_hdl,
	SaSelectionObjectT *imm_appl_selobj)
{
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;

	TRACE_ENTER();

	if (th_do_cancel()) {
		TRACE("Canceled at 1");
		/* Cancellation point no error */
		goto done;
	}

	/* Finalize applier OI handle if needed */
	applier_finalize(*imm_appl_hdl);

	if (th_do_cancel()) {
		TRACE("Canceled at 2");
		/* Cancellation point no error */
		goto done;
	}

	/* Initialize OI for applier and get OI handle */
	ais_rc = immutil_saImmOiInitialize_2(imm_appl_hdl, &callbacks, &immVersion);
	if (ais_rc != SA_AIS_OK) {
		LOG_WA("immutil_saImmOiInitialize_2 Failed %s", saf_error(ais_rc));
		rc = -1;
		goto done;
	}

	if (th_do_cancel()) {
		TRACE("Canceled at 3");
		/* Cancellation point no error */
		applier_finalize(*imm_appl_hdl);
		goto done;
	}

	/* Get selection object for event handling */
	ais_rc = immutil_saImmOiSelectionObjectGet(*imm_appl_hdl, imm_appl_selobj);
	if (ais_rc != SA_AIS_OK) {
		LOG_WA("immutil_saImmOiSelectionObjectGet Failed %s", saf_error(ais_rc));
		applier_finalize(*imm_appl_hdl);
		imm_appl_hdl = 0;
		rc = -1;
		goto done;
	}

	if (th_do_cancel()) {
		TRACE("Canceled at 3");
		/* Cancellation point no error */
		applier_finalize(*imm_appl_hdl);
		goto done;
	}

done:
	TRACE_LEAVE2("rc = %d", rc);
	return rc;
}

/**
 * Become an IMM applier for OpensafConfig class
 * - Register applier name
 * - Become class implementer
 *
 * Note1: TRY AGAIN loops time out after "max_waiting_time..." which can be
 *        up to 60 sec.
 *
 * Note2: Must be done after lgs_imma_init_handle()
 *
 * @return -1 on error
 */
static int applier_set_name_class(SaImmOiHandleT imm_appl_hdl)
{
	SaAisErrorT ais_rc = SA_AIS_OK;
	int rc = 0;

	TRACE_ENTER();

	if (th_do_cancel()) {
		TRACE("Canceled at 1");
		/* Cancellation point no error */
		applier_finalize(imm_appl_hdl);
		goto done;
	}

	/* Become applier */
	ais_rc = immutil_saImmOiImplementerSet(imm_appl_hdl, applier_name);
	if (ais_rc != SA_AIS_OK) {
		LOG_WA("immutil_saImmOiImplementerSet Failed %s", saf_error(ais_rc));
		applier_finalize(imm_appl_hdl);
		rc = -1;
		goto done;
	}

	if (th_do_cancel()) {
		TRACE("Canceled at 2");
		/* Cancellation point no error */
		applier_finalize(imm_appl_hdl);
		goto done;
	}

	/* Become class implementer (applier) for the OpensafConfig class */
	ais_rc = immutil_saImmOiClassImplementerSet(imm_appl_hdl, gcfg_class);
	if (ais_rc != SA_AIS_OK) {
		LOG_WA("immutil_saImmOiClassImplementerSet Failed %s", saf_error(ais_rc));
		applier_finalize(imm_appl_hdl);
		rc = -1;
		goto done;
	}

	if (th_do_cancel()) {
		TRACE("Canceled at 3");
		/* Cancellation point no error */
		applier_finalize(imm_appl_hdl);
		goto done;
	}

done:
	TRACE_LEAVE();
	return rc;
}

/**
 * Finalize the IMM applier
 *
 * @param imm_appl_hdl[in]
 */
static void applier_finalize(SaImmOiHandleT imm_appl_hdl)
{
	SaAisErrorT ais_rc = SA_AIS_OK;

	TRACE_ENTER();

	/* Finalize applier OI handle if needed */
	if (imm_appl_hdl != 0) {
		/* We have a handle to finalize */
		TRACE("saImmOiFinalize");
		ais_rc = immutil_saImmOiFinalize(imm_appl_hdl);
		if (ais_rc != SA_AIS_OK) {
			TRACE("immutil_saImmOiFinalize Failed %s", saf_error(ais_rc));
		}

		imm_appl_hdl = 0;
	} else {
		TRACE("Finalize not needed");
	}

	TRACE_LEAVE();
}


/*******************************
 * 'Private' variables used for:
 */

/**
 * Check if applier init cancellation is activated
 * Return value of cancellation flag and reset flag
 *
 * @return true if cancellation is active
 */
static bool th_do_cancel()
{
	bool rc = false;
	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);
	rc = cancel_flg;
	cancel_flg = false;
	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);
	return rc;
}

/****************************
 * Applier event handler main
 *
 * The applier runs in a separate thread. The thread uses a poll loop to handle
 * IMM events and communication events. Communication is handled using a socket
 * pair.
 * Possible communication events:
 * - Start applier
 *   Set implementer name and become class-applier
 * - Stop applier
 *   Do saImmOiImplementerClear()
 *
 * The handler also needs a cancellation mechanism. This mechanism does not
 * cancel the thread but an ongoing applier initiation must be possible to
 * cancel. This means that if setting of implementer name or class is ongoing
 * it will be stopped and saImmOiImplementerClear() is called
 */

/**
 * Start the applier thread if not already started
 *
 * @param start_info[in] See lgs_thread_info_t
 * @return -1 on error
 */
static int start_applier_thread(lgs_thread_info_t *start_info)
{
	int rc = 0;
	void *thr_arg = start_info;

	TRACE_ENTER();

	if (th_state == TH_NOT_STARTED) {
		rc = pthread_create(&thread_id, NULL, applier_thread, thr_arg);
		if (rc != 0) {
			LOG_ER("pthread_create Fail %d", rc);
			osaf_abort(0);
		}
	} else {
		TRACE("Thread already started");
		rc = -1;
	}

	TRACE_LEAVE();
	return rc;
}

/**
 * Set th_state protected by mutex. See corresponding th_state_get()
 * @param th_state_set[in]
 */
static void inline th_state_set(th_state_t th_state_set)
{
	osaf_mutex_lock_ordie(&lgs_gcfg_applier_mutex);
	th_state = th_state_set;
	osaf_mutex_unlock_ordie(&lgs_gcfg_applier_mutex);
}

/**
 * The applier thread
 * @param info_in[in] See lgs_thread_info_t
 * @return -
 */
static void *applier_thread(void *info_in)
{
	/* Event handling
	 * Note:
	 * IMM selection object can only be part of polling when IMM OI is
	 * initialized. If IMM is finalized the selection object is not valid.
	 * This is handled by placing IMM selection fd as last element in fds
	 * vector and decrease and decrease nfds when IMM is finalized.
	 */
	enum {
		FDA_COM = 0,	/* Communication events */
		FDA_IMM		/* IMM events */
	};

	static struct pollfd fds[2];
	static nfds_t nfds = 1; /* We have no IMM selection object yet */
	int rc = 0;
	SaAisErrorT ais_rc = SA_AIS_OK;
	static SaImmOiHandleT imm_appl_hdl = 0;
	static SaSelectionObjectT imm_appl_selobj = 0;

	TRACE_ENTER2("Thread will never return");
	lgs_thread_info_t *start_info = static_cast<lgs_thread_info_t *>(info_in);
	int com_fd = start_info->socket;

	/* Loop needed for new intialize if imm_appl_hdl becomes invalid or the
	 * applier is restarted
	 */
	while (1) {
		/* Initiate applier */
		th_state_set(TH_STARTING);

		rc = applier_init(&imm_appl_hdl, &imm_appl_selobj);
		if (rc == -1) {
			/* Some error handling */
			LOG_WA("applier_init Fail. Exit the appiler thread");
			return NULL;
		}

		nfds = FDA_IMM + 1; /* IMM selection object is valid */

		rc = applier_set_name_class(imm_appl_hdl);
		if (rc == -1) {
			/* Some error handling */
			LOG_WA("applier_set_name_class Fail. Exit the appier thread");
			return NULL;
		}

		th_state_set(TH_IS_APPLIER);

		/* Event handling */
		fds[FDA_IMM].fd = imm_appl_selobj;
		fds[FDA_IMM].events = POLLIN;
		fds[FDA_COM].fd = com_fd;
		fds[FDA_COM].events = POLLIN;

		/* If no network name is saved, read the network name from the
		 * OpensafConfig class and save it
		 */
		std::string tmp_networkname;
		tmp_networkname = lgs_get_networkname();
		if (tmp_networkname.empty() == true) {
			rc = read_network_name();
			if (rc == -1) {
				LOG_WA("read_network_name() Fail");
			}
		}

		/*** Event handling loop ***/
		while(1) {

			(void) osaf_poll(fds, nfds, -1);

			if (fds[FDA_IMM].revents & POLLIN) {
				TRACE("%s: IMM event", __FUNCTION__);
				ais_rc = saImmOiDispatch(imm_appl_hdl, SA_DISPATCH_ALL);
				if (ais_rc == SA_AIS_ERR_BAD_HANDLE) {
					/* Handle is lost. We must initialize again */
					th_state_set(TH_STARTING);
					break;
				}
			}

			if (fds[FDA_COM].revents & POLLIN) {
				TRACE("%s: COM event", __FUNCTION__);
				/* Handle start and stop requests */
				char cmd_str[256] = {0};
				while (1) {
					rc = read(com_fd, cmd_str, 256);
					if ((rc == -1) && ((errno == EINTR) ||
						(errno == EAGAIN))) {
						/* Try again */
						continue;
					} else break;
				}

				if (rc == -1) {
					LOG_ER("write Fail %s", strerror(errno));
					osaf_abort(0);
				}

				TRACE("Read command '%s'", cmd_str);

				if (!strcmp(cmd_str, CMD_STOP)) {
					TRACE("STOP received");
					/* Reset cancel flag */
					th_do_cancel();
					/* Give up applier */
					applier_finalize(imm_appl_hdl);
					/* The IMM selection object is no longer
					 * valid so we have to stop polling IMM
					 */
					nfds -= 1;
					th_state_set(TH_IDLE);
				} else if (!strcmp(cmd_str, CMD_START)) {
					TRACE("START received");
					/* Leave event loop for outer restart
					 * loop
					 */
					break;
				}
			}
		} /* END Event handling loop */
	} /* END Restart loop */

	return NULL;
}
