/*	  -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
#include "osaf/apitest/utest.h"
#include "tet_ntf.h"
#include "tet_ntf_common.h"


SaAisErrorT ntftest_saNtfInitialize(SaNtfHandleT *ntfHandle, const SaNtfCallbacksT *ntfCallbacks, SaVersionT *version)
{
	int retryNum = 1;
	SaAisErrorT rc;
	do {
		fprintf_v(stdout, "\n Running saNtfInitialize()...");
		rc = saNtfInitialize(ntfHandle, ntfCallbacks, version);
		fprintf_v(stdout, "Done, rc:%u", rc);
	} while (--retryNum > 0 && rc != SA_AIS_OK);

	return rc;
}

SaAisErrorT ntftest_saNtfSelectionObjectGet(SaNtfHandleT ntfHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfSelectionObjectGet()...");
	rc = saNtfSelectionObjectGet(ntfHandle, selectionObject);
	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfDispatch(SaNtfHandleT ntfHandle, SaDispatchFlagsT dispatchFlags)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfDispatch()...");
	rc = saNtfDispatch(ntfHandle, dispatchFlags);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfFinalize(SaNtfHandleT ntfHandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfFinalize()...");
	rc = saNtfFinalize(ntfHandle);
	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}


SaAisErrorT ntftest_saNtfNotificationFree(SaNtfNotificationHandleT notificationHandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationFree()...");
	rc = saNtfNotificationFree(notificationHandle);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfNotificationSend(SaNtfNotificationHandleT notificationHandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationSend()...");
	rc = saNtfNotificationSend(notificationHandle);
	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfNotificationSubscribe(const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
				       SaNtfSubscriptionIdT subscriptionId)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationSubscribe()...");
	rc = saNtfNotificationSubscribe(notificationFilterHandles, subscriptionId);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfNotificationUnsubscribe(SaNtfSubscriptionIdT subscriptionId)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationUnsubscribe()...");

	rc = saNtfNotificationUnsubscribe(subscriptionId);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfNotificationFilterFree(SaNtfNotificationFilterHandleT notificationFilterHandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationFilterFree()...");
	rc = saNtfNotificationFilterFree(notificationFilterHandle);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfNotificationReadInitialize(SaNtfSearchCriteriaT searchCriteria,
					    const SaNtfNotificationTypeFilterHandlesT *notificationFilterHandles,
					    SaNtfReadHandleT *readHandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationReadInitialize()...");

	rc = saNtfNotificationReadInitialize(searchCriteria, notificationFilterHandles, readHandle);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}
SaAisErrorT ntftest_saNtfNotificationReadNext(SaNtfReadHandleT readHandle,
				      SaNtfSearchDirectionT searchDirection, SaNtfNotificationsT *notification)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationReadNext()...");

	rc = saNtfNotificationReadNext(readHandle, searchDirection, notification);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}
SaAisErrorT ntftest_saNtfNotificationReadFinalize(SaNtfReadHandleT readhandle)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfNotificationReadFinalize()...");

	rc = saNtfNotificationReadFinalize(readhandle);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}


SaAisErrorT ntftest_saNtfPtrValAllocate(SaNtfNotificationHandleT notificationHandle,
				SaUint16T dataSize, void **dataPtr, SaNtfValueT *value)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfPtrValAllocate()...");
	rc = saNtfPtrValAllocate(notificationHandle, dataSize,
								dataPtr, value);
	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfAlarmNotificationAllocate(SaNtfHandleT ntfHandle,
					   SaNtfAlarmNotificationT *notification,
					   SaUint16T numCorrelatedNotifications,
					   SaUint16T lengthAdditionalText,
					   SaUint16T numAdditionalInfo,
					   SaUint16T numSpecificProblems,
					   SaUint16T numMonitoredAttributes,
					   SaUint16T numProposedRepairActions,
					   SaInt16T variableDataSize)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfAlarmNotificationAllocate()...");
	rc = saNtfAlarmNotificationAllocate(ntfHandle,
					   notification,
					   numCorrelatedNotifications,
					   lengthAdditionalText,
					   numAdditionalInfo,
					   numSpecificProblems,
					   numMonitoredAttributes,
					   numProposedRepairActions,
					   variableDataSize);
	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfSecurityAlarmNotificationAllocate(SaNtfHandleT ntfHandle,
						   SaNtfSecurityAlarmNotificationT *notification,
						   SaUint16T numCorrelatedNotifications,
						   SaUint16T lengthAdditionalText,
						   SaUint16T numAdditionalInfo,
						   SaInt16T variableDataSize)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfSecurityAlarmNotificationAllocate()...");
	rc = saNtfSecurityAlarmNotificationAllocate(ntfHandle,
						   notification,
						   numCorrelatedNotifications,
						   lengthAdditionalText,
						   numAdditionalInfo,
						   variableDataSize);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfStateChangeNotificationAllocate(SaNtfHandleT ntfHandle,
				     SaNtfStateChangeNotificationT *notification,
				     SaUint16T numCorrelatedNotifications,
				     SaUint16T lengthAdditionalText,
				     SaUint16T numAdditionalInfo,
					 SaUint16T numStateChanges,
					 SaInt16T variableDataSize)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfStateChangeNotificationAllocate()...");
	rc = saNtfStateChangeNotificationAllocate(ntfHandle,
				     notification,
				     numCorrelatedNotifications,
				     lengthAdditionalText,
				     numAdditionalInfo,
					 numStateChanges,
					 variableDataSize);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfObjectCreateDeleteNotificationAllocate(SaNtfHandleT ntfHandle,
					    SaNtfObjectCreateDeleteNotificationT *notification,
					    SaUint16T numCorrelatedNotifications,
					    SaUint16T lengthAdditionalText,
					    SaUint16T numAdditionalInfo,
					    SaUint16T numAttributes,
						SaInt16T variableDataSize)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfObjectCreateDeleteNotificationAllocate()...");
	rc = saNtfObjectCreateDeleteNotificationAllocate(ntfHandle,
					    notification,
					    numCorrelatedNotifications,
					    lengthAdditionalText,
					    numAdditionalInfo,
					    numAttributes,
						variableDataSize);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfAttributeChangeNotificationAllocate(SaNtfHandleT ntfHandle,
					 SaNtfAttributeChangeNotificationT *notification,
					 SaUint16T numCorrelatedNotifications,
					 SaUint16T lengthAdditionalText,
					 SaUint16T numAdditionalInfo,
					 SaUint16T numAttributes,
					 SaInt16T variableDataSize)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfAttributeChangeNotificationAllocate()...");
	rc = saNtfAttributeChangeNotificationAllocate(ntfHandle,
					 notification,
					 numCorrelatedNotifications,
					 lengthAdditionalText,
					 numAdditionalInfo,
					 numAttributes,
					 variableDataSize);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						 SaNtfAlarmNotificationFilterT *notificationFilter,
						 SaUint16T numEventTypes,
						 SaUint16T numNotificationObjects,
						 SaUint16T numNotifyingObjects,
						 SaUint16T numNotificationClassIds,
						 SaUint16T numProbableCauses,
						 SaUint16T numPerceivedSeverities,
						 SaUint16T numTrends)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfAlarmNotificationFilterAllocate()...");
	rc = saNtfAlarmNotificationFilterAllocate(ntfHandle,
						 notificationFilter,
						 numEventTypes,
						 numNotificationObjects,
						 numNotifyingObjects,
						 numNotificationClassIds,
						 numProbableCauses,
						 numPerceivedSeverities,
						 numTrends);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfSecurityAlarmNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							 SaNtfSecurityAlarmNotificationFilterT *notificationFilter,
							 SaUint16T numEventTypes,
							 SaUint16T numNotificationObjects,
							 SaUint16T numNotifyingObjects,
							 SaUint16T numNotificationClassIds,
							 SaUint16T numProbableCauses,
							 SaUint16T numSeverities,
							 SaUint16T numSecurityAlarmDetectors,
							 SaUint16T numServiceUsers,
							 SaUint16T numServiceProviders)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfSecurityAlarmNotificationFilterAllocate()...");
	rc = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
							 notificationFilter,
							 numEventTypes,
							 numNotificationObjects,
							 numNotifyingObjects,
							 numNotificationClassIds,
							 numProbableCauses,
							 numSeverities,
							 numSecurityAlarmDetectors,
							 numServiceUsers,
							 numServiceProviders);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfStateChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
						       SaNtfStateChangeNotificationFilterT *notificationFilter,
						       SaUint16T numEventTypes,
						       SaUint16T numNotificationObjects,
						       SaUint16T numNotifyingObjects,
						       SaUint16T numNotificationClassIds,
						       SaUint16T numSourceIndicators,
							   SaUint16T numChangedStates)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfStateChangeNotificationFilterAllocate()...");
	rc = saNtfStateChangeNotificationFilterAllocate(ntfHandle,
						       notificationFilter,
						       numEventTypes,
						       numNotificationObjects,
						       numNotifyingObjects,
						       numNotificationClassIds,
						       numSourceIndicators,
							   numChangedStates);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfObjectCreateDeleteNotificationFilterAllocate(SaNtfHandleT ntfHandle,
								SaNtfObjectCreateDeleteNotificationFilterT *notificationFilter,
								SaUint16T numEventTypes,
							    SaUint16T numNotificationObjects,
							    SaUint16T numNotifyingObjects,
							    SaUint16T numNotificationClassIds,
							    SaUint16T numSourceIndicators)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfObjectCreateDeleteNotificationFilterAllocate()...");
	rc = saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle,
								notificationFilter,
								numEventTypes,
							    numNotificationObjects,
							    numNotifyingObjects,
							    numNotificationClassIds,
							    numSourceIndicators);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}

SaAisErrorT ntftest_saNtfAttributeChangeNotificationFilterAllocate(SaNtfHandleT ntfHandle,
							   SaNtfAttributeChangeNotificationFilterT *notificationFilter,
							   SaUint16T numEventTypes,
							   SaUint16T numNotificationObjects,
							   SaUint16T numNotifyingObjects,
							   SaUint16T numNotificationClassIds,
							   SaUint16T numSourceIndicators)
{
	SaAisErrorT rc;

	fprintf_v(stdout, "\n Running saNtfAttributeChangeNotificationFilterAllocate()...");
	rc = saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
							   notificationFilter,
							   numEventTypes,
							   numNotificationObjects,
							   numNotifyingObjects,
							   numNotificationClassIds,
							   numSourceIndicators);

	fprintf_v(stdout, "Done, rc:%u", rc);

	return rc;
}
