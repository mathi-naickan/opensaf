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

  DESCRIPTION: ASAPi messages and the handler routines are listed in this file 
 
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#ifndef MSG_MQSV_ASAPI_H_
#define MSG_MQSV_ASAPI_H_

/* ASAPi Headers */
#include "mqsv_asapi_mem.h"
#include "base/ncsencdec_pub.h"

/*****************************************************************************\
                                 C O N S T A N T S
\*****************************************************************************/

/********************************** ASAPi TRACK *****************************/
#define ASAPi_TRACK_ENABLE       0x01
#define ASAPi_TRACK_DISABLE      0x02

/*****************************************************************************
                     Macros for checking the track state
*****************************************************************************/
#define m_ASAPi_TRACK_IS_ENABLE(flag)    (flag & ASAPi_TRACK_ENABLE)
#define m_ASAPi_TRACK_IS_DISABLE(flag)   (flag & ASAPi_TRACK_DISABLE)

/*****************************************************************************
                     Macros for setting the track state
*****************************************************************************/
#define m_ASAPi_TRACK_ENABLE_SET(flag)    (flag |= ASAPi_TRACK_ENABLE)
#define m_ASAPi_TRACK_DISABLE_SET(flag)   (flag |= ASAPi_TRACK_DISABLE)

/*****************************************************************************
                     Macros for resetting the track state
*****************************************************************************/
#define m_ASAPi_TRACK_ENABLE_RESET(flag)    (flag &= ~ASAPi_TRACK_ENABLE)
#define m_ASAPi_TRACK_DISABLE_RESET(flag)    (flag &= ~ASAPi_TRACK_DISABLE)

/****************************** ENUM ****************************************/
typedef enum {
	ASAPi_OBJ_QUEUE = 1,	/* Object type QUEUE */
	ASAPi_OBJ_GROUP,	/* Object type QUEUE GROUPS */
	ASAPi_OBJ_BOTH,		/* Object type BOTH (QUEUE & GROUPS) */
} ASAPi_OBJ_TYPE;

typedef enum {
	ASAPi_MSG_SEND = 1,	/* Message Send */
	ASAPI_MSG_RECEIVE,	/* Message Receive */
} ASAPi_MSG_OPR;

typedef enum {
	ASAPi_QUEUE_ADD = 1,	/* Queue Added */
	ASAPi_QUEUE_DEL,	/* Queue Deleted */
	ASAPi_QUEUE_UPD,	/* Queue Updated */
	ASAPi_GROUP_ADD,	/* Group Added */
	ASAPi_GROUP_DEL,	/* Group Deleted */
	ASAPi_QUEUE_MQND_UP,	/* MQND is UP */
	ASAPi_QUEUE_MQND_DOWN	/*Destination MQND is Down */
} ASAPi_OBJECT_OPR;

typedef enum {
	ASAPi_MSG_REG = 0x01,	/* ASAPi Registration Message */
	ASAPi_MSG_DEREG,	/* ASAPi De-registration Message */
	ASAPi_MSG_REG_RESP,	/* ASAPi Registration Response Message */
	ASAPi_MSG_DEREG_RESP,	/* ASAPi De-registration Response Message */
	ASAPi_MSG_NRESOLVE,	/* ASAPi Name Resolution Message */
	ASAPi_MSG_NRESOLVE_RESP,	/* ASAPi Name Resolution Response Message */
	ASAPi_MSG_GETQUEUE = 0x10,	/* ASAPi Getqueue Message */
	ASAPi_MSG_GETQUEUE_RESP,	/* ASAPi Getqueue Response Message */
	ASAPi_MSG_TRACK,	/* ASAPi Track Message */
	ASAPi_MSG_TRACK_RESP,	/* ASAPi Track Response Message */
	ASAPi_MSG_TRACK_NTFY,	/* ASAPi Track Notification Message */
} ASAPi_MSG_TYPE;

