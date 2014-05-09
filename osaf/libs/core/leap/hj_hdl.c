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

  DESCRIPTION: Implementation for NCS_HDL service, an access-safe,
  use-safe means to fetch and use fleeting and/or volitile objects.
  See ncs_hdl.h for brief write-up of issues/capabilities. See the
  ncshm_ calls.

  This file also contains an implementation for a 'Local Persistence Guard',
  which is a cheap (in CPU cycles) persistence guard scheme (for read
  only object access, like the handle manager) that can be used when an object 
  that is known to exist needs a guard for some object that hangs off of this 
  object. For example, the LEAP sysfpool has a global struct that always exists.
  What we need to know and be assured of is that when we access memory, that 
  the structures managed will persist (destroy() could be called after all)
  while we are in the middle of an operation. See the ncslpg_ calls.

******************************************************************************
*/

#include <semaphore.h>

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncs_hdl.h"
#include "ncssysf_def.h"
#include "ncssysf_tsk.h"
#include "ncssysfpool.h"

/************************************************************************

 H a n d l e   M a n a g e r    P o o l s
 
   N a m e - s p a c e  p a r t i t i o n i n g   

************************************************************************/

static HM_CORE gl_hm;		/* anchor global for this primitive service */

HM_POOL gl_hpool[HM_POOL_CNT] = {
  /*-------------+----------+--------+----------------------*/
	/*             |   name space unit |                      */
	/* pool ID     |   min    |   max  |     approx # of hdls */
  /*-------------+----------+--------+----------------------*/
				/*     0     */ {0, 1},
				/*  2.1 million hdls */
					/*     1     */ {2, 32},
					/* 32.5 million hdls */
					/*     2     */ {33, 64},
					/* 33.5 million hdls */
					/*     3     */ {65, 96},
					/* 33.5 million hdls */
					/*     4     */ {97, 128},
					/* 33.5 million hdls */
					/*     5     */ {129, 160},
					/* 33.5 million hdls */
					/*     6     */ {161, 192},
					/* 33.5 million hdls */
					/*     7     */ {193, 224},
					/* 33.5 million hdls */
					/*     8     */ {225, 255}
					/* 32.5 million hdls */
};

/***************************************************************************
 *
 * P r i v a t e    H a n d l e   M g r    P o o l   F u n c t i o n s 
 *
 ***************************************************************************/

/* This mapping between PoolID and handle is based on the values
   as set in the gl_hpool[ ] array(just above). */
#define m_HM_DETM_POOL_FRM_HDL(lhdl) \
    ( ((uint32_t)(((HM_HDL*)lhdl)->idx1) < 2) ? NCSHM_POOL_LOCAL : ( (uint32_t)((uint32_t)((uint32_t)(((HM_HDL*)lhdl)->idx1) - 1)>>5) + 1 ) )

/* Given the unitID, get the poolID. This mapping has a dependency on
   the values as set in gl_hpool[ ] array. If the array values change, either
   create a new mapping between unitID and poolID, (or) use the function 
   hm_pool_id( )(just below). */
#if 1
#define m_HM_POOL_ID(unit) \
    ( (unit < 2) ? NCSHM_POOL_LOCAL : ( (uint32_t)((((uint32_t)unit) - 1)>>5) + 1 ) )
#else
/* Commented out function. Use this, if mapping for the poolID and unitID changes. */
#define m_HM_POOL_ID(unit) hm_pool_id
uint32_t hm_pool_id(uint8_t unit)
{
	uint32_t i = 0;

	for (i = 0; i < HM_POOL_CNT; i++) {
		if (gl_hpool[i].max >= unit)
			return i;
	}
	return m_LEAP_DBG_SINK(i);	/* This can't/shouldn't happen really */
}
#endif

/*****************************************************************************

   PROCEDURE NAME:   hm_init_pools

   DESCRIPTION:      validate pool definition and pluck useful info

*****************************************************************************/

