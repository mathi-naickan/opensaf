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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <saAis.h>
#include <saImmOi.h>
#include <saImmOm.h>
#include <immutil.h>
#include <util.h>
#include <utest.h>
#include "saNtf.h"
#include "test_ntf_imcn.h"

/*
 * Global variables.
 */
static SaNtfHandleT ntfHandle;
static NotifData rec_notif_data;
static SaImmOiHandleT immOiHnd = 0;

/**
 * Callback routine, called when subscribed notification arrives.
 */
void saNtfNotificationCallback(SaNtfSubscriptionIdT subscriptionId,
		const SaNtfNotificationsT * notification)
{
	rec_notif_data.c_d_notif_ptr = NULL;
	rec_notif_data.a_c_notif_ptr = NULL;
	rec_notif_data.o_s_c_notif_ptr = NULL;

	/*
	 * notification is either of type CreateDelete or AttributeChange.
	 */
	if (notification->notificationType == SA_NTF_TYPE_OBJECT_CREATE_DELETE) {
		rec_notif_data.c_d_notif_ptr = (SaNtfObjectCreateDeleteNotificationT*)
				&(notification->notification.objectCreateDeleteNotification);
		rec_notif_data.nHandle =
				rec_notif_data.c_d_notif_ptr->notificationHandle;
		rec_notif_data.evType = *(rec_notif_data.c_d_notif_ptr->notificationHeader.eventType);
	} else if (notification->notificationType == SA_NTF_TYPE_ATTRIBUTE_CHANGE) {
		rec_notif_data.a_c_notif_ptr = (SaNtfAttributeChangeNotificationT*)
				& notification->notification.attributeChangeNotification;
		rec_notif_data.nHandle =
				rec_notif_data.a_c_notif_ptr->notificationHandle;
		rec_notif_data.evType = *(rec_notif_data.a_c_notif_ptr->notificationHeader.eventType);
	} else if (notification->notificationType == SA_NTF_TYPE_STATE_CHANGE) {
		rec_notif_data.o_s_c_notif_ptr = (SaNtfStateChangeNotificationT*)
				& notification->notification.stateChangeNotification;
		rec_notif_data.nHandle =
				rec_notif_data.o_s_c_notif_ptr->notificationHandle;
		rec_notif_data.evType = *(rec_notif_data.o_s_c_notif_ptr->notificationHeader.eventType);
	}
	rec_notif_data.populated = SA_TRUE;
}

/**
 * Callback routine, called when subscribed notification was discarded.
 */
void saDiscardedCallback(SaNtfSubscriptionIdT subscriptionId,
		SaNtfNotificationTypeT notifType,
		SaUint32T numDiscarded,
		const SaNtfIdentifierT * discardedNotifIds)
{
	printf("number of discarded notifications: %d", numDiscarded);
}

static SaNtfCallbacksT ntfCallbacks = {
	saNtfNotificationCallback,
	saDiscardedCallback
};

/**
 * Initialize the NTF API.
 */
static inline SaAisErrorT init_ntf()
{
	SaVersionT version = {'A', 0x01, 0x01};
	return saNtfInitialize(&ntfHandle, &ntfCallbacks, &version);
}

/**
 * Display buffer content.
 */
