/*      -*- OpenSAF  -*-/ 
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

#include <ncsencdec_pub.h>
#include "ntfsv_enc_dec.h"
#include "ntfsv_mem.h"

typedef union {
	uint32_t uint32_val;
	float float_val;
	uint64_t uint64_val;
	double double_val;
} binfl_t;

static void print_object_attribute(SaNtfAttributeT *input)
{
	(void)TRACE_2("Attr ID: %d\n", (int)input->attributeId);
	(void)TRACE_2("Attr Type: %d\n", (int)input->attributeType);
	(void)TRACE_2("Attr Value: %d\n", (int)input->attributeValue.int32Val);
}

void ntfsv_print_object_attributes(SaNtfAttributeT *objectAttributes, SaUint16T numAttributes)
{
	int i;
	(void)TRACE_2("numAttr: %d\n", (int)numAttributes);

	for (i = 0; i < numAttributes; i++) {
		print_object_attribute(&objectAttributes[i]);
	}
}

static uint32_t encodeSaNtfValueT(NCS_UBAID *uba, uint8_t *p8, SaNtfValueTypeT attributeType,
			       SaNtfValueT *ntfAttr)
{
	uint32_t rv = NCSCC_RC_SUCCESS;
	binfl_t bf_val;
	
	switch (attributeType) {
	case SA_NTF_VALUE_UINT8:
		p8 = ncs_enc_reserve_space(uba, 1);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_8bit(&p8, ntfAttr->uint8Val);
		ncs_enc_claim_space(uba, 1);
		break;
	case SA_NTF_VALUE_INT8:
		p8 = ncs_enc_reserve_space(uba, 1);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_8bit(&p8, ntfAttr->int8Val);
		ncs_enc_claim_space(uba, 1);
		break;
	case SA_NTF_VALUE_UINT16:
		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, ntfAttr->uint16Val);
		ncs_enc_claim_space(uba, 2);
		break;
	case SA_NTF_VALUE_INT16:
		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, ntfAttr->int16Val);
		ncs_enc_claim_space(uba, 2);
		break;
	case SA_NTF_VALUE_UINT32:
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, ntfAttr->uint32Val);
		ncs_enc_claim_space(uba, 4);
		break;
	case SA_NTF_VALUE_INT32:
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, ntfAttr->int32Val);
		ncs_enc_claim_space(uba, 4);
		break;
	case SA_NTF_VALUE_FLOAT:
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		bf_val.float_val = ntfAttr->floatVal;
		ncs_encode_32bit(&p8, bf_val.uint32_val);
		ncs_enc_claim_space(uba, 4);
		break;
	case SA_NTF_VALUE_UINT64:
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_64bit(&p8, ntfAttr->uint64Val);
		ncs_enc_claim_space(uba, 8);
		break;
	case SA_NTF_VALUE_INT64:
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_64bit(&p8, ntfAttr->int64Val);
		ncs_enc_claim_space(uba, 8);
		break;
	case SA_NTF_VALUE_DOUBLE:
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		bf_val.double_val = ntfAttr->doubleVal;
		ncs_encode_64bit(&p8, bf_val.uint64_val);
		ncs_enc_claim_space(uba, 8);
		break;
	case SA_NTF_VALUE_LDAP_NAME:
	case SA_NTF_VALUE_STRING:
	case SA_NTF_VALUE_IPADDRESS:
	case SA_NTF_VALUE_BINARY:
		TRACE_2("enc type: %d offset: %hu size: %hu",
			(int)attributeType, ntfAttr->ptrVal.dataOffset, ntfAttr->ptrVal.dataSize);
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, ntfAttr->ptrVal.dataOffset);
		ncs_encode_16bit(&p8, ntfAttr->ptrVal.dataSize);
		ncs_enc_claim_space(uba, 4);
		break;
	case SA_NTF_VALUE_ARRAY:
		TRACE_2("enc type SA_NTF_VALUE_ARRAY: %d offset: %hu nelem: %hu esize: %hu",
			(int)attributeType,
			ntfAttr->arrayVal.arrayOffset, ntfAttr->arrayVal.numElements, ntfAttr->arrayVal.elementSize);
		p8 = ncs_enc_reserve_space(uba, 6);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, ntfAttr->arrayVal.arrayOffset);
		ncs_encode_16bit(&p8, ntfAttr->arrayVal.numElements);
		ncs_encode_16bit(&p8, ntfAttr->arrayVal.elementSize);
		ncs_enc_claim_space(uba, 6);
		break;
	default:
		TRACE_2("attributeType %d not valid", (int)attributeType);
		return NCSCC_RC_INVALID_INPUT;
	}
	return rv;
}

static uint32_t encodeSaNtfAttributeChangeT(NCS_UBAID *uba, uint8_t *p8, SaNtfAttributeChangeT *ntfAttr)
{
	uint32_t rv = NCSCC_RC_SUCCESS;
		
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_16bit(&p8, ntfAttr->attributeId);
	ncs_encode_16bit(&p8, (uint16_t)ntfAttr->attributeType);
	ncs_encode_32bit(&p8, ntfAttr->oldAttributePresent);
	ncs_enc_claim_space(uba, 8);
	if (ntfAttr->oldAttributePresent) {
		rv = encodeSaNtfValueT(uba,
						 p8, ntfAttr->attributeType, &ntfAttr->oldAttributeValue);
	}
	if (NCSCC_RC_SUCCESS == rv)
		rv = encodeSaNtfValueT(uba, p8, ntfAttr->attributeType, &ntfAttr->newAttributeValue);
	return rv;
}

static uint32_t encodeSaNtfAttribute(NCS_UBAID *uba, uint8_t *p8, SaNtfAttributeT *ntfAttr)
{
	uint32_t rv = NCSCC_RC_SUCCESS;
		
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_16bit(&p8, ntfAttr->attributeId);
	ncs_encode_16bit(&p8, (uint16_t)ntfAttr->attributeType);
	ncs_enc_claim_space(uba, 4);
	rv = encodeSaNtfValueT(uba, p8, ntfAttr->attributeType, &ntfAttr->attributeValue);
	return rv;
}

static uint32_t decodeNtfValueT(NCS_UBAID *uba, SaNtfValueTypeT attributeType, SaNtfValueT *ntfAttr)
{
	uint8_t *p8;
	uint8_t local_data[8];
	binfl_t bf_val;

	switch (attributeType) {
	case SA_NTF_VALUE_UINT8:
		p8 = ncs_dec_flatten_space(uba, local_data, 1);
		ntfAttr->uint8Val = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(uba, 1);
		break;
	case SA_NTF_VALUE_INT8:
		p8 = ncs_dec_flatten_space(uba, local_data, 1);
		ntfAttr->int8Val = ncs_decode_8bit(&p8);
		ncs_dec_skip_space(uba, 1);
		break;
	case SA_NTF_VALUE_UINT16:
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		ntfAttr->uint16Val = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		break;
	case SA_NTF_VALUE_INT16:
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		ntfAttr->int16Val = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		break;
	case SA_NTF_VALUE_UINT32:
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		ntfAttr->uint32Val = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		break;
	case SA_NTF_VALUE_INT32:
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		ntfAttr->int32Val = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
		break;
	case SA_NTF_VALUE_FLOAT:
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		bf_val.uint32_val = ncs_decode_32bit(&p8);
		ntfAttr->floatVal = bf_val.float_val;
		ncs_dec_skip_space(uba, 4);
		break;
	case SA_NTF_VALUE_UINT64:
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		ntfAttr->uint64Val = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		break;
	case SA_NTF_VALUE_INT64:
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		ntfAttr->int64Val = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
		break;
	case SA_NTF_VALUE_DOUBLE:
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		bf_val.uint64_val = ncs_decode_64bit(&p8);
		ntfAttr->doubleVal = bf_val.double_val;
		ncs_dec_skip_space(uba, 8);
		break;
	case SA_NTF_VALUE_LDAP_NAME:
	case SA_NTF_VALUE_STRING:
	case SA_NTF_VALUE_IPADDRESS:
	case SA_NTF_VALUE_BINARY:
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		ntfAttr->ptrVal.dataOffset = ncs_decode_16bit(&p8);
		ntfAttr->ptrVal.dataSize = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 4);
		TRACE_2("dec type: %d offset: %hu size: %hu",
			(int)attributeType, ntfAttr->ptrVal.dataOffset, ntfAttr->ptrVal.dataSize);
		break;
	case SA_NTF_VALUE_ARRAY:
		p8 = ncs_dec_flatten_space(uba, local_data, 6);
		ntfAttr->arrayVal.arrayOffset = ncs_decode_16bit(&p8);
		ntfAttr->arrayVal.numElements = ncs_decode_16bit(&p8);
		ntfAttr->arrayVal.elementSize = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 6);
		TRACE_2("enc type: %d offset: %hu nelem: %hu esize: %hu",
			(int)attributeType,
			ntfAttr->arrayVal.arrayOffset, ntfAttr->arrayVal.numElements, ntfAttr->arrayVal.elementSize);
		break;
	default:
		TRACE_2("attributeType %d not valid", (int)attributeType);
	}
	return NCSCC_RC_SUCCESS;
}

static uint32_t decodeSaNtfAttributeChangeT(NCS_UBAID *uba, SaNtfAttributeChangeT *ntfAttr)
{
	uint8_t *p8, rv;
	uint8_t local_data[8];
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	ntfAttr->attributeId = ncs_decode_16bit(&p8);
	ntfAttr->attributeType = ncs_decode_16bit(&p8);
	ntfAttr->oldAttributePresent = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);
	if (ntfAttr->oldAttributePresent) {
		rv = decodeNtfValueT(uba, ntfAttr->attributeType, &ntfAttr->oldAttributeValue);
	}
	rv = decodeNtfValueT(uba, ntfAttr->attributeType, &ntfAttr->newAttributeValue);
	return rv;
}

static uint32_t decodeSaNtfAttribute(NCS_UBAID *uba, SaNtfAttributeT *ntfAttr)
{
	uint8_t *p8, rv;
	uint8_t local_data[4];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	ntfAttr->attributeId = ncs_decode_16bit(&p8);
	ntfAttr->attributeType = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 4);
	rv = decodeNtfValueT(uba, ntfAttr->attributeType, &ntfAttr->attributeValue);
	return rv;
}

static uint32_t encodeSaNameT(NCS_UBAID *uba, uint8_t *p8, SaNameT *name)
{
	uint32_t rv;
	
	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	if (name->length > SA_MAX_NAME_LENGTH) {
		LOG_ER("SaNameT length too long %hd", name->length);
		osafassert(0);
	}
	ncs_encode_16bit(&p8, name->length);
	ncs_enc_claim_space(uba, 2);
	rv = ncs_encode_n_octets_in_uba(uba, name->value, (uint32_t)name->length);
	return rv;
}

static uint32_t decodeSaNameT(NCS_UBAID *uba, uint8_t *p8, SaNameT *name)
{
	uint8_t local_data[2];
	uint32_t rv;
	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	name->length = ncs_decode_16bit(&p8);
	if (name->length > SA_MAX_NAME_LENGTH) {
		LOG_ER("SaNameT length too long: %hd", name->length);
		/* this should not happen */
		osafassert(0);
	}
	ncs_dec_skip_space(uba, 2);
	rv = ncs_decode_n_octets_from_uba(uba, name->value, (uint32_t)name->length);
	return rv;
}

