#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "assert.h"
#include "plm/apitest/plmtest.h"
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"


void TrackCallbackT(
            SaPlmEntityGroupHandleT entityGroupHandle,
            SaUint64T trackCookie,
            SaInvocationT invocation,
            SaPlmTrackCauseT cause,
            const SaNameT *rootCauseEntity,
            SaNtfIdentifierT rootCorrelationId,
            const SaPlmReadinessTrackedEntitiesT *trackedEntities,
            SaPlmChangeStepT step,
            SaAisErrorT error) {
}

void saPlmInitialize_01(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    rc = saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion);
    test_validate(rc, SA_AIS_OK);
    if (rc == SA_AIS_OK)
   safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
/*	sleep(100000000);*/
}



void saPlmInitialize_02(void)
{
    rc = saPlmInitialize(&plmHandle, NULL, &PlmVersion);
    test_validate(rc, SA_AIS_OK);
    if (rc == SA_AIS_OK)
    	safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    
}

void saPlmInitialize_03(void)
{
   SaPlmCallbacksT plms_cbks; 
   plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
   rc =  saPlmInitialize(0 , &plms_cbks, &PlmVersion);
   test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
}

void saPlmInitialize_04(void)
{
   SaPlmCallbacksT plms_cbks; 
   SaVersionT PlmVersion = {0, 0, 0};
   plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
   rc = saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion); 
   test_validate(rc, SA_AIS_ERR_VERSION);
}


void saPlmInitialize_05(void)
{
   	SaPlmCallbacksT plms_cbks; 
	SaVersionT PlmVersion = {'B', 1, 1};
   	plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
        rc = saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion); 
        test_validate(rc, SA_AIS_ERR_VERSION);
}


void saPlmInitialize_06(void)
{
   	SaPlmCallbacksT plms_cbks; 
	SaVersionT PlmVersion = {'A', 1, 1};
   	plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
        rc = saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion); 
        test_validate(rc, SA_AIS_OK);
}
extern void saPlmSelectionObjectGet_01(void);
extern void saPlmSelectionObjectGet_02(void);
extern void saPlmSelectionObjectGet_03(void);
extern void saPlmDispatch_01(void);
extern void saPlmDispatch_02(void);
extern void saPlmDispatch_03(void);
extern void saPlmDispatch_04(void);
extern void saPlmDispatch_05(void);
extern void saPlmFinalize_01(void);
extern void saPlmFinalize_02(void);

__attribute__ ((constructor)) static void saPlmInitialize_constructor(void)
{
    test_suite_add(1, "Library Life Cycle");
    test_case_add(1, saPlmInitialize_01, "saPlmInitialize - SA_AIS_OK");
    test_case_add(1, saPlmInitialize_02 , "SA_AIS_OK - NULL pointer to callbacks");
    test_case_add(1, saPlmInitialize_03 , "SA_AIS_ERR_INVALID_PARAM - uninitialized plmHandle");
    test_case_add(1, saPlmInitialize_04 , "SA_AIS_ERR_VERSION - uninitialized version");
    test_case_add(1,saPlmInitialize_05 , "SA_AIS_ERR_VERSION - too high release level");
    test_case_add(1,saPlmInitialize_06 , "SA_AIS_OK - minor version set to 1");    
    test_case_add(1,saPlmSelectionObjectGet_01 , "SA_AIS_INVALID_PARAM - null pointer to callback");
    test_case_add(1,saPlmSelectionObjectGet_02 , "SA_AIS_ERR_BAD_HANDLE - bad plmHandle");
    test_case_add(1,saPlmSelectionObjectGet_03 , "SA_AIS_INVALID_PARAM - null selection object");
    test_case_add(1,saPlmDispatch_01, "SA_AIS_OK with SA_DISPATCH_ALL");      
    test_case_add(1,saPlmDispatch_02, "SA_AIS_OK with SA_DISPATCH_ONE");      
    //test_case_add(1,saPlmDispatch_03, "SA_AIS_OK with SA_DISPATCH_BLOCKING");      
    test_case_add(1,saPlmDispatch_04, "SA_AIS_ERR_BAD_HANDLE - bad plmHandle");      
    test_case_add(1,saPlmDispatch_05, "SA_AIS_ERR_INVALID_PARAM - Invalid dispatch option ");      
    test_case_add(1,saPlmFinalize_01, "Finalize with SA_AIS_OK");
    test_case_add(1,saPlmFinalize_02 , "SA_AIS_ERR_BAD_HANDLE - bad handle"); 
}

