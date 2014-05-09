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

  MODULE NAME:       ncs_sprr.c   

  DESCRIPTION:       This file contains the implemenation of SPRR 
                     library. This library consists of two modules,
                     namely:
                     SPIR : Service Provider Instance Registry, and
                     SPLR : Service Provider Library Registry.

******************************************************************************
*/
#include <ncsgl_defs.h>
#include "ncs_sprr_papi.h"
#include "mds_papi.h"
#include "sprr_dl_api.h"
#include "ncssysf_mem.h"
#include "ncspatricia.h"
#include "ncssysf_def.h"

/****************************************************************************\
 * Private macro definitions 
\****************************************************************************/

#ifdef NDEBUG
#define m_NCS_SPRR_DBG_SINK(x,y)  (x)
#else
#define m_NCS_SPRR_DBG_SINK(x,y)  (TRACE("SPRR:%s\n", y),m_LEAP_DBG_SINK(x))
#endif

#define m_NCSSPRR_TRACE_ARG2(x,y)
#define m_NCSSPRR_TRACE_ARG3(x,y,z)
#define m_NCSSPRR_TRACE_ARG4(x,y,z,w)
#define m_NCSSPRR_TRACE_ARG5(x,y,z,a,b)
#define m_NCSSPRR_TRACE_ARG6(x,y,z,a,b,c)
/****** MMGR  alloc and free macros *****/
#define m_MMGR_SPRR_SUBSVC_ID(x)   (6000+(x))
typedef enum {
	NCS_MMGR_SUBSVC_ID_CB,
	NCS_MMGR_SUBSVC_ID_SPLR_ENTRY,
	NCS_MMGR_SUBSVC_ID_SPIR_ENTRY
} NCS_MMGR_SUBSVC_IDS;

/*----*/
#define m_MMGR_ALLOC_NCS_SPRR_CB                            \
      (NCS_SPRR_CB *)m_NCS_MEM_ALLOC(sizeof(NCS_SPRR_CB),   \
                                 NCS_MEM_REGION_PERSISTENT, \
                                 NCS_SERVICE_ID_COMMON,     \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_CB))

#define m_MMGR_FREE_NCS_SPRR_CB(p)                          \
                     m_NCS_MEM_FREE(p,                      \
                                 NCS_MEM_REGION_PERSISTENT, \
                                 NCS_SERVICE_ID_COMMON, \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_CB))
/*----*/
#define m_MMGR_ALLOC_NCS_SPLR_ENTRY                               \
      (NCS_SPLR_ENTRY *)m_NCS_MEM_ALLOC(sizeof(NCS_SPLR_ENTRY),   \
                                 NCS_MEM_REGION_PERSISTENT,       \
                                 NCS_SERVICE_ID_COMMON,     \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_SPLR_ENTRY))

#define m_MMGR_FREE_NCS_SPLR_ENTRY(p)                       \
                     m_NCS_MEM_FREE(p,                      \
                                 NCS_MEM_REGION_PERSISTENT, \
                                 NCS_SERVICE_ID_COMMON, \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_SPLR_ENTRY))
/*----*/
#define m_MMGR_ALLOC_NCS_SPIR_ENTRY                               \
      (NCS_SPIR_ENTRY *)m_NCS_MEM_ALLOC(sizeof(NCS_SPIR_ENTRY),   \
                                 NCS_MEM_REGION_PERSISTENT,       \
                                 NCS_SERVICE_ID_COMMON,     \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_SPIR_ENTRY))

#define m_MMGR_FREE_NCS_SPIR_ENTRY(p)                       \
                     m_NCS_MEM_FREE(p,                      \
                                 NCS_MEM_REGION_PERSISTENT, \
                                 NCS_SERVICE_ID_COMMON, \
                                 m_MMGR_SPRR_SUBSVC_ID(NCS_MMGR_SUBSVC_ID_SPIR_ENTRY))

/****************************************************************************\
 * Private structure definitions 
\****************************************************************************/

