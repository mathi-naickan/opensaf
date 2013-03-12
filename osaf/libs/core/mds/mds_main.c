/*      -*- OpenSAF  -*-
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
 * Author(s): Emerson Network Power
 *
 */

#define _GNU_SOURCE
#include <configmake.h>

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:  MDS layer initialization and destruction entry points

******************************************************************************
*/

#include <pthread.h>
#include <ncsgl_defs.h>
#include "ncssysf_def.h"
#include "ncs_lib.h"
#include "ncssysf_tmr.h"
#include "mds_dl_api.h"
#include "mds_core.h"
#include "patricia.h"
#include "mds_log.h"
#include "ncs_main_pub.h"
#include "mds_dt_tcp.h"
#include "mds_dt_tcp_disc.h"
#include "mds_dt_tcp_trans.h"
#include <netdb.h>
#include <config.h>
#ifdef ENABLE_TIPC_TRANSPORT
#include "mds_dt_tipc.h"
#endif
#include "osaf_utility.h"
#include <configmake.h>
#define MDS_MDTM_CONNECT_PATH PKGLOCALSTATEDIR "/osaf_dtm_intra_server"

extern uint32_t mds_socket_domain;
void mds_init_transport(void);

/* MDS Control Block */
MDS_MCM_CB *gl_mds_mcm_cb = NULL;

/* See mds_core.h for description of this global variable. */
pthread_mutex_t gl_mds_library_mutex;

/* pthreads once control used for initialising s_mds_library_mutex */
static pthread_once_t s_mds_mutex_init_once_control = PTHREAD_ONCE_INIT;

/**
 * @brief Helper function to be used only by mds_mutex_init_once()
 *
 * This is a helper function used by the function mds_mutex_init_once(). It
 * should only be used in that function.
 */
static void mds_mutex_init_once_routine(void)
{
	pthread_mutexattr_t attr;
	int result;
	result = pthread_mutexattr_init(&attr);
	if (result != 0) osaf_abort(result);
	result = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	if (result != 0) osaf_abort(result);
	result = pthread_mutex_init(&gl_mds_library_mutex, &attr);
	if (result != 0) osaf_abort(result);
	result = pthread_mutexattr_destroy(&attr);
	if (result != 0) osaf_abort(result);
}

/**
 * @brief Initialise the MDS library mutex exactly once in a thread safe way
 *
 * This function initialises the MDS library mutex. It is thread safe and
 * guarantees that the mutex is initialised only once even if this function is
 * called from multiple threads concurrently. It is also safe to call this
 * function again when the MDS library mutex already has been initialised.
 */
static void mds_mutex_init_once(void)
{
	int result = pthread_once(&s_mds_mutex_init_once_control,
		mds_mutex_init_once_routine);
	if (result != 0) osaf_abort(result);
}

/* global Log level variable */
uint32_t gl_mds_log_level = 3;
uint32_t gl_mds_checksum = 0;

uint32_t MDS_QUIESCED_TMR_VAL = 80;
uint32_t MDS_AWAIT_ACTIVE_TMR_VAL = 18000;
uint32_t MDS_SUBSCRIPTION_TMR_VAL = 500;
uint32_t MDTM_REASSEMBLE_TMR_VAL = 500;
uint32_t MDTM_CACHED_EVENTS_TMR_VAL = 24000;

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*          mds_lib_req                         */

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

