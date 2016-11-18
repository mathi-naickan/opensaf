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

$$

MODULE NAME: rde_rda.c

..............................................................................

DESCRIPTION:

This file contains an implementation of the RDE rde_rda_cb Interface
The rde_rda_cb Interface is used for communication between RDE
and command line utilities.

This implementation of the rde_rda_cb` Interface uses a Unix domain sockets
interface.

******************************************************************************/

#include "rde/rded/rde_rda.h"
#include "base/logtrace.h"
#include "rde/rded/rde_cb.h"
#include "rde/rded/role.h"

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_sock_open

 DESCRIPTION:          Open the socket associated with
 the RDA Interface

 ARGUMENTS:
 rde_rda_cb         Pointer to RDA Socket Config structure

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully opened the socket
 NCSCC_RC_FAILURE:   Failure opening socket

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_sock_open(RDE_RDA_CB *rde_rda_cb) {
  uint32_t rc = NCSCC_RC_SUCCESS;

  TRACE_ENTER();

  rde_rda_cb->fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (rde_rda_cb->fd < 0) {
    LOG_ER("socket FAILED %s", strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  return rc;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_sock_init

 DESCRIPTION:          Initialize the socket associated with
 the rde_rda_cb Interface

 ARGUMENTS:
 rde_rda_cb         Pointer to RDE  Socket Config structure

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully initialized the socket
 NCSCC_RC_FAILURE:   Failure initializing the socket

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_sock_init(RDE_RDA_CB *rde_rda_cb) {
  struct stat sockStat;
  int rc;

  TRACE_ENTER();

  rc = stat(rde_rda_cb->sock_address.sun_path, &sockStat);
  if (rc == 0) { /* File exists */
    rc = remove(rde_rda_cb->sock_address.sun_path);
    if (rc != 0) {
      LOG_ER("remove FAILED %s", strerror(errno));
      return NCSCC_RC_FAILURE;
    }
  }

  rc = bind(rde_rda_cb->fd, (struct sockaddr *) &rde_rda_cb->sock_address,
            sizeof(rde_rda_cb->sock_address));
  if (rc < 0) {
    LOG_ER("bind FAILED %s", strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  rc = listen(rde_rda_cb->fd, 5);
  if (rc < 0) {
    LOG_ER("listen FAILED %s", strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_sock_close

 DESCRIPTION:          Close the socket associated with
 the rde_rda_cb Interface

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully closed the socket
 NCSCC_RC_FAILURE:   Failure closing the socket

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_sock_close(RDE_RDA_CB *rde_rda_cb) {
  int rc;

  TRACE_ENTER();

  rc = close(rde_rda_cb->fd);
  if (rc < 0) {
    LOG_ER("close FAILED %s", strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  rc = remove(rde_rda_cb->sock_address.sun_path);
  if (rc < 0) {
    LOG_WA("remove %s FAILED %s", rde_rda_cb->sock_address.sun_path,
           strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_write_msg

 DESCRIPTION:    Write a message to a peer on the socket

 ARGUMENTS:

 RETURNS:

 NCSCC_RC_SUCCESS:
 NCSCC_RC_FAILURE:

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_write_msg(int fd, char *msg) {
  int rc = NCSCC_RC_SUCCESS;
  int msg_size = 0;

  TRACE_ENTER2("%u - '%s'", fd, msg);
  msg_size = send(fd, msg, strlen(msg) + 1, 0);
  if (msg_size < 0) {
    if (errno != EINTR && errno != EWOULDBLOCK)
      LOG_ER("send FAILED %s", strerror(errno));

    return NCSCC_RC_FAILURE;
  }

  return rc;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_read_msg

 DESCRIPTION:    Read a complete line from the socket

 ARGUMENTS:

 RETURNS:

 NCSCC_RC_SUCCESS:
 NCSCC_RC_FAILURE:

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_read_msg(int fd, char *msg, int size) {
  int msg_size = 0;

  TRACE_ENTER2("%u", fd);

  msg_size = recv(fd, msg, size, 0);
  if (msg_size < 0) {
    if (errno != EINTR && errno != EWOULDBLOCK)
      /* Non-benign error */
      LOG_ER("recv FAILED %s", strerror(errno));

    return NCSCC_RC_FAILURE;
  }

  /*
   ** Is peer closed?
   */
  if (msg_size == 0) {
    /*
     ** Yes! disconnect client
     */
    snprintf(msg, size, "%d", RDE_RDA_DISCONNECT_REQ);
    LOG_IN("Connection closed by client (orderly shutdown)");
    return NCSCC_RC_SUCCESS;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_process_get_role

 DESCRIPTION:          Process the get role message

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure
 index        Index to identify the readable client fd

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully got role
 NCSCC_RC_FAILURE:   Failure getiing role

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_process_get_role(RDE_RDA_CB *rde_rda_cb, int index) {
  char msg[64] = { 0 };
  TRACE_ENTER();

  snprintf(msg, sizeof(msg), "%d %d", RDE_RDA_GET_ROLE_RES,
          static_cast<int>(rde_rda_cb->role->role()));
  if (rde_rda_write_msg(rde_rda_cb->clients[index].fd, msg)
      != NCSCC_RC_SUCCESS) {
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_process_set_role

 DESCRIPTION:          Process the get role message

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure
 index        Index to identify the readable client fd
 role         HA role to seed into RDE

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully set role
 NCSCC_RC_FAILURE:   Failure setting role

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_process_set_role(RDE_RDA_CB *rde_rda_cb, int index,
                                         int role) {
  char msg[64] = { 0 };

  TRACE_ENTER();

  if (rde_rda_cb->role->SetRole(
      static_cast<PCS_RDA_ROLE>(role)) != NCSCC_RC_SUCCESS)
    snprintf(msg, sizeof(msg), "%d", RDE_RDA_SET_ROLE_NACK);
  else
    snprintf(msg, sizeof(msg), "%d", RDE_RDA_SET_ROLE_ACK);

  if (rde_rda_write_msg(rde_rda_cb->clients[index].fd, msg)
      != NCSCC_RC_SUCCESS) {
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_process_reg_cb

 DESCRIPTION:          Process the register callback message

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure
 index        Index to identify the readable client fd

 RETURNS:

 NCSCC_RC_SUCCESS:
 NCSCC_RC_FAILURE:

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_process_reg_cb(RDE_RDA_CB *rde_rda_cb, int index) {
  char msg[64] = { 0 };

  TRACE_ENTER();

  /*
   ** Asynchronous callback registered by RDA
   */
  rde_rda_cb->clients[index].is_async = true;

  /*
   ** Format ACK
   */
  snprintf(msg, sizeof(msg), "%d", RDE_RDA_REG_CB_ACK);

  if (rde_rda_write_msg(rde_rda_cb->clients[index].fd, msg)
      != NCSCC_RC_SUCCESS) {
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_process_disconnect

 DESCRIPTION:          Process the register callback message

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure
 index        Index to identify the readable client fd

 RETURNS:

 NCSCC_RC_SUCCESS:
 NCSCC_RC_FAILURE:

 NOTES:

 *****************************************************************************/
static uint32_t rde_rda_process_disconnect(RDE_RDA_CB *rde_rda_cb, int index) {
  int rc = 0;
  int iter = 0;

  TRACE_ENTER();

  /*
   ** Save socket fd
   */
  int sockfd = rde_rda_cb->clients[index].fd;

  /*
   ** Delete fd from the client list
   */
  for (iter = index; iter < rde_rda_cb->client_count - 1; iter++)
    rde_rda_cb->clients[iter] = rde_rda_cb->clients[iter + 1];

  rde_rda_cb->client_count--;

  rc = close(sockfd);
  if (rc < 0) {
    LOG_ER("close FAILED %s", strerror(errno));
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_open

 DESCRIPTION:          Initialize the RDA Interface

 ARGUMENTS:

 RETURNS:

 NCSCC_RC_SUCCESS:   rde_rda_cb Interface has been successfully initialized
 NCSCC_RC_FAILURE:   Failure initializing rde_rda_cb Interface

 NOTES:

 *****************************************************************************/
uint32_t rde_rda_open(const char *sock_name, RDE_RDA_CB *rde_rda_cb) {
  TRACE_ENTER();

  strncpy(rde_rda_cb->sock_address.sun_path, sock_name,
          sizeof(rde_rda_cb->sock_address.sun_path));
  rde_rda_cb->sock_address.sun_family = AF_UNIX;
  rde_rda_cb->fd = -1;

  if (rde_rda_sock_open(rde_rda_cb) != NCSCC_RC_SUCCESS) {
    return NCSCC_RC_FAILURE;
  }

  if (rde_rda_sock_init(rde_rda_cb) != NCSCC_RC_SUCCESS) {
    rde_rda_sock_close(rde_rda_cb);
    return NCSCC_RC_FAILURE;
  }

  return NCSCC_RC_SUCCESS;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_client_process_msg

 DESCRIPTION:          Process the client message on socket

 ARGUMENTS:
 rde_rda_cb   Pointer to RDE  Socket Config structure
 index        Index to idetify the readable client fd

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully closed the socket
 NCSCC_RC_FAILURE:   Failure closing the socket

 NOTES:

 *****************************************************************************/
uint32_t rde_rda_client_process_msg(RDE_RDA_CB *rde_rda_cb, int index,
                                    int *disconnect) {
  RDE_RDA_CMD_TYPE cmd_type;
  char msg[256] = { 0 };
  uint32_t rc = NCSCC_RC_SUCCESS;
  int value = 0;
  char *ptr;

  TRACE_ENTER2("%u", index);

  if (rde_rda_read_msg(rde_rda_cb->clients[index].fd, msg, sizeof(msg))
      != NCSCC_RC_SUCCESS) {
    return NCSCC_RC_FAILURE;
  }

  /*
   ** Parse the message for cmd type and value
   */
  ptr = strchr(msg, ' ');
  if (ptr == nullptr) {
    cmd_type = static_cast<RDE_RDA_CMD_TYPE>(atoi(msg));
  } else {
    *ptr = '\0';
    cmd_type = static_cast<RDE_RDA_CMD_TYPE>(atoi(msg));
    value = atoi(++ptr);
  }

  switch (cmd_type) {
    case RDE_RDA_GET_ROLE_REQ:
      rc = rde_rda_process_get_role(rde_rda_cb, index);
      break;
    case RDE_RDA_SET_ROLE_REQ:
      rc = rde_rda_process_set_role(rde_rda_cb, index, value);
      break;
    case RDE_RDA_REG_CB_REQ:
      rc = rde_rda_process_reg_cb(rde_rda_cb, index);
      break;
    case RDE_RDA_DISCONNECT_REQ:
      rc = rde_rda_process_disconnect(rde_rda_cb, index);
      *disconnect = 1;
      break;
    default:
      break;
  }

  return rc;
}

/*****************************************************************************

 PROCEDURE NAME:       rde_rda_send_role

 DESCRIPTION:          Send HA role to RDA

 ARGUMENTS:
 role               HA role to seed into  RDE

 RETURNS:

 NCSCC_RC_SUCCESS:   Successfully set role
 NCSCC_RC_FAILURE:   Failure setting role

 NOTES:

 *****************************************************************************/
uint32_t rde_rda_send_role(int role) {
  int index;
  char msg[64] = { 0 };
  RDE_RDA_CB *rde_rda_cb = nullptr;
  RDE_CONTROL_BLOCK *rde_cb = rde_get_control_block();

  rde_rda_cb = &rde_cb->rde_rda_cb;
  TRACE("Sending role %d to RDA", role);
  /*
   ** Send role to all async clients
   */
  snprintf(msg, sizeof(msg), "%d %d", RDE_RDA_HA_ROLE, role);

  for (index = 0; index < rde_rda_cb->client_count; index++) {
    if (!rde_rda_cb->clients[index].is_async)
      continue;

    /*
     ** Write message
     */
    if (rde_rda_write_msg(rde_rda_cb->clients[index].fd, msg)
        != NCSCC_RC_SUCCESS) {
      /* We have nothing to do here */
    }
  }

  return NCSCC_RC_SUCCESS;
}
