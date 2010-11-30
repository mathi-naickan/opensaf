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

#include "eds.h"
#include "eds_ckpt.h"
#include "logtrace.h"

MDS_CLIENT_MSG_FORMAT_VER
 EDS_WRT_EDA_MSG_FMT_ARRAY[EDS_WRT_EDA_SUBPART_VER_RANGE] = {
	1 /*msg format version for EDA subpart version 1 */
};

/****************************************************************************
  Name          : eds_dec_initialize_msg
 
  Description   : This routine decodes an initialize API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_initialize_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_INIT_PARAM *param = &msg->info.api_info.param.init;
	uns8 local_data[20];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}
	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 3);
	param->version.releaseCode = ncs_decode_8bit(&p8);
	param->version.majorVersion = ncs_decode_8bit(&p8);
	param->version.minorVersion = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 3);
	total_bytes += 3;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_finalize_msg
 
  Description   : This routine decodes a finalize API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_finalize_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_FINALIZE_PARAM *param = &msg->info.api_info.param.finalize;
	uns8 local_data[20];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}
	/* reg_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->reg_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_chan_open_sync_msg
 
  Description   : This routine decodes a chan open sync API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_chan_open_sync_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_SYNC_PARAM *param = &msg->info.api_info.param.chan_open_sync;
	uns8 local_data[256];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* reg_id, chan_open_flags */
	p8 = ncs_dec_flatten_space(uba, local_data, 5);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_open_flags = ncs_decode_8bit(&p8);
	ncs_dec_skip_space(uba, 5);
	total_bytes += 5;

	/* chan_name length */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->chan_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* chan_name */
	ncs_decode_n_octets_from_uba(uba, param->chan_name.value, (uns32)param->chan_name.length);
	total_bytes += (uns32)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_chan_open_async_msg
 
  Description   : This routine decodes a chan open async API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_chan_open_async_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_ASYNC_PARAM *param = &msg->info.api_info.param.chan_open_async;
	uns8 local_data[256];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* invocation, reg_id, chan_open_flags */
	p8 = ncs_dec_flatten_space(uba, local_data, 15);
	param->inv = ncs_decode_64bit(&p8);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_open_flags = ncs_decode_8bit(&p8);
	param->chan_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 15);
	total_bytes += 15;

	/* chan_name */
	ncs_decode_n_octets_from_uba(uba, param->chan_name.value, (uns32)param->chan_name.length);
	total_bytes += (uns32)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_chan_close_msg
 
  Description   : This routine decodes a chan close API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_chan_close_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_CLOSE_PARAM *param = &msg->info.api_info.param.chan_close;
	uns8 local_data[20];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* reg_id, chan_id, chan_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_chan_unlink_msg
 
  Description   : This routine decodes a chan unlink API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_chan_unlink_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_UNLINK_PARAM *param = &msg->info.api_info.param.chan_unlink;
	uns8 local_data[256];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* reg_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->reg_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	/* chan_name length */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->chan_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* chan_name */
	ncs_decode_n_octets_from_uba(uba, param->chan_name.value, (uns32)param->chan_name.length);
	total_bytes += (uns32)param->chan_name.length;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_publish_msg
 
  Description   : This routine decodes a publish API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
uns32 eds_dec_publish_msg(NCS_UBAID *uba, long msg_hdl, uns8 ckpt_flag)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	uns64 num_patterns;
	uns32 fake_value;
	uns32 x;
	SaEvtEventPatternT *pattern_ptr;
	EDSV_EDA_PUBLISH_PARAM *param = NULL;
	uns8 local_data[1024];
	EDSV_MSG *msg = NULL;
	EDS_CKPT_DATA *ckpt_msg = NULL;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	if (ckpt_flag == TRUE) {
		ckpt_msg = (EDS_CKPT_DATA *)msg_hdl;
		if (!ckpt_msg) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
			return (0);
		}
		param = &ckpt_msg->ckpt_rec.retain_evt_rec.data;
	} else {
		msg = (EDSV_MSG *)msg_hdl;
		if (!msg) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
			return (0);
		}
		param = &msg->info.api_info.param.publish;
	}

	if (!param) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return (0);
	}
	/* reg_id, chan_id, chan_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 12);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 12);
	total_bytes += 12;

	/* Decode the patterns */

	/* patternsNumber */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	num_patterns = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	param->pattern_array = m_MMGR_ALLOC_EVENT_PATTERN_ARRAY;
	if (!param->pattern_array) {
		m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return (0);
	}
	param->pattern_array->patternsNumber = num_patterns;
	if (num_patterns) {
		param->pattern_array->patterns = m_MMGR_ALLOC_EVENT_PATTERNS((uns32)num_patterns);
		if (!param->pattern_array->patterns) {
			m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     0);
			return (0);
		}
	} else {
		param->pattern_array->patterns = NULL;
	}

	pattern_ptr = param->pattern_array->patterns;
	for (x = 0; x < (uns32)param->pattern_array->patternsNumber; x++) {
		/* patternSize */
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		pattern_ptr->patternSize = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		total_bytes += 8;

		/* For zero length patterns, fake decode zero */
		if (pattern_ptr->patternSize == 0) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			fake_value = ncs_decode_32bit(&p8);
			pattern_ptr->pattern = m_MMGR_ALLOC_EDSV_EVENT_DATA(0);
			ncs_dec_skip_space(uba, 4);
			total_bytes += 4;
		} else {
			/* pattern */
			pattern_ptr->pattern = m_MMGR_ALLOC_EDSV_EVENT_DATA((uns32)pattern_ptr->patternSize);
			if (!pattern_ptr->pattern) {
				m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__,
					     __LINE__, 0);
				return (0);
			}
			ncs_decode_n_octets_from_uba(uba, pattern_ptr->pattern, (uns32)pattern_ptr->patternSize);
			total_bytes += (uns32)pattern_ptr->patternSize;
		}
		pattern_ptr++;
	}

	/* priority, retention_time */
	p8 = ncs_dec_flatten_space(uba, local_data, 9);
	param->priority = ncs_decode_8bit(&p8);
	param->retention_time = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 9);
	total_bytes += 9;

	/* publisher_name length */
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	param->publisher_name.length = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;

	/* publisher_name */
	ncs_decode_n_octets_from_uba(uba, param->publisher_name.value, (uns32)param->publisher_name.length);
	total_bytes += (uns32)param->publisher_name.length;

	/* event_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->event_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	/* data_len */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->data_len = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	/* data */
	if ((uns32)param->data_len) {
		param->data = m_MMGR_ALLOC_EDSV_EVENT_DATA((uns32)param->data_len);
		if (!param->data) {
			m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     0);
			return (0);
		}

		ncs_decode_n_octets_from_uba(uba, param->data, (uns32)param->data_len);
	} else
		param->data = NULL;

	total_bytes += (uns32)param->data_len;

	/* If checkpoint data, decode publish time */
	if (ckpt_flag == TRUE) {
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		ckpt_msg->ckpt_rec.retain_evt_rec.pubtime = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		total_bytes += 8;
	}

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_subscribe_msg
 
  Description   : This routine decodes a subscribe API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
