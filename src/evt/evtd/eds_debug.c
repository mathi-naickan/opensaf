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
*  MODULE NAME:  eds_debug.c                                                 *
*                                                                            *
*                                                                            *
*  DESCRIPTION:                                                              *
*  This module contains debugging routines used during the development and   *
*  support of the NCS Event Distribution Service Server (EDS).               *
*                                                                            *
*****************************************************************************/
#include "eds.h"
#include "base/logtrace.h"

#define BUFF_SIZE_80    80
#define INT_WIDTH_8      8

/***************************************************************************
 *
 * eds_dump_event_patterns() - Debug routine to dump contents of
 *                             a SAF patternArray.
 *
 ***************************************************************************/
void eds_dump_event_patterns(SaEvtEventPatternArrayT *patternArray)
{
	SaEvtEventPatternT *pEventPattern;
	int32_t x = 0;
	int8_t buf[256];
	TRACE_ENTER();

	if (patternArray == NULL)
		return;
	if (patternArray->patterns == 0)
		return;

	pEventPattern = patternArray->patterns;	/* Point to first pattern */
	for (x = 0; x < (int32_t)patternArray->patternsNumber; x++) {
		memcpy(buf, pEventPattern->pattern, (uint32_t)pEventPattern->patternSize);
		buf[pEventPattern->patternSize] = '\0';
		TRACE("     pattern[%d] =    {%2u, \"%s\"}\n", x, (uint32_t)pEventPattern->patternSize, buf);
		pEventPattern++;
	}
	TRACE_LEAVE();
}

/***************************************************************************
 *
 * eds_dump_pattern_filter() - Dumps the values of a patternArray and
 *                             filterArray and compares them for a match
 *                             based on the filterType.
 *
 * Example Output:
 *
 * pattern {     jp, 2} ,  filter {  SA_EVT_PREFIX_FILTER,       j, 1} [MATCH]
 * pattern {  abcde, 5} ,  filter {  SA_EVT_SUFFIX_FILTER,  abyyyy, 6} [no]
 * pattern { abcxyz, 6} ,  filter {  SA_EVT_SUFFIX_FILTER,     xyz, 3} [MATCH]
 * pattern {   fooy, 4} ,  filter {   SA_EVT_EXACT_FILTER,    FOOY, 4} [no]
 * pattern {  futzy, 5} ,  filter {SA_EVT_PASS_ALL_FILTER, cracker, 7} [MATCH]
 * Match? NO
 *
 ***************************************************************************/