uint32_t hm_init_pools(HM_PMGR *pmgr, HM_POOL *pool)
{
	uint32_t i;
	int32_t last_max = -1;

	for (i = 0; i < HM_POOL_CNT; i++) {
		/* force pool units to be contiguous & non-overlapping */

		if (!((last_max + 1) == pool->min))	/* contiguous */
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		if (pool->max >= 256)	/* greater than max units */
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		if (pool->min > pool->max)	/* malformed range spec */
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		last_max = pool->max;

		pmgr->curr = pool->min;
		pmgr->max = pool->max;

		pmgr++;
		pool++;
	}

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************
 *
 * P u b l i c    H a n d l e  M g r    A P I s (prefix 'ncshm_')
 *
 ***************************************************************************/

/*****************************************************************************

   PROCEDURE NAME:   ncshm_init

   DESCRIPTION:      put handle manager in known start state. Most of this
                     function proves that bitmap manipulation and size
                     assumptions are true for this target machine since
                     NetPlane cannot test them all in our lab.

                     If they are NOT true, tell NetPlane (debug statements)
                     should fire off so that analysis can take place.

*****************************************************************************/
uint32_t gl_im_created = 0;

uint32_t ncshm_init(void)
{
	/* Hdl Mgr does bit-fields; here we do a few exercises up front to make */
	/* sure YOUR target system can cope with bit-stuff we do............... */
	HM_HDL ha;
	HM_HDL hb;
	HM_HDL *p_hdl;
	uint32_t *p_temp;
	uint32_t cnt = 0;

	gl_im_created++;
	if (gl_im_created > 1)
		return NCSCC_RC_SUCCESS;

	assert(sizeof(HM_FREE) == sizeof(HM_CELL));	/* must be same size */

	assert(sizeof(uint32_t) == sizeof(HM_HDL));	/* must be same size */

	ha.idx1 = 1;		/* make up a fake handle with values */
	ha.idx2 = 2;
	ha.idx3 = 3;
	ha.seq_id = 6;

	/* cast to INT PTR, to HDL PTR, deref to HDL; bit-fields still stable ? */

	p_temp = (uint32_t *)(&ha);
	p_hdl = (HM_HDL *)p_temp;
	hb = *p_hdl;

	/* are all the bitfields still in tact?? .............................. */

	assert(((ha.idx1 == hb.idx1) && (ha.idx2 == hb.idx2) && (ha.idx3 == hb.idx3) && (ha.seq_id == hb.seq_id)));

	/* Done with basic tests; now we move on to normal initialization      */

	memset(&gl_hm, 0, sizeof(HM_CORE));
	for (cnt = 0; cnt < HM_POOL_CNT; cnt++)
		m_NCS_LOCK_INIT(&gl_hm.lock[cnt]);

	if (hm_init_pools(gl_hm.pool, gl_hpool) != NCSCC_RC_SUCCESS)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_delete

   DESCRIPTION:      recover all resources

*****************************************************************************/

void ncshm_delete(void)
{
	uint32_t i, j;
	HM_UNIT *unit;

	gl_im_created--;
	if (gl_im_created > 0)
		return;

	/* Destroy all the locks now. */
	for (i = 0; i < HM_POOL_CNT; i++) {
		if (m_NCS_LOCK_DESTROY(&gl_hm.lock[i]) != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
		}
	}

	for (i = 0; i < HM_UNIT_CNT; i++) {
		if ((unit = gl_hm.unit[i]) != NULL) {
			for (j = 0; j < HM_BANK_CNT; j++) {
				if (unit->cells[j] != NULL)
					free(unit->cells[j]);
			}
			free(unit);
		}
	}

	/* Memset the gl_hm data structure. */
	memset(&gl_hm, 0, sizeof(HM_CORE));

	/* ncshm_init(); */	/* put struct back in start state.. Why not?? */
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_create_hdl

   DESCRIPTION:      Secure a handle and bind associated client save data with
                     it. Return the uint32_t handle that leads to saved data.

*****************************************************************************/
uint32_t ncshm_create_hdl(uint8_t pool, NCS_SERVICE_ID id, NCSCONTEXT save)
{
	HM_FREE *free;
	HM_CELL *cell;
	uint32_t ret = 0;

	if (pool >= HM_POOL_CNT)
		return ret;	/* Invalid handle returned. */

	m_NCS_LOCK(&gl_hm.lock[pool], NCS_LOCK_WRITE);

	if ((free = hm_alloc_cell(pool)) != NULL) {
		cell = hm_find_cell(&free->hdl);	/* These two lines are sanity */
		assert(((void *)free == (void *)cell));	/* checks that add no value   */

		ret = (*(uint32_t *)&free->hdl);
		cell->data = save;	/* store user stuff and internal state */
		cell->use_ct = 1;
		cell->svc_id = id;
		cell->busy = true;
	}

	m_NCS_UNLOCK(&gl_hm.lock[pool], NCS_LOCK_WRITE);
	return ret;
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_declare_hdl

   DESCRIPTION:      On backup side, an RE declares that the handle specified
                     should be assigned to the invoker. Hdl Mgr SHOULD NOT run
                     into a name clash (declared name already in-use) as
                     1 - primary & backup side agree to use same Hdl Mgr Pool ID,
                     2 - And Pools by same ID are same shape (same name space).
                     3 - when backup, env only gets hdls via ncshm_declare_hdl()
                         (and never via ncshm_create_hdl()).

*****************************************************************************/

uint32_t ncshm_declare_hdl(uint32_t uhdl, NCS_SERVICE_ID id, NCSCONTEXT save)
{
	HM_FREE *free;
	HM_CELL *cell = NULL;
	HM_HDL *hdl = (HM_HDL *)&uhdl;
	uint32_t ret = NCSCC_RC_FAILURE;
	uint32_t pool_id = 0;

	pool_id = m_HM_DETM_POOL_FRM_HDL(&uhdl);
	if (pool_id >= HM_POOL_CNT)
		return ret;

	m_NCS_LOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);

	if ((free = hm_target_cell(hdl)) != NULL) {	/* must have THIS cell */
		cell = hm_find_cell(hdl);	/* These two lines are sanity */
		assert(((void *)free == (void *)cell));	/* checks that add no value   */

		cell->data = save;	/* store user stuff and internal state */
		cell->use_ct = 1;
		cell->svc_id = id;
		cell->busy = true;
		ret = NCSCC_RC_SUCCESS;
	}

	m_NCS_UNLOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);
	return ret;
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_destroy_hdl

   DESCRIPTION:      destroy the binding between the passed handle and the 
                     associated data. If some other thread currently is using
                     the data, BLOCK this thread till ref-count == 1

*****************************************************************************/

NCSCONTEXT ncshm_destroy_hdl(NCS_SERVICE_ID id, uint32_t uhdl)
{
	HM_CELL *cell = NULL;
	HM_HDL *hdl = (HM_HDL *)&uhdl;
	NCSCONTEXT data = NULL;
	uint32_t pool_id = 0;

	pool_id = m_HM_DETM_POOL_FRM_HDL(&uhdl);
	if (pool_id >= HM_POOL_CNT)
		return NULL;

	m_NCS_LOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);

	if ((cell = hm_find_cell(hdl)) != NULL) {
		if ((cell->seq_id == hdl->seq_id) && ((NCS_SERVICE_ID)cell->svc_id == id) && (cell->busy == true)) {
			cell->busy = false;
			data = cell->data;

			if (cell->use_ct > 1) {
				hm_block_me(cell, (uint8_t)pool_id);	/* must unlock inside */
				m_NCS_LOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);	/* must lock again!!! */
			}
			hm_free_cell(cell, hdl, true);
		}
	}
	m_NCS_UNLOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);

	return data;
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_take_hdl

   DESCRIPTION:      If all validation stuff is in order return the associated
                     data that this hdl leads to.

*****************************************************************************/
NCSCONTEXT ncshm_take_hdl(NCS_SERVICE_ID id, uint32_t uhdl)
{
	HM_CELL *cell = NULL;
	HM_HDL *hdl = (HM_HDL *)&uhdl;
	NCSCONTEXT data = NULL;
	uint32_t pool_id = 0;

	pool_id = m_HM_DETM_POOL_FRM_HDL(&uhdl);
	if (pool_id >= HM_POOL_CNT)
		return NULL;

	m_NCS_LOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);

	if ((cell = hm_find_cell(hdl)) != NULL) {
		if ((cell->seq_id == hdl->seq_id) && ((NCS_SERVICE_ID)cell->svc_id == id) && (cell->busy == true)) {
			if (++cell->use_ct == 0) {
				m_LEAP_DBG_SINK_VOID;	/* Too many takes()s!! */
			}

			data = cell->data;
		}
	}

	m_NCS_UNLOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);
	return data;
}