uns32 eds_dec_subscribe_msg(NCS_UBAID *uba, long msg_hdl, uns8 ckpt_flag)
{
	uns8 *p8;
	uns32 x;
	uns32 fake_value;
	uns64 num_filters;
	uns32 total_bytes = 0;
	SaEvtEventFilterT *filter_ptr;
	EDSV_EDA_SUBSCRIBE_PARAM *param = NULL;
	uns8 local_data[256];
	EDSV_MSG *msg = NULL;
	EDS_CKPT_DATA *ckpt_msg = NULL;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}
	if (ckpt_flag == TRUE) {
		ckpt_msg = (EDS_CKPT_DATA *)msg_hdl;
		if (!ckpt_msg) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
			return (0);
		}
		param = &ckpt_msg->ckpt_rec.subscribe_rec.data;
	} else {
		msg = (EDSV_MSG *)msg_hdl;
		if (!msg) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
			return (0);
		}
		param = &msg->info.api_info.param.subscribe;
	}

	if (!param) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return (0);
	}
	/* reg_id, chan_id, chan_open_id, subscription_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 16);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	param->sub_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 16);
	total_bytes += 16;

	/* Decode the filters.
	 * Must allocate space for these.
	 */

	/* Allocate the filterArray structure */
	param->filter_array = m_MMGR_ALLOC_FILTER_ARRAY;
	if (!param->filter_array) {
		m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return (0);
	}

	/* filtersNumber */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	num_filters = ncs_decode_64bit(&p8);
	param->filter_array->filtersNumber = num_filters;
	ncs_dec_skip_space(uba, 8);
	total_bytes += 8;

	/* Allocate all the filter structures needed */
	if (num_filters) {
		param->filter_array->filters = m_MMGR_ALLOC_EVENT_FILTERS((uns32)num_filters);
		if (!param->filter_array->filters)
			m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     0);
	} else {
		param->filter_array->filters = 0;
	}

	filter_ptr = param->filter_array->filters;

	for (x = 0; x < (uns32)param->filter_array->filtersNumber; x++) {
		/* filterType */
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		filter_ptr->filterType = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		total_bytes += 4;

		/* patternSize */
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		filter_ptr->filter.patternSize = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		total_bytes += 8;

		/* For zero length filters, fake decode zero */
		if (filter_ptr->filter.patternSize == 0) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			fake_value = ncs_decode_32bit(&p8);
			filter_ptr->filter.pattern = m_MMGR_ALLOC_EDSV_EVENT_DATA(0);
			ncs_dec_skip_space(uba, 4);
			total_bytes += 4;
		} else {
			/* pattern */
			filter_ptr->filter.pattern =
			    m_MMGR_ALLOC_EDSV_EVENT_DATA((uns32)filter_ptr->filter.patternSize);
			if (NULL == filter_ptr->filter.pattern) {
				m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__,
					     __LINE__, 0);
				return (0);
			}
			ncs_decode_n_octets_from_uba(uba, filter_ptr->filter.pattern,
						     (uns32)filter_ptr->filter.patternSize);
			total_bytes += (uns32)filter_ptr->filter.patternSize;
		}

		filter_ptr++;
	}

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_unsubscribe_msg
 
  Description   : This routine decodes a unsubscribe API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_unsubscribe_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_UNSUBSCRIBE_PARAM *param = &msg->info.api_info.param.unsubscribe;
	uns8 local_data[24];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}
	/* reg_id, chan_id, chan_open_id, subscription_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 16);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	param->sub_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 16);
	total_bytes += 16;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_dec_retention_time_clr_msg
 
  Description   : This routine decodes a retention time clear API msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_dec_retention_time_clr_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_RETENTION_TIME_CLR_PARAM *param = &msg->info.api_info.param.rettimeclr;
	uns8 local_data[24];

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* reg_id, chan_id, chan_open_id, event_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 16);
	param->reg_id = ncs_decode_32bit(&p8);
	param->chan_id = ncs_decode_32bit(&p8);
	param->chan_open_id = ncs_decode_32bit(&p8);
	param->event_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 16);
	total_bytes += 16;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_enc_initialize_rsp_msg
 
  Description   : This routine encodes an initialize resp msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_enc_initialize_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_INITIALIZE_RSP *param = &msg->info.api_resp_info.param.init_rsp;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* reg_id */
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	ncs_encode_32bit(&p8, param->reg_id);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_enc_limit_get_rsp_msg
 
  Description   : This routine encodes an Limit Get resp msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/

