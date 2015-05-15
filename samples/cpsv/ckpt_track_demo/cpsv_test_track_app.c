/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 */

/*****************************************************************************
  ..............................................................................
  MODULE NAME: cpsv_test_app.c  (CPSv Test Functions)

  .............................................................................
DESCRIPTION:

CPSv routines required for Demo Applications.


 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <saCkpt.h>
#include <saCkpt_B_02_03.h>
#include <time.h>

#define DEMO_CKPT_NAME "safCkpt=DemoCkpt,safApp=safCkptService"

void AppCkptOpenCallback(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle, SaAisErrorT error);
void AppCkptSyncCallback(SaInvocationT invocation, SaAisErrorT error);
void cpsv_test_sync_app_process(void *info);


void AppCkptOpenCallback(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle, SaAisErrorT error)
{
	if (error != SA_AIS_OK) {
		printf("Checkpoint Open Async callback unsuccessful\n");
		return;
	}
	else {
		printf("Checkpoint Open Async callback success and ckpt_hdl %llu \n",checkpointHandle);
		return;
	}
}
void AppCkptSyncCallback(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK)	{
		printf("Checkpoint Sync Callback unsuccessful\n");
		return;
	}
	else {
		printf("Checkpoint Sync Callback success\n");
		return;
	}
}


void AppCkptTrackCallback(const SaCkptCheckpointHandleT checkpointHandle, SaCkptIOVectorElementT *ioVector,SaUint32T numberOfElements)
{
	char rdata[numberOfElements][10000];
	SaCkptIOVectorElementT readVector[numberOfElements];
	int iteration = 0;
	SaCkptSectionIdT sectionId[numberOfElements];

	printf("Ckpt TrackCallback received for SectionId`s = \n");
	while(iteration < numberOfElements) {

		memset(rdata[iteration],0,10000);

		sectionId[iteration].id = (unsigned char *)calloc(2,sizeof(unsigned char));
		memcpy((char *)sectionId[iteration].id,ioVector[iteration].sectionId.id,ioVector[iteration].sectionId.idLen);
		sectionId[iteration].idLen = ioVector[iteration].sectionId.idLen ;

		readVector[iteration].sectionId  = sectionId[iteration];
		readVector[iteration].dataBuffer = rdata[iteration];
		readVector[iteration].dataSize   = ioVector[iteration].dataSize;
		readVector[iteration].dataOffset = 0;
		readVector[iteration].readSize   = 0;

		printf("Section-Id = %s ....\n",readVector[iteration].sectionId.id);
		printf("SectionId-idLen = %d ....\n", readVector[iteration].sectionId.idLen);

		iteration++;
	}

	printf("Reading from Checkpoint %s  Ckpt handle =%llX ....\n",DEMO_CKPT_NAME, checkpointHandle);
	SaUint32T error_index;
	SaAisErrorT rc = saCkptCheckpointRead(checkpointHandle,
			readVector,
			numberOfElements,
			&error_index);
	printf("\n");	
	if(rc == SA_AIS_OK) {
		iteration = 0;
		while(iteration < numberOfElements) {
			printf("CheckpointRead Checkpoint TrackCallback processed \n");
			printf("CheckpointData was written in sectionId: %s = \"%s\"\n",
					readVector[iteration].sectionId.id,
					(char *)readVector[iteration].dataBuffer);
			iteration++;
		}
	}
	else {
		printf("CheckpointRead Failed in Checkpoint TrackCallback  & returned %lld,Error=%d\n",
				(long long int)rc,error_index);
	}
	printf("\n");
}

/****************************************************************************
 * Name          : cpsv_test_sync_app_process
 *
 * Description   : This is the function which is given as the input to the
 *                 Application task.
 *
 * Arguments     : info  - This is the information which is passed during
 *                         spawing Application task.
 *
 * Return Values : None.
 *
 * Notes         : None.
 *****************************************************************************/
