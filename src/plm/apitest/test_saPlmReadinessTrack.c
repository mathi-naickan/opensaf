#include "plm/apitest/plmtest.h"
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"
#include "plm/saf/saPlm.h"
#include <sys/select.h> 

SaPlmEntityGroupHandleT entityGrpHandle;
SaAisErrorT rc_from_test;
fd_set read_fd;
char groupaddoption[10];
SaNameT testDnptr;
int counter=0;

static void checktrackentval(const SaPlmReadinessTrackedEntitiesT *);

typedef struct
{
 SaNameT testDnptr;
 char enitytype[3];
}entity_array;

entity_array entarr[10];
		    	
int num_elements;
int no_of_trk_elements;
typedef enum {
	SA_AIS_INVALID_TRACKCOOKIE=41,
	SA_AIS_INVALID_TRACKENTITY=42,
	SA_AIS_INVALID_CAUSE=43	
	} SaAisErrorTest;

void getDn ( int i ) {
        memset(groupaddoption, 0, 10 * sizeof(char));
	switch ( i )
        {
             case 1 :
		memset(&entarr[0], 0, sizeof(entity_array));
    		memcpy(entarr[0].testDnptr.value,
                       f120_slot_1_dn.value,
                       f120_slot_1_dn.length); 
    		entarr[0].testDnptr.length =f120_slot_1_dn.length;
                strcpy(entarr[0].enitytype,"HE");
 		num_elements=1;
		no_of_trk_elements=1;
		strncpy(groupaddoption,"single",strlen("single"));
		break;
	     case 2 :
 		num_elements = 4;
		memset(entarr, 0, 4 * sizeof(entity_array));
    		memcpy(entarr[0].testDnptr.value,
                       f120_slot_1_dn.value,
                       f120_slot_1_dn.length); 
    		entarr[0].testDnptr.length =f120_slot_1_dn.length;
                strcpy(entarr[0].enitytype,"HE");
    		memcpy(entarr[1].testDnptr.value,
                       amc_slot_1_dn.value,
                       amc_slot_1_dn.length); 
    		entarr[1].testDnptr.length =amc_slot_1_dn.length;
                strcpy(entarr[1].enitytype,"HE");
    		memcpy(entarr[2].testDnptr.value,
                       amc_slot_1_eedn.value,
                       amc_slot_1_eedn.length); 
    		entarr[2].testDnptr.length =amc_slot_1_eedn.length;
                strcpy(entarr[2].enitytype,"EE");
    		memcpy(entarr[3].testDnptr.value,
                       f120_slot_1_eedn.value,
                       f120_slot_1_eedn.length); 
    		entarr[3].testDnptr.length =f120_slot_1_eedn.length;
                strcpy(entarr[3].enitytype,"EE");
		no_of_trk_elements=1;
		strncpy(groupaddoption,"subtree",strlen("subtree"));	
		break;
	     case 3:
 		num_elements=3;
		memset(entarr, 0, num_elements * sizeof(entity_array));
    		memcpy(entarr[0].testDnptr.value,
                       f120_slot_1_dn.value,
                       f120_slot_1_dn.length); 
    		entarr[0].testDnptr.length =f120_slot_1_dn.length;
                strcpy(entarr[0].enitytype,"HE");
    		memcpy(entarr[1].testDnptr.value,
                       f120_slot_1_eedn.value,
                       f120_slot_1_eedn.length); 
    		entarr[1].testDnptr.length = f120_slot_1_eedn.length;
                strcpy(entarr[1].enitytype,"EE");
    		memcpy(entarr[2].testDnptr.value,
                       amc_slot_1_eedn.value,
                       amc_slot_1_eedn.length); 
    		entarr[2].testDnptr.length = amc_slot_1_eedn.length;
                strcpy(entarr[2].enitytype,"EE");
		no_of_trk_elements=1;
		strncpy(groupaddoption,"subtreeEE",strlen("subtreeEE"));	
		break;
	     case 4:
		memset(&entarr[0], 0, sizeof(entity_array));
		memset(&entarr[1], 0, sizeof(entity_array));
    		memcpy(entarr[0].testDnptr.value,
                       f120_slot_1_dn.value,
                       f120_slot_1_dn.length); 
    		entarr[0].testDnptr.length =f120_slot_1_dn.length;
                strcpy(entarr[0].enitytype,"HE");
    		memcpy(entarr[1].testDnptr.value,
                       amc_slot_1_dn.value,
                       amc_slot_1_dn.length); 
    		entarr[1].testDnptr.length =amc_slot_1_dn.length;
                strcpy(entarr[1].enitytype,"HE");
 		num_elements=2;
		no_of_trk_elements=1;
		strncpy(groupaddoption,"subtreeHE",strlen("subtreeHE"));	
		break;
	     default :
                printf ("\n The testcase id provided is invalid ");
	}
}


