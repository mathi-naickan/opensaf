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
 */

#ifndef MBC_APITEST_MBCSV_PURPOSE_H_
#define MBC_APITEST_MBCSV_PURPOSE_H_

/* enum vlaues */
/* typedef enum mbcstm_test_event {
   MBCSTM_PURPOSE_SAMPLE
   
   } MBCSTM_TEST_EVENT; */

#define MBCSTM_PERF_MSG_MAX_SIZE 1010
#define MBCSTM_PERF_MAX_MSG_NUM     25

typedef enum {
  TET_MBCSV_PASS,
  TET_MBCSV_FAIL,
  TET_MBCSV_NORESULT = 8
}TET_MBCSV_RESULT;

typedef enum {
  MBCSTM_CHECK_PEER_LIST,
  MBCSTM_CHECK_FSM,
}MBCSTM_CHECK;

typedef enum {
  MBCSTM_TEST_ACTION_NO,
  MBCSTM_TEST_ACTION_ROLE,
  MBCSTM_TEST_ACTION_CLOSE,
  MBCSTM_TEST_ACTION_GET,
  MBCSTM_TEST_ACTION_SET
} MBCSTM_TEST_ACTION;

typedef enum mbcstm_cb_test {
  MBCSTM_CB_NO_TEST,
  MBCSTM_CB_PEER_INFO_FAIL,
  MBCSTM_CB_PEER_INFO_PASS,
  MBCSTM_CB_PEER_DISC_PERFORM,
  MBCSTM_CB_STANDBY_COLD_ENCODE_FAIL,
  MBCSTM_CB_STANDBY_COLD_DECODE_FAIL,
  MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL,
  MBCSTM_CB_ACTIVE_COLD_ENCODE_FAIL,
  MBCSTM_CB_STANDBY_WARM_ENCODE_FAIL,
  MBCSTM_CB_STANDBY_WARM_DECODE_FAIL,
  MBCSTM_CB_ACTIVE_WARM_DECODE_FAIL,
  MBCSTM_CB_ACTIVE_WARM_ENCODE_FAIL,
  MBCSTM_CB_STANDBY_WARM_CMPL_FAIL,
  MBCSTM_CB_ACTIVE_WARM_ON_OFF,
  MBCSTM_CB_ACTIVE_COLD_TIMER_EXP,
  MBCSTM_CB_ACTIVE_COLD_CMP_TIMER_EXP,
  MBCSTM_CB_ACTIVE_WARM_TIMER_EXP,
  MBCSTM_CB_ACTIVE_WARM_CMP_TIMER_EXP,
  MBCSTM_CB_ACTIVE_DRSP_TIMER_EXP,
  MBCSTM_CB_STANDBY_COLD_ENCODE_CHECK,
  MBCSTM_CB_VERSION_CHECK
} MBCSTM_CB_TEST;

typedef struct ssn_perf_data {
  uint32_t length;
  uint32_t crc;
  char *msg;
} SSN_PERF_DATA;

/* some gloable varialbes */
uint32_t case_num, cases_passed, cases_failed;

/* test  purpose declarations */
uint32_t mbcstm_startup(void);
uint32_t mbcstm_final_results(void);
uint32_t mbcstm_test_print(uint32_t case_num,char * case_name, char *disc,
                     TET_MBCSV_RESULT exp_res, TET_MBCSV_RESULT final_res);
uint32_t mbcstm_test_pupose_model(void);
uint32_t mbcstm_wait_to_end(void);


/* session discovery related cases */
void mbcstm_se_newbackup_with_active(void);
void mbcstm_se_newbackup_with_mactive(void);
void mbcstm_se_xbackup_with_mactive(void);
void mbcstm_se_active_to_active(void);
void mbcstm_se_active_to_standby(void);
void mbcstm_se_active_to_quiesced(void);
void mbcstm_se_standby_to_standby(void);
void mbcstm_se_standby_to_active(void);
void mbcstm_se_standby_to_quiesced(void);
void mbcstm_se_quiesced_to_active(void);
void mbcstm_se_quiesced_to_standby(void);
void mbcstm_se_quiesced_to_quiesced(void);

/* event related cases */
void  mbcstm_peerup_event_newactive_to_active(void);
void  mbcstm_peerup_event_newactive_to_standby(void);
void  mbcstm_peerup_event_newstandby_to_active(void);
void  mbcstm_peerup_event_newstandby_to_standby(void);
void  mbcstm_peerdown_event_active(void);
void  mbcstm_peerdown_event_standby(void);
void  mbcstm_cngrole_event_newactive_to_active(void);
void  mbcstm_cngrole_event_newactive_to_standby(void);
void  mbcstm_cngrole_event_newstandby_to_standby(void);
void  mbcstm_cngrole_event_newstandby_to_active(void);
void  mbcstm_active_multiactive_standby(void);
void  mbcstm_standby_active_quiesced(void);
void  mbcstm_multi_multi_active(void);
void  mbcstm_multi_multi_active_standby(void);

