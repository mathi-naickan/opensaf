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
  MODULE NAME: mbcsv_fsm.c 

  REVISION HISTORY:

  DESCRIPTION:

  This module contains fsm routines for the MBCSv State Machine.

  FUNCTIONS INCLUDED in this module:

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "mbcsv.h"

/* Add ncs_mbcsv_null_func in each state table for the added event */

const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_init_state_tbl[NCS_MBCSV_INIT_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1] = {
  /** INIT -  IDLE STATE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 }
};

const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_standby_state_tbl[NCS_MBCSV_STBY_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1] = {
  /** STANDBY - STBY_STATE_IDLE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  STANDBY - WAIT TO COLD SYNC STATE **/
	{
	 ncs_mbcsv_rcv_async_update,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_rcv_cold_sync_resp,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_rcv_cold_sync_resp_cmplt,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_send_cold_sync,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_send_cold_sync_tmr,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_cold_sync_cmplt_tmr,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  STANDBY - STEADY IN SYNC STATE **/
	{
	 ncs_mbcsv_rcv_async_update,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_send_warm_sync,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_send_warm_sync_tmr,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  STANDBY - WAIT TO WARM SYNC STATE **/
	{
	 ncs_mbcsv_rcv_async_update,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_rcv_warm_sync_resp,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_rcv_warm_sync_resp_cmplt,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_send_warm_sync,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_send_warm_sync_tmr,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_warm_sync_cmplt_tmr,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  STANDBY - VERIFY WARM SYNC STATE **/
	{
	 ncs_mbcsv_rcv_async_update,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_send_data_req,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_rcv_entity_in_sync,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  STANDBY - WAIT FOR DATA RESPONSE STATE **/
	{
	 ncs_mbcsv_rcv_async_update,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_rcv_data_resp,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_rcv_data_resp_cmplt,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_send_data_req,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_send_data_req_tmr,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 }
};

const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_active_state_tbl[NCS_MBCSV_ACT_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1] = {
  /** ACTIVE -  IDLE STATE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE  [for WBB!!] */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_state_to_mul_act,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /** ACTIVE -  WAIT FOR COLD WARM SYNC STATE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_rcv_cold_sync,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_rcv_warm_sync,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_send_async_update,	/* NCSMBCSV_SEND_ASYNC_UPDATE  [for WBB!!] */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_state_to_mul_act,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

  /**  ACTIVE - KEEP STANDBY IN SYNC STATE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_rcv_cold_sync,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_rcv_warm_sync,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_rcv_data_req,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_send_async_update,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_send_cold_sync_resp,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_send_warm_sync_resp,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_send_warm_sync_resp_cmplt,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_send_data_resp,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_send_data_resp_cmplt,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_transmit_tmr,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_state_to_mul_act,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 },

 /**  ACTIVE - MULTIPLE ACTIVE SATE **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_rcv_notify,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_send_notify,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE            */
	 ncs_mbcsv_state_to_wfcs,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_state_to_kstby_sync	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 }

};

const NCS_MBCSV_STATE_ACTION_FUNC_PTR
 ncsmbcsv_quiesced_state_tbl[NCS_MBCSV_QUIESCED_MAX_STATES][NCSMBCSV_NUM_EVENTS - 1] = {
  /** QUIESCED STATE - quiesced state **/
	{
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_ASYNC_UPDATE             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_COLD_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_REQ            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_WARM_SYNC_RESP_COMPLETE  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_REQ                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP                */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_DATA_RESP_COMPLETE       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_NOTIFY                   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_REQ             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_ASYNC_UPDATE              */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_REQ                  */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_COLD_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP            */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_WARM_SYNC_RESP_COMPLETE   */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_DATA_RESP_COMPLETE        */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_ENTITY_IN_SYNC                 */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_SEND_NOTIFY                    */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_COLD_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_SEND_WARM_SYNC       */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_COLD_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_WARM_SYNC_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_DATA_RESP_CMPLT      */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_TMR_TRANSMIT             */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_MULTIPLE_ACTIVE           */
	 ncs_mbcsv_null_func,	/* NCSMBCSV_EVENT_STATE_TO_WAIT_FOR_CW_SYNC */
	 ncs_mbcsv_null_func	/* NCSMBCSV_EVENT_STATE_TO_KEEP_STBY_SYNC */
	 }
};
