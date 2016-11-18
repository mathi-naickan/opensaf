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

  This file contains PG database routines.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "amf/amfnd/avnd.h"

/* static function declarations */
static uint32_t avnd_pgdb_trk_key_cmp(uint8_t *key1, uint8_t *key2);

/****************************************************************************
  Name          : avnd_pgdb_rec_add
 
  Description   : This routine adds a record to the PG database.
 
  Arguments     : cb           - ptr to the AvND control block
                  csi_name     - ptr to the csi-name
                  rc           - ptr to the operation result
 
  Return Values : ptr to the pg rec, if successful
                  0, otherwise
 
  Notes         : None.
 ******************************************************************************/
AVND_PG *avnd_pgdb_rec_add(AVND_CB *cb, const std::string& csi_name, uint32_t *rc)
{
	AVND_PG *pg = 0;
	TRACE_ENTER2("Csi '%s'", csi_name.c_str());

	/* verify if this pg is already present in the db */
	if (cb->pgdb.find(csi_name) != nullptr) {
		*rc = AVND_ERR_DUP_PG;
		goto err;
	}

	/* a fresh pg... */
	pg = new AVND_PG();

	/* update the csi-name (patricia key) */
	pg->csi_name = csi_name;

	/* until avd acknowldges it's presence, it doesn't exit */
	pg->is_exist = false;

	/* initialize the mem-list */
	pg->mem_list.order = NCS_DBLIST_ANY_ORDER;
	pg->mem_list.cmp_cookie = avsv_dblist_saname_cmp;
	pg->mem_list.free_cookie = 0;

	/* initialize the track-list */
	pg->trk_list.order = NCS_DBLIST_ANY_ORDER;
	pg->trk_list.cmp_cookie = avnd_pgdb_trk_key_cmp;
	pg->trk_list.free_cookie = 0;

	/* add to pgdb */
	*rc = cb->pgdb.insert(pg->csi_name, pg);
	if (NCSCC_RC_SUCCESS != *rc) {
		*rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("PG DB record added: CSI = %s", csi_name.c_str());
	return pg;

 err:
	if (pg)
		delete pg;

	LOG_CR("PG DB record addition failed: CSI = %s", csi_name.c_str());
	return 0;
}

/****************************************************************************
  Name          : avnd_pgdb_rec_del
 
  Description   : This routine deletes a PG record from the PG database.
 
  Arguments     : cb           - ptr to the AvND control block
                  csi_name     - ptr to the csi-name
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_pgdb_rec_del(AVND_CB *cb, const std::string& csi_name)
{
	AVND_PG *pg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	/* get the pg record */
	pg = cb->pgdb.find(csi_name);
	if (!pg) {
		rc = AVND_ERR_NO_PG;
		goto err;
	}

	/* delete the mem-list */
	avnd_pgdb_mem_rec_del_all(cb, pg);

	/* delete the track-list */
	avnd_pgdb_trk_rec_del_all(cb, pg);

	/* remove from the pgdb */
	cb->pgdb.erase(csi_name);
	if (NCSCC_RC_SUCCESS != rc) {
		rc = AVND_ERR_TREE;
		goto err;
	}

	TRACE("PG DB record deleted: CSI = %s", csi_name.c_str());

	/* free the memory */
	delete pg;

	return rc;

 err:
	LOG_CR("PG DB record deletion failed: CSI = %s", csi_name.c_str());
	return rc;
}

