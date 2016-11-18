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
*/
#ifndef MBC_MBCSV_PAPI_H_
#define MBC_MBCSV_PAPI_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "base/ncsgl_defs.h"
#include "base/ncs_ubaid.h"
#include "mds/mds_papi.h"
#include "base/ncs_svd.h"

#include "osaf/saf/saAis.h"
#include "base/ncs_saf.h"

typedef uint64_t NCS_MBCSV_CLIENT_HDL;
typedef uint32_t NCS_MBCSV_HDL;
typedef uint32_t NCS_MBCSV_CKPT_HDL;
typedef SS_SVC_ID NCS_MBCSV_CLIENT_SVCID;
typedef uint64_t MBCSV_REO_HDL;

/***************************************************************************

    M e s s a g e   B a s e d   C h e c k p o i n t i n g   S e r v i c e (MBCSv)

    This header file is broken down into these major subsections:

     1) Client registered callback function prototype specification
     2) MBCSv Service Life cycle functions
     3) Remote Peer Client relationship functions
     4) Client Messaging services functions
     5) Subscription services for watching messages of other local clients
     6) MBCSv management and managed object manipulation

***************************************************************************/

/****************************************************************************

     Client registered callback function prototype specification
     ===========================================================

     MBCSv drives HA state machines, and so knows when certain HA activities
     are to be performed. The types and argument set that follow
     characterize the callback operations and arguments that a client is
     responsible for.

***************************************************************************/

typedef enum ncs_mbcsv_msg_type {       /* encode/decode message types */
  /* Messages carrying checkpoint data */
  NCS_MBCSV_MSG_ASYNC_UPDATE = 1,
  NCS_MBCSV_MSG_COLD_SYNC_REQ,
  NCS_MBCSV_MSG_COLD_SYNC_RESP,
  NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE,
  NCS_MBCSV_MSG_WARM_SYNC_REQ,
  NCS_MBCSV_MSG_WARM_SYNC_RESP,
  NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE,
  NCS_MBCSV_MSG_DATA_REQ,
  NCS_MBCSV_MSG_DATA_RESP,
  NCS_MBCSV_MSG_DATA_RESP_COMPLETE,

} NCS_MBCSV_MSG_TYPE;

typedef enum ncs_mbcsv_act_type {       /* basic checkpoint operations        */
  NCS_MBCSV_ACT_DONT_CARE,    /* not interesting in this context    */
  NCS_MBCSV_ACT_ADD,      /* checkpoint data is new             */
  NCS_MBCSV_ACT_RMV,      /* checkpoint data to remove          */
  NCS_MBCSV_ACT_UPDATE,   /* update existing checkpoint         */
  NCS_MBCSV_ACT_NO_PEER,  /* update if STDBY peer doesn't exist */
  NCS_MBCSV_ACT_MAX
} NCS_MBCSV_ACT_TYPE;

/***************************************************************************
 * ENCODE : in general, MBCSv tells the ACTIVE Component to encode a message
 ***************************************************************************/

typedef struct ncs_mbcsv_cb_enc {
  NCS_MBCSV_MSG_TYPE io_msg_type; /* reinvoke during sync. till xxx_COMPLETE */
  NCS_MBCSV_ACT_TYPE io_action;   /* action: ADD,RMV,UPDATE              */
  uint32_t io_reo_type;   /* client's (private) object type      */
  MBCSV_REO_HDL io_reo_hdl;       /* client's (private) object hdl       */
  NCS_UBAID io_uba;       /* Encoded data added to this          */
  uint64_t io_req_context;        /* decoded request info */
  uint16_t i_peer_version;        /* version info of peer as per SAF */

} NCS_MBCSV_CB_ENC;

/***************************************************************************
 * DECODE : in general, MBCSv tells the STANDBY Component to decode a message
 ***************************************************************************/

