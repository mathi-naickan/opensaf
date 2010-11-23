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

  This module contains procedures for reserving and claiming USRBUF space. It
  hides the details of which USRBUF macro set is being used, and works with
  the struct NCS_UBAID, which is a collection of relevent USRBUF variables
  needed for easy/complete USRBUF manipulation.

..............................................................................

  FUNCTIONS INCLUDED in this module:

  This is C++ In C to the extent that there is a class NCS_UBAID, with public
  member functions as per the functions below. The 'this' pointer is always 
  the first argument in these functions.

  Encode functions

  ncs_enc_init_space.......get NCS_UBAID to start state
  ncs_enc_prime_space......get NCS_UBAID to start state w/passed USRBUF
  ncs_enc_reserve_space....reserve 'n' bytes of contiguous space
  ncs_enc_claim_space......claim 'm' of the 'n' bytes reserved (m <= n).
  ncs_enc_ttl_claimed......return total bytes claimed since start
  ncs_enc_append_usrbuf....Append a usrbuf to the end of NCS_UBAID data

  Decode functions

  ncs_dec_init_space.......get NCS_UBAID to start state;put USRBUF in place
  ncs_dec_flatten_space....contiguize count bytes
  ncs_dec_skip_space.......move consumed-data ptr forward
  ncs_dec_ttl_skipped......return total bytes skipped since start

  Simple assist functions

  ncs_set_max..............set max bytes involved in encode OR decode
  ncs_enc_can_i_put........An encoder wants to put n bytes; can he/she?
  ncs_dec_can_i_get........A decoder wants to extract n bytes, can he/she?

******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncs_ubaid.h"
#include "ncssysf_def.h"
#include "ncssysf_mem.h"
#include "ncsencdec_pub.h"
#include "ncsusrbuf.h"
#include "ncs_mds_def.h"

#define SIXTYFOUR_BYTES 64

/*****************************************************************************

                        ! !   W A R N I N G   ! !     

    There is an implicit limit on ncs_enc_reserve_space() in the case where 
    you reserve > PAYLOAD_BUF_SIZE bytes, it will only actually reserve 
    PAYLOAD_BUF_SIZE. 
    
    That is, it is impossible to reserve more bytes then there are in payload
    area of a single USRDATA, in the NetPlane implementation of USRBUFs.
    .
*****************************************************************************/

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_init_space

  DESCRIPTION:
 get all NCS_UBAID fields to start state.

  ARGUMENTS:
 uba:  NCS_UBAID to be initialized.

  RETURNS:
 NCSCC_RC_SUCCESS got space commitment
        NCSCC_RC_FAILURE space not reserved.

  NOTES:

*****************************************************************************/

