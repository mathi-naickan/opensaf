/*      -*- OpenSAF  -*-
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
 * Author(s): Emerson Network Power
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <poll.h>
#include <dlfcn.h>
#include <configmake.h>

#include <rda_papi.h>
#include <nid_api.h>
#include <daemon.h>
#include <saPlm.h>

#include "nid_api.h"
#include "plms.h"
#include "plms_hsm.h"
#include "plms_hrb.h"
#include "plms_mbcsv.h"

#define FD_USR1 0
#define FD_AMF 0
#define FD_MBCSV 1
#define FD_MBX 2
#define FD_IMM 3

static PLMS_CB  _plms_cb;
PLMS_CB *plms_cb = &_plms_cb;


HSM_HA_STATE hsm_ha_state = {PTHREAD_MUTEX_INITIALIZER, 
			     PTHREAD_COND_INITIALIZER,
			     SA_AMF_HA_ACTIVE};
HRB_HA_STATE hrb_ha_state = {PTHREAD_MUTEX_INITIALIZER, 
			     PTHREAD_COND_INITIALIZER,
			     SA_AMF_HA_ACTIVE};

PLMS_PRES_FUNC_PTR plms_HE_pres_state_op[SA_PLM_HE_PRES_STATE_MAX]
                                                [SA_PLM_HPI_HE_PRES_STATE_MAX];

PLMS_ADM_FUNC_PTR plm_HE_adm_state_op[SA_PLM_HE_ADMIN_STATE_MAX]
						[SA_PLM_ADMIN_OP_MAX];

PLMS_ADM_FUNC_PTR plm_EE_adm_state_op[SA_PLM_EE_ADMIN_STATE_MAX]
						[SA_PLM_ADMIN_OP_MAX];

/*********** FUNCTION PROTOTYPES *******************/
static void sigusr1_handler(int sig)
{
        (void)sig;
        signal(SIGUSR1, SIG_IGN);
        ncs_sel_obj_ind(&plms_cb->usr1_sel_obj);
}

static void usr2_sig_handler(int sig)
{
	PLMS_CB *cb = plms_cb;
	PLMS_EVT * evt;
	evt = (PLMS_EVT *)malloc(sizeof(PLMS_EVT));
	memset(evt,0,sizeof(PLMS_EVT));
	evt->req_res = PLMS_REQ;
	evt->req_evt.req_type = PLMS_DUMP_CB_EVT_T;
	(void)sig;
	/* Put it in PLMS's Event Queue */
	m_NCS_IPC_SEND(&cb->mbx, (NCSCONTEXT)evt, NCS_IPC_PRIORITY_HIGH);
	signal(SIGUSR2,usr2_sig_handler);
}