typedef enum {
	ASAPi_OPR_BIND = 1,	/* Binds with ASAPi Layer */
	ASAPi_OPR_UNBIND,	/* Unbinds from the ASAPi Layer */
	ASAPi_OPR_GET_DEST,	/* Retrives the destination address */
	ASAPi_OPR_GET_QUEUE,	/* Retrives the Queue Information */
	ASAPi_OPR_MSG,		/* ASAPi Message handler */
	ASAPi_OPR_TRACK		/* Enable/Disable Tracking */
} ASAPi_OPR_TYPE;

/*****************************************************************************\
                  A S A Pi - Q U E U E    E L E M E N T
\*****************************************************************************/
typedef struct asapi_queue_param {
	MDS_DEST addr;		/* Queue Location */
	SaTimeT retentionTime;	/* Lifetime of the Queue */
	SaNameT name;		/* Queue Name */
	SaMsgQueueSendingStateT status;	/* Sending status of the Queue */
	uint32_t hdl;		/* Queue handle */
	MQSV_QUEUE_OWN_STATE owner;	/* Queue is owned or Orphan */
	uint8_t is_mqnd_down;	/* true if mqnd is down else false */
	SaMsgQueueCreationFlagsT creationFlags;	/* Queue creation flags */
	SaSizeT size[SA_MSG_MESSAGE_LOWEST_PRIORITY + 1];	/* Priority queue sizes */
} ASAPi_QUEUE_PARAM;

/*****************************************************************************\
                  A S A Pi - Q U E U E   I N F O 
\*****************************************************************************/
typedef struct asapi_queue_info {
	NCS_QELEM qelm;		/* Must be first in struct, Queue element */
	ASAPi_QUEUE_PARAM param;	/* Queue parameters */
} ASAPi_QUEUE_INFO;

/*****************************************************************************\
                  A S A Pi - G R O U P   I N F O 
\*****************************************************************************/
typedef struct asapi_group_info {
	NCS_QUEUE qlist;	/* List of Queues */
	ASAPi_QUEUE_INFO *pQueue;	/* Current selected Queue */
	ASAPi_QUEUE_INFO *plaQueue;	/* last selected Queue */
	SaNameT group;		/* Group Name */
	uint32_t lcl_qcnt;		/* Number of local queues in queue qroup */
	SaMsgQueueGroupPolicyT policy;	/* Member selction policy */
} ASAPi_GROUP_INFO;

/*****************************************************************************\
                  A S A Pi - C A C H E   I N F O 
\*****************************************************************************/
typedef struct asapi_cache_info {
	NCS_QELEM qelm;		/* Must be first in struct, Queue element */
	ASAPi_OBJ_TYPE objtype;	/* Which object to look for */
	NCS_LOCK clock;		/* Cache Node lock */
	union {
		ASAPi_QUEUE_INFO qinfo;	/* Queue & its information */
		ASAPi_GROUP_INFO ginfo;	/* Queue Group & its information */
	} info;
} ASAPi_CACHE_INFO;

/*****************************************************************************\
                  A S A Pi - E R R O R   I N F O 
\*****************************************************************************/
typedef struct asapi_err_info {
	SaAisErrorT errcode;	/* Error code */
	uint8_t flag;		/* Error marker */
} ASAPi_ERR_INFO;

/*###########################################################################*\
                         
                              ASAPi Messages 

\*###########################################################################*/

/*****************************************************************************\
                  A S A Pi - R E G E S T R A T I O N    I N F O
\*****************************************************************************/
typedef struct asapi_reg_info {
	ASAPi_OBJ_TYPE objtype;	/* Which object to register */
	ASAPi_QUEUE_PARAM queue;	/* Queue parameters */
	SaNameT group;		/* Queue Group */
	SaMsgQueueGroupPolicyT policy;	/* Member selction policy */
} ASAPi_REG_INFO;