void TrackCallback_01(
            SaPlmEntityGroupHandleT entityGroupHandle,
            SaUint64T trackCookie,
            SaInvocationT invocation,
            SaPlmTrackCauseT cause,
            const SaNameT *rootCauseEntity,
            SaNtfIdentifierT rootCorrelationId,
            const SaPlmReadinessTrackedEntitiesT *trackedEntities,
            SaPlmChangeStepT step,
            SaAisErrorT error) {

        rc_from_test=0;

	if ( trackCookie != 121 &&  error == SA_AIS_OK  ) {
		rc_from_test=SA_AIS_INVALID_TRACKCOOKIE;
	} else if ( entityGroupHandle != entityGrpHandle ) {
		rc_from_test=SA_AIS_ERR_BAD_HANDLE;
	} else if ( cause != SA_PLM_CAUSE_STATUS_INFO) {
		rc_from_test=SA_AIS_INVALID_CAUSE;
	} else {
		checktrackentval(trackedEntities);
	}
}


void TrackCallback_02(
            SaPlmEntityGroupHandleT entityGroupHandle,
            SaUint64T trackCookie,
            SaInvocationT invocation,
            SaPlmTrackCauseT cause,
            const SaNameT *rootCauseEntity,
            SaNtfIdentifierT rootCorrelationId,
            const SaPlmReadinessTrackedEntitiesT *trackedEntities,
            SaPlmChangeStepT step,
            SaAisErrorT error) {

        rc_from_test=0;

        if ( trackCookie != 121 &&  error == SA_AIS_OK  ) {
                rc_from_test=SA_AIS_INVALID_TRACKCOOKIE;
        } else if ( entityGroupHandle != entityGrpHandle ) {
                rc_from_test=SA_AIS_ERR_BAD_HANDLE;
        } else if ( cause != SA_PLM_CAUSE_GROUP_CHANGE) {
                rc_from_test=SA_AIS_INVALID_CAUSE;
        } else {
                checktrackentval(trackedEntities);
        }
}


void TrackCallback_03(
            SaPlmEntityGroupHandleT entityGroupHandle,
            SaUint64T trackCookie,
            SaInvocationT invocation,
            SaPlmTrackCauseT cause,
            const SaNameT *rootCauseEntity,
            SaNtfIdentifierT rootCorrelationId,
            const SaPlmReadinessTrackedEntitiesT *trackedEntities,
            SaPlmChangeStepT step,
            SaAisErrorT error) {
	counter ++;
	printf("\nyour are exectuing %d callback of the program",counter);

}


static void checktrackentval(const SaPlmReadinessTrackedEntitiesT *trackedEntities) {
	int i,j; 	
        if ( trackedEntities->numberOfEntities != num_elements   ) {
		rc_from_test=SA_AIS_INVALID_TRACKENTITY;
        } else {
		for ( i=0; i < trackedEntities->numberOfEntities ; i++ ) {
                        for (j=0 ; j < num_elements ; j++ ) {
			    rc_from_test=SA_AIS_INVALID_TRACKENTITY;
			    /*
                            if ((strcmp(groupaddoption,"subtreeEE")) == 0 && (i > 0)) 
			    {
				if( (strncmp(entarr[j].enitytype,"EE",2)) != 0 ) {
					continue;	
				}		
                            } else if ((strcmp(groupaddoption,"subtreeHE")) == 0) 
			    {
				if( (strncmp(entarr[j].enitytype,"HE",2)!= 0 )) {
                                        continue;
                                }
                            } */	
                            if (((trackedEntities->entities+i)->entityName.length == entarr[j].testDnptr.length) &&
                                memcmp((trackedEntities->entities+i)->entityName.value , entarr[j].testDnptr.value, entarr[j].testDnptr.length) == 0) {
                		rc_from_test=SA_AIS_OK;
				break;
                            } 
                        }
			if ( (int)rc_from_test == SA_AIS_INVALID_TRACKENTITY ) {
				return;
			} 
                }
	}
}  