static void print_buf(const SaUint8T* ptr, SaUint16T numElem)
{
	int i = 0;
	for (i = 0; i < numElem; i++) {
		printf("%x-", ptr[i]);
		if (i > 0 && i % 15 == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

/**
 * Check equality between two equally long buffers.
 */
static SaBoolT bufs_equal(const SaUint8T* b1, const SaUint8T* b2, SaUint16T ln)
{
	SaBoolT rc = SA_TRUE;
	int i = 0;
	for (i = 0; i < ln; i++) {
		if (b1[i] != b2[i]) {
			rc = SA_FALSE;
			break;
		}
	}
	return rc;
}

/**
 * Special case comparison (used when inputting buffer data with immcfg command).
 */
static SaBoolT bufs_equal1(const SaUint8T* recbuf, const SaUint8T* expbuf, SaUint16T rec_ln)
{
	SaBoolT rc = SA_TRUE;
	int i = 0;
	for (i = 0; i < rec_ln; i++) {
		if (recbuf[i] != (expbuf[i * 2 + 1] - '0')) {
			rc = SA_FALSE;
			break;
		}
	}
	return rc;
}

/**
 * Display the content of a notification.
 */
static void print_notif(NotifData* n)
{
	char tmp[256];
	SaUint16T numElem = 0;
	char* str = NULL;
	SaNtfNotificationHeaderT* nHeader = NULL;
	SaUint16T tmpi = 0;

	if (n->c_d_notif_ptr != NULL) {
		nHeader = &n->c_d_notif_ptr->notificationHeader;
		tmpi = *n->c_d_notif_ptr->sourceIndicator;
	} else if (n->a_c_notif_ptr != NULL) {
		nHeader = &n->a_c_notif_ptr->notificationHeader;
		tmpi = *n->a_c_notif_ptr->sourceIndicator;
	} else if (n->o_s_c_notif_ptr != NULL) {
		nHeader = &n->o_s_c_notif_ptr->notificationHeader;
		tmpi = *n->o_s_c_notif_ptr->sourceIndicator;
	} else return;

	printf("\n-------------------------------------------------------------\n");
	printf("                 %s\n", (n == &rec_notif_data) ?
			"RECEIVED notif" : "EXPECTED notif");
	printf("-------------------------------------------------------------\n");

	if (n->evType == SA_NTF_OBJECT_CREATION) strcpy(tmp, "SA_NTF_OBJECT_CREATION");
	else if (n->evType == SA_NTF_OBJECT_DELETION) strcpy(tmp, "SA_NTF_OBJECT_DELETION");
	else if (n->evType == SA_NTF_ATTRIBUTE_CHANGED) strcpy(tmp, "SA_NTF_ATTRIBUTE_CHANGED");
	else if (n->evType == SA_NTF_OBJECT_STATE_CHANGE) strcpy(tmp, "SA_NTF_OBJECT_STATE_CHANGE");

	printf("Event type               : %s\n", tmp);
	printf("Notification object      : %d, %s\n", nHeader->notificationObject->length,
			nHeader->notificationObject->value);
	printf("Notifying object         : %d, %s\n", nHeader->notifyingObject->length,
			nHeader->notifyingObject->value);
	printf("Vendor Id                : %d\n", nHeader->notificationClassId->vendorId);
	printf("Major Id                 : %d\n", nHeader->notificationClassId->majorId);
	printf("Minor Id                 : %d\n", nHeader->notificationClassId->minorId);

	if (tmpi == SA_NTF_OBJECT_OPERATION) strcpy(tmp, "SA_NTF_OBJECT_OPERATION");
	else if (tmpi == SA_NTF_MANAGEMENT_OPERATION) strcpy(tmp, "SA_NTF_MANAGEMENT_OPERATION");
	else strcpy(tmp, "SA_NTF_UNKNOWN_OPERATION");
	printf("Source Indicator         : %s\n", tmp);

	/* return here for object state change notifications */
	if (n->o_s_c_notif_ptr != NULL) {
		return;
	}

	printf("Num additional info      : %d\n", nHeader->numAdditionalInfo);
	int i = 0;
	for (i = 0; i < nHeader->numAdditionalInfo; i++) {
		safassert(saNtfPtrValGet(n->nHandle,
				&nHeader->additionalInfo[i].infoValue,
				(void**) &str,
				&numElem), SA_AIS_OK);
		printf("[%d]: infoId:%d infoType:%d infoValue:(%d)%s\n", i,
				nHeader->additionalInfo[i].infoId,
				nHeader->additionalInfo[i].infoType,
				numElem,
				str);
	}
	if (n->evType == SA_NTF_ATTRIBUTE_CHANGED) {
		printf("Num changed attributes   : %d\n", n->a_c_notif_ptr->numAttributes);
		for (i = 0; i < n->a_c_notif_ptr->numAttributes; i++) {
			if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_STRING) {
				safassert(saNtfPtrValGet(n->nHandle,
						&n->a_c_notif_ptr->changedAttributes[i].newAttributeValue,
						(void**) &str,
						&numElem), SA_AIS_OK);
				printf("[%d]: Id:%d Type:%d Value:(%d)%s\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						numElem,
						str);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_UINT32) {
				printf("[%d]: Id:%d Type:%d Value:%u\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.uint32Val);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_INT32) {
				printf("[%d]: Id:%d Type:%d Value:%d\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.int32Val);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_UINT64) {
				printf("[%d]: Id:%d Type:%d Value:%llu\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.uint64Val);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_INT64) {
				printf("[%d]: Id:%d Type:%d Value:%lld\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.int64Val);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_FLOAT) {
				printf("[%d]: Id:%d Type:%d Value:%f\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.floatVal);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_DOUBLE) {
				printf("[%d]: Id:%d Type:%d Value:%f\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						n->a_c_notif_ptr->changedAttributes[i].newAttributeValue.doubleVal);
			} else if (n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_BINARY ||
					n->a_c_notif_ptr->changedAttributes[i].attributeType == SA_NTF_VALUE_LDAP_NAME) {
				SaUint8T* binptr = NULL;
				safassert(saNtfPtrValGet(n->nHandle,
						&n->a_c_notif_ptr->changedAttributes[i].newAttributeValue,
						(void**) &binptr,
						&numElem), SA_AIS_OK);
				printf("[%d]: Id:%d Type:%d NumBytes:%u\n", i,
						n->a_c_notif_ptr->changedAttributes[i].attributeId,
						n->a_c_notif_ptr->changedAttributes[i].attributeType,
						numElem);
				print_buf(binptr, numElem);
			} else {
				printf("Unsupported attribute type: %d\n", n->a_c_notif_ptr->changedAttributes[i].attributeType);
			}
		}
	} else if (n->evType == SA_NTF_OBJECT_CREATION || n->evType == SA_NTF_OBJECT_DELETION) {
		printf("Num attributes           : %d\n", n->c_d_notif_ptr->numAttributes);
		for (i = 0; i < n->c_d_notif_ptr->numAttributes; i++) {
			if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_STRING) {
				safassert(saNtfPtrValGet(n->nHandle,
						&n->c_d_notif_ptr->objectAttributes[i].attributeValue,
						(void**) &str,
						&numElem), SA_AIS_OK);
				printf("[%d]: Id:%d Type:%d Value:(%d)%s\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						numElem,
						str);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_UINT32) {
				printf("[%d]: Id:%d Type:%d Value:%u\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.uint32Val);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_INT32) {
				printf("[%d]: Id:%d Type:%d Value:%d\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.int32Val);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_UINT64) {
				printf("[%d]: Id:%d Type:%d Value:%llu\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.uint64Val);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_INT64) {
				printf("[%d]: Id:%d Type:%d Value:%lld\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.int64Val);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_FLOAT) {
				printf("[%d]: Id:%d Type:%d Value:%f\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.floatVal);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_DOUBLE) {
				printf("[%d]: Id:%d Type:%d Value:%f\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						n->c_d_notif_ptr->objectAttributes[i].attributeValue.doubleVal);
			} else if (n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_BINARY ||
					n->c_d_notif_ptr->objectAttributes[i].attributeType == SA_NTF_VALUE_LDAP_NAME) {
				SaUint8T* binptr = NULL;
				safassert(saNtfPtrValGet(n->nHandle,
						&n->c_d_notif_ptr->objectAttributes[i].attributeValue,
						(void**) &binptr,
						&numElem), SA_AIS_OK);
				printf("[%d]: Id:%d Type:%d NumBytes:(%d)\n", i,
						n->c_d_notif_ptr->objectAttributes[i].attributeId,
						n->c_d_notif_ptr->objectAttributes[i].attributeType,
						numElem);
				print_buf(binptr, numElem);
			} else {
				printf("Unsupported attribute type: %d\n", n->c_d_notif_ptr->objectAttributes[i].attributeType);
			}
		}
	}
}

/**
 * Check equality between the additional info sections of two (expected and
 * received) notifications.
 */
static SaBoolT is_equal_add_info(NotifData* n_exp, NotifData* n_rec)
{
	SaBoolT rc = SA_TRUE;
	int i = 0;
	int j = 0;
	char* exp_str = NULL;
	char* rec_str = NULL;
	SaUint16T exp_ln = 0;
	SaUint16T rec_ln = 0;
	SaNtfNotificationHeaderT* nHeader_exp = (n_exp->c_d_notif_ptr != NULL) ?
			&n_exp->c_d_notif_ptr->notificationHeader :
			&n_exp->a_c_notif_ptr->notificationHeader;

	SaNtfNotificationHeaderT* nHeader_rec = (n_rec->c_d_notif_ptr != NULL) ?
			&n_rec->c_d_notif_ptr->notificationHeader :
			&n_rec->a_c_notif_ptr->notificationHeader;

	char infoIdVec[2][256];
	memset(infoIdVec[0], 0, nHeader_rec->numAdditionalInfo);
	memset(infoIdVec[1], 0, nHeader_rec->numAdditionalInfo);
	for (i = 0; i < nHeader_rec->numAdditionalInfo; i++) {
		rc = SA_FALSE;
		for (j = 0; j < nHeader_exp->numAdditionalInfo; j++) {
			if (nHeader_exp->additionalInfo[j].infoType ==
					nHeader_rec->additionalInfo[i].infoType &&
					nHeader_rec->additionalInfo[i].infoType == SA_NTF_VALUE_STRING) {
				safassert(saNtfPtrValGet(n_exp->nHandle,
						&nHeader_exp->additionalInfo[j].infoValue,
						(void**) &exp_str,
						&exp_ln), SA_AIS_OK);
				safassert(saNtfPtrValGet(n_rec->nHandle,
						&nHeader_rec->additionalInfo[i].infoValue,
						(void**) &rec_str,
						&rec_ln), SA_AIS_OK);
				if (exp_ln == rec_ln && !strcmp(exp_str, rec_str)) {
					/*
					 * CCBId varies between different test runs so it must
					 * be handled as a special case so that a mismatch is not
					 * reported for this reason.
					 */
					if (!strcmp(exp_str, "SaImmOiCcbIdT")) {
						n_exp->ccbInfoId = nHeader_exp->additionalInfo[j].infoId;
						n_rec->ccbInfoId = nHeader_rec->additionalInfo[i].infoId;
					}
					infoIdVec[0][nHeader_exp->additionalInfo[j].infoId] = 1;
					infoIdVec[1][nHeader_rec->additionalInfo[i].infoId] = 1;
					rc = SA_TRUE;
					break;
				}
			}
		}
		if (rc == SA_FALSE) {
			break;
		}
	}
	if (rc == SA_TRUE) {
		for (i = 0; i < nHeader_rec->numAdditionalInfo; i++) {
			if (infoIdVec[0][i] == 0 || infoIdVec[1][i] == 0) {
				rc = SA_FALSE;
				break;
			}
		}
	}
	return rc;
}

/**
 * Check whether the attributes list data is equal between received and expected
 * attribute change notification.
 */
static SaBoolT is_equal_ch_attr(const NotifData* n_exp, const NotifData* n_rec)
{
	SaBoolT rc = SA_TRUE;
	int i = 0;
	int j = 0;
	char* exp_str = NULL;
	char* rec_str = NULL;
	SaUint16T exp_ln = 0;
	SaUint16T rec_ln = 0;

	char elemVerified[256];
	memset(elemVerified, 0, n_rec->a_c_notif_ptr->numAttributes);
	for (i = 0; i < n_rec->a_c_notif_ptr->numAttributes; i++) {
		rc = SA_FALSE;
		for (j = 0; j < n_exp->a_c_notif_ptr->numAttributes; j++) {
			if ((n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
					n_rec->a_c_notif_ptr->changedAttributes[i].attributeType) &&
					(n_exp->a_c_notif_ptr->changedAttributes[j].oldAttributePresent ==
					n_rec->a_c_notif_ptr->changedAttributes[i].oldAttributePresent) &&
					elemVerified[j] == 0) {

				if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_STRING) {
					safassert(saNtfPtrValGet(n_exp->nHandle,
							&n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue,
							(void**) &exp_str,
							&exp_ln), SA_AIS_OK);
					safassert(saNtfPtrValGet(n_rec->nHandle,
							&n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue,
							(void**) &rec_str,
							&rec_ln), SA_AIS_OK);
					if (!strncmp(rec_str, "immcfg", 6)) {
						exp_ln = 6;
						rec_ln = 6;
					}
					if (exp_ln == rec_ln && !strncmp(exp_str, rec_str, exp_ln)) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_UINT32) {
					if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.uint32Val ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.uint32Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_INT32) {
					if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.int32Val ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.int32Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_UINT64) {
					/*
					 * CCB ID is of this type. Check whether it is the ccb id attribute and
					 * that a varying ccb id value doesn't cause a mismatch.
					 */
					if (n_exp->ccbInfoId == n_exp->a_c_notif_ptr->changedAttributes[j].attributeId &&
							n_rec->ccbInfoId == n_rec->a_c_notif_ptr->changedAttributes[i].attributeId) {
						rc = SA_TRUE;
						break;
					} else if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.uint64Val ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.uint64Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_INT64) {
					if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.int64Val ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.int64Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_FLOAT) {
					if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.floatVal ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.floatVal) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType ==
						SA_NTF_VALUE_DOUBLE) {
					if (n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue.doubleVal ==
							n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue.doubleVal) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_exp->a_c_notif_ptr->changedAttributes[j].attributeType == SA_NTF_VALUE_BINARY ||
						n_exp->a_c_notif_ptr->changedAttributes[j].attributeType == SA_NTF_VALUE_LDAP_NAME) {
					SaUint8T* recptr = NULL;
					SaUint8T* expptr = NULL;
					safassert(saNtfPtrValGet(n_rec->nHandle,
							&n_rec->a_c_notif_ptr->changedAttributes[i].newAttributeValue,
							(void**) &recptr,
							&rec_ln), SA_AIS_OK);
					safassert(saNtfPtrValGet(n_exp->nHandle,
							&n_exp->a_c_notif_ptr->changedAttributes[j].newAttributeValue,
							(void**) &expptr,
							&exp_ln), SA_AIS_OK);
					if (rec_ln == exp_ln && bufs_equal(recptr, expptr, rec_ln)) {
						rc = SA_TRUE;
						break;
					} else if ((rec_ln * 2) == exp_ln && bufs_equal1(recptr, expptr, rec_ln)) {
						rc = SA_TRUE;
						break;
					}
				} else {
					break;
				}
			}
		}
		if (rc == SA_FALSE) {
			break;
		} else {
			elemVerified[j] = 1;
		}
	}
	return rc;
}

/**
 * Check whether the attributes list data is equal between received and expected
 * create/delete notification.
 */
static SaBoolT is_equal_obj_attr(const NotifData* n_exp, const NotifData* n_rec)
{
	SaBoolT rc = SA_TRUE;
	int i = 0;
	int j = 0;
	SaUint16T exp_ln = 0;
	SaUint16T rec_ln = 0;

	char elemVerified[256];
	memset(elemVerified, 0, n_rec->c_d_notif_ptr->numAttributes);
	for (i = 0; i < n_rec->c_d_notif_ptr->numAttributes; i++) {
		rc = SA_FALSE;
		for (j = 0; j < n_exp->c_d_notif_ptr->numAttributes; j++) {
			if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeType ==
					n_rec->c_d_notif_ptr->objectAttributes[i].attributeType &&
					elemVerified[j] == 0) {
				if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_STRING) {
					char* exp_str = NULL;
					char* rec_str = NULL;
					safassert(saNtfPtrValGet(n_exp->nHandle,
							&n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue,
							(void**) &exp_str,
							&exp_ln), SA_AIS_OK);
					safassert(saNtfPtrValGet(n_rec->nHandle,
							&n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue,
							(void**) &rec_str,
							&rec_ln), SA_AIS_OK);
					if (!strncmp(rec_str, "immcfg", 6)) {
						exp_ln = 6;
						rec_ln = 6;
					}
					if (exp_ln == rec_ln && !strncmp(exp_str, rec_str, exp_ln)) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_UINT64) {
					/*
					 * CCB ID is of this type. Check whether it is the ccb id attribute and
					 * that a varying ccb id value doesn't cause a mismatch.
					 */
					if (n_exp->ccbInfoId == n_exp->c_d_notif_ptr->objectAttributes[j].attributeId &&
							n_rec->ccbInfoId == n_rec->c_d_notif_ptr->objectAttributes[i].attributeId) {
						rc = SA_TRUE;
						break;
					} else if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.uint64Val ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.uint64Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_INT64) {
					if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.int64Val ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.int64Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_UINT32) {
					if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.uint32Val ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.uint32Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_INT32) {
					if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.int32Val ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.int32Val) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_FLOAT) {
					if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.floatVal ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.floatVal) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_DOUBLE) {
					if (n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue.doubleVal ==
							n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue.doubleVal) {
						rc = SA_TRUE;
						break;
					}
				} else if (n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_BINARY ||
						n_rec->c_d_notif_ptr->objectAttributes[i].attributeType ==
						SA_NTF_VALUE_LDAP_NAME) {
					SaUint8T* recptr = NULL;
					SaUint8T* expptr = NULL;
					safassert(saNtfPtrValGet(n_rec->nHandle,
							&n_rec->c_d_notif_ptr->objectAttributes[i].attributeValue,
							(void**) &recptr,
							&rec_ln), SA_AIS_OK);
					safassert(saNtfPtrValGet(n_exp->nHandle,
							&n_exp->c_d_notif_ptr->objectAttributes[j].attributeValue,
							(void**) &expptr,
							&exp_ln), SA_AIS_OK);
					if (rec_ln == exp_ln && bufs_equal(recptr, expptr, rec_ln)) {
						rc = SA_TRUE;
						break;
					} else if ((rec_ln * 2) == exp_ln && bufs_equal1(recptr, expptr, rec_ln)) {
						rc = SA_TRUE;
						break;
					}
				} else {
					break;
				}
			}
		}
		if (rc == SA_FALSE) {
			break;
		} else {
			elemVerified[j] = 1;
		}
	}
	return rc;
}

/**
 * Check equality between two (expected and received) notifications.
 */
static SaBoolT compare_notifs(NotifData* n_exp, NotifData* n_rec)
{
	SaBoolT rc = SA_FALSE;
	SaNtfNotificationHeaderT* nHeader_exp = (n_exp->c_d_notif_ptr != NULL) ?
			&n_exp->c_d_notif_ptr->notificationHeader :
			&n_exp->a_c_notif_ptr->notificationHeader;

	SaNtfNotificationHeaderT* nHeader_rec = (n_rec->c_d_notif_ptr != NULL) ?
			&n_rec->c_d_notif_ptr->notificationHeader :
			&n_rec->a_c_notif_ptr->notificationHeader;

	if (n_exp->evType != rec_notif_data.evType);
	else if (nHeader_exp->notificationObject->length != nHeader_rec->notificationObject->length);
	else if (strcmp((char*) nHeader_exp->notificationObject->value, (char*) nHeader_rec->notificationObject->value));
	else if (nHeader_exp->notifyingObject->length != nHeader_rec->notifyingObject->length);
	else if (strcmp((char*) nHeader_exp->notifyingObject->value, (char*) nHeader_rec->notifyingObject->value));
	else if (nHeader_exp->notificationClassId->vendorId != nHeader_rec->notificationClassId->vendorId);
	else if (nHeader_exp->notificationClassId->majorId != nHeader_rec->notificationClassId->majorId);
	else if (nHeader_exp->notificationClassId->minorId != nHeader_rec->notificationClassId->minorId);
	else if (((n_exp->c_d_notif_ptr != NULL) ?
			*n_exp->c_d_notif_ptr->sourceIndicator :
			*n_exp->a_c_notif_ptr->sourceIndicator) !=
			((n_rec->c_d_notif_ptr != NULL) ?
			*n_rec->c_d_notif_ptr->sourceIndicator :
			*n_rec->a_c_notif_ptr->sourceIndicator));
	else if (nHeader_exp->numAdditionalInfo != nHeader_rec->numAdditionalInfo);
	else if (!is_equal_add_info(n_exp, n_rec));
	else if (((n_exp->c_d_notif_ptr != NULL) ?
			n_exp->c_d_notif_ptr->numAttributes :
			n_exp->a_c_notif_ptr->numAttributes) !=
			((n_rec->c_d_notif_ptr != NULL) ?
			n_rec->c_d_notif_ptr->numAttributes :
			n_rec->a_c_notif_ptr->numAttributes));
	else {
		if (n_exp->evType == SA_NTF_ATTRIBUTE_CHANGED) {
			if (is_equal_ch_attr(n_exp, n_rec)) {
				rc = SA_TRUE;
			}
		} else {
			if (is_equal_obj_attr(n_exp, n_rec)) {
				rc = SA_TRUE;
			}
		}
	}
        (n_exp->c_d_notif_ptr != NULL) ? free(n_exp->c_d_notif_ptr) : free(n_exp->a_c_notif_ptr);
	return rc;
}

/**
 * Subscribe to object create/delete/modify notifications.
 */
static SaAisErrorT subscribe_notifications()
{
	SaAisErrorT errorCode = SA_AIS_OK;
	SaNtfAttributeChangeNotificationFilterT attChFilter;
	SaNtfObjectCreateDeleteNotificationFilterT objCrDelFilter;
	SaNtfStateChangeNotificationFilterT objChangeFilter;
	SaNtfNotificationTypeFilterHandlesT notifFilterHandles;
	memset(&notifFilterHandles, 0, sizeof (notifFilterHandles));

	errorCode = saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
			&attChFilter,
			1,
			0,
			0,
			1,
			0);
	if (errorCode == SA_AIS_OK) {
		notifFilterHandles.attributeChangeFilterHandle =
				attChFilter.notificationFilterHandle;
		attChFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_ATTRIBUTE_CHANGED;
		attChFilter.notificationFilterHeader.notificationClassIds->vendorId = 32993;
		attChFilter.notificationFilterHeader.notificationClassIds->majorId = SA_SVC_IMMS;
		attChFilter.notificationFilterHeader.notificationClassIds->minorId = 0;

		errorCode = saNtfObjectCreateDeleteNotificationFilterAllocate(
				ntfHandle,
				&objCrDelFilter,
				2,
				0,
				0,
				1,
				0);
		if (errorCode == SA_AIS_OK) {
			notifFilterHandles.objectCreateDeleteFilterHandle =
					objCrDelFilter.notificationFilterHandle;
			objCrDelFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_CREATION;
			objCrDelFilter.notificationFilterHeader.eventTypes[1] = SA_NTF_OBJECT_DELETION;
			objCrDelFilter.notificationFilterHeader.notificationClassIds->vendorId = 32993;
			objCrDelFilter.notificationFilterHeader.notificationClassIds->majorId = SA_SVC_IMMS;
			objCrDelFilter.notificationFilterHeader.notificationClassIds->minorId = 0;

			errorCode = saNtfStateChangeNotificationFilterAllocate(
					ntfHandle,
					&objChangeFilter,
					1,
					0,
					0,
					1,
					0,
					0);
			if (errorCode == SA_AIS_OK) {
				notifFilterHandles.stateChangeFilterHandle =
						objChangeFilter.notificationFilterHandle;
				objChangeFilter.notificationFilterHeader.eventTypes[0] = SA_NTF_OBJECT_STATE_CHANGE;
				objChangeFilter.notificationFilterHeader.notificationClassIds->vendorId = 32993;
				objChangeFilter.notificationFilterHeader.notificationClassIds->majorId = SA_SVC_IMMS;
				objChangeFilter.notificationFilterHeader.notificationClassIds->minorId = 0;

				/* make the subscription */
				errorCode = saNtfNotificationSubscribe(&notifFilterHandles, 1);
                                saNtfNotificationFilterFree(notifFilterHandles.stateChangeFilterHandle);
                                saNtfNotificationFilterFree(notifFilterHandles.objectCreateDeleteFilterHandle);
                                saNtfNotificationFilterFree(notifFilterHandles.attributeChangeFilterHandle);
			} else {
                            saNtfNotificationFilterFree(notifFilterHandles.objectCreateDeleteFilterHandle);
                            saNtfNotificationFilterFree(notifFilterHandles.attributeChangeFilterHandle);
                        }
		} else {
                    saNtfNotificationFilterFree(notifFilterHandles.attributeChangeFilterHandle);
                }
	}

	return errorCode;
}

/**
 * Unsubscribe to object create/delete/modify notifications.
 */
static SaAisErrorT unsub_notifications()
{
	SaAisErrorT errorCode = SA_AIS_OK;
	int tryCnt = 0;
	while ((errorCode = saNtfNotificationUnsubscribe(1)) == SA_AIS_ERR_TRY_AGAIN
			&& tryCnt++ < 5) {
		usleep(1000000);
	}
	return errorCode;
}

/**
 * Allocate notification. It is allocated as a storage place for expected
 * notification data, i.e. this notification will be compared with the
 * received notification.
 */
static SaAisErrorT set_ntf(NotifData* n_exp, SaNtfEventTypeT ntfEventType,
		const char* dn,
		SaUint16T numAddi, SaUint16T numAttr)
{
	SaAisErrorT error = SA_AIS_ERR_FAILED_OPERATION;
	SaBoolT configObj = !strncmp(dn, "stringRdnCfg", 12);

	n_exp->evType = ntfEventType;
        n_exp->ccbInfoId = 0;
	SaNtfNotificationHeaderT* nHeader = NULL;
	n_exp->o_s_c_notif_ptr = NULL;

	if (n_exp->evType == SA_NTF_OBJECT_CREATION || n_exp->evType == SA_NTF_OBJECT_DELETION) {
		n_exp->a_c_notif_ptr = NULL;
		n_exp->c_d_notif_ptr = malloc(sizeof (SaNtfObjectCreateDeleteNotificationT));

		if ((error = saNtfObjectCreateDeleteNotificationAllocate(ntfHandle,
				n_exp->c_d_notif_ptr,
				0,
				0,
				numAddi,
				numAttr,
				SA_NTF_ALLOC_SYSTEM_LIMIT)) == SA_AIS_OK) {
			nHeader = &n_exp->c_d_notif_ptr->notificationHeader;
			n_exp->nHandle = n_exp->c_d_notif_ptr->notificationHandle;
			*n_exp->c_d_notif_ptr->sourceIndicator = (configObj)
					? SA_NTF_MANAGEMENT_OPERATION
					: SA_NTF_OBJECT_OPERATION;
		}
	} else if (n_exp->evType == SA_NTF_ATTRIBUTE_CHANGED) {
		n_exp->c_d_notif_ptr = NULL;
		n_exp->a_c_notif_ptr = malloc(sizeof (SaNtfAttributeChangeNotificationT));

		if ((error = saNtfAttributeChangeNotificationAllocate(ntfHandle,
				n_exp->a_c_notif_ptr,
				0,
				0,
				numAddi,
				numAttr,
				SA_NTF_ALLOC_SYSTEM_LIMIT)) == SA_AIS_OK) {
			nHeader = &n_exp->a_c_notif_ptr->notificationHeader;
			n_exp->nHandle = n_exp->a_c_notif_ptr->notificationHandle;
			*n_exp->a_c_notif_ptr->sourceIndicator = (configObj)
					? SA_NTF_MANAGEMENT_OPERATION
					: SA_NTF_OBJECT_OPERATION;
		}
	}
	if (nHeader != NULL) {
		if (configObj) {
			strcpy((char*) nHeader->notificationObject->value, dn);
		} else {
			strcpy((char*) nHeader->notificationObject->value, strchr(dn, '=') + 1);
		}
		nHeader->notificationObject->length = strlen((char*) nHeader->notificationObject->value) + 1;
		strcpy((char*) nHeader->notifyingObject->value, "safApp=OpenSaf");
		nHeader->notifyingObject->length = strlen((char*) nHeader->notifyingObject->value) + 1;
		nHeader->notificationClassId->vendorId = 32993;
		nHeader->notificationClassId->majorId = SA_SVC_IMMS;
		nHeader->notificationClassId->minorId = 0;
	}

	return error;
}

/**
 * Set an expected additional value, type is always string.
 */
static SaAisErrorT set_add_info(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT infoId, const char* infoValue)
{
	SaAisErrorT error = SA_AIS_OK;
	SaUint8T* temp = NULL;

	SaNtfNotificationHeaderT* nHeader = (n_exp->c_d_notif_ptr != NULL) ?
			&n_exp->c_d_notif_ptr->notificationHeader :
			&n_exp->a_c_notif_ptr->notificationHeader;

	error = saNtfPtrValAllocate(n_exp->nHandle,
			strlen(infoValue) + 1,
			(void**) &temp,
			&nHeader->additionalInfo[idx].infoValue);
	if (error == SA_AIS_OK) {
		strncpy((char*) temp, infoValue, strlen(infoValue) + 1);
		nHeader->additionalInfo[idx].infoId = infoId;
		nHeader->additionalInfo[idx].infoType = SA_NTF_VALUE_STRING;
	}

	return error;
}

/**
 * Set an expected attribute value, type is string.
 */
static SaAisErrorT set_attr_str(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, const char* attrValue)
{
	SaAisErrorT error = SA_AIS_OK;
	SaUint8T* temp = NULL;

	if (n_exp->c_d_notif_ptr != NULL) {
		error = saNtfPtrValAllocate(n_exp->nHandle,
				strlen(attrValue) + 1,
				(void**) &temp,
				&n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue);
		if (error == SA_AIS_OK) {
			strncpy((char*) temp, attrValue, strlen(attrValue) + 1);
			n_exp->c_d_notif_ptr->objectAttributes[idx].attributeId = attrId;
			n_exp->c_d_notif_ptr->objectAttributes[idx].attributeType = SA_NTF_VALUE_STRING;
		}
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}

	return error;
}

/**
 * Set expected attribute value, type is UINT32/INT32/UINT64/INT64/FLOAT/DOUBLE.
 */
static SaAisErrorT set_attr_scalar(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, SaNtfValueTypeT valType, void* attrValue)
{
	SaAisErrorT error = SA_AIS_OK;

	n_exp->c_d_notif_ptr->objectAttributes[idx].attributeId = attrId;
	n_exp->c_d_notif_ptr->objectAttributes[idx].attributeType = valType;
	if (valType == SA_NTF_VALUE_UINT32) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.uint32Val = *(SaUint32T*) attrValue;
	} else if (valType == SA_NTF_VALUE_INT32) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.int32Val = *(SaInt32T*) attrValue;
	} else if (valType == SA_NTF_VALUE_UINT64) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.uint64Val = *(SaUint64T*) attrValue;
	} else if (valType == SA_NTF_VALUE_INT64) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.int64Val = *(SaInt64T*) attrValue;
	} else if (valType == SA_NTF_VALUE_FLOAT) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.floatVal = *(SaFloatT*) attrValue;
	} else if (valType == SA_NTF_VALUE_DOUBLE) {
		n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue.doubleVal = *(SaDoubleT*) attrValue;
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	return error;
}

/**
 * Set expected attribute value, type is BINARY/NAME.
 */
static SaAisErrorT set_attr_buf(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, SaNtfValueTypeT valType, void* attrValue)
{
	SaAisErrorT error = SA_AIS_OK;
	SaUint16T numAlloc = (valType == SA_NTF_VALUE_BINARY)
			? ((SaAnyT*) attrValue)->bufferSize
			: ((SaNameT*) attrValue)->length;
	SaUint8T* temp = NULL;
	SaUint8T* srcPtr = (valType == SA_NTF_VALUE_BINARY)
			? ((SaAnyT*) attrValue)->bufferAddr
			: ((SaNameT*) attrValue)->value;

	if (n_exp->c_d_notif_ptr != NULL) {
		error = saNtfPtrValAllocate(n_exp->nHandle,
				numAlloc,
				(void**) &temp,
				&n_exp->c_d_notif_ptr->objectAttributes[idx].attributeValue);
		if (error == SA_AIS_OK) {
			memcpy(temp, srcPtr, numAlloc);
			n_exp->c_d_notif_ptr->objectAttributes[idx].attributeId = attrId;
			n_exp->c_d_notif_ptr->objectAttributes[idx].attributeType = valType;
		}
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	return error;
}

/**
 * Set an expected attribute value, type is string.
 */
static SaAisErrorT set_attr_change_str(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, const char* newValue)
{
	SaAisErrorT error = SA_AIS_OK;
	SaUint8T* temp = NULL;

	if (n_exp->a_c_notif_ptr != NULL) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].oldAttributePresent = SA_FALSE;
		error = saNtfPtrValAllocate(n_exp->nHandle,
				strlen(newValue) + 1,
				(void**) &temp,
				&n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue);
		if (error == SA_AIS_OK) {
			strncpy((char*) temp, newValue, strlen(newValue) + 1);
			n_exp->a_c_notif_ptr->changedAttributes[idx].attributeId = attrId;
			n_exp->a_c_notif_ptr->changedAttributes[idx].attributeType = SA_NTF_VALUE_STRING;
		} else {
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	return error;
}

/**
 * Set an expected attribute change value, type is BINARY/NAME.
 */
static SaAisErrorT set_attr_change_buf(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, SaNtfValueTypeT valType, const void* newValue)
{
	SaAisErrorT error = SA_AIS_OK;
	SaUint16T numAlloc = (valType == SA_NTF_VALUE_BINARY)
			? ((SaAnyT*) newValue)->bufferSize
			: ((SaNameT*) newValue)->length;
	SaUint8T* temp = NULL;
	SaUint8T* srcPtr = (valType == SA_NTF_VALUE_BINARY)
			? ((SaAnyT*) newValue)->bufferAddr
			: ((SaNameT*) newValue)->value;

	if (n_exp->a_c_notif_ptr != NULL) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].oldAttributePresent = SA_FALSE;
		error = saNtfPtrValAllocate(n_exp->nHandle,
				numAlloc,
				(void**) &temp,
				&n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue);
		if (error == SA_AIS_OK) {
			memcpy(temp, srcPtr, numAlloc);
			n_exp->a_c_notif_ptr->changedAttributes[idx].attributeId = attrId;
			n_exp->a_c_notif_ptr->changedAttributes[idx].attributeType = valType;
		} else {
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	return error;
}

/**
 * Set expected attribute change value, type is UINT32/INT32/UINT64/INT64/FLOAT/DOUBLE.
 */
static SaAisErrorT set_attr_change_scalar(NotifData* n_exp, SaUint16T idx,
		SaNtfElementIdT attrId, SaNtfValueTypeT valType, void* newValue)
{
	SaAisErrorT error = SA_AIS_OK;

	n_exp->a_c_notif_ptr->changedAttributes[idx].attributeId = attrId;
	n_exp->a_c_notif_ptr->changedAttributes[idx].attributeType = valType;
	n_exp->a_c_notif_ptr->changedAttributes[idx].oldAttributePresent = SA_FALSE;
	if (valType == SA_NTF_VALUE_UINT32) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.uint32Val = *(SaUint32T*) newValue;
	} else if (valType == SA_NTF_VALUE_INT32) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.int32Val = *(SaInt32T*) newValue;
	} else if (valType == SA_NTF_VALUE_UINT64) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.uint64Val = *(SaUint64T*) newValue;
	} else if (valType == SA_NTF_VALUE_INT64) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.int64Val = *(SaInt64T*) newValue;
	} else if (valType == SA_NTF_VALUE_FLOAT) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.floatVal = *(SaFloatT*) newValue;
	} else if (valType == SA_NTF_VALUE_DOUBLE) {
		n_exp->a_c_notif_ptr->changedAttributes[idx].newAttributeValue.doubleVal = *(SaDoubleT*) newValue;
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	return error;
}

