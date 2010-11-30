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

  AvA - AvND communication related definitions.
  
******************************************************************************
*/

#ifndef AVA_MDS_H
#define AVA_MDS_H

/* In Service upgrade support */
#define AVA_MDS_SUB_PART_VERSION   1

#define AVA_AVND_SUBPART_VER_MIN   1
#define AVA_AVND_SUBPART_VER_MAX   1

/*****************************************************************************
                 Macros to fill the MDS message structure
*****************************************************************************/
/* Macro to populate the 'AMF Initialize' message */
#define m_AVA_AMF_INIT_MSG_FILL(m, dst) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_INITIALIZE; \
   (m).info.api_info.dest = (dst); \
}

/* Macro to populate the 'AMF Finalize' message */
#define m_AVA_AMF_FINALIZE_MSG_FILL(m, dst, hd, cn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_FINALIZE; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.finalize.hdl = (hd); \
   (m).info.api_info.param.finalize.comp_name = (cn); \
}

/* Macro to populate the 'component register' message */
#define m_AVA_COMP_REG_MSG_FILL(m, dst, hd, cn, pcn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_COMP_REG; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.reg.hdl = (hd); \
   memcpy((m).info.api_info.param.reg.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.reg.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.reg.proxy_comp_name.value, \
                   (pcn).value, (pcn).length); \
   (m).info.api_info.param.reg.proxy_comp_name.length = (pcn).length; \
}

/* Macro to populate the 'component unregister' message */
#define m_AVA_COMP_UNREG_MSG_FILL(m, dst, hd, cn, pcn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_COMP_UNREG; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.unreg.hdl = (hd); \
   memcpy((m).info.api_info.param.unreg.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.unreg.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.unreg.proxy_comp_name.value, \
                   (pcn).value, (pcn).length); \
   (m).info.api_info.param.unreg.proxy_comp_name.length = (pcn).length; \
}

/* Macro to populate the 'healthcheck start' message */
#define m_AVA_HC_START_MSG_FILL(m, dst, hd, cn, pcn, hck, inv, rr) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_HC_START; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.hc_start.hdl = (hd); \
   memcpy((m).info.api_info.param.hc_start.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.hc_start.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.hc_start.proxy_comp_name.value, \
                   (pcn).value, (pcn).length); \
   (m).info.api_info.param.hc_start.proxy_comp_name.length = (pcn).length; \
   memcpy((m).info.api_info.param.hc_start.hc_key.key, \
                   (hck).key, (hck).keyLen); \
   (m).info.api_info.param.hc_start.hc_key.keyLen = (hck).keyLen; \
   (m).info.api_info.param.hc_start.inv_type = (inv); \
   (m).info.api_info.param.hc_start.rec_rcvr.saf_amf = (rr); \
}

/* Macro to populate the 'healthcheck stop' message */
#define m_AVA_HC_STOP_MSG_FILL(m, dst, hd, cn, pcn, hck) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_HC_STOP; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.hc_stop.hdl = (hd); \
   memcpy((m).info.api_info.param.hc_stop.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.hc_stop.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.hc_stop.proxy_comp_name.value, \
                   (pcn).value,  (pcn).length); \
   (m).info.api_info.param.hc_stop.proxy_comp_name.length = (pcn).length; \
   memcpy((m).info.api_info.param.hc_stop.hc_key.key, \
                   (hck).key, (hck).keyLen); \
   (m).info.api_info.param.hc_stop.hc_key.keyLen = (hck).keyLen; \
}

/* Macro to populate the 'healthcheck confirm' message */
#define m_AVA_HC_CONFIRM_MSG_FILL(m, dst, hd, cn, pcn, hck, res) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_HC_CONFIRM; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.hc_confirm.hdl = (hd); \
   memcpy((m).info.api_info.param.hc_confirm.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.hc_confirm.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.hc_confirm.proxy_comp_name.value, \
                   (pcn).value,  (pcn).length); \
   (m).info.api_info.param.hc_confirm.proxy_comp_name.length = (pcn).length; \
   memcpy((m).info.api_info.param.hc_confirm.hc_key.key, \
                   (hck).key, (hck).keyLen); \
   (m).info.api_info.param.hc_confirm.hc_key.keyLen = (hck).keyLen; \
   (m).info.api_info.param.hc_confirm.hc_res = (res); \
}

/* Macro to populate the 'Passive Monitoring start' message */
#define m_AVA_PM_START_MSG_FILL(m, dst, hd, cn, processId, depth, pmErr, rec) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_PM_START; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.hc_start.hdl = (hd); \
   memcpy((m).info.api_info.param.pm_start.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.pm_start.comp_name.length = (cn).length; \
   (m).info.api_info.param.pm_start.pid = (processId); \
   (m).info.api_info.param.pm_start.desc_tree_depth = (depth); \
   (m).info.api_info.param.pm_start.pm_err = (pmErr); \
   (m).info.api_info.param.pm_start.rec_rcvr.saf_amf = (rec); \
}

