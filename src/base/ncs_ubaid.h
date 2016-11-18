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

  This module contains declarations that facilitate USRBUF usage. These uses
  were needed and developed in the context of SSS IE Encoding, though these
  services may well serve in many other contexts.

  NOTE:
   This include and its corresponding ubuf_aid.c are put 'above' the sig
   directory since these services are not particular to any one sub-system.

   ******************************************************************************
   */

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCS_UBAID_H_
#define BASE_NCS_UBAID_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/ncsusrbuf.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  STRUCTURES, UNIONS and Derived Types

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/* NCS_UBAID is a collection of variables that are grouped for the benefit   */
/* of easy USRBUF management. It maintains the start as well as current     */
/* USRBUF. It also maintains where in the payload we are currently working  */
/* from, and how much space was reserved...                                 */

typedef struct ncs_ubaid {
  /* ENC functions           DEC functions                 */
  /*=======================+===============================*/
  USRBUF *start;  /* first usrbuf to fill    Not used                      */
  USRBUF *ub;     /* current usrbuf to fill  current usrbuf to consume     */
  uint8_t *bufp;  /* inject info here        Not Used                      */
  int32_t res;    /* space reserved          Not Used                      */
  int32_t ttl;    /* total space claimed     total space consumed          */
  int32_t max;    /* max we can encode       max we can decode             */
} NCS_UBAID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  Prototypes

  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/* Encode assist (member) functions */

int32_t ncs_enc_init_space(NCS_UBAID *uba);
int32_t ncs_enc_init_space_pp(NCS_UBAID *uba, uint8_t pool_id, uint8_t prio);
void ncs_enc_prime_space(NCS_UBAID *uba, USRBUF *ub);
uint8_t *ncs_enc_reserve_space(NCS_UBAID *uba, int32_t res);
void ncs_enc_claim_space(NCS_UBAID *uba, int32_t used);
void ncs_enc_append_usrbuf(NCS_UBAID *uba, USRBUF *ub);
uint32_t ncs_encode_n_octets_in_uba(NCS_UBAID *uba, uint8_t *os, unsigned int count);
uint32_t ncs_decode_n_octets_from_uba(NCS_UBAID *uba, uint8_t *os, unsigned int count);

/* Decode assist (member) functions */

void ncs_dec_init_space(NCS_UBAID *uba, USRBUF *ub);
uint8_t *ncs_dec_flatten_space(NCS_UBAID *uba, uint8_t *os, int32_t count);
void ncs_dec_skip_space(NCS_UBAID *uba, int32_t used);

/* These are used if the NCS_UBAID is used to track 'fit' issues */

void ncs_set_max(NCS_UBAID *uba, int32_t max);
bool ncs_enc_can_i_put(NCS_UBAID *uba, int32_t to_put);
bool ncs_dec_can_i_get(NCS_UBAID *uba, int32_t to_get);
void ncs_reset_uba(NCS_UBAID *uba);

/***************************************************************\
      mds_encode_mds_dest: Encodes an MDS_DEST into a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function (NULL if failure)

      mds_encode_mds_dest: Deodes an MDS_DEST from a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function ((NULL if failure)

\***************************************************************/
USRBUF *mds_encode_mds_dest(USRBUF *i_ub, MDS_DEST *i_mds_dest);
USRBUF *mds_decode_mds_dest(USRBUF *i_ub, MDS_DEST *o_mds_dest);

/***************************************************************\
      mds_uba_encode_mds_dest: Encodes an MDS_DEST into a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.

      mds_uba_decode_mds_dest: Deodes an MDS_DEST from a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.

\***************************************************************/
uint32_t mds_uba_encode_mds_dest(NCS_UBAID *uba, MDS_DEST *i_mds_dest);
uint32_t mds_uba_decode_mds_dest(NCS_UBAID *uba, MDS_DEST *o_mds_dest);

uint32_t mds_st_encode_mds_dest(uint8_t **stream, MDS_DEST *idest);
uint32_t mds_st_decode_mds_dest(uint8_t **stream, MDS_DEST *odest);

/***************************************************************\
      ncs_encode_pointer: Encodes a pointer into a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function (NULL if failure)

      ncs_decode_pointer: Deodes a pointer  from a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function ((NULL if failure)

\***************************************************************/
USRBUF *ncs_encode_pointer(USRBUF *i_ub, NCSCONTEXT i_pointer);
USRBUF *ncs_decode_pointer(USRBUF *i_ub, uint64_t *o_recvd_ptr, uint8_t *o_ptr_size_in_bytes);

/***************************************************************\
      ncs_uba_encode_pointer: Encodes a pointer  into a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.

      ncs_uba_decode_pointer: Deodes a pointer from a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.

\***************************************************************/
uint32_t ncs_uba_encode_pointer(NCS_UBAID *uba, NCSCONTEXT i_pointer);
uint32_t ncs_uba_decode_pointer(NCS_UBAID *uba, uint64_t *o_recvd_ptr,
                                uint8_t *o_ptr_size_in_bytes);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_UBAID_H_