/****************************************************************************
 * Name          : plms_db_init
 *
 * Description   : This function initializes the data base of PLMS.
 *                 This includes initialization of all the required trees.
 *
 * Arguments     : PLMS_CB  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t plms_db_init()
{
	NCS_PATRICIA_PARAMS  params;
	PLMS_CB * cb = plms_cb; 

	/* Initialize base_he_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->base_he_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("base_he_info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize base_ee_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->base_ee_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("base_ee_info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize entity_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->entity_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("entity info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize  epath_to_entity_map_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaHpiEntityPathT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->epath_to_entity_map_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("epath_to_entity_map_info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize client_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaPlmHandleT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->client_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("client info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	/* Initialize entity_group_info tree */
	memset(&params,0,sizeof(NCS_PATRICIA_PARAMS));
	params.key_size = sizeof(SaPlmEntityGroupHandleT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->entity_group_info, &params)) != NCSCC_RC_SUCCESS) {
		LOG_ER("entity group info tree init failed");
		return NCSCC_RC_FAILURE;
	}

	return NCSCC_RC_SUCCESS;

}
/****************************************************************************
 * Name          : plms_hsm_hrb_init 
 *
 * Description   : This function initalizes HSM & HRB 
 *
 * Arguments     : create_info:
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
*****************************************************************************/
SaUint32T plms_hsm_hrb_init()
{
	PLMS_CB   *cb = plms_cb;
	SaUint32T (* hsm_init_func_ptr)(PLMS_HPI_CONFIG *hpi_cfg) = NULL;
	SaUint32T (* hrb_init_func_ptr)() = NULL;
	SaUint32T rc = NCSCC_RC_SUCCESS;
	void    *hpi_intf_handle = NULL;

	/* Load the HPI Interface library */
	hpi_intf_handle = dlopen("libplms_hpi.so.0",RTLD_LAZY);
	if( NULL == hpi_intf_handle ) {
		LOG_ER("dlopen() to load libplms_hpi.so failed with error %s",
								dlerror());
		return NCSCC_RC_FAILURE;
	}
	TRACE("Successfully loaded libplms_hpi.so using dlopen");
	cb->hpi_intf_hdl = hpi_intf_handle;

	/* Get the hsm Init func ptr */
	hsm_init_func_ptr = dlsym(cb->hpi_intf_hdl, "plms_hsm_initialize");
	if ( NULL == hsm_init_func_ptr ) {
		LOG_ER("dlsym() failed to get the hsm_init func_ptr,error %s",
								dlerror());
		return NCSCC_RC_FAILURE;
	}

	/* Initialize HSM */
	rc = (* hsm_init_func_ptr)(&cb->hpi_cfg);
	if ( NCSCC_RC_SUCCESS != rc ) {
		LOG_ER("plms_hsm_initialize failed");
		return rc;
	}
	TRACE("hsm_initialize success");

	/* Get the HRB Init func ptr */
	hrb_init_func_ptr = dlsym(cb->hpi_intf_hdl, "plms_hrb_initialize");
	if ( NULL == hrb_init_func_ptr ) {
		LOG_ER("dlsym() failed to get the hrb_init func_ptr,error %s",
							dlerror());
		return NCSCC_RC_FAILURE;
	}

	/* Initialize HRB */
	rc = (* hrb_init_func_ptr)();
	if ( NCSCC_RC_SUCCESS != rc ) {
		LOG_ER("plms_hrb_initialize failed");
		return rc;
	}
	TRACE("hrb_initialize success");
	return rc;
}

