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

 MODULE NAME: rda.h

 ..............................................................................

 DESCRIPTION:  This file declares the RDA functions to interact with RDE

 ******************************************************************************
 */

#ifndef RDE_AGENT_RDA_H_
#define RDE_AGENT_RDA_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>
#include "base/ncsgl_defs.h"
#include "rde/agent/rda_papi.h"

struct RDA_CALLBACK_CB {
  NCSCONTEXT task_handle;
  bool task_terminate;
  PCS_RDA_CB_PTR callback_ptr;
  uint32_t callback_handle;
  int sockfd;
};

struct RDA_CONTROL_BLOCK {
  sockaddr_un sock_address;
};

#endif  // RDE_AGENT_RDA_H_