typedef uint32_t NCS_SPL_INSTANTIATION_FLAGS;

/**** S P L R      S T U F F  ******/
typedef struct ncs_splr_key {
	uint8_t sp_abstract_name[NCS_MAX_SP_ABSTRACT_NAME_LEN + 1];
} NCS_SPLR_KEY;

/*----*/
typedef struct ncs_splr_entry {

	NCS_PATRICIA_NODE pat_node;

	/* Key */
	NCS_SPLR_KEY key;

	/* Data */
	NCS_SPL_INSTANTIATION_FLAGS inst_flags;
	NCS_LIB_REQUEST inst_api;
	void *user_se_api;
	uint32_t inst_count;

} NCS_SPLR_ENTRY;

/*----*/
typedef struct ncs_splr_cb {
	/* Service provider library list */
	NCS_PATRICIA_TREE spl_list;
} NCS_SPLR_CB;

/************ S P I R      S T U F F    ******/
typedef struct ncs_spir_key {
	uint8_t sp_abstract_name[NCS_MAX_SP_ABSTRACT_NAME_LEN + 1];
	SaNameT instance_name;
	uint32_t environment_id;	/* Should we use PW_ENV_ID instead of uns32? */
} NCS_SPIR_KEY;

/*----*/
typedef struct ncs_spir_entry {

	NCS_PATRICIA_NODE pat_node;

	/* Key */
	NCS_SPIR_KEY key;

	/* Data */
	uint32_t use_count;
	uint32_t handle;
	void *arg;
} NCS_SPIR_ENTRY;

/*----*/
typedef struct ncs_spir_cb {
	/* Service provider instance list */
	NCS_PATRICIA_TREE spi_list;
} NCS_SPIR_CB;

/*********  (and finally ...) S P R R    S T U F F  ******/
typedef struct ncs_sprr_cb {
	NCS_LOCK lock;		/* Will never be destroyed */

	NCS_SPLR_CB splr_cb;
	NCS_SPIR_CB spir_cb;
} NCS_SPRR_CB;

/****************************************************************************\
 * Private data structures 
\****************************************************************************/

/* Handle to the SPRR-CB. Only valid between sprr create and destroy calls */
static uint32_t gl_sprr_hdl;

/****************************************************************************\
 * Private functions 
\****************************************************************************/

static uint32_t sprr_create(NCS_LIB_REQ_INFO *create_req);
static uint32_t sprr_destroy(NCS_LIB_REQ_INFO *destroy_req);

/****************************************************************************\
 * Function: sprr_lib_req  (NCS-PRIVATE-API)
\****************************************************************************/
uint32_t sprr_lib_req(NCS_LIB_REQ_INFO *req)
{
	uint32_t rc;

	switch (req->i_op) {
	case NCS_LIB_REQ_CREATE:
		rc = sprr_create(req);
		break;

	case NCS_LIB_REQ_DESTROY:
		rc = sprr_destroy(req);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		break;
	}
	return rc;
}

