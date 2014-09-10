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
 * licensing terms.z
 *
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This module is the main include file for IMM Agent (IMMA).

*****************************************************************************/

#ifndef IMMA_H
#define IMMA_H

#include "immsv.h"
#include "imma_def.h"
#include "imma_cb.h"
#include "imma_proc.h"
#include "imma_mds.h"

extern IMMA_CB imma_cb;
extern const char *imma_sockname;

unsigned int imma_shutdown(NCSMDS_SVC_ID sv_id);
unsigned int imma_startup(NCSMDS_SVC_ID sv_id);
void imma_copyAttrValue(IMMSV_EDU_ATTR_VAL *p,
				 const SaImmValueTypeT attrValueType, const SaImmAttrValueT attrValue);
SaImmAttrValueT imma_copyAttrValue3(const SaImmValueTypeT attrValueType, IMMSV_EDU_ATTR_VAL *attrValue);
void imma_freeAttrValue(IMMSV_EDU_ATTR_VAL *p, const SaImmValueTypeT attrValueType);
void imma_freeAttrValue3(SaImmAttrValueT attrValue, const SaImmValueTypeT attrValueType);
void imma_freeSearchAttrs(SaImmAttrValuesT_2 **attr);
SaAisErrorT imma_evt_fake_evs(IMMA_CB *cb,
                       IMMSV_EVT *i_evt,
				       IMMSV_EVT **o_evt,
				       uint32_t timeout, SaImmHandleT immHandle, bool *locked, bool checkWritable);
SaAisErrorT imma_proc_check_stale(IMMA_CB *cb, SaImmHandleT immHandle,
    SaAisErrorT defaultEr);


#endif   /* IMMA_H */
