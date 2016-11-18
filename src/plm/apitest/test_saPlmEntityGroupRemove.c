#include "plm/apitest/plmtest.h"

void saPlmEntityGroupRemove_01(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_02(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupRemove_03(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_04(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_05(void)
{
    rc=saPlmEntityGroupRemove(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}


void saPlmEntityGroupRemove_06(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}


void saPlmEntityGroupRemove_07(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd( entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(0, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}



void saPlmEntityGroupRemove_08(void)
{
    SaPlmCallbacksT plms_cbks;
    SaNameT zerosize_dn = {strlen(""), ""};
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &zerosize_dn  , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_10(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , 0);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_11(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_12(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_nonexistent , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_13(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE),SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_eedn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_14(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE),SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &amc_slot_1_dn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_15(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_EES_ONLY),SA_AIS_OK);
    rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_eedn , entityNamesNumber);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupRemove_16(void)
{
   SaPlmCallbacksT plms_cbks;
   plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
   safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
   safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
   safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY),SA_AIS_OK);
   rc=saPlmEntityGroupRemove(entityGroupHandle, &amc_slot_1_dn , entityNamesNumber);
   test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
   safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_17(void)
{
   SaPlmCallbacksT plms_cbks;
   plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
   safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
   safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
   safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY),SA_AIS_OK);
   rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_eedn , entityNamesNumber);
   test_validate(rc, SA_AIS_ERR_NOT_EXIST);
   safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupRemove_18(void)
{ 
   SaPlmCallbacksT plms_cbks;
   plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
   safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
   safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
   safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
   rc=saPlmEntityGroupRemove(entityGroupHandle, &f120_slot_1_eedn , entityNamesNumber);
   test_validate(rc, SA_AIS_ERR_NOT_EXIST);
   safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}
