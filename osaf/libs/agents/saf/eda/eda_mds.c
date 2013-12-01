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

#include "logtrace.h"
#include "osaf_poll.h"

#include "eda.h"

MDS_CLIENT_MSG_FORMAT_VER
 EDA_WRT_EDS_MSG_FMT_ARRAY[EDA_WRT_EDS_SUBPART_VER_RANGE] = {
	1 /*msg format version for EDA subpart version 1 */
};

/****************************************************************************
  Name          : eda_enc_initialize_msg
 
  Description   : This routine encodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_initialize_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_INIT_PARAM *param = &msg->info.api_info.param.init;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}
   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 3);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_8bit(&p8, param->version.releaseCode);
	ncs_encode_8bit(&p8, param->version.majorVersion);
	ncs_encode_8bit(&p8, param->version.minorVersion);
	ncs_enc_claim_space(uba, 3);
	total_bytes += 3;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_finalize_msg
 
  Description   : This routine encodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_finalize_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_FINALIZE_PARAM *param = &msg->info.api_info.param.finalize;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_chan_open_sync_msg
 
  Description   : This routine encodes a chan open sync API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_chan_open_sync_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_SYNC_PARAM *param = &msg->info.api_info.param.chan_open_sync;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 7);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_8bit(&p8, param->chan_open_flags);
	ncs_encode_16bit(&p8, param->chan_name.length);
	ncs_enc_claim_space(uba, 7);
	total_bytes += 7;

	ncs_encode_n_octets_in_uba(uba, param->chan_name.value, (uint32_t)param->chan_name.length);
	total_bytes += (uint32_t)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_chan_open_async_msg
 
  Description   : This routine encodes a chan open async API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_chan_open_async_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_ASYNC_PARAM *param = &msg->info.api_info.param.chan_open_async;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 15);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_64bit(&p8, param->inv);
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_8bit(&p8, param->chan_open_flags);
	ncs_encode_16bit(&p8, param->chan_name.length);
	ncs_enc_claim_space(uba, 15);
	total_bytes += 15;

	ncs_encode_n_octets_in_uba(uba, param->chan_name.value, (uint32_t)param->chan_name.length);
	total_bytes += (uint32_t)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_chan_close_msg
 
  Description   : This routine encodes a chan close API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_chan_close_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_CHAN_CLOSE_PARAM *param = &msg->info.api_info.param.chan_close;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_chan_unlink_msg
 
  Description   : This routine encodes a chan unlink API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_chan_unlink_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_CHAN_UNLINK_PARAM *param = &msg->info.api_info.param.chan_unlink;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 6);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_16bit(&p8, param->chan_name.length);
	ncs_enc_claim_space(uba, 6);
	total_bytes += 6;

	ncs_encode_n_octets_in_uba(uba, param->chan_name.value, (uint32_t)param->chan_name.length);
	total_bytes += (uint32_t)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_publish_msg
 
  Description   : This routine encodes a publish API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_publish_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	uint32_t x;
	SaEvtEventPatternT *pattern_ptr;
	EDSV_EDA_PUBLISH_PARAM *param = &msg->info.api_info.param.publish;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 20);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_64bit(&p8, param->pattern_array->patternsNumber);
	ncs_enc_claim_space(uba, 20);
	total_bytes += 20;

	/* Encode the patterns */
	pattern_ptr = param->pattern_array->patterns;
	for (x = 0; x < (uint32_t)param->pattern_array->patternsNumber; x++) {
		/* Save room for the patternSize field (8 bytes) */
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE_4("reserve space failed");
		}
		ncs_encode_64bit(&p8, pattern_ptr->patternSize);
		ncs_enc_claim_space(uba, 8);
		total_bytes += 8;

		/* For zero length patterns, fake encode zero */
		if (pattern_ptr->patternSize == 0) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE_4("reserve space failed");
			}
			ncs_encode_32bit(&p8, 0);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
		} else {
			ncs_encode_n_octets_in_uba(uba, pattern_ptr->pattern, (uint32_t)pattern_ptr->patternSize);
			total_bytes += (uint32_t)pattern_ptr->patternSize;
		}
		pattern_ptr++;
	}

	p8 = ncs_enc_reserve_space(uba, 11);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_8bit(&p8, param->priority);
	ncs_encode_64bit(&p8, param->retention_time);
	ncs_encode_16bit(&p8, param->publisher_name.length);
	ncs_enc_claim_space(uba, 11);
	total_bytes += 11;

	ncs_encode_n_octets_in_uba(uba, param->publisher_name.value, (uint32_t)param->publisher_name.length);
	total_bytes += (uint32_t)param->publisher_name.length;

	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->event_id);
	ncs_encode_64bit(&p8, param->data_len);
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	ncs_encode_n_octets_in_uba(uba, param->data, (uint32_t)param->data_len);
	total_bytes += (uint32_t)param->data_len;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_subscribe_msg
 
  Description   : This routine encodes a subscribe API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_subscribe_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t x;
	uint32_t total_bytes = 0;
	SaEvtEventFilterT *filter_ptr;
	EDSV_EDA_SUBSCRIBE_PARAM *param = &msg->info.api_info.param.subscribe;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 24);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_32bit(&p8, param->sub_id);
	ncs_encode_64bit(&p8, param->filter_array->filtersNumber);
	ncs_enc_claim_space(uba, 24);
	total_bytes += 24;

	/* Encode the filters */
	filter_ptr = param->filter_array->filters;
	for (x = 0; x < (uint32_t)param->filter_array->filtersNumber; x++) {
		/* Save room for the filterType(4 bytes), patternSize(8 bytes)
		 */
		p8 = ncs_enc_reserve_space(uba, 12);
		if (!p8) {
			TRACE_4("reserve space failed");
		}
		ncs_encode_32bit(&p8, filter_ptr->filterType);
		ncs_encode_64bit(&p8, filter_ptr->filter.patternSize);
		ncs_enc_claim_space(uba, 12);
		total_bytes = 12;

		/* For zero length filters, fake encode zero */
		if ((uint32_t)filter_ptr->filter.patternSize == 0) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE_4("reserve space failed");
			}
			ncs_encode_32bit(&p8, 0);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
		} else {
			ncs_encode_n_octets_in_uba(uba, filter_ptr->filter.pattern,
						   (uint32_t)filter_ptr->filter.patternSize);
			total_bytes += (uint32_t)filter_ptr->filter.patternSize;
		}
		filter_ptr++;
	}

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_unsubscribe_msg
 
  Description   : This routine encodes a unsubscribe API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_unsubscribe_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_UNSUBSCRIBE_PARAM *param = &msg->info.api_info.param.unsubscribe;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 16);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_32bit(&p8, param->sub_id);
	ncs_enc_claim_space(uba, 16);
	total_bytes += 16;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_enc_retention_time_clr_msg
 
  Description   : This routine encodes a retention time clear API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_enc_retention_time_clr_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_RETENTION_TIME_CLR_PARAM *param = &msg->info.api_info.param.rettimeclr;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

   /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 16);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, param->reg_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_32bit(&p8, param->event_id);
	ncs_enc_claim_space(uba, 16);
	total_bytes += 16;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_dec_chan_open_cbk_msg
 
  Description   : This routine decodes a channel open callback message
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_dec_chan_open_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	int32_t total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_CBK_PARAM *param = &msg->info.cbk_info.param.chan_open_cbk;
	uint8_t local_data[256];

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

	/* chan_name_len */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->chan_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* chan_name */
	ncs_decode_n_octets_from_uba(uba, param->chan_name.value, (uint32_t)param->chan_name.length);
	total_bytes += (uint32_t)param->chan_name.length;

	/* chan_id, chan_open_id, chan_open_flags, eda_chan_hdl, error */
	p8 = ncs_dec_flatten_space(uba, local_data, 17);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	param->chan_open_flags = ncs_decode_8bit(&p8);
	param->eda_chan_hdl = ncs_decode_32bit(&p8);
	param->error = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 17);
	total_bytes += 17;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_dec_delv_evt_cbk_msg
 
  Description   : This routine decodes a deliver event callback message
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_dec_delv_evt_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t x;
	uint32_t fake_value;
	uint64_t num_patterns;
	uint32_t total_bytes = 0;
	SaEvtEventPatternT *pattern_ptr;
	EDSV_EDA_EVT_DELIVER_CBK_PARAM *param = &msg->info.cbk_info.param.evt_deliver_cbk;
	uint8_t local_data[1024];

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

	/* sub_id, chan_id, chan_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->sub_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;

	/* Decode the patterns.
	 * Must allocate space for these.
	 */

	/* patternsNumber */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	num_patterns = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	param->pattern_array = m_MMGR_ALLOC_EVENT_PATTERN_ARRAY;
	if (!param->pattern_array) {
		TRACE_4("malloc failed for pattern array");
		return 0;
	}
	param->pattern_array->patternsNumber = num_patterns;
	if (num_patterns) {
		param->pattern_array->patterns = m_MMGR_ALLOC_EVENT_PATTERNS((uint32_t)num_patterns);
		if (!param->pattern_array->patterns) {
			TRACE_4("malloc failed for patternarray->patterns");
			return 0;
		}
	} else {
		param->pattern_array->patterns = NULL;
	}

	pattern_ptr = param->pattern_array->patterns;
	for (x = 0; x < param->pattern_array->patternsNumber; x++) {
		/* patternSize */
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		pattern_ptr->patternSize = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		total_bytes += 8;

		/* For zero length patterns, fake decode zero */
		if (pattern_ptr->patternSize == 0) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			fake_value = ncs_decode_32bit(&p8);
			TRACE("pattern size: %u", fake_value);
			/* Do so the free routine is happy */
			pattern_ptr->pattern = m_MMGR_ALLOC_EDSV_EVENT_DATA(0);
			ncs_dec_skip_space(uba, 4);
			total_bytes += 4;
		} else {
			/* pattern */
			pattern_ptr->pattern = m_MMGR_ALLOC_EDSV_EVENT_DATA((uint32_t)pattern_ptr->patternSize);
			if (!pattern_ptr->pattern) {
				TRACE_4("malloc failed for event data");
				return 0;
			}
			ncs_decode_n_octets_from_uba(uba, pattern_ptr->pattern, (uint32_t)pattern_ptr->patternSize);
			total_bytes += (uint32_t)pattern_ptr->patternSize;
		}
		pattern_ptr++;
	}

	/* priority */
	p8 = ncs_dec_flatten_space(uba, local_data, 1);
	param->priority = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 1);
	total_bytes += 1;

	/* publisher_name length */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->publisher_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* publisher_name */
	ncs_decode_n_octets_from_uba(uba, param->publisher_name.value, (uint32_t)param->publisher_name.length);
	total_bytes += (uint32_t)param->publisher_name.length;

	/* publish_time, eda_event_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 24);
	param->publish_time = ncs_decode_64bit(&p8);
	param->retention_time = ncs_decode_64bit(&p8);
	param->eda_event_id = ncs_decode_32bit(&p8);
	param->ret_evt_ch_oid = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 24);
	total_bytes += 24;

	/* data_len */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->data_len = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	/* data */
	if ((uint32_t)param->data_len) {
		param->data = m_MMGR_ALLOC_EDSV_EVENT_DATA((uint32_t)param->data_len);
		if (!param->data) {
			TRACE_4("malloc failedi for event data");
			return 0;
		}
		ncs_decode_n_octets_from_uba(uba, param->data, (uint32_t)param->data_len);
	} else
		param->data = NULL;

	total_bytes += (uint32_t)param->data_len;

	return total_bytes;
}

