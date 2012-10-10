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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************

  DESCRIPTION:
  
  This file contains the IMMSv's service part IMMA's Init/Destory routines.
    
******************************************************************************/

#define _GNU_SOURCE
#include <string.h>

#include "imma.h"

/*****************************************************************************
 global data used by IMMA
 *****************************************************************************/

/* IMMA Agent creation specific LOCK */
static pthread_mutex_t imma_agent_lock = PTHREAD_MUTEX_INITIALIZER;

/* Protected by imma_agent_lock */
static uint32_t imma_use_count = 0;

IMMA_CB imma_cb;

/********************************************************************
 Name    :  imma_sync_with_immnd

 Description : This is for IMMA to sync with IMMND when it gets MDS callback

   This is needed to avoid a potential endless loop of TRY_AGAIN when
   the first client connects. The first client initializes the IMMA
   library. If IMMND is not up when the first need to send to IMMND
   arises, IMMA will reply with TRY_AGAIN, but IMMA will also
   shut down the library (since the initialize failed and this was 
   the first/last client.
   Repeat.
   Code cloned from CkpSv. 
**********************************************************************/

static void imma_sync_with_immnd(IMMA_CB *cb)
{
	NCS_SEL_OBJ_SET set;
	uint32_t timeout = 3000;
	TRACE_ENTER();

	m_NCS_LOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);

	if (cb->is_immnd_up)
	{
		m_NCS_UNLOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);
		return;
	}
	TRACE("Blocking first client");
	cb->immnd_sync_awaited = true;
	m_NCS_SEL_OBJ_CREATE(&cb->immnd_sync_sel);
	m_NCS_UNLOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);

	/* Await indication from MDS saying IMMND is up */
	m_NCS_SEL_OBJ_ZERO(&set);
	m_NCS_SEL_OBJ_SET(cb->immnd_sync_sel, &set);
	m_NCS_SEL_OBJ_SELECT(cb->immnd_sync_sel, &set, 0 , 0, &timeout);
	TRACE("Blocking wait released");

	/* Destroy the sync - object */
	m_NCS_LOCK(&cb->immnd_sync_lock,NCS_LOCK_WRITE);

	cb->immnd_sync_awaited = false;
	m_NCS_SEL_OBJ_DESTROY(cb->immnd_sync_sel);

	m_NCS_UNLOCK(&cb->immnd_sync_lock, NCS_LOCK_WRITE);

	TRACE_LEAVE();
	return;
}


