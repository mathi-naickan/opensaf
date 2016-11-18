#include "plm/apitest/plmtest.h"
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"

void saPlmDispatch_01(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmDispatch(plmHandle, SA_DISPATCH_ALL);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmDispatch_02(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmDispatch(plmHandle, SA_DISPATCH_ONE);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmDispatch_03(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmDispatch(plmHandle, SA_DISPATCH_BLOCKING);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmDispatch_04(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmDispatch(-1, SA_DISPATCH_ONE);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmDispatch_05(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmDispatch(plmHandle, -1);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}