typedef struct ncs_mbcsv_cb_dec {
  NCS_MBCSV_MSG_TYPE i_msg_type;  /* type of message to decode           */
  NCS_MBCSV_ACT_TYPE i_action;    /* checkpoint action: ADD,RMV,UPDATE   */
  uint32_t i_reo_type;    /* Value passed during encode operation */
  NCS_UBAID i_uba;        /* data to decode                      */
  uint64_t o_req_context; /* Keep decoded request information. */
  uint16_t i_peer_version;        /* version info of peer as per SAF */

} NCS_MBCSV_CB_DEC;

/***************************************************************************
 * PEER : MBCSv tells a client about its peer's versioning information.
          An MBCSv client sees this as soon as a new peer is discovered.
***************************************************************************/

typedef struct ncs_mbcsv_peer { /* ncsre_enc::op == NCSRE_PEER_INFO  */
  NCS_MBCSV_CLIENT_SVCID i_service;       /* The clients NCS_SERVICE_ID */
  uint16_t i_peer_version;        /* version info as per SAF */

} NCS_MBCSV_CB_PEER;

/***************************************************************************
 * NOTIFY : my (protection group) peer invoked NCS_MBCSV_OP_SEND_NOTIFY and
 * the message is now delivered to me.
 ***************************************************************************/
typedef struct ncs_mbcsv_cb_notify {
  NCS_UBAID i_uba;        /* encoded message from my peer */
  uint16_t i_peer_version;        /* version info as per SAF */
  NCSCONTEXT i_msg;       /* Notif context to for notif message */

} NCS_MBCSV_CB_NOTIFY;

/***************************************************************************
 * MBCSv tells a application when it sees problems.
 ***************************************************************************/
typedef enum ncs_mbcsv_err_codes {
  /* Different error codes to be send with the error indication should be added here */
  NCS_MBCSV_COLD_SYNC_TMR_EXP,    /* Cold Sync timer expired without getting response */
  NCS_MBCSV_WARM_SYNC_TMR_EXP,    /* Warm Sync timer expired without getting response */
  NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP,       /* Data Response complete timer expired */
  NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP,       /* Cold Sync complete timer expired */
  NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP,       /* Warm Sync complete timer expired */
  NCS_MBCSV_DATA_RESP_TERMINATED, /* Data responce is terminated. Release context */
  NCS_MBCSV_COLD_SYNC_RESP_TERMINATED,    /* Cold Sync terminated. Release context */
  NCS_MBCSV_WARM_SYNC_RESP_TERMINATED     /* Warm Sync terminated. Release context */
} NCS_MBCSV_ERR_CODES;

typedef struct ncs_mbcsv_cb_err_ind {
  NCS_MBCSV_ERR_CODES i_code;     /* notification code ID              */
  bool i_err;     /* if true then ERROR else its INFO  */
  NCSCONTEXT i_arg;       /* value type mapped to code id      */
  uint16_t i_peer_version;        /* version info as per SAF        */

} NCS_MBCSV_CB_ERR_IND;

/***************************************************************************
 *
 * The Callback abstractions shape a single function callback paradigm
 *
 *   1) NCS_MBCSV_CBOP     - enum explains i_op callback operations
 *   2) NCS_MBCSV_CB_ARG   - the arguments that arrive in callback
 *   3) (*NCS_MBCSV_CB)()  - function prototype for client registered callback
 *
 ***************************************************************************/

/* 1) single entry callback operation types */

typedef enum ncs_mbcsv_cbop {

  NCS_MBCSV_CBOP_ENC,     /* ACTIVE sends/encodes state data     */
  NCS_MBCSV_CBOP_DEC,     /* STANDBY rcvs/decodes state data     */
  NCS_MBCSV_CBOP_PEER,    /* PEER versioning info arrives        */
  NCS_MBCSV_CBOP_NOTIFY,  /* NOTIFY msg arrives from peer        */
  NCS_MBCSV_CBOP_ERR_IND, /* Error indication to applicatio      */
  NCS_MBCSV_CBOP_ENC_NOTIFY,      /* NOTIFY msg arrives from peer        */

} NCS_MBCSV_CBOP;

/* 2) the arg-set that is delivered at the single entry callback           */

