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

  MODULE NAME:       ncs_main_pub.c

  DESCRIPTION:       Contains an API that agent starts up NCS.

  TRACE GUIDE:
  Policy is to not use logging/syslog from library code.
  Only the trace part of logtrace is used from library. 

  It is possible to turn on trace for the IMMA library used
  by an application process. This is done by the application 
  defining the environment variable: IMMA_TRACE_PATHNAME.
  The trace will end up in the file defined by that variable.
 
  TRACE   debug traces                 - aproximates DEBUG  
  TRACE_1 normal but important events  - aproximates INFO.
  TRACE_2 user errors with return code - aproximates NOTICE.
  TRACE_3 unusual or strange events    - aproximates WARNING
  TRACE_4 library errors ERR_LIBRARY   - aproximates ERROR
******************************************************************************
*/

#include <configmake.h>

#include <pthread.h>
#include <dlfcn.h>

#include "ncsgl_defs.h"
#include "mds_papi.h"
#include "ncs_main_papi.h"
#include "ncs_mda_papi.h"
#include "ncs_lib.h"
#include "ncssysf_lck.h"
#include "mds_dl_api.h"
#include "sprr_dl_api.h"
#include "mda_dl_api.h"
#include "ncssysf_def.h"
#include "ncs_main_pub.h"
#include "osaf_utility.h"

#if (NCS_AVA == 1)
#include "ava_dl_api.h"
#endif

#if (NCS_AVND == 1)
#include "avnd_dl_api.h"
#endif

#if (NCS_MBCSV == 1)
#include "mbcsv_dl_api.h"
#endif

#if (NCS_GLA == 1)
#include "gla_dl_api.h"
#endif

#if (NCS_GLD == 1)
#include "gld_dl_api.h"
#endif

#if (NCS_GLND == 1)
#include "glnd_dl_api.h"
#endif

#if (NCS_MQA == 1)
#include "mqa_dl_api.h"
#endif

#if (NCS_MQD == 1)
#include "mqd_dl_api.h"
#endif

#if (NCS_CPA == 1)
#include "cpa_dl_api.h"
#endif

#if (NCS_CPD == 1)
#include "cpd_dl_api.h"
#endif

#if (NCS_CPND == 1)
#include "cpnd_dl_api.h"
#endif

#if (NCS_EDA == 1)
#include "eda_dl_api.h"
#endif

#if (NCS_EDS == 1)
#include "eds_dl_api.h"
#endif

/**************************************************************************\

       L O C A L      D A T A    S T R U C T U R E S

\**************************************************************************/

typedef uint32_t (*LIB_REQ) (NCS_LIB_REQ_INFO *);

typedef struct ncs_agent_data {
	uint32_t use_count;
	LIB_REQ lib_req;
} NCS_AGENT_DATA;

typedef struct ncs_main_pub_cb {
	void *lib_hdl;

	NCS_LOCK lock;
	uint32_t lock_create;
	bool core_started;
	uint32_t my_nodeid;
	uint32_t my_procid;

	uint32_t core_use_count;
	uint32_t leap_use_count;
	uint32_t mds_use_count;

	NCS_AGENT_DATA mbca;
} NCS_MAIN_PUB_CB;

typedef struct ncs_sys_params {
	NCS_PHY_SLOT_ID slot_id;
	NCS_CHASSIS_ID shelf_id;
	NCS_NODE_ID node_id;
	uint32_t cluster_id;
	uint32_t pcon_id;
} NCS_SYS_PARAMS;

static uint32_t mainget_node_id(uint32_t *node_id);
static uint32_t ncs_set_config_root(void);
static uint32_t ncs_util_get_sys_params(NCS_SYS_PARAMS *sys_params);
static uint32_t ncs_non_core_agents_startup(void);
static void ncs_get_sys_params_arg(NCS_SYS_PARAMS *sys_params);
static uint32_t ncs_update_sys_param_args(void);

static char ncs_config_root[MAX_NCS_CONFIG_FILEPATH_LEN + 1];

static NCS_MAIN_PUB_CB gl_ncs_main_pub_cb;

/* Global argument definitions */
char *gl_pargv[NCS_MAIN_MAX_INPUT];
uint32_t gl_pargc = 0;

