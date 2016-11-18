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
 * Author(s): Ericsson AB
 *
 */


#ifndef OSAF_APITEST_UTIL_H_
#define OSAF_APITEST_UTIL_H_

#include "osaf/saf/saAis.h"

extern SaTimeT getSaTimeT(void);
extern const char *get_saf_error(SaAisErrorT rc);
extern void safassert_impl(const char* file, unsigned int line, SaAisErrorT actual, SaAisErrorT expected);

/**
 * Pre and post condition checking. Will print information and
 * terminate program if actual and expected status code differ.
 * @param actual
 * @param expected
 */
#define safassert(actual, expected) safassert_impl(__FILE__, __LINE__, actual, expected)

#endif  // OSAF_APITEST_UTIL_H_
