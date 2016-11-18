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
..............................................................................

..............................................................................

  DESCRIPTION:

  AvA handle database related definitions.
  
******************************************************************************
*/

#ifndef AMF_AGENT_AVA_HDL_H_
#define AMF_AGENT_AVA_HDL_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Pending callback list definitions 
 */

/* Pending callback rec */
typedef struct ava_pend_cbk_rec {
	struct ava_pend_cbk_rec *next;
	AVSV_AMF_CBK_INFO *cbk_info;	/* callbk info */
	uint32_t flag;		/* unused, dont remove it */

} AVA_PEND_CBK_REC;

/* Pending response rec */
typedef struct ava_pend_resp_rec {
	struct ava_pend_resp_rec *next;
	AVSV_AMF_CBK_INFO *cbk_info;	/* callbk info */
	uint32_t flag;		/* used to see, if we have responded
				   in callback itself */

} AVA_PEND_RESP_REC;

/* Pending response list */
typedef struct ava_resp_cbk {
	uint16_t num;		/* no of pending callbacks */
	AVA_PEND_RESP_REC *head;
	AVA_PEND_RESP_REC *tail;
} AVA_PEND_RESP;

/*
  Common structure for different initialize API versions(B.01.01, B.04.01 and B.04.2) 
  to remember callbacks passed by user application. This is for internal use only.
*/
typedef struct osaf_amf_cbk {
        SaAmfHealthcheckCallbackT                   saAmfHealthcheckCallback;
        SaAmfComponentTerminateCallbackT            saAmfComponentTerminateCallback;
        SaAmfCSISetCallbackT                        saAmfCSISetCallback;
        SaAmfCSIRemoveCallbackT                     saAmfCSIRemoveCallback;
        SaAmfProtectionGroupTrackCallbackT          saAmfProtectionGroupTrackCallback;
        SaAmfProtectionGroupTrackCallbackT_4        saAmfProtectionGroupTrackCallback_4; //Only in B.04.01
        SaAmfProxiedComponentInstantiateCallbackT   saAmfProxiedComponentInstantiateCallback;
        SaAmfProxiedComponentCleanupCallbackT       saAmfProxiedComponentCleanupCallback;
        SaAmfContainedComponentInstantiateCallbackT saAmfContainedComponentInstantiateCallback;
        SaAmfContainedComponentCleanupCallbackT     saAmfContainedComponentCleanupCallback;
        OsafCsiAttributeChangeCallbackT             osafCsiAttributeChangeCallback; //OpenSAF implementation and not from any spec.
} OsafAmfCallbacksT;

/* AvA handle database records */
typedef struct ava_hdl_rec_tag {
	NCS_PATRICIA_NODE hdl_node;	/* hdl-db tree node */

	uint32_t hdl;		/* AMF handle (derived from hdl-mngr) */

	struct osaf_amf_cbk reg_cbk;	/* callbacks registered by the application */

	SYSF_MBX callbk_mbx;       /* mailbox to hold the callback messages */ 
	AVA_PEND_RESP pend_resp;	/* list of pending AvSv Response */
} AVA_HDL_REC;

/* AvA handle database top level structure */
typedef struct ava_hdl_db_tag {
	NCS_PATRICIA_TREE hdl_db_anchor;	/* root of the handle db */
	uint32_t num;		/* no of hdl db records */
} AVA_HDL_DB;

/*** Macro Definitions */

/* Macro to find the hdl rec */
#define m_AVA_HDL_REC_FIND(db, hdl, o_rec) \
{ \
   uint32_t pat_hdl = (uint32_t)(hdl); \
   (o_rec) = (AVA_HDL_REC *)ncs_patricia_tree_get(&(db)->hdl_db_anchor, \
                                                  (uint8_t *)&pat_hdl); \
}

/* 
 * Macro to get the response record from the 
 * pending respose list. if input key is zero
 * then we will return tail element.
 */
#define m_AVA_HDL_PEND_RESP_GET(list, rec, key) \
            (rec) =  (AVA_PEND_CBK_REC *)ava_hdl_pend_resp_get((list) ,(key))

#define m_AVA_HDL_PEND_RESP_PUSH m_AVA_HDL_PEND_CBK_PUSH
#define m_AVA_HDL_PEND_RESP_POP(list, rec, key) \
            (rec) =  (AVA_PEND_CBK_REC *)ava_hdl_pend_resp_pop((list) ,(key))

/* 
 * Macro to push the callback record to the 
 * tail of the pending callbk list
 */
#define m_AVA_HDL_PEND_CBK_PUSH(list, rec) \
{ \
   if (!((list)->head)) { \
       assert(!((list)->num)); \
       (list)->head = (rec); \
   } \
   else \
   {\
      (list)->tail->next = (rec); \
   }\
   (list)->tail = (rec); \
   ((list)->num)++; \
}