void InitTrackingTest(int i, SaPlmGroupOptionsT groupaddoption ,SaUint8T trackingoption ) {

    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(i); 
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,groupaddoption), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, trackingoption ,121, 0),SA_AIS_OK);
    
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK); 

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
	    FD_CLR(selectionObject , &read_fd); 
        }
    	break;
    }
   
    printf ("\nFollowing is the output return code of the test : rc_from_test "); 
    test_validate(rc_from_test , SA_AIS_OK);     
   // safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}






void saPlmReadinessTrack_01(void)
{
	printf ("\nStarting the test case saPlmReadinessTrack_01\n" );
	InitTrackingTest(1,SA_PLM_GROUP_SINGLE_ENTITY,SA_TRACK_CURRENT); 
	printf ("\nCompleted the test case saPlmReadinessTrack_01" );	
}


void saPlmReadinessTrack_02(void)
{
	printf ("\nStarting the test case saPlmReadinessTrack_02\n" );
	InitTrackingTest(2,SA_PLM_GROUP_SUBTREE,SA_TRACK_CURRENT);	
	printf ("\nCompleted the test case saPlmReadinessTrack_02" );
}



void saPlmReadinessTrack_03(void)
{
	printf ("\nStarting the test case saPlmReadinessTrack_03\n" );
	InitTrackingTest(3,SA_PLM_GROUP_SUBTREE_EES_ONLY ,SA_TRACK_CURRENT);	
	printf ("\nCompleted the test case saPlmReadinessTrack_03" );
}


void saPlmReadinessTrack_04(void)
{
	printf ("\nStarting the test case saPlmReadinessTrack_04\n" );
	InitTrackingTest(4,SA_PLM_GROUP_SUBTREE_HES_ONLY ,SA_TRACK_CURRENT);	
	printf ("\nCompleted the test case saPlmReadinessTrack_04" );
}


void saPlmReadinessTrack_05(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[4];
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_05" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle, &entityGroupHandle), SA_AIS_OK);
    getDn(2);
    SaNtfCorrelationIdsT corr_id;
    memset(&corr_id, 0, sizeof(SaNtfCorrelationIdsT));
    corr_id.rootCorrelationId = SA_NTF_IDENTIFIER_UNUSED;
    corr_id.parentCorrelationId = SA_NTF_IDENTIFIER_UNUSED;
    saPlmEntityReadinessImpact(plmHandle, &entarr[0].testDnptr, SA_PLM_RI_IMMINENT_FAILURE, &corr_id);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SUBTREE),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list, 0, 4 * sizeof(SaPlmReadinessTrackedEntityT));
    entity_list[0].entityName.length = f120_slot_1_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_1_dn.value, f120_slot_1_dn.length);
    entity_list[1].entityName.length = amc_slot_1_dn.length;
    memcpy(entity_list[1].entityName.value, amc_slot_1_dn.value, amc_slot_1_dn.length);
    entity_list[2].entityName.length = amc_slot_1_eedn.length;
    memcpy(entity_list[2].entityName.value, amc_slot_1_eedn.value, amc_slot_1_eedn.length);
    entity_list[3].entityName.length = f120_slot_1_eedn.length;
    memcpy(entity_list[3].entityName.value, f120_slot_1_eedn.value, f120_slot_1_eedn.length);
    ents.numberOfEntities = 4; 
    ents.entities = NULL;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    checktrackentval(&ents);
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessNotificationFree(entityGroupHandle, ents.entities), SA_AIS_OK); 
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_05" );
}


