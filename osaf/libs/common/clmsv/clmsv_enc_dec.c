/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s):  Emerson Network Power
 */

#include <ncsencdec_pub.h>
#include "clmsv_enc_dec.h"
#include "clmsv_msg.h"
#include "osaf_extended_name.h"

uint32_t clmsv_decodeSaNameT(NCS_UBAID *uba, SaNameT *name)
{
	uint8_t local_data[2];
	uint8_t *p8 = NULL;
	uint32_t total_bytes = 0;
	uint16_t length;
	char valueBuffer[256];
	char *value = valueBuffer;

	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	length = ncs_decode_16bit(&p8);
	if (!osaf_is_extended_names_enabled() && length >= SA_MAX_NAME_LENGTH) {
		LOG_ER("SaNameT length too long: %hd", length);
		/* this should not happen */
		osafassert(0);
	}
	if(length >= SA_MAX_NAME_LENGTH) {
		value = (char *)malloc(length + 1);
	}
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;
	ncs_decode_n_octets_from_uba(uba, (uint8_t *)value, (uint32_t)length);
	value[length] = 0;
	memset(name, 0, sizeof(SaNameT));
	osaf_extended_name_lend(value, name);
	total_bytes += length;
	return total_bytes;
}

uint32_t clmsv_decodeNodeAddressT(NCS_UBAID *uba, SaClmNodeAddressT *nodeAddress)
{
	uint8_t local_data[5];
	uint8_t *p8 = NULL;
	uint32_t total_bytes = 0;

	p8 = ncs_dec_flatten_space(uba, local_data, 4);
	nodeAddress->family = ncs_decode_32bit(&p8);
	ncs_dec_skip_space(uba, 4);
	total_bytes += 4;

	p8 = ncs_dec_flatten_space(uba, local_data, 2);
	nodeAddress->length = ncs_decode_16bit(&p8);
	if (nodeAddress->length > SA_MAX_NAME_LENGTH) {
		LOG_ER("nodeAddress->length length too long: %hd", nodeAddress->length);
		/* this should not happen */
		osafassert(0);
	}
	ncs_dec_skip_space(uba, 2);
	total_bytes += 2;
	ncs_decode_n_octets_from_uba(uba, nodeAddress->value, (uint32_t)nodeAddress->length);
	total_bytes += nodeAddress->length;
	return total_bytes;
}

uint32_t clmsv_encodeSaNameT(NCS_UBAID *uba, SaNameT *name)
{
	TRACE_ENTER();
	uint8_t *p8 = NULL;
	uint32_t total_bytes = 0;
	size_t length;

	p8 = ncs_enc_reserve_space(uba, 2);
	if (!p8) {
		TRACE("p8 NULL!!!");
		return 0;
	}
	if (!osaf_is_extended_names_enabled() && name->length >= SA_MAX_NAME_LENGTH) {
		LOG_ER("SaNameT length too long %hd", name->length);
		osafassert(0);
	}
	length = osaf_extended_name_length(name);
	ncs_encode_16bit(&p8, length);
	ncs_enc_claim_space(uba, 2);
	total_bytes += 2;
	ncs_encode_n_octets_in_uba(uba, (uint8_t *)osaf_extended_name_borrow(name), length);
	total_bytes += (uint32_t)length;
	TRACE_LEAVE();
	return total_bytes;
}