/**
 * Create a runtime object in IMM.
 */
static void create_rt_test_object(const SaImmClassNameT cname, const char* dn,
		SaInt32T int32Var,
		SaUint32T uint32Var,
		SaInt64T int64Var,
		SaUint64T uint64Var,
		SaTimeT* timeVar,
		SaNameT* nameVar,
		SaFloatT floatVar,
		SaDoubleT doubleVar,
		SaStringT* stringVar,
		SaAnyT* anyVar)
{
	char* rdn = strchr(dn, '=') + 1;
	void* attr1[] = {&rdn};
	const SaImmAttrValuesT_2 attr_rdnVar = {
		.attrName = (strcmp(cname, "OsafNtfCmTestRT") == 0) ? "stringRdn" : "stringRdn1",
		.attrValueType = SA_IMM_ATTR_SASTRINGT,
		.attrValuesNumber = 1,
		.attrValues = attr1
	};
	void* attr2[] = {&int32Var};
	const SaImmAttrValuesT_2 attr_int32Var = {
		.attrName = "testInt32",
		.attrValueType = SA_IMM_ATTR_SAINT32T,
		.attrValuesNumber = 1,
		.attrValues = attr2
	};
	void* attr3[] = {&uint32Var};
	const SaImmAttrValuesT_2 attr_uint32Var = {
		.attrName = "testUint32",
		.attrValueType = SA_IMM_ATTR_SAUINT32T,
		.attrValuesNumber = 1,
		.attrValues = attr3
	};
	void* attr4[] = {&int64Var};
	const SaImmAttrValuesT_2 attr_int64Var = {
		.attrName = "testInt64",
		.attrValueType = SA_IMM_ATTR_SAINT64T,
		.attrValuesNumber = 1,
		.attrValues = attr4
	};
	void* attr5[] = {&uint64Var};
	const SaImmAttrValuesT_2 attr_uint64Var = {
		.attrName = "testUint64",
		.attrValueType = SA_IMM_ATTR_SAUINT64T,
		.attrValuesNumber = 1,
		.attrValues = attr5
	};
	void* attr6[] = {timeVar};
	const SaImmAttrValuesT_2 attr_timeVar = {
		.attrName = "testTime",
		.attrValueType = SA_IMM_ATTR_SATIMET,
		.attrValuesNumber = 1,
		.attrValues = attr6
	};
	void* attr7[] = {nameVar};
	const SaImmAttrValuesT_2 attr_nameVar = {
		.attrName = "testName",
		.attrValueType = SA_IMM_ATTR_SANAMET,
		.attrValuesNumber = 1,
		.attrValues = attr7
	};
	void* attr8[] = {&floatVar};
	const SaImmAttrValuesT_2 attr_floatVar = {
		.attrName = "testFloat",
		.attrValueType = SA_IMM_ATTR_SAFLOATT,
		.attrValuesNumber = 1,
		.attrValues = attr8
	};
	void* attr9[] = {&doubleVar};
	const SaImmAttrValuesT_2 attr_doubleVar = {
		.attrName = "testDouble",
		.attrValueType = SA_IMM_ATTR_SADOUBLET,
		.attrValuesNumber = 1,
		.attrValues = attr9
	};
	void* attr10[] = {stringVar};
	const SaImmAttrValuesT_2 attr_stringVar = {
		.attrName = "testString",
		.attrValueType = SA_IMM_ATTR_SASTRINGT,
		.attrValuesNumber = 1,
		.attrValues = attr10
	};
	void* attr11[] = {anyVar};
	const SaImmAttrValuesT_2 attr_anyVar = {
		.attrName = "testAny",
		.attrValueType = SA_IMM_ATTR_SAANYT,
		.attrValuesNumber = 1,
		.attrValues = attr11
	};
	const SaImmAttrValuesT_2 * attrVal[] = {
		&attr_rdnVar,
		&attr_int32Var,
		&attr_uint32Var,
		&attr_int64Var,
		&attr_uint64Var,
		&attr_timeVar,
		&attr_nameVar,
		&attr_floatVar,
		&attr_doubleVar,
		&attr_stringVar,
		&attr_anyVar,
		NULL
	};

	safassert(saImmOiInitialize_2(&immOiHnd, NULL, (SaVersionT*)&immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHnd, IMPLEMENTERNAME_RT), SA_AIS_OK);
	saImmOiRtObjectCreate_2(immOiHnd, cname, NULL, attrVal);
	safassert(saImmOiImplementerClear(immOiHnd), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHnd), SA_AIS_OK);
}