static uint32_t eda_dec_clm_status_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	int32_t total_bytes = 0;
	EDSV_EDA_CLM_STATUS_CBK_PARAM *param = &msg->info.cbk_info.param.clm_status_cbk;
	uint8_t local_data[256];

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return 0;
	}

	/* ClmNodeStatus */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->node_status = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;
	return total_bytes;
}

/****************************************************************************
  Name          : eda_eds_msg_proc
 
  Description   : This routine is used to process the ASYNC incoming
                  EDS messages. 
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_eds_msg_proc(EDA_CB *eda_cb, EDSV_MSG *edsv_msg, MDS_SEND_PRIORITY_TYPE prio)
{

	switch (edsv_msg->type) {
	case EDSV_EDS_CBK_MSG:
		switch (edsv_msg->info.cbk_info.type) {
		case EDSV_EDS_CHAN_OPEN:
			{
				EDA_CLIENT_HDL_REC *eda_hdl_rec;
				EDA_CHANNEL_HDL_REC *channel_hdl_rec;
				EDSV_EDA_CHAN_OPEN_CBK_PARAM *cbk_param = &edsv_msg->info.cbk_info.param.chan_open_cbk;
	    /** Create the chan hdl record here before 
             ** queing this message onto the priority queue
             ** so that the dispatch by the application to fetch
             ** the callback is instantaneous.
             **/

	    /** Lookup the hdl rec by reg_id 
             **/
				if (NULL == (eda_hdl_rec = eda_find_hdl_rec_by_regid(eda_cb,
										     edsv_msg->info.cbk_info.
										     eds_reg_id))) {
					TRACE_4("client handle record for reg_id: %u not found",
										 edsv_msg->info.cbk_info.eds_reg_id);
					eda_msg_destroy(edsv_msg);
					return NCSCC_RC_FAILURE;
				}

	    /** Create/add a channel record to the hdl rec with 
             ** the information received in this message. 
             ** only if the return status was SA_AIS_OK.
             **/
				if (SA_AIS_OK == cbk_param->error) {
					if (NULL == (channel_hdl_rec = eda_channel_hdl_rec_add(&eda_hdl_rec,
											       cbk_param->chan_id,
											       cbk_param->chan_open_id,
											       cbk_param->
											       chan_open_flags,
											       &cbk_param->
											       chan_name))) {
						TRACE_4("channel add failed for chan_id: %u, chan_open_id: %u, \
							channelname: %s", cbk_param->chan_id, cbk_param->chan_open_id,
											cbk_param->chan_name.value);
						eda_msg_destroy(edsv_msg);
						return NCSCC_RC_FAILURE;
					}

	      /** pass on the channel_hdl to the application thru cbk
               **/
					cbk_param->eda_chan_hdl = channel_hdl_rec->channel_hdl;
				}

	    /** enqueue this message anyway
             **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&eda_hdl_rec->mbx, edsv_msg, prio)) {
					TRACE_4("IPC send failed failed for msg type: %u", edsv_msg->type);
					return NCSCC_RC_FAILURE;
				}
			}
			break;
		case EDSV_EDS_DELIVER_EVENT:
			{
				EDA_CLIENT_HDL_REC *eda_hdl_rec;
				EDA_CHANNEL_HDL_REC *chan_hdl_rec;
				EDA_EVENT_HDL_REC *evt_hdl_rec;
				EDSV_EDA_EVT_DELIVER_CBK_PARAM *evt_dlv_param =
				    &edsv_msg->info.cbk_info.param.evt_deliver_cbk;

	    /** Lookup the hdl rec 
             **/
				if (NULL == (eda_hdl_rec = eda_find_hdl_rec_by_regid(eda_cb,
										     edsv_msg->info.cbk_info.
										     eds_reg_id))) {
					TRACE_4("reg record not found reg_id: %u", edsv_msg->info.cbk_info.eds_reg_id);
					edsv_free_evt_pattern_array(evt_dlv_param->pattern_array);
					evt_dlv_param->pattern_array = NULL;
		/** free the event data if any **/
					if (evt_dlv_param->data) {
						m_MMGR_FREE_EDSV_EVENT_DATA(evt_dlv_param->data);
						evt_dlv_param->data = NULL;
					}
					eda_msg_destroy(edsv_msg);
					return NCSCC_RC_FAILURE;
				}

	    /** Lookup the channel record to which
             ** this event belongs
             **/
				if (NULL == (chan_hdl_rec = eda_find_chan_hdl_rec_by_chan_id(eda_hdl_rec,
											     evt_dlv_param->chan_id,
											     evt_dlv_param->
											     chan_open_id))) {
					TRACE_4("chan rec not found for chan_id: %u, chan_open_id: %u", 
						evt_dlv_param->chan_id, evt_dlv_param->chan_open_id);
					edsv_free_evt_pattern_array(evt_dlv_param->pattern_array);
					evt_dlv_param->pattern_array = NULL;
		/** free the event data if any **/
					if (evt_dlv_param->data) {
						m_MMGR_FREE_EDSV_EVENT_DATA(evt_dlv_param->data);
						evt_dlv_param->data = NULL;
					}
					eda_msg_destroy(edsv_msg);
					return NCSCC_RC_FAILURE;
				}

	    /** Create/Add the new event record.
             **/
				if (NULL == (evt_hdl_rec = eda_event_hdl_rec_add(&chan_hdl_rec)))
				{
					edsv_free_evt_pattern_array(evt_dlv_param->pattern_array);
					evt_dlv_param->pattern_array = NULL;
		/** free the event data if any **/
					if (evt_dlv_param->data) {
						m_MMGR_FREE_EDSV_EVENT_DATA(evt_dlv_param->data);
						evt_dlv_param->data = NULL;
					}
					eda_msg_destroy(edsv_msg);
					TRACE_4("event record add failed");
					return NCSCC_RC_FAILURE;
				}

	    /** Initialize the fields in the evt_hdl_rec with data
             ** received in the message.
             **/
				evt_hdl_rec->priority = evt_dlv_param->priority;
				evt_hdl_rec->publisher_name = evt_dlv_param->publisher_name;
				evt_hdl_rec->publish_time = evt_dlv_param->publish_time;
				evt_hdl_rec->retention_time = evt_dlv_param->retention_time;
				evt_hdl_rec->event_data_size = evt_dlv_param->data_len;

	    /** mark the event as rcvd.
             **/
				evt_hdl_rec->evt_type |= EDA_EVT_RECEIVED;

	    /** Create/Add the new event inst record.
             **/

	    /** The evt hdl rec will take ownership of the memory
             ** for the patterns & data to avoid too many copies
             ** and not that much use of these in the callback.
             **/
				evt_hdl_rec->del_evt_id = evt_dlv_param->eda_event_id;
				evt_hdl_rec->pattern_array = evt_dlv_param->pattern_array;
				evt_dlv_param->pattern_array = NULL;

				evt_hdl_rec->evt_data = evt_dlv_param->data;
				evt_dlv_param->data = NULL;

				/* assign the newly allocated hdl */
				evt_dlv_param->event_hdl = evt_hdl_rec->event_hdl;

	    /** enqueue this message. MDS & IPC priority match 1-1 
             **/
				if (NCSCC_RC_SUCCESS != m_NCS_IPC_SEND(&eda_hdl_rec->mbx, edsv_msg, prio)) {
					TRACE_4("IPC send failed for msg type: %u", edsv_msg->type);
					return NCSCC_RC_FAILURE;
				}

			}
			break;
		case EDSV_EDS_CLMNODE_STATUS:
			{
				EDSV_EDA_CLM_STATUS_CBK_PARAM *clm_status_param =
				    &edsv_msg->info.cbk_info.param.clm_status_cbk;

				eda_cb->node_status = (SaClmClusterChangesT)clm_status_param->node_status;
				TRACE_1("Local node membership changed to : %u", eda_cb->node_status);
			}
			break;
		default:
			 TRACE_3("unknown message type: %u", edsv_msg->info.cbk_info.type);
			return NCSCC_RC_FAILURE;
			break;
		}
		break;
	case EDSV_EDS_MISC_MSG:
      /** No messages conceived yet **/
		TRACE_1("Unsupported message type");
		return NCSCC_RC_FAILURE;
		break;
	default:
      /** Unexpected message **/
		TRACE_4("Wrong message type");
		return NCSCC_RC_FAILURE;
		break;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : eda_mds_svc_evt
 
  Description   : This is a callback routine that is invoked to inform EDA 
                  of MDS events. EDA had subscribed to these events during
                  through MDS subscription.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uint32_t eda_mds_svc_evt(struct ncsmds_callback_info *mds_cb_info)
{
	uint32_t eda_cb_hdl;
	EDA_CB *eda_cb = NULL;

	/* Retrieve the EDA_CB hdl */
	eda_cb_hdl = (uint32_t)mds_cb_info->i_yr_svc_hdl;

	TRACE("EDA Rcvd MDS subscribe evt from svc %d", mds_cb_info->info.svc_evt.i_svc_id);

	/* Take the EDA hdl */
	if ((eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, eda_cb_hdl)) == NULL) {
		TRACE_4("unable to retrieve global cb handle: %u", eda_cb_hdl);
		return NCSCC_RC_SUCCESS;
	}

	switch (mds_cb_info->info.svc_evt.i_change) {
	case NCSMDS_NO_ACTIVE:
	case NCSMDS_DOWN:
		if (mds_cb_info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_EDS) {
	 /** TBD what to do if EDS goes down
          ** Hold on to the subscription if possible
          ** to send them out if EDS comes back up
          **/
			TRACE_1("Event Server is DOWN on node_id: %u", 
				m_NCS_NODE_ID_FROM_MDS_DEST(mds_cb_info->info.svc_evt.i_dest));
			eda_cb->eds_intf.eds_mds_dest = mds_cb_info->info.svc_evt.i_dest;
			eda_cb->eds_intf.eds_up = false;
			if (mds_cb_info->info.svc_evt.i_change == NCSMDS_DOWN)
				eda_cb->eds_intf.eds_up_publish = false;
		}
		break;
	case NCSMDS_NEW_ACTIVE:
	case NCSMDS_UP:
		switch (mds_cb_info->info.svc_evt.i_svc_id) {
		case NCSMDS_SVC_ID_EDS:
	       /** Store the MDS DEST of the EDS 
                **/
			TRACE_1("Event Server is UP on node_id: %u", 
				m_NCS_NODE_ID_FROM_MDS_DEST(mds_cb_info->info.svc_evt.i_dest));
			m_NCS_LOCK(&eda_cb->eds_sync_lock, NCS_LOCK_WRITE);
			eda_cb->eds_intf.eds_mds_dest = mds_cb_info->info.svc_evt.i_dest;
			eda_cb->eds_intf.eds_up = true;
			eda_cb->eds_intf.eds_up_publish = true;

			if (eda_cb->eds_sync_awaited == true) {
				m_NCS_SEL_OBJ_IND(eda_cb->eds_sync_sel);
			}
			m_NCS_UNLOCK(&eda_cb->eds_sync_lock, NCS_LOCK_WRITE);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	/* Give the hdl back */
	ncshm_give_hdl((uint32_t)eda_cb_hdl);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : eda_mds_rcv
 
  Description   : This is a callback routine that is invoked when EDA message
                  is received from EDS.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/

static uint32_t eda_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	EDSV_MSG *edsv_msg = (EDSV_MSG *)mds_cb_info->info.receive.i_msg;
	EDA_CB *eda_cb = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;

	/* retrieve EDA CB */
	if (NULL == (eda_cb = (EDA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDA, (uint32_t)mds_cb_info->i_yr_svc_hdl))) {
		TRACE_4("Unable to retrieve handle on eda control block: %u", (uint32_t)mds_cb_info->i_yr_svc_hdl);
		eda_msg_destroy(edsv_msg);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

   /** Lock EDA_CB
    **/
	m_NCS_LOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

	/* process the message */
	rc = eda_eds_msg_proc(eda_cb, edsv_msg, mds_cb_info->info.receive.i_priority);
	if (rc != NCSCC_RC_SUCCESS) {
	}
   /** Unlock the EDA_CB
    **/
	m_NCS_UNLOCK(&eda_cb->cb_lock, NCS_LOCK_WRITE);

	/* return EDA CB */
	ncshm_give_hdl((uint32_t)mds_cb_info->i_yr_svc_hdl);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : eda_mds_enc
 
  Description   : This is a callback routine that is invoked to encode EDS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_mds_enc(struct ncsmds_callback_info *info)
{
	EDSV_MSG *msg;
	NCS_UBAID *uba;
	uint8_t *p8;
	uint32_t total_bytes = 0;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						EDA_WRT_EDS_SUBPART_VER_AT_MIN_MSG_FMT,
						EDA_WRT_EDS_SUBPART_VER_AT_MAX_MSG_FMT, EDA_WRT_EDS_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		TRACE_2("Unsupport message format version");
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;
	msg = (EDSV_MSG *)info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		TRACE_4("uba is NULL");
		return NCSCC_RC_FAILURE;
	}

  /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE_4("reserve space failed");
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (EDSV_EDA_API_MSG == msg->type) {
      /** encode the API msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE_4("reserve space failed");
		}
		ncs_encode_32bit(&p8, msg->info.api_info.type);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		switch (msg->info.api_info.type) {
		case EDSV_EDA_INITIALIZE:
			total_bytes += eda_enc_initialize_msg(uba, msg);
			break;
		case EDSV_EDA_FINALIZE:
			total_bytes += eda_enc_finalize_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_OPEN_SYNC:
			total_bytes += eda_enc_chan_open_sync_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_OPEN_ASYNC:
			total_bytes += eda_enc_chan_open_async_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_CLOSE:
			total_bytes += eda_enc_chan_close_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_UNLINK:
			total_bytes += eda_enc_chan_unlink_msg(uba, msg);
			break;
		case EDSV_EDA_PUBLISH:
			total_bytes += eda_enc_publish_msg(uba, msg);
			break;
		case EDSV_EDA_SUBSCRIBE:
			total_bytes += eda_enc_subscribe_msg(uba, msg);
			break;
		case EDSV_EDA_UNSUBSCRIBE:
			total_bytes += eda_enc_unsubscribe_msg(uba, msg);
			break;
		case EDSV_EDA_RETENTION_TIME_CLR:
			total_bytes += eda_enc_retention_time_clr_msg(uba, msg);
			break;
		case EDSV_EDA_LIMIT_GET:
			/* Nothing to encode in this request */
			break;
		default:
			break;
		}
	}

	TRACE("Total bytes encoded in message: %u, msgtype: %u", total_bytes, msg->info.api_info.type);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : eda_dec_initialize_rsp_msg
 
  Description   : This routine decodes an initialize sync response message
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_dec_initialize_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_INITIALIZE_RSP *param = &msg->info.api_resp_info.param.init_rsp;
	uint8_t local_data[100];

	if (NULL == uba) {
		TRACE_4("uba is NULL");
		return 0;
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->reg_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_dec_limit_get_rsp_msg
 
  Description   : This routine decodes an limit get sync response message
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_dec_limit_get_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_LIMIT_GET_RSP *param = &msg->info.api_resp_info.param.limit_get_rsp;
	uint8_t local_data[100];

	if (NULL == uba) {
		TRACE("NULL uba received for decoding limit get response message\n");
		return 0;
	}

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->max_chan = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->max_evt_size = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->max_ptrn_size = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->max_num_ptrns = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->max_ret_time = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_dec_chan_open_sync_rsp_msg
 
  Description   : This routine decodes a channel open sync response message
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uint32_t
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_dec_chan_open_sync_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uint8_t *p8;
	uint32_t total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_SYNC_RSP *param = &msg->info.api_resp_info.param.chan_open_rsp;
	uint8_t local_data[100];

	if (NULL == uba) {
		TRACE_4("uba is NULL");
		return 0;
	}

	/* chan_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->chan_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	/* chan_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->chan_open_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : eda_mds_dec
 
  Description   : This is a callback routine that is invoked to decode EDS
                  messages.
 
  Arguments     : pointer to struct ncsmds_callback_info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_mds_dec(struct ncsmds_callback_info *info)
{
	uint8_t *p8;
	EDSV_MSG *msg;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uint8_t local_data[20];
	uint32_t total_bytes = 0;

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   EDA_WRT_EDS_SUBPART_VER_AT_MIN_MSG_FMT,
					   EDA_WRT_EDS_SUBPART_VER_AT_MAX_MSG_FMT, EDA_WRT_EDS_MSG_FMT_ARRAY)) {
		TRACE_2("Unsupported message format version");
		return NCSCC_RC_FAILURE;
	}

   /** Allocate a new msg in both sync/async cases 
    **/
	if (NULL == (msg = m_MMGR_ALLOC_EDSV_MSG)) {
		TRACE_4("malloc failed");
		return NCSCC_RC_FAILURE;
	}

	memset(msg, '\0', sizeof(EDSV_MSG));

	info->info.dec.o_msg = (uint8_t *)msg;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	msg->type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	switch (msg->type) {
	case EDSV_EDA_API_RESP_MSG:
		{
			TRACE("msg subtype: %u", msg->info.api_resp_info.type.api_rsp);
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			msg->info.api_resp_info.type.raw = ncs_decode_32bit(&p8);
			msg->info.api_resp_info.rc = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 8);
			total_bytes += 8;
			switch (msg->info.api_resp_info.type.api_rsp) {
			case EDSV_EDA_INITIALIZE_RSP_MSG:
				total_bytes += eda_dec_initialize_rsp_msg(uba, msg);
				break;
			case EDSV_EDA_CHAN_OPEN_SYNC_RSP_MSG:
				total_bytes += eda_dec_chan_open_sync_rsp_msg(uba, msg);
				break;
			case EDSV_EDA_LIMIT_GET_RSP_MSG:
				total_bytes += eda_dec_limit_get_rsp_msg(uba, msg);
				break;
			default:
				break;
			}
		}
		break;
	case EDSV_EDS_CBK_MSG:
		{
			TRACE("msg subtype: %u", msg->info.cbk_info.type);
			p8 = ncs_dec_flatten_space(uba, local_data, 16);
			msg->info.cbk_info.type = ncs_decode_32bit(&p8);
			msg->info.cbk_info.eds_reg_id = ncs_decode_32bit(&p8);
			msg->info.cbk_info.inv = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(uba, 16);
			total_bytes += 16;
			switch (msg->info.cbk_info.type) {
			case EDSV_EDS_CHAN_OPEN:
				total_bytes += eda_dec_chan_open_cbk_msg(uba, msg);
				break;
			case EDSV_EDS_DELIVER_EVENT:
				total_bytes += eda_dec_delv_evt_cbk_msg(uba, msg);
				break;
			case EDSV_EDS_CLMNODE_STATUS:
				total_bytes += eda_dec_clm_status_cbk_msg(uba, msg);
			default:
				break;
			}
		}
		break;
	default:
		break;
	}

	TRACE("Total bytes decoded from message: %u", total_bytes);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eda_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eda_mds_enc_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO enc = info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = enc;
	/* Invoke the regular mds_enc routine  */
	rc = eda_mds_enc(info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_4("mds encode failed");

	return rc;
}

/****************************************************************************
 * Name          : eda_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eda_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* Retrieve info from the dec_flat     */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine  */
	rc = eda_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_4("decode flat failed");
	return rc;
}

/****************************************************************************
  Name          : eda_mds_cpy
 
  Description   : This function copies an events sent from EDS.
 
  Arguments     :pointer to struct ncsmds_callback_info
  
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
static uint32_t eda_mds_cpy(struct ncsmds_callback_info *info)
{
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eda_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : cb   : EDS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t eda_mds_callback(struct ncsmds_callback_info *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		eda_mds_cpy,	/* MDS_CALLBACK_COPY      */
		eda_mds_enc,	/* MDS_CALLBACK_ENC       */
		eda_mds_dec,	/* MDS_CALLBACK_DEC       */
		eda_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  */
		eda_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  */
		eda_mds_rcv,	/* MDS_CALLBACK_RECEIVE   */
		eda_mds_svc_evt	/* MDS_CALLBACK_SVC_EVENT */
	};

	if (info->i_op <= MDS_CALLBACK_SVC_EVENT) {
		rc = (*cb_set[info->i_op]) (info);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("invalid callback message: %u", info->i_op);
		return rc;
	} else
		return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : eda_mds_init
 
  Description   : This routine registers the EDA Service with MDS.
 
  Arguments     : cb - ptr to the EDA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t eda_mds_init(EDA_CB *cb)
{
	NCSADA_INFO ada_info;
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_EDS;

   /** Create the ADEST for EDA and get the pwe hdl**/
	memset(&ada_info, '\0', sizeof(ada_info));
	ada_info.req = NCSADA_GET_HDLS;

	if (NCSCC_RC_SUCCESS != (rc = ncsada_api(&ada_info))) {
		TRACE_4("mds adest create/handle failed");
		return NCSCC_RC_FAILURE;
	}

   /** Store the info obtained from MDS ADEST creation 
    **/
	cb->eds_intf.mds_hdl = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;
	cb->eds_intf.eda_mds_dest = ada_info.info.adest_get_hdls.o_adest;

   /** Now install into mds **/
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->eds_intf.mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDA;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = cb->cb_hdl;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;	/* PWE scope */
	mds_info.info.svc_install.i_svc_cb = eda_mds_callback;	/* callback */
	mds_info.info.svc_install.i_mds_q_ownership = false;	/* EDA doesn't own the mds queue */
	mds_info.info.svc_install.i_mds_svc_pvt_ver = EDA_SVC_PVT_SUBPART_VERSION;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE_4("mds install failed");
		return NCSCC_RC_FAILURE;
	}

	/* Now subscribe for events that will be generated by MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->eds_intf.mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDA;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		TRACE_4("mds subscribe failed");
		return rc;
	}
	return rc;
}

/********************************************************************
 Name    :  eda_sync_with_eds

 Description : This is for EDA to sync with EDS when it gets MDS callback
   
**********************************************************************/
void eda_sync_with_eds(EDA_CB *cb)
{
	m_NCS_LOCK(&cb->eds_sync_lock, NCS_LOCK_WRITE);

	if (cb->eds_intf.eds_up) {
		m_NCS_UNLOCK(&cb->eds_sync_lock, NCS_LOCK_WRITE);
		return;
	}

	cb->eds_sync_awaited = true;
	m_NCS_SEL_OBJ_CREATE(&cb->eds_sync_sel);
	m_NCS_UNLOCK(&cb->eds_sync_lock, NCS_LOCK_WRITE);

	/* Await indication from MDS saying EDS is up */
	osaf_poll_one_fd(m_GET_FD_FROM_SEL_OBJ(cb->eds_sync_sel), 30000);

	/* Destroy the sync - object */
	m_NCS_LOCK(&cb->eds_sync_lock, NCS_LOCK_WRITE);

	cb->eds_sync_awaited = false;
	m_NCS_SEL_OBJ_DESTROY(cb->eds_sync_sel);

	m_NCS_UNLOCK(&cb->eds_sync_lock, NCS_LOCK_WRITE);
	return;
}