void eds_dump_pattern_filter(SaEvtEventPatternArrayT *patternArray, SaEvtEventFilterArrayT *filterArray)
{
	int32_t x;
	int32_t match = 0;
	uint8_t *p = NULL;
	SaEvtEventFilterT *filter;
	SaEvtEventPatternT *pattern;
	SaEvtEventPatternT emptyPattern = { 0, 0, NULL };
	int8_t buf[256];
	TRACE_ENTER();

	if ((patternArray == NULL) || (filterArray == NULL)) {
		TRACE_LEAVE();
		return;
	}

	pattern = patternArray->patterns;
	filter = filterArray->filters;

	TRACE("          PATTERN            FILTER\n");

	for (x = 1; x <= (int32_t)filterArray->filtersNumber; x++) {
		/* NULL terminate for printing */
		memcpy(buf, pattern->pattern, (uint32_t)pattern->patternSize);
		buf[(uint32_t)pattern->patternSize] = '\0';
		TRACE(" {%14s, %3u} ,  {", buf, (uint32_t)pattern->patternSize);

		match = 0;
		switch (filter->filterType) {
		case SA_EVT_PREFIX_FILTER:
			TRACE("  SA_EVT_PREFIX_FILTER");
			if (memcmp(filter->filter.pattern, pattern->pattern, (size_t)filter->filter.patternSize) == 0)
				match = 1;
			break;
		case SA_EVT_SUFFIX_FILTER:
			TRACE("  SA_EVT_SUFFIX_FILTER");
			/* Pattern must be at least as long as filter */
			if (pattern->patternSize < filter->filter.patternSize)
				break;	/* No match */

			/* Set p to offset into pattern */
			p = pattern->pattern + ((int)pattern->patternSize - (int)filter->filter.patternSize);
			if (memcmp(filter->filter.pattern, p, (size_t)filter->filter.patternSize) == 0)
				match = 1;
			break;
		case SA_EVT_EXACT_FILTER:
			TRACE("   SA_EVT_EXACT_FILTER");
			if (filter->filter.patternSize == pattern->patternSize)
				if (memcmp(filter->filter.pattern, pattern->pattern,
					   (size_t)filter->filter.patternSize) == 0)
					match = 1;
			break;
		case SA_EVT_PASS_ALL_FILTER:
			TRACE("SA_EVT_PASS_ALL_FILTER");
			match = 1;
			break;
		default:
			TRACE("ERROR: Unknown filterType(%d)\n", filter->filterType);
		}

		/* NULL terminate for printing */
		memcpy(buf, filter->filter.pattern, (uint32_t)filter->filter.patternSize);
		buf[(uint32_t)filter->filter.patternSize] = '\0';
		TRACE(", %14s, %3u}", buf, (uint32_t)filter->filter.patternSize);
		if (match)
			TRACE(" [MATCH]\n");
		else
			TRACE(" [no]\n");

		filter++;
		if (x < (int32_t)patternArray->patternsNumber)
			pattern++;
		else
			pattern = &emptyPattern;

	}			/* End for */

	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_dump_reglist() - DEBUG routine to dump the contents of the regList.
 *
 * This routine prints out all registration IDs and their associated
 * subscription IDs contained in a EDA_REG_LIST registration list.
 *
 ***************************************************************************/
void eds_dump_reglist()
{
	uint32_t num_registrations = 0;
	uint32_t num_subscriptions = 0;
	CHAN_OPEN_LIST *co = NULL;
	SUBSC_LIST *s = NULL;
	char *bp;
	EDA_REG_REC *rl;
	char buff[60];
	EDS_CB *cb = NULL;
	uint32_t temp;
	EDA_DOWN_LIST *down_rec = NULL;
	TRACE_ENTER();

	TRACE("\n-=-=-=-=- REGLIST SUBSCRIPTION IDs -=-=-=-=-\n");
	TRACE("     regIDs         Subscription IDs\n");

	/* Get the cb from the global handle */
	if (NULL == (cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		TRACE_LEAVE();
		return;
	}

	rl = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)0);

	while (rl) {		/* While there are registration entries... */
		num_registrations++;
		TRACE("%6u %-8s->", rl->reg_id, " ");
		buff[0] = '\0';
		bp = buff;

		co = rl->chan_open_list;
		while (co) {	/* While there are channelOpen entries... */
			s = co->subsc_list_head;
			while (s) {	/* While there are subscription entries... */
				num_subscriptions++;
				/* Loop, appending subscription IDs to string */
				snprintf(bp, sizeof(buff) - 1, " %9u ", s->subsc_rec->subscript_id);
				bp = buff + (strlen(buff));

				/* Close to end of buffer? */
				if ((bp - buff >= sizeof(buff) - 12) && ((s->next) || (co->next))) {	/* and something follows? */
					/* Output this chunk and start a new one */
					TRACE("%s\n", buff);
					bp = buff;

					/* Space over the continuation line */
					snprintf(bp, sizeof(buff) - 1, "%24s", " ");
					bp = buff + (strlen(buff));
				}
				s = s->next;
			}
			co = co->next;
		}
		TRACE("%s\n", buff);

		rl = (EDA_REG_REC *)ncs_patricia_tree_getnext(&cb->eda_reg_list, (uint8_t *)&rl->reg_id_Net);
	}
	temp = 0;
	down_rec = cb->eda_down_list_head;
	while (down_rec) {
		temp++;
		down_rec = down_rec->next;
	}

	TRACE("\nTotal Registrations:%d   Total Subscriptions:%d\n", num_registrations, num_subscriptions);
	TRACE("\nEDA DOWN recs : %d Async Update Count :%d \n", temp, cb->async_upd_cnt);

	/* Give back the handle */
	ncshm_give_hdl(gl_eds_hdl);
	TRACE_LEAVE();
}

/****************************************************************************
 *
 * eds_dump_worklist() - DEBUG routine to dump the contents of the workList.
 *
 * This routine prints out all channel IDs and their associated
 * subscription IDs contained in a EDS_WORKLIST work list.
 *
 ***************************************************************************/
