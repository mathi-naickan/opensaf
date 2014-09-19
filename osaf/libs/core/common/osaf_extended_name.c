/*	-*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

#ifndef SA_EXTENDED_NAME_SOURCE
#define SA_EXTENDED_NAME_SOURCE
#endif
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "osaf_extended_name.h"
#include "ncsgl_defs.h"

enum {
	/* Index in the SaNameT._opaque array where the string pointer will be
	   stored when the distinguished name is longer than 255 bytes. By
	   storing the pointer at an aligned address, Valgrind will be able to
	   detect the pointer and thus the memory leak detection in Valgrind
	   will work with these strings. Note though that since the largest
	   member in the SaNameT structure is a 16-bit integer, there is no
	   guarantee that the SaNameT structure itself is stored at an aligned
	   address. */
	kExtendedNamePointerOffset = sizeof(SaConstStringT) / sizeof(SaUint16T)
};

static inline SaConstStringT get_ptr(const SaNameT* name);
static inline void set_ptr(SaConstStringT value, SaNameT* name);

bool osaf_extended_names_enabled = false;
static bool extended_names_initialized = false;

static inline SaConstStringT get_ptr(const SaNameT* name)
{
	union {
		SaConstStringT pointer;
		SaUint8T bytes[sizeof(SaConstStringT)];
	} tmp;
	memcpy(tmp.bytes, name->_opaque + kExtendedNamePointerOffset,
		sizeof(SaConstStringT));
	return tmp.pointer;
}

static inline void set_ptr(SaConstStringT value, SaNameT* name)
{
	union {
		SaConstStringT pointer;
		SaUint8T bytes[sizeof(SaConstStringT)];
	} tmp;
	tmp.pointer = value;
	name->_opaque[0] = kOsafExtendedNameMagic;
	memcpy(name->_opaque + kExtendedNamePointerOffset, tmp.bytes,
		sizeof(SaConstStringT));
}

void osaf_extended_name_init(void)
{
	if (!extended_names_initialized) {
		char* enable = getenv("SA_ENABLE_EXTENDED_NAMES");
		if (enable != NULL && enable[0] == '1' && enable[1] == '\0') {
			osaf_extended_names_enabled = true;
		} else {
			osaf_extended_names_enabled = false;
		}
		extended_names_initialized = true;
	}
}

void osaf_extended_name_lend(SaConstStringT value, SaNameT* name)
{
	size_t length = strlen(value);
	if (length < SA_MAX_UNEXTENDED_NAME_LENGTH) {
		name->_opaque[0] = length;
		memcpy(name->_opaque + 1, value, length + 1);
	} else {
		set_ptr(value, name);
	}
}

SaConstStringT osaf_extended_name_borrow(const SaNameT* name)
{
	size_t length = name->_opaque[0];
	SaConstStringT value;
	if (length != kOsafExtendedNameMagic) {
		value = (SaConstStringT) (name->_opaque + 1);
	} else {
		value = get_ptr(name);
	}
	return value;
}

bool osaf_is_an_extended_name(const SaNameT* name)
{
	return name->_opaque[0] == kOsafExtendedNameMagic;
}

bool osaf_is_extended_name_valid(const SaNameT* name)
{
	size_t length = name->_opaque[0];
	bool is_valid;
	if (length != kOsafExtendedNameMagic) {
		is_valid = length < SA_MAX_UNEXTENDED_NAME_LENGTH;
	} else {
		is_valid = osaf_extended_names_enabled &&
			strnlen(get_ptr(name), SA_MAX_UNEXTENDED_NAME_LENGTH) >=
			SA_MAX_UNEXTENDED_NAME_LENGTH;
	}
	return is_valid;
}

bool osaf_is_extended_name_empty(const SaNameT* name)
{
	size_t length = name->_opaque[0];
	bool is_empty;
	if (length != kOsafExtendedNameMagic) {
		is_empty = length == 0;
	} else {
		is_empty = *get_ptr(name) == '\0';
	}
	return is_empty;
}

size_t osaf_extended_name_length(const SaNameT* name)
{
	size_t length = name->_opaque[0];
	if (length != kOsafExtendedNameMagic) {
		osafassert(length < SA_MAX_UNEXTENDED_NAME_LENGTH);
		length = strnlen((const char*) (name->_opaque + 1), length);
	} else {
		length = strlen(get_ptr(name));
		osafassert(osaf_extended_names_enabled &&
			   length >= SA_MAX_UNEXTENDED_NAME_LENGTH);
	}
	return length;
}

void osaf_extended_name_clear(SaNameT* name)
{
	name->_opaque[0] = 0;
	*(char*) (name->_opaque + 1) = '\0';
	memset(name->_opaque + kExtendedNamePointerOffset, 0,
		sizeof(SaConstStringT));
}

void osaf_extended_name_steal(SaStringT value, SaNameT* name)
{
	if (value != NULL) {
		size_t length = strlen(value);
		if (length < SA_MAX_UNEXTENDED_NAME_LENGTH) {
			name->_opaque[0] = length;
			memcpy(name->_opaque + 1, value, length + 1);
			free(value);
		} else {
			set_ptr(value, name);
		}
	} else {
		osaf_extended_name_clear(name);
	}
}

void osaf_extended_name_alloc(SaConstStringT value, SaNameT* name)
{
	if (value != NULL) {
		size_t length = strlen(value);
		void* pointer;
		if (length < SA_MAX_UNEXTENDED_NAME_LENGTH) {
			name->_opaque[0] = length;
			pointer = name->_opaque + 1;
		} else {
			pointer = malloc(length + 1);
			set_ptr(pointer, name);
		}
		memcpy(pointer, value, length + 1);
	} else {
		osaf_extended_name_clear(name);
	}
}

void osaf_extended_name_free(SaNameT* name)
{
	if (name != NULL) {
		if (name->_opaque[0] == kOsafExtendedNameMagic) {
			free((SaStringT*) get_ptr(name));
		}
		name->_opaque[0] = 0xffff;
		memset(name->_opaque + kExtendedNamePointerOffset, 0,
			sizeof(SaConstStringT));
	}
}