/****************************************************************************
  Name          : eda_mds_finalize
 
  Description   : This routine unregisters the EDA Service from MDS.
 
  Arguments     : cb - ptr to the EDA control block
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
void eda_mds_finalize(EDA_CB *cb)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	/* Un-install your service into MDS. 
	   No need to cancel the services that are subscribed */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->eds_intf.mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDA;
	mds_info.i_op = MDS_UNINSTALL;

	if ((rc = ncsmds_api(&mds_info)) != NCSCC_RC_SUCCESS) {
		TRACE_4("mds unsintall failed");
	}
	return;
}

/****************************************************************************
  Name          : eda_mds_msg_sync_send
 
  Description   : This routine sends the EDA message to EDS. The send 
                  operation is a synchronous call that 
                  blocks until the response is received from EDS.
 
  Arguments     : cb  - ptr to the EDA CB
                  i_msg - ptr to the EDSv message
                  o_msg - double ptr to EDSv message response
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/
uint32_t eda_mds_msg_sync_send(EDA_CB *cb, EDSV_MSG *i_msg, EDSV_MSG **o_msg, uint32_t timeout)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (NULL == i_msg) {
		TRACE_2("NULL message received for mds sync send");
		return NCSCC_RC_FAILURE;
	}
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->eds_intf.mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDA;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	if ((i_msg->info.api_info.type == EDSV_EDA_RETENTION_TIME_CLR) ||
		(i_msg->info.api_info.type == EDSV_EDA_INITIALIZE))
		mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	else
		mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_HIGH;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_EDS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndrsp.i_to_dest = cb->eds_intf.eds_mds_dest;

	/* send the message */
	if (NCSCC_RC_SUCCESS == (rc = ncsmds_api(&mds_info))) {
		/* Retrieve the response and take ownership of the memory 
		 */
		*o_msg = (EDSV_MSG *)mds_info.info.svc_send.info.sndrsp.o_rsp;
		mds_info.info.svc_send.info.sndrsp.o_rsp = NULL;
	} else
		TRACE_2("mds sync send failed. dest: %" PRIx64 ", node_id: %u", cb->eds_intf.eds_mds_dest,
						 m_NCS_NODE_ID_FROM_MDS_DEST(cb->eds_intf.eds_mds_dest));

	return rc;
}

/****************************************************************************
  Name          : eda_mds_msg_async_send
 
  Description   : This routine sends the EDA message to EDS.
 
  Arguments     : cb  - ptr to the EDA CB
                  i_msg - ptr to the EDSv message
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t eda_mds_msg_async_send(struct eda_cb_tag *cb, struct edsv_msg *i_msg, uint32_t prio)
{
	NCSMDS_INFO mds_info;
	uint32_t rc = NCSCC_RC_SUCCESS;
	if (NULL == i_msg) {
		TRACE_2("NULL message received for mds async send");
		return NCSCC_RC_FAILURE;
	}
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->eds_intf.mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDA;
	mds_info.i_op = MDS_SEND;

	/* fill the main send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_msg;
	mds_info.info.svc_send.i_priority = prio;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_EDS;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;

	/* fill the sub send strcuture */
	mds_info.info.svc_send.info.snd.i_to_dest = cb->eds_intf.eds_mds_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		TRACE_2("mds async send failed. dest: %" PRIx64 ", node_id: %u", cb->eds_intf.eds_mds_dest,
			     m_NCS_NODE_ID_FROM_MDS_DEST(cb->eds_intf.eds_mds_dest));

	return rc;
}
