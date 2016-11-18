/*
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

#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

#define USE_DEFAULT_COPY_AND_MOVE_OPERATORS(className)  \
  className(className&&) = default;                     \
  className(const className&) = default;                \
  className& operator=(className&&) = default;          \
  className& operator=(const className&) = default

#define DELETE_COPY_AND_MOVE_OPERATORS(className)       \
  className(className&&) = delete;                      \
  className(const className&) = delete;                 \
  className& operator=(className&&) = delete;           \
  className& operator=(const className&) = delete

#endif  // BASE_MACROS_H_