/****************************************************************************
  Name          : imma_create
 
  Description   : This routine creates & initializes the IMMA control block.
 
  Arguments     : create_info - ptr to the create info
                  sv_id - service id, either NCSMDS_SVC_ID_IMMA_OM or
                          NCSMDS_SVC_ID_IMMA_OI.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t imma_create(NCSMDS_SVC_ID sv_id)
{
	IMMA_CB *cb = &imma_cb;
	uint32_t rc;


	char *value;

	/* Initialize trace system early  so we can see what is going on. 
	   This logic was moved away from a constructor to this initialization
	   routine because we want to avoid initializing the trace library for
	   processes that suck in the IMMA libraries as a side effect of the
	   build system, but actually never use it. 
	 */
	if ((value = getenv("IMMA_TRACE_PATHNAME")) && !trace_category_get()) {
		/* IMMA_TRACE_PATHNAME is defined and trace is not already initialized. */
		if (logtrace_init("imma", value, CATEGORY_ALL) != 0) {
			LOG_WA("Failed to initialize trace to %s in IMMA "
				"library", value);
			/* error, we cannot do anything */
		}
		LOG_NO("IMMA library TRACE initialize done pid:%u svid:%u file:%s", getpid(), sv_id, value);
	}

	/* initialize the imma cb lock */
	if (m_NCS_LOCK_INIT(&cb->cb_lock) != NCSCC_RC_SUCCESS) {
		TRACE_4("Failed to get cb lock");
		goto lock_init_fail;
	}

	/* Initalize the IMMA Trees & Linked lists */
	rc = imma_db_init(cb);
	if (rc != NCSCC_RC_SUCCESS) {
		/* No need to log here, already logged in imma_db_init */
		goto db_init_fail;
	}

	if (cb->sv_id != 0) {
		/*The osafassert below seems to occurr sometimes on some systems. */
		TRACE_4("cb->sv_id is NOT ZERO (%x) on first time entry IMMA svid:%x", cb->sv_id, sv_id);

		osafassert(cb->sv_id == 0);
	}

	cb->sv_id = sv_id;

	if (m_NCS_LOCK_INIT(&cb->immnd_sync_lock) != NCSCC_RC_SUCCESS) {
		TRACE_4("Failed to get immnd_sync_lock lock");
		goto mds_reg_fail;
	}

	/* register with MDS */
	if (imma_mds_register(cb) != NCSCC_RC_SUCCESS) {
		/* No need to log here, already logged in imma_mds_register  */
		goto mds_reg_fail;
	}

	imma_sync_with_immnd(cb); /* Needed to prevent endless TRY_AGAIN loop
								 for first client. */

	/* EDU initialisation ABT: Dont exactly know why we need this but... */
	if (m_NCS_EDU_HDL_INIT(&cb->edu_hdl) != NCSCC_RC_SUCCESS) {
		TRACE_3("Failed to initialize EDU handle");
		goto edu_init_fail;
	}

	TRACE("Client agent successfully initialized");
	return NCSCC_RC_SUCCESS;

 edu_init_fail:
	/*MDS unregister */
	imma_mds_unregister(cb);

 mds_reg_fail:
	cb->sv_id = 0;
	imma_db_destroy(cb);

 db_init_fail:
	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

 lock_init_fail:

	return NCSCC_RC_FAILURE;
}

