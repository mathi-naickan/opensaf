/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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

#include "ntfimcn_notifier.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "ncsgl_defs.h"
#include "saNtf.h"
#include "saAmf.h"
#include "saImm.h"
#include "logtrace.h"
#include "saf_error.h"

#include "ntfimcn_main.h"

/* Release code, major version, minor version */
SaVersionT ntf_version = { 'A', 0x01, 0x01 };
const unsigned int sleep_delay_ms = 500;
const unsigned int max_waiting_time_ms = 7 * 1000;	/* 7 secs */
const unsigned int max_init_waiting_time_ms = 60 * 1000;	/* 60 secs */

/*
 * Global variables
 */
extern ntfimcn_cb_t ntfimcn_cb; /* See ntfimcn_main.c */


/**
 * Initialize notification handling
 *
 * @param *cb[in]
 *
 * @return (-1) if init fail
 */
int ntfimcn_ntf_init(ntfimcn_cb_t *cb)
{
	SaAisErrorT rc;
	int internal_rc = 0;
	
	TRACE_ENTER();


	if (cb->haState == SA_AMF_HA_ACTIVE) {
		int msecs_waited = 0;
		rc = saNtfInitialize(&cb->ntf_handle, NULL, &ntf_version);
		while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_init_waiting_time_ms)) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			rc = saNtfInitialize(&cb->ntf_handle, NULL, &ntf_version);
		}
		if (rc != SA_AIS_OK) {
			LOG_ER("%s NTF API could not be initialized: %s",__FUNCTION__,saf_error(rc));
			internal_rc = (-1);
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Send a previously allocated and filled in notification.
 * Allocated memory is freed
 *
 * @param notificationHandle[in]
 *
 * @return (-1) if error
 */
static int send_notification(SaNtfNotificationHandleT notificationHandle)
{
	SaAisErrorT rc = 0;
	int msecs_waited = 0;
	int internal_rc = 0;

	TRACE_ENTER();
	/*
	 * Send the notification
	 */
	msecs_waited = 0;
	rc = saNtfNotificationSend( notificationHandle);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saNtfNotificationSend( notificationHandle);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfNotificationSend failed %s",__FUNCTION__,
					saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/*
	 * Free the allocated notification
	 */
	msecs_waited = 0;
	rc = saNtfNotificationFree( notificationHandle);
	while ((rc == SA_AIS_ERR_TRY_AGAIN) && (msecs_waited < max_waiting_time_ms)) {
		usleep(sleep_delay_ms * 1000);
		msecs_waited += sleep_delay_ms;
		rc = saNtfNotificationFree( notificationHandle);
	}
	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfNotificationFree failed %s",__FUNCTION__,
					saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in Additional information.
 * infoId = ntf_index
 * infoType = SA_NTF_VALUE_STRING
 * infoValue = "Attribute name"
 *
 * @param notificationHandle[in]
 * @param *additionalInfo[out]
 * @param info_value[in]
 * @param ntf_index[in]
 *
 * @return (-1) on error
 */
 static int fill_additional_info(
		SaNtfNotificationHandleT notificationHandle,
		SaNtfAdditionalInfoT *additionalInfo,
		SaStringT info_value,
		SaUint64T add_index)
{
	int internal_rc = 0;
	SaAisErrorT rc = SA_AIS_OK;
	char *name_ptr = NULL;
	SaUint16T name_len;

	TRACE_ENTER();

	/* Allocate string for attribute name */
	name_len = strlen(info_value) + 1;
	if (name_len > SA_MAX_NAME_LENGTH) {
		name_len = SA_MAX_NAME_LENGTH;
	}
	rc = saNtfPtrValAllocate(
			notificationHandle,
			name_len,
			(void **)&name_ptr,
			&additionalInfo[add_index].infoValue);
	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfPtrValAllocate failed %s",__FUNCTION__,
				saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/* Fill in additional info of Index */
	additionalInfo[add_index].infoId = add_index;
	additionalInfo[add_index].infoType = SA_NTF_VALUE_STRING;
	strncpy((char *)name_ptr, info_value, name_len);
	if (name_len == SA_MAX_NAME_LENGTH) {
		name_ptr[SA_MAX_NAME_LENGTH] = '\0';
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in a value array in allocated notification.
 * Can be any value including a string.
 *
 * @param notificationHandle
 * @param value_in[in]
 * @param value_in_size[in]
 * @param *value_out[out]
 * 
 * @return (-1) on error
 */
static int fill_value_array( SaNtfNotificationHandleT notificationHandle,
		SaUint8T *value_in,
		SaSizeT value_in_size,
		SaNtfValueT *value_out)
{
	int internal_rc = 0;
	SaAisErrorT rc = SA_AIS_OK;
	SaUint8T *dest_ptr;

	TRACE_ENTER();

	rc = saNtfPtrValAllocate(
			notificationHandle,
			value_in_size,
			(void **)&dest_ptr,
			value_out);
	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfPtrValAllocate failed %s",__FUNCTION__,
				saf_error(rc));
		internal_rc = (-1);
		goto done;
	}
	memcpy(dest_ptr, value_in, value_in_size);

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in the attribute value and type. The value can be of
 * different types and may also be a multi-value which is an array of values
 * of the same type.
 *
 * Parameters:
 *
 * attributeValue_in
 *		A pointer to an IMM value or in case of a multi-value a pointer to an
 *		array of values.
 * attributeValue_out
 *		A pointer to the member in the allocted notification struct that shall
 *		be filled in with the value
 *
 * Multi-value:
 * Type SA_NTF_VALUE_ARRAY must be used. The parameter attributeSize must
 * be set to the size of memory used by each attribute. The numElements shall
 * be set to the number of values and must be value > 1.
 *
 * @param notificationHandle[in]
 * @param attrValueType_in[in]
 * @param *attrValues_in[in]
 * @param attrValues_index_in[in]
 * @param attr_id_in[in]
 * @param *attributeId_out[out]
 * @param *attributeType_out[out]
 * @param *value_out[out]
 *
 * @return (-1) on error
 */
static int fill_attribute_value(
		SaNtfNotificationHandleT notificationHandle,
		SaImmValueTypeT attrValueType_in,
		SaImmAttrValueT *attrValues_in,
		SaUint32T attrValues_index_in,
		SaNtfElementIdT attr_id_in,
		SaNtfElementIdT *attributeId_out,
		SaNtfValueTypeT *attributeType_out,
		SaNtfValueT *value_out)
{
	int internal_rc = 0;
	SaSizeT str_len = 0;
	SaAnyT any_value;
	SaNameT name_value;

	TRACE_ENTER();

	/* Insert value dependent on type.
	 * For possible types see SaImmValueTypeT */
	switch (attrValueType_in) {
	case SA_IMM_ATTR_SAINT32T:
		value_out->int32Val = *((SaInt32T *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_INT32;
		*attributeId_out = attr_id_in;
		break;
	case SA_IMM_ATTR_SAUINT32T:
		value_out->uint32Val = *((SaUint32T *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_UINT32;
		*attributeId_out = attr_id_in;
		break;
	case SA_IMM_ATTR_SATIMET:	/* SaTimeT (SaInt64T) */
	case SA_IMM_ATTR_SAINT64T:
		value_out->int64Val = *((SaInt64T *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_INT64;
		*attributeId_out = attr_id_in;
		break;
	case SA_IMM_ATTR_SAUINT64T:
		value_out->uint64Val = *((SaUint64T *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_UINT64;
		*attributeId_out = attr_id_in;
		break;
	case SA_IMM_ATTR_SAFLOATT:
		value_out->floatVal = *((SaFloatT *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_FLOAT;
		*attributeId_out = attr_id_in;
		break;
	case SA_IMM_ATTR_SADOUBLET:
		value_out->doubleVal = *((SaDoubleT *)attrValues_in[attrValues_index_in]);
		*attributeType_out = SA_NTF_VALUE_DOUBLE;
		*attributeId_out = attr_id_in;
		break;

	case SA_IMM_ATTR_SANAMET:	/* SaNameT */
		name_value = *(SaNameT *)attrValues_in[attrValues_index_in];
		internal_rc = fill_value_array(notificationHandle,
				name_value.value,
				name_value.length,
				value_out);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_value_array failed",__FUNCTION__);
			goto done;
		}
		*attributeType_out = SA_NTF_VALUE_LDAP_NAME;
		*attributeId_out = attr_id_in;
		break;

	case SA_IMM_ATTR_SASTRINGT:
		str_len = strlen(*(SaStringT *)attrValues_in[attrValues_index_in])+1;

		internal_rc = fill_value_array(notificationHandle,
				*(SaUint8T **)attrValues_in[attrValues_index_in],
				str_len,
				value_out);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_value_array failed",__FUNCTION__);
			goto done;
		}
		*attributeType_out = SA_NTF_VALUE_STRING;
		*attributeId_out = attr_id_in;
		break;

	case SA_IMM_ATTR_SAANYT:	/* SaAnyT, SA_NTF_VALUE_BINARY */
		any_value = *(SaAnyT *)attrValues_in[attrValues_index_in];
		internal_rc = fill_value_array(notificationHandle,
				(SaUint8T*)any_value.bufferAddr,
				any_value.bufferSize,
				value_out);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_value_array failed",__FUNCTION__);
			goto done;
		}
		*attributeType_out = SA_NTF_VALUE_BINARY;
		*attributeId_out = attr_id_in;
		break;

	default:
		LOG_ER("%s Invalid attributeType %d",__FUNCTION__,attrValueType_in);
		internal_rc = (-1);
		break;
	}

done:
	TRACE_LEAVE();
	return internal_rc;
} /* fill_attribute_value_2 */

/**
 * Fill in the SaNtfNotificationHeader part of a notification.
 * Shall be filled in for all notifications
 *
 * @param CcbId[in]
 * @param *notificationHeader[out]
 * @param NtfEventType[in]
 * @param numAdditionalInfo[in]
 * @param *dist_name[in]
 */
static void fill_notification_header_common_part(
		SaImmOiCcbIdT CcbId,
		SaNtfNotificationHeaderT *notificationHeader,
		SaNtfEventTypeT NtfEventType,
		SaUint16T numAdditionalInfo,
		SaNameT *dist_name)
{
	SaNameT *sa_name_ptr;

	TRACE_ENTER();
	
	/* Event type e.g. SA_NTF_OBJECT_CREATION */
	*notificationHeader->eventType = NtfEventType;

	/* Notification Object. DN of handled object */
	sa_name_ptr = notificationHeader->notificationObject;
	strncpy((char*)sa_name_ptr->value, (char *)dist_name->value,	SA_MAX_NAME_LENGTH);
	sa_name_ptr->value[SA_MAX_NAME_LENGTH-1] = 0;
	sa_name_ptr->length = strlen((char *)sa_name_ptr->value) + 1;

	/* Notifying Object. A constant string */
	sa_name_ptr = notificationHeader->notifyingObject;
	strncpy((char *)sa_name_ptr->value,NTFIMCN_NOTIFYING_OBJECT,SA_MAX_NAME_LENGTH);
	sa_name_ptr->length = sizeof(NTFIMCN_NOTIFYING_OBJECT);

	/* Notification Class Identifier. Constant identifier
	 * except for minor Id that's dependent on event type
	 */
	notificationHeader->notificationClassId->vendorId = SA_NTF_VENDOR_ID_OSAF;
	notificationHeader->notificationClassId->majorId = SA_SVC_IMMS;
	notificationHeader->notificationClassId->minorId = 0;

	/* Event time. Use default */
	*notificationHeader->eventTime = (SaTimeT)SA_TIME_UNKNOWN;

	/* Correlated Notifications. None */
	notificationHeader->numCorrelatedNotifications = 0;

	/* Number of additional info fields and . Dependent on number of attributes in
	 * object that has a value.
	 */
	notificationHeader->numAdditionalInfo = numAdditionalInfo;

	TRACE_LEAVE();
}

/**
 * Handle attribute information; Value and attribute name.
 * Attribute name goes to the Additinal information list and
 * Attribute value goes to the Attribute List.
 * The Attribute name and the attribute value shall have the same index
 * in respective list. The list shall start with (index 0,1 and 2) special
 * attributes:
 * Id
 *  0  if ccb id=0 "SaImmAttrImplementerName or
 *     if ccb id != 0 "SaImmAttrAdminOwner"
 *  1  "saImmAttrClassName"
 *  2  "saImmAttrCcbId"
 *
 * @param CcbId[in]
 * @param imm_attribute_values[in]
 * @param SaNtfObjectCreateNotification[out]
 * @param rdn_attr_name[in]
 * @param ccbLast[in]
 * 
 * @return (-1) on error
 */
static int fill_attribute_info_create(
			SaImmOiCcbIdT CcbId,
			const SaImmAttrValuesT_2 **imm_attribute_values,
			SaNtfObjectCreateDeleteNotificationT *SaNtfObjectCreateNotification,
			SaStringT rdn_attr_name,
			SaBoolT ccbLast)
{
	int internal_rc=0;
	SaUint64T imm_index = 0;
	SaUint64T add_index = 4;
	SaUint64T var_index = 4;

	const SaImmAttrValuesT_2 *imm_attribute;
	SaImmAttrValuesT_2 my_imm_attribute;
	SaNtfAttributeT *ntf_attributes;
	SaImmAttrValueT SaImmAttrValue[1];

	TRACE_ENTER();

	my_imm_attribute.attrValues = &SaImmAttrValue[0];
	imm_attribute = imm_attribute_values[imm_index++];

	if (CcbId == 0) {
		/* Do not include SaImmAttrCcbId and ccbLast */
		add_index = 2;
		var_index = 2;
	}
	
	while (imm_attribute != NULL) {

		if (strcmp(imm_attribute->attrName, rdn_attr_name) == 0) {
			/* The RDN attribute shall not be included in the notification */
		} else if ((strcmp(imm_attribute->attrName, NTFIMCN_ADMIN_OWNER_NAME) == 0) &&
			(CcbId != 0)) {
			/* Id 0: SaImmAttrAdminOwnerName
			 *       If CCB Id <> 0
			 */
			/* Fill in Id 0 */
			internal_rc = fill_additional_info(
					SaNtfObjectCreateNotification->notificationHandle,
					SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
					imm_attribute->attrName,
					0);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Value */
			ntf_attributes = &SaNtfObjectCreateNotification->objectAttributes[0];
			internal_rc = fill_attribute_value(
					SaNtfObjectCreateNotification->notificationHandle,
					imm_attribute->attrValueType,
					imm_attribute->attrValues,
					0, /* Index of attrValues in */
					0, /* Attribute Id */
					&ntf_attributes->attributeId,
					&ntf_attributes->attributeType,
					&ntf_attributes->attributeValue);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
				goto done;
			}
		} else if ((strcmp(imm_attribute->attrName, NTFIMCN_IMPLEMENTER_NAME) == 0) &&
				(CcbId == 0)) {
			/* Id 0: SaImmAttrImplementerName
			 *       if CCB Id = 0
			 */
			/* Fill in Id 0 */
			internal_rc = fill_additional_info(
					SaNtfObjectCreateNotification->notificationHandle,
					SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
					imm_attribute->attrName,
					0);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Value */
			ntf_attributes = &SaNtfObjectCreateNotification->objectAttributes[0];
			internal_rc = fill_attribute_value(
					SaNtfObjectCreateNotification->notificationHandle,
					imm_attribute->attrValueType,
					imm_attribute->attrValues,
					0, /* Index of attrValues in */
					0, /* Attribute Id */
					&ntf_attributes->attributeId,
					&ntf_attributes->attributeType,
					&ntf_attributes->attributeValue);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
				goto done;
			}
		} else if (strcmp(imm_attribute->attrName, NTFIMCN_CLASS_NAME) == 0) {
			/* Id 1: SaImmAttrClassName
			 */
			/* Fill in Id 1 */
			internal_rc = fill_additional_info(
					SaNtfObjectCreateNotification->notificationHandle,
					SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
					imm_attribute->attrName,
					1);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Value */
			ntf_attributes = &SaNtfObjectCreateNotification->objectAttributes[1];
			internal_rc = fill_attribute_value(
					SaNtfObjectCreateNotification->notificationHandle,
					imm_attribute->attrValueType,
					imm_attribute->attrValues,
					0, /* Index of attrValues in */
					1, /* Attribute Id */
					&ntf_attributes->attributeId,
					&ntf_attributes->attributeType,
					&ntf_attributes->attributeValue);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
				goto done;
			}
		} else {
			/* Other attributes */

			/* Fill Additional Info */
			internal_rc = fill_additional_info(
					SaNtfObjectCreateNotification->notificationHandle,
					SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
					imm_attribute->attrName,
					add_index);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Values
			 * Can be a multi value or no value.
			 */
			SaUint32T vindex;
			for (vindex=0; vindex<imm_attribute->attrValuesNumber; vindex++) {
				ntf_attributes =
					&SaNtfObjectCreateNotification->objectAttributes[var_index+vindex];
				internal_rc = fill_attribute_value(
						SaNtfObjectCreateNotification->notificationHandle,
						imm_attribute->attrValueType,
						imm_attribute->attrValues,
						vindex, /* Index of attrValues in */
						add_index, /* Attribute Id */
						&ntf_attributes->attributeId,
						&ntf_attributes->attributeType,
						&ntf_attributes->attributeValue);
				if (internal_rc != 0) {
					LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
					goto done;
				}
			}

			var_index += vindex;
			add_index++;
		}

		imm_attribute = imm_attribute_values[imm_index++];
	}

	/* Id 2: SaImmAttrCcbId
	 *       There is no such attribute so it has to be constructed
	 *		 Fill in only if CcbId is not 0
	 */
	if (CcbId > 0) {
		my_imm_attribute.attrValueType = SA_IMM_ATTR_SAUINT64T;
		my_imm_attribute.attrValuesNumber = 1;
		my_imm_attribute.attrValues[0] = (void *)&CcbId;

		internal_rc = fill_additional_info(
				SaNtfObjectCreateNotification->notificationHandle,
				SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
				NTFIMCN_CCB_ID,
				2);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
				ntf_attributes = &SaNtfObjectCreateNotification->objectAttributes[2];
				internal_rc = fill_attribute_value(
						SaNtfObjectCreateNotification->notificationHandle,
						my_imm_attribute.attrValueType,
						my_imm_attribute.attrValues,
						0, /* Index of attrValues in */
						2, /* Attribute Id */
						&ntf_attributes->attributeId,
						&ntf_attributes->attributeType,
						&ntf_attributes->attributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

	/* Id 3: ccbLast
	 *       There is no such attribute so it has to be constructed
	 *		 Fill in only if CcbId is not 0
	 */
	if (CcbId > 0) {
		my_imm_attribute.attrValueType = SA_IMM_ATTR_SAUINT32T;
		my_imm_attribute.attrValuesNumber = 1;
		my_imm_attribute.attrValues[0] = (void *)&ccbLast;

		internal_rc = fill_additional_info(
				SaNtfObjectCreateNotification->notificationHandle,
				SaNtfObjectCreateNotification->notificationHeader.additionalInfo,
				NTFIMCN_CCB_LAST,
				3);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
				ntf_attributes = &SaNtfObjectCreateNotification->objectAttributes[3];
				internal_rc = fill_attribute_value(
						SaNtfObjectCreateNotification->notificationHandle,
						my_imm_attribute.attrValueType,
						my_imm_attribute.attrValues,
						0, /* Index of attrValues in */
						3, /* Attribute Id */
						&ntf_attributes->attributeId,
						&ntf_attributes->attributeType,
						&ntf_attributes->attributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Handle attribute information; Value and attribute name.
 * Attribute name goes to the Additinal information list and
 * Attribute value goes to the Attribute List.
 * The Attribute name and the attribute value shall have the same index
 * in respective list. The list shall start with (index 0,1 and 2) special
 * attributes:
 * Id
 *  0  if ccb id=0 "SaImmAttrImplementerName or
 *     if ccb id != 0 "SaImmAttrAdminOwner"
 *  1  "saImmAttrClassName"
 *  2  "saImmAttrCcbId"
 *
 * @param CcbId[in]
 * @param invoke_name[in]
 * @param imm_attr_mods_in[in]
 * @param SaNtfAttributeChangeNotification[out]
 * @param ccbLast[in]
 *
 * @return (-1) on error
 */
static int fill_attribute_info_modify(
			SaImmOiCcbIdT CcbId,
			SaStringT invoke_name,
			const SaImmAttrModificationT_2 **imm_attr_mods_in,
			SaNtfAttributeChangeNotificationT *SaNtfAttributeChangeNotification,
			SaBoolT ccbLast)
{
	int internal_rc = 0;
	SaUint64T imm_index = 0;
	SaUint64T add_index = 4;
	SaUint64T var_index = 4;
	const SaImmAttrModificationT_2 *imm_attr_mods;
	SaImmAttrModificationT_2 my_imm_attr_mod;
	SaNtfAttributeChangeT *changedAttributes=NULL;
	SaImmAttrValueT SaImmAttrValue=NULL;
	char *string_v[1];

	TRACE_ENTER();

	my_imm_attr_mod.modAttr.attrValues = &SaImmAttrValue;
	imm_attr_mods = imm_attr_mods_in[imm_index++];

	if (CcbId == 0) {
		/* Do not include SaImmAttrCcbId and ccbLast */
		add_index = 2;
		var_index = 2;
	}

	while (imm_attr_mods != NULL) {

		if (strcmp(imm_attr_mods->modAttr.attrName, NTFIMCN_CLASS_NAME) == 0) {
			/* Id 1: SaImmAttrClassName
			 */
			/* Fill in Id 1 */
			internal_rc = fill_additional_info(
					SaNtfAttributeChangeNotification->notificationHandle,
					SaNtfAttributeChangeNotification->notificationHeader.additionalInfo,
					imm_attr_mods->modAttr.attrName,
					1);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Value */
			changedAttributes = &SaNtfAttributeChangeNotification->changedAttributes[1];
			changedAttributes->oldAttributePresent = SA_FALSE;
			internal_rc = fill_attribute_value(
					SaNtfAttributeChangeNotification->notificationHandle,
					imm_attr_mods->modAttr.attrValueType,
					imm_attr_mods->modAttr.attrValues,
					0, /* Index of attrValues in */
					1, /* Attribute Id */
					&changedAttributes->attributeId,
					&changedAttributes->attributeType,
					&changedAttributes->newAttributeValue);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
				goto done;
			}

		} else if (strcmp(imm_attr_mods->modAttr.attrName, NTFIMCN_IMPLEMENTER_NAME) == 0) {
			/* Do not handle here since it is handled separately. See below*/
		} else if (strcmp(imm_attr_mods->modAttr.attrName, NTFIMCN_ADMIN_OWNER_NAME) == 0) {
			/* Do not handle here since it is handled separately. See below*/
		} else {
			/* Other attributes */
			/* Fill Additional Info */
			internal_rc = fill_additional_info(
					SaNtfAttributeChangeNotification->notificationHandle,
					SaNtfAttributeChangeNotification->notificationHeader.additionalInfo,
					imm_attr_mods->modAttr.attrName,
					add_index);
			if (internal_rc != 0) {
				LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
				goto done;
			}

			/* Fill Corresponding Attribute Value
			 * Can be a multi value or no value.
			 */
			SaUint32T vindex = 0;
			for (vindex=0; vindex<imm_attr_mods->modAttr.attrValuesNumber; vindex++) {
				changedAttributes =
					&SaNtfAttributeChangeNotification->changedAttributes[var_index+vindex];
				changedAttributes->oldAttributePresent = SA_FALSE;
				internal_rc = fill_attribute_value(
						SaNtfAttributeChangeNotification->notificationHandle,
						imm_attr_mods->modAttr.attrValueType,
						imm_attr_mods->modAttr.attrValues,
						vindex, /* Index of attrValues in */
						add_index, /* Attribute Id */
						&changedAttributes->attributeId,
						&changedAttributes->attributeType,
						&changedAttributes->newAttributeValue);
				if (internal_rc != 0) {
					LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
					goto done;
				}
			}

			var_index += vindex;
			add_index++;
		}

		imm_attr_mods = imm_attr_mods_in[imm_index++];
	}

	/* Id 0: AdminOwnerName or ImplementerName
	 *       Must be filled in for all notifications but does not exist as an
	 *       attribute if more than one modify in the same ccb. Therefore the
	 *       name is saved using the userData pointer in the ccb header.
	 */

	string_v[0] = invoke_name;
	my_imm_attr_mod.modAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	my_imm_attr_mod.modAttr.attrValuesNumber = 1;
	my_imm_attr_mod.modAttr.attrValues[0] = &string_v[0];

	char *tmp_str;
	if (CcbId == 0) {
		tmp_str = NTFIMCN_IMPLEMENTER_NAME;
	} else {
		tmp_str = NTFIMCN_ADMIN_OWNER_NAME;
	}

	internal_rc = fill_additional_info(
			SaNtfAttributeChangeNotification->notificationHandle,
			SaNtfAttributeChangeNotification->notificationHeader.additionalInfo,
			tmp_str,
			0);
	if (internal_rc != 0) {
		LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
		goto done;
	}

	/* Fill Corresponding Attribute Value */
	changedAttributes = &SaNtfAttributeChangeNotification->changedAttributes[0];
	changedAttributes->oldAttributePresent = SA_FALSE;
	internal_rc = fill_attribute_value(
			SaNtfAttributeChangeNotification->notificationHandle,
			my_imm_attr_mod.modAttr.attrValueType,
			my_imm_attr_mod.modAttr.attrValues,
			0, /* Index of attrValues in */
			0, /* Attribute Id */
			&changedAttributes->attributeId,
			&changedAttributes->attributeType,
			&changedAttributes->newAttributeValue);
	if (internal_rc != 0) {
		LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
		goto done;
	}

	/* Id 2: SaImmAttrCcbId
	 *       There is no such attribute so it has to be constructed
	 *		 Fill in only if CcbId is not 0
	 */
	if (CcbId > 0) {
		my_imm_attr_mod.modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
		my_imm_attr_mod.modAttr.attrValuesNumber = 1;
		my_imm_attr_mod.modAttr.attrValues[0] = (void *)&CcbId;

		internal_rc = fill_additional_info(
				SaNtfAttributeChangeNotification->notificationHandle,
				SaNtfAttributeChangeNotification->notificationHeader.additionalInfo,
				NTFIMCN_CCB_ID,
				2);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
		changedAttributes = &SaNtfAttributeChangeNotification->changedAttributes[2];
		changedAttributes->oldAttributePresent = SA_FALSE;
		internal_rc = fill_attribute_value(
				SaNtfAttributeChangeNotification->notificationHandle,
				my_imm_attr_mod.modAttr.attrValueType,
				my_imm_attr_mod.modAttr.attrValues,
				0, /* Index of attrValues in */
				2, /* Attribute Id */
				&changedAttributes->attributeId,
				&changedAttributes->attributeType,
				&changedAttributes->newAttributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

	/* Id 3: ccbLast
	 *       There is no such attribute so it has to be constructed
	 *		 Fill in only if CcbId is not 0
	 */
	if (CcbId > 0) {
		my_imm_attr_mod.modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
		my_imm_attr_mod.modAttr.attrValuesNumber = 1;
		my_imm_attr_mod.modAttr.attrValues[0] = (void *)&ccbLast;

		internal_rc = fill_additional_info(
				SaNtfAttributeChangeNotification->notificationHandle,
				SaNtfAttributeChangeNotification->notificationHeader.additionalInfo,
				NTFIMCN_CCB_LAST,
				3);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
				changedAttributes = &SaNtfAttributeChangeNotification->changedAttributes[3];
				changedAttributes->oldAttributePresent = SA_FALSE;
				internal_rc = fill_attribute_value(
						SaNtfAttributeChangeNotification->notificationHandle,
						my_imm_attr_mod.modAttr.attrValueType,
						my_imm_attr_mod.modAttr.attrValues,
						0, /* Index of attrValues in */
						3, /* Attribute Id */
						&changedAttributes->attributeId,
						&changedAttributes->attributeType,
						&changedAttributes->newAttributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * See ntfimcn_fill_attribute_info_create
 * 
 * @param CcbId[in]
 * @param invoke_name[in]
 * @param SaNtfObjectNotification[out]
 * @param ccbLast[in]
 * @return (-1) on error
 */
static int fill_attribute_info_delete(SaImmOiCcbIdT CcbId,
			SaStringT invoke_name,
			SaNtfObjectCreateDeleteNotificationT *SaNtfObjectNotification,
			SaBoolT ccbLast)
{
	int internal_rc=0;
	SaImmAttrValuesT_2 my_imm_attribute;
	SaNtfAttributeT *ntf_attributes;
	/*SaImmAttrValueT my_imm_attr_value;*/
	SaImmAttrValueT my_imm_attr_value[1]; /* COV fix */
	char *string_v[1];

	TRACE_ENTER();

	/*my_imm_attribute.attrValues = &my_imm_attr_value;*/
	my_imm_attribute.attrValues = &my_imm_attr_value[0];
	ntf_attributes = SaNtfObjectNotification->objectAttributes;

	string_v[0] = invoke_name;
	my_imm_attribute.attrValueType = SA_IMM_ATTR_SASTRINGT;
	my_imm_attribute.attrValuesNumber = 1;
	my_imm_attribute.attrValues[0] = &string_v[0];

	if (CcbId != 0) {
		/* Id 0: SaImmAttrAdminOwnerName
		 */
		/* Fill in Id 0 */
		internal_rc = fill_additional_info(
				SaNtfObjectNotification->notificationHandle,
				SaNtfObjectNotification->notificationHeader.additionalInfo,
				NTFIMCN_ADMIN_OWNER_NAME,
				0);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
		ntf_attributes = &SaNtfObjectNotification->objectAttributes[0];
		internal_rc = fill_attribute_value(
				SaNtfObjectNotification->notificationHandle,
				my_imm_attribute.attrValueType,
				my_imm_attribute.attrValues,
				0, /* Index of attrValues in */
				0, /* Attribute Id */
				&ntf_attributes->attributeId,
				&ntf_attributes->attributeType,
				&ntf_attributes->attributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
		
	} else {
		/* Id 0: SaImmAttrImplementerName
		 *       if CCB Id = 0
		 */
		/* Fill in Id 0 */
		internal_rc = fill_additional_info(
				SaNtfObjectNotification->notificationHandle,
				SaNtfObjectNotification->notificationHeader.additionalInfo,
				NTFIMCN_IMPLEMENTER_NAME,
				0);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
		ntf_attributes = &SaNtfObjectNotification->objectAttributes[0];
		internal_rc = fill_attribute_value(
				SaNtfObjectNotification->notificationHandle,
				my_imm_attribute.attrValueType,
				my_imm_attribute.attrValues,
				0, /* Index of attrValues in */
				0, /* Attribute Id */
				&ntf_attributes->attributeId,
				&ntf_attributes->attributeType,
				&ntf_attributes->attributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

	/* Id 1: SaImmAttrCcbId
	 *       There is no such attribute so it has to be constructed
	 */
	my_imm_attribute.attrValueType = SA_IMM_ATTR_SAUINT64T;
	my_imm_attribute.attrValuesNumber = 1;
	my_imm_attribute.attrValues[0] = (void *)&CcbId;

	internal_rc = fill_additional_info(
			SaNtfObjectNotification->notificationHandle,
			SaNtfObjectNotification->notificationHeader.additionalInfo,
			NTFIMCN_CCB_ID,
			1);
	if (internal_rc != 0) {
		LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
		goto done;
	}

	/* Fill Corresponding Attribute Value */
	ntf_attributes = &SaNtfObjectNotification->objectAttributes[1];
	internal_rc = fill_attribute_value(
			SaNtfObjectNotification->notificationHandle,
			my_imm_attribute.attrValueType,
			my_imm_attribute.attrValues,
			0, /* Index of attrValues in */
			1, /* Attribute Id */
			&ntf_attributes->attributeId,
			&ntf_attributes->attributeType,
			&ntf_attributes->attributeValue);
	if (internal_rc != 0) {
		LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
		goto done;
	}


	/* Id 2: ccbLast
	 *       There is no such attribute so it has to be constructed
	 *		 Fill in only if CcbId is not 0
	 */
	if (CcbId > 0) {
		my_imm_attribute.attrValueType = SA_IMM_ATTR_SAUINT32T;
		my_imm_attribute.attrValuesNumber = 1;
		my_imm_attribute.attrValues[0] = (void *)&ccbLast;

		internal_rc = fill_additional_info(
				SaNtfObjectNotification->notificationHandle,
				SaNtfObjectNotification->notificationHeader.additionalInfo,
				NTFIMCN_CCB_LAST,
				2);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_additional_info failed",__FUNCTION__);
			goto done;
		}

		/* Fill Corresponding Attribute Value */
				ntf_attributes = &SaNtfObjectNotification->objectAttributes[2];
				internal_rc = fill_attribute_value(
						SaNtfObjectNotification->notificationHandle,
						my_imm_attribute.attrValueType,
						my_imm_attribute.attrValues,
						0, /* Index of attrValues in */
						2, /* Attribute Id */
						&ntf_attributes->attributeId,
						&ntf_attributes->attributeType,
						&ntf_attributes->attributeValue);
		if (internal_rc != 0) {
			LOG_ER("%s: fill_attribute_value failed",__FUNCTION__);
			goto done;
		}
	}

done:
	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in notification parameters  (SaNtfObjectCreateDeleteNotificationT) and
 * send an object create notification
 *
 * @param CcbUtilOperationData[in]
 * @param rdn_attr_name[in]
 * @param ccbLast[in]
 *
 * @return (-1) if error
 */
int ntfimcn_send_object_create_notification(
		CcbUtilOperationData_t *CcbUtilOperationData, SaStringT rdn_attr_name,
		SaBoolT ccbLast)
{
	SaAisErrorT rc = SA_AIS_OK;
	int internal_rc = 0;
	SaImmOiCcbIdT CcbId;

	SaNtfObjectCreateDeleteNotificationT SaNtfObjectCreateNotification;
	SaUint64T num_attributes = 4;
	SaUint64T num_additional_info = 4;

	TRACE_ENTER();

	if (ntfimcn_cb.haState != SA_AMF_HA_ACTIVE) {
		/* We are standby. Do not send the notification */
		goto done;
	}

	CcbId = CcbUtilOperationData->ccbId;


	if (CcbId == 0) {
		/* Ccb id  0 means that there is no ccb. saImmAttrCcbId shall be
		 * omitted, ccbLast shall be omitted
		 */
		num_attributes = 2;
		num_additional_info = 2;
	}
	/* Find out how many attributes we have to handle. Note that multivalues
	 * are handled as a separate attribute for each value
	 */
	SaUint64T i=0;
	const SaImmAttrValuesT_2 ** values;
	values = CcbUtilOperationData->param.create.attrValues;
	for (i=0; values[i] != NULL; i++) {
		/* Count "class" attributes with a value only
		 * Do not count the "rdn" attribute
		 */
		if (strcmp(values[i]->attrName,rdn_attr_name) == 0) {
			/* Do not count the "rdn" attribute */
			continue;
		}

		if (strncmp(values[i]->attrName,NTFIMCN_IMM_ATTR,
						sizeof(NTFIMCN_IMM_ATTR)-1) != 0) {
			num_attributes += values[i]->attrValuesNumber;
			num_additional_info++;
		}
	}

	/* Allocate memory for the notification structure.
	 * NOTE:	The first two attributes and additional values are not part of
	 *			the IMM attribute list but has to be allocated
	 * NOTE:	SA_AIS_ERR_TRY_AGAIN is not tested for. Is never returned.
	 */
	rc = saNtfObjectCreateDeleteNotificationAllocate(
			ntfimcn_cb.ntf_handle,
			&SaNtfObjectCreateNotification,
			0, /* Number of correlated notifications */
			0, /* Length additional text */
			num_additional_info,	/* Number of additional info */
			num_attributes,	/* Number of attributes */
			SA_NTF_ALLOC_SYSTEM_LIMIT /* variableDataSize */);

	if (rc != SA_AIS_OK) {
		LOG_ER("%s saNtfObjectCreateDeleteNotificationAllocate fail: %s",
				__FUNCTION__,saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/* Fill in Additional info and corresponding Attribute list
	 */
	internal_rc = fill_attribute_info_create(CcbId,
			CcbUtilOperationData->param.create.attrValues,
			&SaNtfObjectCreateNotification,
			rdn_attr_name,
			ccbLast);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_fill_attribute_info failed",__FUNCTION__);
		goto error;
	}

	/* Fill in information in header. Note that information about number of
	 * attributes must be calculated first.
	 */
	fill_notification_header_common_part(/*CcbUtilOperationData,*/ CcbId,
		&SaNtfObjectCreateNotification.notificationHeader,
		SA_NTF_OBJECT_CREATION,
		num_additional_info,
		&CcbUtilOperationData->objectName);

	/* Fill in source indicator
	 */
	if (CcbId == 0) {
		*SaNtfObjectCreateNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;
	} else {
		*SaNtfObjectCreateNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	}

	/* Send the notification */
	internal_rc = send_notification(
		SaNtfObjectCreateNotification.notificationHandle);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_send_notification failed",__FUNCTION__);
		goto error;
	}

done:
	TRACE_LEAVE();
	return internal_rc;

error:
	rc = saNtfNotificationFree(SaNtfObjectCreateNotification.notificationHandle);
	if (rc != SA_AIS_OK) {
		TRACE("%s saNtfNotificationFree fail: %s",__FUNCTION__,saf_error(rc));
	}

	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in notification parameters  (SaNtfAttributeChangeNotificationT) and
 * send an object create notification
 *
 * @param CcbUtilOperationData[in]
 * @param invoke_name[in]
 * @param ccbLast[in]
 *
 * @return (-1) if error
 */
int ntfimcn_send_object_modify_notification(
		CcbUtilOperationData_t *CcbUtilOperationData,
		SaNameT *invoke_name,
		SaBoolT ccbLast)
{
	SaAisErrorT rc = SA_AIS_OK;
	int internal_rc = 0;
	SaImmOiCcbIdT CcbId;
	char invoke_name_str[SA_MAX_NAME_LENGTH+1];

	SaNtfAttributeChangeNotificationT SaNtfAttributeChangeNotification;
	SaUint64T num_attributes = 4;
	SaUint64T num_additional_info = 4;
	/* A dummy modification is sent before a delete modification in order to
	 * get the admin owner / implementer name.
	 * Do not send a notification for this dummy.
	 * The class name attribute is missing in a dummy.
	 */
	bool dummy_modify=true;


	TRACE_ENTER();

	if (ntfimcn_cb.haState != SA_AMF_HA_ACTIVE) {
		/* We are standby. Do not send notification */
		goto done;
	}

	CcbId = CcbUtilOperationData->ccbId;

	if (CcbId == 0) {
		/* Ccb id  0 means that there is no ccb. saImmAttrCcbId shall be
		 * omitted, ccbLast shall be omitted
		 */
		num_attributes = 2;
		num_additional_info = 2;
	}
#if 1
	/* To FIX: The invoker name is saved in the ccb header userData as a
	 * pointer to a SaNameT. This should be changed to a SaStringT.
	 * Until then a conversion is needed.
	 */
	snprintf(invoke_name_str,(invoke_name->length+1),"%s",invoke_name->value);
#endif

	/* Find out how many attributes we have to handle */
	SaUint64T i=0;
	const SaImmAttrModificationT_2 ** attrMods;
	attrMods = CcbUtilOperationData->param.modify.attrMods;
	for (i=0; attrMods[i] != NULL; i++) {
		/* Count "class" attributes with a value only
		 */
		if ((strncmp(attrMods[i]->modAttr.attrName,NTFIMCN_IMM_ATTR,
						sizeof(NTFIMCN_IMM_ATTR)-1) != 0)) {
			num_attributes += attrMods[i]->modAttr.attrValuesNumber;
			num_additional_info++;
		}
		
		if (strncmp(attrMods[i]->modAttr.attrName,NTFIMCN_CLASS_NAME,
					sizeof(NTFIMCN_CLASS_NAME)-1) == 0) {
			dummy_modify = false;
		}
	}

	if (dummy_modify == true) {
		/* A "dummy" modification detected. Do not send notification */
		goto done;
	}
	
	/* Allocate memory for the notification structure.
	 * NOTE:	The first two attributes and additional values are not part of
	 *			the IMM attribute list but has to be allocated
	 * NOTE:	SA_AIS_ERR_TRY_AGAIN is not tested for. Is never returned.
	 */
	rc = saNtfAttributeChangeNotificationAllocate(
			ntfimcn_cb.ntf_handle,
			&SaNtfAttributeChangeNotification,
			0, /* Number of correlated notifications */
			0, /* Length additional text */
			num_additional_info,	/* Number of additional info */
			num_attributes,		/* Number of attributes */
			SA_NTF_ALLOC_SYSTEM_LIMIT /* variableDataSize */);

	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfObjectCreateDeleteNotificationAllocate fail: %s",
				__FUNCTION__,saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/* Fill in Additional info and corresponding Attribute list
	 */
	internal_rc = fill_attribute_info_modify(
			CcbId,
			invoke_name_str,
			CcbUtilOperationData->param.modify.attrMods,
			&SaNtfAttributeChangeNotification,
			ccbLast);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_fill_attribute_info failed",__FUNCTION__);
		goto error;
	}

	/* Fill in information in header. Note that information about number of
	 * attributes must be calculated first.
	 */
	fill_notification_header_common_part(CcbId,
		&SaNtfAttributeChangeNotification.notificationHeader,
		SA_NTF_ATTRIBUTE_CHANGED,
		num_additional_info,
		&CcbUtilOperationData->objectName);

	/* Fill in source indicator
	 */
	if (CcbId == 0) {
		*SaNtfAttributeChangeNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;
	} else {
		*SaNtfAttributeChangeNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	}

	/* Send the notification */
	internal_rc = send_notification(
		SaNtfAttributeChangeNotification.notificationHandle);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_send_notification failed",__FUNCTION__);
		goto done;
	}

done:
	TRACE_LEAVE();
	return internal_rc;

error:
	rc = saNtfNotificationFree(SaNtfAttributeChangeNotification.notificationHandle);
	if (rc != SA_AIS_OK) {
		TRACE("saNtfNotificationFree fail: %s",
				saf_error(rc));
	}

	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Fill in notification parameters  (SaNtfObjectCreateDeleteNotificationT) and
 * send an object delete notification
 *
 * @param CcbUtilOperationData[in]
 * @param invoke_name[in]
 * @param ccbLast[in]
 *
 * @return (-1) if error
 */
int ntfimcn_send_object_delete_notification(CcbUtilOperationData_t *CcbUtilOperationData,
		const SaNameT *invoke_name,
		SaBoolT ccbLast)
{
	SaAisErrorT rc = SA_AIS_OK;
	int internal_rc = 0;
	SaImmOiCcbIdT CcbId;

	SaNtfObjectCreateDeleteNotificationT SaNtfObjectDeleteNotification;
	SaUint64T num_attributes = 3;
	char invoke_name_str[SA_MAX_NAME_LENGTH+1];

	TRACE_ENTER();
	CcbId = CcbUtilOperationData->ccbId;

	if (CcbId == 0) {
		/* There will be no implementer name and ccb id 0 means no ccb.
		 * Therefore implementer nama and ccb id shall not be in the
		 * notification, ccbLast shall be omitted.
		 */
		num_attributes = 0;
	}

	if (ntfimcn_cb.haState != SA_AMF_HA_ACTIVE) {
		/* We are standby. Do not send notification */
		goto done;
	}

	/* Allocate memory for the notification structure.
	 * NOTE:	The first two attributes and additional values are not part of
	 *			the IMM attribute list but has to be allocated
	 * NOTE:	SA_AIS_ERR_TRY_AGAIN is not tested for. Is never returned.
	 */
	rc = saNtfObjectCreateDeleteNotificationAllocate(
			ntfimcn_cb.ntf_handle,
			&SaNtfObjectDeleteNotification,
			0, /* Number of correlated notifications */
			0, /* Length additional text */
			num_attributes,	/* Number of additional info */
			num_attributes,	/* Number of attributes */
			SA_NTF_ALLOC_SYSTEM_LIMIT /* variableDataSize */);

	if (rc != SA_AIS_OK) {
		LOG_ER("%s: saNtfObjectCreateDeleteNotificationAllocate fail: %s",
				__FUNCTION__,saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/* Fill in Additional info and corresponding Attribute list
	 */
	if (num_attributes > 0) {
		memcpy(invoke_name_str,invoke_name->value,
				invoke_name->length);
		invoke_name_str[invoke_name->length] = '\0';
		internal_rc = fill_attribute_info_delete(CcbId,
				invoke_name_str,
				&SaNtfObjectDeleteNotification,
				ccbLast);
		if (internal_rc != 0) {
			LOG_ER("%s: ntfimcn_fill_attribute_info failed",__FUNCTION__);
			goto error;
		}
	}

	/* Fill in information in header.
	 */
	fill_notification_header_common_part(/*CcbUtilOperationData,*/ CcbId,
		&SaNtfObjectDeleteNotification.notificationHeader,
		SA_NTF_OBJECT_DELETION,
		num_attributes,
		&CcbUtilOperationData->objectName);

	/* Fill in source indicator
	 */
	if (CcbId == 0) {
		*SaNtfObjectDeleteNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;
	} else {
		*SaNtfObjectDeleteNotification.sourceIndicator = SA_NTF_MANAGEMENT_OPERATION;
	}

	/* Send the notification */
	internal_rc = send_notification(
		SaNtfObjectDeleteNotification.notificationHandle);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_send_notification failed",__FUNCTION__);
		goto error;
	}

done:
	TRACE_LEAVE();
	return internal_rc;
	
error:
	rc = saNtfNotificationFree(SaNtfObjectDeleteNotification.notificationHandle);
	if (rc != SA_AIS_OK) {
		TRACE("%s saNtfNotificationFree fail: %s",__FUNCTION__,saf_error(rc));
	}

	TRACE_LEAVE();
	return internal_rc;
}

/**
 * Used to report that CM updates from IMM may have been lost.
 * For this purpose a 'dummy' state change notification is used.
 *
 * @return (-1) if error
 */
int ntfimcn_send_lost_cm_notification(void)
{
	SaAisErrorT rc = SA_AIS_OK;
	int internal_rc = 0;
	SaNameT object_name = {12,"osafntfimcnd"};

	SaNtfStateChangeNotificationT SaNtfStateChangeNotification;
	const SaUint64T num_statechanges=0;


	TRACE_ENTER();

	if (ntfimcn_cb.haState != SA_AMF_HA_ACTIVE) {
		/* We are standby. Do not send notification */
		goto done;
	}

	/* Allocate memory for the notification structure.
	 * NOTE:	The first two attributes and additional values are not part of
	 *			the IMM attribute list but has to be allocated
	 * NOTE:	SA_AIS_ERR_TRY_AGAIN is not tested for. Is never returned.
	 */
	rc = saNtfStateChangeNotificationAllocate(
			ntfimcn_cb.ntf_handle,
			&SaNtfStateChangeNotification,
			0, /* Number of correlated notifications */
			0, /* Length additional text */
			0, /* Number of additional info */
			num_statechanges,	/* Number of state changes */
			SA_NTF_ALLOC_SYSTEM_LIMIT /* variableDataSize */);

	if (rc != SA_AIS_OK) {
		LOG_ER("%s saNtfStateChangeNotificationAllocate fail: %s",
				__FUNCTION__,saf_error(rc));
		internal_rc = (-1);
		goto done;
	}

	/* Fill in information in header.
	 */
	fill_notification_header_common_part(
		0, /* CcbId */
		&SaNtfStateChangeNotification.notificationHeader,
		SA_NTF_OBJECT_STATE_CHANGE, /* NtfEventType */
		0, /* numAdditionalInfo */
		&object_name);

	/* Fill in source indicator
	 */
	*SaNtfStateChangeNotification.sourceIndicator = SA_NTF_OBJECT_OPERATION;

	/* Send the notification */
	internal_rc = send_notification(
		SaNtfStateChangeNotification.notificationHandle);
	if (internal_rc != 0) {
		LOG_ER("%s: ntfimcn_send_notification failed",__FUNCTION__);
		goto error;
	}

done:
	TRACE_LEAVE();
	return internal_rc;

error:
	rc = saNtfNotificationFree(SaNtfStateChangeNotification.notificationHandle);
	if (rc != SA_AIS_OK) {
		TRACE("%s saNtfNotificationFree fail: %s",__FUNCTION__,saf_error(rc));
	}

	TRACE_LEAVE();
	return internal_rc;
}
