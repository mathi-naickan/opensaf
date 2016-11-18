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
  DESCRIPTION:  MDS ADM OPs

  ******************************************************************************
  */

#ifndef MDS_MDS_ADM_H_
#define MDS_MDS_ADM_H_

/***********************************************************************
    4) ADM APIs into MDS   (requests into MDS by admin-view)
************************************************************************/

typedef enum {
  MDS_ADMOP_VDEST_CREATE,
  MDS_ADMOP_VDEST_CONFIG,
  MDS_ADMOP_VDEST_DESTROY,
  MDS_ADMOP_VDEST_QUERY,
  MDS_ADMOP_PWE_CREATE,
  MDS_ADMOP_PWE_DESTROY,
  MDS_ADMOP_PWE_QUERY,
} NCSMDS_ADMOP_TYPE;

typedef struct mds_admop_vdest_create_info {
  MDS_DEST i_vdest;       /* Default role = STANDBY */
  NCS_VDEST_TYPE i_policy;        /* The routing policy for this group of vdests. */
  MDS_HDL o_mds_vdest_hdl;
} MDS_ADMOP_VDEST_CREATE_INFO;

typedef struct mds_admop_vdest_config_info {
  MDS_DEST i_vdest;       /* Default role = STANDBY */
  V_DEST_RL i_new_role;
} MDS_ADMOP_VDEST_CONFIG_INFO;

typedef struct mds_admop_vdest_destroy_info {
  MDS_HDL i_vdest_hdl;
} MDS_ADMOP_VDEST_DESTROY_INFO;

typedef struct mds_admop_vdest_query_info {
  MDS_DEST i_local_vdest; /* Local VDEST being queried for */

  V_DEST_QA o_local_vdest_anc;
  MDS_HDL o_local_vdest_hdl;
} MDS_ADMOP_VDEST_QUERY_INFO;

typedef struct mds_admop_pwe_create_info {
  /* i_mds_dest_hdl: A virtual or absolute MDS handle, obtained respectively
     during a VDEST_CREATE or CORE_CREATE. */
  MDS_HDL i_mds_dest_hdl;

  /* i_pwe_id: A virtual or absolute MDS handle, obtained respectively
     during a VDEST_CREATE or CORE_CREATE. */
  PW_ENV_ID i_pwe_id;     /* Environment id for services */

  /* o_mds_pwe_hdl: An "MDS-handle" meant for all normal-MDS-services. These
     normal-MDS-services are a member of a PWE identified by "i_pwe_id"
     (cf. "o_mds_vdest_hdl" and "o_mds_adest_hdl") */
  MDS_HDL o_mds_pwe_hdl;
} MDS_ADMOP_PWE_CREATE_INFO;

typedef struct mds_admop_pwe_destroy_info {
  MDS_HDL i_mds_pwe_hdl;  /* Handle returned by PWE creation. */
} MDS_ADMOP_PWE_DESTROY_INFO;

typedef struct mds_admop_pwe_query_info {
  MDS_HDL i_local_dest_hdl;       /* Handle to the local VDEST (or NULL for ADEST) */
  PW_ENV_ID i_pwe_id;     /* ID of the PWE for which handle is reqd. */

  MDS_HDL o_mds_pwe_hdl;  /* Handle to the PWE. */
} MDS_ADMOP_PWE_QUERY_INFO;

typedef struct ncsmds_admop_info {
  NCSMDS_ADMOP_TYPE i_op; /* The type of operation */
  union {
    MDS_ADMOP_VDEST_CREATE_INFO vdest_create;
    MDS_ADMOP_VDEST_CONFIG_INFO vdest_config;
    MDS_ADMOP_VDEST_DESTROY_INFO vdest_destroy;
    MDS_ADMOP_VDEST_QUERY_INFO vdest_query;
    MDS_ADMOP_PWE_CREATE_INFO pwe_create;
    MDS_ADMOP_PWE_DESTROY_INFO pwe_destroy;
    MDS_ADMOP_PWE_QUERY_INFO pwe_query;
  } info;
} NCSMDS_ADMOP_INFO;

typedef uint32_t (*NCSMDS_ADM_API) (NCSMDS_ADMOP_INFO *mds_adm);
uint32_t ncsmds_adm_api(NCSMDS_ADMOP_INFO *mds_adm);

uint32_t mds_adm_get_adest_hdl(void);

#endif  // MDS_MDS_ADM_H_
