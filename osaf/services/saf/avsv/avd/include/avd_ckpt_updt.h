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

  This module is the include file for Availability Directors checkpointing.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_CKPT_UPDT_H
#define AVD_CKPT_UPDT_H

/* Function Definations of avd_ckpt_updt.c */
uint32_t avd_ckpt_node(AVD_CL_CB *cb, AVD_AVND *avnd, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_app(AVD_CL_CB *cb, AVD_APP *app, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_sg(AVD_CL_CB *cb, AVD_SG *sg, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_su(AVD_CL_CB *cb, AVD_SU *su, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_si(AVD_CL_CB *cb, AVD_SI *si, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_su_oper_list(AVD_CL_CB *cb, AVD_SU *su_ptr, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_sg_admin_si(AVD_CL_CB *cb, NCS_UBAID *uba, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_siass(AVD_CL_CB *cb, AVSV_SU_SI_REL_CKPT_MSG *su_si_ckpt, NCS_MBCSV_CB_DEC *dec);
uint32_t avd_ckpt_si_trans(AVD_CL_CB *cb, AVSV_SI_TRANS_CKPT_MSG *si_trans, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_comp(AVD_CL_CB *cb, AVD_COMP *comp, NCS_MBCSV_ACT_TYPE action);
uint32_t avsv_ckpt_add_rmv_updt_sus_per_si_rank_data(AVD_CL_CB *cb,
							   AVD_SUS_PER_SI_RANK *su_si_rank, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_ckpt_compcstype(AVD_CL_CB *cb,
							AVD_COMPCS_TYPE *comp_cs_type, NCS_MBCSV_ACT_TYPE action);
uint32_t avd_data_clean_up(AVD_CL_CB *cb);

#endif