/**
 * Modify a runtime object.
 */
static void modify_rt_test_object(const char* dn, SaInt32T modType, TestAttributeValue* attr[])
{
	char* rdn = strchr(dn, '=') + 1;
	SaNameT objName;
	strcpy((char*) objName.value, rdn);
	objName.length = strlen(rdn) + 1;

	SaImmAttrModificationT_2 * attrMod[20];
	int i = 0;
	while (attr[i] != NULL) {
		attrMod[i] = malloc(sizeof (SaImmAttrModificationT_2));
		attrMod[i]->modType = modType;
		attrMod[i]->modAttr.attrName = (char*) attr[i]->attrName;
		attrMod[i]->modAttr.attrValueType = attr[i]->attrType;
		attrMod[i]->modAttr.attrValuesNumber = 1;
		attrMod[i]->modAttr.attrValues = attr[i]->attrValues;
		i++;
	}
	attrMod[i] = NULL;

	safassert(saImmOiInitialize_2(&immOiHnd, NULL, (SaVersionT*) & immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHnd, IMPLEMENTERNAME_RT), SA_AIS_OK);
	saImmOiRtObjectUpdate_2(immOiHnd, &objName, (const SaImmAttrModificationT_2 **) attrMod);
	safassert(saImmOiImplementerClear(immOiHnd), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHnd), SA_AIS_OK);
	i = 0;
	while (attrMod[i] != NULL) {
		free(attrMod[i++]);
	}
}

/**
 * Delete runtime object in IMM.
 */
static void delete_rt_test_object(const char* dn)
{
	char* rdn = strchr(dn, '=') + 1;
	SaNameT objName;
	strcpy((char*) objName.value, rdn);
	objName.length = strlen(rdn);

	safassert(saImmOiInitialize_2(&immOiHnd, NULL, (SaVersionT*) & immVersion), SA_AIS_OK);
	safassert(saImmOiImplementerSet(immOiHnd, IMPLEMENTERNAME_RT), SA_AIS_OK);
	saImmOiRtObjectDelete(immOiHnd, &objName);
	safassert(saImmOiImplementerClear(immOiHnd), SA_AIS_OK);
	safassert(saImmOiFinalize(immOiHnd), SA_AIS_OK);
}

/**
 * Create a runtime test object and verify correctness of generated notification.
 */
