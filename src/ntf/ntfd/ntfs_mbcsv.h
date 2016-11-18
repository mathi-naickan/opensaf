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
 * Author(s): Ericsson AB
 *
 */

#ifndef NTF_NTFD_NTFS_MBCSV_H_
#define NTF_NTFD_NTFS_MBCSV_H_

#include "amf/saf/saAmf.h"

#define NTFS_MBCSV_VERSION 2
#define NTFS_MBCSV_VERSION_1 1
#define NTFS_MBCSV_VERSION_MIN 1

/* Checkpoint message types(Used as 'reotype' w.r.t mbcsv)  */

/* Checkpoint update messages are processed similair to ntfsv internal
 * messages handling,  so the types are defined such that the values
 * can be mapped straight with ntfsv message types
 */

typedef enum ntfsv_ckpt_rec_type {
  NTFS_CKPT_INITIALIZE_REC = 1,
  NTFS_CKPT_FINALIZE_REC = 2,
  NTFS_CKPT_AGENT_DOWN = 3,
  NTFS_CKPT_NOTIFICATION = 4,
  NTFS_CKPT_SUBSCRIBE = 5,
  NTFS_CKPT_UNSUBSCRIBE = 6,
  NTFS_CKPT_NOT_LOG_CONFIRM = 7,
  NTFS_CKPT_NOT_SEND_CONFIRM = 8,
  NTFS_CKPT_MSG_MAX
} ntfsv_ckpt_msg_type_t;

typedef enum ntfsv_discarded_rec_type {
  NTFS_NOTIFICATION_OK = 0,
  NTFS_NOTIFICATION_DISCARDED = 1,
  NTFS_NOTIFICATION_DISCARDED_LIST_SENT = 2,
  NTFS_CKPT_DISCARDED_MSG_MAX
} ntfsv_discarded_type_t;

/* Structures for Checkpoint data(to be replicated at the standby) */

/* Registrationlist checkpoint record, used in cold/async checkpoint updates */
typedef struct {
  uint32_t client_id;     /* Registration Id at Active */
  MDS_DEST mds_dest;      /* Handy when an NTFA instance goes away */
  SaVersionT version;     /* SAF version of the client.*/
} ntfs_ckpt_reg_msg_t;

/* finalize checkpoint record, used in cold/async checkpoint updates */
typedef struct {
  uint32_t client_id;     /* Registration Id at Active */
} ntfsv_ckpt_finalize_msg_t;

typedef struct {
  ntfsv_send_not_req_t *arg;
} ntfs_ckpt_notification_t;

typedef struct {
  ntfsv_subscribe_req_t arg;
} ntfs_ckpt_subscribe_t;

typedef struct {
  ntfsv_unsubscribe_req_t arg;
} ntfs_ckpt_unsubscribe_t;

typedef struct {
  SaNtfIdentifierT notificationId;
} ntfs_ckpt_not_log_confirm_t;

typedef struct {
  uint32_t clientId;
  SaNtfSubscriptionIdT subscriptionId;
  SaNtfIdentifierT notificationId;
  ntfsv_discarded_type_t discarded;
} ntfs_ckpt_not_send_confirm_t;

/* Checkpoint message containing ntfs data of a particular type.
 * Used during cold and async updates.
 */
typedef struct {
  ntfsv_ckpt_msg_type_t ckpt_rec_type;    /* Type of ntfs data carried in this checkpoint */
  uint32_t num_ckpt_records;      /* =1 for async updates,>=1 for cold sync */
  uint32_t data_len;              /* Total length of encoded checkpoint data of this type */
} ntfsv_ckpt_header_t;

typedef struct {
  ntfsv_ckpt_header_t header;
  union {
    ntfs_ckpt_reg_msg_t reg_rec;
    ntfsv_ckpt_finalize_msg_t finalize_rec;
    ntfs_ckpt_notification_t notification;
    MDS_DEST agent_dest;
    ntfs_ckpt_subscribe_t subscribe;
    ntfs_ckpt_unsubscribe_t unsubscribe;
    ntfs_ckpt_not_log_confirm_t log_confirm;
    ntfs_ckpt_not_send_confirm_t send_confirm;
  } ckpt_rec;
} ntfsv_ckpt_msg_t;

typedef uint32_t (*NTFS_CKPT_HDLR) (ntfs_cb_t *cb, ntfsv_ckpt_msg_t *data);
uint32_t ntfs_mbcsv_init(ntfs_cb_t *ntfs_cb, SaAmfHAStateT ha_state);
uint32_t ntfs_mbcsv_change_HA_state(ntfs_cb_t *cb, SaAmfHAStateT ha_state);
uint32_t ntfs_mbcsv_dispatch(NCS_MBCSV_HDL mbcsv_hdl);
uint32_t ntfs_send_async_update(ntfs_cb_t *cb, ntfsv_ckpt_msg_t *ckpt_rec, uint32_t action);
void update_standby(ntfsv_ckpt_msg_t *ckpt, uint32_t action);
uint32_t enc_ckpt_reserv_header(NCS_UBAID *uba, ntfsv_ckpt_msg_type_t type, uint32_t num_rec, uint32_t len);
uint32_t enc_mbcsv_client_msg(NCS_UBAID *uba, ntfs_ckpt_reg_msg_t *param);

#endif  // NTF_NTFD_NTFS_MBCSV_H_
