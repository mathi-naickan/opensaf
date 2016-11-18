/*	  -*- OpenSAF  -*-
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
 * Author(s): Goahead 
 */

/* 
 * DESCRIPTION:
 *   This file provides the suggested additions to the C language binding for
 *   the Service Availability(TM) Forum Information Model Management Service (CKPT).
 *   It contains only the prototypes and type definitions that are part of this
 *   proposed addition. 
 *   These additions are currently NON STANDARD. But the intention is to get these
 *   additions approved formally by SAF in the future.
 *
 */

#ifndef _SA_CKPT_B_02_03_H
#define _SA_CKPT_B_02_03_H

#include "saAis.h"

#ifdef  __cplusplus
extern "C" {
#endif

	/***************************************************************************
	@brief    : The Checkpoint Service invokes this callback function when the
			ckpt write, ckpt section create and ckpt section delete happens
			for the ALL the Checkpoints which are opened with CHECKPOINT_WRITE
			Open Flags by the ckptHandle invoking process .
	@param[in]  : checkpointHandle - A pointer to the checkpoint handle which was
			 obtained by saCkptCheckpointOpen.
	@param[out] : ioVector - A pointer to an array that contains elements ioVector[0],
	@param[out] : numberOfElements - Size of the ioVector.
	****************************************************************************/
	typedef void
		(*SaCkptCheckpointTrackCallbackT) (SaCkptCheckpointHandleT checkpointHandle,
				SaCkptIOVectorElementT *ioVector,
				SaUint32T numberOfElements);

	/***************************************************************************
	@brief   :  The callbacks structure is supplied to the Checkpoint Service by a
			process and contains the callback functions that the
			Checkpoint Service can invoke.
	*****************************************************************************/
	typedef struct {
		SaCkptCheckpointOpenCallbackT saCkptCheckpointOpenCallback;
		SaCkptCheckpointSynchronizeCallbackT saCkptCheckpointSynchronizeCallback;
		SaCkptCheckpointTrackCallbackT saCkptCheckpointTrackCallback;
	} SaCkptCallbacksT_2;

	/***************************************************************************
	@brief	    : saCkptInitialize_2 function initializes the Checkpoint Service
			for the invoking process and registers the various callback functions.
			The handle 'ckptHandle' is returned as the reference to this
			association between the process and the Checkpoint Service..
	@param[out] : ckptHandle - A pointer to the handle designating this
			particular initialization of the Checkpoint
			service that it to be returned by the Checkpoint Service.
	@param[in]  : ckptCallbacks-Pointer to a SaCkptCallbacksT_2 structure,
			containing the callback functions of the process
			that the Checkpoint Service may invoke.
	@param[in]  : version - Is a pointer to the version of the Checkpoint
			Service that the invoking process is using.
	@return	 : SA_AIS_OK if successful otherwise appropriate err code,
			Refer to SAI-AIS specification for various return values.
	*****************************************************************************/
	extern SaAisErrorT
		saCkptInitialize_2(SaCkptHandleT *ckptHandle, const SaCkptCallbacksT_2 *callbacks, SaVersionT *version);

	/***************************************************************************
	@brief	  : saCkptTrack function enable/starts the Ckpt Track callback
			function (saCkptCheckpointTrackCallback) which was registered
			at the time of initializes for the ckptHandle invoking process.
			The registered Track Callback function will be invoked whenever
			ckpt write, ckpt section create and ckpt section delete happens
			for the ALL the Checkpoints which are opened with CHECKPOINT_WRITE
			Open Flags by the ckptHandle invoking process .
	@param[in]  : ckptHandle - A pointer to the handle designating this
			particular initialization of the Checkpoint
			service which was returned at the time of initialize().
	@return	 : SA_AIS_OK if successful otherwise appropriate err code,
			Refer to SAI-AIS specification for various return values.
	*****************************************************************************/
	extern SaAisErrorT 
		saCkptTrack(SaCkptHandleT ckptHandle);

	/***************************************************************************
	@brief	  : saCkptTrackStop function disables/stops the Ckpt Track callback
			function (saCkptCheckpointTrackCallback) which was registered at
			the time of initializes for the ckptHandle invoking process.
			The registered Track Callback function will be disabled for
			ckpt write, ckpt section create and ckpt section delete happens
			for the ALL the Checkpoints which are opened with CHECKPOINT_WRITE
			Open Flags by the ckptHandle invoking process .
	@param[in]  : ckptHandle - A pointer to the handle designating this
			particular initialization of the Checkpoint
			service which was returned at the time of initialize().
	@return	 : SA_AIS_OK if successful otherwise appropriate err code ,
			Refer to SAI-AIS specification for various return values.
	*****************************************************************************/
	extern SaAisErrorT
		saCkptTrackStop(SaCkptHandleT ckptHandle);

	/***************************************************************************
	@brief	  : saCkptCheckpointTrack function enable/starts the Ckpt Track
			callback function (saCkptCheckpointTrackCallback) which was
			registered at the time of initializes for a particular checkpointHandle.
			The registered Track Callback function will be invoked 
			whenever ckpt write, ckpt section create and ckpt section
			delete happens for the Checkpoints which are opened with
			CHECKPOINT_WRITE Open Flags by a particular checkpointHandle.
	@param[in]  : checkpointHandle -A pointer to the checkpoint handle
			which was obtained by saCkptCheckpointOpen
	@return	 : SA_AIS_OK if successful otherwise appropriate err code,
			Refer to SAI-AIS specification for various return values.
	*****************************************************************************/
	extern SaAisErrorT
		saCkptCheckpointTrack(SaCkptCheckpointHandleT checkpointHandle);

	/***************************************************************************
	@brief	  : saCkptCheckpointTrack function disables/stops the Ckpt Track
			callback function (saCkptCheckpointTrackCallback) which was
			registered at the time of initializes for a particular checkpointHandle.
			The registered Track Callback function will be disabled for
			ckpt write, ckpt section create and ckpt section delete happens
			for the Checkpoints which are opened with CHECKPOINT_WRITE
			Open Flags by a particular checkpointHandle .
	@param[in]  : checkpointHandle -A pointer to the checkpoint handle which
			was obtained by saCkptCheckpointOpen
	@return	 : SA_AIS_OK if successful otherwise appropriate err code,
			Refer to SAI-AIS specification for various return values.
	*****************************************************************************/
	extern SaAisErrorT 
		saCkptCheckpointTrackStop(SaCkptCheckpointHandleT checkpointHandle);

#ifdef  __cplusplus
}
#endif

#endif   /* _SA_CKPT_B_02_03_H */