static uint32_t ntfsv_enc_not_header(NCS_UBAID *uba, SaNtfNotificationHeaderT *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	int i;

	/* additional text */
	p8 = ncs_enc_reserve_space(uba, 10);
	if (!p8) {
		goto error_done;
	}
	ncs_encode_16bit(&p8, param->numCorrelatedNotifications);
	ncs_encode_16bit(&p8, param->lengthAdditionalText);
	ncs_encode_16bit(&p8, param->numAdditionalInfo);
	ncs_encode_32bit(&p8, *param->eventType);
	ncs_enc_claim_space(uba, 10);
	rv = NCSCC_RC_SUCCESS;

	rv = encodeSaNameT(uba, p8, param->notificationObject);
	if (rv == 0) {
		goto error_done;
	}

	rv = encodeSaNameT(uba, p8, param->notifyingObject);
	if (rv == 0) {
		goto error_done;
	}

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		goto error_done;
	}
	ncs_encode_32bit(&p8, param->notificationClassId->vendorId);
	ncs_encode_16bit(&p8, param->notificationClassId->majorId);
	ncs_encode_16bit(&p8, param->notificationClassId->minorId);
	ncs_enc_claim_space(uba, 8);

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		goto error_done;
	}
	ncs_encode_64bit(&p8, *param->eventTime);
	ncs_enc_claim_space(uba, 8);

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		goto error_done;
	}
	ncs_encode_64bit(&p8, *param->notificationId);
	ncs_enc_claim_space(uba, 8);
	TRACE_2("ENC: Not ptr %p: %llu", param->notificationId, *param->notificationId);

	ncs_encode_n_octets_in_uba(uba, (uint8_t *)param->additionalText, (uint32_t)param->lengthAdditionalText);

	for (i = 0; i < param->numCorrelatedNotifications; i++) {
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8)
			goto error_done;
		ncs_encode_64bit(&p8, param->correlatedNotifications[i]);
		ncs_enc_claim_space(uba, 8);
	}

	TRACE_2("enc : param->numAdditionalInfo %hu\n", param->numAdditionalInfo);
	for (i = 0; i < param->numAdditionalInfo; i++) {
		TRACE_2("enc additionalInfo Type: %d\n", param->additionalInfo[i].infoType);
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			goto error_done;
		ncs_encode_16bit(&p8, param->additionalInfo[i].infoId);
		ncs_encode_16bit(&p8, param->additionalInfo[i].infoType);
		ncs_enc_claim_space(uba, 4);
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->additionalInfo[i].infoType,
						 &param->additionalInfo[i].infoValue);
		if (NCSCC_RC_SUCCESS != rv)
		      break; 
	}
	return rv;
 error_done:
	TRACE("ncs_enc_reserve_space failed");
	return rv;
}

