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

/*****************************************************************************
  FILE NAME: mqsv_init.c

  DESCRIPTION: APIs used to initialize & Destroy the MQND Service Part.

  FUNCTIONS INCLUDED in this module:
  mqnd_lib_req ............ SE API to init and create PWE for MQND
  mqnd_lib_init ........... Function used to init MQND.  
  mqnd_lib_destroy ........ Function used to destroy MQNDND.
  mqnd_main_process ...........Process all the events posted to MQND.
******************************************************************************/

#include "mqnd.h"
#include "mqnd_imm.h"
#include <poll.h>
#define MQND_CLM_API_TIMEOUT 10000000000LL
uint32_t gl_mqnd_cb_hdl = 0;

#define FD_AMF 0
#define FD_CLM 1
#define FD_MBX 2
#define FD_IMM 3		/* Must be the last in the fds array */
static struct pollfd fds[4];
static nfds_t nfds = 4;
/* Static Function Declerations */
static uint32_t mqnd_extract_create_info(int argc, char *argv[], MQSV_CREATE_INFO *create_info);
static uint32_t mqnd_extract_destroy_info(int argc, char *argv[], MQSV_DESTROY_INFO *destroy_info);
static uint32_t mqnd_lib_init(MQSV_CREATE_INFO *info);
static uint32_t mqnd_lib_destroy(MQSV_DESTROY_INFO *info);
static uint32_t mqnd_compare_mqa_dest(uint8_t *valInDb, uint8_t *key);
static uint32_t mqnd_cb_db_init(MQND_CB *cb);
static uint32_t mqnd_cb_db_destroy(MQND_CB *cb);
static uint32_t mqnd_cb_namedb_destroy(MQND_CB *cb);
static uint32_t mqnd_cb_qevt_node_db_destroy(MQND_CB *cb);
static bool mqnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg);
static uint32_t mqnd_mqa_list_init(NCS_DB_LINK_LIST *mqalist);
void mqnd_main_process(uint32_t hdl);
static void mqnd_asapi_bind(MQND_CB *cb);
static void mqnd_asapi_unbind(void);