void saPlmReadinessTrack_06(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[2];
    printf ("\nStarting the test case saPlmReadinessTrack_06" );
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SUBTREE_HES_ONLY),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list, 0, 2 * sizeof(SaPlmReadinessTrackedEntityT));
    entity_list[0].entityName.length = f120_slot_1_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_1_dn.value, f120_slot_1_dn.length);
    entity_list[1].entityName.length = amc_slot_1_dn.length;
    memcpy(entity_list[1].entityName.value, amc_slot_1_dn.value, amc_slot_1_dn.length);
    ents.numberOfEntities = 2;
    ents.entities = entity_list;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    checktrackentval(&ents);
    test_validate(rc_from_test , SA_AIS_OK);
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_06" );
}



void saPlmReadinessTrack_07(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[3];
    printf ("\nStarting the test case saPlmReadinessTrack_07" );
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(3);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements,SA_PLM_GROUP_SUBTREE_EES_ONLY),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list, 0,3 * sizeof(SaPlmReadinessTrackedEntityT));
    entity_list[0].entityName.length = f120_slot_1_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_1_dn.value, f120_slot_1_dn.length);
    entity_list[1].entityName.length = f120_slot_1_eedn.length;
    memcpy(entity_list[1].entityName.value, f120_slot_1_eedn.value, f120_slot_1_eedn.length);
    entity_list[2].entityName.length = amc_slot_1_eedn.length;
    memcpy(entity_list[2].entityName.value, amc_slot_1_eedn.value, amc_slot_1_eedn.length);
    ents.numberOfEntities = 3;
    ents.entities = entity_list;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    checktrackentval(&ents);
    test_validate(rc_from_test , SA_AIS_OK);
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_07" );
}

void saPlmReadinessTrack_08(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[2];
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_08" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list[0], 0, sizeof(SaPlmReadinessTrackedEntityT));
    memset(&entity_list[1], 0, sizeof(SaPlmReadinessTrackedEntityT));
    entity_list[0].entityName.length = f120_slot_1_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_1_dn.value, f120_slot_1_dn.length);
    ents.numberOfEntities = 1;
    ents.entities = entity_list;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    checktrackentval(&ents);
    test_validate(rc_from_test , SA_AIS_OK);
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_08" );
}


void saPlmReadinessTrack_09(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_09" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    rc=saPlmReadinessTrack(0, SA_TRACK_CURRENT,121, 0);
    test_validate(rc , SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_09" );
}


void saPlmReadinessTrack_10(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_10" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    rc=saPlmReadinessTrack(entityGroupHandle,0,121, 0);
    test_validate(rc , SA_AIS_ERR_BAD_FLAGS);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_10" );
}


void saPlmReadinessTrack_11(void)
{
    printf ("\nStarting the test case saPlmReadinessTrack_11" );
    safassert(saPlmInitialize(&plmHandle, NULL , &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    rc=saPlmReadinessTrack(entityGroupHandle,SA_TRACK_CURRENT,121,0);
    test_validate(rc , SA_AIS_ERR_INIT);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_11" );
}


void saPlmReadinessTrack_12(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_12" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    rc=saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , 0 ,SA_PLM_GROUP_SINGLE_ENTITY);
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, 0),SA_AIS_OK);
    test_validate(rc ,SA_AIS_ERR_INVALID_PARAM );
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_12" );
}


void saPlmReadinessTrack_13(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[1];
    entity_list[0].entityName.length = f120_slot_1_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_1_dn.value, f120_slot_1_dn.length);
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_13" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , 1 ,SA_PLM_GROUP_SUBTREE_HES_ONLY),SA_AIS_OK);
    ents.numberOfEntities = 1;
    ents.entities = entity_list;
    rc=saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents);
    test_validate(rc ,SA_AIS_ERR_NO_SPACE);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_13" );
}

void saPlmReadinessTrack_14(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CURRENT ,121, 0),SA_AIS_OK);
    safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_15(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(3);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CURRENT ,121, 0),SA_AIS_OK);
    safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    // safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_16(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CURRENT ,121, 0),SA_AIS_OK);
    safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    //safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_17(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    //safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    rc_from_test=saPlmReadinessTrackStop(entityGroupHandle);
    test_validate(rc_from_test , SA_AIS_ERR_BAD_HANDLE);
}