uint32_t ntfsv_enc_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(uba != NULL);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->notificationType);
	ncs_encode_32bit(&p8, param->client_id);
	ncs_enc_claim_space(uba, 8);

	switch (param->notificationType) {
		int i;
	case SA_NTF_TYPE_ALARM:
		TRACE("enc SA_NTF_TYPE_ALARM");
		ntfsv_enc_not_header(uba, &param->notification.alarm.notificationHeader);
		p8 = ncs_enc_reserve_space(uba, 6);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, param->notification.alarm.numSpecificProblems);
		ncs_encode_16bit(&p8, param->notification.alarm.numMonitoredAttributes);
		ncs_encode_16bit(&p8, param->notification.alarm.numProposedRepairActions);
		ncs_enc_claim_space(uba, 6);
		p8 = ncs_enc_reserve_space(uba, 10);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, *param->notification.alarm.probableCause);
		ncs_encode_32bit(&p8, *param->notification.alarm.perceivedSeverity);
		ncs_encode_16bit(&p8, *param->notification.alarm.trend);
		ncs_enc_claim_space(uba, 10);
		p8 = ncs_enc_reserve_space(uba, 12);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, param->notification.alarm.thresholdInformation->thresholdId);
		ncs_encode_16bit(&p8, param->notification.alarm.thresholdInformation->thresholdValueType);
		ncs_encode_64bit(&p8, param->notification.alarm.thresholdInformation->armTime);
		ncs_enc_claim_space(uba, 12);
		TRACE("thresholdInformation->thresholdValueType %hu",
		      param->notification.alarm.thresholdInformation->thresholdValueType);
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->notification.alarm.thresholdInformation->thresholdValueType,
						 &param->notification.alarm.thresholdInformation->thresholdValue);
		if (NCSCC_RC_SUCCESS != rv)
			goto error_done; 
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->notification.alarm.thresholdInformation->thresholdValueType,
						 &param->notification.alarm.thresholdInformation->thresholdHysteresis);
		if (NCSCC_RC_SUCCESS != rv)
		      goto error_done; 
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->notification.alarm.thresholdInformation->thresholdValueType,
						 &param->notification.alarm.thresholdInformation->observedValue);
		if (NCSCC_RC_SUCCESS != rv)
		      goto error_done; 

		TRACE("thresholdInformation->numSpecificProblems %hu", param->notification.alarm.numSpecificProblems);
		for (i = 0; i < param->notification.alarm.numSpecificProblems; i++) {
			p8 = ncs_enc_reserve_space(uba, 12);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_16bit(&p8, param->notification.alarm.specificProblems[i].problemId);
			ncs_encode_16bit(&p8, param->notification.alarm.specificProblems[i].problemClassId.majorId);
			ncs_encode_16bit(&p8, param->notification.alarm.specificProblems[i].problemClassId.minorId);
			ncs_encode_32bit(&p8, param->notification.alarm.specificProblems[i].problemClassId.vendorId);
			ncs_encode_16bit(&p8, param->notification.alarm.specificProblems[i].problemType);
			ncs_enc_claim_space(uba, 12);
			rv = encodeSaNtfValueT(uba,
							 p8,
							 param->notification.alarm.specificProblems[i].problemType,
							 &param->notification.alarm.specificProblems[i].problemValue);
			if (NCSCC_RC_SUCCESS != rv)
			      goto error_done; 
		}
		TRACE("thresholdInformation->numMonitoredAttributes %hu",
		      param->notification.alarm.numMonitoredAttributes);
		for (i = 0; i < param->notification.alarm.numMonitoredAttributes; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_16bit(&p8, param->notification.alarm.monitoredAttributes[i].attributeId);
			ncs_encode_16bit(&p8, param->notification.alarm.monitoredAttributes[i].attributeType);
			ncs_enc_claim_space(uba, 4);
			rv = encodeSaNtfValueT(uba,
							 p8,
							 param->notification.alarm.monitoredAttributes[i].attributeType,
							 &param->notification.alarm.monitoredAttributes[i].
							 attributeValue);
			if (NCSCC_RC_SUCCESS != rv)
			      goto error_done; 
		}
		TRACE("thresholdInformation->numProposedRepairActions %hu",
		      param->notification.alarm.numProposedRepairActions);
		for (i = 0; i < param->notification.alarm.numProposedRepairActions; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_16bit(&p8, param->notification.alarm.proposedRepairActions[i].actionId);
			ncs_encode_16bit(&p8, param->notification.alarm.proposedRepairActions[i].actionValueType);
			ncs_enc_claim_space(uba, 4);
			rv = encodeSaNtfValueT(uba,
							 p8,
							 param->notification.alarm.proposedRepairActions[i].
							 actionValueType,
							 &param->notification.alarm.proposedRepairActions[i].
							 actionValue);
			if (NCSCC_RC_SUCCESS != rv)
			      goto error_done; 
		}
		break;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		TRACE("enc SA_NTF_TYPE_OBJECT_CREATE_DELETE");
		ntfsv_enc_not_header(uba, &param->notification.objectCreateDelete.notificationHeader);
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, param->notification.objectCreateDelete.numAttributes);
		ncs_encode_16bit(&p8, *param->notification.objectCreateDelete.sourceIndicator);
		TRACE_2("sourceIndicator %d", (int)*param->notification.objectCreateDelete.sourceIndicator);
		ncs_enc_claim_space(uba, 4);
		ntfsv_print_object_attributes(param->notification.objectCreateDelete.objectAttributes,
					      param->notification.objectCreateDelete.numAttributes);

		for (i = 0; i < param->notification.objectCreateDelete.numAttributes; i++) {
			rv = encodeSaNtfAttribute(uba,
							   p8,
							   &param->notification.objectCreateDelete.objectAttributes[i]);
			if (rv != NCSCC_RC_SUCCESS) {
				goto error_done;
			}
		}
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		TRACE("enc SA_NTF_TYPE_ATTRIBUTE_CHANGE");
		ntfsv_enc_not_header(uba, &param->notification.attributeChange.notificationHeader);
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, param->notification.attributeChange.numAttributes);
		ncs_encode_16bit(&p8, *param->notification.attributeChange.sourceIndicator);
		ncs_enc_claim_space(uba, 4);
		for (i = 0; i < param->notification.attributeChange.numAttributes; i++) {
			rv =
			    encodeSaNtfAttributeChangeT(uba,
							p8,
							&param->notification.attributeChange.changedAttributes[i]);
			if (NCSCC_RC_SUCCESS != rv)
			      goto error_done; 
		}
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		TRACE("enc SA_NTF_TYPE_STATE_CHANGE");
		ntfsv_enc_not_header(uba, &param->notification.stateChange.notificationHeader);
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		TRACE("enc numStCh: %d", (int)param->notification.stateChange.numStateChanges);
		ncs_encode_16bit(&p8, param->notification.stateChange.numStateChanges);
		ncs_encode_16bit(&p8, *param->notification.stateChange.sourceIndicator);
		ncs_enc_claim_space(uba, 4);
		for (i = 0; i < param->notification.stateChange.numStateChanges; i++) {
			p8 = ncs_enc_reserve_space(uba, 6);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_16bit(&p8, param->notification.stateChange.changedStates[i].stateId);
			ncs_encode_32bit(&p8, param->notification.stateChange.changedStates[i].oldStatePresent);
			ncs_enc_claim_space(uba, 6);
			if (param->notification.stateChange.changedStates[i].oldStatePresent) {
				p8 = ncs_enc_reserve_space(uba, 2);
				if (!p8) {
					TRACE("ncs_enc_reserve_space failed");
					return NCSCC_RC_OUT_OF_MEM;
				}
				ncs_encode_16bit(&p8, param->notification.stateChange.changedStates[i].oldState);
				ncs_enc_claim_space(uba, 2);
			}
			p8 = ncs_enc_reserve_space(uba, 2);
			if (!p8) {
				TRACE("ncs_enc_reserve_space failed");
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_16bit(&p8, param->notification.stateChange.changedStates[i].newState);
			ncs_enc_claim_space(uba, 2);
		}
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		TRACE("enc SA_NTF_TYPE_SECURITY_ALARM");
		ntfsv_enc_not_header(uba, &param->notification.securityAlarm.notificationHeader);
		p8 = ncs_enc_reserve_space(uba, 10);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, *param->notification.securityAlarm.probableCause);
		ncs_encode_32bit(&p8, *param->notification.securityAlarm.severity);
		ncs_encode_16bit(&p8, param->notification.securityAlarm.securityAlarmDetector->valueType);
		ncs_enc_claim_space(uba, 10);
		TRACE_2("enc Security Alarm Detector Type: %d\n", param->notification.securityAlarm.securityAlarmDetector->valueType);
		rv = encodeSaNtfValueT(uba, p8, param->notification.securityAlarm.securityAlarmDetector->valueType, &param->notification.securityAlarm.securityAlarmDetector->value);
		if (rv != NCSCC_RC_SUCCESS) {
			goto error_done;
		}
		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}		
		ncs_encode_16bit(&p8, param->notification.securityAlarm.serviceUser->valueType);
		ncs_enc_claim_space(uba, 2);
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->notification.securityAlarm.serviceUser->valueType,
						 &param->notification.securityAlarm.serviceUser->value);
		if (NCSCC_RC_SUCCESS != rv)
		      goto error_done; 
		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) {
			TRACE("ncs_enc_reserve_space failed");
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_16bit(&p8, param->notification.securityAlarm.serviceProvider->valueType);
		ncs_enc_claim_space(uba, 2);
		rv = encodeSaNtfValueT(uba,
						 p8,
						 param->notification.securityAlarm.serviceProvider->valueType,
						 &param->notification.securityAlarm.serviceProvider->value);
		if (NCSCC_RC_SUCCESS != rv)
		      goto error_done; 

		break;

	default:
		TRACE("notificationType: %d not valid", (int)param->notificationType);
		rv=  NCSCC_RC_INVALID_INPUT;
		goto error_done;
	}
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->variable_data.size);
	ncs_enc_claim_space(uba, 4);
	if (param->variable_data.size)
		ncs_encode_n_octets_in_uba(uba, param->variable_data.p_base, param->variable_data.size);

 done:
	TRACE_LEAVE();
	return rv;

 error_done:
	TRACE_1("encoding error");
	goto done;
}

