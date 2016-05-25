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

/*****************************************************************************
 *   FILE NAME: cpnd_sec.cc
 *
 *   DESCRIPTION: C++ implementation of section id map
 *
 ****************************************************************************/

#include <cstring>
#include <map>
#include "logtrace.h"
#include "ncsgl_defs.h"
#include "cpnd.h"

struct ltSectionIdT {
  bool operator()(const SaCkptSectionIdT *s1, const SaCkptSectionIdT *s2) const
  {
    bool status(false);

    if (s1->idLen < s2->idLen)
      status = true;
    else if (s1->idLen > s2->idLen)
      status = false;
    else 
      status = (memcmp(s1->id, s2->id, s1->idLen) < 0);

    return status;
  }
};

typedef std::map<const SaCkptSectionIdT *,
                 CPND_CKPT_SECTION_INFO *,
                 ltSectionIdT> SectionMap;

typedef std::map<uint32_t, CPND_CKPT_SECTION_INFO *> LocalSectionIdMap;

void
cpnd_ckpt_sec_map_init(CPND_CKPT_REPLICA_INFO *replicaInfo)
{
  if (replicaInfo->section_db) {
    LOG_ER("section map already exists");
    osafassert(false);
  }

  if (replicaInfo->local_section_db) {
    LOG_ER("section map for local section id already exists");
    osafassert(false);
  }

  replicaInfo->section_db = new SectionMap;
  replicaInfo->local_section_db = new LocalSectionIdMap;
}