void saPlmReadinessTrack_18(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    rc_from_test=saPlmReadinessTrackStop(-1);
    test_validate(rc_from_test , SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_19(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    rc_from_test=saPlmReadinessTrackStop(entityGroupHandle);
    test_validate(rc_from_test , SA_AIS_ERR_NOT_EXIST);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_20(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[4];
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_20" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
   safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SUBTREE),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list[0], 0, sizeof(entity_list));
    ents.numberOfEntities = 4;
    ents.entities = entity_list; 
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    checktrackentval(&ents);
    printf ("\nCompleted the test case saPlmReadinessTrack_08" );
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    test_validate(rc_from_test , SA_AIS_OK);

}

void saPlmReadinessTrack_21(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents;
    SaPlmReadinessTrackedEntityT entity_list[2];
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_08" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list[0], 0, sizeof(SaPlmReadinessTrackedEntityT));
    ents.numberOfEntities = 1;
    ents.entities = NULL;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    rc_from_test=saPlmReadinessNotificationFree(0,ents.entities);
    test_validate(rc_from_test , SA_AIS_ERR_BAD_HANDLE);
    safassert(saPlmReadinessNotificationFree(entityGroupHandle,ents.entities),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_08" );
}


void saPlmReadinessTrack_22(void)
{
    SaPlmCallbacksT plms_cbks;
    SaPlmReadinessTrackedEntitiesT ents,ents1;
    SaPlmReadinessTrackedEntityT entity_list[2];
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_01;
    printf ("\nStarting the test case saPlmReadinessTrack_21" );
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle),SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr , no_of_trk_elements ,SA_PLM_GROUP_SINGLE_ENTITY),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&ents, 0, sizeof(SaPlmReadinessTrackedEntitiesT));
    memset(&entity_list[0], 0, sizeof(SaPlmReadinessTrackedEntityT));
    ents.numberOfEntities = 1;
    ents.entities = NULL;
    safassert(saPlmReadinessTrack(entityGroupHandle, SA_TRACK_CURRENT,121, &ents),SA_AIS_OK);
    memset(&entity_list[0], 0, sizeof(entity_array));
    entity_list[0].entityName.length = f120_slot_16_dn.length;
    memcpy(entity_list[0].entityName.value, f120_slot_16_dn.value, f120_slot_16_dn.length);
    ents.numberOfEntities = 1;
    ents1.entities=ents.entities;
    ents1.numberOfEntities = 1;
    ents.entities = entity_list;
    rc_from_test=saPlmReadinessNotificationFree(entityGroupHandle,ents.entities);
    test_validate(rc_from_test , SA_AIS_ERR_INVALID_PARAM);
    safassert(saPlmReadinessNotificationFree(entityGroupHandle,ents1.entities),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
    printf ("\nCompleted the test case saPlmReadinessTrack_22" );
}

/* GROUP change CASES */

void saPlmReadinessTrack_23(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[1], 0, sizeof(entity_array));
    memcpy(entarr[1].testDnptr.value,
           amc_slot_1_dn.value,
           amc_slot_1_dn.length);
    entarr[1].testDnptr.length =amc_slot_1_dn.length;
    strcpy(entarr[1].enitytype,"HE");   
    num_elements=2;
    no_of_trk_elements=2;
    strncpy(groupaddoption,"single",strlen("single"));
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[1].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK); 
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}

void saPlmReadinessTrack_24(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[4], 0, sizeof(entity_array));
    memcpy(entarr[4].testDnptr.value, amc_slot_16_dn.value, amc_slot_16_dn.length);
    entarr[4].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[4].enitytype,"HE");
    num_elements=5;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[4].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}


void saPlmReadinessTrack_25(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memset(&entarr[4], 0, sizeof(entity_array));
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[1].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[1].enitytype,"HE");
    memcpy(entarr[2].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[2].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[2].enitytype,"HE");
    memcpy(entarr[3].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[3].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[3].enitytype,"EE");
    memcpy(entarr[4].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[4].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[4].enitytype,"EE");
    num_elements=5;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[1].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}