int32 ncs_enc_init_space(NCS_UBAID *uba)
{
	if ((uba->start = m_MMGR_ALLOC_BUFR(sizeof(USRBUF))) == BNULL)
		return NCSCC_RC_FAILURE;

	uba->ub = uba->start;
	uba->res = 0;
	uba->ttl = 0;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_init_space_pp

  DESCRIPTION:
        Same as ncs_enc_init_space, except it takes the pool_id and 
        priority arguments that are forwarded down to the USRBUF 
        implementation.
        
        get all NCS_UBAID fields to start state.

  ARGUMENTS:
 uba:  NCS_UBAID to be initialized.
        pool_id:        which USRBUF pool_id should mem come from
        priority:       what is the 'urgency' of supplying this mem
  RETURNS:
 NCSCC_RC_SUCCESS got space commitment
        NCSCC_RC_FAILURE space not reserved.

  NOTES:
        Not available for old style USRBUFs.

*****************************************************************************/

int32 ncs_enc_init_space_pp(NCS_UBAID *uba, uns8 pool_id, uns8 priority)
{
	if ((uba->start = m_MMGR_ALLOC_POOLBUFR(pool_id, priority)) == BNULL)
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	uba->ub = uba->start;
	uba->res = 0;
	uba->ttl = 0;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_prime_space

  DESCRIPTION:
 get all NCS_UBAID fields to start state starting with the initial 
        USRBUF. In effect, this is another constructor.

  ARGUMENTS:
 uba:  NCS_UBAID to be initialized.

  RETURNS:
        void

  NOTES:

*****************************************************************************/
void ncs_enc_prime_space(NCS_UBAID *uba, USRBUF *ub)
{
	uba->start = ub;
	uba->ub = ub;
	uba->res = 0;
	uba->ttl = 0;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_append_usrbuf

  DESCRIPTION:
 append data from a usrbuf to the end of uba payload.

  ARGUMENTS:
 uba:  NCS_UBAID with relevent USRBUF info contained.
 ub:   USRBUF to be appended.

  RETURNS:

  NOTES:
      The UBA is expected to be fully encoded (with nothing "reserved").
      The total data length of ub will be added to uba->ttl.

*****************************************************************************/

void ncs_enc_append_usrbuf(NCS_UBAID *uba, USRBUF *ub)
{
	m_MMGR_APPEND_DATA(uba->ub, ub);
	uba->ttl += m_MMGR_LINK_DATA_LEN(ub);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_reserve_space

  DESCRIPTION:
 reserve contiguous space in a USRBUF using the appropriate macro set.

  ARGUMENTS:
 uba:  NCS_UBAID with relevent USRBUF info contained.
        res:            how much space to reserve.

  RETURNS:
 NULL            couldn't reserve it
        ptr             here is where to start putting stuff

  NOTES:
      If the space to reserve > 64 bytes, we constrain the reservation to
      only 64 bytes. 'Well behaved' IE encode routines understand this,
      and never try to encode more that 64 bytes anyway.

*****************************************************************************/

uns8 *ncs_enc_reserve_space(NCS_UBAID *uba, int32 res)
{
	if (res > PAYLOAD_BUF_SIZE) {	/* can never reserve > min payload of USRBUF */
		res = PAYLOAD_BUF_SIZE;
	}

	uba->res = res;

	/* HARRY FENG NOTE:
	 ** Most callers of this function don't check NULL pointer (see bug #3349).
	 ** As a patch, we post an out-of-memory message so that 
	 ** there is some indication as to where the crash came from.
	 */
	uba->bufp = m_MMGR_RESERVE_AT_END(&(uba->ub), (uns32)res, uns8 *);

#ifdef _DEBUG
	if (uba->bufp == NULL)
		printf("\nOut of memory in ncs_enc_reserve_space()!");
#endif

	return uba->bufp;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_claim_space

  DESCRIPTION:
 claim contiguous space just reserved by ncs_reserve_space(). The 
        amount claimed must be <= the amount reserved.

  ARGUMENTS:
 uba:  NCS_UBAID with relevent USRBUF info contained.
        used:           how much space was used (is claimed).

  RETURNS:

  NOTES:

*****************************************************************************/

void ncs_enc_claim_space(NCS_UBAID *uba, int32 used)
{
	uba->ttl = (uba->ttl + used);  /* keep our grand total as uns32 */

	/* Free up unused portion which was reserved  */
	m_MMGR_REMOVE_FROM_END(uba->ub, (uns32)(uba->res - used));

	uba->res = 0;		/* wipe out notion that space was reserved... */
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_dec_init_space

  DESCRIPTION:
 get all NCS_UBAID fields to start state, find length of passed USRBUF
        and install in max field. Finally, place USRBUF to be consumed in 
        position (uba->ub).

  ARGUMENTS:
 uba:  NCS_UBAID to be initialized.
        ub:             USRBUG we are preparing to read/consume

  RETURNS:
        void

  NOTES:

*****************************************************************************/
void ncs_dec_init_space(NCS_UBAID *uba, USRBUF *ub)
{
	uba->start = 0;
	uba->ub = ub;
	uba->res = 0;
	uba->ttl = 0;
	uba->max = m_MMGR_LINK_DATA_LEN(ub);	/* find max amount of data as of now */
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_dec_flatten_space

  DESCRIPTION:
 contiguize the next count bytes from the current user_buf and return
        the string of count bytes.

  ARGUMENTS:
 uba:  NCS_UBAID with current state stuff
        os:             String space that holds up to count bytes
        count           number of bytes to contiguize

  RETURNS:
        ptr             a string holding count bytes
        null            no more bytes left

  NOTES:

        If you ask for n bytes but there are only < N (but more than 0), it
        returns a ptr to a string.

*****************************************************************************/

uns8 *ncs_dec_flatten_space(NCS_UBAID *uba, uns8 *os, int32 count)
{
	if (uba->ub == BNULL) {
		m_LEAP_DBG_SINK(NULL);
		return NULL;
	}

	return (uns8 *)m_MMGR_DATA_AT_START(uba->ub, (uns32)count, (char *)os);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_dec_skip_space

  DESCRIPTION:
 move the consumed data buffer pointer up by count bytes. Now we are
        up to where we should continue to decode from.

  ARGUMENTS:
 uba:  NCS_UBAID with current state stuff
        used:           amount of data actually consumed (vs amount flattened)

  RETURNS:

  NOTES:

*****************************************************************************/

void ncs_dec_skip_space(NCS_UBAID *uba, int32 count)
{
	m_MMGR_REMOVE_FROM_START(&uba->ub, (uns32)count);
	uba->ttl = uba->ttl + count;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_set_max

  DESCRIPTION:

        This one API can be used for both encode and decode cases.
 move the consumed data buffer pointer up by count bytes. Now we are
        up to where we should continue to decode from.

  ARGUMENTS:
 uba:  NCS_UBAID with current state stuff
        used:           amount of data actually consumed (vs amount flattened)

  RETURNS:

  NOTES:

*****************************************************************************/

void ncs_set_max(NCS_UBAID *uba, int32 max)
{
	uba->max = max;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_enc_can_i_put

  DESCRIPTION:

        We are keeping a running total of what we have encoded so far.
        ncs_se_max() sets the max field to the largest #of bytes that are
        allowed to be encoded in a single USERBUF chain.

        this function simply checks to se that
          to_put  - the amount of data invoker wants to encode
        + ttl     - the amount of data encoded so far in this USRBUF chain
        -----------
          <total> - This cannot be > max, the most data we can encode

  ARGUMENTS:
 to_put       the amount of data the invoker wants to place in USRBUF

  RETURNS:
        TRUE         it will fit
        FALSE        This exceeds the pre-set threshold

  NOTES:

        If the client environment can support any size, don't ask (dont invoke this
        API) OR set ncs_set_max(0xffff)).

*****************************************************************************/

NCS_BOOL ncs_enc_can_i_put(NCS_UBAID *uba, int32 to_put)
{
	if ((uba->ttl + to_put) <= uba->max)
		return (TRUE);
	else
		return (FALSE);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_dec_can_i_get

  DESCRIPTION:

        We are keeping a running total of what we have decoded so far.
        ncs_se_max() sets the max field to the #of bytes available for 
        consumption. 

        this function simply checks to se that
          ttl     - the amount of data decoded so far
        + to_get  - the amount of data we now want to consume
        -----------
          <total> - This cannot be > max, the most data we can consume

  ARGUMENTS:
 to_get      the amount of data the invoker wants to consume next

  RETURNS:
        TRUE         it is available
        FALSE        This exceeds the pre-set threshold of consumable data

  NOTES:

*****************************************************************************/

NCS_BOOL ncs_dec_can_i_get(NCS_UBAID *uba, int32 to_get)
{
	if ((uba->ttl + to_get) <= uba->max)
		return (TRUE);
	else
		return (FALSE);
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_reset_uba

  DESCRIPTION:

        This API can be used for both encode and decode cases.
        It gets all NCS_UBAID fields to start state and frees any usrbuf
        attached to it.

  ARGUMENTS:
 uba:  NCS_UBAID with current state stuff

  RETURNS:

  NOTES:

*****************************************************************************/

void ncs_reset_uba(NCS_UBAID *uba)
{
	if (uba->ub != NULL)
		m_MMGR_FREE_BUFR_LIST(uba->ub);

	uba->start = NULL;
	uba->ub = NULL;
	uba->res = 0;
	uba->ttl = 0;
	uba->max = 0;
}

/***************************************************************\
      mds_encode_mds_dest: Encodes an MDS_DEST into a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function. 

      mds_decode_mds_dest: Deodes an MDS_DEST from a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function. 

\***************************************************************/
USRBUF *mds_encode_mds_dest(USRBUF *i_ub, MDS_DEST *i_mds_dest)
{
	uns8 *p;

	if ((p = m_MMGR_RESERVE_AT_END(&i_ub, 8, uns8 *)) != (uns8 *)0) {
		ncs_encode_64bit(&p, *i_mds_dest);
		return i_ub;
	} else
		return NULL;
}

USRBUF *mds_decode_mds_dest(USRBUF *i_ub, MDS_DEST *o_mds_dest)
{
	char buff[8];
	uns8 *s;

	s = (uns8 *)m_MMGR_DATA_AT_START(i_ub, 8, buff);
	*o_mds_dest = ncs_decode_64bit(&s);
	m_MMGR_REMOVE_FROM_START(&i_ub, 8);

	return i_ub;		/* Return the new head */
}

/***************************************************************\
      mds_uba_encode_mds_dest: Encodes an MDS_DEST into a UBA.
                               Returns either NCSCC_RC_SUCCESS 
                               or NCSCC_RC_FAILURE.

      mds_uba_decode_mds_dest: Deodes an MDS_DEST from a UBA. 
                               Returns either NCSCC_RC_SUCCESS 
                               or NCSCC_RC_FAILURE.

\***************************************************************/
uns32 mds_uba_encode_mds_dest(NCS_UBAID *uba, MDS_DEST *i_mds_dest)
{
	uns8 *p;

	p = ncs_enc_reserve_space(uba, 8);
	ncs_encode_64bit(&p, *i_mds_dest);
	ncs_enc_claim_space(uba, 8);
	return NCSCC_RC_SUCCESS;
}

uns32 mds_uba_decode_mds_dest(NCS_UBAID *uba, MDS_DEST *o_mds_dest)
{
	uns8 buff[8];
	uns8 *s;

	s = ncs_dec_flatten_space(uba, buff, 8);
	*o_mds_dest = ncs_decode_64bit(&s);
	ncs_dec_skip_space(uba, 8);

	return NCSCC_RC_SUCCESS;
}

uns32 mds_st_encode_mds_dest(uns8 **stream, MDS_DEST *idest)
{
	if ((stream == NULL) || (*stream == NULL) || (idest == NULL))
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	ncs_encode_64bit(stream, *idest);
	return NCSCC_RC_SUCCESS;
}

uns32 mds_st_decode_mds_dest(uns8 **stream, MDS_DEST *odest)
{
	if ((stream == NULL) || (*stream == NULL) || (odest == NULL))
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

	*odest = ncs_decode_64bit(stream);
	return NCSCC_RC_SUCCESS;
}

/***************************************************************\
      ncs_encode_pointer: Encodes a pointer into a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function (NULL if failure)
 
      ncs_decode_pointer: Deodes a pointer  from a USRBUF. The
                           modified USRBUF pointer is returned
                           by this function ((NULL if failure)
 
\***************************************************************/

USRBUF *ncs_encode_pointer(USRBUF *i_ub, NCSCONTEXT i_pointer)
{
	uns8 *p;
	uns16 p_len;

	p_len = sizeof(NCSCONTEXT);
	if ((p = m_MMGR_RESERVE_AT_END(&i_ub, (int32)(p_len + sizeof(uns8)), uns8 *)) != (uns8 *)0) {
		ncs_encode_8bit(&p, p_len);
		if (p_len == sizeof(uns32))
			ncs_encode_32bit(&p, NCS_PTR_TO_INT32_CAST(i_pointer));
		else
			ncs_encode_64bit(&p, NCS_PTR_TO_UNS64_CAST(i_pointer));
		return i_ub;
	} else
		return NULL;
}

USRBUF *ncs_decode_pointer(USRBUF *i_ub, uns64 *o_recvd_ptr, uns8 *o_ptr_size_in_bytes)
{
	uns8 *s;
	uns8 p_len;

	*o_recvd_ptr = 0;
	*o_ptr_size_in_bytes = 0;

	s = (uns8 *)m_MMGR_DATA_AT_START(i_ub, (int32)(sizeof(uns8)), (char *)o_ptr_size_in_bytes);
	p_len = ncs_decode_8bit(&s);
	*o_ptr_size_in_bytes = p_len;
	m_MMGR_REMOVE_FROM_START(&i_ub, sizeof(uns8));

	s = (uns8 *)m_MMGR_DATA_AT_START(i_ub, (int32)p_len, (char *)o_recvd_ptr);

	if (p_len == sizeof(uns32))
		*o_recvd_ptr = ncs_decode_32bit(&s);
	else {
		if (p_len != sizeof(NCSCONTEXT))
			m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
		*o_recvd_ptr = ncs_decode_64bit(&s);
	}

	m_MMGR_REMOVE_FROM_START(&i_ub, p_len);

	return i_ub;		/* Return the new head */
}

/***************************************************************\
      ncs_uba_encode_pointer: Encodes a pointer  into a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.
 
      ncs_uba_decode_pointer: Deodes a pointer from a UBA.
                               Returns either NCSCC_RC_SUCCESS
                               or NCSCC_RC_FAILURE.
 
\***************************************************************/
uns32 ncs_uba_encode_pointer(NCS_UBAID *uba, NCSCONTEXT i_pointer)
{
	uns8 *p;
	uns8 p_len;

	p_len = sizeof(NCSCONTEXT);
	p = ncs_enc_reserve_space(uba, (int32)(p_len + sizeof(uns8)));

	ncs_encode_8bit(&p, p_len);
	if (p_len == sizeof(uns32))
		ncs_encode_32bit(&p, NCS_PTR_TO_INT32_CAST(i_pointer));
	else
		ncs_encode_64bit(&p, NCS_PTR_TO_UNS64_CAST(i_pointer));

	ncs_enc_claim_space(uba, (int32)(p_len + sizeof(uns8)));
	return NCSCC_RC_SUCCESS;
}

uns32 ncs_uba_decode_pointer(NCS_UBAID *uba, uns64 *o_recvd_ptr, uns8 *o_ptr_size_in_bytes)
{
	uns8 *s;
	uns8 p_len;

	*o_recvd_ptr = 0;
	*o_ptr_size_in_bytes = 0;
	s = ncs_dec_flatten_space(uba, (uns8 *)o_ptr_size_in_bytes, (int32)(sizeof(uns8)));
	p_len = ncs_decode_8bit(&s);
	ncs_dec_skip_space(uba, (int32)(sizeof(uns8)));

	*o_ptr_size_in_bytes = p_len;
	s = ncs_dec_flatten_space(uba, (uns8 *)o_recvd_ptr, (int32)p_len);
	if (p_len == sizeof(uns32))
		*o_recvd_ptr = ncs_decode_32bit(&s);
	else {
		if (p_len != sizeof(NCSCONTEXT))
			m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);

		*o_recvd_ptr = ncs_decode_64bit(&s);
	}

	ncs_dec_skip_space(uba, (int32)(p_len));

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_encode_n_octets_in_uba

  DESCRIPTION:

        This API can be used for encoding "count" octets(but, assumed non-zero)
        into uba(NCS_UBAID), without the hassles of having to call
        "reserve"/"claim" APIs.

  ARGUMENTS:
  uba:      NCS_UBAID with current state stuff
  os :      Source pointer
  count:    Number of octets to be encoded

  RETURNS:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES:

*****************************************************************************/
uns32 ncs_encode_n_octets_in_uba(NCS_UBAID *uba, uns8 *os, unsigned int count)
{
	uns8 *p;
	uns32 remaining;
	uns32 try_put;

	if (uba->ub == NULL) {
		/* Operation fail */
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
	if (uba->start == NULL) {
		/* We perform "ncs_enc_init_space" operation on the uba now. */
		if (ncs_enc_init_space(uba) != NCSCC_RC_SUCCESS)
			return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	for (remaining = count; remaining > 0; remaining -= try_put) {
		/* The no of bytes to be put in the current payload has
		   to be the least of
		   (1) Tail-room available (if any)
		   (2) Max bytes per payload
		   (3) Remaining bytes

		   NOTE: tail-room is a best effort attempt. It is not always
		   guaranteed that any tail-room available can be used (because
		   if the payload->RefCnt > 1, then this payload tail-room
		   cannot be used) And it is pretty useless, when the user
		   needs more than PAYLOAD_BUF_SIZE bytes anyway. 
		 */
		try_put = remaining;
		p = m_MMGR_RESERVE_AT_END_AMAP(&(uba->ub), &try_put, uns8 *);	/* Total=FALSE, i.e. only as much as possible */
		if (p != NULL) {
			/*
			 * Build the octet string...Remember a NULL pointer
			 * indicates an all-zero octet-string...
			 */
			if (os == (uns8 *)0)
				memset((char *)p, '\0', try_put);
			else
				memcpy((char *)p, (char *)(os + count - remaining), try_put);
		} else {
			return NCSCC_RC_FAILURE;
		}
	}
	uba->ttl += count;

	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

  PROCEDURE NAME:    ncs_decode_n_octets_from_uba

  DESCRIPTION:

        This API can be used for decoding "count" octets(but, assumed non-zero)
        into uba(NCS_UBAID), without the hassles of having to call
        "flatten"/"skip" APIs.

  ARGUMENTS:
  uba:      NCS_UBAID with current state stuff
  os :      Target pointer
  count:    Number of octets to be decoded

  RETURNS:  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  NOTES:

*****************************************************************************/
uns32 ncs_decode_n_octets_from_uba(NCS_UBAID *uba, uns8 *os, unsigned int count)
{
	uns8 *p8;

	if (uba->start != NULL) {
		/* We perform "ncs_dec_init_space" operation on the uba now. */
		ncs_dec_init_space(uba, uba->start);
	} else if (uba->ub == NULL) {
		/* If both "start" and "ub" are NULL, this is fatal error. */
		return NCSCC_RC_FAILURE;
	}

	p8 = ncs_dec_flatten_space(uba, os, count);
	if (p8 != os) {
		/* If "uba->ub" has the required data, it returns the offset
		   into its "uba->ub" where the data starts. */
		memcpy(os, p8, count);
	} else {
		/* Required data is already available in "os".
		   How??? Because, ncs_dec_flatten_space( ) takes care
		   of populating the "os" segment with data that was spread
		   across "uba->ub" and its "link" USRBUFs. 
		 */
	}
	ncs_dec_skip_space(uba, count);

	return NCSCC_RC_SUCCESS;
}