void cpsv_test_sync_app_process(void *info)
{
	SaCkptHandleT ckptHandle;
	SaCkptCheckpointHandleT  checkpointHandle;
	SaCkptCallbacksT_2 callbk;
	SaVersionT version;
	SaNameT ckptName;
	SaAisErrorT rc;
	SaCkptCheckpointCreationAttributesT ckptCreateAttr;
	SaCkptCheckpointOpenFlagsT ckptOpenFlags;
	SaCkptSectionCreationAttributesT sectionCreationAttributes;
	SaCkptIOVectorElementT writeVector;
	SaUint32T erroneousVectorIndex;
	void *initialData = "Default data in the section";
	SaTimeT timeout = 1000000000;
	unsigned int  temp_var = (unsigned int)(long)info; 


	memset(&ckptName, 0, 255);
	ckptName.length = strlen(DEMO_CKPT_NAME);
	memcpy(ckptName.value,DEMO_CKPT_NAME,strlen(DEMO_CKPT_NAME));

	callbk.saCkptCheckpointOpenCallback = AppCkptOpenCallback;
	callbk.saCkptCheckpointSynchronizeCallback = AppCkptSyncCallback;
	/*callbk.saCkptCheckpointTrackCallback = NULL; */ 
	callbk.saCkptCheckpointTrackCallback = AppCkptTrackCallback;
	version.releaseCode= 'B';
	version.majorVersion = 2;
	version.minorVersion = 3;

	printf("*******************************************************************\n");
	printf("Demonstrating Checkpoint Service Usage with a Track Callback \n");
	printf("*******************************************************************\n");

	printf("Initialising With Checkpoint Service....\n");
	rc = saCkptInitialize_2(&ckptHandle,&callbk,&version);
	if(rc != SA_AIS_OK)
		printf(" saCkptInitialize_2 Failed \n");

	ckptCreateAttr.creationFlags = SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA;
	ckptCreateAttr.checkpointSize = 1024;
	ckptCreateAttr.retentionDuration= 100000;
	ckptCreateAttr.maxSections= 2;
	ckptCreateAttr.maxSectionSize = 700;
	ckptCreateAttr.maxSectionIdSize = 4;


	ckptOpenFlags = SA_CKPT_CHECKPOINT_CREATE|SA_CKPT_CHECKPOINT_READ|SA_CKPT_CHECKPOINT_WRITE;
	printf("Opening Collocated Checkpoint = %s with create flags....\n",ckptName.value);
	rc = saCkptCheckpointOpen(ckptHandle,&ckptName,&ckptCreateAttr,ckptOpenFlags,timeout,&checkpointHandle);
	if(rc != SA_AIS_OK)
		printf(" saCkptCheckpointOpen Failed \n");



	if(temp_var == 1)
	{

		printf("Setting the Active Replica for my checkpoint ....\n");
		rc = saCkptActiveReplicaSet(checkpointHandle);
		if(rc != SA_AIS_OK)
			printf("saCkptActiveReplicaSet Failed \n");


		sectionCreationAttributes.sectionId = (SaCkptSectionIdT*) malloc(sizeof \
				(SaCkptSectionIdT));
		sectionCreationAttributes.sectionId->id = (unsigned char *)"11";
		sectionCreationAttributes.sectionId->idLen = 2;
		/* Cpsv expects `expirationTime` as  absolute time
		   check  section 3.4.3.2 SaCkptSectionCreationAttributesT 
		   of CKPT Specification for more details  */
		sectionCreationAttributes.expirationTime =
			(SA_TIME_ONE_HOUR +( time((time_t*)0) * 1000000000));   /* One Hour */

		printf("Created Section ....\n");
		rc = saCkptSectionCreate(checkpointHandle,&sectionCreationAttributes,initialData,28);
		if(rc != SA_AIS_OK)
			printf("saCkptSectionCreate Failed \n");

		printf("Press <Enter> key to Writing to Checkpoint ...\n");
		getchar();

		writeVector.sectionId.id = (unsigned char *)"11";
		writeVector.sectionId.idLen = 2;
		writeVector.dataBuffer = "************ This is the saCkptCheckpointTrackCallback demo ***********";
		writeVector.dataSize = strlen(writeVector.dataBuffer);
		writeVector.dataOffset = 0;
		writeVector.readSize = 0;

		printf("Writing to Checkpoint %s ....\n",DEMO_CKPT_NAME);
		printf("Section-Id = %s ....\n",writeVector.sectionId.id);
		printf("CheckpointData being written = \"%s\"\n",(char *)writeVector.dataBuffer);
		printf("DataOffset = %llu ....\n",writeVector.dataOffset);
		rc = saCkptCheckpointWrite(checkpointHandle,&writeVector,1,&erroneousVectorIndex);
		if(rc == SA_AIS_OK)
			printf("saCkptCheckpointWrite PASSED \n");
		else
			printf("saCkptCheckpointWrite Failed rc = %d\n", rc);

		printf("Press <Enter> key to  Close Checkpoint ...\n");
		getchar();

	}
	else
	{ 
		fd_set read_fd;
		struct timeval tv;
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		SaSelectionObjectT selobj;

		printf("saCkptTrack being enabled ....\n");
		rc = saCkptTrack(ckptHandle);
		if(rc != SA_AIS_OK)
		{
			printf("saCkptTrack Failed  with code %d.\n", rc);

		} 
		printf("Checkpoint being on Select ..... \n");
		rc = saCkptSelectionObjectGet(ckptHandle, &selobj);
		FD_ZERO(&read_fd);
		FD_SET(selobj, &read_fd);
		rc = select(selobj + 1, &read_fd, NULL, NULL, &tv);
		if(rc == 1)
			rc = saCkptDispatch(ckptHandle,SA_DISPATCH_ONE);
	}

	rc = saCkptCheckpointSynchronize(checkpointHandle,timeout);
	if(rc != SA_AIS_OK)
		printf(" saCkptCheckpointSynchronize Failed \n");

	if(temp_var==1)
	{
		rc = saCkptCheckpointUnlink(ckptHandle,&ckptName);
		if(rc != SA_AIS_OK)
			printf(" saCkptCheckpointUnlink Failed \n");
	} else { 

		printf("saCkptTrackStop ....\n");
		rc = saCkptTrackStop(ckptHandle);
		if(rc != SA_AIS_OK)
			printf("saCkptTrackStop Failed  with code %d.\n", rc);
	}
	printf("Ckpt Closed ....\n");
	rc = saCkptCheckpointClose(checkpointHandle);
	if(rc != SA_AIS_OK)
		printf("saCkptCheckpointClose Failed \n");


	printf("Ckpt Finalize being called ....\n");
	rc = saCkptFinalize(ckptHandle);
	if(rc != SA_AIS_OK)
		printf("saCkptFinalize Failed \n");

	return;
}