uint32_t mds_lib_req(NCS_LIB_REQ_INFO *req)
{
	char *p_field = NULL;
	uint32_t node_id = 0, cluster_id, mds_tipc_ref = 0;	/* this mds tipc ref is random num part of the TIPC id */
	uint32_t status = NCSCC_RC_SUCCESS;
	uint32_t destroy_ack_tmout;
	NCS_SEL_OBJ destroy_ack_obj;
	char *ptr;

	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:

		mds_mutex_init_once();
		osaf_mutex_lock_ordie(&gl_mds_library_mutex);

		if (gl_mds_mcm_cb != NULL) {
			syslog(LOG_ERR, "MDS_LIB_CREATE : MDS is already initialized");
			osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
			return NCSCC_RC_FAILURE;
		}

		/* Initialize mcm database */
		mds_mcm_init();

		/* Extract parameters from req and fill adest and pcon_id */

		/* Get Node_id */
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv, "NODE_ID=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("NODE_ID="), "%d", &node_id) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in NODE_ID argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		/* Get Cluster_id */
		p_field = NULL;
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
							    "CLUSTER_ID=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("CLUSTER_ID="), "%d", &cluster_id) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in CLUSTER_ID argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		/*  to use cluster id in mds prefix? */

		/* Get gl_mds_log_level */

		/*  setting MDS_LOG_LEVEL from environment variable if given */
		if ((ptr = getenv("MDS_LOG_LEVEL")) != NULL) {
			gl_mds_log_level = atoi(ptr);
		} else {

			p_field = NULL;
			p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
								    "MDS_LOG_LEVEL=");
			if (p_field != NULL) {
				if (sscanf(p_field + strlen("MDS_LOG_LEVEL="), "%d", &gl_mds_log_level) != 1) {
					syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in MDS_LOG_LEVEL argument\n");
					mds_mcm_destroy();
					osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
		}
		/* gl_mds_log_level consistency check */
		if (gl_mds_log_level > 5 || gl_mds_log_level < 1) {
			/* gl_mds_log_level specified is outside range so reset to Default = 3 */
			gl_mds_log_level = 3;
		}

		/* Get gl_mds_checksum */

		/*  setting MDS_CHECKSUM from environment variable if given */
		if ((ptr = getenv("MDS_CHECKSUM")) != NULL) {
			gl_mds_checksum = atoi(ptr);
		} else {

			p_field = NULL;
			p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
								    "MDS_CHECKSUM=");
			if (p_field != NULL) {
				if (sscanf(p_field + strlen("MDS_CHECKSUM="), "%d", &gl_mds_checksum) != 1) {
					syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in MDS_CHECKSUM argument\n");
					mds_mcm_destroy();
					osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
		}
		/* gl_mds_checksum consistency check */
		if (gl_mds_checksum != 1) {
			/* gl_mds_checksum specified is not 1 so reset to 0 */
			gl_mds_checksum = 0;
		}

	    /*****************************/
		/* Timer value Configuration */
	    /*****************************/

		/* Get Subscription timer value */
		p_field = NULL;
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
							    "SUBSCRIPTION_TMR_VAL=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("SUBSCRIPTION_TMR_VAL="), "%d", &MDS_SUBSCRIPTION_TMR_VAL) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in SUBSCRIPTION_TMR_VAL argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		/* Get Await Active timer value */
		p_field = NULL;
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
							    "AWAIT_ACTIVE_TMR_VAL=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("AWAIT_ACTIVE_TMR_VAL="), "%d", &MDS_AWAIT_ACTIVE_TMR_VAL) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in AWAIT_ACTIVE_TMR_VAL argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		/* Get Quiesced timer value */
		p_field = NULL;
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
							    "QUIESCED_TMR_VAL=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("QUIESCED_TMR_VAL="), "%d", &MDS_QUIESCED_TMR_VAL) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in QUIESCED_TMR_VAL argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		/* Get Reassembly timer value */
		p_field = NULL;
		p_field = (char *)ncs_util_search_argv_list(req->info.create.argc, req->info.create.argv,
							    "REASSEMBLE_TMR_VAL=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("REASSEMBLE_TMR_VAL="), "%d", &MDTM_REASSEMBLE_TMR_VAL) != 1) {
				syslog(LOG_ERR, "MDS_LIB_CREATE : Problem in REASSEMBLE_TMR_VAL argument\n");
				mds_mcm_destroy();
				osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			}
		}

		mds_init_transport();
		/* Invoke MDTM-INIT.  */
		status = mds_mdtm_init(node_id, &mds_tipc_ref);
		if (status != NCSCC_RC_SUCCESS) {
			/*  todo cleanup */
			return NCSCC_RC_FAILURE;
		}
		gl_mds_mcm_cb->adest = m_MDS_GET_ADEST_FROM_NODE_ID_AND_PROCESS_ID(node_id, mds_tipc_ref);

		/* Initialize logging */
		{
			char buff[50], pref[50];
			snprintf(buff, sizeof(buff), PKGLOGDIR "/mds.log");
			snprintf(pref, sizeof(pref), "<%u>", mds_tipc_ref);
			mds_log_init(buff, pref);
		}

		osaf_mutex_unlock_ordie(&gl_mds_library_mutex);

		break;

	case NCS_LIB_REQ_DESTROY:
		/* STEP 1: Invoke MDTM-Destroy. */
		/* mds_mdtm_destroy (); */
		/* STEP 2: Destroy MCM-CB;      */
		/* ncs_patricia_tree_destroy(&gl_mds_mcm_cb->vdest_list); */
		/* m_MMGR_FREE_MDS_CB(gl_mds_mcm_cb); */

		m_NCS_SEL_OBJ_CREATE(&destroy_ack_obj);

		/* Post a dummy message to MDS thread to guarantee that it
		   wakes up (and thereby sees the destroy_ind) */
		if (mds_destroy_event(destroy_ack_obj) == NCSCC_RC_FAILURE) {
			m_NCS_SEL_OBJ_DESTROY(destroy_ack_obj);
			return NCSCC_RC_FAILURE;
		}
		/* Wait for indication from MDS thread that it is ok to kill it */
		destroy_ack_tmout = 7000;	/* 70seconds */
		m_NCS_SEL_OBJ_POLL_SINGLE_OBJ(destroy_ack_obj, &destroy_ack_tmout);
		m_MDS_LOG_DBG("LIB_DESTROY:Destroy ack from MDS thread in %d ms", destroy_ack_tmout * 10);

		/* Take the lock before killing the thread */
		osaf_mutex_lock_ordie(&gl_mds_library_mutex);

		/* Now two things have happened 
		   (1) MDS thread has acked the destroy-event. So it will do no further things beyound
		   MDS unlock
		   (2) We have obtained MDS-Lock. So, even the un-lock by MDS thead is completed
		   Now we can proceed with the systematic destruction of MDS internal Data */

		/* Free the objects related to destroy-indication. The destroy mailbox event
		   will be automatically freed by MDS processing or during MDS mailbox
		   destruction. Since we will be destroying the MDS-thread, the following 
		   selection-object can no longer be accessed. Hence, it is safe and correct 
		   to destroy it now */
		m_NCS_SEL_OBJ_DESTROY(destroy_ack_obj);
		memset(&destroy_ack_obj, 0, sizeof(destroy_ack_obj));	/* Destroy info */

		/* Sanity check */
		if (gl_mds_mcm_cb == NULL) {
			syslog(LOG_ERR, "MDS_LIB_DESTROY : MDS is already Destroyed");
			osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
			return NCSCC_RC_FAILURE;
		}

		status = mds_mdtm_destroy();
		if (status != NCSCC_RC_SUCCESS) {
			/*  todo anything? */
		}
		status = mds_mcm_destroy();
		if (status != NCSCC_RC_SUCCESS) {
			/*  todo anything? */
		}

		/* Just Unlock the lock, Lock is never destroyed */
		osaf_mutex_unlock_ordie(&gl_mds_library_mutex);
		break;

	default:
		break;
	}

	return NCSCC_RC_SUCCESS;
}