static uns32 eds_enc_limit_get_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_LIMIT_GET_RSP *param = &msg->info.api_resp_info.param.limit_get_rsp;

	if (uba == NULL) {
		return 0;
	}
	/* Decode the limits */
	p8 = ncs_enc_reserve_space(uba, 40);
	if (!p8) {
		return 0;
	}
	ncs_encode_64bit(&p8, param->max_chan);
	ncs_encode_64bit(&p8, param->max_evt_size);
	ncs_encode_64bit(&p8, param->max_ptrn_size);
	ncs_encode_64bit(&p8, param->max_num_ptrns);
	ncs_encode_64bit(&p8, param->max_ret_time);

	ncs_enc_claim_space(uba, 40);
	total_bytes += 40;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_enc_chan_open_sync_rsp_msg
 
  Description   : This routine decodes a chan open sync resp msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_enc_chan_open_sync_rsp_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_SYNC_RSP *param = &msg->info.api_resp_info.param.chan_open_rsp;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* chan_id, chan_open_id */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_enc_chan_open_cbk_msg
 
  Description   : This routine encodes a chan open callback msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_enc_chan_open_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CHAN_OPEN_CBK_PARAM *param = &msg->info.cbk_info.param.chan_open_cbk;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* chan_name length */
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_16bit(&p8, param->chan_name.length);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	/* chan_name */
	ncs_encode_n_octets_in_uba(uba, param->chan_name.value, (uns32)param->chan_name.length);
	total_bytes += (uns32)param->chan_name.length;

	/* chan_id, chan_open_id, chan_open_flags, eda_chan_hdl, error */
	p8 = ncs_enc_reserve_space(uba, 17);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_encode_8bit(&p8, param->chan_open_flags);
	ncs_encode_32bit(&p8, param->eda_chan_hdl);
	ncs_encode_32bit(&p8, param->error);
	ncs_enc_claim_space(uba, 17);
	total_bytes += 17;

	return total_bytes;
}

/****************************************************************************
  Name          : eds_enc_delv_evt_cbk_msg
 
  Description   : This routine encodes an event callback msg
 
  Arguments     : NCS_UBAID *msg,
                  EDSV_MSG *msg
                  
  Return Values : uns32
 
  Notes         : None.
******************************************************************************/
static uns32 eds_enc_delv_evt_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 x;
	uns32 total_bytes = 0;
	SaEvtEventPatternT *pattern_ptr;
	EDSV_EDA_EVT_DELIVER_CBK_PARAM *param = &msg->info.cbk_info.param.evt_deliver_cbk;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* type, reg_id, sub_id, chan_id, chan_open_id */
	p8 = ncs_enc_reserve_space(uba, 12);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_32bit(&p8, param->sub_id);
	ncs_encode_32bit(&p8, param->chan_id);
	ncs_encode_32bit(&p8, param->chan_open_id);
	ncs_enc_claim_space(uba, 12);
	total_bytes += 12;

	/* Encode the patterns */

	/* patternsNumber */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_64bit(&p8, param->pattern_array->patternsNumber);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	/* patterns */
	pattern_ptr = param->pattern_array->patterns;
	for (x = 0; x < param->pattern_array->patternsNumber; x++) {
		/* Save room for the patternSize field (8 bytes) */
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		}
		ncs_encode_64bit(&p8, pattern_ptr->patternSize);
		ncs_enc_claim_space(uba, 8);
		total_bytes += 8;

		/* For zero length patterns, fake encode zero */
		if (pattern_ptr->patternSize == 0) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__,
					     __LINE__, 0);
			}
			ncs_encode_32bit(&p8, 0);
			ncs_enc_claim_space(uba, 4);
			total_bytes += 4;
		} else {
			ncs_encode_n_octets_in_uba(uba, pattern_ptr->pattern, (uns32)pattern_ptr->patternSize);
			total_bytes += (uns32)pattern_ptr->patternSize;
		}
		pattern_ptr++;
	}

	/* priority */
	p8 = ncs_enc_reserve_space(uba, 1);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_8bit(&p8, param->priority);
	ncs_enc_claim_space(uba, 1);
	total_bytes += 1;

	/* publisher name length */
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_16bit(&p8, param->publisher_name.length);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	/* publisher name */
	ncs_encode_n_octets_in_uba(uba, param->publisher_name.value, (uns32)param->publisher_name.length);
	total_bytes += (uns32)param->publisher_name.length;

	/* publish_time,  eda_event_id */
	p8 = ncs_enc_reserve_space(uba, 24);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_64bit(&p8, param->publish_time);
	ncs_encode_64bit(&p8, param->retention_time);
	ncs_encode_32bit(&p8, (uns32)param->eda_event_id);
	/* event_hdl is skipped purposely */
	ncs_encode_32bit(&p8, (uns32)param->ret_evt_ch_oid);
	ncs_enc_claim_space(uba, 24);
	total_bytes += 24;

	/* Encode the data */

	/* data_len */
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_64bit(&p8, param->data_len);
	ncs_enc_claim_space(uba, 8);
	total_bytes += 8;

	/* data */
	ncs_encode_n_octets_in_uba(uba, param->data, (uns32)param->data_len);
	total_bytes += (uns32)param->data_len;

	return total_bytes;
}