/****************************************************************************
 * Name          : plms_init
 *
 * Description   : This is the function which initalize the PLMS libarary.
 *                 This function creates an IPC mail Box and spawns PLMS 
 *                 threads.
 *                 This function initializes the CB,MDS and signal handlers
 *                 to be registered with AMF with respect to the component Type
 *                 (PLMS).
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t plms_init()
{
	PLMS_CB *cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaVersionT ntf_version = { 'A', 0x01, 0x01 };
	SaNtfCallbacksT ntf_callbacks = { NULL, NULL };

	TRACE_ENTER();
	
	/* Get CB first */
	cb = plms_cb;

	if (!cb) {
		/* Throw error */
		LOG_ER("NULL CB Pointer");
		return NCSCC_RC_FAILURE;
	}
	memset(cb,0,sizeof(PLMS_CB));

	/* Initialize the PLMS LOCK */
	m_NCS_LOCK_INIT(&cb->cb_lock);
	m_NCS_LOCK(&cb->cb_lock,NCS_LOCK_WRITE);

	if (ncs_agents_startup() != NCSCC_RC_SUCCESS) {
		LOG_ER("ncs_agents_startup FAILED");
                rc = NCSCC_RC_FAILURE;
		goto done;
	}

	 /* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	if ((rc = rda_get_role(&cb->ha_state)) != NCSCC_RC_SUCCESS) {
		LOG_ER("rda_get_role FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Initialize the database present in CB */
	if ( (rc == plms_db_init()) != NCSCC_RC_SUCCESS) {
		LOG_ER("plms_db initialization FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_CREATE FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("m_NCS_IPC_ATTACH FAILED");
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

        if ((rc = plms_mds_register()) != NCSCC_RC_SUCCESS) {
                LOG_ER("plms_mds_register FAILED %d", rc);
                rc = NCSCC_RC_FAILURE;
		goto done;
        }
        /* Initialise with the MBCSV service  */
        if ((rc = plms_mbcsv_register()) != NCSCC_RC_SUCCESS) {
                LOG_ER("plms_mbcsv_register FAILED %d", rc);
                rc = NCSCC_RC_FAILURE;
		goto done;
        }

        if ((rc = plms_mbcsv_chgrole()) != NCSCC_RC_SUCCESS) {
                LOG_ER("plms_mbcsv_chgrole FAILED %d", rc);
		rc = NCSCC_RC_FAILURE;
                goto done;
        }

	/* Initialize the IMM stuff */
	if (cb->ha_state == SA_AMF_HA_ACTIVE) {
		if ((plms_imm_intf_initialize()) != NCSCC_RC_SUCCESS) {
			LOG_ER("imm_intf initialization failed");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
	}

	if( cb->hpi_cfg.hpi_support && cb->ha_state == SA_AMF_HA_ACTIVE ) {
		rc = plms_hsm_hrb_init();
		if(NCSCC_RC_FAILURE == rc)
			goto done;
		cb->hpi_intf_up = true;
	}

	plms_tmr_handler_install();
	plms_he_pres_fsm_init(plms_HE_pres_state_op);
	plms_he_adm_fsm_init(plm_HE_adm_state_op);
	plms_ee_adm_fsm_init(plm_EE_adm_state_op);

	/* PLMC initialize */
	if ( !cb->hpi_cfg.hpi_support && cb->ha_state == SA_AMF_HA_ACTIVE) {
		rc = plmc_initialize(plms_plmc_connect_cbk,plms_plmc_udp_cbk,
		plms_plmc_error_cbk);
		if (rc) {
			LOG_ER("PLMC initialize failed.");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		cb->plmc_initialized = true;
	}

	/* NTF Initialization */
        rc = saNtfInitialize(&cb->ntf_hdl, &ntf_callbacks, &ntf_version);
        if (rc != SA_AIS_OK) {
                /* log the error code here */
		LOG_ER("NTF Initialization failed");
                goto done;
        }

	/* Create a selection object. This is used for amf initialization*/
        if ((rc = ncs_sel_obj_create(&cb->usr1_sel_obj)) != NCSCC_RC_SUCCESS) {
                LOG_ER("ncs_sel_obj_create failed");
		rc = NCSCC_RC_FAILURE;
                goto done;
        }

        /*
         ** Initialize a signal handler that will use the selection object.
         ** The signal is sent from our script when AMF does instantiate.
         */
        if ((signal(SIGUSR1, sigusr1_handler)) == SIG_ERR) {
                LOG_ER("signal USR1 failed: %s", strerror(errno));
                rc = NCSCC_RC_FAILURE;
                goto done;
        }
	/* Initialize a signal handler for debugging purpose */
	if ((signal(SIGUSR2, usr2_sig_handler)) == SIG_ERR) {
		LOG_ER("signal USR2 failed: %s", strerror(errno));
		rc = NCSCC_RC_FAILURE;
		goto done;
	}
									
        syslog(LOG_INFO, "Initialization Success, role %s",
               (cb->ha_state == SA_AMF_HA_ACTIVE) ? "ACTIVE" : "STANDBY");

done:
	m_NCS_UNLOCK(&cb->cb_lock,NCS_LOCK_WRITE);
         if (nid_notify("PLMD", rc, NULL) != NCSCC_RC_SUCCESS) {
                 LOG_ER("nid_notify failed");
                 rc = NCSCC_RC_FAILURE;
         }
	TRACE_LEAVE();
	return rc;
}


/****************************************************************************
 * Name          : plms_main
 *
 * Description   : This is the main function which initalize the PLMS libarary.
 *                 This function initializes the CB,MDS and signal handlers
 *                 to be registered with AMF with respect to the component Type
 *                 (PLMS).
 *
 * Arguments     : argc(in), argv(in) 
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
int main(int argc, char *argv[])
{
	NCS_SEL_OBJ mbx_fd;
	struct pollfd fds[4];
	SaAisErrorT error;
	SaInt8T num_fds;

	daemonize(argc, argv);

	if ((plms_init()) != NCSCC_RC_SUCCESS) {
		TRACE("PLMS initialization failed");
		goto done;
	}

	mbx_fd = m_NCS_IPC_GET_SEL_OBJ(&plms_cb->mbx);
	
	/* Wait on all the FDs */
	/* Set up all file descriptors to listen to */
        fds[FD_USR1].fd = plms_cb->usr1_sel_obj.rmv_obj;
        fds[FD_USR1].events = POLLIN;
        fds[FD_MBCSV].fd = plms_cb->mbcsv_sel_obj;
        fds[FD_MBCSV].events = POLLIN;
        fds[FD_MBX].fd = mbx_fd.rmv_obj;
        fds[FD_MBX].events = POLLIN;

        while (1) {
		if (plms_cb->oi_hdl != 0) {
			fds[FD_IMM].fd = plms_cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			num_fds = 4;
		} else {
			num_fds = 3;
		}
                int ret = poll(fds, num_fds, -1);

                if (ret == -1) {
                        if (errno == EINTR)
	                        continue;
                        LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_MBCSV].revents & POLLIN) {
			if (plms_mbcsv_dispatch() != NCSCC_RC_SUCCESS) {
				LOG_ER("PLMS MBCSv Dispatch Failed");
				break;
			}
		}

                if (fds[FD_MBX].revents & POLLIN)
                        plms_process_event();

		if (fds[FD_AMF].revents & POLLIN) {
			if (plms_cb->amf_hdl != 0) {
				error = saAmfDispatch(plms_cb->amf_hdl, SA_DISPATCH_ALL);
                                if (error != SA_AIS_OK) {
	                                LOG_ER("PLMS saAmfDispatch failed: %u", error);
                                        break;
		                }
			} else {
				TRACE("SIGUSR1 event rec");
				ncs_sel_obj_rmv_ind(&plms_cb->usr1_sel_obj, true,true);
				ncs_sel_obj_destroy(&plms_cb->usr1_sel_obj);
				
				if (plms_amf_register() != NCSCC_RC_SUCCESS)
					break;

				TRACE("PLMS AMF Initialization SUCCESS......");
				fds[FD_AMF].fd = plms_cb->amf_sel_obj;
			}
		}
		if (plms_cb->oi_hdl != 0) {
			if (fds[FD_IMM].revents & POLLIN) {
				error = saImmOiDispatch(plms_cb->oi_hdl, 
							SA_DISPATCH_ONE);
				if (error == SA_AIS_ERR_BAD_HANDLE) {
					TRACE("main: saImmOiDispatch returned BAD_HANDLE");

					/* 
					 * Invalidate the IMM OI handle, this info is used in other
					 * locations. E.g. giving TRY_AGAIN responses to a create and
					 * close app stream requests. That is needed since the IMM OI
					 * is used in context of these functions.
					 * 
					 * Also closing the handle. Finalize is ok with a bad handle
					 * that is bad because it is stale and this actually clears
					 * the handle from internal agent structures.  In any case
					 * we ignore the return value from Finalize here.
					 */
					saImmOiFinalize(plms_cb->oi_hdl);
					plms_cb->oi_hdl = 0;

					/* 
					 * Skip the IMM file descriptor in next poll(), IMM fd must
					 * be the last in the fd array.
					 */
					num_fds--;

					/* Initiate IMM reinitializtion in the background */
					plm_imm_reinit_bg(plms_cb);

					
				} else if (error != SA_AIS_OK) {
					LOG_ER("saImmOiDispatch FAILED: %u", error);
					break;
				}
			}
		}
	}

done:
        LOG_ER("Failed, exiting...");
        TRACE_LEAVE();
        exit(1);
}
