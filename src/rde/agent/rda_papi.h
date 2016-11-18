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

 MODULE NAME: pcs_rda_papi.h

 ..............................................................................

 DESCRIPTION:  This file declares the interface API for RDF.

 ******************************************************************************
 */

#ifndef RDE_AGENT_RDA_PAPI_H_
#define RDE_AGENT_RDA_PAPI_H_

/*
** Includes
*/
#include <stdio.h>
#include "base/ncsgl_defs.h"
#include "amf/saf/saAmf.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*
** Return/error codes
*/
typedef enum {
  PCSRDA_RC_SUCCESS,
  PCSRDA_RC_TIMEOUT,
  PCSRDA_RC_INVALID_PARAMETER,
  PCSRDA_RC_IPC_CREATE_FAILED,
  PCSRDA_RC_IPC_CONNECT_FAILED,
  PCSRDA_RC_IPC_SEND_FAILED,
  PCSRDA_RC_IPC_RECV_FAILED,
  PCSRDA_RC_TASK_SPAWN_FAILED,
  PCSRDA_RC_MEM_ALLOC_FAILED,
  PCSRDA_RC_CALLBACK_REG_FAILED,
  PCSRDA_RC_CALLBACK_ALREADY_REGD,
  PCSRDA_RC_LEAP_INIT_FAILED,
  PCSRDA_RC_FATAL_IPC_CONNECTION_LOST,
  PCSRDA_RC_ROLE_GET_FAILED,
  PCSRDA_RC_ROLE_SET_FAILED
} PCSRDA_RETURN_CODE;

/*
** Structure declarations
*/
typedef enum {
  PCS_RDA_LIB_INIT,
  PCS_RDA_LIB_DESTROY,
  PCS_RDA_REGISTER_CALLBACK,
  PCS_RDA_UNREGISTER_CALLBACK,
  PCS_RDA_SET_ROLE,
  PCS_RDA_GET_ROLE,
} PCS_RDA_REQ_TYPE;

typedef enum {
  PCS_RDA_UNDEFINED = 0,
  PCS_RDA_ACTIVE,
  PCS_RDA_STANDBY,
  PCS_RDA_QUIESCED,
  PCS_RDA_QUIESCING
} PCS_RDA_ROLE;

typedef enum {
  PCS_RDA_ROLE_CHG_IND,
  PCS_RDA_CB_TYPE_MAX
} PCS_RDA_CB_TYPE;

typedef struct {
  PCS_RDA_CB_TYPE cb_type;
  union {
    PCS_RDA_ROLE io_role;
  } info;
} PCS_RDA_CB_INFO;

/*
** Callback Declaration
*/
typedef void (*PCS_RDA_CB_PTR)(uint32_t callback_handle,
                               PCS_RDA_CB_INFO *cb_info,
                               PCSRDA_RETURN_CODE error_code);

typedef struct {
  PCS_RDA_REQ_TYPE req_type;
  uint32_t callback_handle;
  union {
    PCS_RDA_CB_PTR call_back;
    PCS_RDA_ROLE io_role;
  } info;
} PCS_RDA_REQ;

/*
** API Declaration
*/
PCSRDA_RETURN_CODE pcs_rda_request(PCS_RDA_REQ *pcs_rda_req);

/**
 * Get AMF style HA role from RDE
 * @param ha_state [out]
 *
 * @return uint32_t NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 */
extern uint32_t rda_get_role(SaAmfHAStateT *ha_state);

/**
* Install callback that will be called when role/state of the controller
* change
* @param cb_handle passed to callback function
* @param rda_cb_ptr callback function
*
* @return uint32_t NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*/
extern uint32_t rda_register_callback(uint32_t cb_handle,
                                      PCS_RDA_CB_PTR rda_cb_ptr);

#ifdef  __cplusplus
}
#endif

#endif  // RDE_AGENT_RDA_PAPI_H_
