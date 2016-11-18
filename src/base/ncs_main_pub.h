/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008-2010 The OpenSAF Foundation
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
 *            Wind River Systems
 *
 */

#ifndef BASE_NCS_MAIN_PUB_H_
#define BASE_NCS_MAIN_PUB_H_

#define NCS_MAIN_EOF              (3)
#define NCS_MAIN_ENTER_CHAR       (2)
#define NCS_MAIN_DEF_CLUSTER_ID   (1)
#define NCS_MAIN_DEF_PCON_ID      (0)
#define NCS_MAIN_MAX_INPUT       (128)
#define NCS_MAX_INPUT_ARG_DEF     (7)
#define NCS_MAX_STR_INPUT       (255)

#define MAX_NCS_CONFIG_ROOTDIR_LEN 200
#define MAX_NCS_CONFIG_FILENAME_LEN 30
#define MAX_NCS_CONFIG_FILEPATH_LEN                             \
  (MAX_NCS_CONFIG_ROOTDIR_LEN + MAX_NCS_CONFIG_FILENAME_LEN)

#define NCS_DEF_CONFIG_FILEPATH PKGSYSCONFDIR
#define NODE_ID_FILE PKGLOCALSTATEDIR "/node_id"

/***********************************************************************\
 ****
 ****     Utility APIs defined in  ncs_main_pub.c
 ****
\***********************************************************************/
char *ncs_util_search_argv_list(int argc, char *argv[], char *arg_prefix);
uint32_t file_get_word(FILE **fp, char *o_chword);
uint32_t file_get_string(FILE **fp, char *o_chword);

char *gl_pargv[NCS_MAIN_MAX_INPUT];
uint32_t gl_pargc;

#endif  // BASE_NCS_MAIN_PUB_H_
