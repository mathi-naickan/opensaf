/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2012 The OpenSAF Foundation
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
 * Author(s): Ericsson AB
 *
 */

#include "osaf_unicode.h"
#include <wctype.h>
#include <stdint.h>

/*
 * Helper function to extract a Unicode character from UTF-8. Only intended to
 * be used by the function next_char_from_utf8().
 *
 * It extracts @a n bytes from @a u, adds this to the already extracted value
 * @a c, and checks that the final result is not less than @a m.
 */
static uint32_t extract_utf8(const uint8_t** u, uint32_t c, int n, uint32_t m)
{
	if (n == 0) return c >= m ? c : 0xFFFFFFFF;
	uint32_t c2 = *(*u)++;
	if (c2 < 0x80) return 0xFFFFFFFF;
	if (c2 < 0xC0) return extract_utf8(u, (c << 6) | (c2 & 0x3F), n - 1, m);
	return 0xFFFFFFFF;
}

/*
 * @brief Get the next Unicode character from an UTF-8 encoded string
 *
 * Returns the next Unicode character from an UTF-8 encoded string @a io_utf8,
 * 0x0 if end of the string has been reached, or 0xFFFFFFFF if the string does
 * not contain valid UTF-8. The pointer @a io_utf8 is increased so that it
 * points to the next UTF-8 character in the string when this function
 * returns. The output value of @a io_utf8 is undefined when this function
 * returns 0x0 or 0xFFFFFFFF.
 *
 * This function rejects overlong UTF-8 sequences and UTF-8 sequences longer
 * than four bytes, but it does not check that the decoded character doesn't
 * exceed the maximum allowed value 0x10FFFF. Hence, this function returns an
 * integer that is either 0xFFFFFFFF or within the range [0, 0x1FFFFF].
 */
static uint32_t next_char_from_utf8(const uint8_t** io_utf8)
{
	uint32_t c = *(*io_utf8)++;
	if (c < 0x80) return c;
	if (c < 0xC0) return 0xFFFFFFFF;
	if (c < 0xE0) return extract_utf8(io_utf8, c & 0x1F, 1, 0x80);
	if (c < 0xF0) return extract_utf8(io_utf8, c & 0x0F, 2, 0x800);
	if (c < 0xF8) return extract_utf8(io_utf8, c & 0x07, 3, 0x10000);
	return 0xFFFFFFFF;
}

bool osaf_is_valid_utf8(const char* i_string)
{
	const uint8_t* u = (const uint8_t*) i_string;
	uint32_t c;
	while ((c = next_char_from_utf8(&u)) != 0) {
		if (c > 0x10FFFF) return false;
	}
	return true;
}

bool osaf_is_graph_utf8(const char* i_string)
{
	const uint8_t* u = (const uint8_t*) i_string;
	uint32_t c;
	while ((c = next_char_from_utf8(&u)) != 0) {
		if (c > 0x10FFFF || !iswgraph(c)) return false;
	}
	return true;
}

bool osaf_is_valid_xml_utf8(const char* i_string)
{
	const uint8_t* u = (const uint8_t*) i_string;
	uint32_t c;
	while ((c = next_char_from_utf8(&u)) != 0) {
		if (!(c == 0x9 || c == 0xA || c == 0xD ||
			(c >= 0x20 && c <= 0xD7FF) ||
			(c >= 0xE000 && c <= 0xFFFD) ||
			(c >= 0x10000 && c <= 0x10FFFF))) {
			return false;
		}
	}
	return true;
}