uint32_t ntfsv_enc_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	int i;
	
	TRACE_ENTER();
	osafassert(uba != NULL);
	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	TRACE_3("t:%#x, nd: %u",
		param->notificationType,
		param->numberDiscarded); 
	ncs_encode_32bit(&p8, param->notificationType);
	ncs_encode_32bit(&p8, param->numberDiscarded);
	ncs_enc_claim_space(uba, 8);
	for (i = 0; i < param->numberDiscarded; i++) {
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) {
			TRACE_1("encoding error");
			rv = NCSCC_RC_OUT_OF_MEM;			
			break;
		}
		ncs_encode_64bit(&p8, param->discardedNotificationIdentifiers[i]);
		ncs_enc_claim_space(uba, 8);
	}
	TRACE_LEAVE();  
	return rv;
}

static uint32_t ntfsv_dec_not_header(NCS_UBAID *uba, SaNtfNotificationHeaderT *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	uint8_t local_data[8];
	int i;
	/* SaNtfNotificationHeaderT size params */
	SaUint16T numCorrelatedNotifications;
	SaUint16T lengthAdditionalText;
	SaUint16T numAdditionalInfo;
	TRACE_ENTER();

/* SaNtfNotificationHeaderT size params */
	p8 = ncs_dec_flatten_space(uba, local_data, 6);
	numCorrelatedNotifications = ncs_decode_16bit(&p8);
	lengthAdditionalText = ncs_decode_16bit(&p8);
	numAdditionalInfo = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 6);

	ntfsv_alloc_ntf_header(param, numCorrelatedNotifications, lengthAdditionalText, numAdditionalInfo);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	*param->eventType = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	rv = decodeSaNameT(uba, p8, param->notificationObject);
	rv = decodeSaNameT(uba, p8, param->notifyingObject);

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->notificationClassId->vendorId = ncs_decode_32bit(&p8);
	param->notificationClassId->majorId = ncs_decode_16bit(&p8);
	param->notificationClassId->minorId = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 8);

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	*param->eventTime = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	*param->notificationId = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);

	if (param->lengthAdditionalText > 0) {
		/* dealloc in ntfs_evt.c: proc_send_not_msg */
		rv = ncs_decode_n_octets_from_uba(uba, (uint8_t *)param->additionalText, (uint32_t)param->lengthAdditionalText);
		param->additionalText[param->lengthAdditionalText - 1] = '\0';
	}

	for (i = 0; i < param->numCorrelatedNotifications; i++) {
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		param->correlatedNotifications[i] = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 8);
	}
	for (i = 0; i < param->numAdditionalInfo; i++) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		param->additionalInfo[i].infoId = ncs_decode_16bit(&p8);
		param->additionalInfo[i].infoType = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 4);
		rv = decodeNtfValueT(uba, param->additionalInfo[i].infoType, &param->additionalInfo[i].infoValue);
		TRACE_2("dec additionalInfo Type: %d\n", param->additionalInfo[i].infoType);
		TRACE_2("dec additionaInfo Value int32: %d\n", param->additionalInfo[i].infoValue.int32Val);
	}
	TRACE_LEAVE();
	return rv;
}

uint32_t ntfsv_dec_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	uint8_t local_data[16];

	TRACE_ENTER();

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->notificationType = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	switch (param->notificationType) {
		int i;

	case SA_NTF_TYPE_ALARM:
		TRACE("dec SA_NTF_TYPE_ALARM");
		rv = ntfsv_dec_not_header(uba, &param->notification.alarm.notificationHeader);
		p8 = ncs_dec_flatten_space(uba, local_data, 16);
		param->notification.alarm.numSpecificProblems = ncs_decode_16bit(&p8);
		param->notification.alarm.numMonitoredAttributes = ncs_decode_16bit(&p8);
		param->notification.alarm.numProposedRepairActions = ncs_decode_16bit(&p8);
		ntfsv_alloc_ntf_alarm(&param->notification.alarm,
				      param->notification.alarm.numSpecificProblems,
				      param->notification.alarm.numMonitoredAttributes,
				      param->notification.alarm.numProposedRepairActions);
		*param->notification.alarm.probableCause = ncs_decode_32bit(&p8);
		*param->notification.alarm.perceivedSeverity = ncs_decode_32bit(&p8);
		*param->notification.alarm.trend = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 16);
		p8 = ncs_dec_flatten_space(uba, local_data, 12);
		param->notification.alarm.thresholdInformation->thresholdId = ncs_decode_16bit(&p8);
		param->notification.alarm.thresholdInformation->thresholdValueType = ncs_decode_16bit(&p8);
		param->notification.alarm.thresholdInformation->armTime = ncs_decode_64bit(&p8);
		ncs_dec_skip_space(uba, 12);
		rv = decodeNtfValueT(uba,
					      param->notification.alarm.thresholdInformation->thresholdValueType,
					      &param->notification.alarm.thresholdInformation->thresholdValue);
		rv = decodeNtfValueT(uba,
					      param->notification.alarm.thresholdInformation->thresholdValueType,
					      &param->notification.alarm.thresholdInformation->thresholdHysteresis);
		rv = decodeNtfValueT(uba,
					      param->notification.alarm.thresholdInformation->thresholdValueType,
					      &param->notification.alarm.thresholdInformation->observedValue);
		for (i = 0; i < param->notification.alarm.numSpecificProblems; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 12);
			param->notification.alarm.specificProblems[i].problemId = ncs_decode_16bit(&p8);
			param->notification.alarm.specificProblems[i].problemClassId.majorId = ncs_decode_16bit(&p8);
			param->notification.alarm.specificProblems[i].problemClassId.minorId = ncs_decode_16bit(&p8);
			param->notification.alarm.specificProblems[i].problemClassId.vendorId = ncs_decode_32bit(&p8);
			param->notification.alarm.specificProblems[i].problemType = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(uba, 12);
			rv = decodeNtfValueT(uba,
						      param->notification.alarm.specificProblems[i].problemType,
						      &param->notification.alarm.specificProblems[i].problemValue);
		}
		for (i = 0; i < param->notification.alarm.numMonitoredAttributes; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			param->notification.alarm.monitoredAttributes[i].attributeId = ncs_decode_16bit(&p8);
			param->notification.alarm.monitoredAttributes[i].attributeType = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(uba, 4);
			rv = decodeNtfValueT(uba,
						      param->notification.alarm.monitoredAttributes[i].attributeType,
						      &param->notification.alarm.monitoredAttributes[i].attributeValue);
		}
		for (i = 0; i < param->notification.alarm.numProposedRepairActions; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			param->notification.alarm.proposedRepairActions[i].actionId = ncs_decode_16bit(&p8);
			param->notification.alarm.proposedRepairActions[i].actionValueType = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(uba, 4);
			rv = decodeNtfValueT(uba,
						      param->notification.alarm.proposedRepairActions[i].
						      actionValueType,
						      &param->notification.alarm.proposedRepairActions[i].actionValue);
		}
		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		TRACE("dec SA_NTF_TYPE_OBJECT_CREATE_DELETE");

		rv = ntfsv_dec_not_header(uba, &param->notification.objectCreateDelete.notificationHeader);
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		param->notification.objectCreateDelete.numAttributes = ncs_decode_16bit(&p8);
		ntfsv_alloc_ntf_obj_create_del(&param->notification.objectCreateDelete,
					       param->notification.objectCreateDelete.numAttributes);
		*param->notification.objectCreateDelete.sourceIndicator = ncs_decode_16bit(&p8);
		TRACE_2("sourceIndicator %d", (int)*param->notification.objectCreateDelete.sourceIndicator);
		ncs_dec_skip_space(uba, 4);

		if (param->notification.objectCreateDelete.numAttributes > 0) {
			for (i = 0; i < param->notification.objectCreateDelete.numAttributes; i++) {
				rv =
				    decodeSaNtfAttribute(uba,
							 &param->notification.objectCreateDelete.objectAttributes[i]);
			}
			ntfsv_print_object_attributes(param->notification.objectCreateDelete.objectAttributes,
						      param->notification.objectCreateDelete.numAttributes);
		}
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		TRACE("dec A_NTF_TYPE_ATTRIBUTE_CHANGE");
		rv = ntfsv_dec_not_header(uba, &param->notification.attributeChange.notificationHeader);
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		param->notification.attributeChange.numAttributes = ncs_decode_16bit(&p8);
		ntfsv_alloc_ntf_attr_change(&param->notification.attributeChange,
					    param->notification.objectCreateDelete.numAttributes);
		*param->notification.attributeChange.sourceIndicator = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 4);
		for (i = 0; i < param->notification.attributeChange.numAttributes; i++) {
			rv =
			    decodeSaNtfAttributeChangeT(uba,
							&param->notification.attributeChange.changedAttributes[i]);
		}
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		TRACE("dec SA_NTF_TYPE_STATE_CHANGE");
		rv = ntfsv_dec_not_header(uba, &param->notification.stateChange.notificationHeader);
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		param->notification.stateChange.numStateChanges = ncs_decode_16bit(&p8);
		ntfsv_alloc_ntf_state_change(&param->notification.stateChange,
					     param->notification.stateChange.numStateChanges);
		*param->notification.stateChange.sourceIndicator = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 4);
		TRACE("dec numStCh: %d", (int)param->notification.stateChange.numStateChanges);
		for (i = 0; i < param->notification.stateChange.numStateChanges; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 6);
			param->notification.stateChange.changedStates[i].stateId = ncs_decode_16bit(&p8);
			param->notification.stateChange.changedStates[i].oldStatePresent = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 6);
			if (param->notification.stateChange.changedStates[i].oldStatePresent) {
				p8 = ncs_dec_flatten_space(uba, local_data, 2);
				param->notification.stateChange.changedStates[i].oldState = ncs_decode_16bit(&p8);
				ncs_dec_skip_space(uba, 2);
			}
			p8 = ncs_dec_flatten_space(uba, local_data, 2);
			param->notification.stateChange.changedStates[i].newState = ncs_decode_16bit(&p8);
			ncs_dec_skip_space(uba, 2);
		}
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		TRACE("dec SA_NTF_TYPE_SECURITY_ALARM");
		rv = ntfsv_dec_not_header(uba, &param->notification.securityAlarm.notificationHeader);
		p8 = ncs_dec_flatten_space(uba, local_data, 10);
		ntfsv_alloc_ntf_security_alarm(&param->notification.securityAlarm);
		*param->notification.securityAlarm.probableCause = ncs_decode_32bit(&p8);
		*param->notification.securityAlarm.severity = ncs_decode_32bit(&p8);
		param->notification.securityAlarm.securityAlarmDetector->valueType = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 10);
		rv = decodeNtfValueT(uba,
					      param->notification.securityAlarm.securityAlarmDetector->valueType,
					      &param->notification.securityAlarm.securityAlarmDetector->value);
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		param->notification.securityAlarm.serviceUser->valueType = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		rv = decodeNtfValueT(uba,
					      param->notification.securityAlarm.serviceUser->valueType,
					      &param->notification.securityAlarm.serviceUser->value);
		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		param->notification.securityAlarm.serviceProvider->valueType = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);
		rv = decodeNtfValueT(uba,
					      param->notification.securityAlarm.serviceProvider->valueType,
					      &param->notification.securityAlarm.serviceProvider->value);

		break;
	default:
		TRACE("notificationType not valid");
		return NCSCC_RC_FAILURE;
		break;
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->variable_data.size = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	if (param->variable_data.size) {
		/* freed in ntfsv_dealloc_notification */
		param->variable_data.p_base = calloc(1, param->variable_data.size);
		TRACE("alloc v_data.p_base %p", param->variable_data.p_base);
		ncs_decode_n_octets_from_uba(uba, param->variable_data.p_base, param->variable_data.size);
	}
	TRACE_LEAVE();
	return rv;
}

