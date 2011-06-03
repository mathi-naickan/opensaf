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

#include <configmake.h>

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:  MDS layer initialization and destruction entry points

******************************************************************************
*/

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

extern uns32 mds_socket_domain;
void mds_init_transport(void);

/* MDS Control Block */
MDS_MCM_CB *gl_mds_mcm_cb = NULL;

NCS_LOCK gl_lock;

NCS_LOCK *mds_lock(void)
{
	static int lock_inited = FALSE;
	/* Initialize the lock first time mds_lock() is called */
	if (!lock_inited) {
		m_NCS_LOCK_INIT(&gl_lock);
		lock_inited = TRUE;
	}
	return &gl_lock;
}

/* global Log level variable */
uns32 gl_mds_log_level = 2;
uns32 gl_mds_checksum = 0;

uns32 MDS_QUIESCED_TMR_VAL = 80;
uns32 MDS_AWAIT_ACTIVE_TMR_VAL = 18000;
uns32 MDS_SUBSCRIPTION_TMR_VAL = 500;
uns32 MDTM_REASSEMBLE_TMR_VAL = 500;
uns32 MDTM_CACHED_EVENTS_TMR_VAL = 24000;

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

/*          mds_lib_req                         */

/* ******************************************** */
/* ******************************************** */
/* ******************************************** */

uns32 mds_lib_req(NCS_LIB_REQ_INFO *req)
{
	char *p_field = NULL;
	uns32 node_id = 0, cluster_id, mds_tipc_ref = 0;	/* this mds tipc ref is random num part of the TIPC id */
	uns32 status = NCSCC_RC_SUCCESS;
	uns32 destroy_ack_tmout;
	NCS_SEL_OBJ destroy_ack_obj;
	char *ptr;

	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:

		/* Take lock : Initialization of lock done in mds_lock(), if in case it is not initialized */
		m_NCS_LOCK(mds_lock(), NCS_LOCK_WRITE);

		if (gl_mds_mcm_cb != NULL) {
			syslog(LOG_ERR, "MDS_LIB_CREATE : MDS is already initialized");
			m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
					m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
					return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
				}
			}
		}
		/* gl_mds_log_level consistency check */
		if (gl_mds_log_level > 5 || gl_mds_log_level < 1) {
			/* gl_mds_log_level specified is outside range so reset to Default = 3 */
			gl_mds_log_level = 2;
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
					m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
				m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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

		m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);

		break;

	case NCS_LIB_REQ_DESTROY:
		/* STEP 1: Invoke MDTM-Destroy. */
		/* mds_mdtm_destroy (); */
		/* STEP 2: Destroy MCM-CB;      */
		/* ncs_patricia_tree_destroy(&gl_mds_mcm_cb->vdest_list); */
		/* m_MMGR_FREE_MDS_CB(gl_mds_mcm_cb); */
		/* Destroy lock */
		/* m_NCS_LOCK_INIT(mds_lock); */

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
		m_NCS_LOCK(mds_lock(), NCS_LOCK_WRITE);

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
			m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
		m_NCS_UNLOCK(mds_lock(), NCS_LOCK_WRITE);
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
	int rc;


	tipc_or_tcp = getenv("MDS_TRANSPORT");

	if (NULL == tipc_or_tcp) {
#ifdef ENABLE_TIPC_TRANSPORT
		tipc_or_tcp = "TIPC";
#else
		tipc_or_tcp = "TCP";
#endif
	}

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