void saPlmReadinessTrack_26(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[4], 0, sizeof(entity_array));
    memset(&entarr[5], 0, sizeof(entity_array));
    memset(&entarr[6], 0, sizeof(entity_array));
    memset(&entarr[7], 0, sizeof(entity_array));
    memcpy(entarr[4].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[4].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[4].enitytype,"HE");
    memcpy(entarr[5].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[5].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[5].enitytype,"HE");
    memcpy(entarr[6].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[6].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[6].enitytype,"EE");
    memcpy(entarr[7].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[7].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[7].enitytype,"EE");
    num_elements=8;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[4].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_27(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[4], 0, sizeof(entity_array));
    memset(&entarr[5], 0, sizeof(entity_array));
    memcpy(entarr[4].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[4].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[4].enitytype,"HE");
    memcpy(entarr[5].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[5].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[5].enitytype,"HE");
    num_elements=6;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[4].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_28(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memcpy(entarr[2].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[2].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[2].enitytype,"HE");
    memcpy(entarr[3].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[3].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[3].enitytype,"HE");
    num_elements=4;
    no_of_trk_elements=2;
    memset(groupaddoption, 0, 10 * sizeof(char));
    strncpy(groupaddoption,"subtreeHE",strlen("subtreeHE"));
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[2].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_29(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(3);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[3], 0, sizeof(entity_array));
    memset(&entarr[4], 0, sizeof(entity_array));
    memset(&entarr[5], 0, sizeof(entity_array));
    memcpy(entarr[3].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[3].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[3].enitytype,"HE");
    memcpy(entarr[4].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[4].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[4].enitytype,"EE");
    memcpy(entarr[5].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[5].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[5].enitytype,"EE");
    num_elements=6;
    no_of_trk_elements=2;
    strncpy(groupaddoption,"subtreeEE",strlen("subtreeEE"));
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[3].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_30(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memset(&entarr[4], 0, sizeof(entity_array));
    memcpy(entarr[2].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[2].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[2].enitytype,"HE");
    memcpy(entarr[3].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[3].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[3].enitytype,"EE");
    memcpy(entarr[4].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[4].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[4].enitytype,"EE");
    num_elements=5;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[2].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_31(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[1].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[1].enitytype,"HE");
    memcpy(entarr[2].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[2].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    memcpy(entarr[3].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[3].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[3].enitytype,"EE");
    num_elements=4;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[1].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

/* group remove cases CHANGES category */
void saPlmReadinessTrack_32(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[1], 0, sizeof(entity_array));
    memcpy(entarr[1].testDnptr.value,
           amc_slot_1_dn.value,
           amc_slot_1_dn.length);
    entarr[1].testDnptr.length =amc_slot_1_dn.length;
    strcpy(entarr[1].enitytype,"HE");
    num_elements=2;
    no_of_trk_elements=2;
    strncpy(groupaddoption,"single",strlen("single"));
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[1].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[1].testDnptr , 1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}

void saPlmReadinessTrack_33(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[1].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[1].enitytype,"HE");
    memcpy(entarr[2].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[2].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    memcpy(entarr[3].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[3].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[3].enitytype,"EE");
    num_elements=4;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[1].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[1].testDnptr , 1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_34(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memset(&entarr[4], 0, sizeof(entity_array));
    memcpy(entarr[2].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[2].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[2].enitytype,"HE");
    memcpy(entarr[3].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[3].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[3].enitytype,"EE");
    memcpy(entarr[4].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[4].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[4].enitytype,"EE");
    num_elements=5;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[2].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[2].testDnptr , 1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_35(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES ,121, 0),SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[4], 0, sizeof(entity_array));
    memcpy(entarr[4].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[4].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[4].enitytype,"HE");
    num_elements=5;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[4].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[4].testDnptr , 1  ),SA_AIS_OK); 
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

/* member add and member remove in a group cases CHANGES_ONLY category */
void saPlmReadinessTrack_36(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           amc_slot_1_dn.value,
           amc_slot_1_dn.length);
    entarr[0].testDnptr.length =amc_slot_1_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    num_elements=1;
    no_of_trk_elements=2;
    strncpy(groupaddoption,"single",strlen("single"));
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}


void saPlmReadinessTrack_37(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           amc_slot_1_dn.value,
           amc_slot_1_dn.length);
    entarr[0].testDnptr.length =amc_slot_1_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    num_elements=1;
    no_of_trk_elements=2;
    strncpy(groupaddoption,"single",strlen("single"));
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[0].testDnptr ,1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);

}