typedef struct ncs_mbcsv_cb_arg {
  NCS_MBCSV_CBOP i_op;    /* ENC, DEC, LAYERM or PEER           */
  NCS_MBCSV_CLIENT_HDL i_client_hdl;      /* Client handle supplied with Open */
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* Checkpoint handle to identify checkpoint */
  union {
    NCS_MBCSV_CB_ENC encode;        /* Callback requests encode           */
    NCS_MBCSV_CB_DEC decode;        /* Callback requests decode           */
    NCS_MBCSV_CB_PEER peer; /* callback explains peer's version   */
    NCS_MBCSV_CB_NOTIFY notify;     /* callback msg has arrived from peer */
    NCS_MBCSV_CB_ERR_IND error;     /* callback shows MBCSv internal errors */
  } info;

} NCS_MBCSV_CB_ARG;

/* 3) the func prototype of callback; client provides & of a function      */

typedef uint32_t (*NCS_MBCSV_CB) (NCS_MBCSV_CB_ARG *arg);

/***************************************************************************

     MBCSv Service Life cycle functions
     ================================

      These functions match the basic service life cycle functions called
      out by any of the 6 SAF AIS services. We replicate this paradigm here.

***************************************************************************/

/***************************************************************************
 * INITIALIZE creates an instance of MBCSv and gets it to steady state.
 ***************************************************************************/

typedef struct ncs_mbcsv_initialize {
  NCS_MBCSV_CB i_mbcsv_cb;        /* client gives callback funcs        */
  uint16_t i_version;     /* client version info as per SAF     */
  NCS_MBCSV_CLIENT_SVCID i_service;       /* service id of client               */
  NCS_MBCSV_HDL o_mbcsv_hdl;      /* MBCSv returns handle for calls       */

} NCS_MBCSV_INITIALIZE;

/***************************************************************************
 * SEL_OBJ_GET fetch an operation system handle for Dispatch operations
 ***************************************************************************/

typedef struct ncs_mbcsv_sel_obj_get {
  SaSelectionObjectT o_select_obj;        /* OS Handle                        */

} NCS_MBCSV_SEL_OBJ_GET;

/***************************************************************************
 * DISPATCH is invoked by client to service pending MBCSv callbacks
 ***************************************************************************/

typedef struct ncs_mbcsv_dispatch {
  SaDispatchFlagsT i_disp_flags;  /* one of ONE, ALL or BLOCKING       */

} NCS_MBCSV_DISPATCH;

/***************************************************************************
 * FINALIZE destroys ans instance of MBCSv; associated i_mbcsv_hld no good now
 ***************************************************************************/

typedef struct ncs_mbcsv_finalize {
  uint16_t i_dummy;       /* noop; dummy keeps struct viable    */

} NCS_MBCSV_FINALIZE;

/***************************************************************************

     Remote Peer Client relationship functions
     =========================================

     These functions (are intended to) establish a relationship between two
     peer entities, who are, in general, expecting to relate to each other as
     ACTIVE and STANDBY HA roles in a paired Component Service Instance (CSI)
     relationship.

***************************************************************************/

/***************************************************************************
 * OPEN establishes contact/addressing info of peer entity at first CSI
 * assignment time (seeSaAmfCSISetCallbackT). This will be invoked once per
 * CSI assignment that this component can support.
 ***************************************************************************/

typedef struct ncs_mbcsv_open {
  uint32_t i_pwe_hdl;     /* MBCSv assumes MDS; PWE/Global/etc    */
  NCS_MBCSV_CLIENT_HDL i_client_hdl;      /* MBCSv passes back in callbacks       */
  NCS_MBCSV_CKPT_HDL o_ckpt_hdl;  /* MBCSv binding value for this open    */

} NCS_MBCSV_OPEN;

/***************************************************************************
 * CLOSE disengages peer contact & resources recovered; invoked when CSI is
 * removed from this Component (see SaAmfCSIRevmoveCallbackT)
 ***************************************************************************/

typedef struct ncs_mbcsv_close {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* MBCSv hdl established at OPEN time   */

} NCS_MBCSV_CLOSE;
/***************************************************************************
 * CHG_ROLE sets this end of the peer pair to an HA role of ACTIVE, STANDBY,
 * QUIESCED or STOPPING.
 ***************************************************************************/

