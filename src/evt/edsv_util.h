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

  EDSV utils function prototype declarations
  
*******************************************************************************/

#ifndef EVT_EDSV_UTIL_H_
#define EVT_EDSV_UTIL_H_

SaEvtEventPatternArrayT *edsv_copy_evt_pattern_array(const SaEvtEventPatternArrayT *src_pattern_array,
							      SaAisErrorT *error);

void edsv_free_evt_pattern_array(SaEvtEventPatternArrayT *free_pattern_array);

SaEvtEventFilterArrayT *edsv_copy_evt_filter_array(const SaEvtEventFilterArrayT *src_filter_array,
							    SaAisErrorT *error);

void edsv_free_evt_filter_array(SaEvtEventFilterArrayT *free_filter_array);

MDS_SEND_PRIORITY_TYPE edsv_map_ais_prio_to_mds_snd_prio(uint32_t evt_prio);

uint32_t eds_calc_filter_size(SaEvtEventFilterArrayT *);

void eda_free_event_patterns(SaEvtEventPatternT *, SaSizeT);

/* Macro to fill in the attributes of a lost event */
#define m_EDSV_LOST_EVENT_FILL(m) \
do { \
   (m)->publisher_name.length = 4 ; \
   memset((m)->publisher_name.value,'\0',SA_MAX_NAME_LENGTH);\
   memcpy((m)->publisher_name.value,(SaUint8T *)"NULL",EDSV_DEF_NAME_LEN); \
   (m)->priority = SA_EVT_HIGHEST_PRIORITY; \
   m_NCS_OS_GET_TIME_STAMP(time_of_day); \
   (m)->publish_time = (SaTimeT)time_of_day; \
   (m)->retention_time = 0; \
   (m)->data_len = 0; \
   (m)->eda_event_id = SA_EVT_EVENTID_LOST; \
   (m)->pattern_array=m_MMGR_ALLOC_EVENT_PATTERN_ARRAY; \
   (m)->pattern_array->patternsNumber=1; \
   (m)->pattern_array->patterns=gl_lost_evt_pattern; \
   (m)->data = NULL; \
} while (0);

#endif  // EVT_EDSV_UTIL_H_