void objectCreateTest_01(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* create the runtime object */
	SaInt32T int32Var = INT32VAR1;
	SaUint32T uint32Var = UINT32VAR1;
	SaInt64T int64Var = INT64VAR1;
	SaUint64T uint64Var = UINT64VAR1;
	SaTimeT timeVar = TIMEVAR1;
	SaNameT nameVar = {.length = sizeof (NAME1)};
	memcpy(nameVar.value, NAME1, sizeof (NAME1));
	SaFloatT floatVar = FLOATVAR1;
	SaDoubleT doubleVar = DOUBLEVAR1;
	SaStringT stringVar = STRINGVAR1;
	SaAnyT anyVar = {.bufferSize = sizeof (BUF1), .bufferAddr = (SaUint8T*) BUF1};

	/*
	 * Create the object in IMM.
	 */
	create_rt_test_object("OsafNtfCmTestRT", DNTESTRT, int32Var, uint32Var, int64Var, uint64Var,
			&timeVar, &nameVar, floatVar, doubleVar, &stringVar, &anyVar);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_CREATION, DNTESTRT, 12, 12), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testInt32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testInt64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 6, 6, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 7, 7, "testFloat"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 8, 8, "testDouble"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 9, 9, "testName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 10, 10, "testTime"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 11, 11, "testAny"), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &uint32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 3, 3, SA_NTF_VALUE_INT32, &int32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT64, &uint64Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT64, &int64Var), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 6, 6, stringVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 7, 7, SA_NTF_VALUE_FLOAT, &floatVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 8, 8, SA_NTF_VALUE_DOUBLE, &doubleVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 9, 9, SA_NTF_VALUE_LDAP_NAME, &nameVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 10, 10, SA_NTF_VALUE_INT64, &timeVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 11, 11, SA_NTF_VALUE_BINARY, &anyVar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_02(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaUint32T var1 = UINT32VAR2;
	SaFloatT var2 = FLOATVAR2;
	void* v1[] = {&var1};
	void* v2[] = {&var2};
	TestAttributeValue att1 = {
		.attrName = "testUint32", .attrType = SA_IMM_ATTR_SAUINT32T, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testFloat", .attrType = SA_IMM_ATTR_SAFLOATT, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Modify (REPLACE) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_REPLACE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 4, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testFloat"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &var1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_FLOAT, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_03(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaNameT var1 = {.length = sizeof (NAME2)};
	memcpy(var1.value, NAME2, sizeof (NAME2));
	SaAnyT var2 = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};
	void* v1[] = {&var1};
	void* v2[] = {&var2};
	TestAttributeValue att1 = {
		.attrName = "testName", .attrType = SA_IMM_ATTR_SANAMET, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testAny", .attrType = SA_IMM_ATTR_SAANYT, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Modify (REPLACE) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_REPLACE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 4, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testAny"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 2, 2, SA_NTF_VALUE_LDAP_NAME, &var1), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 3, 3, SA_NTF_VALUE_BINARY, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, INT32) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_04(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaInt32T oldvar = INT32VAR1;
	SaInt32T addvar = INT32VAR2;
	void* v[] = {&addvar};

	TestAttributeValue att = {
		.attrName = "testInt32", .attrType = SA_IMM_ATTR_SAINT32T, .attrValues = v
	};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	TestAttributeValue * attrs[] = {&att, NULL};
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testInt32"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT32, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_INT32, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, UINT32) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_05(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaUint32T oldvar = UINT32VAR2;
	SaInt32T addvar = UINT32VAR3;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testUint32", .attrType = SA_IMM_ATTR_SAUINT32T, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint32"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_UINT32, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, INT64) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_06(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaInt64T oldvar = INT64VAR1;
	SaInt64T addvar = INT64VAR2;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testInt64", .attrType = SA_IMM_ATTR_SAINT64T, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testInt64"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT64, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_INT64, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, UINT64) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_07(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaUint64T oldvar = UINT64VAR1;
	SaUint64T addvar = UINT64VAR2;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testUint64", .attrType = SA_IMM_ATTR_SAUINT64T, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint64"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_UINT64, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, FLOAT) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_08(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaFloatT oldvar = FLOATVAR2;
	SaFloatT addvar = FLOATVAR3;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testFloat", .attrType = SA_IMM_ATTR_SAFLOATT, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testFloat"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_FLOAT, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_FLOAT, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, DOUBLE) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_09(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaDoubleT oldvar = DOUBLEVAR1;
	SaDoubleT addvar = DOUBLEVAR2;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testDouble", .attrType = SA_IMM_ATTR_SADOUBLET, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testDouble"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_DOUBLE, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_DOUBLE, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, TIME) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_10(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaTimeT oldvar = TIMEVAR1;
	SaTimeT addvar = TIMEVAR2;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testTime", .attrType = SA_IMM_ATTR_SATIMET, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testTime"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT64, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_INT64, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, STRING) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_11(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaStringT oldvar = STRINGVAR1;
	SaStringT addvar = STRINGVAR2;
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testString", .attrType = SA_IMM_ATTR_SASTRINGT, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testString"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 2, 2, oldvar), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 3, 2, addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, NAME) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_12(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaNameT oldvar = {.length = sizeof (NAME2)};
	memcpy(oldvar.value, NAME2, sizeof (NAME2));
	SaNameT addvar = {.length = sizeof (NAME3)};
	memcpy(addvar.value, NAME3, sizeof (NAME3));
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testName", .attrType = SA_IMM_ATTR_SANAMET, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testName"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 2, 2, SA_NTF_VALUE_LDAP_NAME, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 3, 2, SA_NTF_VALUE_LDAP_NAME, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, ANY) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_13(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaAnyT oldvar = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};
	SaAnyT addvar = {.bufferSize = sizeof (BUF3), .bufferAddr = (SaUint8T*) BUF3};
	void* v[] = {&addvar};
	TestAttributeValue att = {
		.attrName = "testAny", .attrType = SA_IMM_ATTR_SAANYT, .attrValues = v
	};
	TestAttributeValue * attrs[] = {&att, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testAny"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 2, 2, SA_NTF_VALUE_BINARY, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 3, 2, SA_NTF_VALUE_BINARY, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_14(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaStringT var1 = STRINGVAR3;
	SaDoubleT var2 = DOUBLEVAR3;
	void* v1[] = {&var1};
	void* v2[] = {&var2};
	TestAttributeValue att1 = {
		.attrName = "testString", .attrType = SA_IMM_ATTR_SASTRINGT, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testDouble", .attrType = SA_IMM_ATTR_SADOUBLET, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Modify (REPLACE) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_REPLACE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 4, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testDouble"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 2, 2, var1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_DOUBLE, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_15(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaTimeT var1 = TIMEVAR3;
	void* v1[] = {&var1};
	TestAttributeValue att1 = {
		.attrName = "testTime", .attrType = SA_IMM_ATTR_SATIMET, .attrValues = v1
	};
	TestAttributeValue * attrs[] = {&att1, NULL};

	/*
	 * Modify (REPLACE) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_REPLACE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 3, 3), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testTime"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT64, &var1), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_16(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaStringT svar1 = STRINGVAR3;
	SaStringT svar2 = STRINGVAR2;
	SaInt32T ivar1 = INT32VAR1;
	SaInt32T ivar2 = INT32VAR2;
	SaInt32T ivar3 = INT32VAR3;
	void* v1[] = {&svar2};
	void* v2[] = {&ivar3};
	TestAttributeValue att1 = {
		.attrName = "testString", .attrType = SA_IMM_ATTR_SASTRINGT, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testInt32", .attrType = SA_IMM_ATTR_SAINT32T, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 4, 7), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testInt32"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 2, 2, svar1), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 3, 2, svar2), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 3, SA_NTF_VALUE_INT32, &ivar1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 3, SA_NTF_VALUE_INT32, &ivar2), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 6, 3, SA_NTF_VALUE_INT32, &ivar3), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_17(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaStringT svar1 = STRINGVAR3;
	SaStringT svar2 = STRINGVAR2;
	SaInt32T var1 = INT32VAR1;
	SaInt32T var2 = INT32VAR2;
	SaInt32T var3 = INT32VAR3;
	void* v1[] = {&svar1};
	void* v2[] = {&var1};
	TestAttributeValue att1 = {
		.attrName = "testString", .attrType = SA_IMM_ATTR_SASTRINGT, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testInt32", .attrType = SA_IMM_ATTR_SAINT32T, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Attribute change, DELETE
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_DELETE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 4, 5), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testInt32"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 2, 2, svar2), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_INT32, &var3), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 3, SA_NTF_VALUE_INT32, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_18(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaInt32T var1 = INT32VAR1, var11 = INT32VAR2, var111 = INT32VAR3;
	SaUint32T var2 = UINT32VAR1, var22 = UINT32VAR2, var222 = UINT32VAR3;
	SaInt64T var3 = INT64VAR1, var33 = INT64VAR2, var333 = INT64VAR3;
	SaUint64T var4 = UINT64VAR1, var44 = UINT64VAR2, var444 = UINT64VAR3;
	SaFloatT var5 = FLOATVAR1, var55 = FLOATVAR2, var555 = FLOATVAR3;
	SaDoubleT var66 = DOUBLEVAR2, var666 = DOUBLEVAR3;
	SaTimeT var77 = TIMEVAR2, var777 = TIMEVAR3;
	SaStringT var88 = STRINGVAR2, var888 = STRINGVAR3;
	SaNameT var9 = {.length = sizeof (NAME1)};
	memcpy(var9.value, NAME1, sizeof (NAME1));
	SaNameT var99 = {.length = sizeof (NAME2)};
	memcpy(var99.value, NAME2, sizeof (NAME2));
	SaNameT var999 = {.length = sizeof (NAME3)};
	memcpy(var999.value, NAME3, sizeof (NAME3));
	SaAnyT var10 = {.bufferSize = sizeof (BUF1), .bufferAddr = (SaUint8T*) BUF1};
	SaAnyT var100 = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};
	SaAnyT var1000 = {.bufferSize = sizeof (BUF3), .bufferAddr = (SaUint8T*) BUF3};
	void* v[] = {&var1, &var2, &var333, &var444, &var5, &var66, &var77, &var888, &var9, &var10};

	TestAttributeValue att1 = {
		.attrName = "testInt32", .attrType = SA_IMM_ATTR_SAINT32T, .attrValues = &v[0]
	};
	TestAttributeValue att2 = {
		.attrName = "testUint32", .attrType = SA_IMM_ATTR_SAUINT32T, .attrValues = &v[1]
	};
	TestAttributeValue att3 = {
		.attrName = "testInt64", .attrType = SA_IMM_ATTR_SAINT64T, .attrValues = &v[2]
	};
	TestAttributeValue att4 = {
		.attrName = "testUint64", .attrType = SA_IMM_ATTR_SAUINT64T, .attrValues = &v[3]
	};
	TestAttributeValue att5 = {
		.attrName = "testFloat", .attrType = SA_IMM_ATTR_SAFLOATT, .attrValues = &v[4]
	};
	TestAttributeValue att6 = {
		.attrName = "testDouble", .attrType = SA_IMM_ATTR_SADOUBLET, .attrValues = &v[5]
	};
	TestAttributeValue att7 = {
		.attrName = "testTime", .attrType = SA_IMM_ATTR_SATIMET, .attrValues = &v[6]
	};
	TestAttributeValue att8 = {
		.attrName = "testString", .attrType = SA_IMM_ATTR_SASTRINGT, .attrValues = &v[7]
	};
	TestAttributeValue att9 = {
		.attrName = "testName", .attrType = SA_IMM_ATTR_SANAMET, .attrValues = &v[8]
	};
	TestAttributeValue att10 = {
		.attrName = "testAny", .attrType = SA_IMM_ATTR_SAANYT, .attrValues = &v[9]
	};
	TestAttributeValue * attrs[] = {&att1, &att2, &att3, &att4, &att5, &att6,
		&att7, &att8, &att9, &att10, NULL};

	/*
	 * Attribute change, ADD
	 */
	modify_rt_test_object(DNTESTRT, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT, 12, 29), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testInt32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testUint32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testInt64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testUint64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 6, 6, "testFloat"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 7, 7, "testDouble"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 8, 8, "testTime"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 9, 9, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 10, 10, "testName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 11, 11, "testAny"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT32, &var1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_INT32, &var11), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 2, SA_NTF_VALUE_INT32, &var111), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 3, SA_NTF_VALUE_UINT32, &var2), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 6, 3, SA_NTF_VALUE_UINT32, &var22), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 7, 3, SA_NTF_VALUE_UINT32, &var222), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 8, 4, SA_NTF_VALUE_INT64, &var3), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 9, 4, SA_NTF_VALUE_INT64, &var33), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 10, 4, SA_NTF_VALUE_INT64, &var333), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 11, 5, SA_NTF_VALUE_UINT64, &var4), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 12, 5, SA_NTF_VALUE_UINT64, &var44), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 13, 5, SA_NTF_VALUE_UINT64, &var444), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 14, 6, SA_NTF_VALUE_FLOAT, &var5), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 15, 6, SA_NTF_VALUE_FLOAT, &var55), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 16, 6, SA_NTF_VALUE_FLOAT, &var555), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 17, 7, SA_NTF_VALUE_DOUBLE, &var66), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 18, 7, SA_NTF_VALUE_DOUBLE, &var666), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 19, 8, SA_NTF_VALUE_INT64, &var77), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 20, 8, SA_NTF_VALUE_INT64, &var777), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 21, 9, var88), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 22, 9, var888), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 23, 10, SA_NTF_VALUE_LDAP_NAME, &var9), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 24, 10, SA_NTF_VALUE_LDAP_NAME, &var99), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 25, 10, SA_NTF_VALUE_LDAP_NAME, &var999), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 26, 11, SA_NTF_VALUE_BINARY, &var10), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 27, 11, SA_NTF_VALUE_BINARY, &var100), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 28, 11, SA_NTF_VALUE_BINARY, &var1000), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Delete a runtime test object and verify correctness of generated notification.
 */
void objectDeleteTest_19(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* delete the runtime object */
	delete_rt_test_object(DNTESTRT);

	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_DELETION, DNTESTRT, 0, 0), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Create an object and verify correctness of generated notification.
 */
void objectCreateTest_20(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* create an object */
	sprintf(command, "immcfg -t 20 -c OsafNtfCmTestCFG %s -a testNameCfg=%s -a testStringCfg=%s -a testAnyCfg=%s",
			DNTESTCFG, NAME1, STRINGVAR1, BUF1);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		SaInt32T int32Var = INT32VAR1;
		SaUint32T uint32Var = UINT32VAR1;
		SaInt64T int64Var = INT64VAR1;
		SaUint64T uint64Var = UINT64VAR1;
		SaTimeT timeVar = TIMEVAR1;
		SaNameT nameVar = {.length = sizeof (NAME1)};
		memcpy(nameVar.value, NAME1, sizeof (NAME1));
		SaFloatT floatVar = FLOATVAR1;
		SaDoubleT doubleVar = DOUBLEVAR1;
		SaStringT stringVar = STRINGVAR1;
		SaAnyT anyVar = {.bufferSize = sizeof (BUF1), .bufferAddr = (SaUint8T*) BUF1};

		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_CREATION, DNTESTCFG, 14, 14), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint32Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testInt32Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 6, 6, "testUint64Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 7, 7, "testInt64Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 8, 8, "testStringCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 9, 9, "testFloatCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 10, 10, "testDoubleCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 11, 11, "testNameCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 12, 12, "testTimeCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 13, 13, "testAnyCfg"), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 2;
		safassert(set_attr_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT32, &uint32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT32, &int32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 6, 6, SA_NTF_VALUE_UINT64, &uint64Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 7, 7, SA_NTF_VALUE_INT64, &int64Var), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 8, 8, stringVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 9, 9, SA_NTF_VALUE_FLOAT, &floatVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 10, 10, SA_NTF_VALUE_DOUBLE, &doubleVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 11, 11, SA_NTF_VALUE_LDAP_NAME, &nameVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 12, 12, SA_NTF_VALUE_INT64, &timeVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 13, 13, SA_NTF_VALUE_BINARY, &anyVar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_21(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaUint32T ivar = UINT32VAR2;
	SaFloatT fvar = FLOATVAR2;
	sprintf(command, "immcfg -t 20 -a testUint32Cfg=%u -a testFloatCfg=%f %s", ivar, fvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint32Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testFloatCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT32, &ivar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 5, SA_NTF_VALUE_FLOAT, &fvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_22(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaNameT var1 = {.length = sizeof (NAME2)};
	memcpy(var1.value, NAME2, sizeof (NAME2));
	SaAnyT var2 = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};

	/* modify an object */
	sprintf(command, "immcfg -t 20 -a testNameCfg=%s -a testAnyCfg=%s %s",
			NAME2, BUF2, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testNameCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testAnyCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 4, 4, SA_NTF_VALUE_LDAP_NAME, &var1), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 5, 5, SA_NTF_VALUE_BINARY, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_23(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaInt32T addvar = INT32VAR2;
	SaInt32T oldvar = INT32VAR1;
	sprintf(command, "immcfg -t 20 -a testInt32Cfg+=%d %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testInt32Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT32, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_INT32, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_24(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaUint32T addvar = UINT32VAR3;
	SaUint32T oldvar = UINT32VAR2;
	sprintf(command, "immcfg -t 20 -a testUint32Cfg+=%u %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint32Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT32, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_UINT32, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_25(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaInt64T addvar = INT64VAR2;
	SaInt64T oldvar = INT64VAR1;
	sprintf(command, "immcfg -t 20 -a testInt64Cfg+=%lld %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testInt64Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT64, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_INT64, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_26(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaUint64T addvar = UINT64VAR2;
	SaUint64T oldvar = UINT64VAR1;
	sprintf(command, "immcfg -t 20 -a testUint64Cfg+=%llu %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint64Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT64, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_UINT64, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_27(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaFloatT addvar = FLOATVAR3;
	SaFloatT oldvar = FLOATVAR2;
	sprintf(command, "immcfg -t 20 -a testFloatCfg+=%f %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testFloatCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_FLOAT, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_FLOAT, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_28(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaDoubleT addvar = DOUBLEVAR2;
	SaDoubleT oldvar = DOUBLEVAR1;
	sprintf(command, "immcfg -t 20 -a testDoubleCfg+=%lf %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testDoubleCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_DOUBLE, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_DOUBLE, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_29(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaTimeT addvar = TIMEVAR2;
	SaTimeT oldvar = TIMEVAR1;
	sprintf(command, "immcfg -t 20 -a testTimeCfg+=%lld %s", (SaInt64T) addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testTimeCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT64, &addvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_INT64, &oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_30(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaStringT addvar = STRINGVAR2;
	SaStringT oldvar = STRINGVAR1;
	sprintf(command, "immcfg -t 20 -a testStringCfg+=%s %s", addvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testStringCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 4, 4, addvar), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 5, 4, oldvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_31(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaNameT oldvar = {.length = sizeof (NAME2)};
	memcpy(oldvar.value, NAME2, sizeof (NAME2));
	SaNameT addvar = {.length = sizeof (NAME3)};
	memcpy(addvar.value, NAME3, sizeof (NAME3));
	sprintf(command, "immcfg -a testNameCfg+=%s %s", NAME3, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testNameCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 4, 4, SA_NTF_VALUE_LDAP_NAME, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 5, 4, SA_NTF_VALUE_LDAP_NAME, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_32(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaAnyT oldvar = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};
	SaAnyT addvar = {.bufferSize = sizeof (BUF3), .bufferAddr = (SaUint8T*) BUF3};
	sprintf(command, "immcfg -t 20 -a testAnyCfg+=%s %s", BUF3, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testAnyCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 4, 4, SA_NTF_VALUE_BINARY, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 5, 4, SA_NTF_VALUE_BINARY, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_33(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaStringT svar = STRINGVAR3;
	SaDoubleT dvar = DOUBLEVAR3;
	sprintf(command, "immcfg -t 20 -a testStringCfg=%s -a testDoubleCfg=%lf %s", svar, dvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 6), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testStringCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testDoubleCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 4, 4, svar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 5, SA_NTF_VALUE_DOUBLE, &dvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_34(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaTimeT tvar = TIMEVAR3;
	sprintf(command, "immcfg -t 20 -a testTimeCfg=%lld %s", (SaInt64T) tvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 5), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testTimeCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT64, &tvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_35(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaStringT saddvar = STRINGVAR2;
	SaStringT soldvar = STRINGVAR3;
	SaInt32T iaddvar = INT32VAR3;
	SaInt32T ioldvar1 = INT32VAR1;
	SaInt32T ioldvar2 = INT32VAR2;
	sprintf(command, "immcfg -t 20 -a testStringCfg+=%s -a testInt32Cfg+=%d %s", saddvar, iaddvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 9), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testStringCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testInt32Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 4, 4, saddvar), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 5, 4, soldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 6, 5, SA_NTF_VALUE_INT32, &iaddvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 7, 5, SA_NTF_VALUE_INT32, &ioldvar1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 8, 5, SA_NTF_VALUE_INT32, &ioldvar2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_36(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaStringT sdelvar = STRINGVAR2;
	SaStringT soldvar = STRINGVAR3;
	SaInt32T idelvar = INT32VAR2;
	SaInt32T ivar1 = INT32VAR1;
	SaInt32T ivar2 = INT32VAR3;
	sprintf(command, "immcfg -t 20 -a testStringCfg-=%s -a testInt32Cfg-=%d %s", sdelvar, idelvar, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 7), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testStringCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testInt32Cfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 4, 4, soldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT32, &ivar1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 6, 5, SA_NTF_VALUE_INT32, &ivar2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectModifyTest_37(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaInt32T i32var1 = INT32VAR1, i32var11 = INT32VAR2, i32var111 = INT32VAR3;
	SaUint32T ui32var2 = UINT32VAR1, ui32var22 = UINT32VAR2, ui32var222 = UINT32VAR3;
	SaInt64T i64var3 = INT64VAR1, i64var33 = INT64VAR2, i64var333 = INT64VAR3;
	SaUint64T ui64var4 = UINT64VAR1, ui64var44 = UINT64VAR2, ui64var444 = UINT64VAR3;
	SaFloatT fvar5 = FLOATVAR1, fvar55 = FLOATVAR2, fvar555 = FLOATVAR3;
	SaDoubleT dvar66 = DOUBLEVAR2, dvar666 = DOUBLEVAR3;
	SaTimeT tvar77 = TIMEVAR2, tvar777 = TIMEVAR3;
	SaStringT svar8 = STRINGVAR1, svar888 = STRINGVAR3;
	SaNameT nvar9 = {.length = sizeof (NAME1)};
	memcpy(nvar9.value, NAME1, sizeof (NAME1));
	SaNameT nvar99 = {.length = sizeof (NAME2)};
	memcpy(nvar99.value, NAME2, sizeof (NAME2));
	SaNameT nvar999 = {.length = sizeof (NAME3)};
	memcpy(nvar999.value, NAME3, sizeof (NAME3));
	SaAnyT avar10 = {.bufferSize = sizeof (BUF1), .bufferAddr = (SaUint8T*) BUF1};
	SaAnyT avar100 = {.bufferSize = sizeof (BUF2), .bufferAddr = (SaUint8T*) BUF2};
	SaAnyT avar1000 = {.bufferSize = sizeof (BUF3), .bufferAddr = (SaUint8T*) BUF3};

	sprintf(command, "immcfg -t 20 -a testInt32Cfg+=%d -a testUint32Cfg+=%u"
			" -a testInt64Cfg+=%lld -a testUint64Cfg+=%llu"
			" -a testFloatCfg+=%f -a testDoubleCfg+=%f"
			" -a testTimeCfg+=%lld -a testStringCfg+=%s"
			" -a testNameCfg+=%s -a testAnyCfg+=%s %s"
			, i32var11, ui32var2, i64var333, ui64var444, fvar5, dvar66, tvar77,
			svar8, NAME1, BUF1, DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 14, 31), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testInt32Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testUint32Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 6, 6, "testInt64Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 7, 7, "testUint64Cfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 8, 8, "testFloatCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 9, 9, "testDoubleCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 10, 10, "testTimeCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 11, 11, "testStringCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 12, 12, "testNameCfg"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 13, 13, "testAnyCfg"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT32, &i32var1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_INT32, &i32var11), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 6, 4, SA_NTF_VALUE_INT32, &i32var111), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 7, 5, SA_NTF_VALUE_UINT32, &ui32var2), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 8, 5, SA_NTF_VALUE_UINT32, &ui32var22), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 9, 5, SA_NTF_VALUE_UINT32, &ui32var222), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 10, 6, SA_NTF_VALUE_INT64, &i64var3), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 11, 6, SA_NTF_VALUE_INT64, &i64var33), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 12, 6, SA_NTF_VALUE_INT64, &i64var333), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 13, 7, SA_NTF_VALUE_UINT64, &ui64var4), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 14, 7, SA_NTF_VALUE_UINT64, &ui64var44), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 15, 7, SA_NTF_VALUE_UINT64, &ui64var444), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 16, 8, SA_NTF_VALUE_FLOAT, &fvar5), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 17, 8, SA_NTF_VALUE_FLOAT, &fvar55), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 18, 8, SA_NTF_VALUE_FLOAT, &fvar555), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 19, 9, SA_NTF_VALUE_DOUBLE, &dvar66), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 20, 9, SA_NTF_VALUE_DOUBLE, &dvar666), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 21, 10, SA_NTF_VALUE_INT64, &tvar77), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 22, 10, SA_NTF_VALUE_INT64, &tvar777), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 23, 11, svar8), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 24, 11, svar888), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 25, 12, SA_NTF_VALUE_LDAP_NAME, &nvar9), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 26, 12, SA_NTF_VALUE_LDAP_NAME, &nvar99), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 27, 12, SA_NTF_VALUE_LDAP_NAME, &nvar999), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 28, 13, SA_NTF_VALUE_BINARY, &avar10), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 29, 13, SA_NTF_VALUE_BINARY, &avar100), SA_AIS_OK);
		safassert(set_attr_change_buf(&n_exp, 30, 13, SA_NTF_VALUE_BINARY, &avar1000), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectMultiCcbTest_38(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaUint32T var1 = 32000;
	SaInt32T var2 = -32000;
	SaUint64T var3 = 64000;
	SaInt64T var4 = -64000;
	SaImmHandleT omHandle = 0;
	SaImmAdminOwnerHandleT ownerHandle = 0;
	SaImmCcbHandleT immCcbHandle = 0;
	static SaNameT objectName;
	objectName.length = (SaUint16T) strlen(DNTESTCFG);
	memcpy(objectName.value, DNTESTCFG, objectName.length);

	void* val1[] = {&var1};
	SaImmAttrModificationT_2 am1 = {.modType = SA_IMM_ATTR_VALUES_REPLACE};
	am1.modAttr.attrName = "testUint32Cfg";
	am1.modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
	am1.modAttr.attrValuesNumber = 1;
	am1.modAttr.attrValues = val1;

	void* val2[] = {&var2};
	SaImmAttrModificationT_2 am2 = {.modType = SA_IMM_ATTR_VALUES_REPLACE};
	am2.modAttr.attrName = "testInt32Cfg";
	am2.modAttr.attrValueType = SA_IMM_ATTR_SAINT32T;
	am2.modAttr.attrValuesNumber = 1;
	am2.modAttr.attrValues = val2;

	void* val3[] = {&var3};
	SaImmAttrModificationT_2 am3 = {.modType = SA_IMM_ATTR_VALUES_REPLACE};
	am3.modAttr.attrName = "testUint64Cfg";
	am3.modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
	am3.modAttr.attrValuesNumber = 1;
	am3.modAttr.attrValues = val3;

	void* val4[] = {&var4};
	SaImmAttrModificationT_2 am4 = {.modType = SA_IMM_ATTR_VALUES_REPLACE};
	am4.modAttr.attrName = "testInt64Cfg";
	am4.modAttr.attrValueType = SA_IMM_ATTR_SAINT64T;
	am4.modAttr.attrValuesNumber = 1;
	am4.modAttr.attrValues = val4;

	SaImmAttrModificationT_2 * attrMod1[] = {&am1, &am2, NULL};
	SaImmAttrModificationT_2 * attrMod2[] = {&am3, &am4, NULL};
	safassert(immutil_saImmOmInitialize(&omHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerInitialize(omHandle, "multiCcbOwner", SA_TRUE,
			&ownerHandle), SA_AIS_OK);
	const SaNameT * objName[] = {&objectName, NULL};
	safassert(immutil_saImmOmAdminOwnerSet(ownerHandle,
			objName,
			SA_IMM_ONE), SA_AIS_OK);
	safassert(immutil_saImmOmCcbInitialize(ownerHandle, 0, &immCcbHandle), SA_AIS_OK);
	safassert(immutil_saImmOmCcbObjectModify_2(immCcbHandle,
			(const SaNameT*) &objectName,
			(const SaImmAttrModificationT_2**) attrMod1), SA_AIS_OK);
	safassert(immutil_saImmOmCcbObjectModify_2(immCcbHandle,
			(const SaNameT*) &objectName,
			(const SaImmAttrModificationT_2**) attrMod2), SA_AIS_OK);

	safassert(immutil_saImmOmCcbApply(immCcbHandle), SA_AIS_OK);
	safassert(immutil_saImmOmCcbFinalize(immCcbHandle), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerRelease(ownerHandle,
			objName,
			SA_IMM_ONE), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(immutil_saImmOmFinalize(omHandle), SA_AIS_OK);

	int noOfNotifs = 0;
	while (noOfNotifs < 2) {
		/*
		 * Wait for notification reception.
		 */
		int dwCnt = 0;
		while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
			saNtfDispatch(ntfHandle, SA_DISPATCH_ONE);
			if (!rec_notif_data.populated) usleep(100);
		}

		NotifData n_exp;
		if (rec_notif_data.populated && noOfNotifs == 0) {
			safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 6), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 4, 4, "testUint32Cfg"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 5, 5, "testInt32Cfg"), SA_AIS_OK);
			safassert(set_attr_change_str(&n_exp, 0, 0, "multiCcbOwner"), SA_AIS_OK);
			safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
			SaUint64T ccidVar = 3;
			safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
			SaUint32T ccbLast = 0;
			safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
			safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT32, &var1), SA_AIS_OK);
			safassert(set_attr_change_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT32, &var2), SA_AIS_OK);
		} else if (rec_notif_data.populated && noOfNotifs == 1) {
			safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 6, 6), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 4, 4, "testUint64Cfg"), SA_AIS_OK);
			safassert(set_add_info(&n_exp, 5, 5, "testInt64Cfg"), SA_AIS_OK);
			safassert(set_attr_change_str(&n_exp, 0, 0, "multiCcbOwner"), SA_AIS_OK);
			safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
			SaUint64T ccidVar = 3;
			safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
			SaUint32T ccbLast = 1;
			safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
			safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT64, &var3), SA_AIS_OK);
			safassert(set_attr_change_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT64, &var4), SA_AIS_OK);
		} else {
			error = SA_AIS_ERR_FAILED_OPERATION;
			break;
		}
		rec_notif_data.populated = SA_FALSE;
		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
		noOfNotifs++;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify an object and verify correctness of generated notification.
 */
