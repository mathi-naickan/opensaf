/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#ifndef BASE_GETENV_H_
#define BASE_GETENV_H_

namespace base {

/**
 * @brief Parse an environment variable with a default value.
 *
 * Returns the contents of an environment variable. If the variable is not set
 * or not possible to parse, the @a default_value will be returned. This is a
 * template functions, and the return type will be the same as the type of the
 * @a default_value. Supported types are strings (const char*, std::string), as
 * well as signed and unsigned integer types. When parsing integers, the
 * prefixes 0x and 0 are supported for indicating that the number is hexadecimal
 * or octal, respectively. The parsed value is always logged using the OpenSAF
 * logtrace functionality, and parse errors are logged to syslog.
 */
template<typename T>
T GetEnv(const char* environment_variable, T default_value);

} // namespace base

#endif  // BASE_GETENV_H_
