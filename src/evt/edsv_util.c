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
*                                                                            *
*  MODULE NAME:  edsv_util.c                                                 *
*                                                                            *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module contains common utility routines for the NCS Event Service    *
*  library and server.                                                       *
*                                                                            *
*****************************************************************************/
#include "evt/agent/eda.h"
#include "base/logtrace.h"

/****************************************************************************
  Name          : edsv_copy_evt_pattern_array
 
  Description   : This routine makes a copy of an event pattern array.
 
  Arguments     : const SaEvtEventPatternArrayT *
                  SaAisErrorT *status
  Return Values : Returns the pointer to a newly allocated and copied event pattern
                  array.
 
  Notes         : 
******************************************************************************/
SaEvtEventPatternArrayT *edsv_copy_evt_pattern_array(const SaEvtEventPatternArrayT *src_pattern_array,
						     SaAisErrorT *error)
{
	SaEvtEventPatternArrayT *dst_pattern_array = NULL;
	SaEvtEventPatternT *src_pattern = NULL, *dst_pattern = NULL;
	uint16_t n = 0;
	TRACE_ENTER();
		/** Tracker for the number of patterns **/

	if (NULL == src_pattern_array) {
		*error = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("source pattern array is NULL");
		TRACE_LEAVE();
		return NULL;
	}

	/* Check if its greater than Implememtation Limit */
	if (src_pattern_array->patternsNumber > EDSV_MAX_PATTERNS) {
		*error = SA_AIS_ERR_TOO_BIG;
		TRACE_2("source patterns number is > %u", EDSV_MAX_PATTERNS);
		TRACE_LEAVE();
		return NULL;
	}

   /** Initial alloc for the destination pattern array
    **/
	if (NULL == (dst_pattern_array = m_MMGR_ALLOC_EVENT_PATTERN_ARRAY)) {
		*error = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("malloc failed for dest pattern array");
		TRACE_LEAVE();
		return NULL;
	}
   /** zero the memory
    **/
	memset(dst_pattern_array, '\0', sizeof(SaEvtEventPatternArrayT));

   /** Initialize the allocated number field
    ** This is ignored in the case of AttributesSet */

	dst_pattern_array->allocatedNumber = src_pattern_array->allocatedNumber;

   /** Initialize the pattern number field
    **/
	dst_pattern_array->patternsNumber = src_pattern_array->patternsNumber;

   /** Initialize the  patterns field
    **/
	if (src_pattern_array->patternsNumber != 0) {
		if (NULL ==
		    (dst_pattern_array->patterns =
		     m_MMGR_ALLOC_EVENT_PATTERNS((uint32_t)dst_pattern_array->patternsNumber))) {
			*error = SA_AIS_ERR_NO_MEMORY;
			m_MMGR_FREE_EVENT_PATTERN_ARRAY(dst_pattern_array);
			TRACE_4("malloc failed for dest pattern array");
			TRACE_LEAVE();
			return NULL;
		}

   /** zero the memory
    **/
		memset(dst_pattern_array->patterns, '\0',
		       dst_pattern_array->patternsNumber * sizeof(SaEvtEventPatternT));

		for (n = 0, src_pattern = src_pattern_array->patterns, dst_pattern = dst_pattern_array->patterns;
		     n < src_pattern_array->patternsNumber; n++, src_pattern++, dst_pattern++) {
			if (src_pattern == NULL) {
				*error = SA_AIS_ERR_NO_MEMORY;
				TRACE_2("source pattern is NULL");
				TRACE_LEAVE();
				return NULL;
			}
			/* Check if the pattern size is greater than local limit */
			if (src_pattern->patternSize > EDSV_MAX_PATTERN_SIZE) {
				*error = SA_AIS_ERR_TOO_BIG;
				m_MMGR_FREE_EVENT_PATTERNS(dst_pattern_array->patterns);
				m_MMGR_FREE_EVENT_PATTERN_ARRAY(dst_pattern_array);
				TRACE_2("patternSize > %u", EDSV_MAX_PATTERN_SIZE);
				TRACE_LEAVE();
				return NULL;
			}
      /** Assign the pattern size **/
			dst_pattern->patternSize = src_pattern->patternSize;
			if (dst_pattern->patternSize != 0) {
	/** Allocate memory for the individual pattern **/
				if (NULL == (dst_pattern->pattern = (SaUint8T *)
					     m_MMGR_ALLOC_EDSV_EVENT_DATA((uint32_t)dst_pattern->patternSize))) {
					*error = SA_AIS_ERR_NO_MEMORY;
					m_MMGR_FREE_EVENT_PATTERNS(dst_pattern_array->patterns);
					m_MMGR_FREE_EVENT_PATTERN_ARRAY(dst_pattern_array);
					TRACE_2("malloc failed for dest pattern");
					TRACE_LEAVE();
					return NULL;
				}
	 /** Clear memory for the allocated pattern **/
				memset(dst_pattern->pattern, '\0', (uint32_t)dst_pattern->patternSize);
	 /** Copy the pattern **/
				memcpy(dst_pattern->pattern, src_pattern->pattern, (uint32_t)dst_pattern->patternSize);
			} else if (dst_pattern->patternSize == 0) {
				dst_pattern->pattern = NULL;
			}
		}
	} /*end if patternsNumber == 0 */
	else
		dst_pattern_array->patterns = NULL;

   /** Return the destination pattern array 
    **/
	*error = SA_AIS_OK;
	TRACE_LEAVE();
	return dst_pattern_array;
}

