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

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncs_queue.h"
#include "ncssysfpool.h"
#include "ncssysf_lck.h"

/****************************************************************************

    PROCEDURE NAME:    ncs_enqueue

    DESCRIPTION:

    Enqueue an event into the queue within a queue ctrl blk

    PARAMETERS:        
                       q:  Pointer to queue
         item: Ptr to the object to be queued.

    RETURNS:           Nothing.

    NOTES:

*****************************************************************************/

unsigned int ncs_enqueue(NCS_QUEUE *queue, void *item)
{

	NCS_QELEM *qelem;

	qelem = (NCS_QELEM *)item;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);

	if (queue->tail != NCS_QELEM_NULL)
		queue->tail->next = qelem;
	else
		queue->head = qelem;

	queue->count++;
	queue->tail = qelem;
	qelem->next = NCS_QELEM_NULL;

	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************

   PROCEDURE NAME:    ncs_enqueue_head

   DESCRIPTION:

   Enqueue an event into the queue within a queue ctrl blk at the head

   PARAMETERS:        
      q:  Pointer to queue
      item: Ptr to the object to be queued.

   RETURNS:           Nothing.

   NOTES:
            
*****************************************************************************/
unsigned int ncs_enqueue_head(NCS_QUEUE *queue, void *item)
{

	NCS_QELEM *qelem;

	qelem = (NCS_QELEM *)item;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);

	qelem->next = queue->head;
	queue->head = qelem;
	if (queue->tail == NCS_QELEM_NULL)
		queue->tail = qelem;

	queue->count++;

	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);

	return NCSCC_RC_SUCCESS;

}

/****************************************************************************

    PROCEDURE NAME:    ncs_dequeue

    DESCRIPTION:

    Dequeue a queued event from the top of the queue within a queue ctrl blk.
    Adjust the Q-count appropriately.

    PARAMETERS:        Ptr to the Queue Ctrl Blk

    RETURNS:           NULL ptr if nothing is queued.
                       Ptr to object removed from the queue.

    NOTES:

*****************************************************************************/

void *ncs_dequeue(NCS_QUEUE *queue)
{
	NCS_QELEM *qelem;	/* K100316 */

	qelem = NCS_QELEM_NULL;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
	if (queue->count) {
		queue->count--;
		if ((qelem = queue->head) != NCS_QELEM_NULL) {
			if ((queue->head = qelem->next) == NCS_QELEM_NULL)
				queue->tail = NCS_QELEM_NULL;
			qelem->next = NCS_QELEM_NULL;
		}
	}

	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);

	return ((void *)qelem);

}

/****************************************************************************

    PROCEDURE NAME:    ncs_remove_item

    DESCRIPTION:

    effectively, walk the linked list using the NCSQ_MATCH() function
    to find the target item, and remove it from the list.

    PARAMETERS:        Ptr to the Queue Ctrl Blk
                       void* value representing MATCH criteria
                       function matching prototype NCSQ_MATCH()

    RETURNS:           NULL ptr if a match is not found.
                       Ptr to object MATCHed and removed from the queue.

    NOTES:

*****************************************************************************/

void *ncs_remove_item(NCS_QUEUE *queue, void *key, NCSQ_MATCH match)
{
	NCS_QELEM *front = queue->head;
	NCS_QELEM *behind = (NCS_QELEM *)&queue->head;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
	while (front != NCS_QELEM_NULL) {
		if (match(key, front) == true) {
			behind->next = front->next;

			queue->count--;

			/* 
			 * Check if the number of elements in the queue are zero then only 
			 * set the tail to NULL else tail should point to the last element 
			 * of the queue.
			 */
			if (queue->count == 0)
				queue->tail = NCS_QELEM_NULL;
			else if (front->next == NCS_QELEM_NULL)
				queue->tail = behind;

			m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
			return front;
		}
		behind = front;
		front = front->next;
	}
	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
	return NULL;
}

/****************************************************************************

    PROCEDURE NAME:    ncs_find_item

    DESCRIPTION:

    Walk the linked list using the NCSQ_MATCH() function
    to find the target item. Once found, return the void* ncs_qelem value.

    PARAMETERS:        Ptr to the Queue Ctrl Blk
                       void* value representing MATCH criteria
                       function matching prototype NCSQ_MATCH()

    RETURNS:           NULL ptr if a match is not found.
                       Ptr to object MATCHed and from the queue.

    NOTES:

*****************************************************************************/

