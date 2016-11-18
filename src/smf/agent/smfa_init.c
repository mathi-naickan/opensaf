/*      -*- OpenSAF  -*-
*
* (C) Copyright 2010 The OpenSAF Foundation
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
* Author(s): GoAhead Software
*
*/

/*************************************************************************** 
 * @file	smfa_init.c
 * @brief	This file contains the smf agent init code. 
 *
 * @author	GoAhead Software
*****************************************************************************/
#include "smfa.h"

#define MAX_SMFA_HDL LONG_MAX

SMFA_CB _smfa_cb;
uint32_t smfa_use_count = 0;
static pthread_mutex_t smfa_lock = PTHREAD_MUTEX_INITIALIZER;

/*************************************************************************** 
@brief		: Init cb->cb_lock and register with MDS. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
*****************************************************************************/
uint32_t smfa_create()
{
	SMFA_CB *smfa_cb = &_smfa_cb;

	/* Initialize the cb lock.*/
	if (NCSCC_RC_SUCCESS != m_NCS_LOCK_INIT(&smfa_cb->cb_lock)){
		LOG_ER("SMFA: smfa_cb lock init FAILED.");
		return NCSCC_RC_FAILURE;
	}
	memset(smfa_cb,0,sizeof(SMFA_CB));
	/* Register with MDS.*/
	if (NCSCC_RC_SUCCESS != smfa_mds_register()){
		LOG_ER("SMFA: MDS registration FAILED.");
		m_NCS_LOCK_DESTROY(&smfa_cb->cb_lock);
		return NCSCC_RC_FAILURE;
	}
	/* TODO: We dont have any dependency on SMFND as far as agent init
	is concerned. Should we wait here till SMFND is up??*/

	return NCSCC_RC_SUCCESS;
}

/*************************************************************************** 
@brief		: Agent start up. This is done once per process i.e. agent cb.
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
*****************************************************************************/
uint32_t smfa_init()
{
	if (0 != pthread_mutex_lock(&smfa_lock)){
		LOG_ER("SMFA: pthread_mutex_lock FAILED.");
		return NCSCC_RC_FAILURE;
	}
	if (smfa_use_count > 0){
		/* Lib already created. Increment the count and return.*/
		smfa_use_count++;
		if (0 != pthread_mutex_unlock(&smfa_lock)){
			LOG_ER("SMFA: pthread_mutex_unlock FAILED.");
			 return NCSCC_RC_FAILURE;
		}
		return NCSCC_RC_SUCCESS;
	}

	if (NCSCC_RC_SUCCESS != ncs_agents_startup()){
		LOG_ER("SMFA: ncs_agents_startup FAILED.");
		pthread_mutex_unlock(&smfa_lock);
		return NCSCC_RC_FAILURE;
	}
	
	if (NCSCC_RC_SUCCESS != smfa_create()){
		LOG_ER("SMFA: smfa_create() FAILED.");
		pthread_mutex_unlock(&smfa_lock);
		return NCSCC_RC_FAILURE;
	}else {
		/* First time smfa lib getting initialized.*/
		smfa_use_count = 1;
	}
	if (0 != pthread_mutex_unlock(&smfa_lock)){
		LOG_ER("SMFA: pthread_mutex_unlock FAILED.");
		 return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;
}

/*************************************************************************** 
@brief		: MDS unregister and Destroy cb->cb_lock. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
*****************************************************************************/
uint32_t smfa_destroy()
{
	SMFA_CB *cb = &_smfa_cb;
	uint32_t rc;
	
	rc = smfa_mds_unregister();
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_ER("SMFA: MDS unregister FAILED.");
	}
	
	m_NCS_LOCK_DESTROY(&cb->cb_lock);
	return rc;
}

/*************************************************************************** 
@brief		: Agent shutdown only if all finalizes have been called. 
@return		: NCSCC_RC_FAILURE/NCSCC_RC_SUCCESS
*****************************************************************************/
uint32_t smfa_finalize()
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	SMFA_CB *cb = &_smfa_cb;

	if (0 != pthread_mutex_lock(&smfa_lock)){
		LOG_ER("SMFA: pthread_mutex_lock FAILED.");
		return NCSCC_RC_FAILURE;
	}
	smfa_use_count--;
	if (smfa_use_count){
		/* User still exists.*/
	}else{
		/* Last user and hence destroy the agent.*/
		cb->is_finalized = true;
		rc = smfa_destroy();
		ncs_agents_shutdown();
	}
	
	pthread_mutex_unlock(&smfa_lock);
	return rc;
}