typedef struct ncs_mbcsv_chg_role {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;
  SaAmfHAStateT i_ha_state;       /* Active/Standby/Quisced/Stopping    */

} NCS_MBCSV_CHG_ROLE;

/****************************************************************************

     Client Messaging service functions
     ==================================

     There are two types of messages a client can send to its peer:

     1) An ACTIVE client can send Checkpoint data to its peer. There are three
        options of send that trade off performance vs lock-step state tracking
        sync      - call thread held till STANDBY acknowledges reciept
        usr_async - call thread held till checkpoint message built and
                     passed off to local transport
        mbc_async - call thread places SEND request on MBCSv work queue and
                     returns. Process SEND when MBCSv gets to run.

      2) Any client in any HA state can pass a NOTIFY message to its peer
         entity that it is in contact with.

***************************************************************************/

typedef enum ncs_mbcsv_snd_type {
  NCS_MBCSV_SND_SYNC,     /* hold invoker thread till ACK back from STANDBY */
  NCS_MBCSV_SND_USR_ASYNC,        /* hold invoker thread till MSG goes to Transport */
  NCS_MBCSV_SND_MBC_ASYNC /* MBCSv work queue gets send REQ; process later   */
} NCS_MBCSV_SND_TYPE;

/***************************************************************************
 * CHECKPOINT : MBCSv will only allow this message to continue processing if
 * this client is in the ACTIVE state.
 ***************************************************************************/

typedef struct ncs_mbcsv_send_ckpt {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* Send message to my peer            */
  NCS_MBCSV_SND_TYPE i_send_type; /* SYNC or flavor of ASYNC            */
  uint32_t i_reo_type;    /* pvt REO type; pass to ENCODE       */
  MBCSV_REO_HDL i_reo_hdl;        /* pvt REO ptr/hdl; pass to ENCODE    */
  NCS_MBCSV_ACT_TYPE i_action;    /* ckpt action: ADD,RMV,UPDATE        */
  bool io_no_peer;        /* Appl wants to know about peer existance */

} NCS_MBCSV_SEND_CKPT;

typedef enum ncs_mbcsv_ntfy_msg_dest {
  NCS_MBCSV_ACTIVE,       /* Send message to ACTIVE peer */
  NCS_MBCSV_STANDBY,      /* Send message to all STANDBY peers */
  NCS_MBCSV_ALL_PEERS     /* Send this message to all peers */
} NCS_MBCSV_NTFY_MSG_DEST;

/***************************************************************************
 * NOTIFY is a free-form, stateless message that this side can send to its
 * peer at any time after OPEN time, assuming the other side is 'attached'.
 ***************************************************************************/

typedef struct ncs_mbcsv_send_notify {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* Send message to my peer */
  NCS_MBCSV_NTFY_MSG_DEST i_msg_dest;
  NCSCONTEXT i_msg;       /* Notif context to get in notif enc cbk */

} NCS_MBCSV_SEND_NOTIFY;

/***************************************************************************
 * Data Request is a free-form, stateless message that standby can send to its
 * Active peer at any time after OPEN time, assuming the Active peer is 'attached'.
 ***************************************************************************/

typedef struct ncs_mbcsv_send_data_req {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* Send message to my peer            */
  NCS_UBAID i_uba;        /* encoded message for my peer        */

} NCS_MBCSV_SEND_DATA_REQ;

/***************************************************************************

    Subscription services for watching messages of other local clients
    ==================================================================

    Subscription services allows interested clients to be informed
    via a callback when a particular message was sent or received by some
    other client within this process that is scoped to the same i_mbcsv_hdl.

***************************************************************************/

/***************************************************************************
 * Callback function prototype provided by usr at subscribe time
 ***************************************************************************/

struct ncs_mbcsv_fevt;  /* NCS_MBCSV_FEVT def later in file:compiler is happy     */

typedef void (*NCS_MBCSV_SUB_CB) (struct ncs_mbcsv_fevt * fevt_data);

