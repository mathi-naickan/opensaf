/*
  Header file of SA Forum AIS LCK APIs (SAI-AIS-LCK-B.01.00.03)
  compiled on 26AUG2004 by sayandeb.saha@motorola.com.
*/

#ifndef _SA_LCK_H
#define _SA_LCK_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

	typedef SaUint64T SaLckHandleT;
	typedef SaUint64T SaLckLockIdT;
	typedef SaUint64T SaLckResourceHandleT;
	typedef SaUint64T SaLckWaiterSignalT;

#define SA_LCK_RESOURCE_CREATE 0x1
	typedef SaUint32T SaLckResourceOpenFlagsT;

#define SA_LCK_LOCK_NO_QUEUE 0x1
#define SA_LCK_LOCK_ORPHAN   0x2
	typedef SaUint32T SaLckLockFlagsT;

#define SA_LCK_OPT_ORPHAN_LOCKS      0x1
#define SA_LCK_OPT_DEADLOCK_DETECTION 0x2
	typedef SaUint32T SaLckOptionsT;

	typedef enum {
		SA_LCK_LOCK_GRANTED = 1,
		SA_LCK_LOCK_DEADLOCK = 2,
		SA_LCK_LOCK_NOT_QUEUED = 3,
		SA_LCK_LOCK_ORPHANED = 4,
		SA_LCK_LOCK_NO_MORE = 5,
		SA_LCK_LOCK_DUPLICATE_EX = 6
	} SaLckLockStatusT;

	typedef enum {
		SA_LCK_PR_LOCK_MODE = 1,
		SA_LCK_EX_LOCK_MODE = 2
	} SaLckLockModeT;

	typedef void
	 (*SaLckResourceOpenCallbackT) (SaInvocationT invocation,
					SaLckResourceHandleT lockResourceHandle, SaAisErrorT error);
	typedef void
	 (*SaLckLockGrantCallbackT) (SaInvocationT invocation, SaLckLockStatusT lockStatus, SaAisErrorT error);
	typedef void
	 (*SaLckLockWaiterCallbackT) (SaLckWaiterSignalT waiterSignal,
				      SaLckLockIdT lockId, SaLckLockModeT modeHeld, SaLckLockModeT modeRequested);
	typedef void
	 (*SaLckResourceUnlockCallbackT) (SaInvocationT invocation, SaAisErrorT error);

	typedef struct {
		SaLckResourceOpenCallbackT saLckResourceOpenCallback;
		SaLckLockGrantCallbackT saLckLockGrantCallback;
		SaLckLockWaiterCallbackT saLckLockWaiterCallback;
		SaLckResourceUnlockCallbackT saLckResourceUnlockCallback;
	} SaLckCallbacksT;

	extern SaAisErrorT
	 saLckInitialize(SaLckHandleT *lckHandle, const SaLckCallbacksT *lckCallbacks, SaVersionT *version);
	extern SaAisErrorT
	 saLckSelectionObjectGet(SaLckHandleT lckHandle, SaSelectionObjectT *selectionObject);
	extern SaAisErrorT
	 saLckOptionCheck(SaLckHandleT lckHandle, SaLckOptionsT *lckOptions);
	extern SaAisErrorT
	 saLckDispatch(SaLckHandleT lckHandle, const SaDispatchFlagsT dispatchFlags);
	extern SaAisErrorT
	 saLckFinalize(SaLckHandleT lckHandle);
	extern SaAisErrorT
	 saLckResourceOpen(SaLckHandleT lckHandle,
			   const SaNameT *lockResourceName,
			   SaLckResourceOpenFlagsT resourceFlags,
			   SaTimeT timeout, SaLckResourceHandleT *lockResourceHandle);
	extern SaAisErrorT
	 saLckResourceOpenAsync(SaLckHandleT lckHandle,
				SaInvocationT invocation,
				const SaNameT *lockResourceName, SaLckResourceOpenFlagsT resourceFlags);
	extern SaAisErrorT
	 saLckResourceClose(SaLckResourceHandleT lockResourceHandle);
	extern SaAisErrorT
	 saLckResourceLock(SaLckResourceHandleT lockResourceHandle,
			   SaLckLockIdT *lockId,
			   SaLckLockModeT lockMode,
			   SaLckLockFlagsT lockFlags,
			   SaLckWaiterSignalT waiterSignal, SaTimeT timeout, SaLckLockStatusT *lockStatus);
	extern SaAisErrorT
	 saLckResourceLockAsync(SaLckResourceHandleT lockResourceHandle,
				SaInvocationT invocation,
				SaLckLockIdT *lockId,
				SaLckLockModeT lockMode, SaLckLockFlagsT lockFlags, SaLckWaiterSignalT waiterSignal);
	extern SaAisErrorT
	 saLckResourceUnlock(SaLckLockIdT lockId, SaTimeT timeout);
	extern SaAisErrorT
	 saLckResourceUnlockAsync(SaInvocationT invocation, SaLckLockIdT lockId);
	extern SaAisErrorT
	 saLckLockPurge(SaLckResourceHandleT lockResourceHandle);

#ifdef  __cplusplus
}
#endif

#endif   /* _SA_LCK_H */