/****************************************************************************\
 * Function: ncs_splr_api  (NCS-PUBLIC-API)
\****************************************************************************/
uint32_t ncs_splr_api(NCS_SPLR_REQ_INFO *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_SPRR_CB *sprr_cb;
	NCS_SPLR_KEY splr_key;
	NCS_SPLR_ENTRY *splr_entry;
	uint32_t sp_abs_name_len;

	/* Validate  service-providers abstract name-length */
	sp_abs_name_len = strlen((char *)info->i_sp_abstract_name);
	if ((sp_abs_name_len == 0) || ((sp_abs_name_len + 1) > (sizeof(splr_key.sp_abstract_name)))) {
		/* Name too long */
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Bad SP abstract name");
	}
	/* Length is ok.  Do the copying */
	memset(&splr_key, 0, sizeof(splr_key));
	strcpy((char *)splr_key.sp_abstract_name, (char *)info->i_sp_abstract_name);

	/* Validate and get lock */
	sprr_cb = (NCS_SPRR_CB *)ncshm_take_hdl(NCS_SERVICE_ID_COMMON, gl_sprr_hdl);
	if (sprr_cb == NULL)
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPRR module not initialized");
	m_NCS_LOCK(&sprr_cb->lock, NCS_LOCK_WRITE);

	/* Start processing */
	switch (info->type) {
	case NCS_SPLR_REQ_REG:
		if (ncs_patricia_tree_get(&sprr_cb->splr_cb.spl_list, (uint8_t *)&splr_key)
		    != NULL) {
			/* This is a duplicate entry. Reject this request */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_DUPLICATE_ENTRY, "SPLR duplication attempted");
			goto quit;
		}

		/* Check some other info too */
		if (info->info.reg.instantiation_api == NULL) {
			/* Null function provided. Reject this request */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_INV_VAL, "NULL instantantiation API");
			goto quit;
		}
		if (info->info.reg.instantiation_flags >
		    (NCS_SPLR_INSTANTIATION_PER_ENV_ID | NCS_SPLR_INSTANTIATION_PER_INST_NAME)) {
			/* Null function provided. Reject this request */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_INV_VAL, "Bad instantiation flags");
			goto quit;
		}

		/* Allocate a new entry */
		splr_entry = m_MMGR_ALLOC_NCS_SPLR_ENTRY;
		if (splr_entry == NULL) {
			rc = NCSCC_RC_OUT_OF_MEM;
			goto quit;
		}
		memset(splr_entry, 0, sizeof(*splr_entry));

		/* Set it up */
		splr_entry->pat_node.key_info = (uint8_t *)&splr_entry->key;
		splr_entry->key = splr_key;
		splr_entry->inst_api = info->info.reg.instantiation_api;
		splr_entry->inst_flags = info->info.reg.instantiation_flags;
		splr_entry->user_se_api = info->info.reg.user_se_api;

		/* And finally add it to the SPLR tree */
		ncs_patricia_tree_add(&sprr_cb->splr_cb.spl_list, &splr_entry->pat_node);
		m_NCSSPRR_TRACE_ARG2("SPRR:REG:sp_abstract_name=%s\n", splr_key.sp_abstract_name);
		break;

	case NCS_SPLR_REQ_DEREG:

		/* Check for presence of such an entry */
		splr_entry = (NCS_SPLR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->splr_cb.spl_list, (uint8_t *)&splr_key);

		if (splr_entry == NULL) {
			/* There is no such entry. Reject this request */
			rc = NCSCC_RC_NO_OBJECT;
			goto quit;
		}

		/* Graceful exit would mean there are no SPIR entries at this time
		   to be. So reject request, if there are any SPIR entries corresponding 
		   to this library */
		if (splr_entry->inst_count != 0) {
			/* There are instances linked to this splr entry. Hence,
			   deregistration will be rejected until those instances are
			   deleted 
			 */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPLR entry in use by SPIR");
			m_NCSSPRR_TRACE_ARG3("SPRR:DEREG:sp_abstract_name=%s,inst_count=%d\n",
					     splr_key.sp_abstract_name, splr_entry->inst_count);
			goto quit;
		}

		/* Remove from SPLR tree */
		ncs_patricia_tree_del(&sprr_cb->splr_cb.spl_list, &splr_entry->pat_node);
		m_MMGR_FREE_NCS_SPLR_ENTRY(splr_entry);
		splr_entry = NULL;	/* Precaution */
		m_NCSSPRR_TRACE_ARG2("SPRR:DEREG:sp_abstract_name=%s,inst_count=0\n", splr_key.sp_abstract_name);
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		goto quit;
	}

 quit:
	m_NCS_UNLOCK(&sprr_cb->lock, NCS_LOCK_WRITE);
	ncshm_give_hdl(gl_sprr_hdl);

	return rc;
}