uint32_t ntfsv_dec_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param)
{
	uint8_t *p8;
	uint8_t local_data[8];
	int i;
	
	TRACE_ENTER();
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	param->notificationType = ncs_decode_32bit(&p8);
	param->numberDiscarded = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);
	TRACE_3("t:%#x, nd: %u", param->notificationType, param->numberDiscarded);
	if (param->numberDiscarded) {
		param->discardedNotificationIdentifiers = calloc(1, sizeof(SaNtfIdentifierT) * param->numberDiscarded);
		if (!param->discardedNotificationIdentifiers) {
			TRACE_LEAVE();
			return NCSCC_RC_OUT_OF_MEM;
		}
		for (i = 0; i < param->numberDiscarded; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			param->discardedNotificationIdentifiers[i] = ncs_decode_64bit(&p8);
			ncs_dec_skip_space(uba, 8);
		} 
	} else {
		param->discardedNotificationIdentifiers = NULL;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t ntfsv_enc_filter_header(NCS_UBAID *uba, SaNtfNotificationFilterHeaderT *h)
{
	int i;
	uint32_t rv;
	uint8_t *p8;

	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8)
		goto error_done;
	ncs_encode_16bit(&p8, h->numEventTypes);
	ncs_encode_16bit(&p8, h->numNotificationClassIds);
	ncs_encode_16bit(&p8, h->numNotificationObjects);
	ncs_encode_16bit(&p8, h->numNotifyingObjects);
	ncs_enc_claim_space(uba, 8);
	
	for (i = 0; i < h->numEventTypes; i++) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			goto error_done;
		ncs_encode_32bit(&p8, h->eventTypes[i]);
		ncs_enc_claim_space(uba, 4);
	}
	for (i = 0; i < h->numNotificationClassIds; i++) {
		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8)
			goto error_done;
		ncs_encode_32bit(&p8, h->notificationClassIds[i].vendorId);
		ncs_encode_16bit(&p8, h->notificationClassIds[i].majorId);
		ncs_encode_16bit(&p8, h->notificationClassIds[i].minorId);
		ncs_enc_claim_space(uba, 8);
	}
	for (i = 0; i < h->numNotifyingObjects; i++) {	
		rv = encodeSaNameT(uba, p8, &h->notifyingObjects[i]);
		if (rv != NCSCC_RC_SUCCESS) {
			goto error_done;
		}
	}
	for (i = 0; i < h->numNotificationObjects; i++) {	
		rv = encodeSaNameT(uba, p8, &h->notificationObjects[i]);
		if (rv != NCSCC_RC_SUCCESS) {
			goto error_done;
		}
	}
	return 1; 
	
 error_done:
	TRACE_2("reserv space failed");
	return NCSCC_RC_OUT_OF_MEM;  
}  

