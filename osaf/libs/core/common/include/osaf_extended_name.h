/*      -*- OpenSAF  -*-
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

/** @file
 *
 * This file contains functions to support tunneling strings through the legacy
 * SaNameT structure. The definitions in this file are for internal use within
 * OpenSAF only, and are intended to be used by the agent libraries to support
 * legacy SAF API functions that have SaNameT parameters.
 */

#ifndef OPENSAF_OSAF_LIBS_CORE_COMMON_INCLUDE_OSAF_EXTENDED_NAME_H_
#define OPENSAF_OSAF_LIBS_CORE_COMMON_INCLUDE_OSAF_EXTENDED_NAME_H_

#include <stddef.h>
#include <stdbool.h>
#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum {
  /**
   *  Magic number stored in the .length field (the first 16-bit word) of the
   *  legacy SaNameT type, to indicate that it contains a string longer than or
   *  equal to SA_MAX_UNEXTENDED_NAME_LENGTH bytes. A pointer to the string is
   *  stored immediately after the first 16-bit word (typically in the first
   *  four or eight bytes of the .value field of the legacy SaNameT type.
   */
  kOsafExtendedNameMagic = 0xcd2b,

  /**
   *  Maximum length of a distinguished name, not counting the terminating NUL
   *  character.
   */
  kOsafMaxDnLength = 2048
};

/**
 *  @brief Initialize the extended SaNameT functionality.
 *
 *  This function reads the environment variable SA_ENABLE_EXTENDED_NAMES to
 *  determine whether extended SaNameT shall be enabled or not. It shall be
 *  called by all top-level saXxxInitialize() functions, so that it is
 *  guaranteed to have been called before any other SAF API function.
 */
void osaf_extended_name_init(void);

/**
 *  @brief Check whether extended SaNameT is enabled.
 *
 *  This function returns true if extended SaNameT is enabled, and false
 *  otherwise. The function osaf_extended_name_init() must have been called
 *  prior to calling this function.
 */
static inline bool osaf_is_extended_names_enabled(void);

/**
 *  @brief Set the string pointer in the legacy SaNameT type.
 *
 *  This function sets the legacy SaNameT @a name to the NUL-terminated string
 *  @a value. If length of the string @a value is strictly less than
 *  SA_MAX_UNEXTENDED_NAME_LENGTH bytes, the contents of the string is copied
 *  into the legacy SaNameT type and can be read in a backwards compatible way
 *  by legacy applications. If length of the string @a value is greater than or
 *  equal to SA_MAX_UNEXTENDED_NAME_LENGTH, no copying is performed. Instead, a
 *  reference to the original string @a value is stored in @a name. In this
 *  case, it is important that @a value is not modified or freed until @a name
 *  is either overwritten or freed.
 */
void osaf_extended_name_lend(SaConstStringT value, SaNameT* name);

/**
 *  @brief Get a pointer to the string in the legacy SaNameT type.
 *
 *  This function returns a pointer to the string value in the legacy SaNameT @a
 *  name. If the .length field of the legacy SaNameT structure is not equal to
 *  the magic number @a kOsafExtendedNameMagic, the returned pointer points to a
 *  copy of the string stored inside @a name. Otherwise, the returned pointer
 *  points to memory outside @a name.
 *
 *  NOTE: This function is intended to be used in agent libraries to read
 *  SaNameT structures that may have been set by legacy application
 *  code. Therefore, the returned string pointer is not guaranteed to be NUL
 *  terminated. To get NUL-terminated string, you can do the following: use
 *  osaf_extended_name_length() to calculate the length of the string, call
 *  malloc() to allocate a memory area that is one byte larger than the string
 *  length, then use memcpy() to copy the string to the allocated memory, and
 *  finally add a NUL character at the end of the copied string.
 */
SaConstStringT osaf_extended_name_borrow(const SaNameT* name);

/**
 *  @brief Check if an SaNameT is in using the extended (long) format.
 *
 *  This function returns true if the SaNameT @a name is using the new extended
 *  (long) format where the SaNameT structure contains a pointer to the string,
 *  and false if @a name is using the legacy format where a copy of the string
 *  is stored inside the SaNameT structure.
 */
bool osaf_is_an_extended_name(const SaNameT* name);

