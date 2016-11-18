#include "plm/apitest/plmtest.h"
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"


void saPlmEntityGroupAdd_01(void)
{
    safassert(saPlmInitialize(&plmHandle, NULL, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_02(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_03(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupAdd_04(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_EES_ONLY);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupAdd_05(void)
{
    rc=saPlmEntityGroupAdd((SaPlmEntityGroupHandleT)&entityGroupHandle, &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
}


void saPlmEntityGroupAdd_06(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc = saPlmEntityGroupAdd(entityGroupHandle , 0, entityNamesNumber,SA_PLM_GROUP_SUBTREE_EES_ONLY);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupAdd_07(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , 0, entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupAdd_08(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn  , 0 ,SA_PLM_GROUP_SUBTREE_EES_ONLY);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_09(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn  , 0  ,SA_PLM_GROUP_SUBTREE_HES_ONLY);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupAdd_10(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,-1);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_11(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(-1 , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupAdd_12(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY);
    test_validate(rc, SA_AIS_ERR_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_13(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    /* Nonexistent DN*/
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_nonexistent , entityNamesNumber,SA_PLM_GROUP_SINGLE_ENTITY);
    test_validate(rc, SA_AIS_ERR_NOT_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupAdd_14(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_eedn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_HES_ONLY);
    test_validate(rc, SA_AIS_OK);
    
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupAdd_15(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE_EES_ONLY);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmEntityGroupAdd_16(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &f120_slot_1_dn , entityNamesNumber,SA_PLM_GROUP_SUBTREE);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &amc_slot_1_dn  , entityNamesNumber,SA_PLM_GROUP_SUBTREE);
    test_validate(rc, SA_AIS_ERR_EXIST);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