void mds_init_transport(void)
{
	char *inet_or_unix = NULL;
	char *tipc_or_tcp = NULL;
	struct addrinfo *addr_list;
	int rc,rc1;
	struct stat sockStat;

	rc1 = stat(MDS_MDTM_CONNECT_PATH, &sockStat);
	if (rc1 == 0)  /* dtm intra server exists */
		tipc_or_tcp = "TCP";
	else
		tipc_or_tcp = "TIPC";


	if (strcmp(tipc_or_tcp, "TCP") == 0) {
		mds_mdtm_init = mds_mdtm_init_tcp;
		mds_mdtm_destroy = mds_mdtm_destroy_tcp;
		mds_mdtm_svc_subscribe = mds_mdtm_svc_subscribe_tcp;
		mds_mdtm_svc_unsubscribe = mds_mdtm_svc_unsubscribe_tcp;
		mds_mdtm_svc_install = mds_mdtm_svc_install_tcp;
		mds_mdtm_svc_uninstall = mds_mdtm_svc_uninstall_tcp;
		mds_mdtm_vdest_install = mds_mdtm_vdest_install_tcp;
		mds_mdtm_vdest_uninstall = mds_mdtm_vdest_uninstall_tcp;
		mds_mdtm_vdest_subscribe = mds_mdtm_vdest_subscribe_tcp;
		mds_mdtm_vdest_unsubscribe = mds_mdtm_vdest_unsubscribe_tcp;
		mds_mdtm_tx_hdl_register = mds_mdtm_tx_hdl_register_tcp;
		mds_mdtm_tx_hdl_unregister = mds_mdtm_tx_hdl_unregister_tcp;
		mds_mdtm_send = mds_mdtm_send_tcp;
		mds_mdtm_node_subscribe = mds_mdtm_node_subscribe_tcp;
		mds_mdtm_node_unsubscribe = mds_mdtm_node_unsubscribe_tcp;

		inet_or_unix = getenv("MDS_INTRANODE_TRANSPORT");

		if (NULL == inet_or_unix)
			inet_or_unix = "UNIX";

		if (strcmp(inet_or_unix, "TCP") == 0) {
			if ((rc = getaddrinfo("localhost", NULL,NULL, &addr_list)) != 0) {
				syslog(LOG_ERR,"MDTM:Unable to getaddrinfo() with errno = %d", errno);
				return;
			}
			mds_socket_domain = addr_list->ai_family; /* AF_INET or AF_INET6 */

		} else {
			mds_socket_domain = AF_UNIX;
		}
		return;
	}

#ifdef ENABLE_TIPC_TRANSPORT
	mds_mdtm_init = mdtm_tipc_init;
	mds_mdtm_destroy = mdtm_tipc_destroy;
	mds_mdtm_svc_subscribe = mds_mdtm_svc_subscribe_tipc;
	mds_mdtm_svc_unsubscribe = mds_mdtm_svc_unsubscribe_tipc;
	mds_mdtm_svc_install = mds_mdtm_svc_install_tipc;
	mds_mdtm_svc_uninstall = mds_mdtm_svc_uninstall_tipc;
	mds_mdtm_vdest_install = mds_mdtm_vdest_install_tipc;
	mds_mdtm_vdest_uninstall = mds_mdtm_vdest_uninstall_tipc;
	mds_mdtm_vdest_subscribe = mds_mdtm_vdest_subscribe_tipc;
	mds_mdtm_vdest_unsubscribe = mds_mdtm_vdest_unsubscribe_tipc;
	mds_mdtm_tx_hdl_register = mds_mdtm_tx_hdl_register_tipc;
	mds_mdtm_tx_hdl_unregister = mds_mdtm_tx_hdl_unregister_tipc;
	mds_mdtm_send = mds_mdtm_send_tipc;
	mds_mdtm_node_subscribe = mds_mdtm_node_subscribe_tipc;
	mds_mdtm_node_unsubscribe = mds_mdtm_node_unsubscribe_tipc;
	return;
#endif
	/* Should never come here */
	abort();

}
