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

  MODULE NAME: NCS_ENC.C  

  REVISION HISTORY:

  Date     Version  Name          Description
  -------- -------  ------------  --------------------------------------------
  10-15-97 1.00A    H&J (DS)      Original

  $Id: 

  $Log:

..............................................................................

  DESCRIPTION:

  This module contains useful encoding function used during construction of
  LUNI 1.0 control frames.

..............................................................................

  FUNCTIONS INCLUDED in this module:

The following set does buffer chaining management
  ncs_encode_n_octets....Encode "n" octets in the control frame
  ncs_encode_uint16_t   ....Encode short
  ncs_encode_uint32_t   ....Encode long
  ncs_encode_uint64_t   ....Encode long long
  ncs_prepend_n_octets...Encode "n" octets encapsulating given frame
  ncs_prepend_uint16_t   ...Encode 16 bit unsigned encapsulating given frame
  ncs_prepend_uint32_t   ...Encode 32 bit unsigned encapsulating given frame
  ncs_prepend_uint64_t   ...Encode 64 bit unsigned encapsulating given frame

The following set does NOT do buffer chaining management
  ncs_encode_64bit   ....Encode 64 bits of a 64 bit value
  ncs_encode_32bit   ....Encode 32 bits of a 32 bit value
  ncs_encode_24bit   ....Encode 24 least significant octets of a 32 bit value
  ncs_encode_16bit   ....Encode 16 least significant octets of a 32 bit value
  ncs_encode_8bit    ....Encode  8 least significant octets of a 32 bit value
  ncs_encode_octets  ....Encode "n" octets encapsulating given frame

*******************************************************************************
*/

#include <ncsgl_defs.h>
#include "ncs_osprm.h"
#include "ncsencdec_pub.h"
#include "ncssysf_def.h"
#include "ncssysf_mem.h"
#include "ncs_svd.h"

/** A NULL os implies "count" number of zeros...
 **/

USRBUF *ncs_encode_n_octets(USRBUF *u, uint8_t *os, unsigned int count)
{
	uint8_t *p;
	unsigned int remaining = count;
	unsigned int offset = 0;

	if (remaining > PAYLOAD_BUF_SIZE)
		count = PAYLOAD_BUF_SIZE;
	else
		count = remaining;

	do {
		if ((p = m_MMGR_RESERVE_AT_END(&u, count, uint8_t *)) != (uint8_t *)0) {
			/*
			 * Build the octet string...Remember a NULL pointer
			 * indicates an all-zero octet-string...
			 */
			if (os == (uint8_t *)0)
				memset((char *)p, '\0', (size_t)count);
			else
				memcpy((char *)p, (char *)(os + offset), (size_t)count);
		} else {
			break;
		}
		remaining = remaining - count;
		offset = offset + count;

		if (remaining > PAYLOAD_BUF_SIZE)
			count = PAYLOAD_BUF_SIZE;
		else
			count = remaining;

	} while (remaining > 0);
	return u;
}

USRBUF *ncs_encode_uns8(USRBUF *u, uint8_t val8)
{
	uint8_t *p8;

	if ((p8 = m_MMGR_RESERVE_AT_END(&u, 1, uint8_t *)) != (uint8_t *)0)
		*p8 = val8;

	return u;
}

USRBUF *ncs_encode_uns16(USRBUF *u, uint16_t val16)
{
	uint8_t *p16;

	if ((p16 = m_MMGR_RESERVE_AT_END(&u, 2, uint8_t *)) != (uint8_t *)0) {
		*p16++ = (uint8_t)(val16 >> 8);
		*p16 = (uint8_t)(val16);
	}

	return u;
}

USRBUF *ncs_encode_uns32(USRBUF *u, uint32_t val32)
{
	uint8_t *p32;

	if ((p32 = m_MMGR_RESERVE_AT_END(&u, 4, uint8_t *)) != (uint8_t *)0) {
		*p32++ = (uint8_t)(val32 >> 24);
		*p32++ = (uint8_t)(val32 >> 16);
		*p32++ = (uint8_t)(val32 >> 8);
		*p32 = (uint8_t)(val32);
	}

	return u;
}

USRBUF *ncs_encode_uns64(USRBUF *u, uint64_t val64)
{
	uint8_t *p64;

	if ((p64 = m_MMGR_RESERVE_AT_END(&u, 8, uint8_t *)) != (uint8_t *)0) {
		m_NCS_OS_HTONLL_P(p64, val64);
	}

	return u;
}

USRBUF *ncs_encode_float(USRBUF *u, float obj_val)
{
	float *pfloat;

	if ((pfloat = m_MMGR_RESERVE_AT_END(&u, (uint32_t)sizeof(obj_val), float *)) != (float *)0)
		 m_NCS_ENCODE_FLOAT(obj_val, pfloat);

	return u;
}

