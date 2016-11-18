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

  DESCRIPTION:

  This module contains the header file of doubly Link list library. Using this
  library, we can perform Queue/Stack/Ordered List operations.

  ******************************************************************************
  */

#ifndef BASE_NCSDLIB_H_
#define BASE_NCSDLIB_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"

#define m_NCS_DBLIST_FIND_FIRST(list_ptr) (list_ptr)->start_ptr
#define m_NCS_DBLIST_FIND_LAST(list_ptr) (list_ptr)->end_ptr
#define m_NCS_DBLIST_FIND_NEXT(node_ptr) (node_ptr)->next
#define m_NCS_DBLIST_FIND_PREV(node_ptr) (node_ptr)->prev

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Enumerated type used to arrange the doubly link list in order.
 *****************************************************************************/

typedef enum ncs_db_link_list_order {
  NCS_DBLIST_ASSCEND_ORDER = 1,
  NCS_DBLIST_DESCEND_ORDER,
  NCS_DBLIST_ANY_ORDER
} NCS_DB_LINK_LIST_ORDER;

/*****************************************************************************
 * This is the doubly link list Node, which stores actuall user data.
 *****************************************************************************/
typedef struct ncs_db_link_list_node {
  struct ncs_db_link_list_node *next;
  struct ncs_db_link_list_node *prev;
  uint8_t *key;
} NCS_DB_LINK_LIST_NODE;

/*****************************************************************************
 * NOTE:
 * this callback should return     0, if keys are equal
 *                              (> 0)  1, if key1 is greater than key2
 *                              (<0)   2, if key2 is greater than key1
 *****************************************************************************/

typedef uint32_t (*NCS_DB_LINK_LIST_CMP) (uint8_t *key1, uint8_t *key2);

/*****************************************************************************
 * Typedef for freeing the data which has been stored in the doubly linked
 * list library.
 *****************************************************************************/
typedef uint32_t (*NCS_DB_LINK_LIST_FREE) (NCS_DB_LINK_LIST_NODE *free_node);

/*****************************************************************************
 * Structure used to store all the important information about the doubly link
 * list nodes.
 *****************************************************************************/
typedef struct ncs_db_link_list {
  NCS_DB_LINK_LIST_ORDER order;   /* how to arrange the database     */
  NCS_DB_LINK_LIST_NODE *start_ptr;       /* start pointer of the db linklist */
  NCS_DB_LINK_LIST_NODE *end_ptr; /* End pointer of the db linklist  */
  NCS_DB_LINK_LIST_CMP cmp_cookie;        /* func ptr to compare the key     */
  NCS_DB_LINK_LIST_FREE free_cookie;      /* func ptr to free the node       */
  uint32_t n_nodes;               /* number of nodes present in list */
} NCS_DB_LINK_LIST;

/*****************************************************************************
 * This is the API used to add the node in the doubly linked list.
 * NOTE:
 *     Expect user to allocate memory for node_ptr, before calling this API.
 *****************************************************************************/
uint32_t ncs_db_link_list_add(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr);

/*****************************************************************************
 * This is the API used to find the node in the doubly linked list.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find(NCS_DB_LINK_LIST *list_ptr, uint8_t *key);

/*****************************************************************************
 * This is the API used to find the next node for the given key
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find_next(NCS_DB_LINK_LIST *list_ptr, uint8_t *prev_key);

/*****************************************************************************
 * This is the API used to find the prev node for the given key
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_find_prev(NCS_DB_LINK_LIST *list_ptr, uint8_t *pres_key);

/*****************************************************************************
 * This is the API used to delete the node for the given key. This API will use
 * the free callback which the user has registerd while initializing with the doubly
 * link list library.
 * NOTE:
 *     This API will fail, if you fail to register the free callback function.
 * If you just want to remove the node from the list then call the API
 * "ncs_db_link_list_remove()".
 *****************************************************************************/
uint32_t ncs_db_link_list_del(NCS_DB_LINK_LIST *list_ptr, uint8_t *key);

/*****************************************************************************
 * This is the API used to just delink the node from the doubly linked list
 * library. It won't free the node.
 * NOTE:
       It is the users responsibility to free the node pointer.
*****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_remove(NCS_DB_LINK_LIST *list_ptr, uint8_t *key);

/*****************************************************************************
 * This is the API used to just delink the node from the doubly linked list
 * library. Instead of key, the node ptr to be delinked is provided.
 * NOTE:
       It is the users responsibility to free the node pointer.
*****************************************************************************/
uint32_t ncs_db_link_list_delink(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node);

/*****************************************************************************
 * This is the API used to enqueue the given node to the top of the list.
 *****************************************************************************/
uint32_t ncs_db_link_list_enqeue(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr);

/*****************************************************************************
 * This is the API used to dequeue the node from the bottom of the list.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_dequeue(NCS_DB_LINK_LIST *list_ptr);

/*****************************************************************************
 * This is the API used to push the given node to the top of the list.
 *****************************************************************************/
uint32_t ncs_db_link_list_push(NCS_DB_LINK_LIST *list_ptr, NCS_DB_LINK_LIST_NODE *node_ptr);

/*****************************************************************************
 * This is the API used to pop the given node from the top of the list.
 *****************************************************************************/
NCS_DB_LINK_LIST_NODE *ncs_db_link_list_pop(NCS_DB_LINK_LIST *list_ptr);

#ifdef __cplusplus
}
#endif

#endif  // BASE_NCSDLIB_H_