void objectMultiCcbTest_39(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify an object */
	SaUint32T var1 = 32323;
	SaUint32T oldvar1 = 32000;
	SaInt32T var2 = -32323;
	SaUint64T var3 = 64000;
	SaInt64T var4 = -64000;
	SaDoubleT var5 = DOUBLEVAR2;
	SaDoubleT oldvar5 = DOUBLEVAR3;
	SaFloatT var6 = FLOATVAR2;
	SaFloatT oldvar6 = FLOATVAR1;
	SaFloatT oldvar66 = FLOATVAR3;
	SaImmHandleT omHandle = 0;
	SaImmAdminOwnerHandleT ownerHandle = 0;
	SaImmCcbHandleT immCcbHandle = 0;
	static SaNameT objectName;
	objectName.length = (SaUint16T) strlen(DNTESTCFG);
	memcpy(objectName.value, DNTESTCFG, objectName.length);

	void* val1[] = {&var1};
	SaImmAttrModificationT_2 am1 = {.modType = SA_IMM_ATTR_VALUES_ADD};
	am1.modAttr.attrName = "testUint32Cfg";
	am1.modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
	am1.modAttr.attrValuesNumber = 1;
	am1.modAttr.attrValues = val1;

	void* val2[] = {&var2};
	SaImmAttrModificationT_2 am2 = {.modType = SA_IMM_ATTR_VALUES_REPLACE};
	am2.modAttr.attrName = "testInt32Cfg";
	am2.modAttr.attrValueType = SA_IMM_ATTR_SAINT32T;
	am2.modAttr.attrValuesNumber = 1;
	am2.modAttr.attrValues = val2;

	void* val3[] = {&var3};
	SaImmAttrModificationT_2 am3 = {.modType = SA_IMM_ATTR_VALUES_DELETE};
	am3.modAttr.attrName = "testUint64Cfg";
	am3.modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
	am3.modAttr.attrValuesNumber = 1;
	am3.modAttr.attrValues = val3;

	void* val4[] = {&var4};
	SaImmAttrModificationT_2 am4 = {.modType = SA_IMM_ATTR_VALUES_DELETE};
	am4.modAttr.attrName = "testInt64Cfg";
	am4.modAttr.attrValueType = SA_IMM_ATTR_SAINT64T;
	am4.modAttr.attrValuesNumber = 1;
	am4.modAttr.attrValues = val4;

	void* val5[] = {&var5};
	SaImmAttrModificationT_2 am5 = {.modType = SA_IMM_ATTR_VALUES_DELETE};
	am5.modAttr.attrName = "testDoubleCfg";
	am5.modAttr.attrValueType = SA_IMM_ATTR_SADOUBLET;
	am5.modAttr.attrValuesNumber = 1;
	am5.modAttr.attrValues = val5;

	void* val6[] = {&var6};
	SaImmAttrModificationT_2 am6 = {.modType = SA_IMM_ATTR_VALUES_DELETE};
	am6.modAttr.attrName = "testFloatCfg";
	am6.modAttr.attrValueType = SA_IMM_ATTR_SAFLOATT;
	am6.modAttr.attrValuesNumber = 1;
	am6.modAttr.attrValues = val6;

	SaImmAttrModificationT_2 * attrMod1[] = {&am1, NULL};
	SaImmAttrModificationT_2 * attrMod2[] = {&am2, NULL};
	SaImmAttrModificationT_2 * attrMod3[] = {&am3, &am4, &am5, &am6, NULL};

	safassert(immutil_saImmOmInitialize(&omHandle, NULL, &immVersion), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerInitialize(omHandle, "multiCcbOwner", SA_TRUE,
			&ownerHandle), SA_AIS_OK);
	const SaNameT * objName[] = {&objectName, NULL};
	safassert(immutil_saImmOmAdminOwnerSet(ownerHandle,
			objName,
			SA_IMM_ONE), SA_AIS_OK);

	/* Initialize CCB */
	safassert(immutil_saImmOmCcbInitialize(ownerHandle, 0, &immCcbHandle), SA_AIS_OK);

	/* modify ADD */
	safassert(immutil_saImmOmCcbObjectModify_2(immCcbHandle,
			(const SaNameT*) &objectName,
			(const SaImmAttrModificationT_2**) attrMod1), SA_AIS_OK);
	/* modify REPLACE */
	safassert(immutil_saImmOmCcbObjectModify_2(immCcbHandle,
			(const SaNameT*) &objectName,
			(const SaImmAttrModificationT_2**) attrMod2), SA_AIS_OK);
	/* modify DELETE */
	safassert(immutil_saImmOmCcbObjectModify_2(immCcbHandle,
			(const SaNameT*) &objectName,
			(const SaImmAttrModificationT_2**) attrMod3), SA_AIS_OK);

	safassert(immutil_saImmOmCcbApply(immCcbHandle), SA_AIS_OK);

	/* Finalize CCB */
	safassert(immutil_saImmOmCcbFinalize(immCcbHandle), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerRelease(ownerHandle,
			objName,
			SA_IMM_ONE), SA_AIS_OK);
	safassert(immutil_saImmOmAdminOwnerFinalize(ownerHandle), SA_AIS_OK);
	safassert(immutil_saImmOmFinalize(omHandle), SA_AIS_OK);

	int noOfNotifs = 0;
	/* three notifications (same CCB) are expected */
    while (noOfNotifs < 3) {
        /*
         * Wait for notification reception.
         */
        int dwCnt = 0;
        while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
            saNtfDispatch(ntfHandle, SA_DISPATCH_ONE);
            if (!rec_notif_data.populated) usleep(100);
        }

        NotifData n_exp;
        if (rec_notif_data.populated && noOfNotifs == 0) {
            safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 6), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 4, 4, "testUint32Cfg"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 0, 0, "multiCcbOwner"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
            SaUint64T ccidVar = 3;
            safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
            SaUint32T ccbLast = 0;
            safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT32, &var1), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 5, 4, SA_NTF_VALUE_UINT32, &oldvar1), SA_AIS_OK);
        } else if (rec_notif_data.populated && noOfNotifs == 1) {
            safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 5, 5), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 4, 4, "testInt32Cfg"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 0, 0, "multiCcbOwner"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
            SaUint64T ccidVar = 3;
            safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
            SaUint32T ccbLast = 0;
            safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 4, 4, SA_NTF_VALUE_INT32, &var2), SA_AIS_OK);
        } else if (rec_notif_data.populated && noOfNotifs == 2) {

            /*
             * the non-multivalue attributes testUint64Cfg and testInt64Cfg was deleted (see
             * earlier TCs), therefor no attribute values exist, only additional info.
             */
            safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTCFG, 8, 7), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 2, 2, "SaImmOiCcbIdT"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 3, 3, "ccbLast"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 4, 4, "testUint64Cfg"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 5, 5, "testInt64Cfg"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 6, 6, "testFloatCfg"), SA_AIS_OK);
            safassert(set_add_info(&n_exp, 7, 7, "testDoubleCfg"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 0, 0, "multiCcbOwner"), SA_AIS_OK);
            safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestCFG"), SA_AIS_OK);
            SaUint64T ccidVar = 3;
            safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
            SaUint32T ccbLast = 1; /* Last notification of the CCB */
            safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 4, 6, SA_NTF_VALUE_FLOAT, &oldvar6), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 5, 6, SA_NTF_VALUE_FLOAT, &oldvar66), SA_AIS_OK);
            safassert(set_attr_change_scalar(&n_exp, 6, 7, SA_NTF_VALUE_DOUBLE, &oldvar5), SA_AIS_OK);

        } else {
            error = SA_AIS_ERR_FAILED_OPERATION;
            break;
        }
        rec_notif_data.populated = SA_FALSE;
        if (!compare_notifs(&n_exp, &rec_notif_data)) {
            print_notif(&n_exp);
            print_notif(&rec_notif_data);
            error = SA_AIS_ERR_FAILED_OPERATION;
        }
        safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
        safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
        noOfNotifs++;
    }
    safassert(unsub_notifications(), SA_AIS_OK);
    safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
    test_validate(error, SA_AIS_OK);
}