void saPlmReadinessTrack_38(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[0].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[1].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[1].enitytype,"EE");
    memcpy(entarr[2].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[2].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    
    strncpy(groupaddoption,"subtreeEE",strlen("subtreeEE"));	
    num_elements=3;
    no_of_trk_elements=2;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_39(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(1);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[0].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[1].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[1].enitytype,"EE");
    memcpy(entarr[2].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[2].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    strncpy(groupaddoption,"subtreeEE",strlen("subtreeEE"));	
    num_elements=3;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[0].testDnptr , 1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_40(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[0].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[1].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[1].enitytype,"EE");
    memcpy(entarr[2].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[2].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    num_elements=3;
    no_of_trk_elements=2;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_41(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(4);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE_HES_ONLY), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memset(&entarr[2], 0, sizeof(entity_array));
    memset(&entarr[3], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           f120_slot_16_dn.value,
           f120_slot_16_dn.length);
    entarr[0].testDnptr.length =f120_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    memcpy(entarr[1].testDnptr.value,
           f120_slot_16_eedn.value,
           f120_slot_16_eedn.length);
    entarr[1].testDnptr.length =f120_slot_16_eedn.length;
    strcpy(entarr[1].enitytype,"EE");
    memcpy(entarr[2].testDnptr.value,
           amc_slot_16_eedn.value,
           amc_slot_16_eedn.length);
    entarr[2].testDnptr.length =amc_slot_16_eedn.length;
    strcpy(entarr[2].enitytype,"EE");
    num_elements=3;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SUBTREE_EES_ONLY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[0].testDnptr , 1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_42(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    memcpy(entarr[1].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[0].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    num_elements=1;
    no_of_trk_elements=2;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}


void saPlmReadinessTrack_43(void)
{
    SaPlmCallbacksT plms_cbks;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_02;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[0].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    num_elements=1;
    no_of_trk_elements=2;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[0].testDnptr ,1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ONE),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    printf ("\nFollowing is the output return code of the test : rc_from_test ");
    test_validate(rc_from_test , SA_AIS_OK);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}

void saPlmReadinessTrack_44(void)
{
    SaPlmCallbacksT plms_cbks;
    int i=0;
    plms_cbks.saPlmReadinessTrackCallback = &TrackCallback_03;
    safassert(saPlmInitialize(&plmHandle, &plms_cbks, &PlmVersion), SA_AIS_OK);
    safassert(saPlmEntityGroupCreate(plmHandle,&entityGroupHandle), SA_AIS_OK);
    getDn(2);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES_ONLY ,121, 0),SA_AIS_OK);
    i++;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, no_of_trk_elements,SA_PLM_GROUP_SUBTREE), SA_AIS_OK);
    entityGrpHandle=entityGroupHandle;
    memset(&entarr[0], 0, sizeof(entity_array));
    memset(&entarr[1], 0, sizeof(entity_array));
    memcpy(entarr[0].testDnptr.value,
           amc_slot_16_dn.value,
           amc_slot_16_dn.length);
    entarr[0].testDnptr.length =amc_slot_16_dn.length;
    strcpy(entarr[0].enitytype,"HE");
    num_elements=1;
    no_of_trk_elements=2;
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES|SA_TRACK_CURRENT,121, 0),SA_AIS_OK);
    i++;
    i++;
    safassert(saPlmEntityGroupAdd(entityGroupHandle , &entarr[0].testDnptr, 1 ,SA_PLM_GROUP_SINGLE_ENTITY), SA_AIS_OK);
    safassert(saPlmReadinessTrack( entityGroupHandle, SA_TRACK_CHANGES|SA_TRACK_CURRENT,121, 0),SA_AIS_OK);
    i++;
    i++;
    safassert(saPlmEntityGroupRemove(entityGroupHandle, &entarr[0].testDnptr ,1 ),SA_AIS_OK);
    safassert(saPlmSelectionObjectGet(plmHandle, &selectionObject),SA_AIS_OK);

    FD_ZERO(&read_fd);
    FD_SET(selectionObject , &read_fd);
    while (select(selectionObject+1 , &read_fd, 0, 0, NULL) > 0 )
    {
        /* process all the PLM  messages */
        if (FD_ISSET(selectionObject ,&read_fd))
        {
            safassert(saPlmDispatch(plmHandle, SA_DISPATCH_ALL),SA_AIS_OK);
            FD_CLR(selectionObject , &read_fd);
        }
        break;
    }

    test_validate(counter , i);
    printf ("Total callbacks called using DISPATCH_ALL : %d" , counter);
    safassert(saPlmReadinessTrackStop(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmEntityGroupDelete(entityGroupHandle),SA_AIS_OK);
    safassert(saPlmFinalize(plmHandle), SA_AIS_OK);
}