/**
 *  @brief Check if an SaNameT passes some sanity checks.
 *
 *  This function returns true if the SaNameT @a name passes some sanity checks,
 *  e.g. that the length field is either equal to the magic number @a
 *  kOsafExtendedNameMagic, or strictly less than SA_MAX_UNEXTENDED_NAME_LENGTH.
 *  It returns false if @a name failed some of the sanity checks. It may of
 *  course also crash the process if @a name is severely corrupted, e.g. if the
 *  pointer stored inside @a name points to inaccessible memory.
 *
 *  NOTE: This function is intended to be used in agent libraries to read
 *  SaNameT structures that may have been set by legacy application
 *  code. Therefore, it does not require the string to be NUL terminated if it
 *  is stored in the legacy (short) format.
 */
bool osaf_is_extended_name_valid(const SaNameT* name);

/**
 *  @brief Check if an SaNameT contains the empty string.
 *
 *  This function works also with legacy SaNameT structures containing a string
 *  that is not NUL-terminated, and strings where there is a mismatch between
 *  the string length as indicated by the .length field an any NUL character
 *  contained in the string. It is equivalent to checking if the result of
 *  osaf_extended_name_length() is equal to zero, but may execute faster.
 */
bool osaf_is_extended_name_empty(const SaNameT* name);

/**
 *  @brief Calculate the length of an SaNameT.
 *
 *  This function returns the length of the SaNameT @a name. It may abort the
 *  process if @a name is not valid, but the sanity checks in this function are
 *  guaranteed to be no stricter than the checks in the
 *  osaf_is_extended_name_valid() function.
 *
 *  This function works also with legacy SaNameT structures containing a string
 *  that is not NUL-terminated, and strings where there is a mismatch between
 *  the string length as indicated by the .length field an any NUL character
 *  contained in the string (in which case the shortest of the two string
 *  lengths is returned).
 */
size_t osaf_extended_name_length(const SaNameT* name);

/**
 *  @brief Set an SaNameT to the empty string.
 *
 *  This function sets @a name to the empty string, i.e. a string of length
 *  zero.
 */
void osaf_extended_name_clear(SaNameT* name);

/**
 *  @brief Move a dynamically allocated string into an SaNameT structure.
 *
 *  This function sets the SaNameT structure @a name to point to the dynamically
 *  allocated string @a value. The string @a value must have been dynamically
 *  allocated using malloc(), calloc(), strdup() or a similar function. When
 *  this function returns, the original string pointer @a value is no longer
 *  valid and must not be used. When the SaNameT structure @a name is no longer
 *  needed, the dynamically allocated memory must be freed by a call to
 *  osaf_extended_name_free().
 *
 *  This function works also in the case when @a value is a NULL pointer, in
 *  which case it simply calls osaf_extended_name_clear().
 */
void osaf_extended_name_steal(SaStringT value, SaNameT* name);

/**
 *  @brief Make a copy of a string and store it in an SaNameT structure.
 *
 *  This function sets @a name to point to a newly allocated copy of the string
 *  @a value. When the SaNameT structure @a name is no longer needed, the
 *  dynamically allocated memory must be freed by a call to
 *  osaf_extended_name_free().
 *
 *  When the length of the string is strictly less than
 *  SA_MAX_UNEXTENDED_NAME_LENGTH, the SaNameT structure can be used in a
 *  backwards compatible way by legacy applications that do not understand the
 *  new extended SaNameT format.
 *
 *  This function works also in the case when @a value is a NULL pointer, in
 *  which case it simply calls osaf_extended_name_clear().
 */
void osaf_extended_name_alloc(SaConstStringT value, SaNameT* name);

/**
 *  @brief Free dynamically allocated memory pointed to by an SaNameT.
 *
 *  This function checks if the SaNameT @a name contains a pointer (i.e. is
 *  using the new extended format), and if so, it will call free() to release
 *  the dynamically allocated memory referenced by this SaNameT. When this
 *  function returns, @a name is no longer valid and must not be used (until it
 *  is re-initialized).
 *
 *  When the length of the string is strictly less than
 *  SA_MAX_UNEXTENDED_NAME_LENGTH, the SaNameT structure can be used in a
 *  backwards compatible way by legacy applications that do not understand the
 *  new extended SaNameT format.
 *
 *  This function works also in the case when @a name is a NULL pointer, in
 *  which case it does nothing.
 */
void osaf_extended_name_free(SaNameT* name);

static inline bool osaf_is_extended_names_enabled(void) {
  extern bool osaf_extended_names_enabled;
  return osaf_extended_names_enabled;
}

#ifdef  __cplusplus
}
#endif

#endif  /* OPENSAF_OSAF_LIBS_CORE_COMMON_INCLUDE_OSAF_EXTENDED_NAME_H_ */