void *ncs_find_item(NCS_QUEUE *queue, void *key, NCSQ_MATCH match)
{
	NCS_QELEM *front = queue->head;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
	while (front != NCS_QELEM_NULL) {
		if (match(key, front) == true) {
			m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
			return front;
		}

		front = front->next;
	}
	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_WRITE, NCS_SERVICE_ID_COMMON, 22);
	return NULL;
}

/****************************************************************************

    PROCEDURE NAME:    ncs_peek_queue

    DESCRIPTION:

    Return the first queued event from the top of the queue.

    PARAMETERS:        Ptr to the Queue Ctrl Blk

    RETURNS:           NULL ptr if nothing is queued.
                       Ptr to object removed from the queue.

    NOTES:

*****************************************************************************/

void *ncs_peek_queue(NCS_QUEUE *queue)
{
	NCS_QELEM *qelem;

	qelem = NCS_QELEM_NULL;

	m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
	if (queue->count)
		qelem = queue->head;
	m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);

	return ((void *)qelem);

}

/****************************************************************************

    PROCEDURE NAME:    ncs_walk_items

    DESCRIPTION:

    find the next item in the list and return it.

    PARAMETERS:        Ptr to the Queue Ctrl Blk

    RETURNS:           NULL ptr if we are at the end or there are no elements
                       Ptr to the next object on the queue.

    NOTES:             itr->state must be set to NULL for first invocation
                       and must then be LEFT ALONE.
                       
    IMPORTANT:         When itr->state == 0, this function locks this queue.
                       It will remain locked until it has walked the entire
                       list.. That is, until it returns a NULL. This implies,
                       and NCS_QUEUE expects, that you are in a tight loop and
                       will break out of the loop once all nodes are visited.

*****************************************************************************/

void *ncs_walk_items(NCS_QUEUE *queue, NCS_Q_ITR *itr)
{
	NCS_QELEM *qelem;

	if (itr->state == NULL) {	/* first time in, prime the pump */
		if (queue->head) {
			m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
			return (itr->state = queue->head);
		} else
			return NULL;
	}

	qelem = (NCS_QELEM *)itr->state;	/* now its routine walking */
	if (qelem->next != NULL)
		return (itr->state = qelem->next);
	else {			/* hit the end of the line */

		m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
		return NULL;
	}
}

/****************************************************************************

    PROCEDURE NAME:    ncs_queue_get_next

    DESCRIPTION:

    Get the next item in the list and return it.

    PARAMETERS:        Ptr to the Queue Ctrl Blk

    RETURNS:           NULL ptr if we are at the end or there are no elements
                       Ptr to the next object on the queue.

    NOTES:             itr->state must be set to NULL for first invocation
                       and must then be LEFT ALONE.
                       
*****************************************************************************/

void *ncs_queue_get_next(NCS_QUEUE *queue, NCS_Q_ITR *itr)
{
	NCS_QELEM *qelem;

	if (itr->state == NULL) {	/* first time in, prime the pump */
		if (queue->head) {
			m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
			itr->state = queue->head;
			m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
			return (itr->state);
		} else
			return NULL;
	}

	qelem = (NCS_QELEM *)itr->state;	/* now its routine walking */
	if (qelem->next != NULL) {
		m_NCS_LOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
		itr->state = qelem->next;
		m_NCS_UNLOCK_V2(&queue->queue_lock, NCS_LOCK_READ, NCS_SERVICE_ID_COMMON, 22);
		return (itr->state);
	} else {		/* hit the end of the line */

		return NULL;
	}
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_create_queue

  DESCRIPTION:

  Primitive to Create and Initialize a NCS_QUEUE.

  PARAMETERS:

  NCS_QUEUE --- a queue task control block

  RETURNS:         Nothing

  NOTES:
*****************************************************************************/
void ncs_create_queue(NCS_QUEUE *queue)
{
	queue->count = 0;
	queue->head = NCS_QELEM_NULL;
	queue->tail = NCS_QELEM_NULL;
	m_NCS_LOCK_INIT_V2(&queue->queue_lock, NCS_SERVICE_ID_COMMON, 22);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_destroy_queue

  DESCRIPTION:

  Primitive to Destroy and Initialize a NCS_QUEUE.

  PARAMETERS:

  NCS_QUEUE --- a queue task control block

  RETURNS:         Nothing

  NOTES:
*****************************************************************************/
void ncs_destroy_queue(NCS_QUEUE *queue)
{
	m_NCS_LOCK_DESTROY_V2(&queue->queue_lock, NCS_SERVICE_ID_COMMON, 22);
}
