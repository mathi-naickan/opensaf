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

#ifndef NTF_NTFSV_ENC_DEC_H_
#define NTF_NTFSV_ENC_DEC_H_

#include "ntf/saf/saNtf.h"
#include "base/ncsgl_defs.h"
#include "base/ncs_lib.h"
#include "mds/mds_papi.h"
#include "base/ncs_mda_pvt.h"
#include "mbc/mbcsv_papi.h"
#include "base/ncs_edu_pub.h"
#include "base/ncs_util.h"
#include "base/logtrace.h"
#include "ntf/ntfsv_msg.h"

#ifdef  __cplusplus
extern "C" {
#endif
uint32_t ntfsv_enc_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param);
uint32_t ntfsv_enc_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param);
uint32_t ntfsv_dec_not_msg(NCS_UBAID *uba, ntfsv_send_not_req_t *param);
uint32_t ntfsv_dec_discard_msg(NCS_UBAID *uba, ntfsv_discarded_info_t *param);
uint32_t ntfsv_enc_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param);
uint32_t ntfsv_dec_subscribe_msg(NCS_UBAID *uba, ntfsv_subscribe_req_t *param);
uint32_t ntfsv_enc_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param);
uint32_t ntfsv_dec_unsubscribe_msg(NCS_UBAID *uba, ntfsv_unsubscribe_req_t *param);
uint32_t ntfsv_enc_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg);
uint32_t ntfsv_dec_reader_initialize_msg(NCS_UBAID *uba, ntfsv_msg_t *msg);
uint32_t ntfsv_enc_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg);
uint32_t ntfsv_dec_reader_initialize_msg_2(NCS_UBAID *uba, ntfsv_msg_t *msg);
uint32_t ntfsv_enc_64bit_msg(NCS_UBAID *uba, uint64_t param);
uint32_t ntfsv_dec_64bit_msg(NCS_UBAID *uba, uint64_t *param);
uint32_t ntfsv_enc_32bit_msg(NCS_UBAID *uba, uint32_t param);
uint32_t ntfsv_dec_32bit_msg(NCS_UBAID *uba, uint32_t *param);

void ntfsv_print_object_attributes(SaNtfAttributeT *objectAttributes, SaUint16T numAttributes);
#ifdef  __cplusplus
}
#endif

#endif  // NTF_NTFSV_ENC_DEC_H_
