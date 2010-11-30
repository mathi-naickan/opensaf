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

******************************************************************************
*/

#ifndef MDS_PAPI_H
#define MDS_PAPI_H

#include "ncsgl_defs.h"
#include "ncs_osprm.h"
#include "ncs_ubaid.h"

#include "saAis.h"
#include "saAmf.h"
#include "ncs_saf.h"
#include "ncssysf_ipc.h"

#ifdef  __cplusplus
extern "C" {
#endif

/***********************************************************************\
  INDEX of this FILE
    1) Basic data types
    2) SVC APIs into MDS   (requests into MDS by Services(clients) )
    3) SVC APIs out of MDS (indications out of MDS to Services(clients) )
    4) Definitions to talk to MDS as a service provider over the SPRR
       interface.
    5) MDS inventory dump functions (meant for debugging/diagnostics)
    6) Utilities to manipulate MDS_DEST

\**********************************************************************/

/**********************************************************************\
   1) Basic data type
\**********************************************************************/

/* The version of MDS that this header ships with */
#define MDS_MAJOR_VERSION 2
#define MDS_MINOR_VERSION 1

	typedef NCS_NODE_ID NODE_ID;	/* Node ID to distinguish among blades */
	typedef uns32 MDS_SVC_ID;	/* MDS Service ID replaces SS_SVC_ID(an uns8) */
	typedef uns16 PW_ENV_ID;	/* Private World Environment ID        */

/* MDS-DEST is defined globally in "ncsgl_defs.h" */
	typedef uns64 V_DEST_QA;
	typedef uns16 MDS_VDEST_ID;

/* Typedefs for the 64-BIT Changes */
	typedef uns32 MDS_HDL;
	typedef uns64 MDS_CLIENT_HDL;

/* Added for the version/in-service changes */
	typedef uns8 MDS_SVC_PVT_SUB_PART_VER;	/*MDS svc private sub part ver */
	typedef uns16 MDS_CLIENT_MSG_FORMAT_VER;	/* Message format version included in mds header */

/* MDS return Types */
	typedef enum {
		MDS_RC_SUCCESS = NCSCC_RC_SUCCESS,	/* Operation with mds has been success */
		MDS_RC_FAILURE = NCSCC_RC_FAILURE,	/* Operation with mds has been failure */
		MDS_RC_TIMEOUT = NCSCC_RC_REQ_TIMOUT,	/* Timeout occurred in receiving the response/ack for the message sent to the destination */
		MDS_RC_MSG_NO_BUFFERING = 100,	/* Msgs are being dropped(not queued in MDS), as the destination is in noactive state */
	} MDS_RC_TYPES;

	MDS_DEST mds_fetch_qa(void);
#define V_DEST_QA_1 (V_DEST_QA)mds_fetch_qa()	/* For backward compatibility */
#define V_DEST_QA_2 (V_DEST_QA)mds_fetch_qa()	/* For backward compatibility */

	typedef enum {
		NCSMDS_NONE = 0,	/* Null change. An application will never see it */

		/* Events specific to simple subscription */
		NCSMDS_NO_ACTIVE = 1,	/* The only active has disappeared. Awaiting a new one */
		NCSMDS_NEW_ACTIVE = 2,	/* A new ACTIVE has appeared                     */

		/* Events applicable to simple as well as redundant view but with
		   different interpretation. 
		 */
		NCSMDS_UP = 3,
		NCSMDS_DOWN = 4,
		NCSMDS_GONE = NCSMDS_DOWN,

		NCSMDS_RED_UP = 5,
		NCSMDS_RED_DOWN = 6,
		/* Events applicable to simple as well as redundant view but with
		   identical interpretation.
		 */
		NCSMDS_CHG_ROLE = 7,	/* Destination just changed role (A->S) | (A->S)) */
	} NCSMDS_CHG;
	
	typedef enum {
                NCSMDS_NODE_UP = 0,
                NCSMDS_NODE_DOWN = 1,
	} NCSMDS_NODE_CHG;
		

/* Virtual destination role                                               */
	typedef enum {
		V_DEST_RL_QUIESCED = SA_AMF_HA_QUIESCED,
		V_DEST_RL_ACTIVE = SA_AMF_HA_ACTIVE,
		V_DEST_RL_STANDBY = SA_AMF_HA_STANDBY,
		V_DEST_RL_INVALID
	} V_DEST_RL;

	typedef enum {
		/* More than 1 VDEST instance allowed to be ACTIVE simultaneously */
		NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN = 1,
		NCS_VDEST_TYPE_MIN = NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,

		/* Exactly one of a max M VDEST instances allowed to be active */
		NCS_VDEST_TYPE_MxN = 2,

		NCS_VDEST_TYPE_DEFAULT = NCS_VDEST_TYPE_MxN,
		NCS_VDEST_TYPE_MAX
	} NCS_VDEST_TYPE;

#define  NCSMDS_SYS_EV_REMOTE_NODE_FLTR  (EVT_FLTR)0x00001000
	typedef uns32 EVT_FLTR;

/**************************************************************************
 * SVC_ID  Service Identifier: A well known name that a service goes by
 */
	typedef enum ncsmds_svc_id {
		NCSMDS_SVC_ID_NCSMIN = 1,
		/* BEGIN: These are NCS internal use service-id definitions */
		NCSMDS_SVC_ID_DTS = NCSMDS_SVC_ID_NCSMIN,
		NCSMDS_SVC_ID_DTA = 2,
		NCSMDS_SVC_ID_GLA = 3,
		NCSMDS_SVC_ID_GLND = 4,
		NCSMDS_SVC_ID_GLD = 5,
		NCSMDS_SVC_ID_VDA = 6,
		NCSMDS_SVC_ID_EDS = 7,
		NCSMDS_SVC_ID_EDA = 8,
		NCSMDS_SVC_ID_MQA = 9,
		NCSMDS_SVC_ID_MQND = 10,
		NCSMDS_SVC_ID_MQD = 11,
		NCSMDS_SVC_ID_AVD = 12,
		NCSMDS_SVC_ID_AVND = 13,
		NCSMDS_SVC_ID_AVA = 14,
		NCSMDS_SVC_ID_CLA = 15,
		NCSMDS_SVC_ID_CPD = 16,
		NCSMDS_SVC_ID_CPND = 17,
		NCSMDS_SVC_ID_CPA = 18,
		NCSMDS_SVC_ID_MBCSV = 19,
		NCSMDS_SVC_ID_LGS = 20,
		NCSMDS_SVC_ID_LGA = 21,
		NCSMDS_SVC_ID_AVND_CNTLR = 22,
		NCSMDS_SVC_ID_GFM = 23,
		NCSMDS_SVC_ID_IMMD = 24,
		NCSMDS_SVC_ID_IMMND = 25,
		NCSMDS_SVC_ID_IMMA_OM = 26,
		NCSMDS_SVC_ID_IMMA_OI = 27,
		NCSMDS_SVC_ID_NTFS = 28,
		NCSMDS_SVC_ID_NTFA = 29,
    		NCSMDS_SVC_ID_SMFD = 30,
    		NCSMDS_SVC_ID_SMFND = 31,
    		NCSMDS_SVC_ID_SMFA = 32,
    		NCSMDS_SVC_ID_RDE = 33,
 		NCSMDS_SVC_ID_CLMS = 34,
                NCSMDS_SVC_ID_CLMA = 35,
                NCSMDS_SVC_ID_CLMNA = 36,
		NCSMDS_SVC_ID_PLMS = 37,
		NCSMDS_SVC_ID_PLMS_HRB = 38,
		NCSMDS_SVC_ID_PLMA = 39,
		NCSMDS_SVC_ID_NCSMAX,	/* This mnemonic always last */

		/* The range below is for OpenSAF internal use */
		NCSMDS_SVC_ID_INTERNAL_MIN = 256,

		/* The range below is for OpenSAF external use */
		NCSMDS_SVC_ID_EXTERNAL_MIN = 512,
	} NCSMDS_SVC_ID;

/*
 * NCSMDS_MAX_PWES
 *
 * This constant defines the highest Private World Environment
 * identifier that will be defined on any VDEST. Private World
 * Environment 'names' or IDs assigned by the customer must be
 * some number LESS THAN this value. Zero (0) is an illegal PWE name.
 *
 */

#define NCSMDS_MAX_PWES     63
#define NCSMDS_MAX_VDEST    32767
#define NCSMDS_MAX_SVCS     1023

#define MDS_DIRECT_BUF_MAXSIZE 8000
	typedef uns8 *MDS_DIRECT_BUFF;
#define m_MDS_ALLOC_DIRECT_BUFF(size) mds_alloc_direct_buff(size)
#define m_MDS_FREE_DIRECT_BUFF(x) mds_free_direct_buff(x)
	MDS_DIRECT_BUFF mds_alloc_direct_buff(uns16 size);
	void mds_free_direct_buff(MDS_DIRECT_BUFF buff);

/************************************************************************
 * SCOPES for install, broadcast and subscription.
 * (See also
 ************************************************************************/
	typedef enum ncsmds_scopes {
		NCSMDS_SCOPE_INTRAPCON = 1,	/* For intra-process-constellation events. */
		NCSMDS_SCOPE_INTRANODE = 2,	/* For intra-node events. */
		/* INTRACHASSIS Scope Not supported as of now */
		NCSMDS_SCOPE_INTRACHASSIS = 3,	/* For intra-chassis scoping. */
		NCSMDS_SCOPE_NONE = 4,
	} NCSMDS_SCOPE_TYPE;

/*************************************************************************
  Structures for different forms of MDS Send
*************************************************************************/
	typedef struct mds_sync_snd_ctxt {
#define MDS_SYNC_SND_CTXT_LEN_MAX 12
		uns8 length;	/* length of the stored data */
		uns8 data[MDS_SYNC_SND_CTXT_LEN_MAX];	/* the actual data           */
	} MDS_SYNC_SND_CTXT;

	typedef struct mds_sendtype_snd_info {
		MDS_DEST i_to_dest;	/* A virtual or absolute destination */
	} MDS_SENDTYPE_SND_INFO;

	typedef struct mds_sendtype_sndrsp_info {
		MDS_DEST i_to_dest;	/* A virtual or absolute destination */
		uns32 i_time_to_wait;	/* Timeout duration in 10ms units */
		NCSCONTEXT o_rsp;	/* Place for the response for normal message   */
		MDS_DIRECT_BUFF buff;	/* For response of type direct send */
		uns16 len;
		MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver;	/* For the message format ver */
	} MDS_SENDTYPE_SNDRSP_INFO;

	typedef struct mds_sendtype_sndrspack_info {	/* Wait for an ack to the response */
		MDS_DEST i_sender_dest;	/* Sender's the one who gets this response */
		uns32 i_time_to_wait;	/* Timeout duration in 10ms units */
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* MDS supplied data to identify request */
	} MDS_SENDTYPE_SNDRSPACK_INFO;

	typedef struct mds_sendtype_sndack_info {
		MDS_DEST i_to_dest;	/* A virtual or absolute destination */
		uns32 i_time_to_wait;	/* Timeout duration in 10ms units */
	} MDS_SENDTYPE_SNDACK_INFO;

	typedef struct mds_sendtype_rsp_info {
		MDS_DEST i_sender_dest;	/* Sender's the one who gets this response */
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* MDS supplied data to identify request */
	} MDS_SENDTYPE_RSP_INFO;

	typedef struct mds_sendtype_red_info {
		MDS_DEST i_to_vdest;	/* A virtual destination             */
		V_DEST_QA i_to_anc;	/* Anchor within virtual destination */
	} MDS_SENDTYPE_RED_INFO;

	typedef struct mds_sendtype_redrsp_info {
		MDS_DEST i_to_vdest;	/* A virtual destination             */
		V_DEST_QA i_to_anc;	/* Anchor within virtual destination */
		uns32 i_time_to_wait;	/* in 10ms units                  */
		NCSCONTEXT o_rsp;	/* Place for the message context  */
		MDS_DIRECT_BUFF buff;	/* For response of type direct send */
		uns16 len;
		MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver;	/* For the message format ver */
	} MDS_SENDTYPE_REDRSP_INFO;

	typedef struct mds_sendtype_redrspack_info {	/* Wait for an ack to the response */
		MDS_DEST i_to_vdest;	/* A virtual destination             */
		V_DEST_QA i_to_anc;	/* Anchor within virtual destination */
		uns32 i_time_to_wait;	/* in 10ms units                  */
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* MDS supplied data to identify request */
	} MDS_SENDTYPE_REDRSPACK_INFO;

	typedef struct mds_sendtype_redack_info {
		MDS_DEST i_to_vdest;	/* A virtual destination             */
		V_DEST_QA i_to_anc;	/* Anchor within virtual destination */
		uns32 i_time_to_wait;	/* in 10ms units                */
	} MDS_SENDTYPE_REDACK_INFO;

	typedef struct mds_sendtype_rrsp_info {
		MDS_DEST i_to_dest;	/* Sender's the one who gets this response */
		V_DEST_QA i_to_anc;
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* MDS supplied data to identify request */
	} MDS_SENDTYPE_RRSP_INFO;

	typedef struct mds_sendtype_bcast_info {
		NCSMDS_SCOPE_TYPE i_bcast_scope;	/* Scope of broadcast (viz. node, etc.) */
	} MDS_SENDTYPE_BCAST_INFO;

	typedef struct mds_sendtype_rbcast_info {
		NCSMDS_SCOPE_TYPE i_bcast_scope;	/* Scope of broadcast (viz. node, etc.) */
	} MDS_SENDTYPE_RBCAST_INFO;

	typedef enum {
		MDS_SENDTYPE_SND = 0,	/* Simply send a message and forget about it           */
		MDS_SENDTYPE_SNDRSP = 1,	/* Send a message and wait for the response or timeout */
		MDS_SENDTYPE_SNDRACK = 2,	/* Send a response and wait for the ack or timeout     */
		MDS_SENDTYPE_SNDACK = 3,	/* Send a message and wait for the ack or timeout      */
		MDS_SENDTYPE_RSP = 4,	/* Send a response to the message received thro SNDRSP */
		MDS_SENDTYPE_RED = 5,	/* Simply send a message and forget about it           */
		MDS_SENDTYPE_REDRSP = 6,	/* Send a message and wait for the response or timeout */
		MDS_SENDTYPE_REDRACK = 7,	/* Send a response and wait for the ack or timeout     */
		MDS_SENDTYPE_REDACK = 8,	/* Send a message and wait for the ack or timeout      */
		MDS_SENDTYPE_RRSP = 9,	/* Send a response to the message received thro REDRSP */
		MDS_SENDTYPE_BCAST = 10,	/* Send a broadcast message to all primaries           */
		MDS_SENDTYPE_RBCAST = 11,	/* Send a broadcast message to both anchors            */
		MDS_SENDTYPE_EMPTY1 = 12,
		MDS_SENDTYPE_EMPTY2 = 13,
		MDS_SENDTYPE_ACK = 14,
		MDS_SENDTYPE_RACK = 15,
	} MDS_SENDTYPES;

	typedef enum {
		MDS_SEND_PRIORITY_LOW = NCS_IPC_PRIORITY_LOW,
		MDS_SEND_PRIORITY_MEDIUM = NCS_IPC_PRIORITY_NORMAL,
		MDS_SEND_PRIORITY_HIGH = NCS_IPC_PRIORITY_HIGH,
		MDS_SEND_PRIORITY_VERY_HIGH = NCS_IPC_PRIORITY_VERY_HIGH
	} MDS_SEND_PRIORITY_TYPE;

/* Prototype for callback registed by an MDS client. It is a single-entry
   API for all MDS callbacks including, encode/decode/receive, etc. */
	struct ncsmds_callback_info;
	typedef uns32 (*NCSMDS_CALLBACK_API) (struct ncsmds_callback_info * info);

/************************************************************************
    2) SVC APIs into MDS   (requests into MDS by Services(clients) )
*************************************************************************/
	typedef enum {
		MDS_INSTALL = 0,
		MDS_UNINSTALL = 1,
		MDS_SUBSCRIBE = 2,	/* For subscribing under a simple-view    */
		MDS_RED_SUBSCRIBE = 3,	/* For subscribing under a redundant-view */
		MDS_CANCEL = 4,
		MDS_SYS_SUBSCRIBE = 5,	/* subscribe to system events: node up/down */
		MDS_SEND = 6,
		MDS_DIRECT_SEND = 7,
		MDS_RETRIEVE = 8,
		MDS_CHG_ROLE = 9,	/* Changes role service's underlying VDEST  */
		MDS_QUERY_DEST = 10,
		MDS_QUERY_PWE = 11,
		MDS_NODE_SUBSCRIBE = 12,
		MDS_NODE_UNSUBSCRIBE = 13,
	} NCSMDS_TYPE;

	typedef struct mds_install_info {
		MDS_CLIENT_HDL i_yr_svc_hdl;	/* Your svc hdl                          */
		NCSMDS_SCOPE_TYPE i_install_scope;	/* Advertisement scope for svc presence */
		NCSMDS_CALLBACK_API i_svc_cb;
		MDS_DEST o_dest;	/* Virtual or an absolute destination */
		V_DEST_QA o_anc;	/* Non-zero only for virtual destination */

   /*---------------------------------------------------------------------*\
   With respect to an "incoming queue" ownership, MDS supports two models

        1)  MDS maintains a message queue:- In this model, MDS takes up the
            responsibility of maintaining a message queue.  To retrieve
            messages from this queue an MDS-client invokes the "MDS_RETRIEVE"
            API. To know of message-arrival, an MDS-client will need to use
            the NCS selection-object primitives on the "i_sel_obj" return
            during MDS_INSTALL. To opt for this model, an MDS-client will have to set the
            "i_mds_q_ownership" flag to TRUE.

            (NOTE: In this model, MDS will never invoke the
            MDS_CALLBACK_RECEIVE.)

        2)  MDS does NOT maintain a message queue:- In this model, MDS simply
            invokes the MDS_CALLBACK_RECEIVE function to deliver messages to
            an MDS-client. It is upto the callback implementation to
            appropriately store or process the message.

            To opt for this model, an MDS-client will have to set the
            "i_mds_q_ownership" flag to FALSE.
   \*---------------------------------------------------------------------*/

		/* An MDS client has to choose an MDS model using "i_mds_q_ownership".
		   Please see the m_NCS_SEL_OBJ_* macros to understand the "o_sel_obj" */
		NCS_BOOL i_mds_q_ownership;
		NCS_SEL_OBJ o_sel_obj;	/* Valid only if "i_mds_q_ownership=TRUE" */
		MDS_SVC_PVT_SUB_PART_VER i_mds_svc_pvt_ver;
		NCS_BOOL i_fail_no_active_sends;	/* Default messages will be buufered in MDS when destination is
							   in No-Active state, else dropped.
							   A return value of MDS_RC_MSG_NO_BUFFERING 
							   will be returned when this member is set to 1 */
	} MDS_INSTALL_INFO;

	typedef void (*MDS_Q_MSG_FREE_CB) (NCSCONTEXT msg_to_be_freed);

	typedef struct mds_uninstall_info {
		/* i_msg_free_cb: This is a callback that needs to be provided
		   in the MDS-q-ownership model case. This callback
		   is invoked once for free every message in MDS
		   queue that has not been processed.

		   This prevents memory-leaks at the time that a service
		   uninstalls with MDS by freeing up all unprocessed
		   messages.

		   N O T E : This callback pointer can safely be NULL
		   in the case when MDS-q-ownership model is not being
		   used.
		 */
		MDS_Q_MSG_FREE_CB i_msg_free_cb;
	} MDS_UNINSTALL_INFO;

/* A service can subscribe to multiple service-ids in a single call
   by passing any array through the "i_svc_id" pointer. It is also
   allowed to make multiple calls to subscribe to multiple service-ids
   and/or multiple scopes.
*/
	typedef struct mds_subscribe_info {
		NCSMDS_SCOPE_TYPE i_scope;	/* Only events within the scope to be reported */
		uns8 i_num_svcs;	/* Number of elements in the following array */
		MDS_SVC_ID *i_svc_ids;	/* The service-ids that are being subscribed
					   to. MDS will copy this array, not free */
	} MDS_SUBSCRIBE_INFO;

	typedef MDS_SUBSCRIBE_INFO MDS_RED_SUBSCRIBE_INFO;

/* A service cancels all of its subscriptions by the following call.*/
	typedef struct mds_cancel_info {
		uns8 i_num_svcs;	/* Number of elements in the following array */
		MDS_SVC_ID *i_svc_ids;	/* The service-ids that are being cancelled
					   MDS will copy this array, not free */
	} MDS_CANCEL_INFO;

	typedef struct mds_sys_subscribe_info {
		EVT_FLTR i_evt_map;	/* Bit mask of events to be subscribed to        */
		/* Set to 0 to cancel all previous subscriptions */
	} MDS_SYS_SUBSCRIBE_INFO;

	typedef struct mds_send_info {
		NCSCONTEXT i_msg;	/* Pointer to the message */
		MDS_SVC_ID i_to_svc;	/* The service at the destination  */
		MDS_SEND_PRIORITY_TYPE i_priority;
		MDS_SENDTYPES i_sendtype;

		union {
			MDS_SENDTYPE_SND_INFO snd;
			MDS_SENDTYPE_SNDRSP_INFO sndrsp;
			MDS_SENDTYPE_SNDRSPACK_INFO sndrack;
			MDS_SENDTYPE_SNDACK_INFO sndack;
			MDS_SENDTYPE_RSP_INFO rsp;
			MDS_SENDTYPE_RED_INFO red;
			MDS_SENDTYPE_REDRSP_INFO redrsp;
			MDS_SENDTYPE_REDRSPACK_INFO redrack;
			MDS_SENDTYPE_REDACK_INFO redack;
			MDS_SENDTYPE_RRSP_INFO rrsp;
			MDS_SENDTYPE_BCAST_INFO bcast;
			MDS_SENDTYPE_RBCAST_INFO rbcast;
		} info;
	} MDS_SEND_INFO;

	typedef struct mds_direct_send_info {
		MDS_DIRECT_BUFF i_direct_buff;	/* Pointer to the message */
		uns16 i_direct_buff_len;
		MDS_SVC_ID i_to_svc;	/* The service at the destination  */
		MDS_SEND_PRIORITY_TYPE i_priority;
		MDS_SENDTYPES i_sendtype;
		MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver;

		union {
			MDS_SENDTYPE_SND_INFO snd;
			MDS_SENDTYPE_SNDRSP_INFO sndrsp;
			MDS_SENDTYPE_SNDRSPACK_INFO sndrack;
			MDS_SENDTYPE_SNDACK_INFO sndack;
			MDS_SENDTYPE_RSP_INFO rsp;
			MDS_SENDTYPE_RED_INFO red;
			MDS_SENDTYPE_REDRSP_INFO redrsp;
			MDS_SENDTYPE_REDRSPACK_INFO redrack;
			MDS_SENDTYPE_REDACK_INFO redack;
			MDS_SENDTYPE_RRSP_INFO rrsp;
			MDS_SENDTYPE_BCAST_INFO bcast;
			MDS_SENDTYPE_RBCAST_INFO rbcast;
		} info;
	} MDS_DIRECT_SEND_INFO;

	struct ncsmds_callback_info;
	typedef struct mds_retrieve_info {
		/* Whether callback retrieval should block if there are no callbacks pending */
		SaDispatchFlagsT i_dispatchFlags;

	} MDS_RETRIEVE_INFO;

	typedef struct mds_chg_role_info {
		V_DEST_RL new_role;
	} MDS_CHG_ROLE_INFO;

/* MDS_QUERY_VDEST_INFO: Can be used to query information on a 
   service instance. Note the request can fail if the user has not
   subscribed for that service.
*/
	typedef struct mds_query_dest_info {
		/* i_dest: Virtual Destination */
		MDS_DEST i_dest;

		/* i_svc_id: The service-id of the service that is being queried about */
		MDS_SVC_ID i_svc_id;

		/* i_query_for_role: FALSE ==> Return "anchor" of active VDEST-instance 
		   and                TRUE ==> Return role of VDEST-instance specified. */
		NCS_BOOL i_query_for_role;
		union {
			struct {
				V_DEST_RL i_vdest_rl;
				V_DEST_QA o_anc;	/* Anchor for (any) active service-instance */
			} query_for_anc;

			struct {
				V_DEST_QA i_anc;
				V_DEST_RL o_vdest_rl;	/* Role of service-instance specified */
			} query_for_role;

		} info;

		NCS_BOOL o_local;	/* TRUE => local, FALSE => REMOTE   */
		NODE_ID o_node_id;	/* Node on which <VDEST,anc> is situated */
		MDS_DEST o_adest;	/* ADEST of MDS-core of <VDEST, anc>     */

	} MDS_QUERY_DEST_INFO;

/* MDS_QUERY_PWE_INFO: Can be used by a client to query information on itself.
   This information is provided based on the client's "pwe=handle"
*/
	typedef struct mds_query_pwe_info {
		/* This API supplies information using the MDS-handle supplied in
		   the request. */
		PW_ENV_ID o_pwe_id;	/* 0, if global hdl; else PWE ID       */
		NCS_BOOL o_absolute;	/* True if this PWE is on an absolute addr */
		union {
			struct {
				MDS_DEST o_adest;
			} abs_info;	/* Info valid if "o_absolute=TRUE" */

			struct {
				MDS_DEST o_vdest;
				V_DEST_QA o_anc;
				V_DEST_RL o_role;
			} virt_info;	/* Info valid if "o_absolute=FALSE" */
		} info;

	} MDS_QUERY_PWE_INFO;

/* MDS_SUBSCRIBE_NODE_INFO: Can be used by a client to get the NODE_UP and NODE_DOWN .
*/
	typedef struct mds_subscribe_node_info {
		uns32 i_dummy;	/* Unused */

	} MDS_SUBSCRIBE_NODE_INFO;

/* MDS_UNSUBSCRIBE_NODE_INFO: Can be used by a client to get the NODE_UP and NODE_DOWN .
*/
	typedef struct mds_unsubscribe_node_info {
		uns32 i_dummy;	/* Unused */

	} MDS_UNSUBSCRIBE_NODE_INFO;

	typedef struct ncsmds_info {
		/* i_mds_hdl:  This can be one of
		   1) mds_adest_hdl: Given by "global-MDS-services" on absolute dest.
		   2) mds_vdest_hdl: Given "global-MDS-services" on virtual dest.
		   3) mds_pwe_hdl: Given by "normal-MDS-services" living in a PWE.
		 */
		MDS_HDL i_mds_hdl;

		/* The service that is making this request */
		MDS_SVC_ID i_svc_id;

		/* The MDS operation that is to be performed.  */
		NCSMDS_TYPE i_op;
		union {
			MDS_INSTALL_INFO svc_install;
			MDS_UNINSTALL_INFO svc_uninstall;
			MDS_SUBSCRIBE_INFO svc_subscribe;
			MDS_RED_SUBSCRIBE_INFO red_subscribe;
			MDS_CANCEL_INFO svc_cancel;
			MDS_SYS_SUBSCRIBE_INFO svc_sys_subscribe;
			MDS_SEND_INFO svc_send;
			MDS_DIRECT_SEND_INFO svc_direct_send;
			MDS_RETRIEVE_INFO retrieve_msg;
			MDS_CHG_ROLE_INFO chg_role;
			MDS_QUERY_DEST_INFO query_dest;
			MDS_QUERY_PWE_INFO query_pwe;
			MDS_SUBSCRIBE_NODE_INFO subscribe_node;  
			MDS_UNSUBSCRIBE_NODE_INFO unsubscribe_node;  
		} info;
	} NCSMDS_INFO;

/* Return codes of "ncsmds_api"

   NCSCC_RC_SUCCESS     : Things went fine.
   NCSCC_RC_FAILURE     : The API call failed.
   NCSCC_RC_REQ_TIMEOUT : The API call timed out
*/
	typedef uns32 (*NCSMDS_API) (NCSMDS_INFO *svc_to_mds_info);
	EXTERN_C uns32 ncsmds_api(NCSMDS_INFO *svc_to_mds_info);

/************************************************************************
    3) SVC APIs out of MDS (indications out of MDS to Services(clients) )
*************************************************************************/

	typedef enum {
		MDS_CALLBACK_COPY = 0,
		MDS_CALLBACK_ENC = 1,
		MDS_CALLBACK_DEC = 2,
		MDS_CALLBACK_ENC_FLAT = 3,
		MDS_CALLBACK_DEC_FLAT = 4,
		MDS_CALLBACK_RECEIVE = 5,
		MDS_CALLBACK_SVC_EVENT = 6,
		MDS_CALLBACK_SYS_EVENT = 7,
		MDS_CALLBACK_QUIESCED_ACK = 8,	/* Acknowledgement of quiesced action */
		MDS_CALLBACK_DIRECT_RECEIVE = 9,
		MDS_CALLBACK_NODE_EVENT = 10,

		MDS_CALLBACK_SVC_MAX
	} NCSMDS_CALLBACK_TYPE;

	typedef struct {
		/* MDS Client is supposed to make a copy of "i_msg" and return a pointer
		   to this copy through the "o_cpy" member. The destination of the message is
		   "i_to_svc_id".

		   "i_last" is set to TRUE if "i_msg" will not be needed after this instance
		   of the COPY call. Thus it will FALSE for all but the last "MDS-sends" that
		   comprise an MDS-broadcast-send. It will also be TRUE for COPY resulting
		   from non-broadcast MDS send.  A client is expected to use "i_last", to
		   know when to clean up its "i_msg" structure - "i_msg" could be safely
		   freed when "i_last" is set to TRUE, because MDS will not use "i_msg"
		   after that call.
		 */

		NCSCONTEXT i_msg;	/* Message to be copied */
		NCS_BOOL i_last;	/* TRUE if last COPY for this "i_msg" */
		MDS_SVC_ID i_to_svc_id;	/* Destined service-id */
		NCSCONTEXT o_cpy;	/* A copy of "i_msg" */
		MDS_SVC_PVT_SUB_PART_VER i_rem_svc_pvt_ver;	/* Remote service id subpart version */
		MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver;

	} MDS_CALLBACK_COPY_INFO;

	typedef struct mds_callback_enc_info {
		/* The MDS-client should encode the msg into the USRBUF in "io_uba". The
		   message will go to a different processor. Since that processor could have
		   a different "endianness" and "byte-packing",  all data bytes in the message
		   should be packed in a "network order" format in the USRBUF */

		NCSCONTEXT i_msg;	/* Message to be encoded */
		MDS_SVC_ID i_to_svc_id;	/* Destined service-id */
		struct ncs_ubaid *io_uba;	/* MDS supplied USRBUF to encode into */
		MDS_SVC_PVT_SUB_PART_VER i_rem_svc_pvt_ver;	/* Remote service id subpart version */
		MDS_CLIENT_MSG_FORMAT_VER o_msg_fmt_ver;

	} MDS_CALLBACK_ENC_INFO;

	typedef struct mds_callback_dec_info {
		/* The MDS-client should decode the msg from the USRBUF in "io_uba". The
		   message is coming from a different processor. Since that processor could
		   have a different "endianness" and "byte-packing",  all data in the message
		   should be unpacked from a "network order" format in the USRBUF to a
		   "host-order" format */

		struct ncs_ubaid *io_uba;	/* MDS supplied USRBUF to decode from */
		MDS_SVC_ID i_fr_svc_id;	/* Source service-id                  */
		NCS_BOOL i_is_resp;	/* For internal use only              */
		NCSCONTEXT o_msg;	/* New message resulting from decode  */
		NODE_ID i_node_id;	/* Node id of the sender              */
		MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver;

	} MDS_CALLBACK_DEC_INFO;

/* The MDS-client should flatten the msg into the USRBUF in "io_uba". The
message will goto another process within the same processor. Since
"endianness" or "byte packing" is going to be the same, only linked-list
structures need to be flattened into a USRBUF. There is no need to
pack individual structure members. */
	typedef MDS_CALLBACK_ENC_INFO MDS_CALLBACK_ENC_FLAT_INFO;

/* The MDS-client should de-flatten the msg from the USRBUF in "io_uba". The
message is coming from another process within the same processor. Since
"endianness" or "byte packing" is going to be the same, only linked-list
structures need to be reconstructed from the USRBUF. There is no need to
unpack individual structure members. */
	typedef MDS_CALLBACK_DEC_INFO MDS_CALLBACK_DEC_FLAT_INFO;

	typedef struct {
		/* If an MDS-Client has NOT opted for MDS-incoming-queue-ownership
		   model, then messages will be delivered to it using this callback.
		   (See also: MDS_INSTALL structure)
		 */
		NCSCONTEXT i_msg;
		NCS_BOOL i_rsp_reqd;	/* TRUE if send is awaiting a response */
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* Valid only if "i_rsp_expected == TRUE" */
		MDS_DEST i_fr_dest;
		MDS_SVC_ID i_fr_svc_id;
		V_DEST_QA i_fr_anc;	/* Can be used for redundant messages */
		MDS_DEST i_to_dest;
		MDS_SVC_ID i_to_svc_id;
		MDS_SEND_PRIORITY_TYPE i_priority;	/* Priority with which this message was sent */
		NODE_ID i_node_id;	/* Node id of the sender              */

		/* Local handle to the PWE of "svc_id". It is meaningful only to "Global
		   MDS services". Global MDS services are global to a "MDS_DEST" scope.
		   They get to receive messages from services living on any PWE that
		   is _also_ present on that "MDS_DEST". (Note: This implies that a Global
		   MDS service, living on MDS_DEST = D, cannot receive a message from a
		   service living somewhere on PWE-ID, say 2, unless PWE-ID 2 has also been
		   created on D)
		 */
		MDS_HDL sender_pwe_hdl;
		MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver;

	} MDS_CALLBACK_RECEIVE_INFO;

	typedef struct {
		/* If an MDS-Client has NOT opted for MDS-incoming-queue-ownership
		   model, then messages will be delivered to it using this callback.
		   (See also: MDS_INSTALL structure)
		 */
		MDS_DIRECT_BUFF i_direct_buff;	/* Pointer to the message */
		uns16 i_direct_buff_len;
		NCS_BOOL i_rsp_reqd;	/* TRUE if send is awaiting a response */
		MDS_SYNC_SND_CTXT i_msg_ctxt;	/* Valid only if "i_rsp_expected == TRUE" */
		MDS_DEST i_fr_dest;
		MDS_SVC_ID i_fr_svc_id;
		V_DEST_QA i_fr_anc;	/* Can be used for redundant messages */
		MDS_DEST i_to_dest;
		MDS_SVC_ID i_to_svc_id;
		MDS_SEND_PRIORITY_TYPE i_priority;	/* Priority with which this message was sent */
		NODE_ID i_node_id;	/* Node id of the sender              */

		/* Local handle to the PWE of "svc_id". It is meaningful only to "Global
		   MDS services". Global MDS services are global to a "MDS_DEST" scope.
		   They get to receive messages from services living on any PWE that
		   is _also_ present on that "MDS_DEST". (Note: This implies that a Global
		   MDS service, living on MDS_DEST = D, cannot receive a message from a
		   service living somewhere on PWE-ID, say 2, unless PWE-ID 2 has also been
		   created on D)
		 */
		MDS_HDL sender_pwe_hdl;
		MDS_CLIENT_MSG_FORMAT_VER i_msg_fmt_ver;

	} MDS_CALLBACK_DIRECT_RECEIVE_INFO;

	typedef struct {
		/*
		   MDS uses the following callback to inform an MDS-clients about
		   services coming up or going down, along with their MDS address. An
		   MDS-client is expected to explicitly subscribe for all services that
		   it is interested in.
		 */
		NCSMDS_CHG i_change;
		MDS_DEST i_dest;
		V_DEST_QA i_anc;	/* Non-zero only if "i_dest" is virtual    */
		V_DEST_RL i_role;	/* Non-zero only if "i_dest" is virtual    */
		NODE_ID i_node_id;	/* Node-id on which "i_dest" lives         */
		PW_ENV_ID i_pwe_id;
		MDS_SVC_ID i_svc_id;	/* The Service Id this event applies to    */
		MDS_SVC_ID i_your_id;	/* svc id of subscriber (YOU)              */

		/* Local handle to the PWE of "svc_id". It is meaningful only to "Global
		   MDS services". Global MDS services are global to a "MDS_DEST" scope.
		   They get to receive up/down events on services living on any PWE that
		   is _also_ present on that "MDS_DEST". (Note: This implies that a Global
		   MDS service, living on MDS_DEST = D, cannot receive an event about a
		   service living somewhere on PWE-ID, say 2, unless PWE-ID 2 has also been
		   created on D)
		 */
		MDS_HDL svc_pwe_hdl;
		MDS_SVC_PVT_SUB_PART_VER i_rem_svc_pvt_ver;	/* Remote service id subpart version */

	} MDS_CALLBACK_SVC_EVENT_INFO;

	typedef struct {
		/* MDS uses the following callback to inform an MDS-clients about
		   system events to which the clients have subscribed.
		   Currently, only node up/down is supported. More events can be
		   added later on.
		 */
		NCSMDS_CHG i_change;	/* GONE, UP, DOWN                    */
		NODE_ID i_node_id;	/* Node-id which is affected         */
		EVT_FLTR i_evt_mask;	/* Bit map of events                 */
	} MDS_CALLBACK_SYS_EVENT_INFO;

	typedef struct mds_callback_quiesced_ack_info {
		/* MDS uses the following callback to inform an MDS-client that it
		   has done informed the remain system that this MDS-client has
		   quiesced. Beyond this point MDS-Client will cease to receive
		   messages sent addressed to active VDESTs
		 */
		uns32 i_dummy;	/* Unused */
	} MDS_CALLBACK_QUIESCED_ACK_INFO;

	typedef struct mds_callback_node_event_info {
		/* MDS uses the following callback to inform an MDS-client that 
		   a node is UP or down 
		 */
		NCSMDS_NODE_CHG  node_chg;
		NODE_ID node_id;
	} MDS_CALLBACK_NODE_EVENT_INFO;

	typedef struct ncsmds_callback_info {
		MDS_CLIENT_HDL i_yr_svc_hdl;	/* Handle to MDS Client's context */
		MDS_SVC_ID i_yr_svc_id;	/* Service ID of yourself         */
		NCSMDS_CALLBACK_TYPE i_op;
		union {
			MDS_CALLBACK_COPY_INFO cpy;
			MDS_CALLBACK_ENC_INFO enc;
			MDS_CALLBACK_DEC_INFO dec;
			MDS_CALLBACK_ENC_FLAT_INFO enc_flat;
			MDS_CALLBACK_DEC_FLAT_INFO dec_flat;
			MDS_CALLBACK_RECEIVE_INFO receive;
			MDS_CALLBACK_DIRECT_RECEIVE_INFO direct_receive;
			MDS_CALLBACK_SVC_EVENT_INFO svc_evt;
			MDS_CALLBACK_SYS_EVENT_INFO sys_evt;
			MDS_CALLBACK_QUIESCED_ACK_INFO quiesced_ack;
			MDS_CALLBACK_NODE_EVENT_INFO node_evt;
		} info;
	} NCSMDS_CALLBACK_INFO;

/************************************************************************
    4) Definitions to talk to MDS as a service provider over the SPRR
       interface.
*************************************************************************/

/* Service provider abstract name */
#define m_MDS_SP_ABST_NAME  "NCS_MDS"
#define m_SPRR_VDEST_POLICY_ATTR_NAME "NCS_MDS_VDEST_POLICY"

/* Reserved instance names for SPIR interface */
	EXTERN_C const SaNameT glmds_adest_inst_name;
#define m_MDS_SPIR_ADEST_NAME  glmds_adest_inst_name

#define m_MDS_FIXED_VDEST_TO_INST_NAME(i_vdest_id, o_name)   \
        mds_fixed_vdest_to_inst_name(i_vdest_id, o_name)

#define m_MDS_INST_NAME_TO_FIXED_VDEST(i_name, o_vdest_id)   \
        mds_inst_name_to_fixed_vdest(i_name, o_vdest_id)

	EXTERN_C void mds_fixed_vdest_to_inst_name(uns32 vdest_id, SaNameT *o_name);
	EXTERN_C uns32 mds_inst_name_to_fixed_vdest(SaNameT *i_name, uns32 *o_vdest_id);

/* Single-entry API  to be used by service-users of MDS is  :"ncsmds_api()" */

/************************************************************************
    5) MDS inventory dump functions (meant for debugging/diagnostics)
*************************************************************************/
#ifdef NCS_DEBUG

	EXTERN_C void ncsmds_pp(void);
#endif

/***********************************************************************\
    6) Utilities to manipulate MDS_DEST
       (See also ncsgl_defs.h for defintions and utilities)
\***********************************************************************/
#define m_NCS_MDS_DEST_EQUAL(x,y) (memcmp((x), (y), sizeof(MDS_DEST)) == 0)

/* MDS_DEST Utilities */
/* m_NCS_GET_VDEST_ID_FROM_MDS_DEST:Returns vdest-id if the MDS_DEST provided
                                is a virtual destination. Returns 0
                                if the MDS_DEST provided is an absolute
                                destination.
*/
	MDS_VDEST_ID ncs_get_vdest_id_from_mds_dest(MDS_DEST mdsdest);
#define m_MDS_GET_VDEST_ID_FROM_MDS_DEST(mdsdest) (ncs_get_vdest_id_from_mds_dest(mdsdest))

/* m_NCS_SET_VDEST_ID_IN_MDS_DEST:
*/

#define m_NCS_SET_VDEST_ID_IN_MDS_DEST(mdsdest, vdest_id) (mdsdest = vdest_id)

/* m_NCS_IS_AN_ADEST: A macro to check if an MDS-DEST is an absolute
                      destination.
                      - TRUE if MDS-DEST supplied is an absolute destination.
                      - FALSE otherwise
*/
#define m_MDS_DEST_IS_AN_ADEST(d) (m_NCS_NODE_ID_FROM_MDS_DEST(d)!=0)

#include "ncs_mda_papi.h"

#ifdef  __cplusplus
}
#endif

#endif