/**
 * Delete the object and verify correctness of generated notification.
 */
void objectDeleteTest_40(void)
{
	char command[1024];
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* delete object */
	sprintf(command, "immcfg -t 20 -d %s", DNTESTCFG);
	assert(system(command) != -1);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_DELETION, DNTESTCFG, 3, 3), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrAdminOwnerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmOiCcbIdT"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "ccbLast"), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 0, 0, "immcfg_xxx"), SA_AIS_OK);
		SaUint64T ccidVar = 3;
		safassert(set_attr_scalar(&n_exp, 1, 1, SA_NTF_VALUE_UINT64, &ccidVar), SA_AIS_OK);
		SaUint32T ccbLast = 1;
		safassert(set_attr_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &ccbLast), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Create a runtime test object and verify correctness of generated notification.
 */
void objectCreateTest_3401(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* create the runtime object */
	SaInt32T int32Var = INT32VAR1;
	SaUint32T uint32Var = UINT32VAR1;
	SaInt64T int64Var = INT64VAR1;
	SaUint64T uint64Var = UINT64VAR1;
	SaTimeT timeVar = TIMEVAR1;
	SaNameT nameVar = {.length = sizeof (NAME1)};
	memcpy(nameVar.value, NAME1, sizeof (NAME1));
	SaFloatT floatVar = FLOATVAR1;
	SaDoubleT doubleVar = DOUBLEVAR1;
	SaStringT stringVar = STRINGVAR1;
	SaAnyT anyVar = {.bufferSize = sizeof (BUF1), .bufferAddr = (SaUint8T*) BUF1};

	/*
	 * Create the object in IMM.
	 */
	create_rt_test_object("OsafNtfCmTestRT1", DNTESTRT1, int32Var, uint32Var, int64Var, uint64Var,
			&timeVar, &nameVar, floatVar, doubleVar, &stringVar, &anyVar);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_CREATION, DNTESTRT1, 12, 12), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testInt32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 4, 4, "testUint64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 5, 5, "testInt64"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 6, 6, "testString"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 7, 7, "testFloat"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 8, 8, "testDouble"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 9, 9, "testName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 10, 10, "testTime"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 11, 11, "testAny"), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 1, 1, "OsafNtfCmTestRT1"), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &uint32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 3, 3, SA_NTF_VALUE_INT32, &int32Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 4, 4, SA_NTF_VALUE_UINT64, &uint64Var), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 5, 5, SA_NTF_VALUE_INT64, &int64Var), SA_AIS_OK);
		safassert(set_attr_str(&n_exp, 6, 6, stringVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 7, 7, SA_NTF_VALUE_FLOAT, &floatVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 8, 8, SA_NTF_VALUE_DOUBLE, &doubleVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 9, 9, SA_NTF_VALUE_LDAP_NAME, &nameVar), SA_AIS_OK);
		safassert(set_attr_scalar(&n_exp, 10, 10, SA_NTF_VALUE_INT64, &timeVar), SA_AIS_OK);
		safassert(set_attr_buf(&n_exp, 11, 11, SA_NTF_VALUE_BINARY, &anyVar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify a runtime test object and verify correctness of generated notification.
 */
void objectModifyTest_3402(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	SaUint32T var1 = UINT32VAR2;
	SaFloatT var2 = FLOATVAR2;
	void* v1[] = {&var1};
	void* v2[] = {&var2};
	TestAttributeValue att1 = {
		.attrName = "testUint32", .attrType = SA_IMM_ATTR_SAUINT32T, .attrValues = v1
	};
	TestAttributeValue att2 = {
		.attrName = "testFloat", .attrType = SA_IMM_ATTR_SAFLOATT, .attrValues = v2
	};
	TestAttributeValue * attrs[] = {&att1, &att2, NULL};

	/*
	 * Modify (REPLACE) the object in IMM.
	 */
	modify_rt_test_object(DNTESTRT1, SA_IMM_ATTR_VALUES_REPLACE, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT1, 4, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testUint32"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 3, 3, "testFloat"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT1"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_UINT32, &var1), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 3, SA_NTF_VALUE_FLOAT, &var2), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Modify (multi-value, INT32) a runtime test object and verify correctness
 * of generated notification.
 */
void objectModifyTest_3403(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* modify the runtime object */

	SaInt32T oldvar = INT32VAR1;
	SaInt32T addvar = INT32VAR2;
	void* v[] = {&addvar};

	TestAttributeValue att = {
		.attrName = "testInt32", .attrType = SA_IMM_ATTR_SAINT32T, .attrValues = v
	};

	/*
	 * Modify (ADD) the object in IMM.
	 */
	TestAttributeValue * attrs[] = {&att, NULL};
	modify_rt_test_object(DNTESTRT1, SA_IMM_ATTR_VALUES_ADD, attrs);

	/*
	 * Wait for notification reception.
	 */
	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(100);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_ATTRIBUTE_CHANGED, DNTESTRT1, 3, 4), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 0, 0, "SaImmAttrImplementerName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 1, 1, "SaImmAttrClassName"), SA_AIS_OK);
		safassert(set_add_info(&n_exp, 2, 2, "testInt32"), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 0, 0, IMPLEMENTERNAME_RT), SA_AIS_OK);
		safassert(set_attr_change_str(&n_exp, 1, 1, "OsafNtfCmTestRT1"), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 2, 2, SA_NTF_VALUE_INT32, &oldvar), SA_AIS_OK);
		safassert(set_attr_change_scalar(&n_exp, 3, 2, SA_NTF_VALUE_INT32, &addvar), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}

