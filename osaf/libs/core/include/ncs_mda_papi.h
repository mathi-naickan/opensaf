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

  DESCRIPTION:  Contains agent APIs to manage creation, configuration and
                destruction of MDS-DESTs.   The APIs are grouped into two
                classes for this purpose:
                1) ADA APIs : Absolute destination agent APIs
                2) VDA APIs : Virtual  destination agent APIs

******************************************************************************
*/

#ifndef NCS_MDA_PAPI_H
#define NCS_MDA_PAPI_H

#include "mds_papi.h"

#ifdef  __cplusplus
extern "C" {
#endif

/***********************************************************************\
         Range of unnamed VDEST values reserved for 
         services external NCS implementation.
\***********************************************************************/
#define NCSVDA_EXTERNAL_UNNAMED_MIN          1001
#define NCSVDA_EXTERNAL_UNNAMED_MAX          2000

#define NCSVDA_UNNAMED_MAX                   NCSMDS_MAX_VDEST

/* Macro to be used to create an unnamed VDEST using the external
   unnamed min and max 

   Usage:
      MDS_DEST abc;
      uns16    some_external_unnamed_value = NCSVDA_EXTERNAL_UNNAMED_MIN;

      memset(&abc, 0, sizeof(abc));
      m_NCSVDA_SET_VDEST(abc, some_external_unnamed_value);
*/

#define m_NCSVDA_SET_VDEST(mds_dest, external_unnamed_value) \
    m_NCS_SET_VDEST_ID_IN_MDS_DEST(mds_dest, external_unnamed_value)

/***********************************************************************\
         
\***********************************************************************/
	typedef enum {
		NCSVDA_VDEST_CREATE_SPECIFIC,
		NCSVDA_VDEST_CREATE_NAMED,
	} NCSVDA_VDEST_CREATE_TYPE;

	typedef struct ncsvda_vdest_create_info {
		/*       
		   This is for obtaining an MDS virtual address against an arbitrary string.

		   When any arbitrary string is provided, VDA will contact a VDS and try
		   to obtain a MDS virtual destination(VDEST) address and an MDS virtual
		   destination anchor.  On receiving such a VDEST address, it will create 
		   that VDEST on the local MDS core, create a PWE 1 on it and return the 
		   handle to that PWE.

		   When the first time a string is used to get a virtual-address, a unique
		   unused VDEST would be generated. It will be assigned a VDEST anchor
		   value equal to 1. While this VDEST is in use, further VDEST_CREATE calls 
		   with the same string would return the same VDEST but a different anchor
		   value. The "persistent" option will determine whether and till when
		   a VDEST remains associated to a string.

		   If the "persistent" option is TRUE, then a string is permanently
		   attached to a virtual-address. If a process that uses a 
		   certain virtual address were to die, this association will 
		   NOT be removed.  It will be maintained until an explicit
		   VDEST_DESTROY call is made (once for every corresponding VDEST_CREATE 
		   call)

		   If "persistent" option is FALSE, then a virtual address will cease
		   to be associated with a string as soon as all processes using this
		   virtual address either expire or invoke the VDEST_DESTROY API.

		   The "i_policy" determines the load-share policy among various 
		   instances of a VDEST. Each VDEST instance is associated with
		   an anchor value, making <VDEST, anchor> a unique address in the
		   system. The load-sharing will be done among those VDEST instances
		   which are running in an ACTIVE role. The load-share will only load-share
		   requests originated locally; MDS will not attempt an exact load-share 
		   on a global basis.

		   NOTE: 
		   1) The "persistent" option should uniformly be either a TRUE or 
		   a FALSE across multiple VDEST_CREATE calls for the same string-name.

		   2) The VDEST_CREATE is a blocking call. The VDA uses the invoker's task 
		   context to contact the VDS and returns only upon getting a response from 
		   VDS.
		 */
		NCS_BOOL i_persistent;
		NCS_VDEST_TYPE i_policy;

		NCSVDA_VDEST_CREATE_TYPE i_create_type;
		union {
			struct {
				MDS_DEST i_vdest;	/* Caller specified VDEST             */
			} specified;	/* if create_type = NCSVDA_VDEST_CREATE_SPECIFIC   */

			struct {
				SaNameT i_name;
				MDS_DEST o_vdest;	/* VDA allocated VDEST                */
			} named;	/* if create_type = NCSVDA_VDEST_CREATE_NAMED    */

		} info;

		MDS_HDL o_mds_pwe1_hdl;	/* Handle to PWE 1 on vdest            */
		MDS_HDL o_mds_vdest_hdl;	/* Handle to vdest for global services */

	} NCSVDA_VDEST_CREATE_INFO;

	typedef struct ncsvda_vdest_lookup_info {
		SaNameT i_name;
		MDS_DEST o_vdest;
	} NCSVDA_VDEST_LOOKUP_INFO;

	typedef struct ncsvda_vdest_chg_role_info {
		MDS_DEST i_vdest;
		V_DEST_RL i_new_role;
	} NCSVDA_VDEST_CHG_ROLE_INFO;

/* ncsvda_vdest_destroy_info : Destroy a VDEST instance. A VDEST instance is
                               is identified by a <VDEST, anchor>. Additionally,
                               if the "i_make_vdest_non_persistent" is TRUE, 
                               then VDEST will be marked non-persistent. VDEST 
                               would be freed after all VDEST instances disappear
                               from the system. If this  
                               "i_make_vdest_non_persitent" is FALSE, then the 
                               VDEST's persistency is NOT changed.
*/
	typedef struct ncsvda_vdest_destroy_info {
		NCSVDA_VDEST_CREATE_TYPE i_create_type;	/* The way that this VDEST was created */
		SaNameT i_name;	/* Required iff i_create_type == NCSVDA_VDEST_NAMED */
		MDS_DEST i_vdest;
		NCS_BOOL i_make_vdest_non_persistent;
	} NCSVDA_VDEST_DESTROY_INFO;

/* ncsvda_pwe_create_info :    Creates a PWE on a VDEST */
	typedef struct ncsvda_pwe_create_info {
		/* i_mds_dest_hdl: A virtual MDS-destination handle, obtainable
		   through a NCSVDA_VDEST_CREATE call. */
		MDS_HDL i_mds_vdest_hdl;

		/* i_pwe_id: The environement-identifier for the PWE being created */
		PW_ENV_ID i_pwe_id;

		/* o_mds_pwe_hdl: An "MDS-handle" meant for all normal-MDS-services. These
		   normal-MDS-services are a member of a PWE identified by "i_pwe_id" */
		MDS_HDL o_mds_pwe_hdl;
	} NCSVDA_PWE_CREATE_INFO;

/* ncsvda_pwe_destroy_info :   Destroys a PWE on a VDEST. Any services
                               installed on this PWE are also automatically
                               destroyed by MDS. 
*/
	typedef struct ncsvda_pwe_destroy_info {

		/* i_mds_pwe_hdl: Handle to PWE obtained through a NCSVDA_PWE_CREATE
		   request. */
		MDS_HDL i_mds_pwe_hdl;

	} NCSVDA_PWE_DESTROY_INFO;

	typedef enum {
		NCSVDA_VDEST_CREATE,
		NCSVDA_VDEST_LOOKUP,
		NCSVDA_VDEST_CHG_ROLE,
		NCSVDA_VDEST_DESTROY,
		NCSVDA_PWE_CREATE,
		NCSVDA_PWE_DESTROY,

	} NCSVDA_REQ_TYPE;

	typedef struct ncsvda_info {

		NCSVDA_REQ_TYPE req;

		/* o_result: This will indicate the result of this operation 
		   (NCSCC_RC_SUCCESS or NCSCC_RC_FAILURE). This will the same the
		   value returned by the ncsvda_api. */
		uns32 o_result;

		union {
			NCSVDA_VDEST_CREATE_INFO vdest_create;
			NCSVDA_VDEST_LOOKUP_INFO vdest_lookup;
			NCSVDA_VDEST_CHG_ROLE_INFO vdest_chg_role;
			NCSVDA_VDEST_DESTROY_INFO vdest_destroy;
			NCSVDA_PWE_CREATE_INFO pwe_create;
			NCSVDA_PWE_DESTROY_INFO pwe_destroy;
		} info;
	} NCSVDA_INFO;

	typedef uns32 (*NCSVDA_API) (NCSVDA_INFO *vda_info);
	EXTERN_C uns32 ncsvda_api(NCSVDA_INFO *vda_info);

/*************************************************************************
    Absolute-destination-library APIs (requests into LEAP by services)
**************************************************************************/

	typedef enum {
		NCSADA_GET_HDLS,
		NCSADA_PWE_CREATE,
		NCSADA_PWE_DESTROY,

	} NCSADA_REQ_TYPE;

	typedef struct ncsada_get_hdls_info {
		/* 
		   This API is provided to MDS services to get a handle to PWE1 on
		   the absolute MDS destination local to an MDS core.  Since an
		   absolute MDS destination is created on an MDS core as soon 
		   as the MDS core is created, there is no explicit request needed
		   to create it.  

		   This API simply creates a PWE1 on the absolute destination, if
		   not already created, and returns the handle to that PWE1.
		 */

		/* Absolute destination of this core */
		MDS_DEST o_adest;

		MDS_HDL o_mds_pwe1_hdl;	/* Handle to PWE 1 on adest            */
		MDS_HDL o_mds_adest_hdl;	/* Handle to adest for global services */

	} NCSADA_GET_HDLS_INFO;

/* ncsada_pwe_create_info :    Creates a PWE on an ADEST */
	typedef struct ncsada_pwe_create_info {

		/* i_mds_dest_hdl: A absolute MDS-destination handle, obtainable
		   through the NCSADA_GET_HDLS_INFO call. */
		MDS_HDL i_mds_adest_hdl;

		/* i_pwe_id: The environement-identifier for the PWE being created */
		PW_ENV_ID i_pwe_id;

		/* o_mds_pwe_hdl: An "MDS-handle" meant for all normal-MDS-services. These
		   normal-MDS-services are a member of a PWE identified by "i_pwe_id" */
		MDS_HDL o_mds_pwe_hdl;

	} NCSADA_PWE_CREATE_INFO;

/* ncsada_pwe_destroy_info :   Destroys a PWE on a ADEST. Any services
                               installed on this PWE are also automatically
                               destroyed by MDS. 
*/
	typedef struct ncsada_pwe_destroy_info {

		/* i_mds_pwe_hdl: Handle to PWE obtained through a NCSADA_PWE_CREATE
		   request. */
		MDS_HDL i_mds_pwe_hdl;

	} NCSADA_PWE_DESTROY_INFO;

	typedef struct ncsada_info {

		NCSADA_REQ_TYPE req;
		union {
			NCSADA_GET_HDLS_INFO adest_get_hdls;
			NCSADA_PWE_CREATE_INFO pwe_create;
			NCSADA_PWE_DESTROY_INFO pwe_destroy;
		} info;
	} NCSADA_INFO;

	typedef uns32 (*NCSADA_API) (NCSADA_INFO *ada_info);
	EXTERN_C uns32 ncsada_api(NCSADA_INFO *ada_info);

#ifdef  __cplusplus
}
#endif

#endif
