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

#include "log/logd/lgs_file.h"
#include "log/logd/lgs_filehdl.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#include "base/logtrace.h"
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "base/ncsgl_defs.h"

#include "log/logd/lgs.h"
#include "base/osaf_utility.h"
#include "base/osaf_time.h"
#include "lgs_config.h"

pthread_mutex_t lgs_ftcom_mutex;        /* For locking communication */
static pthread_cond_t request_cv;       /* File thread waiting for request */
static pthread_cond_t answer_cv;        /* API waiting for answer (timed) */

struct file_communicate {
  bool request_f; /* True if pending request */
  bool answer_f;  /* True if pending answer */
  bool timeout_f; /* True if API has got a timeout. Thread shall not answer */
  lgsf_treq_t request_code;       /* Request code from API */
  int return_code;        /* Return code from handlers */
  void *indata_ptr;       /* In-parameters for handlers */
  size_t outdata_size;
  void *outdata_ptr;      /* Out data from handlers */

  file_communicate() :
    request_f(false),
    answer_f(false),
    timeout_f(false),
    request_code(LGSF_NOREQ),
    return_code(LGSF_NORETC),
    indata_ptr(NULL),
    outdata_size(0),
    outdata_ptr(NULL) {
  }
};

/* Used for synchronizing and transfer of data ownership between main thread
 * and file thread.
 */
static struct file_communicate lgs_com_data;

static pthread_t file_thread_id;

/*****************************************************************************
 * Utility functions
 *****************************************************************************/

static int start_file_thread();

/**
 * Creates absolute time to use with pthread_cond_timedwait.
 *
 * @param timeout_time[out]
 * @param timeout_ms[in] in ms
 */
