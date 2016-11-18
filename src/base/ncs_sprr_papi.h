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

  DESCRIPTION:  This file provides SPRR public-API definition

  ******************************************************************************
  */
#ifndef BASE_NCS_SPRR_PAPI_H_
#define BASE_NCS_SPRR_PAPI_H_

#include "base/ncsgl_defs.h"
#include "base/ncs_osprm.h"
#include "base/ncs_main_papi.h"
#include "base/ncs_lib.h"
#include "base/ncs_saf.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define NCS_MAX_SP_ABSTRACT_NAME_LEN   31       /* Excluding the terminating '\0' */
#define NCS_ENV_ID_DEFAULT              1

/*****************************************************************\
 ******************** SPLR API request types *********************
\*****************************************************************/
typedef enum {

  /* Register a service provider library with SPLR */
  NCS_SPLR_REQ_REG,

  /* De-register a service provider library from SPLR */
  NCS_SPLR_REQ_DEREG,
} NCS_SPLR_REQ_TYPES;

#define NCS_SPLR_INSTANTIATION_PER_INST_NAME 0x1
#define NCS_SPLR_INSTANTIATION_PER_ENV_ID    0x2

/**** SPLR API REGISTER request structure ****/
typedef struct {
  /* Flags will be ORed NCS_SPLR_INSTANTIATION_* values */
  uint8_t instantiation_flags;
  NCS_LIB_REQUEST instantiation_api;      /* API for instantiating service-provider   */
  void *user_se_api;      /* Single-entry API for use by service-user */
} NCS_SPLR_REQ_REG_INFO;

/**** SPLR API DEREGISTER request structure ****/
typedef struct {
  uint8_t dummy;
} NCS_SPLR_REQ_DEREG_INFO;

/**** SPLR API request structure ****/
typedef struct {

  NCS_SPLR_REQ_TYPES type;

  char *i_sp_abstract_name;

  union {
    NCS_SPLR_REQ_REG_INFO reg;
    NCS_SPLR_REQ_DEREG_INFO dereg;
  } info;
} NCS_SPLR_REQ_INFO;

/**** SPLR API ****/
uint32_t ncs_splr_api(NCS_SPLR_REQ_INFO *info);

/*****************************************************************\
 ******************** SPIR API request types *********************
\*****************************************************************/
typedef enum {
  /*****************************************\
   ** API invoked by a Service Provider     **
   ** (to populates handles before demand)  **
   \*****************************************/
  /* Simple add for a newly created instance */
  NCS_SPIR_REQ_ADD_INST,

  /* Simple remove for any next registered instance */
  NCS_SPIR_REQ_RMV_INST,

  /*****************************************\
   ** API types invoked by Service User     **
   ** (to populates handles ON demand)      **
   \*****************************************/
  /* Simple lookup for a registered instance */
  NCS_SPIR_REQ_LOOKUP_INST,

  /* Simple lookup for any next registered instance */
  NCS_SPIR_REQ_LOOKUP_NEXT_INST,

  /* Atomic lookup + creation (if required) of instance,
     (triggers a LIB_INSTANTIATE call by SPRR if required) */
  NCS_SPIR_REQ_LOOKUP_CREATE_INST,

  /* Release of an instance (triggers a LIB_UNINSTANTIATE) */
  NCS_SPIR_REQ_REL_INST,

} NCS_SPIR_REQ_TYPES;

typedef struct {
  SaAmfCSIAttributeListT i_inst_attrs;    /* Attributes of the instance-name */
  uint32_t i_handle;
  void *i_arg;    /* Cookie */
} NCS_SPIR_REQ_ADD_INST_INFO;

typedef struct {
  void *o_user_se_api;
  uint64_t o_handle;
  void *o_arg;    /* Cookie */
} NCS_SPIR_REQ_LOOKUP_INST_INFO;

typedef struct {

  /* Key to next instance */
  SaNameT o_next_instance_name;
  uint32_t o_next_environment_id;

  /* Date on the next instance */
  void *o_user_se_api;
  uint64_t o_handle;
  void *o_arg;    /* Cookie */

} NCS_SPIR_REQ_LOOKUP_NEXT_INST_INFO;

typedef struct {
  SaAmfCSIAttributeListT i_inst_attrs;    /* Attributes of the instance-name */

  bool o_created; /* true if instance not preexisting */
  void *o_user_se_api;
  uint64_t o_handle;
  void *o_arg;    /* Cookie */
} NCS_SPIR_REQ_LOOKUP_CREATE_INST_INFO;

/**** SPIR API request structure ****/
typedef struct {

  NCS_SPIR_REQ_TYPES type;

  /* SPIR entry key */
  char *i_sp_abstract_name;       /* Name will be copied(so, pointer can be on-stack) */
  SaNameT i_instance_name;
  uint32_t i_environment_id;

  /* SPIR entry attribute info */
  union {
    /* The following two are invoked by the service-provider to
       populate his handles before actual "demand" */
    NCS_SPIR_REQ_ADD_INST_INFO add_inst;
    uint32_t rmv_inst;      /* dummy */

    /* The following two are invoked by the service-provider to
       populate his handles on actual "demand" */
    NCS_SPIR_REQ_LOOKUP_INST_INFO lookup_inst;
    NCS_SPIR_REQ_LOOKUP_NEXT_INST_INFO lookup_next_inst;
    NCS_SPIR_REQ_LOOKUP_CREATE_INST_INFO lookup_create_inst;
    uint32_t rel_inst;      /*dummy */
  } info;
} NCS_SPIR_REQ_INFO;

/**** SPIR API ****/
uint32_t ncs_spir_api(NCS_SPIR_REQ_INFO *info);

/** Function to setup NCS infrastructure elements for an environement **/
uint32_t ncs_environment_setup(uint32_t env_id);
uint32_t ncs_environment_clear(uint32_t env_id);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_NCS_SPRR_PAPI_H_
