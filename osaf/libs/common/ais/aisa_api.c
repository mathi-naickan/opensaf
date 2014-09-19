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

/*****************************************************************************
  DESCRIPTION:

  The SaNameT type is deprecated will be replaced with string parameters in new
  SAF APIs. As an intermediate solution, the extended format of the SaNameT type
  can be used to pass string parameters to and from old SAF APIs as well, by
  tunneling them through the SaNameT type. To enable the extended SaNameT
  format, the application source code has to be compiled with the
  SA_EXTENDED_NAME_SOURCE preprocessor macro defined, and the environment
  variable SA_ENABLE_EXTENDED_NAMES must be set to the value 1 before the first
  call to any SAF API function.

  When the extended SaNameT format is enabled, the SA_MAX_NAME_LENGTH constant
  must not be used, and the application must treat the SaNameT type as opaque
  and not access any of its members directly. Instead, the saAisNameLend() and
  saAisNameBorrow() access functions shall be used. The
  SA_MAX_UNEXTENDED_NAME_LENGTH constant can be used to refer to the maximum
  string length that can be stored in the unextended SaNameT type.

*****************************************************************************/

#ifndef SA_EXTENDED_NAME_SOURCE
#define SA_EXTENDED_NAME_SOURCE
#endif
#define _GNU_SOURCE
#include "saAis.h"
#include <stddef.h>
#include <stdbool.h>
#include "osaf_extended_name.h"
#include "logtrace.h"

static void ais_name_lend(SaConstStringT value, SaNameT* name);
static SaConstStringT ais_name_borrow(const SaNameT* name);

/****************************************************************************
  Name		:  saAisNameLend

  Description	:  Tunnel a NUL-terminated string through a SaNameT type. If
		   length of the string is strictly less than
		   SA_MAX_UNEXTENDED_NAME_LENGTH bytes, the contents of the
		   string is copied into the SaNameT type and can be read in a
		   backwards compatible way by legacy applications that do not
		   support the extended SaNameT format. If length of the string
		   is greater than or equal to SA_MAX_UNEXTENDED_NAME_LENGTH, no
		   copying is performed. Instead, a reference to the original
		   string is stored in the SaNameT type. In this case, it is
		   therefore important that the original string is not modified
		   or freed for as long as the SaNameT type may still used.

  Arguments	:  value [in] - A pointer to a NUL-terminated string that will
				be tunneled through the SaNameT type.

		   name [out] - A pointer to an SaNameT type to be used for
				tunneling.

  Return Values :

  Notes		:
******************************************************************************/
void saAisNameLend(SaConstStringT value, SaNameT* name)
	__attribute__ ((weak, alias ("ais_name_lend")));

/****************************************************************************
  Name		:  saAisNameBorrow

  Description :	   Retrieve a tunneled string from an SaNameT type. Before
		   calling this function, the SaNameT stucture must have been
		   initialized either by a call to the saAisNameLend() function
		   or by being used as an output parameter of any other SAF API
		   function. If the length of the returned string is strictly
		   less than SA_MAX_UNEXTENDED_NAME_LENGTH bytes, the returned
		   pointer points to a copy of the string stored inside the
		   SaNameT type. Otherwise, the returned pointer is equal to the
		   original string pointer that was passed as a parameter to the
		   saAisNameLend() function.

  Arguments	:  name [in] -	A pointer to an SaNameT type that has been
				previously set using the saAisNameLend()
				function.

  Return Values :  A pointer to a NUL-terminated string, or a NULL pointer in
		   case of a failure.

  Notes		:
******************************************************************************/
SaConstStringT saAisNameBorrow(const SaNameT* name)
	__attribute__ ((weak, alias ("ais_name_borrow")));

void ais_name_lend(SaConstStringT value, SaNameT* name)
{
	osaf_extended_name_lend(value, name);
}

SaConstStringT ais_name_borrow(const SaNameT* name)
{
	SaConstStringT value = osaf_extended_name_borrow(name);
	size_t length = name->_opaque[0];
	if (length != kOsafExtendedNameMagic) {
		/*
		 * Check that the string inside SaNameT is properly
		 * NUL-terminated. If not, we return a NULL pointer. We allow
		 * the terminating NUL character to be counted in the length
		 * field, as this is a common mistake and still makes the string
		 * NUL-terminated (although with a length that is different from
		 * what the length field says).
		 *
		 * The reason for performing this check is that a C string MUST
		 * be NUL terminated, but the legacy SaNameT type did not
		 * require the string stored in the value field to be NUL
		 * terminated. It will always be NUL terminated if the SaNameT
		 * was set using the saAisNameLend() function, but potentially
		 * we could be passed an SaNameT that was set using legacy code.
		 */
		bool valid = length < SA_MAX_UNEXTENDED_NAME_LENGTH &&
			value[length] == '\0';
		if (!valid) value = NULL;
	}
	return value;
}