USRBUF *ncs_prepend_n_octets(USRBUF *pbuf, uint8_t *os, unsigned int length)
{
	uint8_t *pch;

	pch = m_MMGR_RESERVE_AT_START(&pbuf, length, uint8_t *);
	if (pch == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return BNULL;
	}

	memcpy((char *)pch, (char *)os, (size_t)length);
	return (pbuf);
}

USRBUF *ncs_prepend_uns16(USRBUF *pbuf, uint16_t val16)
{
	uint8_t *p16;

	p16 = m_MMGR_RESERVE_AT_START(&pbuf, (uint32_t)sizeof(uint16_t), uint8_t *);
	if (p16 == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return BNULL;
	}
	*p16++ = (uint8_t)(val16 >> 8);
	*p16 = (uint8_t)(val16);

	return (pbuf);
}

USRBUF *ncs_prepend_uns32(USRBUF *pbuf, uint32_t val32)
{
	uint8_t *p32;

	p32 = m_MMGR_RESERVE_AT_START(&pbuf, (uint32_t)sizeof(uint32_t), uint8_t *);
	if (p32 == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return BNULL;
	}
	*p32++ = (uint8_t)(val32 >> 24);
	*p32++ = (uint8_t)(val32 >> 16);
	*p32++ = (uint8_t)(val32 >> 8);
	*p32 = (uint8_t)(val32);

	return (pbuf);
}

USRBUF *ncs_prepend_uns64(USRBUF *pbuf, uint64_t val64)
{
	uint8_t *p64;

	p64 = m_MMGR_RESERVE_AT_START(&pbuf, 8, uint8_t *);
	if (p64 == NULL) {
		m_LEAP_DBG_SINK_VOID;
		return BNULL;
	}

	m_NCS_OS_HTONLL_P(p64, val64);

	return (pbuf);
}

/*****************************************************************************
 *
 * Routines to encode 16, 24, 32, or 64 ints into network order in pdu ...
 *
 * "stream" points to the start of a n-octet  in the host order.
 * Network-order is the same as "big-endian" order (MSB first).
 * These functions has a built-in network-to-host order effect.
 *
 *****************************************************************************/

uint64_t ncs_encode_64bit(uint8_t **stream, uint64_t val)
{

	m_NCS_OS_HTONLL_P((*stream), val);
	(*stream) += 8;
	return 8;
}

uint32_t ncs_encode_32bit(uint8_t **stream, uint32_t val)
{
	*(*stream)++ = (uint8_t)(val >> 24);
	*(*stream)++ = (uint8_t)(val >> 16);
	*(*stream)++ = (uint8_t)(val >> 8);
	*(*stream)++ = (uint8_t)(val);
	return 4;
}

uint32_t ncs_encode_24bit(uint8_t **stream, uint32_t val)
{
	*(*stream)++ = (uint8_t)(val >> 16);
	*(*stream)++ = (uint8_t)(val >> 8);
	*(*stream)++ = (uint8_t)(val);
	return 3;
}

uint32_t ncs_encode_16bit(uint8_t **stream, uint32_t val)
{
	*(*stream)++ = (uint8_t)(val >> 8);
	*(*stream)++ = (uint8_t)(val);
	return 2;
}

uint32_t ncs_encode_8bit(uint8_t **stream, uint32_t val)
{
	*(*stream)++ = (uint8_t)(val);
	return 1;
}

uint32_t ncs_encode_key(uint8_t **stream, NCS_KEY *key)
{
	uint8_t len;

	*(*stream)++ = (uint8_t)(key->svc);
	*(*stream)++ = (uint8_t)(key->fmat);
	*(*stream)++ = (uint8_t)(key->type);

	switch (key->fmat) {
	case NCS_FMT_NUM:
		break;

	case NCS_FMT_STR:
		len = (uint8_t)strlen((char *)key->val.str);
		ncs_encode_8bit(stream, len);
		ncs_encode_octets(stream, key->val.str, len);
		return (3 + len + 1);
		break;

	case NCS_FMT_OCT:
		len = key->val.oct.len;
		ncs_encode_8bit(stream, len);
		ncs_encode_octets(stream, key->val.oct.data, len);
		return (3 + len + 1);
		break;
	}
	return m_LEAP_DBG_SINK(0);
}

uint32_t ncs_encode_octets(uint8_t **stream, uint8_t *val, uint32_t count)
{
	uint32_t i;
	for (i = 0; i < count; i++)
		*(*stream)++ = *val++;
	return count;
}