void
cpnd_ckpt_sec_map_destroy(CPND_CKPT_REPLICA_INFO *replicaInfo)
{
  delete static_cast<SectionMap *>(replicaInfo->section_db);
  delete static_cast<LocalSectionIdMap *>(replicaInfo->local_section_db);

  replicaInfo->section_db = 0;
  replicaInfo->local_section_db = 0;
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_get
 *
 * Description   : Function to Find the section in a checkpoint.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *               : SaCkptSectionIdT id - Section Identifier
 *                 
 * Return Values :  NULL/CPND_CKPT_SECTION_INFO
 *
 * Notes         : None.
 *****************************************************************************/
CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get(const CPND_CKPT_NODE *cp_node, const SaCkptSectionIdT *id)
{
  CPND_CKPT_SECTION_INFO *sectionInfo(0);

  TRACE_ENTER();

  if (cp_node->replica_info.n_secs) {
    SectionMap *map(static_cast<SectionMap *>
      (cp_node->replica_info.section_db));

    if (map) {
      SectionMap::iterator it(map->find(id));

      if (it != map->end())
        sectionInfo = it->second;
    }
    else {
      LOG_ER("can't find map in cpnd_ckpt_sec_get");
      osafassert(false);
    }
  }
  else {
    TRACE_4("cpnd replica has no section for ckpt_id:%llx",cp_node->ckpt_id);
  }

  TRACE_LEAVE();
  return sectionInfo;
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_get_create
 *
 * Description   : Function to Find the section in a checkpoint before create.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *               : SaCkptSectionIdT id - Section Identifier
 *                 
 * Return Values :  NULL/CPND_CKPT_SECTION_INFO
 *
 * Notes         : None.
 *****************************************************************************/
CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_create(const CPND_CKPT_NODE *cp_node,
                         const SaCkptSectionIdT *id)
{
  return cpnd_ckpt_sec_get(cp_node, id);
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_find
 *
 * Description   : Function to Find the section in a checkpoint.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *               : SaCkptSectionIdT id - Section Identifier
 *                 
 * Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t
cpnd_ckpt_sec_find(const CPND_CKPT_NODE *cp_node, const SaCkptSectionIdT *id)
{
  return (cpnd_ckpt_sec_get(cp_node, id)) ? NCSCC_RC_SUCCESS : NCSCC_RC_FAILURE;
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_del
 *
 * Description   : Function to remove the section from a checkpoint.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *               : SaCkptSectionIdT id - Section Identifier
 *                 
 * Return Values :  ptr to CPND_CKPT_SECTION_INFO/NULL;
 *
 * Notes         : None.
 *****************************************************************************/
CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_del(CPND_CKPT_NODE *cp_node, SaCkptSectionIdT *id)
{
  CPND_CKPT_SECTION_INFO *sectionInfo(0);

  TRACE_ENTER();

  SectionMap *map(static_cast<SectionMap *>(cp_node->replica_info.section_db));

  if (map) {
    SectionMap::iterator it(map->find(id));

    if (it != map->end()) {
      sectionInfo = it->second;
      map->erase(it);
    }
  }
  else {
    LOG_ER("can't find map in cpnd_ckpt_sec_del");
    osafassert(false);
  }

  LocalSectionIdMap *localSecMap(static_cast<LocalSectionIdMap *>
    (cp_node->replica_info.local_section_db));

  if (localSecMap) {
    if (sectionInfo) {
      LocalSectionIdMap::iterator it(localSecMap->find(sectionInfo->lcl_sec_id));

      if (it != localSecMap->end())
        localSecMap->erase(it);
    }
  }
  else {
    LOG_ER("can't find local sec map in cpnd_ckpt_sec_del");
    osafassert(false);
  }

  if (sectionInfo) {
    cp_node->replica_info.n_secs--;
    cp_node->replica_info.mem_used = cp_node->replica_info.mem_used - (sectionInfo->sec_size);

    // UPDATE THE SECTION HEADER
    uint32_t rc(cpnd_sec_hdr_update(sectionInfo, cp_node));
    if (rc == NCSCC_RC_FAILURE) {
      TRACE_4("cpnd sect hdr update failed");
    }

    // UPDATE THE CHECKPOINT HEADER
    rc = cpnd_ckpt_hdr_update(cp_node);
    if (rc == NCSCC_RC_FAILURE) {
      TRACE_4("cpnd ckpt hdr update failed");
    }
  }

  TRACE_LEAVE();

  return sectionInfo;
}

/****************************************************************************
 * Name          : cpnd_ckpt_sec_add_db
 *
 * Description   : Function to add the section to a checkpoint.
 *
 * Arguments     : CPND_CKPT_REPLICA_INFO *replicaInfo - Check point replica.
 *               : CPND_CKPT_SECTION_INFO sectionInfo - Section Info
 *                 
 * Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
uint32_t
cpnd_ckpt_sec_add_db(CPND_CKPT_REPLICA_INFO *replicaInfo,
                     CPND_CKPT_SECTION_INFO *sectionInfo)
{
  uint32_t rc(NCSCC_RC_SUCCESS);

  SectionMap *map(static_cast<SectionMap *>(replicaInfo->section_db));

  if (map) {
    std::pair<SectionMap::iterator, bool> p(map->insert(
      std::make_pair(&sectionInfo->sec_id, sectionInfo)));

    if (!p.second) {
      LOG_ER("unable to add section info to map");
      rc = NCSCC_RC_FAILURE;
      return rc;
    }
  }
  else {
    LOG_ER("can't find map in cpnd_ckpt_sec_add_db");
    osafassert(false);
  }

  LocalSectionIdMap *localSecMap(
    static_cast<LocalSectionIdMap *>(replicaInfo->local_section_db));

  if (localSecMap) {
    std::pair<LocalSectionIdMap::iterator, bool> p(localSecMap->insert(
      std::make_pair(sectionInfo->lcl_sec_id, sectionInfo)));

    if (!p.second) {
      LOG_ER("unable to add section info to local section id map - the id %d already existed", 
             sectionInfo->lcl_sec_id);
      rc = NCSCC_RC_FAILURE;

      /* Erase the element was inserted into section_db */
      map->erase(&sectionInfo->sec_id);
    }
  }
  else {
    LOG_ER("can't find local sec map in cpnd_ckpt_sec_add_db");
    osafassert(false);
  }

  return rc;
}

/****************************************************************************
 * Name          : cpnd_ckpt_delete_all_sect
 *
 * Description   : Function to add the section to a checkpoint.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *                 
 * Return Values :  NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
 *
 * Notes         : None.
 *****************************************************************************/
void
cpnd_ckpt_delete_all_sect(CPND_CKPT_NODE *cp_node)
{
  LocalSectionIdMap *localSecMap(static_cast<LocalSectionIdMap *>
    (cp_node->replica_info.local_section_db));

  if (localSecMap) {
    localSecMap->erase(localSecMap->begin(), localSecMap->end());
  }
  else {
    LOG_ER("can't find local sec map in cpnd_ckpt_delete_all_sect");
    osafassert(false);
  }

  SectionMap *map(static_cast<SectionMap *>(cp_node->replica_info.section_db));

  if (map) {
    SectionMap::iterator it(map->begin());

    while (it != map->end()) {
      cp_node->replica_info.n_secs--;

      CPND_CKPT_SECTION_INFO *section(it->second);

      if (section->ckpt_sec_exptmr.is_active)
        cpnd_tmr_stop(&section->ckpt_sec_exptmr);

      m_CPND_FREE_CKPT_SECTION(section);

      SectionMap::iterator tmpIt(it);
      ++it;

      map->erase(tmpIt);
    }
  }
  else {
    LOG_ER("can't find sec map in cpnd_ckpt_delete_all_sect");
    osafassert(false);
  }
}

/****************************************************************************
 * Name          : cpnd_get_sect_with_id 
 *
 * Description   : Function to Find the section in a checkpoint.
 *
 * Arguments     : CPND_CKPT_NODE *cp_node - Check point node.
 *               : lck_sec_id -  lcl Section Identifier
 *                 
 * Return Values :  NULL/ pointer to CPND_CKPT_SECTION_INFO
 *
 * Notes         : None.
 *****************************************************************************/
CPND_CKPT_SECTION_INFO *
cpnd_get_sect_with_id(const CPND_CKPT_NODE *cp_node, uint32_t lcl_sec_id)
{
  CPND_CKPT_SECTION_INFO *sectionInfo(0);

  if (cp_node->replica_info.n_secs) {
    LocalSectionIdMap *map(static_cast<LocalSectionIdMap *>
      (cp_node->replica_info.local_section_db));

    if (map) {
      LocalSectionIdMap::iterator it(map->find(lcl_sec_id));

      if (it != map->end())
        sectionInfo = it->second;
    }
    else {
      LOG_ER("can't find sec map in cpnd_get_sect_with_id");
      osafassert(false);
    }
  }
  else {
    TRACE_4("cpnd replica has no sections for ckpt_id:%llx",cp_node->ckpt_id);
  }

  return sectionInfo;
}

bool
cpnd_ckpt_sec_empty(const CPND_CKPT_REPLICA_INFO *replicaInfo)
{
  SectionMap *map(static_cast<SectionMap *>(replicaInfo->section_db));

  return map ? map->empty() : true;
}

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_first(const CPND_CKPT_REPLICA_INFO *replicaInfo)
{
  CPND_CKPT_SECTION_INFO *sectionInfo(0);

  SectionMap *map(static_cast<SectionMap *>(replicaInfo->section_db));

  if (map) {
    SectionMap::iterator it(map->begin());

    if (it != map->end())
      sectionInfo = it->second;
  }
  else {
    LOG_ER("can't find sec map in cpnd_ckpt_sec_get_first");
  }

  return sectionInfo;
}

CPND_CKPT_SECTION_INFO *
cpnd_ckpt_sec_get_next(const CPND_CKPT_REPLICA_INFO *replicaInfo,
                       const CPND_CKPT_SECTION_INFO *section)
{
  CPND_CKPT_SECTION_INFO *sectionInfo(0);

  SectionMap *map(static_cast<SectionMap *>(replicaInfo->section_db));

  if (map) {
    SectionMap::iterator it(map->find(&section->sec_id));

    if (it != map->end()) {
      if (++it != map->end())
        sectionInfo = it->second;
    }
  }
  else {
    LOG_ER("can't find sec map in cpnd_ckpt_sec_get_next");
  }

  return sectionInfo;
}