/****************************************************************************\
 * Function: ncs_spir_api  (NCS-PUBLIC-API)
\****************************************************************************/
uint32_t ncs_spir_api(NCS_SPIR_REQ_INFO *info)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	NCS_SPRR_CB *sprr_cb;
	NCS_SPIR_KEY spir_key;
	NCS_SPIR_ENTRY *spir_entry;
	NCS_SPLR_ENTRY *splr_entry;
	uint32_t sp_abs_name_len;
	NCS_LIB_REQ_INFO lib_req;

	memset(&lib_req, 0, sizeof(lib_req));

	/* MEMSET the SPIR-KEY. It will be used as PATRICIA key */
	memset(&spir_key, 0, sizeof(spir_key));

   /*---STEP : Validate  service-provider info and copy  it*/
	sp_abs_name_len = strlen((char *)info->i_sp_abstract_name);
	if ((sp_abs_name_len == 0) || ((sp_abs_name_len + 1) > (sizeof(spir_key.sp_abstract_name)))) {
		/* Name too long */
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Bad SP abstract name");
	}
	strcpy((char *)spir_key.sp_abstract_name, (char *)info->i_sp_abstract_name);

   /*---STEP : Validate the SaNameT given and make a (htonl) copy of it */
	if (info->i_instance_name.length > sizeof(info->i_instance_name.value)) {
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Bad instance name length");
	}
	spir_key.instance_name.length = m_HTON_SANAMET_LEN(info->i_instance_name.length);
	memcpy(spir_key.instance_name.value, info->i_instance_name.value, info->i_instance_name.length);

   /*---STEP : Validate the "environment-id" given and copy it */
	if (info->i_environment_id > NCSMDS_MAX_PWES) {
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Invalid environment id");
	}
	spir_key.environment_id = info->i_environment_id;

   /*---STEP : Validate and get SPRR lock */
	sprr_cb = (NCS_SPRR_CB *)ncshm_take_hdl(NCS_SERVICE_ID_COMMON, gl_sprr_hdl);
	if (sprr_cb == NULL)
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPRR module not initialized");
	m_NCS_LOCK(&sprr_cb->lock, NCS_LOCK_WRITE);

   /*---STEP : Check if this service-provider has registered */
	splr_entry = (NCS_SPLR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->splr_cb.spl_list,
							     (uint8_t *)spir_key.sp_abstract_name);
	if (splr_entry == NULL) {
		/* Service provider library has not registered */
		rc = NCSCC_RC_FAILURE;
		goto quit;
	}

	/* Modify lookup keys based on the Library's instantiation flags */
	if (!(splr_entry->inst_flags & NCS_SPLR_INSTANTIATION_PER_INST_NAME)) {
		/* This library does not instantiate an inst-name basis. So use
		   a blank inst-name instead */
		memset(&spir_key.instance_name, 0, sizeof(spir_key.instance_name));
	} else if ((spir_key.instance_name.length == 0) && (info->type != NCS_SPIR_REQ_LOOKUP_NEXT_INST)) {
		/* This library necessarily needs to be provided a valid instance-name 
		   but the instance-name provided was NULL. Hence, fail this request */
		rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Null inst-name for a per-instance Service-Provider");
		goto quit;
	}
	if (!(splr_entry->inst_flags & NCS_SPLR_INSTANTIATION_PER_ENV_ID)) {
		/* This library does not instantiate a per PWE basis. So use
		   a blank PWE-ID instead */
		spir_key.environment_id = 0;
	} else if ((spir_key.environment_id == 0) && (info->type != NCS_SPIR_REQ_LOOKUP_NEXT_INST)) {
		/* This library necessarily needs to be provided a valid PWE-ID
		   but the PWE-ID provided was NULL. Hence, fail this request */
		rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE,
					 "Null environment-id for a per-environment Service-Provider");
		goto quit;
	}

   /*---STEP : Process the request */
	switch (info->type) {
	case NCS_SPIR_REQ_ADD_INST:

		if (ncs_patricia_tree_get(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key)

		    != NULL) {
			/* This is a duplicate entry. Reject this request */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_DUPLICATE_ENTRY, "Add for an already existing instance");
			m_NCSSPRR_TRACE_ARG4("SPIR:ADD_INST:DUPLICATE:ab-name=%s:instance_name=%s:env-id=%d\n",
					     spir_key.sp_abstract_name, spir_key.instance_name.value,
					     spir_key.environment_id);
			goto quit;
		}

		/* Allocate a new entry */
		spir_entry = m_MMGR_ALLOC_NCS_SPIR_ENTRY;
		if (spir_entry == NULL) {
			rc = NCSCC_RC_OUT_OF_MEM;
			goto quit;
		}
		memset(spir_entry, 0, sizeof(*spir_entry));

		/* Set it up */
		spir_entry->pat_node.key_info = (uint8_t *)&spir_entry->key;
		spir_entry->key = spir_key;
		spir_entry->use_count = 1;
		splr_entry->inst_count++;	/*Note:Splr-entry */
		spir_entry->arg = info->info.add_inst.i_arg;
		spir_entry->handle = info->info.add_inst.i_handle;

		/* And finally add it to the SPIR tree */
		ncs_patricia_tree_add(&sprr_cb->spir_cb.spi_list, &spir_entry->pat_node);
		m_NCSSPRR_TRACE_ARG4("SPIR:ADD_INST:SUCCESS:ab-name=%s:instance_name=%s:env-id=%d\n",
				     spir_key.sp_abstract_name, spir_key.instance_name.value, spir_key.environment_id);
		break;

	case NCS_SPIR_REQ_RMV_INST:
		/* Check for presence of such an entry */
		spir_entry = (NCS_SPIR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key);

		if (spir_entry == NULL) {
			/* There is no such entry. Reject this request */
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_NO_OBJECT, "Remove for unknown instance");
			m_NCSSPRR_TRACE_ARG4("SPIR:RMV_INST:FAILURE:ab-name=%s:instance_name=%s:env-id=%d\n",
					     spir_key.sp_abstract_name, spir_key.instance_name.value,
					     spir_key.environment_id);
			goto quit;
		}

		/* Decrement count of SPLR entry */
		if (splr_entry->inst_count < spir_entry->use_count) {
			splr_entry->inst_count = 0;
			rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_NO_OBJECT, "Internal error on SPLR count");
		} else
			splr_entry->inst_count = splr_entry->inst_count - spir_entry->use_count;

		/* Remove from SPIR tree */
		ncs_patricia_tree_del(&sprr_cb->spir_cb.spi_list, &spir_entry->pat_node);
		m_MMGR_FREE_NCS_SPIR_ENTRY(spir_entry);
		spir_entry = NULL;	/* Precaution */
		m_NCSSPRR_TRACE_ARG6("SPIR:RMV_INST:SUCCESS:ab-name=%s:instance_name=%s:env-id=%d"
				     ":splr_entry->inst_count=%d:spir_entry->use_count=%d\n",
				     spir_key.sp_abstract_name, spir_key.instance_name.value,
				     spir_key.environment_id, splr_entry->inst_count, spir_entry->use_count);
		break;

	case NCS_SPIR_REQ_LOOKUP_INST:
		/* Check for presence of such an entry */
		spir_entry = (NCS_SPIR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key);

		if (spir_entry == NULL) {
			/* There is no such entry. Reject this request */
			rc = NCSCC_RC_NO_OBJECT;
			goto quit;
		}

		/* Item found. Populate response and return */
		info->info.lookup_inst.o_arg = spir_entry->arg;
		info->info.lookup_inst.o_handle = spir_entry->handle;
		info->info.lookup_inst.o_user_se_api = splr_entry->user_se_api;
		break;

	case NCS_SPIR_REQ_LOOKUP_NEXT_INST:
		/* Check for presence of such an entry */
		spir_entry = (NCS_SPIR_ENTRY *)ncs_patricia_tree_getnext(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key);

		if ((spir_entry == NULL) ||
		    (0 !=
		     strcmp((const char *)spir_entry->key.sp_abstract_name, (const char *)spir_key.sp_abstract_name))) {
			/* There is no such entry. Reject this request */
			rc = NCSCC_RC_NO_OBJECT;
			goto quit;
		}

		/* Item found. Populate response and return */
		info->info.lookup_next_inst.o_next_environment_id = spir_entry->key.environment_id;
		info->info.lookup_next_inst.o_next_instance_name = spir_entry->key.instance_name;
		info->info.lookup_next_inst.o_arg = spir_entry->arg;
		info->info.lookup_next_inst.o_handle = spir_entry->handle;
		info->info.lookup_next_inst.o_user_se_api = splr_entry->user_se_api;
		break;

	case NCS_SPIR_REQ_LOOKUP_CREATE_INST:
		/* Check for presence of such an entry */
		spir_entry = (NCS_SPIR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key);

		if (spir_entry == NULL) {
			info->info.lookup_create_inst.o_created = true;
			/* There is no such entry. We will need to create one. */
			/* So, allocate a new entry */
			spir_entry = m_MMGR_ALLOC_NCS_SPIR_ENTRY;
			if (spir_entry == NULL) {
				rc = NCSCC_RC_OUT_OF_MEM;
				goto quit;
			}
			memset(spir_entry, 0, sizeof(*spir_entry));

			/* Set it up */
			spir_entry->pat_node.key_info = (uint8_t *)&spir_entry->key;
			spir_entry->key = spir_key;
			spir_entry->use_count = 0;

			/* Invoke the library's instantiation API */
			lib_req.i_op = NCS_LIB_REQ_INSTANTIATE;
			lib_req.info.inst.i_env_id = spir_entry->key.environment_id;
			lib_req.info.inst.i_inst_name.length = m_NTOH_SANAMET_LEN(spir_entry->key.instance_name.length);
			memcpy(lib_req.info.inst.i_inst_name.value,
			       spir_entry->key.instance_name.value, lib_req.info.inst.i_inst_name.length);
			lib_req.info.inst.o_arg = info->info.lookup_create_inst.o_arg;	/* o_arg should actually be io_arg? */
			lib_req.info.inst.i_inst_attrs = info->info.lookup_create_inst.i_inst_attrs;

			if ((*splr_entry->inst_api) (&lib_req) != NCSCC_RC_SUCCESS) {
				m_MMGR_FREE_NCS_SPIR_ENTRY(spir_entry);
				spir_entry = NULL;	/* precaution */
				rc = m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Instantiate api returns failure");
				m_NCSSPRR_TRACE_ARG4
				    ("SPIR:LOOKUP_CREATE_INST:FAILURE(instantiation):ab-name=%s:instance_name=%s:env-id=%d",
				     spir_key.sp_abstract_name, spir_key.instance_name.value, spir_key.environment_id);
				goto quit;
			}
			spir_entry->arg = lib_req.info.inst.o_arg;
			spir_entry->handle = lib_req.info.inst.o_inst_hdl;

			/* And finally add it to the SPIR tree */
			ncs_patricia_tree_add(&sprr_cb->spir_cb.spi_list, &spir_entry->pat_node);
		} else {
			info->info.lookup_create_inst.o_created = false;
		}
		spir_entry->use_count++;
		splr_entry->inst_count++;	/*Note:Splr-entry */
		/* There another service-user for this instance. Increment
		   the use-count */
		info->info.lookup_create_inst.o_arg = spir_entry->arg;
		info->info.lookup_create_inst.o_handle = spir_entry->handle;
		info->info.lookup_create_inst.o_user_se_api = splr_entry->user_se_api;
		m_NCSSPRR_TRACE_ARG6("SPIR:LOOKUP_CREATE_INST:SUCCESS:ab-name=%s:instance_name=%s:env-id=%d"
				     ":splr_entry->inst_count=%d:spir_entry->use_count=%d\n",
				     spir_key.sp_abstract_name, spir_key.instance_name.value,
				     spir_key.environment_id, splr_entry->inst_count, spir_entry->use_count);
		break;

	case NCS_SPIR_REQ_REL_INST:
		/* Check for presence of such an entry */
		spir_entry = (NCS_SPIR_ENTRY *)ncs_patricia_tree_get(&sprr_cb->spir_cb.spi_list, (uint8_t *)&spir_key);

		/* Do we need to check for instantiation flags here ? */
		if (spir_entry == NULL) {
			/* There is no such entry. Reject this request */
			rc = NCSCC_RC_NO_OBJECT;
			m_NCSSPRR_TRACE_ARG4("SPIR:REL_INST:FAILURE:ab-name=%s:instance_name=%s:env-id=%d\n",
					     spir_key.sp_abstract_name, spir_key.instance_name.value,
					     spir_key.environment_id);
			goto quit;
		}

		spir_entry->use_count--;
		splr_entry->inst_count--;	/*Note:Splr-entry */

		if (spir_entry->use_count == 0) {
			/* Invoke the library's uninstantiate API */
			lib_req.i_op = NCS_LIB_REQ_UNINSTANTIATE;
			lib_req.info.uninst.i_env_id = spir_entry->key.environment_id;
			lib_req.info.uninst.i_inst_name.length =
			    m_NTOH_SANAMET_LEN(spir_entry->key.instance_name.length);
			memcpy(lib_req.info.uninst.i_inst_name.value,
			       spir_entry->key.instance_name.value, lib_req.info.uninst.i_inst_name.length);
			lib_req.info.uninst.i_inst_hdl = spir_entry->handle;
			lib_req.info.uninst.i_arg = spir_entry->arg;

			(*splr_entry->inst_api) (&lib_req);	/* return value don't care */

			/* And finally add it to the SPIR tree */
			ncs_patricia_tree_del(&sprr_cb->spir_cb.spi_list, &spir_entry->pat_node);
			m_MMGR_FREE_NCS_SPIR_ENTRY(spir_entry);
			spir_entry = NULL;	/* precaution */
			m_NCSSPRR_TRACE_ARG6("SPIR:REL_INST:SUCCESS:ab-name=%s:instance_name=%s:env-id=%d"
					     ":splr_entry->inst_count=%d:spir_entry->use_count=%d\n",
					     spir_key.sp_abstract_name, spir_key.instance_name.value,
					     spir_key.environment_id, splr_entry->inst_count, 0);
		} else {
			m_NCSSPRR_TRACE_ARG6("SPIR:REL_INST:SUCCESS:ab-name=%s:instance_name=%s:env-id=%d"
					     ":splr_entry->inst_count=%d:spir_entry->use_count=%d\n",
					     spir_key.sp_abstract_name, spir_key.instance_name.value,
					     spir_key.environment_id, splr_entry->inst_count, spir_entry->use_count);
		}
		break;

	default:
		rc = NCSCC_RC_FAILURE;
		goto quit;
	}

 quit:
	m_NCS_UNLOCK(&sprr_cb->lock, NCS_LOCK_WRITE);
	ncshm_give_hdl(gl_sprr_hdl);

	return rc;
}

