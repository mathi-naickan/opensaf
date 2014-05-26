/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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

#include "lgs_file.h"
#include "lgs_filehdl.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#include <logtrace.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <ncsgl_defs.h>

#include "lgs.h"
#include "osaf_utility.h"
#include "osaf_time.h"

pthread_mutex_t lgs_ftcom_mutex;	/* For locking communication */
static pthread_cond_t request_cv;	/* File thread waiting for request */
static pthread_cond_t answer_cv;	/* API waiting for answer (timed) */

/* Max time to wait for file thread to finish */
static uint32_t max_waittime_ms = 500;

struct file_communicate {
	bool request_f;	/* True if pending request */
	bool answer_f;	/* True if pending answer */
	bool timeout_f; /* True if API has got a timeout. Thread shall not answer */
	lgsf_treq_t request_code;	/* Request code from API */
	int return_code;	/* Return code from handlers */
	void *indata_ptr;	/* In-parameters for handlers */
	size_t outdata_size;
	void *outdata_ptr;	/* Out data from handlers */
};

/* Used for synchronizing and transfer of data ownership between main thread
 * and file thread.
 */
static struct file_communicate lgs_com_data = {
	.answer_f = false,
	.request_f = false,
	.timeout_f = false,
	.request_code = LGSF_NOREQ,
	.return_code = LGSF_NORETC,
	.indata_ptr = NULL,
	.outdata_ptr = NULL
};

static pthread_t file_thread_id;

/*****************************************************************************
 * Utility functions
 *****************************************************************************/

static int start_file_thread(void);

/**
 * Creates absolute time to use with pthread_cond_timedwait.
 * 
 * @param timeout_time[out]
 * @param timeout_ms[in] in ms
 */
static void get_timeout_time(struct timespec *timeout_time, uint32_t timeout_ms)
{
	struct timespec start_time, add_time;
	
	osaf_clock_gettime(CLOCK_REALTIME, &start_time);
	osaf_millis_to_timespec((uint64_t) timeout_ms, &add_time);
	osaf_timespec_add(&start_time, &add_time, timeout_time);
}

/*****************************************************************************
 * Thread handling
 *****************************************************************************/

/**
 * Thread:
 * Handle functions using file I/O
 * - Wait for request (cond_wait
 * - Handle request
 * - Return result
 * 
 * @param _init_params[in]
 * @return void
 */
static void *file_hndl_thread(void *noparam)
{
	int rc = 0;
	int hndl_rc = 0;
	int dummy;
	
	TRACE("%s - is started",__FUNCTION__);
	/* Configure cancellation so that thread can be canceled at any time */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &dummy);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dummy);
	
	osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK */
	while(1) {
		/* Wait for request */
		if (lgs_com_data.request_f == false) {
			rc = pthread_cond_wait(&request_cv, &lgs_ftcom_mutex); /* -> UNLOCK -> LOCK */
			if (rc != 0) osaf_abort(rc);
		} else {

			/* Handle the request.
			 * A handler is handling file operations that may 'hang'. Therefore
			 * the mutex cannot be locked since that may cause the main thread
			 * to hang.
			 * The handler is supposed to know the format of it's own in and
			 * out data. The data is given to the corresponding API that also is
			 * assumed to know the data format.
			 */
			osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK */
		
			/* Invoke requested handler function */
			switch (lgs_com_data.request_code)	{
			case LGSF_FILEOPEN:
				hndl_rc = fileopen_hdl(lgs_com_data.indata_ptr, 
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_FILECLOSE:
				hndl_rc = fileclose_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_DELETE_FILE:
				hndl_rc = delete_file_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_GET_NUM_LOGFILES:
				hndl_rc = get_number_of_log_files_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_MAKELOGDIR:
				hndl_rc = make_log_dir_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_WRITELOGREC:
				hndl_rc = write_log_record_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size,
						&lgs_com_data.timeout_f);
				break;
			case LGSF_CREATECFGFILE:
				hndl_rc = create_config_file_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_RENAME_FILE:
				hndl_rc = rename_file_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_CHECKPATH:
				hndl_rc = check_path_exists_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
				break;
			case LGSF_CHECKDIR:
				hndl_rc = path_is_writeable_dir_hdl(lgs_com_data.indata_ptr,
						lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
			default:
				break;
			}
			
			osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK */

			/* Handle answer flag and return data
			 * Note: This must be done after handler is done (handler may hang)
			 */
			lgs_com_data.request_f = false; /* Prepare to take a new request */
			lgs_com_data.request_code = LGSF_NOREQ;
			
			/* The following cannot be done if the API has timed out */
			if (lgs_com_data.timeout_f == false) {
				lgs_com_data.answer_f = true;
				lgs_com_data.return_code = hndl_rc;

				/* Signal the API function that we are done */
				rc = pthread_cond_signal(&answer_cv); 
				if (rc != 0) osaf_abort(rc);
			}
		}	
	} /* End while(1) */
}

/**
 * Start the file handling thread.
 * 
 * @param 
 */
