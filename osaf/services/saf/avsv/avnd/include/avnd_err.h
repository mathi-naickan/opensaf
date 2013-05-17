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
  
  AvND Error related definitions.
    
******************************************************************************
*/

#ifndef AVND_ERR_H
#define AVND_ERR_H

/***************************************************************************
 **********  S T R U C T U R E / E N U M  D E F I N I T I O N S  ***********
 ***************************************************************************/

/* 
 * Error Escalation Level Enum Definitions.
 * AVND_ERR_ESC_LEVEL_0 indicates that either the error escalation algo 
 * hasn't started (if comp-restart-cnt is 0) OR 
 * comp-restart probation is active.
 * Rest as given in the B-spec.
 */
typedef enum avnd_err_esc_level {
	AVND_ERR_ESC_LEVEL_0 = 1,
	AVND_ERR_ESC_LEVEL_1,
	AVND_ERR_ESC_LEVEL_2,
	AVND_ERR_ESC_LEVEL_3,
	AVND_ERR_ESC_LEVEL_MAX
} AVND_ERR_ESC_LEVEL;

/* component error source enums */
typedef enum avnd_err_src {
	AVND_ERR_SRC_REP = 1,	/* comp reports an error */
	AVND_ERR_SRC_HC,	/* healthcheck failure */
	AVND_ERR_SRC_PM,	/* passive monitoring failure */
	AVND_ERR_SRC_AM,	/* active monitoring failure (AM_START/STOP 
				   execution failed) */
	AVND_ERR_SRC_CMD_FAILED,	/* AMF command fails (INSTANTIATE cmd fails for NPI
					   comp as a result of CSI assignment) */
	AVND_ERR_SRC_CBK_HC_TIMEOUT,	/* AMF health check callback times out */
	AVND_ERR_SRC_CBK_HC_FAILED,	/* AMF callback failed */
	AVND_ERR_SRC_AVA_DN,	/* AvA down */
	AVND_ERR_SRC_PXIED_REG_TIMEOUT,	/* orphaned state timed out */
	AVND_ERR_SRC_CBK_CSI_SET_TIMEOUT,	/* AMF csi set callback times out */
	AVND_ERR_SRC_CBK_CSI_REM_TIMEOUT,	/* AMF csi rem callback times out */
	AVND_ERR_SRC_CBK_CSI_SET_FAILED,	/* AMF callback failed */
	AVND_ERR_SRC_CBK_CSI_REM_FAILED,	/* AMF callback failed */
	/* Add other sources of error detection */
	AVND_ERR_SRC_QSCING_COMPL_TIMEOUT,	/* AMF qscing complete times out */

	AVND_ERR_SRC_MAX
} AVND_ERR_SRC;

/* component error information */
typedef struct avnd_cerr_info_tag {
	AVND_ERR_SRC src;	/* error source */
	SaAmfRecommendedRecoveryT def_rec;	/* default comp recovery */
	SaTimeT detect_time;	/* error detection time */
	uint32_t restart_cnt;	/* restart counter */
} AVND_CERR_INFO;

/* wrapper structure used to carry error info across routines */
typedef struct avnd_err_tag {
	AVND_ERR_SRC src;	/* err-src */
	union {
		uint32_t raw;
		AVSV_ERR_RCVR avsv_ext;
		SaAmfRecommendedRecoveryT saf_amf;
	} rec_rcvr;
} AVND_ERR_INFO;

/***************************************************************************
 ******************  M A C R O   D E F I N I T I O N S  ********************
 ***************************************************************************/
#define m_AVND_ERR_ESC_LVL_GET(su, esc_level) \
{ \
   if((su)->comp_restart_max != 0) \
      (esc_level) = AVND_ERR_ESC_LEVEL_0; \
   else if ((su)->su_restart_max != 0 && !m_AVND_SU_IS_SU_RESTART_DIS(su)) \
      (esc_level) = AVND_ERR_ESC_LEVEL_1; \
   else if (((cb)->su_failover_max != 0) || (true == su->su_is_external)) \
      (esc_level) = AVND_ERR_ESC_LEVEL_2; \
   else \
      (esc_level) = AVND_ERR_ESC_LEVEL_3; \
}

/***************************************************************************
 ******  E X T E R N A L   F U N C T I O N   D E C L A R A T I O N S  ******
 ***************************************************************************/

struct avnd_cb_tag;
struct avnd_comp_tag;
struct avnd_su_tag;

extern uint32_t avnd_err_process(struct avnd_cb_tag *, struct avnd_comp_tag *, AVND_ERR_INFO *);
extern uint32_t avnd_err_su_repair(struct avnd_cb_tag *, struct avnd_su_tag *);

#endif   /* !AVND_ERR_H */