/****************************************************************************\
 * Function: sprr_create  (FILE-PRIVATE)
\****************************************************************************/
static uint32_t sprr_create(NCS_LIB_REQ_INFO *create_req)
{
	NCS_PATRICIA_PARAMS params;
	NCS_SPRR_CB *sprr_cb;

	if (gl_sprr_hdl != 0)
		/* SPRR handle is valid, implies SPRR is already created */
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPRR already created!");

	sprr_cb = m_MMGR_ALLOC_NCS_SPRR_CB;
	if (sprr_cb == NULL)
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Out of memory");
	memset(sprr_cb, 0, sizeof(*sprr_cb));

	/*  Start initialization of SPRR CB */

	/* STEP : Initialize CB's lock */
	m_NCS_LOCK_INIT(&sprr_cb->lock);

	/* STEP : Setup the SPLR database */
	memset(&params, 0, sizeof(params));
	params.key_size = sizeof(NCS_SPLR_KEY);
	if (ncs_patricia_tree_init(&sprr_cb->splr_cb.spl_list, &params)
	    != NCSCC_RC_SUCCESS) {
		m_NCS_LOCK_DESTROY(&sprr_cb->lock);
		m_MMGR_FREE_NCS_SPRR_CB(sprr_cb);
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "patricia init failed");
	}

	/* STEP : Setup the SPIR database */
	memset(&params, 0, sizeof(params));
	params.key_size = sizeof(NCS_SPIR_KEY);
	if (ncs_patricia_tree_init(&sprr_cb->spir_cb.spi_list, &params)
	    != NCSCC_RC_SUCCESS) {
		ncs_patricia_tree_destroy(&sprr_cb->splr_cb.spl_list);
		m_NCS_LOCK_DESTROY(&sprr_cb->lock);
		m_MMGR_FREE_NCS_SPRR_CB(sprr_cb);
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "patricia init failed");
	}

	/* Create a handle to SPRR database. SPRR API calls from this point
	   on will be honored */
	gl_sprr_hdl = ncshm_create_hdl(NCS_HM_POOL_ID_COMMON, NCS_SERVICE_ID_COMMON, sprr_cb);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************\
 * Function: sprr_create  (FILE-PRIVATE)
 *
 * Notes: Responsibility of ensuring 
