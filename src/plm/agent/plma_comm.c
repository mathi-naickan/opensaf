#include "plma.h"


/***********************************************************************//**
* @brief	This routine is used to clean up the mailbox.
*
* @param[in]	arg - 
* @param[in]	msg - 
*
* @return	0 - If failure
* @return	1 - If success
***************************************************************************/
static bool plma_client_cleanup_mbx(NCSCONTEXT arg, NCSCONTEXT msg)
{
        PLMS_EVT *cbk_msg, *pnext;
        TRACE_ENTER();
        pnext = cbk_msg = (PLMS_EVT *)msg;
        while (pnext) {
                pnext = cbk_msg->next;
                free(cbk_msg);
                cbk_msg = pnext;
        }
        TRACE_LEAVE();
        return true;

}

/***********************************************************************//**
* @brief	This routine is used to initialize the queue for the callbacks.
*
* @param[in]	client_info - pointer to the client info.
*
* @return	NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
***************************************************************************/
uint32_t plma_callback_ipc_init(PLMA_CLIENT_INFO *client_info)
{
        uint32_t rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	
	if ((NCSCC_RC_SUCCESS == m_NCS_IPC_CREATE(&client_info->callbk_mbx))){
                if (m_NCS_IPC_ATTACH(&client_info->callbk_mbx) == 
							NCSCC_RC_SUCCESS) {
			
			return NCSCC_RC_SUCCESS;
		}
		LOG_ER("IPC ATTACH FAILED");
		m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
		LOG_ER("IPC CREATE FAILED");
		rc = NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE();
	return rc;
}




/****************************************************************************
  Name          : plma _callback_ipc_destroy

  Description   : This routine is used to destroy the queue for the callbacks

  Arguments     : client_info - pointer to the client info

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
void plma_callback_ipc_destroy(PLMA_CLIENT_INFO *client_info)
{
	TRACE_ENTER();
	/* detach the mail box */
	m_NCS_IPC_DETACH(&client_info->callbk_mbx, plma_client_cleanup_mbx, client_info);

	/* delete the mailbox */
	m_NCS_IPC_RELEASE(&client_info->callbk_mbx, NULL);
	TRACE_LEAVE();
	return;
}