static void get_timeout_time(struct timespec *timeout_time, uint32_t timeout_ms) {
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
static void *file_hndl_thread(void *noparam) {
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
       * Note:
       * Mutex is unlocked inside the _hdl functions while calling
       * file I/O functions. Mutex is locked when _hdl function returns.
       */

      /* Invoke requested handler function */
      switch (lgs_com_data.request_code)      {
        case LGSF_FILEOPEN:
          hndl_rc = fileopen_hdl(lgs_com_data.indata_ptr,
                                 lgs_com_data.outdata_ptr, lgs_com_data.outdata_size,
                                 &lgs_com_data.timeout_f);
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
          break;
        case LGSF_OWN_LOGFILES:
          hndl_rc = own_log_files_by_group_hdl(lgs_com_data.indata_ptr,
                                               lgs_com_data.outdata_ptr, lgs_com_data.outdata_size);
          break;
        case LGSF_GET_FILE_PAR:
          hndl_rc = lgs_get_file_params_hdl(lgs_com_data.indata_ptr,
                                            lgs_com_data.outdata_ptr,
                                            lgs_com_data.outdata_size);
          break;
        default:
          break;
      }

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
static int start_file_thread() {
  int rc = 0;
  int tbd_inpar=1;

  TRACE_ENTER();

  /* Init thread handling */
  rc = pthread_mutex_init(&lgs_ftcom_mutex, NULL);
  if (rc != 0) {
    LOG_ER("pthread_mutex_init fail %s",strerror(errno));
    goto done;
  }
  rc = pthread_cond_init (&request_cv,NULL);
  if (rc != 0) {
    LOG_ER("pthread_cond_init fail %s",strerror(errno));
    goto done;
  }
  rc = pthread_cond_init (&answer_cv,NULL);
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
uint32_t lgs_file_init() {
  uint32_t rc = NCSCC_RC_SUCCESS;

  TRACE_ENTER();

  if (start_file_thread() != 0) {
    rc = NCSCC_RC_FAILURE;
  }

  TRACE_LEAVE2("rc=%d",rc);
  return rc;
}

/*
 * List for saving file descriptors.
 * A file descriptor is added to the list if a close request fails because
 * the file thread is busy. If the thread is busy the close function is never
 * called. A new attempt to close is made the next time the file API is used.
 */

typedef struct fd_list {
  struct fd_list *fd_next_p;
  int32_t fd;
} fd_list_t;

static fd_list_t *fd_first_p = NULL;
static fd_list_t *fd_last_p = NULL;

/**
 * Add stream file descriptor to list
 * @param fd
 */
void lgs_fd_list_add(int32_t fd) {
  fd_list_t *fd_new_p;

  TRACE_ENTER2("fd = %d", fd);

  fd_new_p = static_cast<fd_list_t *>(malloc(sizeof(fd_list_t)));
  osafassert(fd_new_p);

  if (fd_first_p == NULL) {
    /* First in list */
    fd_first_p = fd_new_p;
    fd_last_p = fd_new_p;
  } else {
    fd_last_p->fd_next_p = fd_new_p;
    fd_last_p = fd_new_p;
  }

  fd_new_p->fd = fd;
  fd_new_p->fd_next_p = NULL;

  TRACE_LEAVE();
}

/**
 * Get and remove file descriptor from list
 * @return fd If list empty return -1
 */
int32_t lgs_fd_list_get() {
  int32_t r_fp;
  fd_list_t *fd_rem_p;

  if (fd_first_p == NULL) {
    /* List empty */
    return -1;
  }

  r_fp = fd_first_p->fd; /* fd to return */
  fd_rem_p = fd_first_p;
  fd_first_p = fd_rem_p->fd_next_p;
  if (fd_first_p == NULL) {
    /* List is empty */
    fd_last_p = NULL;
  }

  free(fd_rem_p);

  return r_fp;
}

/**
 * Generic file API handler
 * Handles everything that is generic thread handling for the APIs
 *
 * @param apipar_in
 * @return A lgsf return code
 */
lgsf_retcode_t log_file_api(lgsf_apipar_t *apipar_in) {
  lgsf_retcode_t api_rc = LGSF_SUCESS;
  int rc = 0;
  struct timespec timeout_time;
  int fd = 0;
  /* If this flag is true always return LGSF_BUSY */
  bool busy_close_flag = false;
  uint32_t max_waittime_ms = 0;

  osaf_mutex_lock_ordie(&lgs_ftcom_mutex); /* LOCK */

  /* If busy_f is true the file thread is hanging. In this case don't send
   * a request.
   */
  if (lgs_com_data.request_f == true) {
    api_rc = LGSF_BUSY;
    TRACE("%s - LGSF_BUSY",__FUNCTION__);
    goto api_exit;
  }

  /* Close files in fd list if any
   * The original request is overridden. Always return busy
   */
  if ((fd = lgs_fd_list_get()) != -1) {
    TRACE("Closing files in fd list. fd = %d, replaced req code = %d",
          fd, apipar_in->req_code_in);
    apipar_in->req_code_in = LGSF_FILECLOSE;
    apipar_in->data_in_size = sizeof(int);
    apipar_in->data_in = (void*) &fd;
    apipar_in->data_out_size = 0;
    apipar_in->data_out = NULL;
    busy_close_flag = true;
  }

  /* Free request data before allocating new memeory */
  if (lgs_com_data.indata_ptr != NULL) {
    free(lgs_com_data.indata_ptr);
    lgs_com_data.indata_ptr = NULL;
  }
  if (lgs_com_data.outdata_ptr != NULL) {
    free(lgs_com_data.outdata_ptr);
    lgs_com_data.outdata_ptr = NULL;
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

  lgs_com_data.request_f = true;  /* We have a pending request */
  lgs_com_data.timeout_f = false;

  /* Wake up the thread */
  rc = pthread_cond_signal(&request_cv);
  if (rc != 0) osaf_abort(rc);

  /* Wait for an answer */
  max_waittime_ms = *static_cast<const SaUint32T *>(lgs_cfg_get(LGS_IMM_FILEHDL_TIMEOUT));
  get_timeout_time(&timeout_time, max_waittime_ms);

  while (lgs_com_data.answer_f == false) {
    rc = pthread_cond_timedwait(
        &answer_cv, &lgs_ftcom_mutex, &timeout_time); /* -> UNLOCK -> LOCK */
    if ((rc == ETIMEDOUT) && (lgs_com_data.answer_f == false)) {
      TRACE("Timed out before answer");
      api_rc = LGSF_TIMEOUT;
      lgs_com_data.timeout_f = true; /* Inform thread about timeout */
      goto api_timeout;
    } else if ((rc != 0) && (rc != ETIMEDOUT)) {
      LOG_ER("pthread wait Failed - %s",strerror(rc));
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

api_timeout:
  /* Prepare to take a new answer */
  lgs_com_data.answer_f = false;
  lgs_com_data.return_code = LGSF_NORETC;

api_exit:
  osaf_mutex_unlock_ordie(&lgs_ftcom_mutex); /* UNLOCK */

  if (busy_close_flag == true) {
    api_rc = LGSF_BUSY;
  }

  return api_rc;
}

/**
 * Return lgsf return code as a string
 * @param rc
 * @return
 */
char *lgsf_retcode_str(lgsf_retcode_t rc) {
  static char errstr[256];

  switch (rc) {
    case LGSF_SUCESS:
      return const_cast<char *>("LGSF_SUCESS");
    case LGSF_BUSY:
      return const_cast<char *>("LGSF_BUSY");
    case LGSF_TIMEOUT:
      return const_cast<char *>("LGSF_TIMEOUT");
    case LGSF_FAIL:
      return const_cast<char *>("LGSF_FAIL");
    default:
      sprintf(errstr,"Unknown lgsf_retcode %d",rc);
      return errstr;
  }
}
