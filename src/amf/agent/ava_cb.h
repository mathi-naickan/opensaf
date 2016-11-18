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

  AvA CB related definitions.
  
******************************************************************************
*/

#ifndef AMF_AGENT_AVA_CB_H_
#define AMF_AGENT_AVA_CB_H_

#ifdef  __cplusplus
extern "C" {
#endif

/* AvA control block */
typedef struct ava_cb_tag {
	uint32_t cb_hdl;		/* CB hdl returned by hdl mngr */
	EDU_HDL edu_hdl;	/* EDU handle */
	uint8_t pool_id;		/* pool-id used by hdl mngr */
	NCS_LOCK lock;		/* CB lock */
	uint32_t pend_dis;		/* Number of pending dispaches */
	uint32_t pend_fin;		/* Number of pending agent destroy */

	SaNameT comp_name;	/* comp-name */
	uint32_t flag;		/* flags */
	SaVersionT version; /* API version used by client/application */

	/* mds parameters */
	MDS_HDL mds_hdl;	/* mds handle */
	MDS_DEST ava_dest;	/* AvA absolute address */
	MDS_DEST avnd_dest;	/* AvND absolute address */
	NCS_SEL_OBJ sel_obj;	/* sel obj for mds sync indication */

	/* AvA handle database */
	AVA_HDL_DB hdl_db;
} AVA_CB;

/* constants for PM_START param */
#define AVA_PM_START_ALL_DESCENDENTS -1

/* AvA flags */
#define AVA_FLAG_COMP_NAME  0x00000001
#define AVA_FLAG_AVND_UP    0x00000002
#define AVA_FLAG_FD_VALID   0x00000004

/* Macro to manage the ava flag */
#define m_AVA_FLAG_IS_AVND_UP(cb)     (cb->flag & AVA_FLAG_AVND_UP)
#define m_AVA_FLAG_IS_COMP_NAME(cb)   (cb->flag & AVA_FLAG_COMP_NAME)
#define m_AVA_FLAG_IS_FD_VALID(cb)    (cb->flag & AVA_FLAG_FD_VALID)
#define m_AVA_FLAG_SET(cb, bitmap)    (cb->flag |= bitmap)
#define m_AVA_FLAG_RESET(cb, bitmap)  (cb->flag &= ~bitmap)

/* Macro to validate the AMF version */
#define m_AVA_VER_IS_VALID(ver) \
              ( (ver->releaseCode == 'B') && \
                (ver->majorVersion <= 0x01) )

/* Macro to validate the dispatch flags */
#define m_AVA_DISPATCH_FLAG_IS_VALID(flag) \
               ( (SA_DISPATCH_ONE == flag) || \
                 (SA_DISPATCH_ALL == flag) || \
                 (SA_DISPATCH_BLOCKING == flag) )

/* Macro to validate the pg flags */
#define m_AVA_PG_FLAG_IS_VALID(flag) \
               ( \
                 !( ( !(flag & SA_TRACK_CURRENT) && \
                      !(flag & SA_TRACK_CHANGES) && \
                      !(flag & SA_TRACK_CHANGES_ONLY) ) || \
                    ( (flag & SA_TRACK_CHANGES) && \
                      (flag & SA_TRACK_CHANGES_ONLY) ) \
                  ) \
               )

/* Macro to validate the AMF error response */
#define m_AVA_AMF_RESP_ERR_CODE_IS_VALID(err) \
               ( (SA_AIS_OK == err) || \
                 (SA_AIS_ERR_FAILED_OPERATION == err) )

/*** Extern function declarations ***/

uint32_t ava_create(NCS_LIB_CREATE *);

void ava_destroy(NCS_LIB_DESTROY *);

uint32_t ava_avnd_msg_prc(AVA_CB *, AVSV_NDA_AVA_MSG *);

bool ava_B4_ver_used(AVA_CB *in_cb);

void ava_cpy_protection_group_ntf(SaAmfProtectionGroupNotificationT_4  *to_ntf,
				const SaAmfProtectionGroupNotificationT *from_ntf,
				SaUint32T items,
				SaAmfHAReadinessStateT ha_read_state);
bool ava_sanamet_is_valid(const SaNameT* pName);
void amf_copy_from_SaAmfCallbacksT_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk,
                                const SaAmfCallbacksT *cbk);
void amf_copy_from_SaAmfCallbacksT_4_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk,
               const SaAmfCallbacksT_4 *cbk);
void amf_copy_from_SaAmfCallbacksT_o4_to_OsafAmfCallbacksT(OsafAmfCallbacksT *osaf_cbk,
               const SaAmfCallbacksT_o4 *cbk);


#ifdef  __cplusplus
}
#endif
#endif  // AMF_AGENT_AVA_CB_H_