/***************************************************************************
 *  NCS_MBCSV_DIR specifies if we are interested in a message sent or rvcd
 ***************************************************************************/

typedef enum ncs_mbcsv_dir {
  NCS_MBCSV_DIR_SENT,     /* msg sent by watched (the 'other' client)    */
  NCS_MBCSV_DIR_RCVD      /* msg rcvd by watched (the 'other' client)    */
} NCS_MBCSV_DIR;

/***************************************************************************
 * NCS_MBCSV_FLTR is an arg to subscription APIs to qualify event to watch for
 ***************************************************************************/

typedef struct ncs_mbcsv_fltr { /* Subscription Service Filter Values */
  NCS_MBCSV_SUB_CB i_cb_func;     /* callback func when event occurs    */
  NCSCONTEXT i_my_ctxt;   /* optional callback assist value     */
  NCS_MBCSV_CKPT_HDL i_its_ckpt_hdl;      /* ckpt_hdl of watched msg stream  */
  NCS_MBCSV_DIR i_dir;    /* watch for msg going this direction */
  NCS_MBCSV_MSG_TYPE i_msg_id;    /* watch for msg of this type         */

} NCS_MBCSV_FLTR;

/***************************************************************************
 * NCS_MBCSV_FEVT is passed back to the usr at callback time to explain whats up
 ***************************************************************************/
typedef enum ncs_mbcsv_evt_status {
  NCS_MBCSV_STATUS_SUCCESS,
  NCS_MBCSV_STATUS_TMR_EXP,
  NCS_MBCSV_STATUS_CKPT_CLOSED
} NCS_MBCSV_EVT_STATUS;

typedef struct ncs_mbcsv_fevt { /* Subscription Service Filter Event Values     */
  NCS_MBCSV_EVT_STATUS i_status;  /* SUCCESS:msg seen, FAILURE: tmr exp */
  NCSCONTEXT i_my_ctxt;   /* optional cb assist value arrives   */
  NCS_MBCSV_CKPT_HDL i_its_ckpt_hdl;      /* ckpt_hdl of watched stream     */
  NCS_MBCSV_DIR i_dir;    /* direction of 'found' message       */
  NCS_MBCSV_MSG_TYPE i_msg_id;    /* watch for msg of this type         */

} NCS_MBCSV_FEVT;

/***************************************************************************

       MBCSv management and managed object manipulation
       ==============================================

       MBCSv management allows some client to fetch or set the values of a set
       of managed objects as follows:
       follows:

       SET Set an MBCSv OBJ value for one client or a Collection of Clients.
       GET Get the value associated with one client

***************************************************************************/

/***************************************************************************/
/* MBCSv Configuration Object Identifiers whose values to get or set..       */
/* ======================================================================= */
/* RW : Read or Write; can do get or set operations on this object         */
/* R- : Read only; can do get operations on this object                    */
/* -W : Write only; can do set operations on this object                   */

typedef enum {          /*     V a l u e   E x p r e s s i o n       */
  /*-------------------------------------------*/
  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF, /* RW ENABLE|DISABLE warm sync msg SMM review */
  NCS_MBCSV_OBJ_TMR_WSYNC,        /* RW 10msec Send warm sync @ expiry         */

} NCS_MBCSV_OBJ;

/***************************************************************************
 * OBJ_SET : Set an object of one client or a collection of clients
 ***************************************************************************/

typedef struct ncs_mbcsv_obj_set {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* the client instance changing      */
  NCS_MBCSV_OBJ i_obj;    /* the OBJ ID whose value is to SET  */
  uint32_t i_val; /* the value the OBJ is to be SET to */

} NCS_MBCSV_OBJ_SET;

/***************************************************************************
 * OBJ_GET : Get an object value scoped to a particular client
 ***************************************************************************/

typedef struct ncs_mbcsv_obj_get {
  NCS_MBCSV_CKPT_HDL i_ckpt_hdl;  /* Fetched value is from this client */
  NCS_MBCSV_OBJ i_obj;    /* the OBJID of value to fetch       */
  uint32_t o_val; /* the value fetched goes here       */

} NCS_MBCSV_OBJ_GET;