static uint32_t ntfsv_enc_filter_msg(NCS_UBAID *uba, ntfsv_filter_ptrs_t *f_rec)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	int i;

	TRACE_3("alarm filter p: %p", f_rec->alarm_filter);
	if (f_rec->alarm_filter != NULL) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, SA_NTF_TYPE_ALARM);
		ncs_enc_claim_space(uba, 4);
		rv = ntfsv_enc_filter_header(uba, &f_rec->alarm_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			return NCSCC_RC_OUT_OF_MEM;

		p8 = ncs_enc_reserve_space(uba, 6);
		if (!p8) 
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_16bit(&p8, f_rec->alarm_filter->numProbableCauses);
		ncs_encode_16bit(&p8, f_rec->alarm_filter->numPerceivedSeverities);
		ncs_encode_16bit(&p8, f_rec->alarm_filter->numTrends);
		ncs_enc_claim_space(uba, 6);

		for (i = 0; i < f_rec->alarm_filter->numProbableCauses; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_32bit(&p8, f_rec->alarm_filter->probableCauses[i]);
			ncs_enc_claim_space(uba, 4);
		}
		for (i = 0; i < f_rec->alarm_filter->numPerceivedSeverities; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->alarm_filter->perceivedSeverities[i]);
			ncs_enc_claim_space(uba, 4);
		}
		for (i = 0; i < f_rec->alarm_filter->numTrends; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->alarm_filter->trends[i]);
			ncs_enc_claim_space(uba, 4);
		}  
	} else {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, 0);
		ncs_enc_claim_space(uba, 4);
	}

	TRACE_3("sec alarm filter p: %p", f_rec->sec_al_filter);
	if (f_rec->sec_al_filter != NULL) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, SA_NTF_TYPE_SECURITY_ALARM);
		ncs_enc_claim_space(uba, 4);
		rv = ntfsv_enc_filter_header(uba, &f_rec->sec_al_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			return NCSCC_RC_OUT_OF_MEM;
		p8 = ncs_enc_reserve_space(uba, 10);
		if (!p8) {
			return NCSCC_RC_OUT_OF_MEM;
		}		
		ncs_encode_16bit(&p8, f_rec->sec_al_filter->numProbableCauses);
		ncs_encode_16bit(&p8, f_rec->sec_al_filter->numSeverities);
		ncs_encode_16bit(&p8, f_rec->sec_al_filter->numSecurityAlarmDetectors);
		ncs_encode_16bit(&p8, f_rec->sec_al_filter->numServiceProviders);
		ncs_encode_16bit(&p8, f_rec->sec_al_filter->numServiceUsers);
		ncs_enc_claim_space(uba, 10);

		for (i = 0; i < f_rec->sec_al_filter->numProbableCauses; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_32bit(&p8, f_rec->sec_al_filter->probableCauses[i]);
			ncs_enc_claim_space(uba, 4);
		}
		for (i = 0; i < f_rec->sec_al_filter->numSeverities; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->sec_al_filter->severities[i]);
			ncs_enc_claim_space(uba, 4);
		}

		for (i = 0; i < f_rec->sec_al_filter->numSecurityAlarmDetectors; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->sec_al_filter->securityAlarmDetectors[i].valueType);
			ncs_enc_claim_space(uba, 4);
			rv = encodeSaNtfValueT(uba, p8, f_rec->sec_al_filter->securityAlarmDetectors[i].valueType, &f_rec->sec_al_filter->securityAlarmDetectors[i].value); 
			if (rv != NCSCC_RC_SUCCESS) return NCSCC_RC_OUT_OF_MEM;  
		}

		for (i = 0; i < f_rec->sec_al_filter->numServiceProviders; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->sec_al_filter->serviceProviders[i].valueType);
			ncs_enc_claim_space(uba, 4);
			rv = encodeSaNtfValueT(uba, p8, f_rec->sec_al_filter->serviceProviders[i].valueType, &f_rec->sec_al_filter->serviceProviders[i].value);			
			if (rv != NCSCC_RC_SUCCESS) return NCSCC_RC_OUT_OF_MEM;   
		}

		for (i = 0; i < f_rec->sec_al_filter->numServiceUsers; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8)
				return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_32bit(&p8, f_rec->sec_al_filter->serviceUsers[i].valueType);
			ncs_enc_claim_space(uba, 4);
			rv = encodeSaNtfValueT(uba, p8, f_rec->sec_al_filter->serviceUsers[i].valueType, &f_rec->sec_al_filter->serviceUsers[i].value);
			if (rv != NCSCC_RC_SUCCESS) return NCSCC_RC_OUT_OF_MEM;  
		}
	} else {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8) {
			return NCSCC_RC_OUT_OF_MEM;
		}
		ncs_encode_32bit(&p8, 0);
		ncs_enc_claim_space(uba, 4);
	}

	TRACE_3("att_ch_filter p: %p", f_rec->att_ch_filter);
	if (f_rec->att_ch_filter != NULL) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, SA_NTF_TYPE_ATTRIBUTE_CHANGE);
		ncs_enc_claim_space(uba, 4);
		rv = ntfsv_enc_filter_header(uba, &f_rec->att_ch_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			return NCSCC_RC_OUT_OF_MEM;

		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) 
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_16bit(&p8, f_rec->att_ch_filter->numSourceIndicators);
		ncs_enc_claim_space(uba, 2);

		for (i = 0; i < f_rec->att_ch_filter->numSourceIndicators; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_32bit(&p8, f_rec->att_ch_filter->sourceIndicators[i]);
			ncs_enc_claim_space(uba, 4);
		}
	} else {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, 0);
		ncs_enc_claim_space(uba, 4);
	}

	TRACE_3("obj_cr_del_filter p: %p", f_rec->obj_cr_del_filter);
	if (f_rec->obj_cr_del_filter != NULL) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, SA_NTF_TYPE_OBJECT_CREATE_DELETE);
		ncs_enc_claim_space(uba, 4);
		rv = ntfsv_enc_filter_header(uba, &f_rec->obj_cr_del_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			return NCSCC_RC_OUT_OF_MEM;

		p8 = ncs_enc_reserve_space(uba, 2);
		if (!p8) 
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_16bit(&p8, f_rec->obj_cr_del_filter->numSourceIndicators);
		ncs_enc_claim_space(uba, 2);

		for (i = 0; i < f_rec->obj_cr_del_filter->numSourceIndicators; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_32bit(&p8, f_rec->obj_cr_del_filter->sourceIndicators[i]);
			ncs_enc_claim_space(uba, 4);
		}
	} else {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, 0);
		ncs_enc_claim_space(uba, 4);
	}

	TRACE_3("sta_ch_filter p: %p", f_rec->sta_ch_filter);
	if (f_rec->sta_ch_filter != NULL) {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, SA_NTF_TYPE_STATE_CHANGE);
		ncs_enc_claim_space(uba, 4);
		rv = ntfsv_enc_filter_header(uba, &f_rec->sta_ch_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			return NCSCC_RC_OUT_OF_MEM;

		p8 = ncs_enc_reserve_space(uba, 8);
		if (!p8) 
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, f_rec->sta_ch_filter->numSourceIndicators);
		ncs_encode_32bit(&p8, f_rec->sta_ch_filter->numStateChanges);
		ncs_enc_claim_space(uba, 8);
		TRACE_3("numSourceIndicators: %d", (int)f_rec->sta_ch_filter->numSourceIndicators);
		for (i = 0; i < f_rec->sta_ch_filter->numSourceIndicators; i++) {
			p8 = ncs_enc_reserve_space(uba, 4);
			if (!p8) {
				return NCSCC_RC_OUT_OF_MEM;
			}
			ncs_encode_32bit(&p8, f_rec->sta_ch_filter->sourceIndicators[i]);
			ncs_enc_claim_space(uba, 4);
		}
		TRACE_3("numStateChanges: %d", (int)f_rec->sta_ch_filter->numStateChanges);
		for (i = 0; i < f_rec->sta_ch_filter->numStateChanges; i++) {
			p8 = ncs_enc_reserve_space(uba, 8);
			if (!p8)	return NCSCC_RC_OUT_OF_MEM;
			ncs_encode_16bit(&p8, f_rec->sta_ch_filter->changedStates[i].stateId);
			ncs_encode_16bit(&p8, f_rec->sta_ch_filter->changedStates[i].newState);
			ncs_encode_32bit(&p8, f_rec->sta_ch_filter->changedStates[i].oldStatePresent);
			ncs_enc_claim_space(uba, 8);
			TRACE_3("oldStatePresent: %d", f_rec->sta_ch_filter->changedStates[i].oldStatePresent);
			if (f_rec->sta_ch_filter->changedStates[i].oldStatePresent) {
				p8 = ncs_enc_reserve_space(uba, 2);
				if (!p8) return NCSCC_RC_OUT_OF_MEM;
				ncs_encode_16bit(&p8, f_rec->sta_ch_filter->changedStates[i].oldState);
				ncs_enc_claim_space(uba, 2);
			}
		}
	} else {
		p8 = ncs_enc_reserve_space(uba, 4);
		if (!p8)
			return NCSCC_RC_OUT_OF_MEM;
		ncs_encode_32bit(&p8, 0);
		ncs_enc_claim_space(uba, 4);
	}	
	return rv;
}

uint32_t ntfsv_enc_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();
	osafassert(uba != NULL);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->subscriptionId);
	ncs_enc_claim_space(uba, 8);
	
	rv = ntfsv_enc_filter_msg(uba, &param->f_rec);
	if (rv != NCSCC_RC_SUCCESS) {
		return rv;
	}
		
	rv = ntfsv_enc_discard_msg(uba, &param->d_info);

	TRACE_LEAVE();
	return rv;
}