\****************************************************************************/
static uint32_t sprr_destroy(NCS_LIB_REQ_INFO *destroy_req)
{
	NCS_SPRR_CB *sprr_cb;

	/* Verify handles etc. */
	sprr_cb = (NCS_SPRR_CB *)ncshm_take_hdl(NCS_SERVICE_ID_COMMON, gl_sprr_hdl);
	if (sprr_cb == NULL)
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPRR module not initialized");

	m_NCS_LOCK(&sprr_cb->lock, NCS_LOCK_WRITE);
	/* STEP : Check if SPLR (and automatically SPIR) list is empty. */
	if (NULL != ncs_patricia_tree_getnext(&sprr_cb->splr_cb.spl_list, NULL)) {
		/* SPLR list is non-empty. Destroy is not allowed */
		m_NCS_UNLOCK(&sprr_cb->lock, NCS_LOCK_WRITE);
		ncshm_give_hdl(gl_sprr_hdl);
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "SPRR in use (SPLR list non-empty)");
	}

	/* Start destruction */
	m_NCS_UNLOCK(&sprr_cb->lock, NCS_LOCK_WRITE);
	ncshm_give_hdl(gl_sprr_hdl);

	/* Destroy handle */
	sprr_cb = (NCS_SPRR_CB *)ncshm_destroy_hdl(NCS_SERVICE_ID_COMMON, gl_sprr_hdl);
	gl_sprr_hdl = 0;
	if (sprr_cb == NULL)
		return m_NCS_SPRR_DBG_SINK(NCSCC_RC_FAILURE, "Parellel destruction!");

	ncs_patricia_tree_destroy(&sprr_cb->spir_cb.spi_list);
	ncs_patricia_tree_destroy(&sprr_cb->splr_cb.spl_list);
	m_NCS_LOCK_DESTROY(&sprr_cb->lock);
	m_MMGR_FREE_NCS_SPRR_CB(sprr_cb);

	return NCSCC_RC_SUCCESS;
}

uint32_t ncs_environment_setup(uint32_t env_id)
{
	/* Stub the function until there is some meat to put in here */
	if ((env_id < 0) || (env_id >= NCSMDS_MAX_PWES))
		return NCSCC_RC_FAILURE;
	else
		return NCSCC_RC_SUCCESS;
}

uint32_t ncs_environment_clear(uint32_t env_id)
{
	/* Stub the function until there is some meat to put in here */
	if ((env_id < 0) || (env_id >= NCSMDS_MAX_PWES))
		return NCSCC_RC_FAILURE;
	else
		return NCSCC_RC_SUCCESS;
}

