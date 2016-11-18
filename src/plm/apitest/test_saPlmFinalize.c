#include "plm/apitest/plmtest.h"
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"

void saPlmFinalize_01(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmFinalize(plmHandle);
    test_validate(rc, SA_AIS_OK);
}

void saPlmFinalize_02(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmFinalize(-1);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}