/* 
 * Macro to pop the callback record from the 
 * head of the pending callbk list
 */
#define m_AVA_HDL_PEND_CBK_POP(list, o_rec) \
{ \
   if ((list)->head) { \
      (o_rec) = (list)->head; \
      (list)->head = (o_rec)->next; \
      (o_rec)->next = 0; \
      if ((list)->tail == (o_rec)) (list)->tail = 0; \
      (list)->num--; \
   } else (o_rec) = 0; \
}

/* 
 * Macro to determine if the required callbacks were supplied 
 * during saAmfInitialize() for component registration to succeed
 */
#define m_AVA_HDL_ARE_REG_CBKS_PRESENT(hdl_rec, proxy_comp_name) \
           ((!proxy_comp_name)?\
             hdl_rec->reg_cbk.saAmfCSISetCallback && \
             hdl_rec->reg_cbk.saAmfCSIRemoveCallback && \
             hdl_rec->reg_cbk.saAmfComponentTerminateCallback :\
             hdl_rec->reg_cbk.saAmfCSISetCallback && \
             hdl_rec->reg_cbk.saAmfCSIRemoveCallback && \
             hdl_rec->reg_cbk.saAmfComponentTerminateCallback &&\
             hdl_rec->reg_cbk.saAmfProxiedComponentInstantiateCallback &&\
             hdl_rec->reg_cbk.saAmfProxiedComponentCleanupCallback \
            )

/* Macro to determine if healthcheck callback was supplied during saAmfInitialize() */
#define m_AVA_HDL_IS_HC_CBK_PRESENT(hdl_rec) \
           ( hdl_rec->reg_cbk.saAmfHealthcheckCallback )

/* Macro to determine if pg callback was supplied during saAmfInitialize() */
#define m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec) \
           ( hdl_rec->reg_cbk.saAmfProtectionGroupTrackCallback  || \
             hdl_rec->reg_cbk.saAmfProtectionGroupTrackCallback_4)

/* define all flags here */
#define AVA_HDL_CBK_RESP_DONE                0x00000001
#define AVA_HDL_CBK_REC_IN_DISPATCH          0x00000002

/* Check if flag is set */
#define m_AVA_HDL_IS_CBK_RESP_DONE(x)        (((x)->flag) & AVA_HDL_CBK_RESP_DONE)
#define m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(x)        (((x)->flag) & AVA_HDL_CBK_REC_IN_DISPATCH)

   /* set the flag */
#define m_AVA_HDL_FLAG_SET(x, bitmap)        (((x)->flag) |= (bitmap))
#define m_AVA_HDL_CBK_RESP_DONE_SET(x)        m_AVA_HDL_FLAG_SET(x, AVA_HDL_CBK_RESP_DONE)
#define m_AVA_HDL_CBK_REC_IN_DISPATCH_SET(x)  m_AVA_HDL_FLAG_SET(x, AVA_HDL_CBK_REC_IN_DISPATCH)

   /* reset the flag */
#define m_AVA_HDL_FLAG_RESET(x, bitmap)       (((x)->flag) &= ~(bitmap))
#define m_AVA_HDL_CBK_RESP_DONE_RESET(x)        m_AVA_HDL_FLAG_RESET(x, AVA_HDL_CBK_RESP_DONE)
#define m_AVA_HDL_CBK_REC_IN_DISPATCH_RESET(x)  m_AVA_HDL_FLAG_RESET(x, AVA_HDL_CBK_REC_IN_DISPATCH)

/*** Extern function declarations ***/

struct ava_cb_tag;

uint32_t ava_hdl_cbk_param_add(struct ava_cb_tag *, AVA_HDL_REC *, AVSV_AMF_CBK_INFO *);

uint32_t ava_hdl_init(AVA_HDL_DB *);

void ava_hdl_del(struct ava_cb_tag *);

void ava_hdl_rec_del(struct ava_cb_tag *, AVA_HDL_DB *, AVA_HDL_REC **);

AVA_HDL_REC *ava_hdl_rec_add(struct ava_cb_tag *, AVA_HDL_DB *, const OsafAmfCallbacksT *);

uint32_t ava_hdl_cbk_dispatch(struct ava_cb_tag **, AVA_HDL_REC **, SaDispatchFlagsT);

void ava_hdl_cbk_rec_del(AVA_PEND_CBK_REC *);

AVA_PEND_RESP_REC *ava_hdl_pend_resp_pop(AVA_PEND_RESP *, SaInvocationT);

AVA_PEND_RESP_REC *ava_hdl_pend_resp_get(AVA_PEND_RESP *, SaInvocationT);

uint32_t ava_callback_ipc_init(AVA_HDL_REC *hdl_rec);

#ifdef  __cplusplus
}
#endif
#endif  // AMF_AGENT_AVA_HDL_H_