/**
 * Delete a runtime test object and verify correctness of generated notification.
 */
void objectDeleteTest_3404(void)
{
	SaAisErrorT error = SA_AIS_OK;
	safassert(init_ntf(), SA_AIS_OK);
	safassert(subscribe_notifications(), SA_AIS_OK);
	rec_notif_data.populated = SA_FALSE;

	/* delete the runtime object */
	delete_rt_test_object(DNTESTRT1);

	int dwCnt = 0;
	while (dwCnt++ < POLLWAIT && !rec_notif_data.populated) {
		saNtfDispatch(ntfHandle, SA_DISPATCH_ALL);
		if (!rec_notif_data.populated) usleep(10000);
	}

	if (rec_notif_data.populated) {
		NotifData n_exp;
		safassert(set_ntf(&n_exp, SA_NTF_OBJECT_DELETION, DNTESTRT1, 0, 0), SA_AIS_OK);

		if (!compare_notifs(&n_exp, &rec_notif_data)) {
			print_notif(&n_exp);
			print_notif(&rec_notif_data);
			error = SA_AIS_ERR_FAILED_OPERATION;
		}
		safassert(saNtfNotificationFree(rec_notif_data.nHandle), SA_AIS_OK);
		safassert(saNtfNotificationFree(n_exp.nHandle), SA_AIS_OK);
	} else {
		error = SA_AIS_ERR_FAILED_OPERATION;
	}
	safassert(unsub_notifications(), SA_AIS_OK);
	safassert(saNtfFinalize(ntfHandle), SA_AIS_OK);
	test_validate(error, SA_AIS_OK);
}


__attribute__((constructor)) static void notificationFilterVerification_constructor(void)
{
	int rc = system("immcfg -f //hostfs//repl-opensaf//ntfsv_test_classes.xml");
	if (rc != 0) {
            rc = system("immcfg -f //hostfs//ntfsv_test_classes.xml");
            if (rc != 0) {
		printf("ntfsv_test_classes.xml file not installed (see README)");
		return;
            }
	}
	test_suite_add(32, "CM notifications test");
	test_case_add(32, objectCreateTest_01, "CREATE, runtime (OsafNtfCmTestRT) object");
	test_case_add(32, objectModifyTest_02, "runtime, attr ch, REPLACE (UINT32, FLOAT)");
	test_case_add(32, objectModifyTest_03, "runtime, attr ch, REPLACE (NAME, ANY)");
	test_case_add(32, objectModifyTest_04, "runtime, attr ch, ADD (INT32)");
	test_case_add(32, objectModifyTest_05, "runtime, attr ch, ADD (UINT32)");
	test_case_add(32, objectModifyTest_06, "runtime, attr ch, ADD (INT64)");
	test_case_add(32, objectModifyTest_07, "runtime, attr ch, ADD (UINT64)");
	test_case_add(32, objectModifyTest_08, "runtime, attr ch, ADD (FLOAT)");
	test_case_add(32, objectModifyTest_09, "runtime, attr ch, ADD (DOUBLE)");
	test_case_add(32, objectModifyTest_10, "runtime, attr ch, ADD (TIME)");
	test_case_add(32, objectModifyTest_11, "runtime, attr ch, ADD (STRING)");
	test_case_add(32, objectModifyTest_12, "runtime, attr ch, ADD (NAME)");
	test_case_add(32, objectModifyTest_13, "runtime, attr ch, ADD (ANY)");
	test_case_add(32, objectModifyTest_14, "runtime, attr ch, REPLACE (STRING, DOUBLE)");
	test_case_add(32, objectModifyTest_15, "runtime, attr ch, REPLACE (TIME)");
	test_case_add(32, objectModifyTest_16, "runtime, attr ch, ADD (STRING, INT32)");
	test_case_add(32, objectModifyTest_17, "runtime, attr ch, DELETE (STRING, INT32)");
	test_case_add(32, objectModifyTest_18, "runtime, attr ch, ADD (ALL TYPES)");
	test_case_add(32, objectDeleteTest_19, "DELETE, runtime (OsafNtfCmTestRT) object");
	test_case_add(32, objectCreateTest_20, "CREATE, config (OsafNtfCmTestCFG) object");
	test_case_add(32, objectModifyTest_21, "config, attr ch, REPLACE (UINT32, FLOAT)");
	test_case_add(32, objectModifyTest_22, "config, attr ch, REPLACE (NAME, ANY)");
	test_case_add(32, objectModifyTest_23, "config, attr ch, ADD (INT32)");
	test_case_add(32, objectModifyTest_24, "config, attr ch, ADD (UINT32)");
	test_case_add(32, objectModifyTest_25, "config, attr ch, ADD (INT64)");
	test_case_add(32, objectModifyTest_26, "config, attr ch, ADD (UINT64)");
	test_case_add(32, objectModifyTest_27, "config, attr ch, ADD (FLOAT)");
	test_case_add(32, objectModifyTest_28, "config, attr ch, ADD (DOUBLE)");
	test_case_add(32, objectModifyTest_29, "config, attr ch, ADD (TIME)");
	test_case_add(32, objectModifyTest_30, "config, attr ch, ADD (STRING)");
	test_case_add(32, objectModifyTest_31, "config, attr ch, ADD (NAME)");
	test_case_add(32, objectModifyTest_32, "config, attr ch, ADD (ANY)");
	test_case_add(32, objectModifyTest_33, "config, attr ch, REPLACE (STRING, DOUBLE)");
	test_case_add(32, objectModifyTest_34, "config, attr ch, REPLACE (TIME)");
	test_case_add(32, objectModifyTest_35, "config, attr ch, ADD (STRING, INT32)");
	test_case_add(32, objectModifyTest_36, "config, attr ch, DELETE (STRING, INT32)");
	test_case_add(32, objectModifyTest_37, "config, attr ch, ADD (ALL TYPES)");
	test_case_add(32, objectMultiCcbTest_38, "config, multiple op in ccb, 2 REPLACE");
	test_case_add(32, objectMultiCcbTest_39, "config, multiple op in ccb, ADD, REPLACE, DELETE");
	test_case_add(32, objectDeleteTest_40, "DELETE, config (OsafNtfCmTestCFG) object");
	test_suite_add(34, "CM notifications test, persistent runtime");
	test_case_add(34, objectCreateTest_3401, "CREATE, runtime (OsafNtfCmTestRT1) object");
	test_case_add(34, objectModifyTest_3402, "runtime, attr ch, REPLACE (UINT32, FLOAT)");
	test_case_add(34, objectModifyTest_3403, "runtime, attr ch, ADD (INT32)");
	test_case_add(34, objectDeleteTest_3404, "DELETE, runtime (OsafNtfCmTestRT1) object");
}

