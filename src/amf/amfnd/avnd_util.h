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

  DESCRIPTION:

  This file contains the prototypes for utility operations.
  
******************************************************************************
*/

#ifndef AMF_AMFND_AVND_UTIL_H_
#define AMF_AMFND_AVND_UTIL_H_

struct avnd_cb_tag;
struct avnd_comp_tag;
enum avnd_comp_clc_cmd_type;

extern const char *presence_state[];
extern const char *ha_state[];

void avnd_msg_content_free(struct avnd_cb_tag *, AVND_MSG *);

uint32_t avnd_msg_copy(struct avnd_cb_tag *, AVND_MSG *, AVND_MSG *);
extern void avnd_msgid_assert(uint32_t msg_id);
void avnd_comp_cleanup_launch(AVND_COMP *comp);

bool avnd_failed_state_file_exist(void);
void avnd_failed_state_file_create(void);
void avnd_failed_state_file_delete(void);
const char *avnd_failed_state_file_location(void);

void dnd_msg_free(AVSV_DND_MSG *msg);
void nda_ava_msg_free(AVSV_NDA_AVA_MSG *msg);
void nda_ava_msg_content_free(AVSV_NDA_AVA_MSG *msg);
void amf_csi_attr_list_copy(SaAmfCSIAttributeListT *dattr, const SaAmfCSIAttributeListT *sattr);
void amf_csi_attr_list_free(SaAmfCSIAttributeListT *attrs);
uint32_t amf_cbk_copy(AVSV_AMF_CBK_INFO **o_dcbk, const AVSV_AMF_CBK_INFO *scbk);
void amf_cbk_free(AVSV_AMF_CBK_INFO* cbk_info);
void nd2nd_avnd_msg_free(AVSV_ND2ND_AVND_MSG *msg);

void free_n2d_nd_csicomp_state_info(AVSV_DND_MSG *msg);
void free_n2d_nd_sisu_state_info(AVSV_DND_MSG *msg);
SaAisErrorT saImmOmInitialize_cond(SaImmHandleT *immHandle,
	const SaImmCallbacksT *immCallbacks, SaVersionT *version);
uint32_t avnd_cpy_SU_DN_from_DN(std::string&, const std::string&);
SaAisErrorT amf_saImmOmAccessorInitialize(SaImmHandleT& immHandle,
      SaImmAccessorHandleT& accessorHandle);
SaAisErrorT amf_saImmOmSearchInitialize_o2(SaImmHandleT& immHandle,
      const std::string& rootName,
      SaImmScopeT scope,
      SaImmSearchOptionsT searchOptions,
      const SaImmSearchParametersT_2 *
      searchParam,
      const SaImmAttrNameT *
      attributeNames,
      SaImmSearchHandleT& searchHandle);
SaAisErrorT amf_saImmOmAccessorGet_o2(SaImmHandleT& immHandle,
	SaImmAccessorHandleT& accessorHandle,
	const std::string& objectName,
	const SaImmAttrNameT * attributeNames,
	SaImmAttrValuesT_2 *** attributes);
void amfnd_free_csi_attr_list(AVSV_CSI_ATTRS *attrs);
void amfnd_copy_csi_attrs(AVSV_CSI_ATTRS *src_attrs, AVSV_CSI_ATTRS *dest_attrs);
#endif  // AMF_AMFND_AVND_UTIL_H_