void eds_dump_worklist()
{
	uint32_t num_events = 0;
	uint32_t num_subscriptions = 0;
	CHAN_OPEN_REC *co = NULL;
	SUBSC_REC *s = NULL;
	EDS_RETAINED_EVT_REC *retd_evt_rec = NULL;
	char *bp;
	char buff[BUFF_SIZE_80];
	SaUint8T list_iter;
	EDS_CB *eds_cb = NULL;
	EDS_WORKLIST *wp = NULL;
	EDS_CNAME_REC *cn = NULL;
	TRACE_ENTER();

	TRACE("\n-=-=-=-=- WORKLIST SUBSCRIPTION IDs -=-=-=-=-\n");
	TRACE("     Channel               Copenid:retained events\n");

	TRACE("   OpenID:regID            Subscription IDs\n");

	/* Get the cb from the global handle. */
	if (NULL == (eds_cb = (EDS_CB *)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		TRACE_LEAVE();
		return;
	}

	/* Get the worklist */
	wp = eds_cb->eds_work_list;

	while (wp) {		/* While there are channel entries... */
		num_events++;
		if (wp->chan_attrib & CHANNEL_UNLINKED)	/* Flag if channel is UnLinked */
			snprintf(buff, BUFF_SIZE_80 - 1, "u");
		else
			snprintf(buff, BUFF_SIZE_80 - 1, " ");
		TRACE("\n%s%6u %s", buff, wp->chan_id, wp->cname);

		TRACE("         ");
		for (list_iter = SA_EVT_HIGHEST_PRIORITY; list_iter <= SA_EVT_LOWEST_PRIORITY; list_iter++) {
			retd_evt_rec = wp->ret_evt_list_head[list_iter];

			TRACE("    P:%u", list_iter);
			while (retd_evt_rec) {
				TRACE("  %d:%u", retd_evt_rec->retd_evt_chan_open_id, retd_evt_rec->event_id);
				retd_evt_rec = retd_evt_rec->next;
			}
		}

		TRACE("\n");
		bp = buff;

		co = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)0);
		while (co) {	/* While there are channelOpen entries... */
			snprintf(bp, BUFF_SIZE_80 - 1, "%10d:%-6d", co->chan_open_id, co->reg_id);
			bp = buff + (strlen(buff));
			s = co->subsc_rec_head;
			while (s) {	/* While there are subscription entries... */
				num_subscriptions++;
				/* Loop, appending subscription IDs to string */
				snprintf(bp, BUFF_SIZE_80 - 1, " %12u", s->subscript_id);
				bp = buff + (strlen(buff));

				/* Close to end of buffer? */
				if ((bp - buff >= sizeof(buff) - (INT_WIDTH_8 * 2)) && (s->next)) {	/* and something follows? */
					/* Output this chunk and start a new one */
					TRACE("%s\n", buff);
					bp = buff;

					/* Space over the continuation line */
					snprintf(bp, BUFF_SIZE_80 - 1, "%25s", " ");
					bp = buff + (strlen(buff));
				}
				s = s->next;
			}
			TRACE("%s\n", buff);
			bp = buff;

			co = (CHAN_OPEN_REC *)ncs_patricia_tree_getnext(&wp->chan_open_rec, (uint8_t *)&co->copen_id_Net);
		}
		wp = wp->next;
	}

	TRACE("\nTotal Channel IDs:%d   Total Subscriptions:%d\n", num_events, num_subscriptions);

/* Code for CB DUMP */
	TRACE("\nEDS DATA BASE \n");
	cn = (EDS_CNAME_REC *)ncs_patricia_tree_getnext(&eds_cb->eds_cname_list, (uint8_t *)NULL);

	while (cn) {
		if (cn->wp_rec)
			TRACE("\nDB : %s                 Actual: %s", cn->chan_name.value, (cn->wp_rec)->cname);
		cn = (EDS_CNAME_REC *)ncs_patricia_tree_getnext(&eds_cb->eds_cname_list, (uint8_t *)&cn->chan_name);
	}

	/* Give back the handle */
	ncshm_give_hdl(gl_eds_hdl);

	TRACE_LEAVE();
}

/* End eds_debug.c */