/* Macro to populate the 'Passive Monitoring stop' message */
#define m_AVA_PM_STOP_MSG_FILL(m, dst, hd, cn, stop, processId, pmErr) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_PM_STOP; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.pm_stop.hdl = (hd); \
   memcpy((m).info.api_info.param.pm_stop.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.pm_stop.comp_name.length = (cn).length; \
   (m).info.api_info.param.pm_stop.stop_qual = (stop); \
   (m).info.api_info.param.pm_stop.pid = (processId); \
   (m).info.api_info.param.pm_stop.pm_err = (pmErr); \
}

/* Macro to populate the 'ha state get' message */
#define m_AVA_HA_STATE_GET_MSG_FILL(m, dst, hd, cn, csi) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_HA_STATE_GET; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.ha_get.hdl = (hd); \
   memcpy((m).info.api_info.param.ha_get.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.ha_get.comp_name.length = (cn).length; \
   memcpy((m).info.api_info.param.ha_get.csi_name.value, \
                   (csi).value, (csi).length); \
   (m).info.api_info.param.ha_get.csi_name.length = (csi).length; \
}

/* Macro to populate the 'pg track start' message */
#define m_AVA_PG_START_MSG_FILL(m, dst, hd, csin, fl, syn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_PG_START; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.pg_start.hdl = (hd); \
   memcpy((m).info.api_info.param.pg_start.csi_name.value, \
                   (csin).value, (csin).length); \
   (m).info.api_info.param.pg_start.csi_name.length = (csin).length; \
   (m).info.api_info.param.pg_start.flags = (fl); \
   (m).info.api_info.param.pg_start.is_syn = (syn); \
}

/* Macro to populate the 'pg track stop' message */
#define m_AVA_PG_STOP_MSG_FILL(m, dst, hd, csin) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_PG_STOP; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.pg_stop.hdl = (hd); \
   memcpy((m).info.api_info.param.pg_stop.csi_name.value, \
                   (csin).value, (csin).length); \
   (m).info.api_info.param.pg_stop.csi_name.length = (csin).length; \
}

/* Macro to populate the 'error report' message */
#define m_AVA_ERR_REP_MSG_FILL(m, dst, hd, ec, et, rr) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_ERR_REP; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.err_rep.hdl = (hd); \
   memcpy((m).info.api_info.param.err_rep.err_comp.value, \
                   (ec).value, (ec).length); \
   (m).info.api_info.param.err_rep.err_comp.length = (ec).length; \
   (m).info.api_info.param.err_rep.detect_time = (et); \
   (m).info.api_info.param.err_rep.rec_rcvr.saf_amf = (rr); \
}

/* Macro to populate the 'error clear' message */
#define m_AVA_ERR_CLEAR_MSG_FILL(m, dst, hd, cn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_ERR_CLEAR; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.err_clear.hdl = (hd); \
   memcpy((m).info.api_info.param.err_clear.comp_name.value, \
                   (cn).value, (cn).length); \
   (m).info.api_info.param.err_clear.comp_name.length = (cn).length; \
}

/* Macro to populate the 'AMF response' message */
#define m_AVA_AMF_RESP_MSG_FILL(m, dst, hd, in, er, cn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_RESP; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.resp.hdl = (hd); \
   (m).info.api_info.param.resp.inv = (in); \
   (m).info.api_info.param.resp.err = (er); \
   (m).info.api_info.param.resp.comp_name = (cn); \
}

/* Macro to populate the 'AMF response' message */
#define m_AVA_CSI_QUIESCING_COMPL_MSG_FILL(m, dst, hd, in, er, cn) \
{ \
   (m).type = AVSV_AVA_API_MSG; \
   (m).info.api_info.type = AVSV_AMF_CSI_QUIESCING_COMPLETE; \
   (m).info.api_info.dest = (dst); \
   (m).info.api_info.param.csiq_compl.hdl = (hd); \
   (m).info.api_info.param.csiq_compl.inv = (in); \
   (m).info.api_info.param.csiq_compl.err = (er); \
   (m).info.api_info.param.csiq_compl.comp_name = (cn); \
}

/*** Extern function declarations ***/
struct ava_cb_tag;
EXTERN_C uns32 ava_mds_reg(struct ava_cb_tag *);

EXTERN_C uns32 ava_mds_unreg(struct ava_cb_tag *);

EXTERN_C uns32 ava_mds_cbk(NCSMDS_CALLBACK_INFO *);

EXTERN_C uns32 ava_mds_send(struct ava_cb_tag *, AVSV_NDA_AVA_MSG *, AVSV_NDA_AVA_MSG **);

#endif   /* !AVA_MDS_H */
