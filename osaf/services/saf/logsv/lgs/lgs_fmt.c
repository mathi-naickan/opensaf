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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <logtrace.h>
#include <ncsgl_defs.h>
#include "lgs_fmt.h"

/**
 * 
 * @param inputString
 * @param numOfDigits
 * 
 * @return SaInt32T
 */
static SaInt32T checkFieldSize(SaStringT inputString, SaUint16T *numOfDigits)
{
	SaInt32T result = 0;
	SaInt8T dest[SA_MAX_NAME_LENGTH];
	char **endptr = NULL;

	(void)strcpy(dest, "");	/* Init dest */
	*numOfDigits = 0;

	/* Build string of characters */
	while ((isdigit(*inputString) != 0) && (*numOfDigits < SA_MAX_NAME_LENGTH)) {
		*numOfDigits = *numOfDigits + 1;
		(void)strncat(dest, inputString++, sizeof(SaInt8T));
	}

	result = strtol(dest, endptr, 0);
	return result;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param tokenFlags
 * @param twelveHourModeFlag
 * @param logStreamId
 * 
 * @return SaBoolT
 */
static SaBoolT validateComToken(SaStringT fmtExpPtr,
				SaUint16T *fmtExpPtrOffset,
				SaUint32T *tokenFlags, SaBoolT *twelveHourModeFlag, logStreamTypeT logStreamType)
{
	SaBoolT tokenOk = SA_TRUE;
	SaInt32T fieldSize = 0;
	SaUint16T fieldSizeOffset = 0;
	SaUint16T shiftOffset;
	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	switch (*fmtExpPtr++) {
	case C_LR_ID_LETTER:
		shiftOffset = (int)C_LR_ID_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_LR_TIME_STAMP_LETTER:
		shiftOffset = (int)C_LR_TIME_STAMP_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_HOUR_LETTER:
		shiftOffset = (int)C_TIME_STAMP_HOUR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_MINUTE_LETTER:
		shiftOffset = (int)C_TIME_STAMP_MINUTE_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_SECOND_LETTER:
		shiftOffset = (int)C_TIME_STAMP_SECOND_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_12_24_MODE_LETTER:
		shiftOffset = (int)C_TIME_STAMP_12_24_MODE_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		*twelveHourModeFlag = SA_TRUE;
		break;

	case C_TIME_STAMP_MONTH_LETTER:
		shiftOffset = (int)C_TIME_STAMP_MONTH_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_MON_LETTER:
		shiftOffset = (int)C_TIME_STAMP_MON_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_DAY_LETTER:
		shiftOffset = (int)C_TIME_STAMP_DAY_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_DAYN_LETTER:
		shiftOffset = (int)C_TIME_STAMP_DAYN_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_YEAR_LETTER:
		shiftOffset = (int)C_TIME_STAMP_YEAR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_TIME_STAMP_FULL_YEAR_LETTER:
		shiftOffset = (int)C_TIME_STAMP_FULL_YEAR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_NOTIFICATION_CLASS_ID_LETTER:
		shiftOffset = (int)C_NOTIFICATION_CLASS_ID_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_LR_TRUNCATION_INFO_LETTER:
		shiftOffset = (int)C_LR_TRUNCATION_INFO_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case C_LR_STRING_BODY_LETTER:
		shiftOffset = (int)C_LR_STRING_BODY_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case C_LR_HEX_CHAR_BODY_LETTER:
		shiftOffset = (int)C_LR_HEX_CHAR_BODY_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;

		/* This token is valid for application log streams only */
		if (logStreamType <= STREAM_TYPE_SYSTEM) {
			tokenOk = SA_FALSE;
		}
		break;

	default:		/* Non valid token letter */
		*fmtExpPtrOffset = 1;
		tokenOk = SA_FALSE;
		break;
	}

	return tokenOk;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param tokenFlags
 * @param twelveHourModeFlag
 * 
 * @return SaBoolT
 */
static SaBoolT validateNtfToken(SaStringT fmtExpPtr,
				SaUint16T *fmtExpPtrOffset, SaUint32T *tokenFlags, SaBoolT *twelveHourModeFlag)
{
	SaBoolT tokenOk = SA_TRUE;
	SaInt32T fieldSize = 0;
	SaUint16T fieldSizeOffset;
	SaUint16T shiftOffset = 0;
	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	switch (*fmtExpPtr++) {
	case N_NOTIFICATION_ID_LETTER:
		shiftOffset = (int)N_NOTIFICATION_ID_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_LETTER:
		shiftOffset = (int)N_EVENT_TIME_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_HOUR_LETTER:
		shiftOffset = (int)N_EVENT_TIME_HOUR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_MINUTE_LETTER:
		shiftOffset = (int)N_EVENT_TIME_MINUTE_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;
	case N_EVENT_TIME_SECOND_LETTER:
		shiftOffset = (int)N_EVENT_TIME_SECOND_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_12_24_MODE_LETTER:
		shiftOffset = (int)N_EVENT_TIME_12_24_MODE_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		*twelveHourModeFlag = SA_TRUE;
		break;

	case N_EVENT_TIME_MONTH_LETTER:
		shiftOffset = (int)N_EVENT_TIME_MONTH_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_MON_LETTER:
		shiftOffset = (int)N_EVENT_TIME_MON_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_DAY_LETTER:
		shiftOffset = (int)N_EVENT_TIME_DAY_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_DAYN_LETTER:
		shiftOffset = (int)N_EVENT_TIME_DAYN_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_YEAR_LETTER:
		shiftOffset = (int)N_EVENT_TIME_YEAR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TIME_FULL_YEAR_LETTER:
		shiftOffset = (int)N_EVENT_TIME_FULL_YEAR_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case N_EVENT_TYPE_LETTER:
		shiftOffset = (int)N_EVENT_TYPE_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case N_NOTIFICATION_OBJECT_LETTER:
		shiftOffset = (int)N_NOTIFICATION_OBJECT_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case N_NOTIFYING_OBJECT_LETTER:
		shiftOffset = (int)N_NOTIFYING_OBJECT_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	default:		/* Non valid token letter */
		*fmtExpPtrOffset = 1;
		tokenOk = SA_FALSE;
		break;
	}

	return tokenOk;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param tokenFlags
 * 
 * @return SaBoolT
 */
static SaBoolT validateSysToken(SaStringT fmtExpPtr, SaUint16T *fmtExpPtrOffset, SaUint32T *tokenFlags)
{
	SaBoolT tokenOk = SA_TRUE;
	SaInt32T fieldSize = 0;
	SaUint16T fieldSizeOffset = 0;
	SaUint16T shiftOffset;
	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	switch (*fmtExpPtr++) {
	case S_SEVERITY_ID_LETTER:
		shiftOffset = (int)S_SEVERITY_ID_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		break;

	case S_LOGGER_NAME_LETTER:
		shiftOffset = (int)S_LOGGER_NAME_SHIFT_OFFSET;
		if ((SaBoolT)((*tokenFlags >> shiftOffset) & 1) == SA_TRUE) {
			tokenOk = SA_FALSE;	/* Same token used two times */
		} else {
			*tokenFlags = (*tokenFlags | (1 << shiftOffset));
		}
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize > MAX_FIELD_SIZE) {
			tokenOk = SA_FALSE;
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	default:		/* Non valid token letter */
		*fmtExpPtrOffset = 1;
		tokenOk = SA_FALSE;
		break;
	}

	return tokenOk;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param truncationLetterPos
 * @param inputPos
 * @param twelveHourModeFlag
 * @param timeStampData
 * @param commonData
 * @param genHeader
 * 
 * @return SaStringT
 */
static int extractCommonField(char *dest, size_t dest_size,
			      SaStringT fmtExpPtr,
			      SaUint16T *fmtExpPtrOffset,
			      SaInt32T *truncationLetterPos,
			      SaInt32T inputPos,
			      SaUint32T logRecordIdCounter,
			      const SaBoolT *twelveHourModeFlag,
			      const struct tm *timeStampData, const SaLogRecordT *logRecord, SaUint16T rec_size)
{
	SaInt32T fieldSize;
	size_t stringSize;
	SaUint16T fieldSizeOffset = 0;
	int characters = 0;
	char *hex_string = NULL, *hex_string_ptr = NULL;
        int no_ch = 0, i;

	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	switch (*fmtExpPtr++) {	/* input points to a possible field size */
	case C_LR_ID_LETTER:
		stringSize = 11 * sizeof(char);
		characters = snprintf(dest, dest_size, "% 10d", (int)logRecordIdCounter);
		break;

	case C_LR_TIME_STAMP_LETTER:
		stringSize = 19 * sizeof(char);
		characters = snprintf(dest, dest_size, "%#016llx", logRecord->logTimeStamp);
		break;

	case C_TIME_STAMP_HOUR_LETTER:
		stringSize = 3 * sizeof(char);
		if (*twelveHourModeFlag == SA_TRUE) {
			characters = snprintf(dest, dest_size, "%02d", (timeStampData->tm_hour % 12));
		} else {
			characters = snprintf(dest, dest_size, "%02d", timeStampData->tm_hour);
		}
		break;

	case C_TIME_STAMP_MINUTE_LETTER:
		stringSize = 3 * sizeof(char);
		characters = snprintf(dest, dest_size, "%02d", timeStampData->tm_min);
		break;

	case C_TIME_STAMP_SECOND_LETTER:
		stringSize = 3 * sizeof(char);
		characters = snprintf(dest, dest_size, "%02d", timeStampData->tm_sec);
		break;

	case C_TIME_STAMP_12_24_MODE_LETTER:
		stringSize = 3 * sizeof(char);
		if (timeStampData->tm_hour >= 00 && timeStampData->tm_hour < 12) {
			characters = snprintf(dest, dest_size, "am");
		} else {
			characters = snprintf(dest, dest_size, "pm");
		}
		break;

	case C_TIME_STAMP_MONTH_LETTER:
		stringSize = 3 * sizeof(char);
		characters = snprintf(dest, dest_size, "%02d", (timeStampData->tm_mon + 1));
		break;

	case C_TIME_STAMP_MON_LETTER:
		stringSize = 4 * sizeof(char);
		switch (timeStampData->tm_mon) {
		case MONTH_JANUARY:
			characters = snprintf(dest, dest_size, "Jan");
			break;

		case MONTH_FEBRUARY:
			characters = snprintf(dest, dest_size, "Feb");
			break;

		case MONTH_MARCH:
			characters = snprintf(dest, dest_size, "Mar");
			break;

		case MONTH_APRIL:
			characters = snprintf(dest, dest_size, "Apr");
			break;

		case MONTH_MAY:
			characters = snprintf(dest, dest_size, "May");
			break;

		case MONTH_JUNE:
			characters = snprintf(dest, dest_size, "Jun");
			break;

		case MONTH_JULY:
			characters = snprintf(dest, dest_size, "Jul");
			break;

		case MONTH_AUGUST:
			characters = snprintf(dest, dest_size, "Aug");
			break;

		case MONTH_SEPTEMBER:
			characters = snprintf(dest, dest_size, "Sep");
			break;

		case MONTH_OCTOBER:
			characters = snprintf(dest, dest_size, "Oct");
			break;

		case MONTH_NOVEMBER:
			characters = snprintf(dest, dest_size, "Nov");
			break;

		case MONTH_DECEMBER:
			characters = snprintf(dest, dest_size, "Dec");
			break;

		default:
			(void)strcpy(dest, "");
			break;

		}

		break;
		
	case C_TIME_STAMP_DAY_LETTER:
		stringSize = 3 * sizeof(char);
		characters = snprintf(dest, dest_size, "%02d", (timeStampData->tm_mday));
		break;

	case C_TIME_STAMP_DAYN_LETTER:
		stringSize = 3 * sizeof(char);
		switch (timeStampData->tm_wday) {
		case DAY_SUNDAY:
			characters = snprintf(dest, dest_size, "Sun");
			break;

		case DAY_MONDAY:
			characters = snprintf(dest, dest_size, "Mon");
			break;

		case DAY_TUESDAY:
			characters = snprintf(dest, dest_size, "Tue");
			break;

		case DAY_WEDNESDAY:
			characters = snprintf(dest, dest_size, "Wed");
			break;

		case DAY_THURSDAY:
			characters = snprintf(dest, dest_size, "Thu");
			break;

		case DAY_FRIDAY:
			characters = snprintf(dest, dest_size, "Fri");
			break;

		case DAY_SATURDAY:
			characters = snprintf(dest, dest_size, "Sat");
			break;

		default:
			(void)strcpy(dest, "");
			break;
		}
		break;

	case C_TIME_STAMP_YEAR_LETTER:
		stringSize = 3 * sizeof(char);
		characters = strftime(dest, dest_size, "%y", timeStampData);
		break;

	case C_TIME_STAMP_FULL_YEAR_LETTER:
		stringSize = 5 * sizeof(char);
		characters = strftime(dest, dest_size, "%Y", timeStampData);
		break;

	case C_NOTIFICATION_CLASS_ID_LETTER:
		stringSize = 30 * sizeof(char);
		if (logRecord->logHdrType == SA_LOG_GENERIC_HEADER) {
			characters = snprintf(dest, dest_size,
					"NCI[0x%#08x,0x%#04x,0x%#04x]",
					(unsigned int)logRecord->logHeader.genericHdr.notificationClassId->
					vendorId, logRecord->logHeader.genericHdr.notificationClassId->majorId,
					logRecord->logHeader.genericHdr.notificationClassId->minorId);
		} else
			characters = snprintf(dest, dest_size,
					"NCI[0x%08x,0x%04x,0x%04x]",
					(unsigned int)logRecord->logHeader.ntfHdr.notificationClassId->vendorId,
					logRecord->logHeader.ntfHdr.notificationClassId->majorId,
					logRecord->logHeader.ntfHdr.notificationClassId->minorId);

		break;

	case C_LR_TRUNCATION_INFO_LETTER:
		stringSize = 2 * sizeof(char);
		/* A space inserted at the truncationCharacter:s position */
		characters = snprintf(dest, dest_size, " ");
		if ((inputPos + 1) == rec_size)
			*truncationLetterPos = inputPos - 1;
		else
			*truncationLetterPos = inputPos;        /* The position of the truncation 
                                                           character in the log record */
		break;

	case C_LR_STRING_BODY_LETTER:
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		stringSize = logRecord->logBuffer->logBufSize + 1;
		if (fieldSize == 0) {	/* Copy whole body */

			if (stringSize > dest_size)
				stringSize = dest_size;
			characters = snprintf(dest, stringSize, "%s", (SaStringT)logRecord->logBuffer->logBuf);
		} else {	/* Truncate or pad the body with blanks until fieldSize */
			characters = snprintf(dest, dest_size,
					      "%*.*s",
					      (int)-fieldSize, (int)fieldSize, (SaStringT)logRecord->logBuffer->logBuf);
		}

		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case C_LR_HEX_CHAR_BODY_LETTER:
		stringSize = logRecord->logBuffer->logBufSize;
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		hex_string = (char *)malloc(2 * stringSize + 1);

		if (hex_string == NULL){
			osafassert(0);
		}

		hex_string_ptr = hex_string;

		for (i = 0; i < stringSize; i++) {
			no_ch = sprintf(hex_string_ptr, "%x", (int)logRecord->logBuffer->logBuf[i]);
			hex_string_ptr = hex_string_ptr + no_ch;
		}
		*hex_string_ptr = '\0';

		if (fieldSize == 0) {

			characters = snprintf(dest, dest_size, "%s", hex_string);
		} else {

			characters = snprintf(dest, dest_size, "%*.*s", (int)fieldSize, (int)fieldSize, hex_string);
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		free(hex_string);
		break;

	default:
		characters = 0;
		break;

	}

	/* Error */
	if (characters == -1)
		characters = 0;

	if (characters > dest_size)
		characters = dest_size;

	return characters;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param twelveHourModeFlag
 * @param commonData
 * @param ntfHeader
 * 
 * @return SaStringT
 */
static int extractNotificationField(char *dest, size_t dest_size,
				    SaStringT fmtExpPtr,
				    SaUint16T *fmtExpPtrOffset,
				    const SaBoolT *twelveHourModeFlag, const SaLogRecordT *logRecord)
{
	struct tm *eventTimeData;
	SaTimeT totalTime;
	SaInt32T fieldSize;
	SaInt32T characters = 0;
	SaUint16T fieldSizeOffset = 0;

	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	/* Convert event time into */
	totalTime = (logRecord->logHeader.ntfHdr.eventTime / (SaTimeT)SA_TIME_ONE_SECOND);

	/* Split timestamp in timeStampData */
	struct tm tm_info;
	eventTimeData = localtime_r((const time_t *)&totalTime, &tm_info);
	osafassert(eventTimeData);

	switch (*fmtExpPtr++) {
	case N_NOTIFICATION_ID_LETTER:
		characters = snprintf(dest, dest_size, "0x%#016llx", logRecord->logHeader.ntfHdr.notificationId);
		break;

	case N_EVENT_TIME_LETTER:
		characters = snprintf(dest, dest_size, "%#016llx", logRecord->logHeader.ntfHdr.eventTime);
		break;

	case N_EVENT_TIME_HOUR_LETTER:
		if (*twelveHourModeFlag == SA_TRUE) {
			characters = snprintf(dest, dest_size, "%02d", (eventTimeData->tm_hour % 12));
		} else {
			characters = snprintf(dest, dest_size, "%02d", eventTimeData->tm_hour);
		}
		break;

	case N_EVENT_TIME_MINUTE_LETTER:
		characters = snprintf(dest, dest_size, "%02d", eventTimeData->tm_min);
		break;

	case N_EVENT_TIME_SECOND_LETTER:
		characters = snprintf(dest, dest_size, "%02d", eventTimeData->tm_sec);
		break;

	case N_EVENT_TIME_12_24_MODE_LETTER:
		if (eventTimeData->tm_hour >= 00 && eventTimeData->tm_hour < 12) {
			characters = snprintf(dest, dest_size, "am");
		} else {
			characters = snprintf(dest, dest_size, "pm");
		}
		break;

	case N_EVENT_TIME_MONTH_LETTER:
		characters = snprintf(dest, dest_size, "%02d", (eventTimeData->tm_mon + 1));
		break;

	case N_EVENT_TIME_MON_LETTER:
		switch (eventTimeData->tm_mon) {
		case MONTH_JANUARY:
			characters = snprintf(dest, dest_size, "Jan");
			break;

		case MONTH_FEBRUARY:
			characters = snprintf(dest, dest_size, "Feb");
			break;

		case MONTH_MARCH:
			characters = snprintf(dest, dest_size, "Mar");
			break;

		case MONTH_APRIL:
			characters = snprintf(dest, dest_size, "Apr");
			break;

		case MONTH_MAY:
			characters = snprintf(dest, dest_size, "May");
			break;

		case MONTH_JUNE:
			characters = snprintf(dest, dest_size, "Jun");
			break;

		case MONTH_JULY:
			characters = snprintf(dest, dest_size, "Jul");
			break;

		case MONTH_AUGUST:
			characters = snprintf(dest, dest_size, "Aug");
			break;

		case MONTH_SEPTEMBER:
			characters = snprintf(dest, dest_size, "Sep");
			break;

		case MONTH_OCTOBER:
			characters = snprintf(dest, dest_size, "Oct");
			break;

		case MONTH_NOVEMBER:
			characters = snprintf(dest, dest_size, "Nov");
			break;

		case MONTH_DECEMBER:
			characters = snprintf(dest, dest_size, "Dec");
			break;

		default:
			(void)strcpy(dest, "");
			break;

		}
		break;

	case N_EVENT_TIME_DAYN_LETTER:
		switch (eventTimeData->tm_wday) {
		case DAY_SUNDAY:
			characters = snprintf(dest, dest_size, "Sun");
			break;

		case DAY_MONDAY:
			characters = snprintf(dest, dest_size, "Mon");
			break;

		case DAY_TUESDAY:
			characters = snprintf(dest, dest_size, "Tue");
			break;

		case DAY_WEDNESDAY:
			characters = snprintf(dest, dest_size, "Wed");
			break;

		case DAY_THURSDAY:
			characters = snprintf(dest, dest_size, "Thu");
			break;

		case DAY_FRIDAY:
			characters = snprintf(dest, dest_size, "Fri");
			break;

		case DAY_SATURDAY:
			characters = snprintf(dest, dest_size, "Sat");
			break;

		default:
			(void)strcpy(dest, "");
			break;
		}
		break;

	case N_EVENT_TIME_YEAR_LETTER:
		characters = snprintf(dest, dest_size, "%02d",
				eventTimeData->tm_year - (YEAR_2000 - START_YEAR));
		break;

	case N_EVENT_TIME_FULL_YEAR_LETTER:
		characters = snprintf(dest, dest_size, "%d", (eventTimeData->tm_year + START_YEAR));
		break;

	case N_EVENT_TYPE_LETTER:
		/* Check field size */
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize == 0) {
			characters = snprintf(dest, dest_size, "%#x", logRecord->logHeader.ntfHdr.eventType);

		} else {

			/* TODO!!! Fit hex output to size => two steps for non strings */
                	/* 0x included in total field size */
			fieldSize = (fieldSize > 2) ? (fieldSize - 2) : 2;
			characters = snprintf(dest, dest_size,
                                      "%#.*x", fieldSize,
										logRecord->logHeader.ntfHdr.eventType);
		}

		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case N_NOTIFICATION_OBJECT_LETTER:
		/* Check field size and trunkate alternative pad with blanks */
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize == 0) {
			characters = snprintf(dest, dest_size, "%s",
						logRecord->logHeader.ntfHdr.notificationObject->value);
		} else {
			characters = snprintf(dest, dest_size,
						"%*.*s",
						(int) -fieldSize,
						(int) fieldSize, logRecord->logHeader.ntfHdr.notificationObject->value);
		}

		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case N_NOTIFYING_OBJECT_LETTER:
		/* Check field size and trunkate alternative pad with blanks */
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize == 0) {
			characters = snprintf(dest,
						dest_size, "%s", logRecord->logHeader.ntfHdr.notifyingObject->value);
		} else {
			characters = snprintf(dest, dest_size,
						"%*.*s",
						(int) -fieldSize,
						(int) fieldSize, logRecord->logHeader.ntfHdr.notifyingObject->value);
		}

		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	default:
		characters = 0;
		break;
	}

	/* Error */
	if (characters == -1)
		characters = 0;

	if (characters > dest_size)
		characters = dest_size;

	return characters;
}

/**
 * 
 * @param fmtExpPtr
 * @param fmtExpPtrOffset
 * @param commonData
 * @param genHeader
 * 
 * @return SaStringT
 */
static int extractSystemField(char *dest, size_t dest_size,
			      SaStringT fmtExpPtr, SaUint16T *fmtExpPtrOffset, const SaLogRecordT *logRecord)
{
	SaInt32T fieldSize;
	SaInt32T characters = 0;
	SaUint16T fieldSizeOffset = 0;
	*fmtExpPtrOffset = DEFAULT_FMT_EXP_PTR_OFFSET;

	switch (*fmtExpPtr++) {	/* input points to a possible field size */
	case S_LOGGER_NAME_LETTER:
		fieldSize = checkFieldSize(fmtExpPtr, &fieldSizeOffset);
		if (fieldSize != 0) {
			characters = snprintf(dest, dest_size,
					      "%*.*s",
					      (int)-fieldSize,
					      (int)fieldSize, logRecord->logHeader.genericHdr.logSvcUsrName->value);
		} else {
			characters = snprintf(dest, dest_size,
					      "%s", logRecord->logHeader.genericHdr.logSvcUsrName->value);
		}
		*fmtExpPtrOffset = *fmtExpPtrOffset + fieldSizeOffset;
		break;

	case S_SEVERITY_ID_LETTER:
		switch (logRecord->logHeader.genericHdr.logSeverity) {
		case SA_LOG_SEV_EMERGENCY:
			characters = snprintf(dest, dest_size, "EM");
			break;

		case SA_LOG_SEV_ALERT:
			characters = snprintf(dest, dest_size, "AL");
			break;

		case SA_LOG_SEV_CRITICAL:
			characters = snprintf(dest, dest_size, "CR");
			break;

		case SA_LOG_SEV_ERROR:
			characters = snprintf(dest, dest_size, "ER");
			break;

		case SA_LOG_SEV_WARNING:
			characters = snprintf(dest, dest_size, "WA");
			break;

		case SA_LOG_SEV_NOTICE:
			characters = snprintf(dest, dest_size, "NO");
			break;

		case SA_LOG_SEV_INFO:
			characters = snprintf(dest, dest_size, "IN");
			break;

		default:
			(void)strcpy(dest, "");
			break;

		}
		break;

	default:
		(void)strcpy(dest, "");
		break;

	}

	/* Error */
	if (characters == -1)
		characters = 0;

	if (characters > dest_size)
		characters = dest_size;

	return characters;
}

/**
 * Validate a format expression
 * @param formatExpression
 * @param logStreamType
 * @param twelveHourModeFlag
 * 
 * @return SaBoolT
 */
SaBoolT lgs_is_valid_format_expression(const SaStringT formatExpression,
				       logStreamTypeT logStreamType, SaBoolT *twelveHourModeFlag)
{
	SaBoolT formatExpressionOk = SA_FALSE;
	SaBoolT tokenOk = SA_FALSE;
	SaUint32T comTokenFlags = 0x00000000;
	SaUint32T ntfTokenFlags = 0x00000000;
	SaUint32T sysTokenFlags = 0x00000000;
	SaUint16T fmtExpTokenOffset = 1;
	SaStringT fmtExpPtr = &formatExpression[0];
	SaStringT fmtExpPtrSnabel = &formatExpression[1];

	osafassert(formatExpression != NULL);

	/* Main checking loop */
	for (;;) {		/* Scan the Format Expression */
		if ((*fmtExpPtr == TOKEN_START_SYMBOL) && (*fmtExpPtrSnabel != STRING_END_CHARACTER)) {
			switch (*fmtExpPtrSnabel++) {	/* New token to be validated */
			case COMMON_LOG_RECORD_FIELD_TYPE:
				tokenOk = validateComToken(fmtExpPtrSnabel,
							   &fmtExpTokenOffset,
							   &comTokenFlags, twelveHourModeFlag, logStreamType);
				break;

			case NOTIFICATION_LOG_RECORD_FIELD_TYPE:
				tokenOk = validateNtfToken(fmtExpPtrSnabel,
							   &fmtExpTokenOffset, &ntfTokenFlags, twelveHourModeFlag);
				if (logStreamType > STREAM_TYPE_NOTIFICATION) {
					tokenOk = SA_FALSE;	/* These tokens valid for alarm and
								   ntf log streams only */
				}
				break;

			case SYSTEM_LOG_RECORD_FIELD_TYPE:
				tokenOk = validateSysToken(fmtExpPtrSnabel, &fmtExpTokenOffset, &sysTokenFlags);
				if (logStreamType < STREAM_TYPE_SYSTEM) {
					tokenOk = SA_FALSE;	/* These tokens valid for app and
								   sys log streams only */
				}
				break;

			default:	/* Non valid log record field type */
				tokenOk = SA_FALSE;
				break;
			}
		} else if (*fmtExpPtrSnabel != STRING_END_CHARACTER) {
			/* All chars between tokens */
			fmtExpTokenOffset = LITTERAL_CHAR_OFFSET;
			tokenOk = SA_TRUE;
		} else {	/* End of formatExpression */
			if (*fmtExpPtr == TOKEN_START_SYMBOL) {
				tokenOk = SA_FALSE;	/* Illegal litteral character at the end */
			}
			break;
		}

		/* Step forward according to token offset */
		fmtExpPtr += fmtExpTokenOffset;
		if ((tokenOk == SA_FALSE) || (*fmtExpPtr == STRING_END_CHARACTER)) {
			break;	/* Illegal token or end of formatExpression */
		}
		fmtExpPtrSnabel = (fmtExpPtr + 1);
	}
	formatExpressionOk = tokenOk;

	return formatExpressionOk;
}

/**
 * Format a log record
 * 
 * @param logRecord
 * @param formatExpression format string
 * @param fixedLogRecordSize if 0 do not pad
 * @param dest_size size of dest
 * @param dest write at most dest_size bytes to dest
 * @param logRecordIdCounter
 * 
 * @return int number of bytes written to dest
 */
int lgs_format_log_record(SaLogRecordT *logRecord, const SaStringT formatExpression,
	SaUint16T fixedLogRecordSize, size_t dest_size, char *dest, SaUint32T logRecordIdCounter)
{
	SaStringT fmtExpPtr = &formatExpression[0];
	SaStringT fmtExpPtrSnabel = &formatExpression[1];
	SaTimeT totalTime;
	SaInt8T truncationCharacter = (SaInt8T)COMPLETED_LOG_RECORD;
	SaUint16T fmtExpTokenOffset = 1;
	SaInt32T truncationLetterPos = -1;
	struct tm *timeStampData;
	SaBoolT _twelveHourModeFlag = SA_FALSE;
	const SaBoolT *twelveHourModeFlag = &_twelveHourModeFlag;
	int i = 0;
	SaUint16T rec_size = dest_size;

	if (formatExpression == NULL) {
		goto error_exit;
	}

	/* Init output vector with a '\0' */
	(void)strcpy(dest, "");

	totalTime = (logRecord->logTimeStamp / (SaTimeT)SA_TIME_ONE_SECOND);

	/* Split timestamp in timeStampData */
	struct tm tm_info;
	timeStampData = localtime_r((const time_t *)&totalTime, &tm_info);
	osafassert(timeStampData);

	/* Main formatting loop */
	for (;;) {
		/* Scan the Format Expression */
		if ((*fmtExpPtr == TOKEN_START_SYMBOL) && (*fmtExpPtrSnabel != STRING_END_CHARACTER)) {
			switch (*fmtExpPtrSnabel++) {
				/* Check log record field types if a new token is present */
			case COMMON_LOG_RECORD_FIELD_TYPE:
				i += extractCommonField(&dest[i],
							dest_size - i,
							fmtExpPtrSnabel,
							&fmtExpTokenOffset,
							&truncationLetterPos,
							(SaInt32T)strlen(dest),
							logRecordIdCounter,
							twelveHourModeFlag, timeStampData, logRecord,rec_size);
				break;

			case NOTIFICATION_LOG_RECORD_FIELD_TYPE:
				i += extractNotificationField(&dest[i],
							      dest_size - i,
							      fmtExpPtrSnabel,
							      &fmtExpTokenOffset, twelveHourModeFlag, logRecord);
				break;

			case SYSTEM_LOG_RECORD_FIELD_TYPE:
				i += extractSystemField(&dest[i],
							dest_size - i, fmtExpPtrSnabel, &fmtExpTokenOffset, logRecord);
				break;

			default:
				TRACE("Invalid token %u", *(fmtExpPtrSnabel - 1));
				i = 0;
				goto error_exit;

			}

		} else {	/* All chars between tokens */
			fmtExpTokenOffset = 1;
			/* Insert litteral chars i.e. [:, ,/ and "] */
			if (i < dest_size) {
				dest[i++] = *fmtExpPtr;
			}

			if (*fmtExpPtrSnabel == STRING_END_CHARACTER)
				break;	/* End of formatExpression */
		}

		if (i >= dest_size){
			/* Truncation exists */
			truncationCharacter = (SaInt8T)TRUNCATED_LOG_RECORD;
			break;
		}

		/* Step forward */
		fmtExpPtr += fmtExpTokenOffset;
		if (*fmtExpPtr == STRING_END_CHARACTER)
			break;	/* End of formatExpression */

		fmtExpPtrSnabel = (fmtExpPtr + 1);
	}			/* for ( ; ; ) */

	/* Pad log record to fixed log record fieldSize */
	if ((fixedLogRecordSize > 0) && (i < fixedLogRecordSize)) {
		memset(&dest[i], ' ', fixedLogRecordSize - i);
		dest[fixedLogRecordSize - 1] = '\n';
		i = fixedLogRecordSize;
	} else {
		dest[i - 1] = '\n';
	}

	if (truncationLetterPos != -1) {	/* Insert truncation info letter */
		dest[truncationLetterPos] = truncationCharacter;
	}

 error_exit:
	return i;
}