/*****************************************************************************\
         A S A Pi - R E G E S T R A T I O N   R E S P O N S E   I N F O 
\*****************************************************************************/
typedef struct asapi_reg_resp_info {
	ASAPi_OBJ_TYPE objtype;	/* Which object got registered */
	ASAPi_ERR_INFO err;	/* Error Information */
	SaNameT group;		/* Queue Group Name */
	SaNameT queue;		/* Queue Name */
} ASAPi_REG_RESP_INFO;

/*****************************************************************************\
                  A S A Pi - D E R E G E S T R A T I O N    I N F O 
\*****************************************************************************/
typedef struct asapi_dereg_info {
	ASAPi_OBJ_TYPE objtype;	/* Which object to register */
	SaNameT group;		/* Queue Group Name */
	SaNameT queue;		/* Queue Name */
} ASAPi_DEREG_INFO;

/*****************************************************************************\
         A S A Pi - D E R E G E S T R A T I O N   R E S P O N S E   I N F O 
\*****************************************************************************/
typedef struct asapi_dereg_resp_info {
	ASAPi_OBJ_TYPE objtype;	/* Which object got deregistered */
	ASAPi_ERR_INFO err;	/* Error Information */
	SaNameT group;		/* Queue Group Name */
	SaNameT queue;		/* Queue Name */
} ASAPi_DEREG_RESP_INFO;

/*****************************************************************************\
               A S A Pi - N A M E  R E S O L U T I O N   I N F O
\*****************************************************************************/
typedef struct asapi_nresolve_info {
	SaNameT object;		/* Queue/Group Name */
	uint8_t track;		/* Track Flag */
} ASAPi_NRESOLVE_INFO;

/*****************************************************************************\
          A S A Pi - O B J E C T   P A R A M E T E R   I N F O
\*****************************************************************************/
typedef struct asapi_object_info {
	SaNameT group;		/* Queue Group Name */
	SaMsgQueueGroupPolicyT policy;	/* Member Selection Policy */
	ASAPi_QUEUE_PARAM *qparam;	/* Queue Parameters */
	uint16_t qcnt;		/* Number of Queues */
} ASAPi_OBJECT_INFO;

/*****************************************************************************\
       A S A Pi - N A M E  R E S O L U T I O N  R E S P O N S E  I N F O
\*****************************************************************************/
typedef struct asapi_nresolve_resp_info {
	ASAPi_OBJECT_INFO oinfo;	/* Object Information */
	ASAPi_ERR_INFO err;	/* Error Information */
} ASAPi_NRESOLVE_RESP_INFO;

/*****************************************************************************\
                  A S A Pi - V A L I D A T E  I N F O 
\*****************************************************************************/
typedef struct asapi_getqueue_info {
	SaNameT queue;		/* Queue Name */
} ASAPi_GETQUEUE_INFO;

/*****************************************************************************\
            A S A Pi - V A L I D A T E   R E S P O N S E   I N F O
\*****************************************************************************/
typedef struct asapi_getqueue_resp_info {
	ASAPi_QUEUE_PARAM queue;	/* Queue Information */
	ASAPi_ERR_INFO err;	/* Error Information */
} ASAPi_GETQUEUE_RESP_INFO;

/*****************************************************************************\
                  A S A Pi - T R A C K  I N F O
\*****************************************************************************/
typedef struct asapi_track_info {
	SaNameT object;		/* Object Name */
	uint8_t val;		/* Enable/Disable Tracking */
} ASAPi_TRACK_INFO;

/*****************************************************************************\
                  A S A Pi - T R A C K  R E S P  I N F O
\*****************************************************************************/
typedef struct asapi_track_resp_info {
	ASAPi_ERR_INFO err;	/* Error Information */
	ASAPi_OBJECT_INFO oinfo;	/* Object Information */
} ASAPi_TRACK_RESP_INFO;

