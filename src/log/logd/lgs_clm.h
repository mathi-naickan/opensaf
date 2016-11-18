/*      -*- OpenSAF -*-
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
 * Author(s): Oracle
 *
 */

#ifndef LOG_LOGD_LGS_CLM_H_
#define LOG_LOGD_LGS_CLM_H_

#include "clm/saf/saClm.h"
#include <map>
#include <utility>

#define m_LGS_GET_NODE_ID_FROM_ADEST(adest) (NODE_ID) ((uint64_t)adest >> 32)

typedef struct {
  NODE_ID clm_node_id;
} lgs_clm_node_t;

/*
 * @brief  Creates a thread to initialize with CLM.
 */
void lgs_init_with_clm(void);
bool is_client_clm_member(NODE_ID node_id, SaVersionT *ver);
uint32_t lgs_clm_node_map_init(lgs_cb_t *lgs_cb);
bool is_client_clm_member(NODE_ID node_id, SaVersionT *ver);

#endif  // LOG_LOGD_LGS_CLM_H_
