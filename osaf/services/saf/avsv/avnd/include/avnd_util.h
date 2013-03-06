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

..............................................................................

  DESCRIPTION:

  This file contains the prototypes for utility operations.
  
******************************************************************************
*/

#ifndef AVND_UTIL_H
#define AVND_UTIL_H

struct avnd_cb_tag;
struct avnd_comp_tag;
enum avnd_comp_clc_cmd_type;

extern const char *presence_state[];
extern const char *ha_state[];

void avnd_msg_content_free(struct avnd_cb_tag *, AVND_MSG *);

uint32_t avnd_msg_copy(struct avnd_cb_tag *, AVND_MSG *, AVND_MSG *);
extern void avnd_msgid_assert(uint32_t msg_id);
void avnd_comp_cleanup_launch(AVND_COMP *comp);

bool avnd_failed_state_file_exist(void);
void avnd_failed_state_file_create(void);
void avnd_failed_state_file_delete(void);
const char *avnd_failed_state_file_location(void);

#endif   /* !AVND_UTIL_H */