/*****************************************************************************\
          A S A Pi - T R A C  N O T I F I C A T I O N   I N F O
\*****************************************************************************/
typedef struct asapi_track_ntfy_info {
	ASAPi_OBJECT_INFO oinfo;	/* Object Information */
	ASAPi_OBJECT_OPR opr;	/* Object Operation */
} ASAPi_TRACK_NTFY_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
               A S A Pi - M E S S A G E  E N V E L O P 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_msg_info {
	ASAPi_MSG_TYPE msgtype;	/* ASAPi Message Type */
	union {
		ASAPi_REG_INFO reg;	/* Registration Request Message */
		ASAPi_DEREG_INFO dereg;	/* Deregistration Request Message */
		ASAPi_NRESOLVE_INFO nresolve;	/* Name Resolution Request Message */
		ASAPi_GETQUEUE_INFO getqueue;	/* Getqueue Queue Request Message */
		ASAPi_TRACK_INFO track;	/* Track Request Message */

		ASAPi_REG_RESP_INFO rresp;	/* Registration Response Message */
		ASAPi_DEREG_RESP_INFO dresp;	/* Deregistration Response Message */
		ASAPi_NRESOLVE_RESP_INFO nresp;	/* Name Resolution Response Message */
		ASAPi_GETQUEUE_RESP_INFO vresp;	/* Getqueue Queue Response Message */
		ASAPi_TRACK_RESP_INFO tresp;	/* Track Response Message */
		ASAPi_TRACK_NTFY_INFO tntfy;	/* Track Notification Message */
	} info;
	uint16_t usg_cnt;		/* Usage count */
} ASAPi_MSG_INFO;

/***************************************************************************
                     The Service User's Callback Prototype
****************************************************************************/
typedef uint32_t (*ASAPi_CALLBACK_HDLR) (struct asapi_msg_info *);

/*###########################################################################*\
                         
                       ASAPi_CB (Global Control Block)

\*###########################################################################*/
typedef struct asapi_cb {
	NCS_SERVICE_ID my_svc_id;	/* Service ID of the User */
	NCS_QUEUE cache;	/* Cache Information (Optional) */
	ASAPi_CALLBACK_HDLR usrhdlr;	/* User Notification Handler */
	MDS_HDL mds_hdl;	/* User's MDS Handle */
	MDS_SVC_ID mds_svc_id;	/* MDS service id of the User */
	MDS_DEST my_dest;	/* MDS Dest of the  User */
} ASAPi_CB;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                        A S A Pi - B I N D  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_bind_info {
	NCS_SERVICE_ID i_my_id;	/* User's service ID */
	ASAPi_CALLBACK_HDLR i_indhdlr;	/* Service User Callback handler */
	MDS_HDL i_mds_hdl;	/* User's MDS Handle */
	MDS_SVC_ID i_mds_id;	/* User'd MDS service id */
	MDS_DEST i_mydest;	/* User'd MDS Destination */
} ASAPi_BIND_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                  A S A Pi - U N B I N D  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_unbind_info {
	uint32_t dummy;
} ASAPi_UNBIND_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                  A S A Pi - D E S T I N A T I O N  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_dest_info {
	SaNameT i_object;	/* Queue Group Name */
	MQSV_SEND_INFO i_sinfo;	/* Destination information, Only required
				   while sending message */
	uint8_t i_track;
	ASAPi_CACHE_INFO *o_cache;	/* Cache Entry */
} ASAPi_DEST_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                  A S A Pi - C O N F G  Q U E U E  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_q_info {
	SaNameT i_name;		/* Queue Name */
	MQSV_SEND_INFO i_sinfo;	/* Destination information, Only required
				   while sending message */
	ASAPi_QUEUE_PARAM o_parm;	/* Queue Information */
} ASAPi_Q_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                  A S A Pi - G R O U P  T R A C K  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