/****************************************************************************
  Name          : imma_destroy
 
  Description   : This routine destroys the IMMA control block.
 
  Arguments     : destroy_info - ptr to the destroy info
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None
******************************************************************************/
static uint32_t imma_destroy(NCSMDS_SVC_ID sv_id)
{
	IMMA_CONTINUATION_RECORD *cont, *tmp;
	IMMA_CB *cb = &imma_cb;
	TRACE_ENTER();

	/* MDS unregister. */
	imma_mds_unregister(cb);

	m_NCS_EDU_HDL_FLUSH(&cb->edu_hdl);

	/* Destroy the IMMA database */
	osafassert(cb->sv_id == sv_id);
	cb->sv_id = 0;
	imma_db_destroy(cb);

	/* destroy the lock */
	m_NCS_LOCK_DESTROY(&cb->cb_lock);

	/* free possibly stored continuations */
	for (cont = cb->imma_continuations; cont != NULL;) {
		tmp = cont;
		cont = tmp->next;
		free(tmp);
	}

	/* and wipe what remains */
	memset(cb, 0, sizeof(*cb));

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          :  imma_startup

  Description   :  This routine creates a IMMSv agent infrastructure to 
                   interface with IMMSv service. Once the infrastructure is 
                   created from then on use_count is incremented for every 
                   startup request.

  Arguments     :  sv_id - service id, either NCSMDS_SVC_ID_IMMA_OM or
                           NCSMDS_SVC_ID_IMMA_OI.

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         :  None
******************************************************************************/
unsigned int imma_startup(NCSMDS_SVC_ID sv_id)
{
	unsigned int rc = NCSCC_RC_SUCCESS;

	int pt_err = pthread_mutex_lock(&imma_agent_lock);
	if(pt_err) {
		TRACE_4("Could not obtain mutex lock error(%u):%s", 
			pt_err, strerror(pt_err));
		rc = NCSCC_RC_FAILURE;
		goto done_nolock;
	}

	TRACE_ENTER2("use count %u", imma_use_count);

	if (imma_use_count > 0) {
		/* Already created, so just increment the use_count */
		imma_use_count++;
		goto done;
	}

	if ((rc = ncs_agents_startup()) != NCSCC_RC_SUCCESS) {
		TRACE_3("Agents_startup failed");
		goto done;
	}

	/* Init IMMA */
	if ((rc = imma_create(sv_id)) != NCSCC_RC_SUCCESS) {
		TRACE_3("Failure in startup of client agent");
		ncs_agents_shutdown();
		goto done;
	} else {
		imma_use_count = 1;
	}

 done:
	TRACE_LEAVE2("use count %u", imma_use_count);
	pt_err = pthread_mutex_unlock(&imma_agent_lock);
	if(pt_err) {
		TRACE_4("Could not release mutex lock error(%u):%s",
			pt_err, strerror(pt_err));
		rc = NCSCC_RC_FAILURE;
	}

 done_nolock:
	return rc;
}

/****************************************************************************
  Name          :  imma_shutdown 

  Description   :  This routine destroys the IMMSv agent infrastructure used
                   to interface IMMSv service. If the registered users 
                   are > 1, it just decrements the use_count.   

  Arguments     :  - NIL -

  Return Values :  NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         :  None
******************************************************************************/
unsigned int imma_shutdown(NCSMDS_SVC_ID sv_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	int pt_err = pthread_mutex_lock(&imma_agent_lock);
	if(pt_err) {
		TRACE_4("Could not obtain mutex lock error(%u):%s", 
			pt_err, strerror(pt_err));
		rc = NCSCC_RC_FAILURE;
		goto done_nolock;
	}

	TRACE_ENTER2("use count %u", imma_use_count);

	if (imma_use_count > 1) {
		/* Users still exist, just decrement the use count */
		imma_use_count--;
	} else if (imma_use_count == 1) {
		rc = imma_destroy(sv_id);
		ncs_agents_shutdown();
		imma_use_count = 0;
	}

	TRACE_LEAVE2("use count %u", imma_use_count);
	pt_err = pthread_mutex_unlock(&imma_agent_lock);
	if(pt_err) {
		TRACE_4("Could not release mutex lock error(%u):%s",
			pt_err, strerror(pt_err));
		rc = NCSCC_RC_FAILURE;
	}

 done_nolock:
	return rc;
}

void imma_freeAttrValue3(SaImmAttrValueT p, const SaImmValueTypeT attrValueType)
{
	SaAnyT *saAnyTp = NULL;
	SaStringT *saStringTp = NULL;

	switch (attrValueType) {
		case SA_IMM_ATTR_SAINT32T:
		case SA_IMM_ATTR_SAUINT32T:
		case SA_IMM_ATTR_SAINT64T:
		case SA_IMM_ATTR_SAUINT64T:
		case SA_IMM_ATTR_SATIMET:
		case SA_IMM_ATTR_SAFLOATT:
		case SA_IMM_ATTR_SADOUBLET:
		case SA_IMM_ATTR_SANAMET:
			break;

		case SA_IMM_ATTR_SASTRINGT:
			saStringTp = (SaStringT *)p;
			free(*saStringTp);
			break;
		case SA_IMM_ATTR_SAANYT:
			saAnyTp = (SaAnyT *)p;
			if (saAnyTp->bufferSize) {
				free(saAnyTp->bufferAddr);
			}
			break;

		default:
			TRACE_4("Incorrect value for SaImmValueTypeT:%u. "
				"Did you forget to set the attrValueType member in a "
				"SaImmAttrValuesT_2 value ?", attrValueType);
			abort();
	}

	free(p);
}

void imma_copyAttrValue(IMMSV_EDU_ATTR_VAL *p, const SaImmValueTypeT attrValueType, const SaImmAttrValueT attrValue)
{
	/*
	  Copies ONE attribute value. 
	  Multivalued attributes need to copy each value.
	*/
	SaUint32T valueSize = 0;

	SaAnyT *saAnyTp = 0;
	SaNameT *saNameTp = 0;
	SaStringT *saStringTp = 0;

	switch (attrValueType) {
		case SA_IMM_ATTR_SAINT32T:
			p->val.saint32 = *((SaInt32T *)attrValue);
			return;
		case SA_IMM_ATTR_SAUINT32T:
			p->val.sauint32 = *((SaUint32T *)attrValue);
			return;
		case SA_IMM_ATTR_SAINT64T:
			p->val.saint64 = *((SaInt64T *)attrValue);
			return;
		case SA_IMM_ATTR_SAUINT64T:
			p->val.sauint64 = *((SaUint64T *)attrValue);
			return;
		case SA_IMM_ATTR_SATIMET:
			p->val.satime = *((SaTimeT *)attrValue);
			return;
		case SA_IMM_ATTR_SAFLOATT:
			p->val.safloat = *((SaFloatT *)attrValue);
			return;
		case SA_IMM_ATTR_SADOUBLET:
			p->val.sadouble = *((SaDoubleT *)attrValue);
			return;

		case SA_IMM_ATTR_SANAMET:
			saNameTp = (SaNameT *)attrValue;
			if (saNameTp) {
				osafassert(saNameTp->length < SA_MAX_NAME_LENGTH);
				valueSize = strnlen((char *)saNameTp->value, SA_MAX_NAME_LENGTH) + 1;
				if (saNameTp->length + 1 < valueSize) {
					valueSize = saNameTp->length + 1;
				}
			} else {
				valueSize = 0;
			}
			break;

		case SA_IMM_ATTR_SASTRINGT:
			/*TODO: I should have some form of sanity check on length. */
			saStringTp = (SaStringT *)attrValue;
			valueSize = (saStringTp) ? strlen(*(saStringTp)) + 1 : 0;
			break;

		case SA_IMM_ATTR_SAANYT:
			saAnyTp = (SaAnyT *)attrValue;
			if(saAnyTp)
				valueSize = (saAnyTp->bufferAddr) ? (saAnyTp->bufferSize + 1) : 0;
			else
				valueSize = 0;
			break;

		default:
			TRACE_4("Incorrect value for SaImmValueTypeT:%u. "
				"Did you forget to set the attrValueType member in a "
				"SaImmAttrValuesT_2 value ?\n", attrValueType);
			abort();
	}

	p->val.x.size = valueSize;
	p->val.x.buf = (valueSize) ? malloc(valueSize) : NULL;

	if (attrValue && valueSize) {
		switch (attrValueType) {
			case SA_IMM_ATTR_SASTRINGT:
				(void)memcpy(p->val.x.buf, *saStringTp, valueSize);
				break;
			case SA_IMM_ATTR_SANAMET:
				(void)memcpy(p->val.x.buf, saNameTp->value, valueSize);
				break;
			case SA_IMM_ATTR_SAANYT:
				(void)memcpy(p->val.x.buf, saAnyTp->bufferAddr, valueSize - 1);
				break;
			default:
				abort();/*If I get here then I have introduced a bug 
					  somewhere above. */
		}
		p->val.x.buf[valueSize - 1] = '\0';
	} else {
		abort();
	}
}

SaImmAttrValueT imma_copyAttrValue3(const SaImmValueTypeT attrValueType, IMMSV_EDU_ATTR_VAL *attrValue)
{
	/*
	  Copies ONE attribute value. 
	  Multivalued attributes need to copy each value.
	  Allocates the root value on the heap. 
	  WARNING!, for dynamic/large sized data (SaStringT, SaAnyT) the buffer
	  is stolen from the attrValue.
	*/
	size_t valueSize = 0;
	SaAnyT *saAnyTp = 0;
	SaNameT *saNameTp = 0;
	SaStringT *saStringTp = 0;
	SaImmAttrValueT retVal = NULL;

	switch (attrValueType) {
		case SA_IMM_ATTR_SAINT32T:
			valueSize = sizeof(SaInt32T);
			break;
		case SA_IMM_ATTR_SAUINT32T:
			valueSize = sizeof(SaUint32T);
			break;
		case SA_IMM_ATTR_SAINT64T:
			valueSize = sizeof(SaInt64T);
			break;
		case SA_IMM_ATTR_SAUINT64T:
			valueSize = sizeof(SaUint64T);
			break;
		case SA_IMM_ATTR_SATIMET:
			valueSize = sizeof(SaTimeT);
			break;
		case SA_IMM_ATTR_SAFLOATT:
			valueSize = sizeof(SaFloatT);
			break;
		case SA_IMM_ATTR_SADOUBLET:
			valueSize = sizeof(SaDoubleT);
			break;
		case SA_IMM_ATTR_SANAMET:
			valueSize = sizeof(SaNameT);
			break;
		case SA_IMM_ATTR_SASTRINGT:
			valueSize = sizeof(SaStringT);
			break;
		case SA_IMM_ATTR_SAANYT:
			valueSize = sizeof(SaAnyT);
			break;

		default:
			TRACE_4("Illegal value type: %u", attrValueType);
			abort();
	}

	retVal = calloc(1, valueSize);

	switch (attrValueType) {
		case SA_IMM_ATTR_SAINT32T:
			*((SaInt32T *)retVal) = attrValue->val.saint32;
			break;
		case SA_IMM_ATTR_SAUINT32T:
			*((SaUint32T *)retVal) = attrValue->val.sauint32;
			break;
		case SA_IMM_ATTR_SAINT64T:
			*((SaInt64T *)retVal) = attrValue->val.saint64;
			break;
		case SA_IMM_ATTR_SAUINT64T:
			*((SaUint64T *)retVal) = attrValue->val.sauint64;
			break;
		case SA_IMM_ATTR_SATIMET:
			*((SaTimeT *)retVal) = attrValue->val.satime;
			break;
		case SA_IMM_ATTR_SAFLOATT:
			*((SaFloatT *)retVal) = attrValue->val.safloat;
			break;
		case SA_IMM_ATTR_SADOUBLET:
			*((SaDoubleT *)retVal) = attrValue->val.sadouble;
			break;

		case SA_IMM_ATTR_SANAMET:
			saNameTp = (SaNameT *)retVal;
			saNameTp->length = strnlen(attrValue->val.x.buf, attrValue->val.x.size);
			osafassert(saNameTp->length <= SA_MAX_NAME_LENGTH);
			memcpy(saNameTp->value, attrValue->val.x.buf, saNameTp->length);
			break;

		case SA_IMM_ATTR_SASTRINGT:
			saStringTp = (SaStringT *)retVal;/*pointer TO string-pointer. */
			/* Steal the buffer. */
			if (attrValue->val.x.size) {
				(*saStringTp) = attrValue->val.x.buf;
				attrValue->val.x.buf = NULL;
				attrValue->val.x.size = 0;
			} else {
				(*saStringTp) = NULL;
			}
			break;

		case SA_IMM_ATTR_SAANYT:
			saAnyTp = (SaAnyT *)retVal;
			/*Steal the value buffer. */
			saAnyTp->bufferSize = attrValue->val.x.size;
			if (attrValue->val.x.size) {
				/*Steal the buffer. */
				saAnyTp->bufferSize = attrValue->val.x.size - 1;
				saAnyTp->bufferAddr = (SaUint8T *)attrValue->val.x.buf;
				attrValue->val.x.buf = NULL;
				attrValue->val.x.size = 0;
			} else {
				saAnyTp->bufferSize = 0;
				saAnyTp->bufferAddr = NULL;
			}
			break;

		default:
			abort();
	}
	return retVal;
}