/* call back related cases */
void mbcstm_coldsync_standby_encode_fail(void);
void mbcstm_coldsync_coldrequest_noresponse(void);
void mbcstm_coldsync_stdby_decode_fail(void);
void mbcstm_coldsync_active_encode_fail(void);
void mbcstm_coldsync_timer(void);
void mbcstm_coldsync_cmp_timer(void);
void mbcstm_warmsync_timer(void);
void mbcstm_warmsync_cmp_timer(void);
void mbcstm_warmsync_complete(void);
void mbcstm_data_request_idle(void);

/* check point related cases */
void  mbcstm_ckpt_sendsync(void);
void  mbcstm_ckpt_send_usrasync(void);
void  mbcstm_ckpt_send_mbcasync(void);
void  mbcstm_ckpt_stb_send(void);
void  mbcstm_ckpt_standby_sendsync(void);
void  mbcstm_ckpt_idle_sendsync(void);

/* notify related cases */
void mbcstm_notify_send_all(void);
void mbcstm_send_notify_idle(void);
void mbcstm_notify_send_to_active(void);
void mbcstm_notify_send_to_standby(void);
void mbcstm_notify_send_active_to_multiactive(void);
void mbcstm_notify_send_active_to_active(void);

/* set get object cases */
void mbcstm_set_warm_sync_off_active(void);
void mbcstm_set_warm_sync_off_standby(void);
void mbcstm_set_warm_sync_on_standby(void);
void mbcstm_set_warm_sync_timer_active(void);
void mbcstm_set_warm_sync_timer_standby(void);
void mbcstm_get_warm_sync_timer(void);
void mbcstm_get_warm_sync_on_off(void);

/* MDS cases */
void mbcstm_mbcsv_down_event(void);
void mbcstm_mbcsv_up_event(void);

/* message verification or performance related cases */
void mbcstm_test_sync_send_messages(void);
void mbcstm_test_usrasync_send_messages(void);
void mbcstm_test_mbcasync_send_messages(void);
void mbcstm_peer_discover_performance(void);
/* General cases */
void mbcstm_open_up_down(void);

uint32_t mbcstm_sync_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys, 
                           MBCSTM_CB_TEST sync,uint32_t warm,uint32_t wait);
uint32_t mbcstm_data_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                           uint32_t wait, MBCSTM_CB_TEST sync);
uint32_t mbcstm_notify_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                             uint32_t isys, MBCSTM_CB_TEST sync, 
                             NCS_MBCSV_NTFY_MSG_DEST msg_dest,char *str, 
                             uint32_t len);
uint32_t mbcstm_cb_test_cases(NCS_MBCSV_CB_ARG *arg);
void mbcstm_coldsync_standby_encode_fail(void);
void mbcstm_coldsync_coldrequest_noresponse(void);
void mbcstm_coldsync_stdby_decode_fail(void); 
void mbcstm_coldsync_active_encode_fail(void);
void mbcstm_coldsync_timer(void);
void mbcstm_coldsync_cmp_timer(void);
void mbcstm_warmsync_timer(void);
void mbcstm_warmsync_cmp_timer(void);
void mbcstm_warmsync_complete(void);
void mbcstm_data_request_idle(void);
void mbcstm_send_notify_idle(void);
void mbcstm_notify_send_all(void);
void mbcstm_notify_send_to_active(void);
void mbcstm_notify_send_to_standby(void);
void mbcstm_notify_send_active_to_active(void);
void mbcstm_notify_send_active_to_multiactive(void);
uint32_t mbcstm_create_data_point(uint32_t svc_index, uint32_t ssn_index);
uint32_t mbcstm_print_data_points(uint32_t svc_index, uint32_t ssn_index);
uint32_t mbcstm_destroy_data_point(uint32_t svc_index, uint32_t ssn_index);
uint32_t mbcstm_ckpt_send_purpose(uint32_t svc_index, uint32_t ssn_index,uint32_t asys,
                               uint32_t send_index, uint32_t send_count,
                               NCS_MBCSV_ACT_TYPE action, 
                               NCS_MBCSV_MSG_TYPE send_type,
                               MBCSTM_CB_TEST sync);
void mbcstm_ckpt_sendsync(void);
void mbcstm_ckpt_standby_sendsync(void);
void mbcstm_ckpt_idle_sendsync(void);
void mbcstm_ckpt_send_usrasync(void);
void mbcstm_ckpt_send_mbcasync(void);
uint32_t mbcstm_close(void);
//uint32_t tet_mbcsv_startup(void);

#endif  // MBC_APITEST_MBCSV_PURPOSE_H_
