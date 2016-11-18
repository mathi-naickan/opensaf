/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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
 * Author(s): Genband
 * 
 */
#ifndef CKPT_CKPTND_CPND_SEC_H_
#define CKPT_CKPTND_CPND_SEC_H_

#ifdef __cplusplus
extern "C" {
#endif

void cpnd_ckpt_sec_map_init(CPND_CKPT_REPLICA_INFO *);

void cpnd_ckpt_sec_map_destroy(CPND_CKPT_REPLICA_INFO *);

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_first(const CPND_CKPT_REPLICA_INFO *);

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_next(const CPND_CKPT_REPLICA_INFO *,
                       const CPND_CKPT_SECTION_INFO *);

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get(const CPND_CKPT_NODE *, const SaCkptSectionIdT *);

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_create(const CPND_CKPT_NODE *, const SaCkptSectionIdT *);

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_del(CPND_CKPT_NODE *, SaCkptSectionIdT *);

CPND_CKPT_SECTION_INFO *
cpnd_get_sect_with_id(const CPND_CKPT_NODE *, uint32_t lcl_sec_id);

uint32_t cpnd_ckpt_sec_find(const CPND_CKPT_NODE *, const SaCkptSectionIdT *);

uint32_t cpnd_ckpt_sec_add_db(CPND_CKPT_REPLICA_INFO *,
                              CPND_CKPT_SECTION_INFO *);

void cpnd_ckpt_delete_all_sect(CPND_CKPT_NODE *);

bool cpnd_ckpt_sec_empty(const CPND_CKPT_REPLICA_INFO *);

#ifdef __cplusplus
}
#endif

#endif  // CKPT_CKPTND_CPND_SEC_H_
