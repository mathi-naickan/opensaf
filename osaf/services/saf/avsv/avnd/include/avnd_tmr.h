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
  
  AvND Timer related definitions.
    
******************************************************************************
*/

#ifndef AVND_TMR_H
#define AVND_TMR_H

#include "ncssysf_tmr.h"

struct avnd_cb_tag;

/* timer type enums */
typedef enum avnd_tmr_type {
	AVND_TMR_HC = 1,	/* health check timer */
	AVND_TMR_CBK_RESP,	/* callback response timer */
	AVND_TMR_MSG_RSP,	/* message response timer */
	AVND_TMR_CLC_COMP_REG,	/* CLC comp register timer */
	AVND_TMR_SU_ERR_ESC,	/* su error escalation timer */
	AVND_TMR_NODE_ERR_ESC,	/* node error escalation timer */
	AVND_TMR_CLC_PXIED_COMP_INST,	/* proxied inst timer */
	AVND_TMR_CLC_PXIED_COMP_REG,	/* proxied orphan timer */
	AVND_TMR_HB_DURATION,
	AVND_TMR_QSCING_CMPL_RESP,	/* Qscing complete timer */
	AVND_TMR_MAX
} AVND_TMR_TYPE;

/* AvND Timer definition */
typedef struct avnd_tmr {
	tmr_t tmr_id;
	AVND_TMR_TYPE type;	/* timer type */
	uns32 opq_hdl;		/* hdl to retrive the timer context */
	NCS_BOOL is_active;
} AVND_TMR;

/* Macro to determine if AvND timer is active */
#define m_AVND_TMR_IS_ACTIVE(tmr)  ((tmr).is_active)

/* Macro to start the hc timer */
#define m_AVND_TMR_COMP_HC_START(cb, rec, rc) \
            (rc) = avnd_start_tmr ((cb), &(rec).tmr, \
                                   AVND_TMR_HC, \
                                   (rec).period, \
                                   (rec).opq_hdl);

/* Macro to stop the hc timer */
#define m_AVND_TMR_COMP_HC_STOP(cb, rec) \
            avnd_stop_tmr ((cb), &(rec).tmr);

/* Macro to start the callback response timer */
#define m_AVND_TMR_COMP_CBK_RESP_START(cb, cbn, per, rc) \
            (rc) = avnd_start_tmr((cb), &(cbn).resp_tmr, \
                                  AVND_TMR_CBK_RESP, \
                                  per, (cbn).opq_hdl);

/* Macro to stop the callback response timer */
#define m_AVND_TMR_COMP_CBK_RESP_STOP(cb, cbn) \
            avnd_stop_tmr((cb), &(cbn).resp_tmr);


/* Macro to start the qscing complete timer */
#define m_AVND_TMR_COMP_QSCING_CMPL_START(cb, comp) \
            (rc) = avnd_start_tmr((cb), &(comp->qscing_tmr), \
                                  AVND_TMR_QSCING_CMPL_RESP, \
                                  comp->quies_complete_cbk_timeout, comp->qscing_tmr.opq_hdl);

/* Macro to stop the qscing complete timer */
#define m_AVND_TMR_COMP_QSCING_CMPL_STOP(cb, comp) \
            avnd_stop_tmr((cb), &(comp->qscing_tmr));


/* Macro to start the comp-reg timer */
#define m_AVND_TMR_COMP_REG_START(cb, comp, rc) \
{ \
   SaTimeT start = 0, curr = 0 , per = 0; \
   start = (comp).clc_info.inst_cmd_ts; \
   m_GET_TIME_STAMP(curr); \
   per = (comp).clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout \
         - (curr - start); \
   (rc) = avnd_start_tmr ((cb), &(comp).clc_info.clc_reg_tmr, \
                          AVND_TMR_CLC_COMP_REG, \
                          per, \
                          (comp).comp_hdl); \
};

/* Macro to stop the comp-reg timer */
#define m_AVND_TMR_COMP_REG_STOP(cb, comp) \
            avnd_stop_tmr ((cb), &(comp).clc_info.clc_reg_tmr);