/* mutex for synchronising agent startup and shutdown */
static pthread_mutex_t s_agent_startup_mutex = PTHREAD_MUTEX_INITIALIZER;

/* mutex protecting gl_ncs_main_pub_cb.core_started and
 * gl_ncs_main_pub_cb.core_use_count  */
static pthread_mutex_t s_leap_core_mutex = PTHREAD_MUTEX_INITIALIZER;

/***************************************************************************\

  PROCEDURE    :    ncs_agents_startup

\***************************************************************************/
unsigned int ncs_agents_startup(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = ncs_core_agents_startup();
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	rc = ncs_non_core_agents_startup();
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	return rc;
}

/***************************************************************************\

  PROCEDURE    :    ncs_agents_shutdown

\***************************************************************************/
unsigned int ncs_agents_shutdown(void)
{
	ncs_mbca_shutdown();
	ncs_core_agents_shutdown();

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************\

  PROCEDURE    :    ncs_leap_startup

\***************************************************************************/
unsigned int ncs_leap_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (gl_ncs_main_pub_cb.leap_use_count > 0) {
		gl_ncs_main_pub_cb.leap_use_count++;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_SUCCESS;
	}

	/* Print the process-id for information sakes */
	gl_ncs_main_pub_cb.my_procid = (uint32_t)getpid();
	TRACE("\nNCS:PROCESS_ID=%d\n", gl_ncs_main_pub_cb.my_procid);

	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	/* Initalize basic services */
	if (leap_env_init() != NCSCC_RC_SUCCESS) {
		TRACE_4("\nERROR: Couldn't initialised LEAP basic services \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}

	if (sprr_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		TRACE_4("\nERROR: SPRR lib_req failed \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}

	/* Get & Update system specific arguments */
	if (ncs_update_sys_param_args() != NCSCC_RC_SUCCESS) {
		TRACE_4("ERROR: Update System Param args \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}
	gl_ncs_main_pub_cb.leap_use_count = 1;

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	/* start initializing all the required agents */
	gl_ncs_main_pub_cb.lib_hdl = dlopen(NULL, RTLD_LAZY|RTLD_GLOBAL);

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************\

  PROCEDURE    :    ncs_mds_startup

\***************************************************************************/
unsigned int ncs_mds_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (!gl_ncs_main_pub_cb.leap_use_count) {
		TRACE_4("\nLEAP core not yet started.... \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}

	if (gl_ncs_main_pub_cb.mds_use_count > 0) {
		gl_ncs_main_pub_cb.mds_use_count++;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_SUCCESS;
	}

	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = gl_pargc;
	lib_create.info.create.argv = gl_pargv;

	/* STEP : Initialize the MDS layer */
	if (mds_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERROR: MDS lib_req failed \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}

	/* STEP : Initialize the ADA/VDA layer */
	if (mda_lib_req(&lib_create) != NCSCC_RC_SUCCESS) {
		TRACE_4("ERROR: MDA lib_req failed \n");
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return NCSCC_RC_FAILURE;
	}

	gl_ncs_main_pub_cb.mds_use_count = 1;

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************\

  PROCEDURE    :    ncs_non_core_agents_startup

\***************************************************************************/
uint32_t ncs_non_core_agents_startup(void)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	rc = ncs_mbca_startup();

	return rc;
}

/***************************************************************************\

  PROCEDURE    :    ncs_core_agents_startup

\***************************************************************************/
unsigned int ncs_core_agents_startup(void)
{
	osaf_mutex_lock_ordie(&s_leap_core_mutex);

	if (gl_ncs_main_pub_cb.core_use_count) {
		gl_ncs_main_pub_cb.core_use_count++;
		osaf_mutex_unlock_ordie(&s_leap_core_mutex);
		return NCSCC_RC_SUCCESS;
	}

	if (ncs_leap_startup() != NCSCC_RC_SUCCESS) {
		TRACE_4("ERROR: LEAP svcs startup failed \n");
		osaf_mutex_unlock_ordie(&s_leap_core_mutex);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	if (ncs_mds_startup() != NCSCC_RC_SUCCESS) {
		TRACE_4("ERROR: MDS startup failed \n");
		osaf_mutex_unlock_ordie(&s_leap_core_mutex);
		return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
	}

	gl_ncs_main_pub_cb.core_started = true;
	gl_ncs_main_pub_cb.core_use_count = 1;

	osaf_mutex_unlock_ordie(&s_leap_core_mutex);
	return NCSCC_RC_SUCCESS;
}

/***************************************************************************\

  PROCEDURE    :    ncs_mbca_startup

\***************************************************************************/
unsigned int ncs_mbca_startup(void)
{
	NCS_LIB_REQ_INFO lib_create;

	if (!gl_ncs_main_pub_cb.core_started) {
		TRACE_4("\nNCS core not yet started.... \n");
		return NCSCC_RC_FAILURE;
	}

	memset(&lib_create, 0, sizeof(lib_create));
	lib_create.i_op = NCS_LIB_REQ_CREATE;
	lib_create.info.create.argc = 0;
	lib_create.info.create.argv = NULL;

	if (gl_ncs_main_pub_cb.lib_hdl == NULL)
		return NCSCC_RC_SUCCESS;	/* No agents to load */

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (gl_ncs_main_pub_cb.mbca.use_count > 0) {
		/* Already created, so just increment the use_count */
		gl_ncs_main_pub_cb.mbca.use_count++;
	} else {
		gl_ncs_main_pub_cb.mbca.lib_req =
		    (LIB_REQ)dlsym(gl_ncs_main_pub_cb.lib_hdl, "mbcsv_lib_req");
		if (gl_ncs_main_pub_cb.mbca.lib_req == NULL) {
			TRACE_4("\nMBCSV:MBCA:OFF");
		} else {
			if ((*gl_ncs_main_pub_cb.mbca.lib_req) (&lib_create) != NCSCC_RC_SUCCESS) {
				osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
				return m_LEAP_DBG_SINK(NCSCC_RC_FAILURE);
			} else {
				TRACE("\nMBCSV:MBCA:ON");
				gl_ncs_main_pub_cb.mbca.use_count = 1;
			}
		}
	}

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	return NCSCC_RC_SUCCESS;
}

/***************************************************************************\

  PROCEDURE    :    ncs_mbca_shutdown

\***************************************************************************/
unsigned int ncs_mbca_shutdown(void)
{
	NCS_LIB_REQ_INFO lib_destroy;
	uint32_t rc = NCSCC_RC_SUCCESS;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);
	if (gl_ncs_main_pub_cb.mbca.use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		gl_ncs_main_pub_cb.mbca.use_count--;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return rc;
	}

	memset(&lib_destroy, 0, sizeof(lib_destroy));
	lib_destroy.i_op = NCS_LIB_REQ_DESTROY;
	lib_destroy.info.destroy.dummy = 0;;

	if (gl_ncs_main_pub_cb.mbca.lib_req != NULL)
		rc = (*gl_ncs_main_pub_cb.mbca.lib_req) (&lib_destroy);

	gl_ncs_main_pub_cb.mbca.use_count = 0;
	gl_ncs_main_pub_cb.mbca.lib_req = NULL;

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	return rc;
}

/***************************************************************************\

  PROCEDURE    :    ncs_leap_shutdown

\***************************************************************************/
void ncs_leap_shutdown()
{
	NCS_LIB_REQ_INFO lib_destroy;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (gl_ncs_main_pub_cb.leap_use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		gl_ncs_main_pub_cb.leap_use_count--;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return;
	}

	memset(&lib_destroy, 0, sizeof(lib_destroy));
	lib_destroy.i_op = NCS_LIB_REQ_DESTROY;
	lib_destroy.info.destroy.dummy = 0;

	dlclose(gl_ncs_main_pub_cb.lib_hdl);
	gl_ncs_main_pub_cb.lib_hdl = NULL;

	sprr_lib_req(&lib_destroy);

	leap_env_destroy();

	gl_ncs_main_pub_cb.leap_use_count = 0;
	gl_ncs_main_pub_cb.core_started = false;

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	return;
}

/***************************************************************************\

  PROCEDURE    :    ncs_mds_shutdown

\***************************************************************************/
void ncs_mds_shutdown()
{
	NCS_LIB_REQ_INFO lib_destroy;
	uint32_t tmp_ctr;

	osaf_mutex_lock_ordie(&s_agent_startup_mutex);

	if (gl_ncs_main_pub_cb.mds_use_count > 1) {
		/* Still users extis, so just decrement the use_count */
		gl_ncs_main_pub_cb.mds_use_count--;
		osaf_mutex_unlock_ordie(&s_agent_startup_mutex);
		return;
	}

	memset(&lib_destroy, 0, sizeof(lib_destroy));
	lib_destroy.i_op = NCS_LIB_REQ_DESTROY;
	lib_destroy.info.destroy.dummy = 0;

	mda_lib_req(&lib_destroy);
	mds_lib_req(&lib_destroy);

	gl_ncs_main_pub_cb.mds_use_count = 0;
	gl_ncs_main_pub_cb.core_started = false;

	for (tmp_ctr = 0; tmp_ctr < gl_pargc; tmp_ctr++)
		free(gl_pargv[tmp_ctr]);
	gl_pargc = 0;

	osaf_mutex_unlock_ordie(&s_agent_startup_mutex);

	return;
}

/***************************************************************************\

  PROCEDURE    :    ncs_core_agents_shutdown

\***************************************************************************/
unsigned int ncs_core_agents_shutdown()
{
	osaf_mutex_lock_ordie(&s_leap_core_mutex);

	if (!gl_ncs_main_pub_cb.core_use_count) {
		TRACE_4("\nNCS core not yet started.... \n");
		osaf_mutex_unlock_ordie(&s_leap_core_mutex);
		return NCSCC_RC_FAILURE;
	}

	if (gl_ncs_main_pub_cb.core_use_count > 1) {
		/* Decrement the use count */
		gl_ncs_main_pub_cb.core_use_count--;
		osaf_mutex_unlock_ordie(&s_leap_core_mutex);
		return NCSCC_RC_SUCCESS;
	}

	/* Shutdown basic services */
	ncs_mds_shutdown();
	ncs_leap_shutdown();
	gl_ncs_main_pub_cb.core_use_count = 0;

	osaf_mutex_unlock_ordie(&s_leap_core_mutex);
	return (NCSCC_RC_SUCCESS);
}

/***************************************************************************\

  PROCEDURE    :    ncs_get_node_id

\***************************************************************************/
NCS_NODE_ID ncs_get_node_id(void)
{
	if (!gl_ncs_main_pub_cb.leap_use_count) {
		return m_LEAP_DBG_SINK(0);
	}

	return gl_ncs_main_pub_cb.my_nodeid;
}

uint32_t file_get_word(FILE **fp, char *o_chword)
{
	int temp_char;
	unsigned int temp_ctr = 0;
 try_again:
	temp_ctr = 0;
	temp_char = getc(*fp);
	while ((temp_char != EOF) && (temp_char != '\n') && (temp_char != ' ') && (temp_char != '\0')) {
		o_chword[temp_ctr] = (char)temp_char;
		temp_char = getc(*fp);
		temp_ctr++;
	}
	o_chword[temp_ctr] = '\0';
	if (temp_char == EOF) {
		return (NCS_MAIN_EOF);
	}
	if (temp_char == '\n') {
		return (NCS_MAIN_ENTER_CHAR);
	}
	if (o_chword[0] == 0x0)
		goto try_again;
	return (0);
}

uint32_t file_get_string(FILE **fp, char *o_chword)
{
	int temp_char;
	unsigned int temp_ctr = 0;
 try_again:
	temp_ctr = 0;
	temp_char = getc(*fp);
	while ((temp_char != EOF) && (temp_char != '\n') && (temp_char != '\0')) {
		o_chword[temp_ctr] = (char)temp_char;
		temp_char = getc(*fp);
		temp_ctr++;
	}
	o_chword[temp_ctr] = '\0';
	if (temp_char == EOF) {
		return (NCS_MAIN_EOF);
	}
	if (temp_char == '\n') {
		return (NCS_MAIN_ENTER_CHAR);
	}
	if (o_chword[0] == 0x0)
		goto try_again;
	return (0);
}

uint32_t mainget_node_id(uint32_t *node_id)
{
	FILE *fp;
	char get_word[256];
	uint32_t res = NCSCC_RC_SUCCESS;
	uint32_t d_len, f_len;

#ifdef __NCSINC_LINUX__
#if (MDS_MULTI_HUB_PER_OS_INSTANCE == 1)
	{
		char *tmp = getenv("NCS_SIM_NODE_ID");
		if (tmp != NULL) {
			TRACE("\nNCS: Reading node_id(%s) from environment var.\n", tmp);
			*node_id = atoi(tmp);
			return NCSCC_RC_SUCCESS;
		}
	}
#endif
	d_len = strlen(ncs_config_root);
	f_len = strlen("/node_id");
	if ((d_len + f_len) >= MAX_NCS_CONFIG_FILEPATH_LEN) {
		TRACE_4("\n Filename too long \n");
		return NCSCC_RC_FAILURE;
	}
	/* Hack ncs_config_root to construct path */
	sprintf(ncs_config_root + d_len, "%s", "/node_id");

	/* LSB changes. Pick nodeid from PKGLOCALSTATEDIR */

	fp = fopen(NODE_ID_FILE, "r");

	/* Reverse hack ncs_config_root to original value */
	ncs_config_root[d_len] = 0;
#else
	fp = fopen("c:\\ncs\\node_id", "r");
#endif
	if (fp == NULL) {
		res = NCSCC_RC_FAILURE;
	} else {
		do {
			file_get_word(&fp, get_word);

			if (sscanf((const char *)&get_word, "%x", node_id) != 1) {
				res = NCSCC_RC_FAILURE;
				break;
			}
			fclose(fp);

		} while (0);
	}

	return (res);
}


static uint32_t ncs_set_config_root(void)
{
	char *tmp;
	static bool config_root_init = false;

	if (config_root_init == true)
		return NCSCC_RC_SUCCESS;
	else
		config_root_init = true;

	tmp = getenv("NCS_SIMULATION_CONFIG_ROOTDIR");
	if (tmp != NULL) {
		if (strlen(tmp) >= MAX_NCS_CONFIG_ROOTDIR_LEN) {
			TRACE_4("Config directory root name too long\n");
			return NCSCC_RC_FAILURE;
		}
		sprintf(ncs_config_root, "%s", tmp);

		TRACE("\nNCS: Using %s as config directory root\n", ncs_config_root);
	} else {
		sprintf(ncs_config_root, "/%s", NCS_DEF_CONFIG_FILEPATH);
	}

	return NCSCC_RC_SUCCESS;
}

uint32_t ncs_util_get_sys_params(NCS_SYS_PARAMS *sys_params)
{
	char *tmp_ptr;
	uint32_t res = NCSCC_RC_SUCCESS;

	memset(sys_params, 0, sizeof(NCS_SYS_PARAMS));

	if ((res = ncs_set_config_root()) != NCSCC_RC_SUCCESS) {
		TRACE_4("Unable to set config root \n");
		return NCSCC_RC_FAILURE;
	}

	if (mainget_node_id(&sys_params->node_id) != NCSCC_RC_SUCCESS) {
		TRACE_4("Not able to get the NODE ID\n");
		return (NCSCC_RC_FAILURE);
	}

	if ((tmp_ptr = getenv("NCS_PCON_ID")) != NULL) {
		sys_params->pcon_id = atoi(tmp_ptr);
	} else {
		sys_params->pcon_id = NCS_MAIN_DEF_PCON_ID;
	}

	return NCSCC_RC_SUCCESS;
}

void ncs_get_sys_params_arg(NCS_SYS_PARAMS *sys_params)
{
	char *p_field;
	uint32_t tmp_ctr;
	uint32_t orig_argc;
	NCS_SUB_SLOT_ID sub_slot_id = 0;
	NCS_SYS_PARAMS params;
	char *ptr;
	int argc = 0;
	char argv[256];

	orig_argc = gl_pargc;
	for (tmp_ctr = 0; tmp_ctr < NCS_MAX_INPUT_ARG_DEF; tmp_ctr++) {
		gl_pargv[(gl_pargc) + tmp_ctr] = (char *)malloc(NCS_MAX_STR_INPUT);
		memset(gl_pargv[(gl_pargc) + tmp_ctr], 0, NCS_MAX_STR_INPUT);
	}
	gl_pargc += tmp_ctr;

	/* Check argv[argc-1] through argv[1] */
	for (; argc > 1; argc--) {
		p_field = strstr(&argv[argc - 1], "NODE_ID=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("NODE_ID="), "%d", &params.node_id) == 1)
				sys_params->node_id = params.node_id;
			continue;
		} else
			p_field = strstr(&argv[argc - 1], "PCON_ID=");
		if (p_field != NULL) {
			if (sscanf(p_field + strlen("PCON_ID="), "%d", &params.pcon_id) == 1)
				sys_params->pcon_id = params.pcon_id;
			continue;
		} else {
			/* else store whatever comes */
			gl_pargv[gl_pargc] = (char *)malloc(NCS_MAX_STR_INPUT);
			memset(gl_pargv[gl_pargc], 0, NCS_MAX_STR_INPUT);
			strcpy(gl_pargv[gl_pargc], &argv[argc - 1]);
			gl_pargc = (gl_pargc) + 1;
		}
	}

	if ((ptr = getenv("NCS_ENV_NODE_ID")) != NULL)
		sys_params->node_id = atoi(ptr);

	TRACE("NCS:NODE_ID=0x%08X\n", sys_params->node_id);

	gl_ncs_main_pub_cb.my_nodeid = sys_params->node_id;

	if (m_NCS_GET_PHYINFO_FROM_NODE_ID(sys_params->node_id, &sys_params->shelf_id,
					   &sys_params->slot_id, &sub_slot_id) != NCSCC_RC_SUCCESS) {

		m_LEAP_DBG_SINK_VOID;
		return;
	}

	sprintf(gl_pargv[orig_argc + 0], "NONE");
	sprintf(gl_pargv[orig_argc + 1], "CLUSTER_ID=%d", sys_params->cluster_id);
	sprintf(gl_pargv[orig_argc + 2], "SHELF_ID=%d", sys_params->shelf_id);
	sprintf(gl_pargv[orig_argc + 3], "SLOT_ID=%d", sys_params->slot_id);
	sprintf(gl_pargv[orig_argc + 4], "NODE_ID=%d", sys_params->node_id);
	sprintf(gl_pargv[orig_argc + 5], "PCON_ID=%d", sys_params->pcon_id);

	return;
}

uint32_t ncs_update_sys_param_args(void)
{
	NCS_SYS_PARAMS sys_params;

	/* Get the system specific parameters */
	ncs_util_get_sys_params(&sys_params);

	/* Frame input arguments */
	ncs_get_sys_params_arg(&sys_params);

	return NCSCC_RC_SUCCESS;
}


/***************************************************************************\

  PROCEDURE    :    ncs_util_search_argv_list

\***************************************************************************/
char *ncs_util_search_argv_list(int argc, char *argv[], char *arg_prefix)
{
	char *tmp;

	/* Check   argv[argc-1] through argv[1] */
	for (; argc > 1; argc--) {
		tmp = strstr(argv[argc - 1], arg_prefix);
		if (tmp != NULL)
			return tmp;
	}
	return NULL;
}


/****************************************************************************
  Name          :  ncs_get_phyinfo_from_node_id

  Description   :  This function extracts chassis id,physical slot
                    id and sub slot id from node_id

  Arguments     :  i_node_id  -    node id
                   *o_chassis_id  - chassis id
                   *o_phy_slot_id - physical slot id
                   *o_sub_slot_id - slot id

  Return Values :  On Failure NCSCC_RC_FAILURE
                   On Success NCSCC_RC_SUCCESS

  Notes         :  None.
******************************************************************************/
uint8_t ncs_get_phyinfo_from_node_id(NCS_NODE_ID i_node_id, NCS_CHASSIS_ID *o_chassis_id,
				  NCS_PHY_SLOT_ID *o_phy_slot_id, NCS_SUB_SLOT_ID *o_sub_slot_id)
{
	if (o_sub_slot_id != NULL)
		*o_sub_slot_id = ((NCS_SUB_SLOT_ID)i_node_id);

	if (o_chassis_id != NULL)
		*o_chassis_id = (NCS_CHASSIS_ID)(i_node_id >> 16);

	if (o_phy_slot_id != NULL)
		*o_phy_slot_id = (NCS_PHY_SLOT_ID)(i_node_id >> 8);

	return NCSCC_RC_SUCCESS;
}