/****************************************************************************
  Name          : edsv_free_evt_pattern_array
 
  Description   : This routine makes a copy of an event pattern array.
 
  Arguments     : const SaEvtEventPatternArrayT *

  Return Values : Frees up the pattern array provided.
 
  Notes         : 
******************************************************************************/
void edsv_free_evt_pattern_array(SaEvtEventPatternArrayT *free_pattern_array)
{
	TRACE_ENTER();

	if (NULL == free_pattern_array) {
		TRACE_2("null pattern array");
		TRACE_LEAVE();
		return;
	}

	eda_free_event_patterns(free_pattern_array->patterns, free_pattern_array->patternsNumber);
	m_MMGR_FREE_EVENT_PATTERN_ARRAY(free_pattern_array);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : edsv_copy_evt_filter_array
 
  Description   : This routine makes a copy of an event pattern array.
 
  Arguments     : const SaEvtEventFilterArrayT *
                  SaAisErrorT *

  Return Values : Returns the pointer to a newly allocated and copied event filter
                  array.
 
  Notes         : 
******************************************************************************/
SaEvtEventFilterArrayT *edsv_copy_evt_filter_array(const SaEvtEventFilterArrayT *src_filter_array, SaAisErrorT *error)
{
	SaEvtEventFilterArrayT *dst_filter_array = NULL;
	SaEvtEventFilterT *src_filter = NULL, *dst_filter = NULL;
	uint16_t n = 0;
	TRACE_ENTER();

		/** Tracker for the number of patterns **/

   /** Vaidate the passed in filter array **/
	if (NULL == src_filter_array) {
		*error = SA_AIS_ERR_INVALID_PARAM;
		TRACE_2("source filter array is NULL");
		TRACE_LEAVE();
		return NULL;
	}

	/* Is the filter number bigger than the implementation limit */
	if (src_filter_array->filtersNumber > EDSV_MAX_PATTERNS) {
		*error = SA_AIS_ERR_TOO_BIG;
		TRACE_2("filtersNumber is > %u", EDSV_MAX_PATTERNS);
		TRACE_LEAVE();
		return NULL;
	}
   /** Initial alloc for the destination filter array 
    **/
	if (NULL == (dst_filter_array = m_MMGR_ALLOC_FILTER_ARRAY)) {
		*error = SA_AIS_ERR_NO_MEMORY;
		TRACE_4("malloc failed for dest filter array");
		TRACE_LEAVE();
		return NULL;
	}

   /** zero the memory
    **/
	memset(dst_filter_array, '\0', (sizeof(SaEvtEventFilterArrayT)));

   /** Initialize the filters number field
    **/
	dst_filter_array->filtersNumber = src_filter_array->filtersNumber;

   /** Initialize the filters field
    **/
	if (src_filter_array->filtersNumber != 0) {
		if (NULL ==
		    (dst_filter_array->filters = m_MMGR_ALLOC_EVENT_FILTERS((uint32_t)dst_filter_array->filtersNumber))) {
			*error = SA_AIS_ERR_NO_MEMORY;
			m_MMGR_FREE_FILTER_ARRAY(dst_filter_array);
			TRACE_4("malloc failed for dest filter");
			TRACE_LEAVE();
			return NULL;
		}

      /** zero the memory
      **/
		n = src_filter_array->filtersNumber;
		memset(dst_filter_array->filters, '\0', n * sizeof(SaEvtEventFilterT));

		for (n = 0, src_filter = src_filter_array->filters, dst_filter = dst_filter_array->filters;
		     n < src_filter_array->filtersNumber; n++, src_filter++, dst_filter++) {
			/* Check if the filter size is greater than local limit */
			if (src_filter->filter.patternSize > EDSV_MAX_PATTERN_SIZE) {
				*error = SA_AIS_ERR_TOO_BIG;
				m_MMGR_FREE_EVENT_FILTERS(dst_filter_array->filters);
				m_MMGR_FREE_FILTER_ARRAY(dst_filter_array);
				TRACE_2("filter size > %u", EDSV_MAX_PATTERN_SIZE);
				TRACE_LEAVE();
				return NULL;
			}
	 /** Assign the filtertype and  pattern size **/
			dst_filter->filterType = src_filter->filterType;
			dst_filter->filter.patternSize = src_filter->filter.patternSize;
			if (dst_filter->filter.patternSize != 0) {
	   /** Allocate memory for the individual pattern **/
				if (NULL == (dst_filter->filter.pattern = (SaUint8T *)
					     m_MMGR_ALLOC_EDSV_EVENT_DATA((uint32_t)dst_filter->filter.patternSize))) {
					*error = SA_AIS_ERR_NO_MEMORY;
					m_MMGR_FREE_EVENT_FILTERS(dst_filter_array->filters);
					m_MMGR_FREE_FILTER_ARRAY(dst_filter_array);
					TRACE_4("malloc failed for filters");
					TRACE_LEAVE();
					return NULL;
				}
	   /** Clear memory for the allocated pattern **/
				memset(dst_filter->filter.pattern, '\0', (uint32_t)dst_filter->filter.patternSize);
	    /** Copy the pattern **/
				memcpy(dst_filter->filter.pattern,
				       src_filter->filter.pattern, (uint32_t)dst_filter->filter.patternSize);
			} else if (dst_filter->filter.patternSize == 0) {
				dst_filter->filter.pattern = NULL;
			}
		}
	} else {
		dst_filter_array->filters = NULL;
	}

  /** Return the destination pattern array 
  **/
	TRACE_LEAVE();
	return dst_filter_array;
}

/****************************************************************************
  Name          : edsv_free_evt_filter_array
 
  Description   : This routine makes a copy of an event pattern array.
 
  Arguments     : const SaEvtEventFilterArrayT *

  Return Values : Frees up the filter array provided
 
  Notes         : 
******************************************************************************/
void edsv_free_evt_filter_array(SaEvtEventFilterArrayT *free_filter_array)
{
	uint16_t n;
	SaEvtEventFilterT *free_filter;
	TRACE_ENTER();

	if (NULL == free_filter_array) {
		TRACE_2("null filter array");
		TRACE_LEAVE();
		return;
	}

	if (free_filter_array->filtersNumber != 0) {
		/* First Free the pattern storage area */
		for (free_filter = free_filter_array->filters, n = 0;
		     n < free_filter_array->filtersNumber; n++, free_filter++) {
			if (free_filter->filter.pattern)
				m_MMGR_FREE_EDSV_EVENT_DATA(free_filter->filter.pattern);
		}

		/* Now free the pattern array header */
		m_MMGR_FREE_EVENT_FILTERS(free_filter_array->filters);
	}
	m_MMGR_FREE_FILTER_ARRAY(free_filter_array);

	TRACE_LEAVE();
	return;
}

/****************************************************************************
  Name          : edsv_map_ais_prio_to_mds_snd_prio
 
  Description   : This routine spits out the equivalent MDS priority
                  for a given AIS Evt priority. 
 
  Arguments     : AIS EVT priority

  Return Values : MDS send priority
 
  Notes         : 
******************************************************************************/
MDS_SEND_PRIORITY_TYPE edsv_map_ais_prio_to_mds_snd_prio(uint32_t evt_prio)
{
	TRACE_ENTER2("event prio: %u", evt_prio);
	
	switch (evt_prio) {
	case SA_EVT_HIGHEST_PRIORITY:
		return MDS_SEND_PRIORITY_VERY_HIGH;
	case 1:
		return MDS_SEND_PRIORITY_HIGH;
	case 2:
		return MDS_SEND_PRIORITY_MEDIUM;
	case SA_EVT_LOWEST_PRIORITY:
		return MDS_SEND_PRIORITY_LOW;
	default:
		return MDS_SEND_PRIORITY_LOW;
	}

	TRACE_LEAVE();
}

void eda_free_event_patterns(SaEvtEventPatternT *patterns, SaSizeT patternsNumber)
{
	uint16_t n;
	SaEvtEventPatternT *free_pattern;
	TRACE_ENTER();

	if (patternsNumber != 0) {

		/* First Free the pattern storage area */
		for (free_pattern = patterns, n = 0; n < patternsNumber; n++, free_pattern++) {
			if (free_pattern->pattern)
				m_MMGR_FREE_EDSV_EVENT_DATA(free_pattern->pattern);
		}

		/* Now free the pattern array header */
		m_MMGR_FREE_EVENT_PATTERNS(patterns);
	}
	TRACE_LEAVE();
}
