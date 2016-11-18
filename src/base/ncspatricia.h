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

 @@@@@     @@     @@@@@  @@@@@      @     @@@@      @      @@            @    @
 @    @   @  @      @    @    @     @    @    @     @     @  @           @    @
 @    @  @    @     @    @    @     @    @          @    @    @          @@@@@@
 @@@@@   @@@@@@     @    @@@@@      @    @          @    @@@@@@   @@@    @    @
 @       @    @     @    @   @      @    @    @     @    @    @   @@@    @    @
 @       @    @     @    @    @     @     @@@@      @    @    @   @@@    @    @

..............................................................................

  DESCRIPTION:

  This module contains declarations pertaining to implementation of
  patricia tree search/add/delete functions.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSPATRICIA_H_
#define BASE_NCSPATRICIA_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "base/ncsgl_defs.h"

#define m_KEY_CMP(t, k1, k2) memcmp(k1, k2, (size_t)(t)->params.key_size)
#define m_GET_BIT(key, bit)  ((bit < 0) ? 0 : ((int)((*((key) + (bit >> 3))) >> (7 - (bit & 0x07))) & 0x01))

typedef struct ncs_patricia_params {
  int key_size;   /* 1..NCS_PATRICIA_MAX_KEY_SIZE - in OCTETS */
} NCS_PATRICIA_PARAMS;

#define NCS_PATRICIA_MAX_KEY_SIZE 600   /* # octets */

typedef struct ncs_patricia_node {
  int bit;        /* must be signed type (bits start at -1) */
  struct ncs_patricia_node *left;
  struct ncs_patricia_node *right;
  uint8_t *key_info;
} NCS_PATRICIA_NODE;

#define NCS_PATRICIA_NODE_NULL ((NCS_PATRICIA_NODE *)0)

typedef uint8_t NCS_PATRICIA_LEXICAL_STACK;     /* ancient history... */

typedef struct ncs_patricia_tree {
  NCS_PATRICIA_NODE root_node;    /* A tree always has a root node. */
  NCS_PATRICIA_PARAMS params;
  unsigned int n_nodes;
} NCS_PATRICIA_TREE;

unsigned int ncs_patricia_tree_init(NCS_PATRICIA_TREE *const pTree,
                                    const NCS_PATRICIA_PARAMS *const pParams);
unsigned int ncs_patricia_tree_destroy(NCS_PATRICIA_TREE *const pTree);
void ncs_patricia_tree_clear(NCS_PATRICIA_TREE *const pTree);
unsigned int ncs_patricia_tree_add(NCS_PATRICIA_TREE *const pTree,
                                   NCS_PATRICIA_NODE *const pNode);
unsigned int ncs_patricia_tree_del(NCS_PATRICIA_TREE *const pTree,
                                   NCS_PATRICIA_NODE *const pNode);
NCS_PATRICIA_NODE *ncs_patricia_tree_get(const NCS_PATRICIA_TREE *const pTree,
                                         const uint8_t *const pKey);
NCS_PATRICIA_NODE *ncs_patricia_tree_get_best(const NCS_PATRICIA_TREE *const pTree, const uint8_t *const pKey, uint16_t KeyLen);        /* Length of key (in BITS) */
NCS_PATRICIA_NODE *ncs_patricia_tree_getnext(NCS_PATRICIA_TREE *const pTree, const uint8_t *const pKey);        /* NULL means get 1st */

int ncs_patricia_tree_size(const NCS_PATRICIA_TREE *const pTree);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSPATRICIA_H_