/*****************************************************************************

   PROCEDURE NAME:   ncshm_give_hdl

   DESCRIPTION:      Inform Hdl Manager that you are done with associated 
                     data.

*****************************************************************************/
void ncshm_give_hdl(uint32_t uhdl)
{
	HM_CELL *cell = NULL;
	HM_HDL *hdl = (HM_HDL *)&uhdl;
	uint32_t pool_id = 0;

	pool_id = m_HM_DETM_POOL_FRM_HDL(&uhdl);
	if (pool_id >= HM_POOL_CNT)
		return;

	m_NCS_LOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);

	if ((cell = hm_find_cell(hdl)) != NULL) {
		if (cell->seq_id == hdl->seq_id) {
			if (--cell->use_ct < 1) {
				m_LEAP_DBG_SINK_VOID;	/* Client BUG..Too many give()s!! */
				cell->use_ct++;
			} else {
				if ((cell->busy == false) && (cell->use_ct == 1))
					hm_unblock_him(cell);
			}
		}
	}
	m_NCS_UNLOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);
}

/***************************************************************************
 *
 * P r i v a t e  H a n d l e   M g r   F u n c t i o n s (prefix 'hm_')
 *
 ***************************************************************************/

/*****************************************************************************

   PROCEDURE NAME:   hm_alloc_cell

   DESCRIPTION:      get a cell. If there are none, create a bunch.

*****************************************************************************/

