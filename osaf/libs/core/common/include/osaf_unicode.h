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

/** @file
 *
 * This file contains functions for processing UTF-8 encoded strings.
 */

#ifndef OPENSAF_CORE_OSAF_UNICODE_H_
#define OPENSAF_CORE_OSAF_UNICODE_H_

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * @brief Check if the contents of a string is valid UTF-8.
 *
 * Returns true if the string @a i_string is encoded in valid UTF-8, and false
 * otherwise. This function will reject UTF-8 sequences longer than four bytes,
 * overlong UTF-8 character sequences, and Unicode characters above U+10FFFF.
 */
bool osaf_is_valid_utf8(const char* i_string);

/*
 * @brief Check if the contents of a string is valid UTF-8 and belongs to the
 *        wide-character class "graph" of the current locale.
 *
 * Returns true if the string @a i_string is encoded in valid UTF-8, and all its
 * characters belong to the wide-character class "graph" of the current locale.
 * Returns false otherwise. The caller must ensure that LC_CTYPE of the locale
 * is set up appropriately before calling this function.
 */
bool osaf_is_graph_utf8(const char* i_string);

/*
 * @brief Check if a UTF-8 encoded string is valid within an XML 1.0 document.
 *
 * Returns true if the UTF-8 encoded string @a i_string is valid within an XML
 * 1.0 document. This means that the string is encoded in valid UTF-8, and only
 * contains characters that are guaranteed to be allowed according to the XML
 * 1.0 standard. Returns false otherwise.
 */
bool osaf_is_valid_xml_utf8(const char* i_string);

#ifdef	__cplusplus
}
#endif

#endif	/* OPENSAF_CORE_OSAF_UNICODE_H_ */
