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

  This is the header file used for dynamic library invocation and destruction
  in NCS framework.

  ******************************************************************************
  */

/*
 * Module Inclusion Control...
 */

#ifndef BASE_NCS_LIB_H_
#define BASE_NCS_LIB_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/ncs_saf.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
  NCS_LIB_REQ_CREATE = 1,
  NCS_LIB_REQ_INSTANTIATE,
  NCS_LIB_REQ_UNINSTANTIATE,
  NCS_LIB_REQ_DESTROY,
  NCS_LIB_REQ_TYPE_MAX
} NCS_LIB_REQ_TYPE;

typedef struct ncs_lib_create {
  /* Creation parameters, if any, are provided to libraries in a
  ** command-line arguments style
  */
  uint32_t argc;
  char **argv;
} NCS_LIB_CREATE;

typedef struct ncs_lib_instantiate {
  /* Instantiation parameters, */
  SaNameT i_inst_name;

  /* Environment to which the instance belongs */
  uint32_t i_env_id;

  /* Additionaly parameters (if any) passed to an SPRR lookup+creation request */
  SaAmfCSIAttributeListT i_inst_attrs;    /* Attributes of the instance-name */

  /* Handle to the instance */
  uint32_t o_inst_hdl;

  /* Opaque to the instance */
  void *o_arg;

} NCS_LIB_INSTANTIATE;

typedef struct ncs_lib_uninstantiate {
  /* Instantiation parameters, */
  SaNameT i_inst_name;

  /* Environment to which the instance belongs */
  uint32_t i_env_id;

  /* Handle to the instance */
  uint32_t i_inst_hdl;

  /* Opaque to the instance */
  void *i_arg;
} NCS_LIB_UNINSTANTIATE;

typedef struct ncs_lib_destroy {
  uint32_t dummy; /* Not used as of now */
} NCS_LIB_DESTROY;

typedef struct ncs_lib_req_info {
  NCS_LIB_REQ_TYPE i_op;
  union {
    NCS_LIB_CREATE create;
    NCS_LIB_INSTANTIATE inst;
    NCS_LIB_UNINSTANTIATE uninst;
    NCS_LIB_DESTROY destroy;
  } info;
} NCS_LIB_REQ_INFO;

typedef uint32_t (*NCS_LIB_REQUEST) (NCS_LIB_REQ_INFO *req_info);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_LIB_H_