HM_FREE *hm_alloc_cell(uint8_t id)
{
	HM_FREE *free;
	HM_PMGR *pmgr = &gl_hm.pool[id];

	if (pmgr->free_pool == NULL) {
		if (hm_make_free_cells(pmgr) != NCSCC_RC_SUCCESS) {
			m_LEAP_DBG_SINK_VOID;
			return NULL;
		}
	}

	/* OK, pool should be replenished, lets get one */

	free = pmgr->free_pool;
	pmgr->free_pool = free->next;

	m_HM_STAT_ADD_IN_USE(pmgr->in_use);
	m_HM_STAT_RMV_FR_Q(pmgr->in_q);

	return free;
}

/*****************************************************************************

   PROCEDURE NAME:   hm_find_cell

   DESCRIPTION:      Given a handle, find the associated cell. This is the
                     master lookup routine. Its fast.

*****************************************************************************/

HM_CELL *hm_find_cell(HM_HDL *hdl)
{
	HM_UNIT *unit;
	HM_CELLS *spot;

	if ((unit = gl_hm.unit[hdl->idx1]) == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return NULL;
	}

	if ((spot = unit->cells[hdl->idx2]) == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return NULL;
	}

	return &(spot->cell[hdl->idx3]);
}

/*****************************************************************************

   PROCEDURE NAME:   hm_free_cell

   DESCRIPTION:      put a used cell back in the available cell pool.

*****************************************************************************/

void hm_free_cell(HM_CELL *cell, HM_HDL *hdl, bool recycle)
{
	HM_PMGR *pmgr;
	HM_FREE *free = (HM_FREE *)cell;

	free->hdl = *hdl;
	free->hdl.seq_id++;

	if (free->hdl.seq_id == 0)
		free->hdl.seq_id++;	/* seq_id must be non-zero always */

	pmgr = &gl_hm.pool[m_HM_POOL_ID((uint8_t)free->hdl.idx1)];
	free->next = pmgr->free_pool;
	pmgr->free_pool = free;
	m_HM_STAT_ADD_TO_Q(pmgr->in_q);
	m_HM_STAT_RMV_IN_USE(recycle, pmgr->in_use);
}

