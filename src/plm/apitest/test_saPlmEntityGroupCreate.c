#include "plm/apitest/plmtest.h"


void saPlmEntityGroupCreate_01(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmEntityGroupCreate(plmHandle,&entityGroupHandle);
    rc = saPlmEntityGroupCreate(plmHandle,&entityGroupHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupCreate_02(void)
{
    safassert(saPlmInitialize(&plmHandle, NULL , &PlmVersion), SA_AIS_OK);
    rc = saPlmEntityGroupCreate(plmHandle,&entityGroupHandle);
    test_validate(rc, SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmEntityGroupCreate_03(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmEntityGroupCreate( -1 ,&entityGroupHandle);
    test_validate(rc, SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}



void saPlmEntityGroupCreate_04(void)
{
    SaPlmCallbacksT plms_cbks; 
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallbackT;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    rc = saPlmEntityGroupCreate(plmHandle,NULL);
    test_validate(rc, SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

extern void saPlmEntityGroupAdd_01(void);
extern void saPlmEntityGroupAdd_02(void);
extern void saPlmEntityGroupAdd_03(void);
extern void saPlmEntityGroupAdd_04(void);
extern void saPlmEntityGroupAdd_05(void);
extern void saPlmEntityGroupAdd_06(void);
extern void saPlmEntityGroupAdd_07(void);
extern void saPlmEntityGroupAdd_08(void);
extern void saPlmEntityGroupAdd_09(void);
extern void saPlmEntityGroupAdd_10(void);
extern void saPlmEntityGroupAdd_11(void);
extern void saPlmEntityGroupAdd_12(void);
extern void saPlmEntityGroupAdd_13(void);
extern void saPlmEntityGroupAdd_14(void);
extern void saPlmEntityGroupAdd_15(void);
extern void saPlmEntityGroupAdd_16(void);
extern void saPlmEntityGroupDelete_01(void);
extern void saPlmEntityGroupDelete_02(void);
extern void saPlmEntityGroupDelete_03(void);
void saPlmEntityGroupRemove_01(void);
void saPlmEntityGroupRemove_02(void);
void saPlmEntityGroupRemove_03(void);
void saPlmEntityGroupRemove_04(void);
void saPlmEntityGroupRemove_05(void);
void saPlmEntityGroupRemove_06(void);
void saPlmEntityGroupRemove_07(void);
void saPlmEntityGroupRemove_08(void);
void saPlmEntityGroupRemove_10(void);
void saPlmEntityGroupRemove_11(void);
void saPlmEntityGroupRemove_12(void);
void saPlmEntityGroupRemove_13(void);
void saPlmEntityGroupRemove_14(void);
void saPlmEntityGroupRemove_15(void);
void saPlmEntityGroupRemove_16(void);
void saPlmEntityGroupRemove_17(void);
void saPlmEntityGroupRemove_18(void);

__attribute__ ((constructor)) static void saPlmEntityGroupCreate_constructor(void)
{
   test_suite_add(2, "Entity group create");
   test_case_add(2, saPlmEntityGroupCreate_01, "saPlmEntityGroupCreate - SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupCreate_02, "saPlmEntityGroupCreate with NULL callbacks - SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupCreate_03, "saPlmEntityGroupCreate with bad plmhandle  - SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2, saPlmEntityGroupCreate_04, "saPlmEntityGroupCreate with invalid param -SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2,saPlmEntityGroupAdd_01 , "saPlmEntityGroupAdd_01 with SA_PLM_GROUP_SINGLE_ENTITY option - SA_AIS_OK");
   test_case_add(2,saPlmEntityGroupAdd_02 , "saPlmEntityGroupAdd_02 with SA_PLM_GROUP_SUBTREE option - SA_AIS_OK");
   test_case_add(2,saPlmEntityGroupAdd_03 , "saPlmEntityGroupAdd_03 with SA_PLM_GROUP_SUBTREE_HES_ONLY option - SA_AIS_OK");
   test_case_add(2,saPlmEntityGroupAdd_04 , "saPlmEntityGroupAdd_04 with SA_PLM_GROUP_SUBTREE_EES_ONLY option - SA_AIS_OK");
//   test_case_add(2,saPlmEntityGroupAdd_05 , "saPlmEntityGroupAdd_05 with bad plmhandle SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2,saPlmEntityGroupAdd_06 , "saPlmEntityGroupAdd_06 with invalid DN SA_AIS_ERR_INVALID_PARAM ");
   test_case_add(2,saPlmEntityGroupAdd_07 , "saPlmEntityGroupAdd_07 with invalid DN SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2,saPlmEntityGroupAdd_08 , "saPlmEntityGroupAdd_08 with invalid entityNamesNumber SA_AIS_ERR_INVALID_PARAM");
   //test_case_add(2,saPlmEntityGroupAdd_09 , "saPlmEntityGroupAdd_09 with invalid entityNamesNumber SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2,saPlmEntityGroupAdd_10 , "saPlmEntityGroupAdd_10 with invalid group add option SA_AIS_ERR_INVALID_PARAM ");
   test_case_add(2,saPlmEntityGroupAdd_11 , "saPlmEntityGroupAdd_11 with invalid plmhandle SA_AIS_ERR_BAD_HANDLE ");
   test_case_add(2,saPlmEntityGroupAdd_12 , "saPlmEntityGroupAdd_12 with already added DN SA_AIS_ERR_EXIST ");
   test_case_add(2,saPlmEntityGroupAdd_13 , "saPlmEntityGroupAdd_13 - with nonexistent DN SA_AIS_ERR_NOT_EXIST");
   test_case_add(2,saPlmEntityGroupAdd_14 , "saPlmEntityGroupAdd_14 - Added EE in a group with SA_PLM_GROUP_SUBTREE_HES_ONLY SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2,saPlmEntityGroupAdd_15 , "saPlmEntityGroupAdd_15 - Added HE in a group with SA_PLM_GROUP_SUBTREE_EES_ONLY SA_AIS_OK");
   test_case_add(2,saPlmEntityGroupAdd_16 , "saPlmEntityGroupAdd_16 - Added HE after adding the parent with SA_PLM_GROUP_SUBTREE_HES_ONLY option SA_AIS_ERR_EXIST");
   test_case_add(2, saPlmEntityGroupRemove_01, "saPlmEntityGroupRemove_1 with SA_PLM_GROUP_SINGLE_ENTITY option - SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupRemove_02, "saPlmEntityGroupRemove_2 with SA_PLM_GROUP_SUBTREE option - SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupRemove_03, "saPlmEntityGroupRemove_3 with SA_PLM_GROUP_SUBTREE_HES_ONLY option -SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupRemove_04, "saPlmEntityGroupRemove_4 with SA_PLM_GROUP_SUBTREE_EES_ONLY option -SA_AIS_OK");
   test_case_add(2, saPlmEntityGroupRemove_05, "saPlmEntityGroupRemove_5 with bad plmhandle SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2, saPlmEntityGroupRemove_06, "saPlmEntityGroupRemove_6 with bad grouphandle SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2, saPlmEntityGroupRemove_07, "saPlmEntityGroupRemove_7 with bad grouphandle SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2, saPlmEntityGroupRemove_08, "saPlmEntityGroupRemove_8 with no DN name specified SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2, saPlmEntityGroupRemove_10, "saPlmEntityGroupRemove_10 with no of entities are blank SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2, saPlmEntityGroupRemove_11, "saPlmEntityGroupRemove_11 with DN already removed  SA_AIS_ERR_NOT_EXIST");
   test_case_add(2, saPlmEntityGroupRemove_12, "saPlmEntityGroupRemove_12 with non existent DN");
   test_case_add(2, saPlmEntityGroupRemove_13, "saPlmEntityGroupRemove_13 with different delete option than add SA_AIS_ERR_INVALID_PARAM ");
   test_case_add(2, saPlmEntityGroupRemove_14, "saPlmEntityGroupRemove_14 with different delete option than add SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2, saPlmEntityGroupRemove_15, "saPlmEntityGroupRemove_15 with different delete option than add SA_AIS_ERR_INVALID_PARAM");
   test_case_add(2, saPlmEntityGroupRemove_16, "saPlmEntityGroupRemove_16 to remove a child while parent is added with subtree option SA_AIS_OK");
   
   test_case_add(2, saPlmEntityGroupRemove_17, "saPlmEntityGroupRemove_17 to remove a child while parent is added with different subtree option SA_AIS_ERR_INVALID_PARAM ");
   test_case_add(2, saPlmEntityGroupRemove_18, "saPlmEntityGroupRemove_18 to remove a entity which does not belong to group SA_AIS_ERR_NOT_EXIST ");
   test_case_add(2,saPlmEntityGroupDelete_01, "saPlmEntityGroupDelete_01 - SA_AIS_OK ");
   test_case_add(2,saPlmEntityGroupDelete_02, "saPlmEntityGroupDelete_02 with invalid grouphandle - SA_AIS_ERR_BAD_HANDLE");
   test_case_add(2,saPlmEntityGroupDelete_03, "saPlmEntityGroupDelete_03 with invalid grouphandle after plmhandle got finalize - SA_AIS_ERR_BAD_HANDLE");

}
 