/***************************************************************************
 *
 * =========================================================================
 *
 * E N D   All Contributing MBCSv Abstractions; We now put it all together
 *
 * =========================================================================
 *
 ***************************************************************************/

/***************************************************************************
 *
 * The MBCSv abstractions shape a single function entry point paradigm
 *
 *   1) NCS_MBCSV_OP       - enum explains i_op MBCSv operations
 *   2) NCS_MBCSV_ARG      - the arguments filled in by invoking client
 *   3) ncs_mbcsv_svc      - func entry point (and prototype) for MBCSv service
 *
 ***************************************************************************/

/* 1) single entry MBCSv operation types                                   */

typedef enum ncs_mbcsv_op {

  /* Message based checkpoint library life cycle OP codes                  */

  NCS_MBCSV_OP_INITIALIZE,        /* put an MBCSv instance in start state   */
  NCS_MBCSV_OP_SEL_OBJ_GET,       /* Get SELECT obj to coordinate Dispatch */
  NCS_MBCSV_OP_DISPATCH,  /* Service any pending Callbacks        */
  NCS_MBCSV_OP_FINALIZE,  /* Recover MBCSv instance resources       */

  /* engage and/or disengage checkpointing, change HA role OP codes        */

  NCS_MBCSV_OP_OPEN,      /* initiate contact with peer entity    */
  NCS_MBCSV_OP_CLOSE,     /* dis-engage from peer entity relation */
  NCS_MBCSV_OP_CHG_ROLE,  /* change client's HA state-machine     */

  /* Checkpoint and stateless messages to peer OP codes                    */

  NCS_MBCSV_OP_SEND_CKPT, /* Send a Checkpoint to my peer entity  */
  NCS_MBCSV_OP_SEND_NOTIFY,       /* Send a freeform msg to my peer entity */
  NCS_MBCSV_OP_SEND_DATA_REQ,     /* Send a Data request Message to Active peer */

  /* configuration SET and GET operations only after init OP codes         */

  NCS_MBCSV_OP_OBJ_GET,   /* GET val scoped to client            */
  NCS_MBCSV_OP_OBJ_SET    /* SET val scoped to client|collection */
} NCS_MBCSV_OP;

/* 2) The MBCSv service arguments filled in by invoking client               */

typedef struct ncs_mbcsv_arg {
  NCS_MBCSV_OP i_op;      /* which sub-structure is populated?   */
  NCS_MBCSV_HDL i_mbcsv_hdl;      /* operate on this MBCSv instance        */

  union {

    /* Message based checkpoint library life cycle functions               */

    NCS_MBCSV_INITIALIZE initialize;
    NCS_MBCSV_SEL_OBJ_GET sel_obj_get;
    NCS_MBCSV_DISPATCH dispatch;
    NCS_MBCSV_FINALIZE finalize;

    /* engage and/or disengage checkpointing, change HA role               */

    NCS_MBCSV_OPEN open;
    NCS_MBCSV_CLOSE close;
    NCS_MBCSV_CHG_ROLE chg_role;

    /* Checkpoint message types, and stateless notify message to peer      */

    NCS_MBCSV_SEND_CKPT send_ckpt;
    NCS_MBCSV_SEND_NOTIFY send_notify;
    NCS_MBCSV_SEND_DATA_REQ send_data_req;

    /* configuration SET and GET operations only after init                */

    NCS_MBCSV_OBJ_SET obj_set;
    NCS_MBCSV_OBJ_GET obj_get;

  } info;

} NCS_MBCSV_ARG;

/* 3) function entry point and prototype for MBCSv service                   */

/*typedef uint32_t (*NCS_MBCSV) (NCS_MBCSV_ARG* arg);  function prototype           */
uint32_t ncs_mbcsv_svc(NCS_MBCSV_ARG *arg);     /* MBCSv function instance        */

uint32_t mbcsv_prt_inv(void);

#ifdef  __cplusplus
}
#endif

#endif  // MBC_MBCSV_PAPI_H_
