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

 MODULE NAME: rde_rda.h

 ..............................................................................

 DESCRIPTION: Contains definitions of a Unix-domain Sockets implementation
 of the RDE RDA interface
 which is used for communication between RDE and and RDA.

 ..............................................................................

 ******************************************************************************
 */

#ifndef RDE_RDED_RDE_RDA_H_
#define RDE_RDED_RDE_RDA_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdint>

class Role;

/*****************************************************************************\
*                                                                             *
*   Constants and Enumerated Values                                           *
*                                                                             *
\*****************************************************************************/

/*
 ** Limits
 **
 **
 */

enum RDE_RDA_SOCK_LIMITS {
  RDE_RDA_SOCK_BUFFER_SIZE = 3200
};

/*****************************************************************************\
*                                                                             *
*   Structure Definitions                                                     *
*                                                                             *
\*****************************************************************************/

#define REPLY_SIZE 4096
#define MAX_RDA_CLIENTS 64

/*
 * Forward declarations
 */

struct RDE_RDA_CLIENT {
  int fd;
  bool is_async;
};

/*
 *  Socket Management related information
 */

using rde_rda_sock_addr = sockaddr_un;

struct RDE_RDA_CB {
  struct sockaddr_un sock_address;
  int fd; /* File descriptor */
  int flags; /* Flags specified for open */
  int client_count;
  Role* role;
  RDE_RDA_CLIENT clients[MAX_RDA_CLIENTS];
};

/***************************************************************\
*                                                               *
*         RDE RDA interface function prototypes                 *
*                                                               *
\***************************************************************/

uint32_t rde_rda_open(const char *sockname, RDE_RDA_CB *rde_rda_cb);
uint32_t rde_rda_process_msg(RDE_RDA_CB *rde_rda_cb);
uint32_t rde_rda_client_process_msg(RDE_RDA_CB *rde_rda_cb, int index,
                                    int *disconnect);
uint32_t rde_rda_send_role(int role);

#endif  // RDE_RDED_RDE_RDA_H_
