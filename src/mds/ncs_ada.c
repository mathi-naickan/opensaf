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

  MODULE NAME:       ncs_ada.c   

  DESCRIPTION:       

******************************************************************************
*/
#include "base/ncsgl_defs.h"
#include "base/ncssysf_def.h"
#include "base/ncs_sprr_papi.h"
#include "mds/mds_papi.h"
#include "mds_adm.h"
#include "mda_pvt_api.h"

/*****************************************************************************\
**                       PRIVATE DATA STRUCTURES/DEFINITIONS                 **
\*****************************************************************************/

#define m_NCSADA_TRACE_ARG1(X)

/*****************************************************************************\
**                       PRIVATE FUNCTIONS                                   **
\*****************************************************************************/
static uint32_t ada_create(NCS_LIB_REQ_INFO *req);
static uint32_t ada_destroy(NCS_LIB_REQ_INFO *req);
static uint32_t ada_instantiate(NCS_LIB_REQ_INFO *req);
static uint32_t ada_uninstantiate(NCS_LIB_REQ_INFO *req);
/*****************************************************************************\
**                       PUBLIC ADA FUNCTIONS                                **
\*****************************************************************************/
uint32_t ada_lib_req(NCS_LIB_REQ_INFO *req)
{
	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:
		m_NCSADA_TRACE_ARG1("ADA:LIB_CREATE\n");
		return ada_create(req);

	case NCS_LIB_REQ_INSTANTIATE:
		m_NCSADA_TRACE_ARG1("ADA:LIB_INSTANTIATE\n");
		return ada_instantiate(req);

	case NCS_LIB_REQ_UNINSTANTIATE:
		m_NCSADA_TRACE_ARG1("ADA:LIB_UNINSTANTIATE\n");
		return ada_uninstantiate(req);

	case NCS_LIB_REQ_DESTROY:
		m_NCSADA_TRACE_ARG1("ADA:LIB_DESTROY\n");
		return ada_destroy(req);

	default:
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}
}

static uint32_t ada_create(NCS_LIB_REQ_INFO *req)
{
	NCS_SPLR_REQ_INFO splr_req;

	memset(&splr_req, 0, sizeof(splr_req));

	/* STEP : Register VDA as a service provider */
	splr_req.i_sp_abstract_name = m_ADA_SP_ABST_NAME;
	splr_req.type = NCS_SPLR_REQ_REG;
	splr_req.info.reg.instantiation_api = ada_lib_req;
	splr_req.info.reg.instantiation_flags = 0;
	splr_req.info.reg.user_se_api = (NCSCONTEXT)ncsada_api;

	if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	m_NCSADA_TRACE_ARG1("ADA:LIB_CREATE:DONE\n");
	return NCSCC_RC_SUCCESS;
}

static uint32_t ada_destroy(NCS_LIB_REQ_INFO *req)
{
	NCS_SPLR_REQ_INFO splr_req;
	memset(&splr_req, 0, sizeof(splr_req));

	/* STEP : TODO : Reset ADEST stuff */
	/* STEP : Register VDA as a service provider */
	splr_req.i_sp_abstract_name = "NCS_ADA";
	splr_req.type = NCS_SPLR_REQ_DEREG;
	splr_req.info.dereg.dummy = 0;

	if (ncs_splr_api(&splr_req) != NCSCC_RC_SUCCESS) {
		return NCSCC_RC_FAILURE;
	}
	m_NCSADA_TRACE_ARG1("ADA:LIB_DESTROY:DONE\n");
	return NCSCC_RC_SUCCESS;
}

static uint32_t ada_instantiate(NCS_LIB_REQ_INFO *req)
{

	/* STEP : Get a handle to local mds core */
	req->info.inst.o_inst_hdl = mds_adm_get_adest_hdl();
	m_NCSADA_TRACE_ARG1("ADA:LIB_INSTANTIATE:DONE\n");
	return NCSCC_RC_SUCCESS;

}

static uint32_t ada_uninstantiate(NCS_LIB_REQ_INFO *req)
{
	m_NCSADA_TRACE_ARG1("ADA:LIB_UNINSTANTIATE:DONE\n");
	return NCSCC_RC_SUCCESS;
}