/****************************************************************************
  Name          : avnd_pgdb_trk_rec_add
 
  Description   : This routine adds/modifies a PG track record to/in the PG 
                  record.
 
  Arguments     : cb       - ptr to the AvND control block
                  pg       - ptr to the pg rec
                  trk_info - ptr to the track info
 
  Return Values : ptr to the pg track rec, if successful
                  0, otherwise
 
  Notes         : None.
******************************************************************************/
AVND_PG_TRK *avnd_pgdb_trk_rec_add(AVND_CB *cb, AVND_PG *pg, AVND_PG_TRK_INFO *trk_info)
{
	AVND_PG_TRK *pg_trk = 0;
	TRACE_ENTER();

	/* get the trk rec */
	pg_trk = m_AVND_PGDB_TRK_REC_GET(*pg, trk_info->key);
	if (!pg_trk) {
		/* a new record.. alloc & link it to the dll */
		pg_trk = new AVND_PG_TRK();

		/* update the record key */
		pg_trk->info.key = trk_info->key;
		pg_trk->pg_dll_node.key = (uint8_t *)(&(pg_trk->info.key));

		/* add to the dll */
		if (NCSCC_RC_SUCCESS != ncs_db_link_list_add(&pg->trk_list, &pg_trk->pg_dll_node))
			goto err;

		pg_trk->info.flags = trk_info->flags;
	} else {

		/* Revert the flags for CHANGES and CHANGES_ONLY */
		if(m_AVND_PG_TRK_IS_CHANGES(pg_trk) && (trk_info->flags & SA_TRACK_CHANGES_ONLY)) {
			m_AVND_PG_TRK_CHANGES_RESET(pg_trk);
			m_AVND_PG_TRK_CHANGES_ONLY_SET(pg_trk);
		}
		if(m_AVND_PG_TRK_IS_CHANGES_ONLY(pg_trk) && (trk_info->flags & SA_TRACK_CHANGES)) {
			m_AVND_PG_TRK_CHANGES_ONLY_RESET(pg_trk);
			m_AVND_PG_TRK_CHANGES_SET(pg_trk);
		}
		/* If the current is also set, then set it in DB. Anyway, this gets reset in the end of the flow in 
		   avnd_pg_track_start(). */
		if(trk_info->flags & SA_TRACK_CURRENT) {
			m_AVND_PG_TRK_CURRENT_SET(pg_trk);
		}
	}
	pg_trk->info.mds_ctxt = trk_info->mds_ctxt;
	pg_trk->info.is_syn = trk_info->is_syn;
	TRACE_LEAVE();
	return pg_trk;

 err:
	if (pg_trk)
		delete pg_trk;

	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_pgdb_trk_rec_del
 
  Description   : This routine deletes a PG track record from the PG record.
 
  Arguments     : cb  - ptr to the AvND control block
                  pg  - ptr to the pg rec
                  key - ptr to the pg track key
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pgdb_trk_rec_del(AVND_CB *cb, AVND_PG *pg, AVND_PG_TRK_KEY *key)
{
	AVND_PG_TRK *pg_trk = 0;
	TRACE_ENTER();

	/* get the pg track record */
	pg_trk = m_AVND_PGDB_TRK_REC_GET(*pg, *key);
	if (!pg_trk)
		return;

	/* remove from the dll */
	ncs_db_link_list_remove(&pg->trk_list, (uint8_t *)key);

	/* free the memory */
	delete pg_trk;

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pgdb_trk_rec_del_all
 
  Description   : This routine deletes all the PG track records from the 
                  specified PG record.
 
  Arguments     : cb  - ptr to the AvND control block
                  pg  - ptr to the pg rec
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pgdb_trk_rec_del_all(AVND_CB *cb, AVND_PG *pg)
{
	AVND_PG_TRK *curr = 0;
	TRACE_ENTER();

	while (0 != (curr = (AVND_PG_TRK *)m_NCS_DBLIST_FIND_FIRST(&pg->trk_list)))
		avnd_pgdb_trk_rec_del(cb, pg, &curr->info.key);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pgdb_mem_rec_add
 
  Description   : This routine adds/modifies a PG member record to/in the PG 
                  record.
 
  Arguments     : cb       - ptr to the AvND control block
                  pg       - ptr to the pg rec
                  mem_info - ptr to the member info
 
  Return Values : ptr to the pg member rec, if successful
                  0, otherwise
 
  Notes         : None.
******************************************************************************/
AVND_PG_MEM *avnd_pgdb_mem_rec_add(AVND_CB *cb, AVND_PG *pg, SaAmfProtectionGroupNotificationT *mem_info)
{
	AVND_PG_MEM *pg_mem = 0;
	TRACE_ENTER();

	/* get the mem rec */
	pg_mem = m_AVND_PGDB_MEM_REC_GET(*pg, mem_info->member.compName);
	if (!pg_mem) {
		/* a new record.. alloc & link it to the dll */
		pg_mem = new AVND_PG_MEM();

		/* a fresh rec.. mark this member as a new addition */
		pg_mem->info.change = mem_info->change;

		/* update the record key */
		osaf_extended_name_alloc(osaf_extended_name_borrow(&mem_info->member.compName),
								 &pg_mem->info.member.compName);
		pg_mem->pg_dll_node.key = (uint8_t *)&pg_mem->info.member.compName;

		/* add to the dll */
		if (NCSCC_RC_SUCCESS != ncs_db_link_list_add(&pg->mem_list, &pg_mem->pg_dll_node))
			goto err;
	} else {
		pg_mem->info.change = SA_AMF_PROTECTION_GROUP_STATE_CHANGE;
	}

	/* update other params */
	pg_mem->info.member.haState = mem_info->member.haState;
	pg_mem->info.member.rank = mem_info->member.rank;

	TRACE_LEAVE();
	return pg_mem;

 err:
	if (pg_mem)
		delete pg_mem;

	TRACE_LEAVE();
	return 0;
}

/****************************************************************************
  Name          : avnd_pgdb_mem_rec_rmv
 
  Description   : This routine removes a PG member record from the PG record.
 
  Arguments     : cb            - ptr to the AvND control block
                  pg            - ptr to the pg rec
                  comp_name - ptr to the comp-name
 
  Return Values : ptr to the pg member rec, if successfully removed
                  0, otherwise
 
  Notes         : This routine only pops the cooresponding member record. It
                  doesn't delete it.
******************************************************************************/
AVND_PG_MEM *avnd_pgdb_mem_rec_rmv(AVND_CB *cb, AVND_PG *pg, const std::string& comp_name)
{
	AVND_PG_MEM *pg_mem = 0;
	SaNameT comp;

	TRACE_ENTER();
	osaf_extended_name_alloc(comp_name.c_str(), &comp);

	/* get the pg mem record */
	pg_mem = m_AVND_PGDB_MEM_REC_GET(*pg, comp);
	if (!pg_mem)
		return 0;

	/* remove from the dll */
	ncs_db_link_list_remove(&pg->mem_list, (uint8_t *)&comp);

	/* update the params that are no longer valid */
	pg_mem->info.change = SA_AMF_PROTECTION_GROUP_REMOVED;
	pg_mem->info.member.haState = static_cast<SaAmfHAStateT>(0);

	osaf_extended_name_free(&comp);
	TRACE_LEAVE();
	return pg_mem;
}

/****************************************************************************
  Name          : avnd_pgdb_mem_rec_del
 
  Description   : This routine pops & deletes a PG member record from the PG
                  record.
 
  Arguments     : cb            - ptr to the AvND control block
                  pg            - ptr to the pg rec
                  comp_name - ptr to the comp-name
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pgdb_mem_rec_del(AVND_CB *cb, AVND_PG *pg, const std::string& comp_name)
{
	AVND_PG_MEM *pg_mem = 0;
	TRACE_LEAVE();

	/* remove the pg mem record */
	pg_mem = avnd_pgdb_mem_rec_rmv(cb, pg, comp_name);
	if (!pg_mem)
		return;

	/* free the memory */
	delete pg_mem;

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pgdb_mem_rec_del_all
 
  Description   : This routine deletes all the PG member records from the 
                  specified PG record.
 
  Arguments     : cb  - ptr to the AvND control block
                  pg  - ptr to the pg rec
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_pgdb_mem_rec_del_all(AVND_CB *cb, AVND_PG *pg)
{
	AVND_PG_MEM *curr = 0;
	TRACE_LEAVE();

	while (0 != (curr = (AVND_PG_MEM *)m_NCS_DBLIST_FIND_FIRST(&pg->mem_list)))
		avnd_pgdb_mem_rec_del(cb, pg, Amf::to_string(&curr->info.member.compName));

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : avnd_pgdb_trk_key_cmp
 
  Description   : This routine compares the AVND_PG_TRK_KEY keys. It is used
                  by DLL library.
 
  Arguments     : key1 - ptr to the 1st key
                  key2 - ptr to the 2nd key
 
  Return Values : 0, if keys are equal
                  1, if key1 is greater than key2
                  2, if key1 is lesser than key2
 
  Notes         : None.
******************************************************************************/
uint32_t avnd_pgdb_trk_key_cmp(uint8_t *key1, uint8_t *key2)
{
	int i = 0;

	i = memcmp(key1, key2, sizeof(AVND_PG_TRK_KEY));

	return ((i == 0) ? 0 : ((i > 0) ? 1 : 2));
}
