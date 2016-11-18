#include "plm/apitest/plmtest.h"

void saPlmEntityGroupDelete_01(void)
{
     SaPlmCallbacksT plms_cbks;
     plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;

    safassert(saPlmInitialize(&plmHandle, &plms_cbks , &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber),SA_AIS_OK);
    rc=saPlmEntityGroupDelete(entityGroupHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupDelete_02(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc=saPlmEntityGroupDelete(entityGroupHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupDelete_03(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks , &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    rc=saPlmEntityGroupDelete(entityGroupHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}