/****************************************************************************
 * Name          : mqnd_lib_req
 *
 * Description   : This is the SE API which is used to init/destroy or 
 *                 Create/destroy PWE's of MQND. This will be called by SBOM.
 *
 * Arguments     : req_info  - This is the pointer to the input information 
 *                             which SBOM gives.  
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t mqnd_lib_req(NCS_LIB_REQ_INFO *req_info)
{
	uint32_t rc = NCSCC_RC_FAILURE;
	MQSV_CREATE_INFO create_info;
	MQSV_DESTROY_INFO destroy_info;
	TRACE_ENTER();

	switch (req_info->i_op) {
	case NCS_LIB_REQ_CREATE:
		if (mqnd_extract_create_info(req_info->info.create.argc,
					     req_info->info.create.argv, &create_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = mqnd_lib_init(&create_info);
		break;
	case NCS_LIB_REQ_DESTROY:
		if (mqnd_extract_destroy_info(req_info->info.create.argc,
					      req_info->info.create.argv, &destroy_info) == NCSCC_RC_FAILURE) {
			break;
		}
		rc = mqnd_lib_destroy(&destroy_info);
		break;
	default:
		break;
	}
	if(rc != NCSCC_RC_SUCCESS)
	LOG_ER("Either library initialization or destroy failed");

	TRACE_LEAVE2("Returned with return code %d", rc);
	return (rc);
}

/****************************************************************************
 * Name          : mqnd_extract_create_info
 *
 * Description   : Used to extract the create information from argc & agrv
 *
 * Arguments     : argc  - This is the Number of arguments received.
 *                 argv  - string received.
 *                 create_info - Structure to be filled for initing the service.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_extract_create_info(int argc, char *argv[], MQSV_CREATE_INFO *create_info)
{

	memset(create_info, 0, sizeof(MQSV_CREATE_INFO));

	/* SUD:TBD Need to change this once we get these parameters in the argv */
	create_info->pool_id = NCS_HM_POOL_ID_COMMON;

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : mqnd_extract_destroy_info
 *
 * Description   : Used to extract the destroy information from argc & agrv
 *
 * Arguments     : argc  - This is the Number of arguments received.
 *                 argv  - string received.
 *                 destroy_info - Structure to be filled for initing the service.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_extract_destroy_info(int argc, char *argv[], MQSV_DESTROY_INFO *destroy_info)
{

	memset(destroy_info, 0, sizeof(MQSV_DESTROY_INFO));

	/* SUD:TBD Need to fill this once we get these parameters in the argv
	   destroy_info->i_vcard_id  */

	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : mqnd_lib_init
 *
 * Description   : This is the function which initalize the MQND libarary.
 *                 This function creates an IPC mail Box and spawns MQND
 *                 thread.
 *                 This function initializes the CB, handle manager, MDS, CPA 
 *                 and Registers with AMF with respect to the component Type 
 *                 (MQND).
 *
 * Arguments     : create_info: pointer to struct MQSV_CREATE_INFO.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_lib_init(MQSV_CREATE_INFO *info)
{
	MQND_CB *cb;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAmfHealthcheckKeyT healthy;
	char *health_key = NULL;
	SaAisErrorT amf_error;
	SaClmCallbacksT clm_cbk;
	SaClmClusterNodeT cluster_node;
	SaVersionT clm_version;
	char str_vector[10] = "";
	int fd;
	size_t bytes_read;

	TRACE_ENTER();

	/* allocate a CB  */
	cb = m_MMGR_ALLOC_MQND_CB;

	if (cb == NULL) {
		rc = NCSCC_RC_FAILURE;
		LOG_CR("CB Creation Failed");
		return rc;
	}
	memset(cb, 0, sizeof(MQND_CB));
	cb->hm_pool = info->pool_id;

	/* Set attributes of queue in global variable */
	fd = open("/proc/sys/kernel/msgmax", O_RDONLY);
	if (fd == -1) {
		LOG_ER("open msgmax failed - %s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	bytes_read = read(fd, str_vector, sizeof(str_vector));
	if (bytes_read == -1) {
		LOG_ER("read msgmax failed - %s", strerror(errno));
		close(fd);
		return NCSCC_RC_FAILURE;
	}

	cb->gl_msg_max_msg_size = atoi(str_vector);

	close(fd);

	fd = open("/proc/sys/kernel/msgmni", O_RDONLY);
	if (fd == -1) {
		LOG_ER("open msgmni failed - '%s'", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	bytes_read = read(fd, str_vector, sizeof(str_vector));
	if (bytes_read == -1) {
		LOG_ER("read msgmni failed - %s", strerror(errno));
		close(fd);
		return NCSCC_RC_FAILURE;
	}

	cb->gl_msg_max_no_of_q = atoi(str_vector);

	close(fd);

	fd = open("/proc/sys/kernel/msgmnb", O_RDONLY);
	if (fd == -1) {
		LOG_ER("open msgmnb failed - '%s'", strerror(errno));
		return NCSCC_RC_FAILURE;
	}

	bytes_read = read(fd, str_vector, sizeof(str_vector));
	if (bytes_read == -1) {
		LOG_ER("read msgmnb failed - %s", strerror(errno));
		close(fd);
		return NCSCC_RC_FAILURE;
	}

	cb->gl_msg_max_q_size = atoi(str_vector);

	close(fd);

	/* As there is no specific limit for priority queue size at present it is kept as max msg size */
	cb->gl_msg_max_prio_q_size = cb->gl_msg_max_q_size;

	/* END: Set attributes of queue in global variable */

	/* Init the EDU Handle */
	m_NCS_EDU_HDL_INIT(&cb->edu_hdl);

	if ((rc = mqnd_cb_db_init(cb)) != NCSCC_RC_SUCCESS) {
		LOG_ER("CB Database Init Failed");
		goto mqnd_cb_init_fail;
	}
	TRACE_1("CB Database Init Success");

	if ((cb->cb_hdl = ncshm_create_hdl(cb->hm_pool, NCS_SERVICE_ID_MQND, (NCSCONTEXT)cb)) == 0) {
		LOG_ER("CB Handle Creation Failed");
		rc = NCSCC_RC_FAILURE;
		goto mqnd_hdl_fail;
	}
	/* Store the handle in some global location */
	m_MQND_STORE_HDL(cb->cb_hdl);
	TRACE_1("CB Handle Creation Successfull");

	/* create a mail box */
	if ((rc = m_NCS_IPC_CREATE(&cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_CR("IPC Create Failed");
		goto mqnd_ipc_create_fail;
	}
	TRACE_1("IPC Create Successfull");

	/*create a shared memory segment to store queue stats in a node */
	if ((rc = mqnd_shm_create(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_4("Shared memory creation failed");
		goto mqnd_shm_create_fail;
	}

	/* Attach the IPC to mail box */
	if ((rc = m_NCS_IPC_ATTACH(&cb->mbx)) != NCSCC_RC_SUCCESS) {
		LOG_ER("IPC attatch failed");
		goto mqnd_ipc_att_fail;
	}
	TRACE_1("IPC attatch Successfull");

	/* Initialise with AMF service */
	if ((rc = mqnd_amf_init(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_4("Initialization With AMF Failed");
		goto amf_init_err;
	}
	TRACE_1("Initialization With AMF Successfull");

	/* register with the AMF service */
	if ((rc = mqnd_amf_register(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Registering  With AMF services Failed");
		goto amf_reg_err;
	}
	TRACE_1("Registration With AMF Successfull");

	/* B301 changes */

	m_MQSV_GET_AMF_VER(clm_version);
	clm_cbk.saClmClusterNodeGetCallback = NULL;
	clm_cbk.saClmClusterTrackCallback = mqnd_clm_cluster_track_cbk;

	rc = saClmInitialize(&cb->clm_hdl, &clm_cbk, &clm_version);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmInitialize failed with return code %d", rc);
		goto mqnd_mds_fail;
	}

	rc = saClmClusterNodeGet(cb->clm_hdl, SA_CLM_LOCAL_NODE_ID, MQND_CLM_API_TIMEOUT, &cluster_node);
	if (rc != SA_AIS_OK) {
		LOG_ER("saClmClusterNodeGet failed with return code %d", rc);
		goto mqnd_clm_fail;
	}

	cb->nodeid = cluster_node.nodeId;
	if (cb->nodeid == 0) {
		LOG_ER("Cluster Node ID is getting 0");
		assert(0);
	}

	cb->clm_node_joined = 1;

	/* Imm Initialization */
	amf_error = mqnd_imm_initialize(cb);
	if (amf_error != SA_AIS_OK) {
		/* we need to log here (NCSFL_SEV_ERROR, "Imm Initialization  Failed %u\n", amf_error); */
		LOG_ER("mqnd_imm_initialize Failed: %u", amf_error);
		goto amf_reg_err;
	}

	/* IMM Declare the Implementor */
	mqnd_imm_declare_implementer(cb);
	if ((rc = mqnd_mds_register(cb)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Reg with MDS Failed");
		goto mqnd_mds_fail;
	}
	TRACE_1("Reg with MDS Successfull");

	/* Bind with ASAPi */
	mqnd_asapi_bind(cb);

	/* Code for No Redundancy Support */
	/* start the AMF Health Check */
	memset(&healthy, 0, sizeof(healthy));
	health_key = getenv("MQSV_ENV_HEALTHCHECK_KEY");
	if (health_key == NULL) {
		strcpy((char *)healthy.key, "E5F6");
	} else {
		strncpy((char *)healthy.key, health_key, SA_AMF_HEALTHCHECK_KEY_MAX - 1);
	}
	healthy.keyLen = strlen((char *)healthy.key);

	amf_error = saAmfHealthcheckStart(cb->amf_hdl, &cb->comp_name, &healthy,
					  SA_AMF_HEALTHCHECK_AMF_INVOKED, SA_AMF_COMPONENT_RESTART);
	if (amf_error != SA_AIS_OK) {
		LOG_ER("saAmfHealthcheckStart Failed with error value %d", amf_error);
		goto amf_reg_err;
	}
	/* End of code for No Redundanccy Support */
	TRACE_1("saAmfHealthcheckStart Successfull");

	TRACE_1("Initialization Success");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

 mqnd_mds_fail:
	/* IMM FInalize. */
	saImmOiFinalize(cb->immOiHandle);
 mqnd_clm_fail:
	saClmFinalize(cb->clm_hdl);
 amf_reg_err:
	mqnd_amf_de_init(cb);
 amf_init_err:
	m_NCS_IPC_DETACH(&cb->mbx, mqnd_clear_mbx, cb);

 mqnd_ipc_att_fail:
	m_NCS_IPC_RELEASE(&cb->mbx, NULL);

 mqnd_shm_create_fail:
	mqnd_shm_destroy(cb);

 mqnd_ipc_create_fail:
	ncshm_destroy_hdl(NCS_SERVICE_ID_MQND, cb->cb_hdl);

 mqnd_hdl_fail:
	mqnd_cb_db_destroy(cb);
	mqnd_cb_namedb_destroy(cb);
	mqnd_cb_qevt_node_db_destroy(cb);
 mqnd_cb_init_fail:

	m_MMGR_FREE_MQND_CB(cb);

	TRACE_2("Init Failed with return code %u", rc);

	return (rc);
}

/****************************************************************************
 * Name          : mqnd_lib_destroy
 *
 * Description   : This is the function which destroy the mqnd libarary.
 *                 This function releases the Task and the IPX mail Box.
 *                 This function unregisters with AMF, destroies handle 
 *                 manager, CB and clean up all the component specific 
 *                 databases.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_lib_destroy(MQSV_DESTROY_INFO *info)
{
	MQND_CB *cb;
	uint32_t mqnd_hdl;
	uint32_t rc = NCSCC_RC_SUCCESS;
	SaAisErrorT saErr;
	TRACE_ENTER();

	mqnd_hdl = m_MQND_GET_HDL();
	if ((cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_MQND, mqnd_hdl))
	    == NULL) {
		TRACE_2("CB Take Failed");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	saClmFinalize(cb->clm_hdl);
	cb->clm_node_joined = 0;

	m_NCS_IPC_DETACH(&cb->mbx, mqnd_clear_mbx, cb);

	m_NCS_IPC_RELEASE(&cb->mbx, NULL);

	ncshm_destroy_hdl(NCS_SERVICE_ID_MQND, cb->cb_hdl);

	mqnd_mds_unregister(cb);

	mqnd_asapi_unbind();

	saErr = immutil_saImmOiImplementerClear(cb->immOiHandle);
	if (saErr != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerClear failed: err = %u", saErr);
	}

	ncshm_give_hdl(cb->cb_hdl);

	mqnd_cb_db_destroy(cb);

	mqnd_cb_namedb_destroy(cb);

	mqnd_cb_qevt_node_db_destroy(cb);

	m_MMGR_FREE_MQND_CB(cb);

	TRACE_1("Destroy Success");

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : mqnd_cb_db_init
 *
 * Description   : This is the function which initializes all the data 
 *                 structures and locks used belongs to MQND.
 *
 * Arguments     : cb  - MQND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_cb_db_init(MQND_CB *cb)
{
	NCS_PATRICIA_PARAMS params;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* initialze the queue handle tree */
	params.key_size = sizeof(SaMsgQueueHandleT);
	params.info_size = 0;
	if ((rc = ncs_patricia_tree_init(&cb->qhndl_db, &params))
	    != NCSCC_RC_SUCCESS) {
		TRACE_2("Controlblock Initialization Failed");
		return rc;
	}
	cb->is_qhdl_db_up = true;
	/* Initialise the queuename tree */
	params.key_size = sizeof(SaNameT);
	params.info_size = 0;
	if ((ncs_patricia_tree_init(&cb->qname_db, &params)) != NCSCC_RC_SUCCESS) {
		TRACE_2("Initialization of Queue database Failed");
		rc = NCSCC_RC_FAILURE;
	}

	cb->is_qname_db_up = true;
	/*Initialize the qevt node tree */
	params.key_size = sizeof(SaMsgQueueHandleT);
	params.info_size = 0;
	if ((rc = ncs_patricia_tree_init(&cb->q_transfer_evt_db, &params))
	    != NCSCC_RC_SUCCESS) {
		TRACE_2("Controlblock Initialization Failed");
		return rc;
	}
	cb->is_qevt_hdl_db_up = true;
	cb->mqa_dfrd_evt_rsp_list_head = NULL;
	rc = mqnd_mqa_list_init(&cb->mqa_list_info);

	TRACE_LEAVE();
	return (rc);
}

/****************************************************************************
 * Name          : mqnd_cb_db_destroy
 *
 * Description   : Destoroy the databases in CB
 *
 * Arguments     : cb  - Ifsv control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_cb_db_destroy(MQND_CB *cb)
{
	MQND_QUEUE_NODE *qnode;
	uint32_t qhdl = 0;
	ASAPi_OPR_INFO opr;
	TRACE_ENTER();

	/* Delete all the members in the qhdl_tree */
	mqnd_queue_node_getnext(cb, qhdl, &qnode);
	while (qnode) {
		qhdl = qnode->qinfo.queueHandle;

		/* Delete the queue */
		mqnd_mq_destroy(&qnode->qinfo);

		/*Invalidate the shm area of the queue */
		mqnd_shm_queue_ckpt_section_invalidate(cb, qnode);

		/* Delete the listener queue if it exists */
		mqnd_listenerq_destroy(&qnode->qinfo);

		/* Deregister the Queue with ASAPi */
		memset(&opr, 0, sizeof(ASAPi_OPR_INFO));
		opr.type = ASAPi_OPR_MSG;
		opr.info.msg.opr = ASAPi_MSG_SEND;

		/* Fill MDS info */
		opr.info.msg.sinfo.to_svc = NCSMDS_SVC_ID_MQD;
		opr.info.msg.sinfo.dest = cb->mqd_dest;
		opr.info.msg.sinfo.stype = MDS_SENDTYPE_SNDRSP;

		opr.info.msg.req.msgtype = ASAPi_MSG_DEREG;
		opr.info.msg.req.info.dereg.objtype = ASAPi_OBJ_QUEUE;
		opr.info.msg.req.info.dereg.queue = qnode->qinfo.queueName;

		/* Request the ASAPi */
		asapi_opr_hdlr(&opr);

		/* Delete the Queue Node */
		mqnd_queue_node_del(cb, qnode);

		/* Free the Queue Node */
		m_MMGR_FREE_MQND_QUEUE_NODE(qnode);

		mqnd_queue_node_getnext(cb, qhdl, &qnode);

	}

	if (cb->is_qhdl_db_up)
		ncs_patricia_tree_destroy(&cb->qhndl_db);

	cb->is_qhdl_db_up = false;

	/* deregister with the AMF */
	if (mqnd_amf_deregister(cb) != NCSCC_RC_SUCCESS)
		return NCSCC_RC_FAILURE;

	/* uninitialise with AMF */
	mqnd_amf_de_init(cb);

	TRACE_LEAVE();
	return (NCSCC_RC_SUCCESS);
}

/****************************************************************************
 * Name          : mqnd_cb_namedb_destroy
 *
 * Description   : Destoroy the name databases in CB
 *
 * Arguments     : cb  - MQND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_cb_namedb_destroy(MQND_CB *cb)
{
	MQND_QNAME_NODE *qnode = 0;
	SaNameT qname;
	TRACE_ENTER();

	memset(&qname, 0, sizeof(SaNameT));
	mqnd_qname_node_getnext(cb, qname, &qnode);
	while (qnode) {
		qname = qnode->qname;
		qname.length = m_NTOH_SANAMET_LEN(qnode->qname.length);
		mqnd_qname_node_del(cb, qnode);
		m_MMGR_FREE_MQND_QNAME_NODE(qnode);
		mqnd_qname_node_getnext(cb, qname, &qnode);
	}

	if (cb->is_qname_db_up)
		ncs_patricia_tree_destroy(&cb->qname_db);
	cb->is_qname_db_up = false;

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqnd_cb_qevt_node_db_destroy
 *
 * Description   : Destoroy the qevent node databases in CB
 *
 * Arguments     : cb  - MQND control block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqnd_cb_qevt_node_db_destroy(MQND_CB *cb)
{
	SaMsgQueueHandleT qhdl = 0;
	MQND_QTRANSFER_EVT_NODE *qevt_node = NULL;
	TRACE_ENTER();

	mqnd_qevt_node_getnext(cb, qhdl, &qevt_node);
	while (qevt_node) {
		qhdl = qevt_node->tmr.qhdl;
		mqnd_qevt_node_del(cb, qevt_node);
		m_MMGR_FREE_MQND_QTRANSFER_EVT_NODE(qevt_node);
		mqnd_qevt_node_getnext(cb, qhdl, &qevt_node);
	}

	if (cb->is_qevt_hdl_db_up)
		ncs_patricia_tree_destroy(&cb->q_transfer_evt_db);
	cb->is_qevt_hdl_db_up = false;
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqnd_compare_mqa_dest
 *
 * Description   : Compare the key with the value of mqa mds destination in the list.
 *
 * Arguments     : valInDb - MDS destination value in the database.
 *                 key     - MDS destination value from the function.
 *
 * Return Values : 0/1.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqnd_compare_mqa_dest(uint8_t *valInDb, uint8_t *key)
{
	MDS_DEST *dbVal, *inVal;
	TRACE_ENTER();

	uint32_t rc = NCSCC_RC_FAILURE;
	dbVal = (MDS_DEST *)valInDb;
	inVal = (MDS_DEST *)key;
	if (m_NCS_MDS_DEST_EQUAL(dbVal, inVal) == 1)
		return rc = 0;
	TRACE_LEAVE2("Returned with return code %u", rc);
	return rc;
}

/****************************************************************************
 * Name          : mqnd_mqa_list_init
 *
 * Description   : Initialize the mqa list which are up during restart.
 *
 * Arguments     : mqalist  - MQA Doubly linked list pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqnd_mqa_list_init(NCS_DB_LINK_LIST *mqalist)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	mqalist->order = NCS_DBLIST_ANY_ORDER;
	mqalist->cmp_cookie = mqnd_compare_mqa_dest;
	return rc;
}

/****************************************************************************
 * Name          : mqnd_clear_mbx
 *
 * Description   : This is the function which deletes all the messages from 
 *                 the mail box.
 *
 * Arguments     : arg     - argument to be passed.
 *                 msg     - Event start pointer.
 *
 * Return Values : true/false
 *
 * Notes         : None.
 *****************************************************************************/
static bool mqnd_clear_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
	return true;
}

/****************************************************************************
 * Name          : mqnd_main_process
 *
 * Description   : This is the function which is given as a input to the 
 *                 MQND task.
 *                 This function will be select of both the FD's (AMF FD and
 *                 Mail Box FD), depending on which FD has been selected, it
 *                 will call the corresponding routines.
 *
 * Arguments     : mbx  - This is the mail box pointer on which IfD/IfND is 
 *                        going to block.  
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void mqnd_main_process(uint32_t hdl)
{
	MQND_CB *cb = 0;
	NCS_SEL_OBJ mbx_fd;
	MQSV_EVT *evt = NULL;
	MQSV_DSEND_EVT *dsend_evt = NULL;
	SaSelectionObjectT amf_sel_obj, clm_sel_obj;
	SaAisErrorT clm_error, err;
	TRACE_ENTER();

	cb = ncshm_take_hdl(NCS_SERVICE_ID_MQND, hdl);
	if (!cb) {
		LOG_ER("Cb is NULL");
		return;
	}

	mbx_fd = ncs_ipc_get_sel_obj(&cb->mbx);
	err = saAmfSelectionObjectGet(cb->amf_hdl, &amf_sel_obj);

	if (err != SA_AIS_OK) {
		LOG_ER("saAmfSelectionObjectGet Failed with error %d", err);
		return;
	}
	TRACE_1("saAmfSelectionObjectGet Successfull");

	if (saClmSelectionObjectGet(cb->clm_hdl, &clm_sel_obj) != SA_AIS_OK) {
		LOG_ER("saClmSelectionObjectGet Failed");
		return;
	}

	fds[FD_AMF].fd = amf_sel_obj;
	fds[FD_AMF].events = POLLIN;
	fds[FD_CLM].fd = clm_sel_obj;
	fds[FD_CLM].events = POLLIN;
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;
	fds[FD_IMM].fd = cb->imm_sel_obj;
	fds[FD_IMM].events = POLLIN;
	
        if (saClmClusterTrack(cb->clm_hdl, (SA_TRACK_CURRENT | SA_TRACK_CHANGES), NULL) != SA_AIS_OK) {
		LOG_ER("saClmClusterTrack Failed");
                return;
        }


	while (1) {

		if (cb->immOiHandle != 0) {
			fds[FD_IMM].fd = cb->imm_sel_obj;
			fds[FD_IMM].events = POLLIN;
			nfds = FD_IMM + 1;
		} else {
			nfds = FD_IMM;
		}

		int ret = poll(fds, nfds, -1);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			LOG_ER("poll failed - %s", strerror(errno));
			break;
		}

		if (fds[FD_AMF].revents & POLLIN) {
			if (cb->amf_hdl != 0) {
				err = saAmfDispatch(cb->amf_hdl, SA_DISPATCH_ALL);
				if (err != SA_AIS_OK) {
					LOG_ER("saAmfDispatch Failed with error %d", err);
				}
			} else
				LOG_ER("cb->amf_hdl is NULL");
		}

		if (fds[FD_CLM].revents & POLLIN) {
			clm_error = saClmDispatch(cb->clm_hdl, SA_DISPATCH_ALL);
			if (clm_error != SA_AIS_OK) {
				LOG_ER("saClmDispatch Failed with error %d", clm_error);
			}
		}

		if (fds[FD_MBX].revents & POLLIN) {
			if (NULL != (evt = (MQSV_EVT *)m_NCS_IPC_NON_BLK_RECEIVE(&cb->mbx, evt))) {
				if (evt->evt_type == MQSV_NOT_DSEND_EVENT)
					mqnd_process_evt(evt);	/* now got the IPC mail box event */
				else if (evt->evt_type == MQSV_DSEND_EVENT) {
					dsend_evt = (MQSV_DSEND_EVT *)evt;
					mqnd_process_dsend_evt(dsend_evt);
				} else {
					/*Assert */
				}

			}
		}

		if (fds[FD_IMM].revents & POLLIN) {
			err = saImmOiDispatch(cb->immOiHandle, SA_DISPATCH_ONE);
			if (err == SA_AIS_ERR_BAD_HANDLE) {
				LOG_ER("saImmOiDispatch returned BAD_HANDLE with error %u", err);
				/*
				 ** Invalidate the IMM OI handle, this info is used in other
				 ** locations. E.g. giving TRY_AGAIN responses to a create and
				 ** close resource requests. That is needed since the IMM OI
				 ** is used in context of these functions.
				 */
				cb->immOiHandle = 0;
				/* Reinitiate IMM */
				mqnd_imm_reinit_bg(cb);
			} else if (err != SA_AIS_OK) {
				LOG_ER("saImmOiDispatch Failed with error %u", err); 
				break;
			}
		}
	}
	sleep(1);
	exit(EXIT_FAILURE);
	TRACE_LEAVE();
	return;
}

/****************************************************************************\
 PROCEDURE NAME : mqnd_asapi_bind

 DESCRIPTION    : This routines binds the MQND with the ASAPi layer

 ARGUMENTS      : cb  - MNQD Control block

 RETURNS        : none.
\*****************************************************************************/
static void mqnd_asapi_bind(MQND_CB *cb)
{
	ASAPi_OPR_INFO opr;
	opr.type = ASAPi_OPR_BIND;
	opr.info.bind.i_indhdlr = 0;
	opr.info.bind.i_mds_hdl = cb->my_mds_hdl;
	opr.info.bind.i_mds_id = NCSMDS_SVC_ID_MQND;
	opr.info.bind.i_my_id = NCS_SERVICE_ID_MQND;
	opr.info.bind.i_mydest = cb->my_dest;
	asapi_opr_hdlr(&opr);
	return;
}

/****************************************************************************\
 PROCEDURE NAME : mqnd_asapi_unbind

 DESCRIPTION    : This routines unbinds the MQND from the ASAPi layer

 ARGUMENTS      : cb  - MQND Control block

 RETURNS        : none.
\*****************************************************************************/
static void mqnd_asapi_unbind(void)
{
	ASAPi_OPR_INFO opr;
	opr.type = ASAPi_OPR_UNBIND;
	asapi_opr_hdlr(&opr);
	return;
}