uint32_t ntfsv_dec_filter_header(NCS_UBAID *uba, SaNtfNotificationFilterHeaderT *h)
{
	int i;
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_FAILURE;
	uint8_t local_data[8];
	
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	h->numEventTypes = ncs_decode_16bit(&p8);
	h->numNotificationClassIds = ncs_decode_16bit(&p8);
	h->numNotificationObjects = ncs_decode_16bit(&p8);
	h->numNotifyingObjects = ncs_decode_16bit(&p8);
	ncs_dec_skip_space(uba, 8);
	ntfsv_filter_header_alloc(h, h->numEventTypes, h->numNotificationObjects, h->numNotifyingObjects,
		h->numNotificationClassIds);
	for (i = 0; i < h->numEventTypes; i++) {
		p8 = ncs_dec_flatten_space(uba, local_data, 4);
		if (!p8)
			goto error_done;
		h->eventTypes[i] = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 4);
	}
	for (i = 0; i < h->numNotificationClassIds; i++) {
		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		if (!p8)
			goto error_done;
		h->notificationClassIds[i].vendorId = ncs_decode_32bit(&p8);
		h->notificationClassIds[i].majorId = ncs_decode_16bit(&p8);
		h->notificationClassIds[i].minorId = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 8);
	}
	for (i = 0; i < h->numNotifyingObjects; i++) {	
		rv = decodeSaNameT(uba, p8, &h->notifyingObjects[i]);
		if (rv != NCSCC_RC_SUCCESS) {
			goto error_done;
		}
	}
	for (i = 0; i < h->numNotificationObjects; i++) {	
		rv = decodeSaNameT(uba, p8, &h->notificationObjects[i]);
		if (rv != NCSCC_RC_SUCCESS) {
			goto error_done;
		}
	} 
	return NCSCC_RC_SUCCESS;
	
 error_done:
	 ntfsv_filter_header_free(h);
	TRACE_2("reserv space failed");
	return rv;   
}

static uint32_t ntfsv_dec_filter_msg(NCS_UBAID *uba, ntfsv_filter_ptrs_t *f_rec)
{
	int i;
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_OUT_OF_MEM;
	uint32_t filter_type;
	uint8_t local_data[10];
	
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	filter_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

/* memory allocated is freed in filter destructors */
	if (filter_type == SA_NTF_TYPE_ALARM) {
		TRACE_2("dec: SA_NTF_TYPE_ALARM");
		f_rec->alarm_filter = malloc(sizeof(*f_rec->alarm_filter));
		if (f_rec->alarm_filter == NULL) {
			goto error_free;
		}
		rv = ntfsv_dec_filter_header(uba, &f_rec->alarm_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		p8 = ncs_dec_flatten_space(uba, local_data, 6);
		f_rec->alarm_filter->numProbableCauses = ncs_decode_16bit(&p8);
		f_rec->alarm_filter->numPerceivedSeverities = ncs_decode_16bit(&p8);
		f_rec->alarm_filter->numTrends = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 6);

		rv = ntfsv_filter_alarm_alloc(f_rec->alarm_filter,
			f_rec->alarm_filter->numProbableCauses,
			f_rec->alarm_filter->numPerceivedSeverities,
			f_rec->alarm_filter->numTrends);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		for (i = 0; i < f_rec->alarm_filter->numProbableCauses; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->alarm_filter->probableCauses[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);

		}
		for (i = 0; i < f_rec->alarm_filter->numPerceivedSeverities; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->alarm_filter->perceivedSeverities[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}
		for (i = 0; i < f_rec->alarm_filter->numTrends; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->alarm_filter->trends[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}  				  
	} else {
		osafassert(!filter_type);
		f_rec->alarm_filter = NULL;
	}
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	filter_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	if (filter_type == SA_NTF_TYPE_SECURITY_ALARM) {
		TRACE_2("dec: SA_NTF_TYPE_SECURITY_ALARM");
		f_rec->sec_al_filter = malloc(sizeof(*f_rec->sec_al_filter));
		if (f_rec->sec_al_filter == NULL) {
			goto error_free;
		}
		rv = ntfsv_dec_filter_header(uba, &f_rec->sec_al_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;
		p8 = ncs_dec_flatten_space(uba, local_data, 10);
		f_rec->sec_al_filter->numProbableCauses = ncs_decode_16bit(&p8);
		f_rec->sec_al_filter->numSeverities = ncs_decode_16bit(&p8);
		f_rec->sec_al_filter->numSecurityAlarmDetectors = ncs_decode_16bit(&p8);
		f_rec->sec_al_filter->numServiceProviders = ncs_decode_16bit(&p8);
		f_rec->sec_al_filter->numServiceUsers = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 10);

		rv = ntfsv_filter_sec_alarm_alloc(f_rec->sec_al_filter,
			f_rec->sec_al_filter->numProbableCauses,
			f_rec->sec_al_filter->numSeverities,
			f_rec->sec_al_filter->numSecurityAlarmDetectors,
			f_rec->sec_al_filter->numServiceUsers,
			f_rec->sec_al_filter->numServiceProviders);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		for (i = 0; i < f_rec->sec_al_filter->numProbableCauses; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sec_al_filter->probableCauses[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);

		}
		for (i = 0; i < f_rec->sec_al_filter->numSeverities; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sec_al_filter->severities[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}
		for (i = 0; i < f_rec->sec_al_filter->numSecurityAlarmDetectors; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sec_al_filter->securityAlarmDetectors[i].valueType = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
			rv = decodeNtfValueT(uba, f_rec->sec_al_filter->securityAlarmDetectors[i].valueType, &f_rec->sec_al_filter->securityAlarmDetectors[i].value);
			if (rv != NCSCC_RC_SUCCESS)
				goto error_free;
		}  				  		
		for (i = 0; i < f_rec->sec_al_filter->numServiceProviders; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sec_al_filter->serviceProviders[i].valueType = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
			rv = decodeNtfValueT(uba, f_rec->sec_al_filter->serviceProviders[i].valueType, &f_rec->sec_al_filter->serviceProviders[i].value);
			if (rv != NCSCC_RC_SUCCESS)
				goto error_free;
		}  				  
		for (i = 0; i < f_rec->sec_al_filter->numServiceUsers; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sec_al_filter->serviceUsers[i].valueType = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
			rv = decodeNtfValueT(uba, f_rec->sec_al_filter->serviceUsers[i].valueType, &f_rec->sec_al_filter->serviceUsers[i].value);
			if (rv != NCSCC_RC_SUCCESS)
				goto error_free;
		}  				  
	} else {
		osafassert(!filter_type);
		f_rec->sec_al_filter = NULL;
	}

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	filter_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4) ;
	if (filter_type == SA_NTF_TYPE_ATTRIBUTE_CHANGE) {
		TRACE_2("dec: SA_NTF_TYPE_ATTRIBUTE_CHANGE");
		f_rec->att_ch_filter = malloc(sizeof(*f_rec->att_ch_filter));
		if (f_rec->att_ch_filter == NULL) {
			goto error_free;
		}
		rv = ntfsv_dec_filter_header(uba, &f_rec->att_ch_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		f_rec->att_ch_filter->numSourceIndicators = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);

		rv = ntfsv_filter_attr_change_alloc(f_rec->att_ch_filter,
			f_rec->att_ch_filter->numSourceIndicators);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		for (i = 0; i < f_rec->att_ch_filter->numSourceIndicators; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->att_ch_filter->sourceIndicators[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}
	} else {
		osafassert(!filter_type);
		f_rec->att_ch_filter = NULL;
	}

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	filter_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4) ;
	if (filter_type == SA_NTF_TYPE_OBJECT_CREATE_DELETE) {
		TRACE_2("dec: SA_NTF_TYPE_OBJECT_CREATE_DELETE");
		f_rec->obj_cr_del_filter = malloc(sizeof(*f_rec->obj_cr_del_filter));
		if (f_rec->obj_cr_del_filter == NULL) {
			goto error_free;
		}
		rv = ntfsv_dec_filter_header(uba, &f_rec->obj_cr_del_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		p8 = ncs_dec_flatten_space(uba, local_data, 2);
		f_rec->obj_cr_del_filter->numSourceIndicators = ncs_decode_16bit(&p8);
		ncs_dec_skip_space(uba, 2);

		rv = ntfsv_filter_obj_cr_del_alloc(f_rec->obj_cr_del_filter,
			f_rec->obj_cr_del_filter->numSourceIndicators);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		for (i = 0; i < f_rec->obj_cr_del_filter->numSourceIndicators; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->obj_cr_del_filter->sourceIndicators[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}
	} else {
		osafassert(!filter_type);
		f_rec->obj_cr_del_filter = NULL;
	}

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	filter_type = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4) ;
	if (filter_type == SA_NTF_TYPE_STATE_CHANGE) {
		TRACE_2("dec: SA_NTF_TYPE_STATE_CHANGE");
		f_rec->sta_ch_filter = malloc(sizeof(*f_rec->sta_ch_filter));
		if (f_rec->sta_ch_filter == NULL) {
			goto error_free;
		}
		rv = ntfsv_dec_filter_header(uba, &f_rec->sta_ch_filter->notificationFilterHeader);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;

		p8 = ncs_dec_flatten_space(uba, local_data, 8);
		f_rec->sta_ch_filter->numSourceIndicators = ncs_decode_32bit(&p8);
		f_rec->sta_ch_filter->numStateChanges = ncs_decode_32bit(&p8);
		ncs_dec_skip_space(uba, 8);

		rv = ntfsv_filter_state_ch_alloc(f_rec->sta_ch_filter,
			f_rec->sta_ch_filter->numSourceIndicators, 
			f_rec->sta_ch_filter->numStateChanges);
		if (rv != NCSCC_RC_SUCCESS)
			goto error_free;
		TRACE_2("numSourceIndicators: %d", f_rec->sta_ch_filter->numSourceIndicators);

		for (i = 0; i < f_rec->sta_ch_filter->numSourceIndicators; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 4);
			f_rec->sta_ch_filter->sourceIndicators[i] = ncs_decode_32bit(&p8);
			ncs_dec_skip_space(uba, 4);
		}
		TRACE_2("numStateChanges: %d", f_rec->sta_ch_filter->numStateChanges);
		for (i = 0; i < f_rec->sta_ch_filter->numStateChanges; i++) {
			p8 = ncs_dec_flatten_space(uba, local_data, 8);
			f_rec->sta_ch_filter->changedStates[i].stateId = ncs_decode_16bit(&p8);
			f_rec->sta_ch_filter->changedStates[i].newState = ncs_decode_16bit(&p8);
			f_rec->sta_ch_filter->changedStates[i].oldStatePresent = ncs_decode_32bit(&p8);
			TRACE_2("oldStatePresent: %d", f_rec->sta_ch_filter->changedStates[i].oldStatePresent);
			ncs_dec_skip_space(uba, 8);
			if (f_rec->sta_ch_filter->changedStates[i].oldStatePresent){
				p8 = ncs_dec_flatten_space(uba, local_data, 2);
				f_rec->sta_ch_filter->changedStates[i].oldState = ncs_decode_16bit(&p8);				
				ncs_dec_skip_space(uba, 2);
			}
		}
	} else {
		osafassert(!filter_type);
		f_rec->sta_ch_filter = NULL;
	}

done:
	return rv;

error_free:
   TRACE("No memory");
	free(f_rec->alarm_filter);
	free(f_rec->att_ch_filter);
	free(f_rec->obj_cr_del_filter);
	free(f_rec->sec_al_filter);
	free(f_rec->sta_ch_filter);
	goto done;
}

uint32_t ntfsv_dec_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_FAILURE;
	uint8_t local_data[10];

	param->f_rec.alarm_filter = NULL;
	param->f_rec.att_ch_filter = NULL;
	param->f_rec.obj_cr_del_filter = NULL;
	param->f_rec.sec_al_filter = NULL;
	param->f_rec.sta_ch_filter = NULL;
	
	TRACE_ENTER();
	/* client_id  */
	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->client_id = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	param->subscriptionId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);

	rv = ntfsv_dec_filter_msg(uba, &param->f_rec);
	if (rv != NCSCC_RC_SUCCESS)
		goto done;		
	
	rv = ntfsv_dec_discard_msg(uba, &param->d_info);
	if (rv != NCSCC_RC_SUCCESS)
		free(param->d_info.discardedNotificationIdentifiers);

done:	
	TRACE_8("NTFSV_SUBSCRIBE_REQ");
	TRACE_LEAVE();
	return rv;
}