static uns32 eds_enc_clm_status_cbk_msg(NCS_UBAID *uba, EDSV_MSG *msg)
{
	uns8 *p8;
	uns32 total_bytes = 0;
	EDSV_EDA_CLM_STATUS_CBK_PARAM *param = &msg->info.cbk_info.param.clm_status_cbk;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return 0;
	}

	/* ClmNodeStatus */
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_16bit(&p8, param->node_status);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;

	return total_bytes;
}

/****************************************************************************
 * Name          : eds_mds_cpy
 *
 * Description   : MDS copy.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_cpy(struct ncsmds_callback_info *info)
{
	/* TODO; */
	return NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : eds_mds_enc
 *
 * Description   : MDS encode.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_enc(struct ncsmds_callback_info *info)
{
	EDSV_MSG *msg;
	NCS_UBAID *uba;
	uns8 *p8;
	uns32 total_bytes = 0;
	MDS_CLIENT_MSG_FORMAT_VER msg_fmt_version;

	msg_fmt_version = m_NCS_ENC_MSG_FMT_GET(info->info.enc.i_rem_svc_pvt_ver,
						EDS_WRT_EDA_SUBPART_VER_AT_MIN_MSG_FMT,
						EDS_WRT_EDA_SUBPART_VER_AT_MAX_MSG_FMT, EDS_WRT_EDA_MSG_FMT_ARRAY);
	if (0 == msg_fmt_version) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR,
			     0, __FILE__, __LINE__, info->info.enc.i_rem_svc_pvt_ver);
		return NCSCC_RC_FAILURE;
	}
	info->info.enc.o_msg_fmt_ver = msg_fmt_version;
	msg = (EDSV_MSG *)info->info.enc.i_msg;
	uba = info->info.enc.io_uba;

	if (uba == NULL) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

   /** encode the type of message **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
	}
	ncs_encode_32bit(&p8, msg->type);
	ncs_enc_claim_space(uba, 4);
	total_bytes += 4;

	if (EDSV_EDA_API_RESP_MSG == msg->type) {
     /** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.type.raw);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		/* rc */
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		}
		ncs_encode_32bit(&p8, msg->info.api_resp_info.rc);
		ncs_enc_claim_space(uba, 4);
		total_bytes += 4;

		switch (msg->info.api_resp_info.type.api_rsp) {
		case EDSV_EDA_INITIALIZE_RSP_MSG:
			total_bytes += eds_enc_initialize_rsp_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_OPEN_SYNC_RSP_MSG:
			total_bytes += eds_enc_chan_open_sync_rsp_msg(uba, msg);
			break;
		case EDSV_EDA_CHAN_UNLINK_SYNC_RSP_MSG:
			break;
		case EDSV_EDA_CHAN_RETENTION_TIME_CLEAR_SYNC_RSP_MSG:
			break;
		case EDSV_EDA_LIMIT_GET_RSP_MSG:
			total_bytes += eds_enc_limit_get_rsp_msg(uba, msg);
			break;
		default:
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     msg->info.api_resp_info.type.raw);
			break;
		}
	} else if (EDSV_EDS_CBK_MSG == msg->type) {
     /** encode the API RSP msg subtype **/
		p8 = ncs_enc_reserve_space(uba, 16);
		if (!p8) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		}
		ncs_encode_32bit(&p8, msg->info.cbk_info.type);
		ncs_encode_32bit(&p8, msg->info.cbk_info.eds_reg_id);
		ncs_encode_64bit(&p8, msg->info.cbk_info.inv);
		ncs_enc_claim_space(uba, 16);
		total_bytes += 16;

		switch (msg->info.api_resp_info.type.cbk) {
		case EDSV_EDS_CHAN_OPEN:
			total_bytes += eds_enc_chan_open_cbk_msg(uba, msg);
			break;
		case EDSV_EDS_DELIVER_EVENT:
			total_bytes += eds_enc_delv_evt_cbk_msg(uba, msg);
			break;
		case EDSV_EDS_CLMNODE_STATUS:
			total_bytes += eds_enc_clm_status_cbk_msg(uba, msg);
			break;
		default:
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     msg->info.api_resp_info.type.raw);
			break;
		}
	} else {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, msg->type);
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_mds_dec
 *
 * Description   : MDS decode
 *
 * Arguments     : pointer to ncsmds_callback_info 
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_dec(struct ncsmds_callback_info *info)
{
	uns8 *p8;
	EDSV_EDS_EVT *evt;
	NCS_UBAID *uba = info->info.dec.io_uba;
	uns8 local_data[20];
	uns32 total_bytes = 0;
	long msg_hdl = 0;
	EDSV_MSG *msg = NULL;

	if (0 == m_NCS_MSG_FORMAT_IS_VALID(info->info.dec.i_msg_fmt_ver,
					   EDS_WRT_EDA_SUBPART_VER_AT_MIN_MSG_FMT,
					   EDS_WRT_EDA_SUBPART_VER_AT_MAX_MSG_FMT, EDS_WRT_EDA_MSG_FMT_ARRAY)) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0,
			     __FILE__, __LINE__, info->info.dec.i_msg_fmt_ver);
		return NCSCC_RC_FAILURE;
	}

   /** allocate an EDSV_EDS_EVENT now **/
	if (NULL == (evt = m_MMGR_ALLOC_EDSV_EDS_EVT)) {
		m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	memset(evt, '\0', sizeof(EDSV_EDS_EVT));

	/* Initialize the evt type */
	evt->evt_type = EDSV_EDS_EDSV_MSG;

	/* Assign the allocated event */
	info->info.dec.o_msg = (uns8 *)evt;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	evt->info.msg.type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	if (EDSV_EDA_API_MSG == evt->info.msg.type) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		evt->info.msg.info.api_info.type = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		total_bytes += 4;

		switch (evt->info.msg.info.api_info.type) {
		case EDSV_EDA_INITIALIZE:
			total_bytes += eds_dec_initialize_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_FINALIZE:
			total_bytes += eds_dec_finalize_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_CHAN_OPEN_SYNC:
			total_bytes += eds_dec_chan_open_sync_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_CHAN_OPEN_ASYNC:
			total_bytes += eds_dec_chan_open_async_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_CHAN_CLOSE:
			total_bytes += eds_dec_chan_close_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_CHAN_UNLINK:
			total_bytes += eds_dec_chan_unlink_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_PUBLISH:
			msg = &evt->info.msg;
			msg_hdl = (long)msg;	/* Pass the handle */
			total_bytes += eds_dec_publish_msg(uba, msg_hdl, FALSE);
			break;
		case EDSV_EDA_SUBSCRIBE:
			msg = &evt->info.msg;
			msg_hdl = (long)msg;	/* Pass the handle */
			total_bytes += eds_dec_subscribe_msg(uba, msg_hdl, FALSE);
			break;
		case EDSV_EDA_UNSUBSCRIBE:
			total_bytes += eds_dec_unsubscribe_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_RETENTION_TIME_CLR:
			total_bytes += eds_dec_retention_time_clr_msg(uba, &evt->info.msg);
			break;
		case EDSV_EDA_LIMIT_GET:
			/* Nothing to be decoded here */
			break;
		default:
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__,
				     evt->info.msg.info.api_info.type);
			break;
		}
	}

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_mds_enc_flat
 *
 * Description   : MDS encode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_enc_flat(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Retrieve info from the enc_flat */
	MDS_CALLBACK_ENC_INFO enc = info->info.enc_flat;
	/* Modify the MDS_INFO to populate enc */
	info->info.enc = enc;
	/* Invoke the regular mds_enc routine */
	rc = eds_mds_enc(info);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__, 0);

	return rc;
}