static int start_file_thread(void)
{
	int rc = 0;
	int tbd_inpar=1;

	TRACE_ENTER();

	/* Init thread handling */
	rc = pthread_mutex_init(&lgs_ftcom_mutex, NULL);
	if (rc != 0) {
		LOG_ER("pthread_mutex_init fail %s",strerror(errno));
		goto done;
	}
	pthread_cond_init (&request_cv,NULL);
	if (rc != 0) {
		LOG_ER("pthread_cond_init fail %s",strerror(errno));
		goto done;
	}
	pthread_cond_init (&answer_cv,NULL);
	if (rc != 0) {
		LOG_ER("pthread_cond_init fail %s",strerror(errno));
		goto done;
	}

	/* Create thread. 
	 */
	rc = pthread_create(&file_thread_id, NULL, file_hndl_thread, (void *) &tbd_inpar);
	if (rc != 0) {
		LOG_ER("pthread_create fail %s",strerror(errno));
		goto done;
	}

done:
	TRACE_LEAVE2("rc=%d",rc);
	return rc;
}

/**
 * Initialize threaded file handling
 */
uint32_t lgs_file_init(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	
	/* Initiate timeouts from Log service IMM configuration object */
	max_waittime_ms = *(SaUint32T*) lgs_imm_logconf_get(
									LGS_IMM_FILEHDL_TIMEOUT, NULL);
	TRACE("max_waittime_ms = %d",max_waittime_ms);
	
	if (start_file_thread() != 0) {
		rc = NCSCC_RC_FAILURE;
	}
	
	TRACE_LEAVE2("rc=%d",rc);
	return rc;
}

/**
 * Generic file API handler
 * Handles everything that is generic thread handling for the APIs
 * 
 * @param apipar_in
 * @return A lgsf return code
 */
lgsf_retcode_t log_file_api(lgsf_apipar_t *apipar_in)
{
	lgsf_retcode_t api_rc = LGSF_SUCESS;
	int rc = 0;
	struct timespec timeout_time;
	
	osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK */
	
	/* If busy_f is true the file thread is hanging. In this case don't send
	 * a request.
	 */
	if (lgs_com_data.request_f == true) {
		api_rc = LGSF_BUSY;
		TRACE("%s - LGSF_BUSY",__FUNCTION__);
		goto api_exit;
	}
	
	/* Free request data before allocating new memeory */
	if (lgs_com_data.indata_ptr != NULL) {
		free(lgs_com_data.indata_ptr);
		lgs_com_data.indata_ptr = NULL;
	}
	if (lgs_com_data.outdata_ptr != NULL) {
		free(lgs_com_data.outdata_ptr);
		lgs_com_data.indata_ptr = NULL;
	}

	/* Allocate memory and enter data for a request */
	lgs_com_data.request_code = apipar_in->req_code_in;
	if (apipar_in->data_in_size != 0) {
		lgs_com_data.indata_ptr = malloc(apipar_in->data_in_size);
		if (lgs_com_data.indata_ptr == NULL) {
			LOG_ER("%s Could not allocate memory for in data", __FUNCTION__);
			api_rc = LGSF_FAIL;
			goto api_exit;
		}
		memcpy(lgs_com_data.indata_ptr, apipar_in->data_in, apipar_in->data_in_size);
	} else {
		lgs_com_data.indata_ptr = NULL;
	}
	
	if (apipar_in->data_out_size != 0) {
		lgs_com_data.outdata_ptr = malloc(apipar_in->data_out_size);
		if (lgs_com_data.outdata_ptr == NULL) {
			LOG_ER("%s Could not allocate memory for out data", __FUNCTION__);
			api_rc = LGSF_FAIL;
			goto api_exit;
		}
		*(char *) lgs_com_data.outdata_ptr = '\0';
	} else {
		lgs_com_data.outdata_ptr = NULL;
	}
	lgs_com_data.outdata_size = apipar_in->data_out_size;
	
	lgs_com_data.request_f = true;	/* We have a pending request */
	lgs_com_data.timeout_f = false;
	
	/* Wake up the thread */
	rc = pthread_cond_signal(&request_cv);
	if (rc != 0) osaf_abort(rc);
	
	/* Wait for an answer */
	get_timeout_time(&timeout_time, max_waittime_ms);
	
	while (lgs_com_data.answer_f == false) {
		rc = pthread_cond_timedwait(
				&answer_cv, &lgs_ftcom_mutex, &timeout_time); /* -> UNLOCK -> LOCK */
		if (rc == ETIMEDOUT) {
			TRACE("Timed out before answer");
			api_rc = LGSF_TIMEOUT;
			lgs_com_data.timeout_f = true; /* Inform thread about timeout */
			goto api_exit;
		} else if (rc != 0) {
			TRACE("pthread wait Failed - %s",strerror(rc));
			osaf_abort(rc);
		}
	}
	
	/* We have an answer
	 * NOTE!
	 * The out-data buffer 'par_out' must be big enough to handle the
	 * 'outdata'. It is assumed that the calling function knows the format of
	 * the returned data.
	 */
	apipar_in->hdl_ret_code_out = lgs_com_data.return_code;
	memcpy(apipar_in->data_out, lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);

	/* Prepare to take a new answer */
	lgs_com_data.answer_f = false;
	lgs_com_data.return_code = LGSF_NORETC;
	
api_exit:
	osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK */

	return api_rc;	
}

/**
 * Return lgsf return code as a string
 * @param rc
 * @return 
 */
char *lgsf_retcode_str(lgsf_retcode_t rc)
{
	static char errstr[256];
	
	switch (rc) {
	case LGSF_SUCESS:
		return "LGSF_SUCESS";
	case LGSF_BUSY:
		return "LGSF_BUSY";
	case LGSF_TIMEOUT:
		return "LGSF_TIMEOUT";
	case LGSF_FAIL:
		return "LGSF_FAIL";
	default:
		sprintf(errstr,"Unknown lgsf_retcode %d",rc);
		return errstr;
	}
	return "dummy"; 
}
