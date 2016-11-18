/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

/** @file
 *
 * This file contains references to otherwise unused AIS functions. The purpose
 * of this file is make sure the 'cppcheck' tool does not flag these AIS
 * functions as unused. This file is not meant to ever be compiled - it should
 * only be used when analyzing the OpenSAF source code with the 'cppcheck' tool.
 */

#include "saAmf.h"
#include "saCkpt.h"
#include "saClm.h"
#include "saEvt.h"
#include "saImmOi_A_2_15.h"
#include "saImmOm_A_2_17.h"
#include "saLck.h"
#include "saLog.h"
#include "saMsg.h"
#include "saNtf.h"
#include "saPlm.h"
#include "saSmf.h"

void* unused_functions[] = {
  reintrepret_cast<void*>(saAmfComponentErrorClear_4),
  reintrepret_cast<void*>(saAmfComponentErrorReport_4),
  reintrepret_cast<void*>(saAmfCorrelationIdsGet),
  reintrepret_cast<void*>(saAmfHAReadinessStateSet),
  reintrepret_cast<void*>(saAmfHAStateGet),
  reintrepret_cast<void*>(saAmfHealthcheckConfirm),
  reintrepret_cast<void*>(saAmfHealthcheckStop),
  reintrepret_cast<void*>(saAmfProtectionGroupNotificationFree_4),
  reintrepret_cast<void*>(saAmfProtectionGroupTrack),
  reintrepret_cast<void*>(saAmfProtectionGroupTrackStop),
  reintrepret_cast<void*>(saAmfProtectionGroupTrack_4),
  reintrepret_cast<void*>(saAmfResponse_4),
  reintrepret_cast<void*>(saCkptActiveReplicaSet),
  reintrepret_cast<void*>(saCkptCheckpointOpen),
  reintrepret_cast<void*>(saCkptCheckpointOpenAsync),
  reintrepret_cast<void*>(saCkptCheckpointRead),
  reintrepret_cast<void*>(saCkptCheckpointRetentionDurationSet),
  reintrepret_cast<void*>(saCkptCheckpointStatusGet),
  reintrepret_cast<void*>(saCkptCheckpointSynchronize),
  reintrepret_cast<void*>(saCkptCheckpointSynchronizeAsync),
  reintrepret_cast<void*>(saCkptCheckpointTrack),
  reintrepret_cast<void*>(saCkptCheckpointTrackStop),
  reintrepret_cast<void*>(saCkptCheckpointUnlink),
  reintrepret_cast<void*>(saCkptCheckpointWrite),
  reintrepret_cast<void*>(saCkptDispatch),
  reintrepret_cast<void*>(saCkptFinalize),
  reintrepret_cast<void*>(saCkptIOVectorElementDataFree),
  reintrepret_cast<void*>(saCkptInitialize),
  reintrepret_cast<void*>(saCkptInitialize_2),
  reintrepret_cast<void*>(saCkptSectionCreate),
  reintrepret_cast<void*>(saCkptSectionDelete),
  reintrepret_cast<void*>(saCkptSectionExpirationTimeSet),
  reintrepret_cast<void*>(saCkptSectionIdFree),
  reintrepret_cast<void*>(saCkptSectionIterationFinalize),
  reintrepret_cast<void*>(saCkptSectionIterationInitialize),
  reintrepret_cast<void*>(saCkptSectionIterationNext),
  reintrepret_cast<void*>(saCkptSectionOverwrite),
  reintrepret_cast<void*>(saCkptSelectionObjectGet),
  reintrepret_cast<void*>(saCkptTrack),
  reintrepret_cast<void*>(saCkptTrackStop),
  reintrepret_cast<void*>(saClmClusterNodeGetAsync),
  reintrepret_cast<void*>(saClmClusterNodeGet_4),
  reintrepret_cast<void*>(saClmClusterNotificationFree_4),
  reintrepret_cast<void*>(saEvtChannelClose),
  reintrepret_cast<void*>(saEvtChannelOpen),
  reintrepret_cast<void*>(saEvtChannelOpenAsync),
  reintrepret_cast<void*>(saEvtChannelUnlink),
  reintrepret_cast<void*>(saEvtDispatch),
  reintrepret_cast<void*>(saEvtEventAllocate),
  reintrepret_cast<void*>(saEvtEventAttributesGet),
  reintrepret_cast<void*>(saEvtEventAttributesSet),
  reintrepret_cast<void*>(saEvtEventDataGet),
  reintrepret_cast<void*>(saEvtEventFree),
  reintrepret_cast<void*>(saEvtEventPatternFree),
  reintrepret_cast<void*>(saEvtEventPublish),
  reintrepret_cast<void*>(saEvtEventRetentionTimeClear),
  reintrepret_cast<void*>(saEvtEventSubscribe),
  reintrepret_cast<void*>(saEvtEventUnsubscribe),
  reintrepret_cast<void*>(saEvtFinalize),
  reintrepret_cast<void*>(saEvtInitialize),
  reintrepret_cast<void*>(saEvtLimitGet),
  reintrepret_cast<void*>(saEvtSelectionObjectGet),
  reintrepret_cast<void*>(saImmOiAugmentCcbInitialize),
  reintrepret_cast<void*>(saImmOiInitialize_o3),
  reintrepret_cast<void*>(saImmOiObjectImplementerRelease),
  reintrepret_cast<void*>(saImmOiObjectImplementerRelease_o3),
  reintrepret_cast<void*>(saImmOiObjectImplementerSet),
  reintrepret_cast<void*>(saImmOiObjectImplementerSet_o3),
  reintrepret_cast<void*>(saImmOiRtObjectCreate_o3),
  reintrepret_cast<void*>(saImmOiRtObjectDelete_o3),
  reintrepret_cast<void*>(saImmOiRtObjectUpdate_o3),
  reintrepret_cast<void*>(saImmOmAccessorGet_o3),
  reintrepret_cast<void*>(saImmOmAdminOperationContinuationClear),
  reintrepret_cast<void*>(saImmOmAdminOperationContinuationClear_o3),
  reintrepret_cast<void*>(saImmOmAdminOperationContinue),
  reintrepret_cast<void*>(saImmOmAdminOperationContinueAsync),
  reintrepret_cast<void*>(saImmOmAdminOperationContinueAsync_o3),
  reintrepret_cast<void*>(saImmOmAdminOperationContinue_o2),
  reintrepret_cast<void*>(saImmOmAdminOperationContinue_o3),
  reintrepret_cast<void*>(saImmOmAdminOperationInvokeAsync_2),
  reintrepret_cast<void*>(saImmOmAdminOperationInvokeAsync_o3),
  reintrepret_cast<void*>(saImmOmAdminOwnerClear_o3),
  reintrepret_cast<void*>(saImmOmAdminOwnerRelease_o3),
  reintrepret_cast<void*>(saImmOmAdminOwnerSet_o3),
  reintrepret_cast<void*>(saImmOmCcbObjectCreate_o3),
  reintrepret_cast<void*>(saImmOmCcbObjectDelete_o3),
  reintrepret_cast<void*>(saImmOmCcbObjectModify_o3),
  reintrepret_cast<void*>(saImmOmSearchInitialize_o3),
  reintrepret_cast<void*>(saImmOmSearchNext_o3),
  reintrepret_cast<void*>(saLckDispatch),
  reintrepret_cast<void*>(saLckFinalize),
  reintrepret_cast<void*>(saLckInitialize),
  reintrepret_cast<void*>(saLckLockPurge),
  reintrepret_cast<void*>(saLckOptionCheck),
  reintrepret_cast<void*>(saLckResourceClose),
  reintrepret_cast<void*>(saLckResourceLock),
  reintrepret_cast<void*>(saLckResourceLockAsync),
  reintrepret_cast<void*>(saLckResourceOpen),
  reintrepret_cast<void*>(saLckResourceOpenAsync),
  reintrepret_cast<void*>(saLckResourceUnlock),
  reintrepret_cast<void*>(saLckResourceUnlockAsync),
  reintrepret_cast<void*>(saLckSelectionObjectGet),
  reintrepret_cast<void*>(saLogFilterSetCallback),
  reintrepret_cast<void*>(saLogLimitGet),
  reintrepret_cast<void*>(saLogStreamOpenAsync_2),
  reintrepret_cast<void*>(saLogStreamOpenCallback),
  reintrepret_cast<void*>(saLogWriteLog),
  reintrepret_cast<void*>(saMsgDispatch),
  reintrepret_cast<void*>(saMsgFinalize),
  reintrepret_cast<void*>(saMsgInitialize),
  reintrepret_cast<void*>(saMsgMessageCancel),
  reintrepret_cast<void*>(saMsgMessageDataFree),
  reintrepret_cast<void*>(saMsgMessageGet),
  reintrepret_cast<void*>(saMsgMessageReply),
  reintrepret_cast<void*>(saMsgMessageReplyAsync),
  reintrepret_cast<void*>(saMsgMessageSend),
  reintrepret_cast<void*>(saMsgMessageSendAsync),
  reintrepret_cast<void*>(saMsgMessageSendReceive),
  reintrepret_cast<void*>(saMsgQueueGroupCreate),
  reintrepret_cast<void*>(saMsgQueueGroupDelete),
  reintrepret_cast<void*>(saMsgQueueGroupInsert),
  reintrepret_cast<void*>(saMsgQueueGroupNotificationFree),
  reintrepret_cast<void*>(saMsgQueueGroupRemove),
  reintrepret_cast<void*>(saMsgQueueGroupTrack),
  reintrepret_cast<void*>(saMsgQueueGroupTrackStop),
  reintrepret_cast<void*>(saMsgQueueOpen),
  reintrepret_cast<void*>(saMsgQueueOpenAsync),
  reintrepret_cast<void*>(saMsgQueueRetentionTimeSet),
  reintrepret_cast<void*>(saMsgQueueStatusGet),
  reintrepret_cast<void*>(saMsgQueueUnlink),
  reintrepret_cast<void*>(saMsgSelectionObjectGet),
  reintrepret_cast<void*>(saNtfArrayValAllocate),
  reintrepret_cast<void*>(saNtfArrayValGet),
  reintrepret_cast<void*>(saNtfLocalizedMessageFree),
  reintrepret_cast<void*>(saNtfLocalizedMessageGet),
  reintrepret_cast<void*>(saPlmEntityGroupDelete),
  reintrepret_cast<void*>(saPlmEntityReadinessImpact),
  reintrepret_cast<void*>(saPlmFinalize),
  reintrepret_cast<void*>(saPlmReadinessNotificationFree),
  reintrepret_cast<void*>(saPlmReadinessTrackStop),
  reintrepret_cast<void*>(saSmfCallbackScopeUnregister)
};

int main(int argc, char** argv) {
  (void) argv;
  if (argc < sizeof(unused_functions) / sizeof(unused_functions[0])) {
    return reinterpret_cast<int>(unused_functions[argc]);
  } else {
    return 0;
  }
}