/****************************************************************************
 * Name          : eds_mds_dec_flat
 *
 * Description   : MDS decode and flatten
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_dec_flat(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* Retrieve info from the dec_flat */
	MDS_CALLBACK_DEC_INFO dec = info->info.dec_flat;
	/* Modify the MDS_INFO to populate dec */
	info->info.dec = dec;
	/* Invoke the regular mds_dec routine */
	rc = eds_mds_dec(info);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__, 0);
	return rc;
}

/****************************************************************************
 * Name          : eds_mds_rcv
 *
 * Description   : MDS rcv evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_rcv(struct ncsmds_callback_info *mds_info)
{
	EDSV_EDS_EVT *edsv_evt = (EDSV_EDS_EVT *)mds_info->info.receive.i_msg;
	EDS_CB *eds_cb = 0;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* retrieve EDS CB */
	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, (uns32)mds_info->i_yr_svc_hdl))) {
		m_LOG_EDSV_S(EDS_CB_TAKE_HANDLE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		eds_evt_destroy(edsv_evt);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

   /** Initialize the Event here **/
	edsv_evt->evt_type = EDSV_EDS_EDSV_MSG;
	edsv_evt->cb_hdl = (uns32)mds_info->i_yr_svc_hdl;
	edsv_evt->fr_node_id = mds_info->info.receive.i_node_id;
	edsv_evt->fr_dest = mds_info->info.receive.i_fr_dest;
	edsv_evt->rcvd_prio = mds_info->info.receive.i_priority;
	edsv_evt->mds_ctxt = mds_info->info.receive.i_msg_ctxt;

   /** The ESV_MSG has already been decoded into the event earlier 
    **/

	/* Push the event and we are done */
	if (NCSCC_RC_FAILURE == (rc = m_NCS_IPC_SEND(&eds_cb->mbx, edsv_evt, mds_info->info.receive.i_priority))) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, edsv_evt->evt_type, __FILE__,
			     __LINE__, mds_info->info.receive.i_node_id);
		eds_evt_destroy(edsv_evt);
		return NCSCC_RC_FAILURE;
	}

	/* return EDA CB */
	ncshm_give_hdl((uns32)mds_info->i_yr_svc_hdl);

	return rc;
}