typedef struct asapi_group_track_info {
	SaMsgQueueGroupNotificationBufferT notification_buffer;
	SaMsgQueueGroupPolicyT policy;
	SaUint32T qcnt;
} ASAPi_GROUP_TRACK_INFO;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                  A S A Pi - D E S T I N A T I O N  I N F O 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_grp_track_info {
	SaNameT i_group;	/* Queue Group Name */
	SaUint8T i_flags;	/* Track Flag */
	ASAPi_GROUP_TRACK_INFO o_ginfo;	/* Group Information Node */
	uint8_t i_option;		/* Track Option (0x01 - ENABLE, 0x02 - DISABLE) */
	MQSV_SEND_INFO i_sinfo;	/* Destination information, Only required
				   while sending message */
} ASAPi_GRP_TRACK_INFO;

typedef struct asapi_msg_arg {
	ASAPi_MSG_OPR opr;	/* ASAPi Operation to be performed vis-a-vis MSG */
	MQSV_SEND_INFO sinfo;	/* Destination information, Only required while 
				   sending message */
	ASAPi_MSG_INFO req;	/* ASAPi Request message */
	ASAPi_MSG_INFO *resp;	/* ASAPi Response message */
} ASAPi_MSG_ARG;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*\
                     A S A Pi - R E Q U E S T   E N V E L O P 
\*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
typedef struct asapi_opr_info {
	ASAPi_OPR_TYPE type;	/* what operation to perform */
	union {
		ASAPi_BIND_INFO bind;	/* Bind Information */
		ASAPi_UNBIND_INFO unbind;	/* Unbind Information */
		ASAPi_DEST_INFO dest;	/* Destination Information */
		ASAPi_GRP_TRACK_INFO track;	/* Group Track Information */
		ASAPi_Q_INFO queue;	/* Queue Information */
		ASAPi_MSG_ARG msg;	/* ASAPi Message Information */
	} info;
} ASAPi_OPR_INFO;

extern  ASAPi_CB asapi;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*\
                                 A S A Pi - A P Is 

 This API is used by the USER of the ASAPi layer, who want's to use the ASAPi
 functionality. This is a SE API with diffrent request options. 
\*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
uint32_t asapi_opr_hdlr(struct asapi_opr_info *);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*\
                      MDS ENCODE/DECODE/COPY ROUTINES

 These routines are to be only used by MDS for Encoding / Decoding / Copying 
 ASAPi messages.
\*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void asapi_msg_enc(ASAPi_MSG_INFO *, NCS_UBAID *);
uint32_t asapi_msg_dec(NCS_UBAID *, ASAPi_MSG_INFO **);
uint32_t asapi_msg_cpy(ASAPi_MSG_INFO *, ASAPi_MSG_INFO **);

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*\
                           ASAPi ROUTINES
        These routines are to be only used by ASAPi & MQSv internally
\*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/
void asapi_msg_free(ASAPi_MSG_INFO **);
uint32_t asapi_queue_select(ASAPi_GROUP_INFO *);

/*
 * m_ASAPi_DBG_SINK
 *
 * If ASAPi fails an if-conditional or other test that we would not expect
 * under normal conditions, it will call this macro. 
 * 
 * If ASAPi_DEBUG == 1, fall into the sink function. A customer can put
 * a breakpoint there or leave the CONSOLE print statement, etc.
 *
 * If ASAPi_DEBUG == 0, just echo the value passed. It is typically a 
 * return code or a NULL.
 *
*/
#if (ASAPi_DEBUG == 1)

uint32_t asapi_dbg_sink(uint32_t, char *, uint32_t);

/* m_ASAPi_DBG_VOID() used to keep compiler happy @ void return functions */

#define m_ASAPi_DBG_SINK(r)  asapi_dbg_sink(__LINE__,__FILE__,(uint32_t)r)
#define m_ASAPi_DBG_VOID     asapi_dbg_sink(__LINE__,__FILE__,1)
#else

#define m_ASAPi_DBG_SINK(r)  r
#define m_ASAPi_DBG_VOID
#endif

#endif  // MSG_MQSV_ASAPI_H_