uint32_t ntfsv_enc_64bit_msg(NCS_UBAID *uba, uint64_t param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(uba != NULL);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_64bit(&p8, param);
	ncs_enc_claim_space(uba, 8);

	TRACE_LEAVE();
	return rv;
    }

uint32_t ntfsv_dec_64bit_msg(NCS_UBAID *uba, uint64_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	uint8_t local_data[8];

	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	*param = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 8);
	TRACE_8("NTFSV_ENC_64bit");
	return rv;
}

uint32_t ntfsv_enc_32bit_msg(NCS_UBAID *uba, uint32_t param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	osafassert(uba != NULL);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 4);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param);
	ncs_enc_claim_space(uba, 4);

	TRACE_LEAVE();
	return rv;
}

uint32_t ntfsv_dec_32bit_msg(NCS_UBAID *uba, uint32_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	uint8_t local_data[4];

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	*param = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	TRACE_8("NTFSV_ENC_32bit");
	return rv;
}

uint32_t ntfsv_enc_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	osafassert(uba != NULL);

	/** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 8);
	if (!p8) {
		TRACE("ncs_enc_reserve_space failed");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_32bit(&p8, param->subscriptionId);
	ncs_enc_claim_space(uba, 8);

	return rv;
}

uint32_t ntfsv_dec_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param)
{
	uint8_t *p8;
	uint32_t rv = NCSCC_RC_SUCCESS;
	uint8_t local_data[8];

	/* client_id, lstr_id, lstr_open_id */
	p8 = ncs_dec_flatten_space(uba, local_data, 8);
	if (!p8) {TRACE("ncs_enc_reserve_space failed"); return NCSCC_RC_OUT_OF_MEM;}
	param->client_id = ncs_decode_32bit(&p8);
	param->subscriptionId = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 8);

	TRACE_8("NTFSV_UNSUBSCRIBE_REQ");
	return rv;
}

uint32_t ntfsv_enc_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_init_req_t *param = &msg->info.api_info.param.reader_init;

	TRACE_ENTER();
	osafassert(uba != NULL);

    /** encode the contents **/
	p8 = ncs_enc_reserve_space(uba, 22);
	if (!p8) {
		TRACE("NULL pointer");
		return NCSCC_RC_OUT_OF_MEM;
	}
	ncs_encode_32bit(&p8, param->client_id);
	ncs_encode_16bit(&p8, param->searchCriteria.searchMode);
	ncs_encode_64bit(&p8, param->searchCriteria.eventTime);
	ncs_encode_64bit(&p8, param->searchCriteria.notificationId);
	ncs_enc_claim_space(uba, 22);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

uint32_t ntfsv_dec_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint8_t *p8;
	ntfsv_reader_init_req_t *param = &msg->info.api_info.param.reader_init;
	uint8_t local_data[22];

	/* releaseCode, majorVersion, minorVersion */
	p8 = ncs_dec_flatten_space(uba, local_data, 22);
	param->client_id = ncs_decode_32bit(&p8);
	param->searchCriteria.searchMode = ncs_decode_16bit(&p8);
	param->searchCriteria.eventTime = ncs_decode_64bit(&p8);
	param->searchCriteria.notificationId = ncs_decode_64bit(&p8);
	ncs_dec_skip_space(uba, 22);

	TRACE_8("NTFSV_reader_initialize_REQ");
	return NCSCC_RC_SUCCESS;
}
 
uint32_t ntfsv_enc_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint32_t rv; 
	rv = ntfsv_enc_reader_initialize_msg(uba,msg);
	if (rv != NCSCC_RC_SUCCESS) 
		return rv; 
	return ntfsv_enc_filter_msg(uba, &msg->info.api_info.param.reader_init_2.f_rec); 
}

uint32_t ntfsv_dec_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg)
{
	uint32_t rv;   
	rv = ntfsv_dec_reader_initialize_msg(uba, msg);
	if (rv != NCSCC_RC_SUCCESS) 
		return rv; 
	return ntfsv_dec_filter_msg(uba, &msg->info.api_info.param.reader_init_2.f_rec);
}


