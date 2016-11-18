/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008-2014 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power, Ericsson
 *
 */

/*****************************************************************************
..............................................................................

  MODULE NAME:  NCS_ENDEC.H

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  07-22-97 1.00A    H&J (NSG)     Original

  *
  * 14    4/04/01 10:36a Fengh
  * float to float
  *
  * 13    10/24/00 11:53a Pseverin
  *
  * 12    8/02/00 5:20p Stevem
  * Added encode/decode of NCS_KEY stuff.
  *
  * 11    12/15/99 12:16p Ptutlian
  * remove warnings for the vxworks compiler
  *
  * 10    12/02/99 5:28p Saula
  * Merge RSVP
  *
  * 12    12/02/99 5:13p Saula
  * Merge from main line
  *
  * 7     9/10/98 3:26p Nidhi
  *
  * 5     9/03/98 5:56p Stevem
  *
  * 4     8/13/98 6:13p Kchin
  *
  * 1     6/03/98 3:15p Daha
  * New files for LEC 2.0 and MPC 1.0.
  *
  * 9     5/21/98 2:54p Nsg
  * changes to compile using gcc without warnings...
  *
  * 8     5/11/98 6:26p Daha
  * Changes to make gcc compile warning free.
  *
  * 7     12/10/97 2:43p Bfox
  * Added ncs_cpy_mac_addr()
  *
  * 6     12/02/97 10:20p Bfox
  * Added prototype for ncs_make_addr_from_string()
  *
  * 5     11/20/97 7:39a Billh
  *
  * 4     10/28/97 3:55p Billh
  *
  * 3     10/24/97 8:16a Billh
  *
  * 2     10/24/97 7:38a Billh
  * Newly added  common include files.
  *
  * 1     10/24/97 7:37a Billh

..............................................................................

  DESCRIPTION:
  Function prototypes for utility encode/decode operations.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */
#ifndef BASE_NCSENCDEC_PUB_H_
#define BASE_NCSENCDEC_PUB_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "osaf/saf/saAis.h"
#include "clm/saf/saClm.h"
#include "base/ncs_ubaid.h"
#include "base/ncsusrbuf.h"
#include "base/ncs_svd.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * NCS_ENC encode functions
 */
USRBUF *ncs_encode_n_octets(USRBUF *, uint8_t *, unsigned int);
USRBUF *ncs_encode_uns8(USRBUF *u, uint8_t val8);
USRBUF *ncs_encode_uns16(USRBUF *u, uint16_t val16);
USRBUF *ncs_encode_uns32(USRBUF *u, uint32_t val32);
USRBUF *ncs_encode_uns64(USRBUF *u, uint64_t val64);
USRBUF *ncs_prepend_n_octets(USRBUF *pbuf, uint8_t *os, unsigned int);
USRBUF *ncs_prepend_uns16(USRBUF *u, uint16_t);
USRBUF *ncs_prepend_uns32(USRBUF *u, uint32_t);
USRBUF *ncs_prepend_uns64(USRBUF *u, uint64_t);
USRBUF *ncs_encode_float(USRBUF *u, float obj_val);

uint64_t ncs_encode_64bit(uint8_t **stream, uint64_t);
uint32_t ncs_encode_32bit(uint8_t **stream, uint32_t);
uint32_t ncs_encode_24bit(uint8_t **stream, uint32_t);
uint32_t ncs_encode_16bit(uint8_t **stream, uint32_t);
uint32_t ncs_encode_8bit(uint8_t **stream, uint32_t);
uint32_t ncs_encode_key(uint8_t **stream, NCS_KEY *key);
uint32_t ncs_encode_octets(uint8_t **stream, uint8_t *val, uint32_t count);

/*
 * NCS_DEC decode functions
 */
USRBUF *ncs_decode_n_octets(USRBUF *, uint8_t *, uint32_t);

USRBUF *ncs_skip_n_octets(USRBUF *, uint32_t);
uint8_t *ncs_flatten_n_octets(USRBUF *u, uint8_t *os, uint32_t count);

uint32_t ncs_decode_short(uint8_t **stream);
uint32_t ncs_decode_24bit(uint8_t **stream);
uint32_t ncs_decode_32bit(uint8_t **stream);
uint16_t ncs_decode_16bit(uint8_t **stream);
uint64_t ncs_decode_64bit(uint8_t **stream);
uint8_t ncs_decode_8bit(uint8_t **stream);
uint32_t ncs_decode_key(uint8_t **stream, NCS_KEY *key);
float ncs_decode_float(uint8_t **stream);

/***** new style (2014) encoding/decoding functions follows ******/
void osaf_encode_uint8(NCS_UBAID *ub, uint8_t value);
void osaf_decode_uint8(NCS_UBAID *ub, uint8_t *to);
void osaf_encode_uint16(NCS_UBAID *ub, uint16_t value);
void osaf_decode_uint16(NCS_UBAID *ub, uint16_t *to);
void osaf_encode_uint32(NCS_UBAID *ub, uint32_t value);
void osaf_decode_uint32(NCS_UBAID *ub, uint32_t *to);
void osaf_decode_int(NCS_UBAID *ub, int *to);
void osaf_encode_uint64(NCS_UBAID *ub, uint64_t value);
void osaf_decode_uint64(NCS_UBAID *ub, uint64_t *to);
void osaf_encode_sanamet(NCS_UBAID *ub, const SaNameT *name);
void osaf_decode_sanamet(NCS_UBAID *ub, SaNameT *name);
void osaf_encode_sanamet_o2(NCS_UBAID *ub, SaConstStringT name);
void osaf_encode_saconststring(NCS_UBAID *ub, SaConstStringT str);
void osaf_encode_satimet(NCS_UBAID *ub, SaTimeT time);
void osaf_decode_satimet(NCS_UBAID *ub, SaTimeT *time);
void osaf_encode_bool(NCS_UBAID *ub, bool value);
void osaf_decode_bool(NCS_UBAID *ub, bool *to);
void osaf_encode_saclmnodeaddresst(NCS_UBAID *ub, const SaClmNodeAddressT *addr);
void osaf_decode_saclmnodeaddresst(NCS_UBAID *ub, SaClmNodeAddressT *addr);

/* encode float */
#define m_NCS_ENCODE_FLOAT(f, enc) {                    \
    *((uint32_t *)(enc)) = htonl(*((uint32_t *)&(f)));  \
  }

/* decode float */
#define m_NCS_DECODE_FLOAT(n, dec) {            \
    *((uint32_t *) (dec)) = (n);                \
  }

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCSENCDEC_PUB_H_