/****************************************************************************
 * Name          : eds_mds_svc_event
 *
 * Description   : MDS subscription evt.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_svc_event(struct ncsmds_callback_info *info)
{
	uns32 eds_cb_hdl;
	EDS_CB *eds_cb = NULL;
	EDSV_EDS_EVT *evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;

	eds_cb_hdl = (uns32)info->i_yr_svc_hdl;

	/* Take the hdl */
	if ((eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, eds_cb_hdl)) == NULL) {
		m_LOG_EDSV_S(EDS_CB_TAKE_HANDLE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}

	/* First make sure that this event is indeed for us */
	if (info->info.svc_evt.i_your_id != NCSMDS_SVC_ID_EDS) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, info->info.svc_evt.i_your_id,
			     __FILE__, __LINE__, 0);
		rc = NCSCC_RC_FAILURE;
		goto give_hdl;
	}

	/* If this evt was sent from EDA act on this */
	if (info->info.svc_evt.i_svc_id == NCSMDS_SVC_ID_EDA) {
		if (info->info.svc_evt.i_change == NCSMDS_DOWN) {
			/* As of now we are only interested in EDA events */
			if (NULL == (evt = m_MMGR_ALLOC_EDSV_EDS_EVT)) {
				rc = NCSCC_RC_FAILURE;
				m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__,
					     __LINE__, 0);
				goto give_hdl;
			}

			memset(evt, '\0', sizeof(EDSV_EDS_EVT));
			evt->evt_type = EDSV_EDS_EVT_EDA_DOWN;

	 /** Initialize the Event Header **/
			evt->cb_hdl = eds_cb_hdl;
			evt->fr_node_id = info->info.svc_evt.i_node_id;
			evt->fr_dest = info->info.svc_evt.i_dest;

	 /** Initialize the MDS portion of the header **/
			evt->info.mds_info.node_id = info->info.svc_evt.i_node_id;
			evt->info.mds_info.mds_dest_id = info->info.svc_evt.i_dest;

			/* Push the event and we are done */
			if (m_NCS_IPC_SEND(&eds_cb->mbx, evt, NCS_IPC_PRIORITY_NORMAL) == NCSCC_RC_FAILURE) {
				m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR,
					     info->info.receive.i_node_id, __FILE__, __LINE__, evt->evt_type);
				eds_evt_destroy(evt);
				rc = NCSCC_RC_FAILURE;
				goto give_hdl;
			}

		}
	}
 give_hdl:
	/* Give the hdl back */
	ncshm_give_hdl((uns32)eds_cb_hdl);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_mds_sys_evt
 *
 * Description   : MDS sys evt .
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_sys_event(struct ncsmds_callback_info *mds_info)
{

	/* Not supported now */
	return NCSCC_RC_FAILURE;

}

/****************************************************************************
 * Name          : eds_mds_quiesced_ack
 *
 * Description   : MDS quised ack.
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_quiesced_ack(struct ncsmds_callback_info *mds_info)
{
	uns32 eds_cb_hdl;
	EDS_CB *eds_cb = NULL;
	EDSV_EDS_EVT *edsv_evt;
	uns32 rc = NCSCC_RC_SUCCESS;

	eds_cb_hdl = (uns32)mds_info->i_yr_svc_hdl;

   	/** allocate an EDSV_EDS_EVENT now **/
	if (NULL == (edsv_evt = m_MMGR_ALLOC_EDSV_EDS_EVT)) {
		m_LOG_EDSV_S(EDS_MEM_ALLOC_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		return NCSCC_RC_FAILURE;
	}
	memset(edsv_evt, '\0', sizeof(EDSV_EDS_EVT));

	/* retrieve EDS CB */
	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, (uns32)mds_info->i_yr_svc_hdl))) {
		m_LOG_EDSV_S(EDS_CB_TAKE_HANDLE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, 0);
		rc = NCSCC_RC_FAILURE;
		eds_evt_destroy(edsv_evt);
		return rc;
	}

	if (eds_cb->is_quisced_set == TRUE) {
      /** Initialize the Event here **/
		edsv_evt->evt_type = EDSV_EVT_QUIESCED_ACK;
		edsv_evt->cb_hdl = (uns32)mds_info->i_yr_svc_hdl;

      /** The ESV_MSG has already been decoded into the event earlier 
      **/

		/* Push the event and we are done */
		if (NCSCC_RC_FAILURE == m_NCS_IPC_SEND(&eds_cb->mbx, edsv_evt, NCS_IPC_PRIORITY_NORMAL)) {
			m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, edsv_evt->evt_type, __FILE__,
				     __LINE__, 0);
			ncshm_give_hdl((uns32)mds_info->i_yr_svc_hdl);
			eds_evt_destroy(edsv_evt);
			return NCSCC_RC_FAILURE;
		}

		/* return EDA CB */
		ncshm_give_hdl((uns32)mds_info->i_yr_svc_hdl);
		m_LOG_EDSV_S(EDS_MDS_SUCCESS, NCSFL_LC_EDSV_INIT, NCSFL_SEV_NOTICE, edsv_evt->evt_type, __FILE__,
			     __LINE__, 0);
	} else {
		ncshm_give_hdl((uns32)mds_info->i_yr_svc_hdl);
		eds_evt_destroy(edsv_evt);
	}

	return rc;
}

/****************************************************************************
 * Name          : eds_mds_callback
 *
 * Description   : Callback Dispatcher for various MDS operations.
 *
 * Arguments     : cb   : EDS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/

static uns32 eds_mds_callback(struct ncsmds_callback_info *info)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	static NCSMDS_CALLBACK_API cb_set[MDS_CALLBACK_SVC_MAX] = {
		eds_mds_cpy,	/* MDS_CALLBACK_COPY      */
		eds_mds_enc,	/* MDS_CALLBACK_ENC       */
		eds_mds_dec,	/* MDS_CALLBACK_DEC       */
		eds_mds_enc_flat,	/* MDS_CALLBACK_ENC_FLAT  */
		eds_mds_dec_flat,	/* MDS_CALLBACK_DEC_FLAT  */
		eds_mds_rcv,	/* MDS_CALLBACK_RECEIVE   */
		eds_mds_svc_event,	/* MDS_CALLBACK_SVC_EVENT */
		eds_mds_sys_event,	/* MDS_CALLBACK_SYS_EVENT */
		eds_mds_quiesced_ack	/* MDS_CALLBACK_QUIESCED_ACK */
	};

	if (info->i_op <= MDS_CALLBACK_QUIESCED_ACK) {
		rc = (*cb_set[info->i_op]) (info);
		return rc;
	} else {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, 0, __FILE__, __LINE__, info->i_op);
		return NCSCC_RC_SUCCESS;
	}
}

