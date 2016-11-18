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

  Library of subroutines to maintain "Patricia Tree" structures.
  These trees allow efficient implementation of indexed file operations
  with records with long keys.
..............................................................................

  GLOBAL FUNCTIONS INCLUDED in this module:

  ncs_patricia_tree_init()
  ncs_patricia_tree_destroy()
  ncs_patricia_tree_clear()
  ncs_patricia_tree_add()
  ncs_patricia_tree_del()
  ncs_patricia_tree_get()
  ncs_patricia_tree_getnext()
  ncs_patricia_tree_size()

*******************************************************************************
*/

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/ncssysfpool.h"
#include "base/ncssysf_def.h"
#include "base/ncspatricia.h"

const static uint8_t BitMasks[9] = {
	0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/*****************************************************************************
 * PRIVATE (static) FUNCTIONS
 *****************************************************************************/

/*****************************************************************************

  DATA OBJECT NAME: search

  DESCRIPTION:

    Walks the tree checking the bit values at each node to find a match.
    The search ends when an upward link is found in a node.

  ARGUMENTS:

    NCS_PATRICIA_TREE * - Pointer to the table structure
    uint8_t *key: Key of the data to insert

  RETURNS:

    NCS_PATRICIA_NODE *node - Node that matches the key search

  NOTES:

*****************************************************************************/
static NCS_PATRICIA_NODE *search(const NCS_PATRICIA_TREE *const pTree, const uint8_t *const key)
{
	NCS_PATRICIA_NODE *pNode;
	NCS_PATRICIA_NODE *pPrevNode;

	pNode = (NCS_PATRICIA_NODE *)&pTree->root_node;

	do {
		pPrevNode = pNode;

		if (m_GET_BIT(key, pNode->bit) == 0) {
			pNode = pNode->left;
		} else {
			pNode = pNode->right;
		}

	} while (pNode->bit > pPrevNode->bit);

	return pNode;
}

/****************************************************************************
 * KeyBitMatch:  compares 'n' bits of the keys.
 *               Returns 'TRUE' if they're equal.
 ****************************************************************************/
static int KeyBitMatch(const uint8_t *p1, const uint8_t *p2, unsigned int bitcount)
{
	while (bitcount > 8) {
		if (*p1 != *p2) {
			return (((int)*p1) - ((int)*p2));
		}
		p1++;
		p2++;
		bitcount -= 8;
	}

	return (((int)(*p1 & BitMasks[bitcount])) - ((int)(*p2 & BitMasks[bitcount])));

}

/****************************************************************************
 * PUBLIC FUNCTIONS
 ****************************************************************************/

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_init

  DESCRIPTION:

    Creates and initializes a Patricia Tree.  Given the parameter structure
    for key size,
  
  ARGUMENTS:

    NCS_PATRICIA_TREE *  ptr to tree to initialize.
    NCS_PATRICIA_PARMS * ptr to parameters

  RETURNS:

    NCSCC_RC_SUCCESS

  NOTES:

*****************************************************************************/

unsigned int ncs_patricia_tree_init(NCS_PATRICIA_TREE *const pTree, const NCS_PATRICIA_PARAMS *const pParams)
{
	if (pParams == NULL)
		return (unsigned int)m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	if ((pParams->key_size < 1)
	    || (pParams->key_size > NCS_PATRICIA_MAX_KEY_SIZE)
	    ) {
		return (unsigned int)m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	pTree->params = *pParams;

	/* Initialize the root node, which is actually part of the tree structure. */
	pTree->root_node.key_info = (uint8_t *)0;
	pTree->root_node.bit = -1;
	pTree->root_node.left = pTree->root_node.right = &pTree->root_node;
	if ((pTree->root_node.key_info = (NCS_PATRICIA_LEXICAL_STACK *)malloc(pTree->params.key_size)) == NULL) {
		return (unsigned int)m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	memset(pTree->root_node.key_info, '\0', (uint32_t)pTree->params.key_size);
	pTree->n_nodes = 0;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_destroy

  DESCRIPTION:

    Destroys and frees the entire Patrica tree, root node, table structure.
    This will totally clear all memory allocated by the tree.
  
  ARGUMENTS:

    NCS_PATRICIA_TREE * - Pointer to the table structure

  RETURNS:

    Nothing.

  NOTES:

*****************************************************************************/

unsigned int ncs_patricia_tree_destroy(NCS_PATRICIA_TREE *const pTree)
{
	ncs_patricia_tree_clear(pTree);
	free(pTree->root_node.key_info);
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_clear

  DESCRIPTION:

     Begins clearing the nodes of the tree.  This starts with the root node.
     The root only has elements on the left side, and doesn't need the right
     side to be cleared.  Clear walk is a recursive function to destroy 
     nodes in the tree.

  ARGUMENTS:

    NCS_PATRICIA_TREE *  - Pointer to the table structure

  RETURNS:

    Nothing.

  NOTES:

*****************************************************************************/

void ncs_patricia_tree_clear(NCS_PATRICIA_TREE *const pTree)
{

	pTree->root_node.left = pTree->root_node.right = &pTree->root_node;
	pTree->n_nodes = 0;

}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_add

  DESCRIPTION:

    Adds a new node to the tree and updates the index
      
  ARGUMENTS:

    NCS_PATRICIA_TREE *pTree: Patricia Tree table sturcture
    NCS_PATRICIA_NODE *pNode: Node to add.  Key indicated in it.

  RETURNS:

    int - NCSCC_RC_SUCCESS if key added
          NCSCC_RC_FAILURE key is duplicate, or other error.

  NOTES:

*****************************************************************************/

unsigned int ncs_patricia_tree_add(NCS_PATRICIA_TREE *const pTree, NCS_PATRICIA_NODE *const pNode)
{
	NCS_PATRICIA_NODE *pSrch;
	NCS_PATRICIA_NODE *pTmpNode;
	NCS_PATRICIA_NODE *pPrevNode;
	int bit;

	pTmpNode = search(pTree, pNode->key_info);
	if (m_KEY_CMP(pTree, pNode->key_info, pTmpNode->key_info) == 0) {
		return (NCSCC_RC_FAILURE);	/* duplicate!. */
	}

	bit = 0;

	while (m_GET_BIT(pNode->key_info, bit) == ((pTmpNode->bit < 0) ? 0 : m_GET_BIT(pTmpNode->key_info, bit))) {
		bit++;
	}

	pSrch = &pTree->root_node;

	do {
		pPrevNode = pSrch;
		if (m_GET_BIT(pNode->key_info, pSrch->bit) == 0)
			pSrch = pSrch->left;
		else
			pSrch = pSrch->right;
	} while ((pSrch->bit < bit) && (pSrch->bit > pPrevNode->bit));

	pNode->bit = bit;

	if (m_GET_BIT(pNode->key_info, bit) == 0) {
		pNode->left = pNode;
		pNode->right = pSrch;
	} else {
		pNode->left = pSrch;
		pNode->right = pNode;
	}

	if (m_GET_BIT(pNode->key_info, pPrevNode->bit) == 0) {
		pPrevNode->left = pNode;
	} else {
		pPrevNode->right = pNode;
	}

	pTree->n_nodes++;
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_del

  DESCRIPTION:

    Deletes the node (passed) from the patricia tree.
  
  ARGUMENTS:

    NCS_PATRICIA_TREE *  - the tree
    NCS_PATRICIA_NODE *  - the node to delete.

  RETURNS:

    NCSCC_RC_SUCCESS

  NOTES:

  it is the responsibility of the caller to insure that the node passed 
  is in the tree before calling this routine.

*****************************************************************************/

unsigned int ncs_patricia_tree_del(NCS_PATRICIA_TREE *const pTree, NCS_PATRICIA_NODE *const pNode)
{
	NCS_PATRICIA_NODE *pNextNode;
	NCS_PATRICIA_NODE **pLegDownToNode;
	NCS_PATRICIA_NODE *pDelNode;
	NCS_PATRICIA_NODE **pPrevLeg;
	NCS_PATRICIA_NODE **pNextLeg;
	int UpWentRight;

	UpWentRight = 0;

	/* Start left of root (there is no right). */
	pNextNode = &pTree->root_node;
	pLegDownToNode = &pNextNode->left;

	while ((pDelNode = *pLegDownToNode) != pNode) {
		if (pDelNode->bit <= pNextNode->bit) {
			return (unsigned int)m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);	/* Key not found. */
		}

		pNextNode = pDelNode;
		pLegDownToNode = ((m_GET_BIT(pNode->key_info, pNextNode->bit) != 0) ?
				  &pNextNode->right : &pNextNode->left);

	}

	/* pDelNode points to the one to delete.
	 * pLegDownToNode points to the down-pointer which points to it.
	 */

	pPrevLeg = pLegDownToNode;
	pNextNode = pNode;

	/* keep going 'down' until we find the one which 
	 * points back to pNode as an up-pointer. 
	 */

	while (1) {
		UpWentRight = (m_GET_BIT(pNode->key_info, pNextNode->bit) != 0);
		pNextLeg = ((UpWentRight) ? &pNextNode->right : &pNextNode->left);
		pDelNode = *pNextLeg;

		if (pDelNode == pNode)
			break;

		if (pDelNode->bit <= pNextNode->bit) {
			return (unsigned int)m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);	/* panic??? */
		}

		/* loop around again. */
		pNextNode = pDelNode;
		pPrevLeg = pNextLeg;
	}

	/* At this point, 
	 * pNextNode is the one pointing UP to the one to delete. 
	 * pPrevLeg points to the down-leg which points to pNextNode
	 * UpWentRight is the direction which pNextNode took (in the UP
	 *      direction) to get to the one to delete.)
	 */

	/* We need to rearrange the tree.
	 * BE CAREFUL.  The order of the following statements
	 * is critical.
	 */
	pNextNode->bit = pNode->bit;	/* it gets the 'bit' value of the evacuee. */
	*pLegDownToNode = pNextNode;

	*pPrevLeg = ((UpWentRight) ? pNextNode->left : pNextNode->right);
	pNextNode->right = pNode->right;
	pNextNode->left = pNode->left;

	pTree->n_nodes--;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_get

  DESCRIPTION:

    Get a node with a given key.  This routine will search the tree for the 
    only possible match, and then check the entire key for a perfect match.

  ARGUMENTS:

    NCS_PATRICIA_TREE *pTree: Pointer to the Patricia table
    uint8_t *key: Pointer to the key

  RETURNS:

    NCS_PATRICIA_NODE * - Pointer to the data structure with the given key

  NOTES:

*****************************************************************************/

NCS_PATRICIA_NODE *ncs_patricia_tree_get(const NCS_PATRICIA_TREE *const pTree, const uint8_t *const pKey)
{
	NCS_PATRICIA_NODE *pNode;

	/*
	 * See if last getNext happened to be same key.
	 *
	 * Important assumtion: lastNode will be set to NULL if any
	 * nodes are deleted from the tree.
	 */

	pNode = search(pTree, pKey);

	if ((pNode == &pTree->root_node) || (m_KEY_CMP(pTree, pNode->key_info, pKey) != 0)
	    ) {
		pNode = NCS_PATRICIA_NODE_NULL;
	}

	return pNode;
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_getnext

  DESCRIPTION:

    Gets the next greater lexical order key value.
       
  ARGUMENTS:

    NCS_PATRICIA_TREE *pTree: Pointer to the Patricia table
    uint8_t *key: Pointer to the key
           (NULL pointer requests 1st node in table)

  RETURNS:

    NCS_PATRICIA_NODE * - Pointer to the data structure with the given key

  NOTES:

*****************************************************************************/

NCS_PATRICIA_NODE *ncs_patricia_tree_getnext(NCS_PATRICIA_TREE *const pTree, const uint8_t *const pKey)
{
	uint8_t Target[NCS_PATRICIA_MAX_KEY_SIZE];
	NCS_PATRICIA_NODE *pSrch;
	NCS_PATRICIA_NODE *pPrev;
	register uint8_t *p1;
	register uint8_t *p2;
	register int bit;

	if (pKey == (const uint8_t *)0) {
		/* Start at root of tree. */
		memset(Target, '\0', pTree->params.key_size);
	} else {
		memcpy(Target, pKey, pTree->params.key_size);
	}

	p1 = Target + pTree->params.key_size - 1;	/* point to last byte of key */
	while (p1 >= Target) {
		*p1 += 1;
		if (*p1 != '\0') {
			break;
		}
		p1--;
	}
	if (p1 < Target) {
		return NCS_PATRICIA_NODE_NULL;
	}

	pSrch = &pTree->root_node;

	do {
		pPrev = pSrch;

		if (m_GET_BIT(Target, pSrch->bit) == 0) {
			pSrch = pSrch->left;
		} else {
			pSrch = pSrch->right;
		}

		if (pSrch->bit <= pPrev->bit) {
			if ((memcmp(Target, pSrch->key_info, pTree->params.key_size) <= 0) &&
			    (KeyBitMatch(Target, pSrch->key_info, 1 + pPrev->bit) == 0)
			    ) {
				return pSrch;
			}

			do {
				if (pSrch == pPrev->left) {
					/* We went left to get here */
					if (pPrev->bit < 0) {
						return NCS_PATRICIA_NODE_NULL;
					}
					pSrch = pPrev;

					p1 = pSrch->key_info;
					p2 = Target;

					for (bit = pSrch->bit; bit >= 8; bit -= 8) {
						*p2++ = *p1++;
					}
					/* Bring over SOME of the bits from pSrch. */
					*p2 = (uint8_t)(*p1 & ((uint8_t)(BitMasks[bit])));

					*p2 |= (uint8_t)(0x80 >> bit);

					p2++;

					while (p2 < (Target + pTree->params.key_size)) {
						*p2++ = '\0';
					}
					break;
				} else {
					/* We went right to get here */
					if (pPrev->bit <= 0) {
						return NCS_PATRICIA_NODE_NULL;
					}

					p1 = pPrev->key_info;
					p2 = Target;

					for (bit = pPrev->bit; bit >= 8; bit -= 8) {
						*p2++ = *p1++;
					}
					if (bit > 0) {
						*p2 = (uint8_t)(*p1 & BitMasks[bit]);
					}
					*p2 |= (uint8_t)(0xff >> bit);
					for (p1 = p2 + 1; p1 < (Target + pTree->params.key_size); p1++) {
						*p1 = '\0';
					}
					do {
						++*p2;
						if (*p2 != '\0') {
							break;
						}
					} while (--p2 >= Target);

					if (p2 < Target) {
						return NCS_PATRICIA_NODE_NULL;
					}

					pSrch = pPrev;

					pPrev = &pTree->root_node;
					do {
						if (m_GET_BIT(pSrch->key_info, pPrev->bit) == 0) {
							if (pPrev->left == pSrch) {
								break;
							}
							pPrev = pPrev->left;
						} else {
							if (pPrev->right == pSrch) {
								break;
							}
							pPrev = pPrev->right;
						}

					} while (true);

					if (KeyBitMatch(Target, pSrch->key_info, 1 + pSrch->bit) == 0) {
						break;
					}
				}

			} while (true);

		} /* if (pSrch->bit <= pPrev->bit) */
		else {
			/* We're still going 'down'... but make sure we haven't gone down too far. */
			bit = KeyBitMatch(Target, pSrch->key_info, pSrch->bit);

			if (bit < 0) {
				p1 = pSrch->key_info;
				p2 = Target;
				for (bit = pSrch->bit; bit >= 8; bit -= 8) {
					*p2++ = *p1++;
				}
				if (bit != 0) {
					*p2++ = ((uint8_t)(*p1 & BitMasks[bit]));
				}
				while (p2 < Target + pTree->params.key_size) {
					*p2++ = '\0';
				}
			} else if (bit > 0) {

				do {
					if (pSrch == pPrev->left) {
						/* We went left to get here */
						if (pPrev->bit < 0) {
							return NCS_PATRICIA_NODE_NULL;
						}
						pSrch = pPrev;

						p1 = pSrch->key_info;
						p2 = Target;

						for (bit = pSrch->bit; bit >= 8; bit -= 8) {
							*p2++ = *p1++;
						}
						/* Bring over SOME of the bits from pSrch. */
						*p2 = (uint8_t)(*p1 & ((uint8_t)(BitMasks[bit])));

						*p2 |= (uint8_t)(0x80 >> bit);

						p2++;

						while (p2 < (Target + pTree->params.key_size)) {
							*p2++ = '\0';
						}
						break;
					} else {
						/* We went right to get here */
						if (pPrev->bit <= 0) {
							return NCS_PATRICIA_NODE_NULL;
						}

						p1 = pPrev->key_info;
						p2 = Target;

						for (bit = pPrev->bit; bit >= 8; bit -= 8) {
							*p2++ = *p1++;
						}
						if (bit > 0) {
							*p2 = (uint8_t)(*p1 & BitMasks[bit]);
						}
						*p2 |= (uint8_t)(0xff >> bit);
						for (p1 = p2 + 1; p1 < (Target + pTree->params.key_size); p1++) {
							*p1 = '\0';
						}
						do {
							++*p2;
							if (*p2 != '\0') {
								break;
							}
						} while (--p2 >= Target);

						if (p2 < Target) {
							return NCS_PATRICIA_NODE_NULL;
						}

						pSrch = pPrev;

						pPrev = &pTree->root_node;
						do {
							if (m_GET_BIT(pSrch->key_info, pPrev->bit) == 0) {
								if (pPrev->left == pSrch) {
									break;
								}
								pPrev = pPrev->left;
							} else {
								if (pPrev->right == pSrch) {
									break;
								}
								pPrev = pPrev->right;
							}

						} while (true);

						if (KeyBitMatch(Target, pSrch->key_info, 1 + pSrch->bit) == 0) {
							break;
						}
					}

				} while (true);
			}
		}
	} while (true);
}

/*****************************************************************************

  DATA OBJECT NAME: ncs_patricia_tree_size

  DESCRIPTION:

    Return the size of the Patricia tree node

  ARGUMENTS:

    NCS_PATRICIA_TREE * - Pointer to the table structure

  RETURNS:

    int: Size of the node structure

  NOTES:

*****************************************************************************/

int ncs_patricia_tree_size(const NCS_PATRICIA_TREE *const pTree)
{
	return pTree->n_nodes;
}