/*****************************************************************************

   PROCEDURE NAME:   ncs_make_free_cells

   DESCRIPTION:      create a bunch of cells and put them in the free pool.

*****************************************************************************/
uint32_t hm_make_free_cells(HM_PMGR *pmgr)
{
	HM_UNIT *unit;
	HM_CELLS *cells;
	HM_CELL *cell;
	HM_HDL hdl;
	uint32_t i;

	unit = gl_hm.unit[pmgr->curr];

	/* first time this pool has been used ?? */

	if (unit == NULL) {
		if ((unit = (HM_UNIT*) malloc(sizeof(HM_UNIT))) == NULL)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		memset(unit, 0, sizeof(HM_UNIT));
		gl_hm.unit[pmgr->curr] = unit;
	}

	/* another million hdls used up ?? */

	/* Check to see if BACKUP has caused random cell banks to be created */

	while (unit->cells[unit->curr] != NULL) {	/* BACKUP has been here!! */
		/* Commenting as condition is always false and it was giving warnings */
		++unit->curr;
	}

	/* Now go make HM_CELL_CNT (4096) new cells */
	if ((cells = (HM_CELLS*) malloc(sizeof(HM_CELLS))) == NULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	memset(cells, 0, sizeof(HM_CELLS));

	hdl.idx1 = pmgr->curr;	/* set handle conditions */
	hdl.idx2 = unit->curr;
	hdl.seq_id = 0;

	unit->cells[unit->curr++] = cells;	/* update curr++ for next time */

	for (i = 0; i < HM_CELL_CNT; i++) {	/* carve um up and put in free-po0l */
		hdl.idx3 = i;
		cell = &(cells->cell[i]);
		hm_free_cell(cell, &hdl, false);
	}

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

   PROCEDURE NAME:   hm_target_cell

   DESCRIPTION:      We MUST find the target cell available or something is
                     not right!!!! this is used by BACKUP who declares what
                     the handle value should be.

   NOTE..............This is used on the backup and can be slow depending on
                     the size of the free pool. The lookup algorithm is
                     optimized for the primary side, where performance is a
                     more critical issue.

*****************************************************************************/

HM_FREE *hm_target_cell(HM_HDL *hdl)
{
	HM_PMGR *pmgr;
	HM_UNIT *unit;
	HM_CELLS *cells;
	HM_CELL *cell;
	HM_HDL tmp_hdl;
	HM_FREE *back;
	HM_FREE *found;
	uint32_t i;

	uint32_t tgt = *((uint32_t *)hdl);

	pmgr = &gl_hm.pool[m_HM_POOL_ID((uint8_t)hdl->idx1)];	/* determine pool */

	if ((unit = gl_hm.unit[hdl->idx1]) == NULL) {
		if ((unit = (HM_UNIT*)  malloc(sizeof(HM_UNIT))) == NULL) {
			m_LEAP_DBG_SINK_VOID;
			return NULL;
		}

		memset(unit, 0, sizeof(HM_UNIT));
		gl_hm.unit[hdl->idx1] = unit;
	}

	if ((cells = unit->cells[hdl->idx2]) == NULL) {
		if ((cells = (HM_CELLS*) malloc(sizeof(HM_CELLS))) == NULL) {
			m_LEAP_DBG_SINK_VOID;
			return NULL;
		}

		memset(cells, 0, sizeof(HM_CELLS));

		tmp_hdl.idx1 = hdl->idx1;
		tmp_hdl.idx2 = hdl->idx2;
		tmp_hdl.seq_id = 0;

		unit->cells[hdl->idx2] = cells;	/* put it where it goes */

		for (i = 0; i < HM_CELL_CNT; i++) {	/* carve um up and put in free-pool */
			tmp_hdl.idx3 = i;
			cell = &(cells->cell[i]);
			hm_free_cell(cell, &tmp_hdl, false);
		}
	}

	/* prepare to walk free list and find target cell */

	back = (HM_FREE *)&pmgr->free_pool;

	while (back->next != NULL) {
		uint32_t tst = *((uint32_t *)&back->next->hdl);
		if (tst == tgt) {
			found = back->next;	/* found it */
			back->next = back->next->next;	/* splice it out */

			m_HM_STAT_ADD_IN_USE(pmgr->in_use);
			m_HM_STAT_RMV_FR_Q(pmgr->in_q);

			return found;	/* return it */
		}
		back = back->next;
	}

	m_LEAP_DBG_SINK_VOID;
	return NULL;
}

/*****************************************************************************

   PROCEDURE NAME:   hm_block_me

   DESCRIPTION:      The destroy()er found ref-count > 1; pend this thread.

*****************************************************************************/

void hm_block_me(HM_CELL *cell, uint8_t pool_id)
{
	int rc;
	sem_t sem;
	m_HM_STAT_CRASH(gl_hm.woulda_crashed);

	rc = sem_init(&sem, 0, 0);		/* Create a semaphor to block this thread */
	osafassert(rc == 0);
	cell->data = &sem;
	m_NCS_UNLOCK(&gl_hm.lock[pool_id], NCS_LOCK_WRITE);	/* let others run */

wait_again:						/* stay here till refcount == 1 */
	if (sem_wait(&sem) == -1) {
		if (errno == EINTR)
			goto wait_again;
		else
			osafassert(0);
	}

	(void)sem_destroy(&sem);	/* OK, all set, continue on.... */
}

/*****************************************************************************

   PROCEDURE NAME:   hm_unblock_him

   DESCRIPTION:      A give()r made ref-count <= 1; unblock the destroy()er
                     thread.

*****************************************************************************/

void hm_unblock_him(HM_CELL *cell)
{
	int rc = sem_post((sem_t*)cell->data);	/* unblock that destroy thread */
	osafassert(rc == 0);
}

/***************************************************************************
 *
 *
 *
 * P u b l i c    L o c a l   P e r s i s t e n c e   G u a r d  
 *
 *                         A P I s (prefix 'ncslpg_')
 *
 *
 *
 ***************************************************************************/

/*****************************************************************************

   PROCEDURE NAME:   ncslpg_take

   DESCRIPTION:      If all validation stuff is in order return true, which
                     means this thread can enter this object.

*****************************************************************************/

bool ncslpg_take(NCSLPG_OBJ *pg)
{
	m_NCS_OS_ATOMIC_INC(&(pg->inhere));	/* set first, ask later.. to beat 'closing' */
	if (pg->open == true)
		return true;	/* its open, lets go in */
	else
		m_NCS_OS_ATOMIC_DEC(&(pg->inhere));
	return false;		/* its closed */
}

/*****************************************************************************

   PROCEDURE NAME:   ncslpg_give

   DESCRIPTION:      decriment the refcount, as this thread is leaving now.

*****************************************************************************/

uint32_t ncslpg_give(NCSLPG_OBJ *pg, uint32_t ret)
{
	m_NCS_OS_ATOMIC_DEC(&(pg->inhere));
	return ret;
}

/*****************************************************************************

   PROCEDURE NAME:   ncslpg_create

   DESCRIPTION:      Put the passed NCSLPG_OBJ in start state. If its already
                     in start state, return FAILURE.

*****************************************************************************/

uint32_t ncslpg_create(NCSLPG_OBJ *pg)
{
	if (pg->open == true)
		m_LEAP_DBG_SINK_VOID;
	pg->open = true;
	pg->inhere = 0;
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

   PROCEDURE NAME:   ncslpg_destroy

   DESCRIPTION:      Close this LPG. Wait for all other threads to leave before
                     returning to the invoker, allowing her to proceed. Note
                     that if this object is already closed, this function
                     returns false (invoker should not proceed, as the object is
                     already destroyed or being destoyed.

*****************************************************************************/

bool ncslpg_destroy(NCSLPG_OBJ *pg)
{
	if (pg->open == false)
		return false;	/* already closed            */
	pg->open = false;	/* stop others from entering */
	while (pg->inhere != 0)	/* Anybody inhere??          */
		m_NCS_TASK_SLEEP(1);	/* OK, I'll wait; could do semaphore I suppose */

	return true;		/* Invoker can proceed to get rid of protected thing */
}
