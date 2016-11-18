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

#ifndef CKPT_CKPTD_CPD_SBEDU_H_
#define CKPT_CKPTD_CPD_SBEDU_H_

#define FUNC_NAME(DS) cpsv_edp_##DS##_info

uint32_t FUNC_NAME(CPD_NODE_REF_INFO) ();
uint32_t FUNC_NAME(CPD_A2S_CKPT_CREATE) ();
uint32_t FUNC_NAME(CPD_A2S_CKPT_UNLINK) ();
uint32_t FUNC_NAME(CPSV_CKPT_RDSET) ();
uint32_t FUNC_NAME(CPSV_CKPT_DEST_INFO) ();
uint32_t FUNC_NAME(CPD_A2S_CKPT_USR_INFO) ();

uint32_t cpd_sb_proc_ckpt_create(CPD_CB *cb, CPD_MBCSV_MSG *msg);
uint32_t cpd_process_sb_msg(CPD_CB *cb, CPD_MBCSV_MSG *msg);

uint32_t cpd_a2s_ckpt_create(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node);
uint32_t cpd_mbcsv_init(CPD_CB *cb);
uint32_t cpd_mbcsv_open(CPD_CB *cb);
uint32_t cpd_mbcsv_selobj_get(CPD_CB *cb);
uint32_t cpd_mbcsv_async_update(CPD_CB *cb, CPD_MBCSV_MSG *msg);
uint32_t cpd_mbcsv_register(CPD_CB *cb);
uint32_t cpd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
uint32_t cpd_mbcsv_finalize(CPD_CB *cb);
uint32_t cpd_mbcsv_chgrole(CPD_CB *cb);

#endif  // CKPT_CKPTD_CPD_SBEDU_H_