/****************************************************************************
 * Name          : eds_mds_vdest_create
 *
 * Description   : This function created the VDEST for EDS
 *
 * Arguments     : pointer to ncsmds_callback_info
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 eds_mds_vdest_create(EDS_CB *eds_cb)
{
	NCSVDA_INFO vda_info;
	uns32 rc = NCSCC_RC_SUCCESS;
	/*SaNameT name = {5, "EDSV"}; */
	memset(&vda_info, '\0', sizeof(NCSVDA_INFO));

#if 1
	eds_cb->vaddr = EDS_VDEST_ID;
#endif
	vda_info.req = NCSVDA_VDEST_CREATE;
	vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
	vda_info.info.vdest_create.i_persistent = FALSE;	/* Up-to-the application */
	/*  vda_info.info.vdest_create.i_persistent = TRUE; * Up-to-the application */
	/*  vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC; */
	vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;

#if 1				/* Commented for named vdest */
	/* We are using fixed values here for now */
	vda_info.info.vdest_create.info.specified.i_vdest = eds_cb->vaddr;
#endif
	/* Create the VDEST address */
	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		m_LOG_EDSV_S(EDS_MDS_VDEST_CREATE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__,
			     0);
		return rc;
	}
	m_LOG_EDSV_S(EDS_MDS_VDEST_CREATE_SUCCESS, NCSFL_LC_EDSV_INIT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__, 0);

	/* Store the info returned by MDS */
	eds_cb->mds_hdl = vda_info.info.vdest_create.o_mds_pwe1_hdl;
	eds_cb->mds_vdest_hdl = vda_info.info.vdest_create.o_mds_vdest_hdl;

	return rc;
}

/****************************************************************************
 * Name          : eds_mds_init
 *
 * Description   : This function creates the VDEST for eds and installs/suscribes
 *                 into MDS.
 *
 * Arguments     : cb   : EDS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 eds_mds_init(EDS_CB *cb)
{
	NCSMDS_INFO mds_info;
	uns32 rc;
	MDS_SVC_ID svc = NCSMDS_SVC_ID_EDA;

	/* Create the VDEST for EDS */
	if (NCSCC_RC_SUCCESS != (rc = eds_mds_vdest_create(cb))) {
		LOG_ER(" eds_mds_init: named vdest create FAILED");
		return rc;
	}

	/* Set the role of MDS */
	if (cb->ha_state == SA_AMF_HA_ACTIVE)
		cb->mds_role = V_DEST_RL_ACTIVE;
	if (cb->ha_state == SA_AMF_HA_STANDBY)
		cb->mds_role = V_DEST_RL_STANDBY;

	if (NCSCC_RC_SUCCESS != (rc = eds_mds_change_role(cb))) {
		m_LOG_EDSV_S(EDS_MDS_INIT_ROLE_CHANGE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__,
			     __LINE__, 0);
		LOG_ER(" eds_mds_init: MDS role change to %d FAILED\n", cb->mds_role);
		return rc;
	}
	m_LOG_EDSV_S(EDS_MDS_INIT_ROLE_CHANGE_SUCCESS, NCSFL_LC_EDSV_INIT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__, 0);

	/* Install your service into MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDS;
	mds_info.i_op = MDS_INSTALL;

	mds_info.info.svc_install.i_yr_svc_hdl = cb->my_hdl;
	mds_info.info.svc_install.i_install_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_install.i_svc_cb = eds_mds_callback;
	mds_info.info.svc_install.i_mds_q_ownership = FALSE;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = EDS_SVC_PVT_SUBPART_VERSION;

	if (NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))) {
		LOG_ER(" eds_mds_init: MDS Install FAILED\n");
		m_LOG_EDSV_S(EDS_MDS_INSTALL_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__, 0);
		return rc;
	}
	m_LOG_EDSV_S(EDS_MDS_INSTALL_SUCCESS, NCSFL_LC_EDSV_INIT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__, 0);

	/* Now subscribe for EDS events in MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDS;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_svc_ids = &svc;

	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER(" eds_mds_init: MDS subscribe FAILED\n");
		m_LOG_EDSV_S(EDS_MDS_SUBSCRIBE_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__, 0);
		return rc;
	}
	m_LOG_EDSV_S(EDS_MDS_SUBSCRIBE_SUCCESS, NCSFL_LC_EDSV_INIT, NCSFL_SEV_INFO, rc, __FILE__, __LINE__, 0);
	return rc;
}

/****************************************************************************
 * Name          : eds_mds_change_role
 *
 * Description   : This function informs mds of our vdest role change 
 *
 * Arguments     : cb   : EDS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 eds_mds_change_role(EDS_CB *cb)
{
	NCSVDA_INFO arg;

	memset(&arg, 0, sizeof(NCSVDA_INFO));

	arg.req = NCSVDA_VDEST_CHG_ROLE;
	arg.info.vdest_chg_role.i_vdest = cb->vaddr;
	arg.info.vdest_chg_role.i_new_role = cb->mds_role;

	if (ncsvda_api(&arg) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : eds_mds_vdest_destroy
 *
 * Description   : This function Destroys the Virtual destination of EDS
 *
 * Arguments     : cb   : EDS control Block pointer.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 eds_mds_vdest_destroy(EDS_CB *eds_cb)
{
	NCSVDA_INFO vda_info;
	uns32 rc;

	memset(&vda_info, '\0', sizeof(NCSVDA_INFO));

	vda_info.req = NCSVDA_VDEST_DESTROY;
	vda_info.info.vdest_destroy.i_vdest = eds_cb->vaddr;

	if (NCSCC_RC_SUCCESS != (rc = ncsvda_api(&vda_info))) {
		m_LOG_EDSV_S(EDS_MDS_VDEST_DESTROY_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__,
			     0);
		return rc;
	}

	return rc;
}

/****************************************************************************
 * Name          : eds_mds_finalize
 *
 * Description   : This function un-registers the EDS Service with MDS.
 *
 * Arguments     : Uninstall EDS from MDS.
 *
 * Return Values : NCSCC_RC_SUCCESS/Error Code.
 *
 * Notes         : None.
 *****************************************************************************/