/* Macro to start the msg response timer */
#define m_AVND_TMR_MSG_RESP_START(cb, rec, rc) \
            (rc) = avnd_start_tmr((cb), &(rec).resp_tmr, \
                                  AVND_TMR_MSG_RSP, \
                                  (cb)->msg_resp_intv, (rec).opq_hdl);

/* Macro to stop the msg response timer */
#define m_AVND_TMR_MSG_RESP_STOP(cb, rec) \
            avnd_stop_tmr((cb), &(rec).resp_tmr);

/* Macro to start the component-error-escalation timer */
#define m_AVND_TMR_COMP_ERR_ESC_START(cb, su, rc)\
            (rc) = avnd_start_tmr ((cb), &(su)->su_err_esc_tmr,\
                                   AVND_TMR_SU_ERR_ESC, \
                                   (su)->comp_restart_prob, (su)->su_hdl);

/* Macro to stop the component-error-escalation timer */
#define m_AVND_TMR_COMP_ERR_ESC_STOP(cb, su)\
            avnd_stop_tmr ((cb), &(su)->su_err_esc_tmr);

/* Macro to start the su-error-escalation timer */
#define m_AVND_TMR_SU_ERR_ESC_START(cb, su, rc)\
            (rc) = avnd_start_tmr ((cb), &(su)->su_err_esc_tmr,\
                                   AVND_TMR_SU_ERR_ESC, \
                                   (su)->su_restart_prob, (su)->su_hdl);

/* Marcro to stop the su-error-escalation timer */
#define m_AVND_TMR_SU_ERR_ESC_STOP(cb, su)\
            avnd_stop_tmr ((cb), &(su)->su_err_esc_tmr);

/* Macro to start the node-error-escalation timer */
#define m_AVND_TMR_NODE_ERR_ESC_START(cb, rc)\
            (rc) = avnd_start_tmr ((cb), &(cb)->node_err_esc_tmr, \
                                     AVND_TMR_NODE_ERR_ESC,\
                                    (cb)->su_failover_prob, 0);

/* Macro to stop the node-error-escalation timer */
#define m_AVND_TMR_NODE_ERR_ESC_STOP(cb)\
            avnd_stop_tmr((cb), &(cb)->node_err_esc_tmr);

/* Macro to start the proxied comp-inst timer */
#define m_AVND_TMR_PXIED_COMP_INST_START(cb, comp, rc) \
{ \
   SaTimeT per = 0; \
   per = (comp).clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout; \
   (rc) = avnd_start_tmr ((cb), &(comp).clc_info.clc_reg_tmr, \
                          AVND_TMR_CLC_PXIED_COMP_INST, \
                          per, \
                          (comp).comp_hdl); \
};

/* Macro to stop the proxied comp-inst timer */
#define m_AVND_TMR_PXIED_COMP_INST_STOP(cb, comp) \
            avnd_stop_tmr ((cb), &(comp).clc_info.clc_reg_tmr);

/* Macro to start the proxied comp-reg timer */
#define m_AVND_TMR_PXIED_COMP_REG_START(cb, comp, rc) \
{ \
   SaTimeT per = 0; \
   per = (comp).clc_info.cmds[AVND_COMP_CLC_CMD_TYPE_INSTANTIATE - 1].timeout; \
   (rc) = avnd_start_tmr ((cb), &(comp).orph_tmr, \
                          AVND_TMR_CLC_PXIED_COMP_REG, \
                          per, \
                          (comp).comp_hdl); \
};

/* Macro to stop the proxied comp-reg timer */
#define m_AVND_TMR_PXIED_COMP_REG_STOP(cb, comp) \
            avnd_stop_tmr ((cb), &(comp).orph_tmr);

/*** Extern function declarations ***/

EXTERN_C void avnd_tmr_exp(void *);

EXTERN_C uns32 avnd_start_tmr(struct avnd_cb_tag *, AVND_TMR *, AVND_TMR_TYPE, SaTimeT, uns32);

EXTERN_C void avnd_stop_tmr(struct avnd_cb_tag *, AVND_TMR *);

#endif   /* !AVND_TMR_H */