__attribute__ ((constructor)) static void saPlmReadinessTrackCurrent_constructor(void)
{
    test_suite_add(3, "Readiness track");
    test_case_add(3, saPlmReadinessTrack_01 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SINGLE_ENTITY option (async call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_02 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE option (async call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_03 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE_EES_ONLY (async call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_04 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE_HES_ONLY (async call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_05 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE option (sync call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_06 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE_HE_ONLY option (sync call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_07 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SUBTREE_EE_ONLY option (sync call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_08 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with SA_PLM_GROUP_SINGLE_ENTITY option (sync call) - SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_09 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with bad plmhandle SA_AIS_ERR_BAD_HANDLE ");
    test_case_add(3, saPlmReadinessTrack_10 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with bad flag SA_AIS_ERR_BAD_FLAGS");
    test_case_add(3, saPlmReadinessTrack_11 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with null callback SA_AIS_ERR_INIT");
    test_case_add(3, saPlmReadinessTrack_12 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with invalid group option SA_AIS_ERR_INVALID_PARAM");
    test_case_add(3, saPlmReadinessTrack_13 , "\nsaPlmReadinesstrack SA_TRACK_CURRENT with the value of no of entities smaller than no of entities tracked  - SA_AIS_ERR_NO_SPACE");
    test_case_add(3, saPlmReadinessTrack_14 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with only dispatch SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_15 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with only dispatch SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_16 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with only dispatch SA_AIS_OK");
    test_case_add(3, saPlmReadinessTrack_17 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with trackstop after finalize of plmhandle");
    test_case_add(3, saPlmReadinessTrack_18 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with invalid group handle in the trackstop");
    test_case_add(3, saPlmReadinessTrack_19 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with trackstop without corrosponding track start");
    // test_case_add(3, saPlmReadinessTrack_20 , "\nsaPlmReadinessNotificationFree - SA_AIS_OK");
   test_case_add(3, saPlmReadinessTrack_21 , "\nsaPlmReadinessNotificationFree with bad handle - SA_AIS_ERR_BAD_HANDLE");
    test_case_add(3, saPlmReadinessTrack_22 , "\nsaPlmReadinessTrack SA_TRACK_CURRENT with invalid param - SA_AIS_ERR_INVALID_PARAM");
    test_case_add(3, saPlmReadinessTrack_23 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of single entity - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_24 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of single entity after the subtree already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_25 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_26 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree after the subtree already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_27 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree with HES only after the subtree already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_28 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree with HES only after the subtree with HES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_29 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree with EES only after the subtree with EES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_30 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree with HES only after the subtree with EES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_31 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add of subtree with EES only after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_32 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add and group remove of single entity - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_33 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add and group remove  of subtree with EES only after the single entity already added in the group - SA-AIS_OK");
    
    test_case_add(3, saPlmReadinessTrack_34 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add and group remove of subtree with HES only after the subtree with EES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_35 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES with group add and group remove of single entity after the entities with subtree option already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_36 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add of single enity  after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_37 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add and group remove of single enity  after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_38 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add of subtree with EES only  after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_39 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add and group remove of subtree with EES only  after the single entity already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_40 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add of subtree with EES only  after the subtree with HES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_41 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add and group remove of subtree with EES only  after the subtree with HES only already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_42 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add of single entity  after the entity with subtree option already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_43 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add and group remove  of single entity  after the entity with subtree option already added in the group - SA-AIS_OK");
    test_case_add(3, saPlmReadinessTrack_44 , "\nsaPlmReadinessTrack SA_TRACK_CHANGES_ONLY with group add and group remove  of single entity  after the entity with subtree option already added in the group - SA-AIS_OK");
}