uns32 eds_mds_finalize(EDS_CB *cb)
{
	NCSMDS_INFO mds_info;
	uns32 rc;

	/* Un-install EDS service from MDS */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDS;
	mds_info.i_op = MDS_UNINSTALL;

	rc = ncsmds_api(&mds_info);

	if (rc != NCSCC_RC_SUCCESS) {
		m_LOG_EDSV_S(EDS_MDS_UNINSTALL_FAILED, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__, 0);
		return rc;
	}

	/* Destroy the virtual Destination of EDS */
	rc = eds_mds_vdest_destroy(cb);
	return rc;
}

/****************************************************************************
  Name          : eds_mds_msg_send
 
  Description   : This routine sends the EDA message to EDS. The send 
                  operation may be a 'normal' send or a synchronous call that 
                  blocks until the response is received from EDS.
 
  Arguments     : cb  - ptr to the EDA CB
                  i_msg - ptr to the EDSv message
                  dest  - MDS destination to send to.
                  mds_ctxt - ctxt for synch mds req-resp.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/

uns32 eds_mds_msg_send(EDS_CB *cb,
		       EDSV_MSG *msg, MDS_DEST *dest, MDS_SYNC_SND_CTXT *mds_ctxt, MDS_SEND_PRIORITY_TYPE prio)
{
	NCSMDS_INFO mds_info;
	MDS_SEND_INFO *send_info = &mds_info.info.svc_send;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* populate the mds params */
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDS;
	mds_info.i_op = MDS_SEND;

	send_info->i_msg = msg;
	send_info->i_to_svc = NCSMDS_SVC_ID_EDA;
	send_info->i_priority = prio;	/* Discuss the priority assignments TBD */

	if (NULL == mds_ctxt || 0 == mds_ctxt->length) {
		/* regular send */
		MDS_SENDTYPE_SND_INFO *send = &send_info->info.snd;

		send_info->i_sendtype = MDS_SENDTYPE_SND;
		send->i_to_dest = *dest;
	} else {
		/* response message (somebody is waiting for it) */
		MDS_SENDTYPE_RSP_INFO *resp = &send_info->info.rsp;

		send_info->i_sendtype = MDS_SENDTYPE_RSP;
		resp->i_sender_dest = *dest;
		resp->i_msg_ctxt = *mds_ctxt;
	}

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if (rc != NCSCC_RC_SUCCESS)
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, rc, __FILE__, __LINE__,
			     m_NCS_NODE_ID_FROM_MDS_DEST(*dest));
	return rc;
}

/****************************************************************************
  Name          : eds_mds_ack_send
 
  Description   : This routine sends the EDA message to EDS. The send 
                  operation blocks until an MDS ack is received from EDS.
 
  Arguments     : cb  - ptr to the EDA CB
                  msg - ptr to the EDSv message
                  dest - MDS dest to send to.
                  timeout - time to wait for the ack from EDA. 
                  prio - priority of the message to send.
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE/timeout
 
  Notes         : None.
******************************************************************************/

uns32 eds_mds_ack_send(EDS_CB *cb, EDSV_MSG *msg, MDS_DEST dest, uns32 timeout, uns32 prio)
{
	NCSMDS_INFO mds_info;
	uns32 rc = NCSCC_RC_SUCCESS;
	if (NULL == msg)
		return NCSCC_RC_FAILURE;
	memset(&mds_info, '\0', sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_EDS;
	mds_info.i_op = MDS_SEND;

	/* Fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)msg;
	mds_info.info.svc_send.i_priority = prio;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_EDA;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDACK;

	/* fill the sub send rsp strcuture */
	mds_info.info.svc_send.info.sndack.i_time_to_wait = timeout;	/* timeto wait in 10ms */
	mds_info.info.svc_send.info.sndack.i_to_dest = dest;	/* This dest is eda's. */

	/* send the message */
	if (NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))) {
		m_LOG_EDSV_S(EDS_MDS_FAILURE, NCSFL_LC_EDSV_INIT, NCSFL_SEV_ERROR, msg->type, __FILE__, __LINE__,
			     m_NCS_NODE_ID_FROM_MDS_DEST(dest));
	}

	return rc;
}
