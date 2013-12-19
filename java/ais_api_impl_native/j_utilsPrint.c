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
 * Author(s): Nokia Siemens Networks, OptXware Research & Development LLC.
 */

/**************************************************************************
 * DESCRIPTION:
 * This file defines printing utility functions for AIS data structures
 * TODO add a bit more on this...
 *
 *************************************************************************/

/**************************************************************************
 * Include files
 *************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <errno.h>
#include <saAis.h>
#include <saClm.h>
#include <saCkpt.h>
#include <saAmf.h>
#include "j_utils.h"
#include "j_utilsPrint.h"
#include "tracer.h"

/**************************************************************************
 * Constants
 *************************************************************************/

/**************************************************************************
 * Macros
 *************************************************************************/

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable definitions
 *************************************************************************/

/**************************************************************************
 * Function declarations
 *************************************************************************/

/**************************************************************************
 * Function definitions
 *************************************************************************/


/**************************************************************************
 * FUNCTION:      U_printSaName
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaName(
    const char* msg,
    const SaNameT* saNamePtr )
{
    _TRACE_INLINE( "%s", msg );
    if( saNamePtr == NULL ){
        _TRACE2( "NULL STRING \n" );
        return; // EXIT POINT!!!
    }

    _TRACE_INLINE("%.*s (length:%d)\n", saNamePtr->length, saNamePtr->value, saNamePtr->length);
}

/**************************************************************************
 * FUNCTION:      U_printSaSelectionObjectInfo
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaSelectionObjectInfo(
    const char* msg,
    const SaSelectionObjectT saSelectionObject )
{
    // VARIABLES
    struct stat _selectionObjectStat;

    // BODY
    _TRACE_INLINE( "%s", msg );
    if( fstat( (int) saSelectionObject, &_selectionObjectStat ) < 0 ){
        _TRACE2( "fstat ERROR: negative return value...\n" );
        return; // EXIT POINT
    }
    // file type
    if( S_ISREG( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tregular file\n" );
    }
    else if( S_ISDIR( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tdirectory\n" );
    }
    else if( S_ISCHR( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tcharacter special file\n" );
        // device number
        _TRACE2( "\tfile system major: %d \n", major(_selectionObjectStat.st_rdev) );
        _TRACE2( "\tfile system minor: %d \n", minor(_selectionObjectStat.st_rdev) );
    }
    else if( S_ISBLK( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tblock special file\n" );
        // device number
        _TRACE2( "\tfile system major: %d \n", major(_selectionObjectStat.st_rdev) );
        _TRACE2( "\tfile system minor: %d \n", minor(_selectionObjectStat.st_rdev) );
    }
    else if( S_ISFIFO( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tFIFO (pipe)\n" );
    }
#ifdef S_ISLNK
    else if( S_ISLNK( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tsymbolic link\n" );
    }
#endif // S_ISLNK
#ifdef S_ISSOCK
    else if( S_ISSOCK( _selectionObjectStat.st_mode ) ){
        _TRACE2( "\tsocket\n" );
    }
#endif // S_ISSOCK
    else{
        _TRACE2( "fstat ERROR: unknown type...\n" );
    }
    // i-node
    _TRACE2( "\ti-node number: %lu \n", (unsigned long int)_selectionObjectStat.st_ino );
    // file system device number
    _TRACE2( "\tfile system major: %d \n", major(_selectionObjectStat.st_dev) );
    _TRACE2( "\tfile system minor: %d \n", minor(_selectionObjectStat.st_dev) );

}


// CLM
/**************************************************************************
 * FUNCTION:      U_printSaClusterNotificationBuffer
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNotificationBuffer(
    const char* msg,
    const SaClmClusterNotificationBufferT* saClusterNotificationBufferPtr )
{
    // VARIABLES
    int _idx;

    // BODY
    _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \tAddress of saClusterNotificationBuffer:%p:\n", saClusterNotificationBufferPtr );
    _TRACE2( "NATIVE: \t viewNumber:%lu\n", (unsigned long) saClusterNotificationBufferPtr->viewNumber );
    _TRACE2( "NATIVE: \t numberOfItems:%u\n", (unsigned int) saClusterNotificationBufferPtr->numberOfItems );
    _TRACE2( "NATIVE: \t notification: %p\n", saClusterNotificationBufferPtr->notification );
    // TODO should be some sensible error handling for invalid numberOfItems values...
    for( _idx = 0; _idx < saClusterNotificationBufferPtr->numberOfItems; _idx++ ){
        _TRACE2( "NATIVE: \t  notification[%d]:\n", _idx );
        U_printSaClusterNotification( "", &(saClusterNotificationBufferPtr->notification[_idx]) );
    }

}

/**************************************************************************
 * FUNCTION:      U_printSaClusterNotificationBuffer_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNotificationBuffer_4(
    const char* msg,
    const SaClmClusterNotificationBufferT_4* saClusterNotificationBufferPtr )
{
    // VARIABLES
    int _idx;

    // BODY
    _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \tAddress of saClusterNotificationBuffer:%p:\n", saClusterNotificationBufferPtr );
    _TRACE2( "NATIVE: \t viewNumber:%lu\n", (unsigned long) saClusterNotificationBufferPtr->viewNumber );
    _TRACE2( "NATIVE: \t numberOfItems:%u\n", (unsigned int) saClusterNotificationBufferPtr->numberOfItems );
    _TRACE2( "NATIVE: \t notification: %p\n", saClusterNotificationBufferPtr->notification );
    // TODO should be some sensible error handling for invalid numberOfItems values...
    for( _idx = 0; _idx < saClusterNotificationBufferPtr->numberOfItems; _idx++ ){
        _TRACE2( "NATIVE: \t  notification[%d]:\n", _idx );
        U_printSaClusterNotification_4( "", &(saClusterNotificationBufferPtr->notification[_idx]) );
    }

}

/**************************************************************************
 * FUNCTION:      U_printSaClusterNotification
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNotification(
    const char* msg,
    const SaClmClusterNotificationT* saClusterNotificationPtr )
{
    // BODY
        _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t(Address of saClusterNotification:%p:)\n", saClusterNotificationPtr );
    _TRACE2( "NATIVE: \t\t clusterChange:%u\n", saClusterNotificationPtr->clusterChange );
    U_printSaClusterNode( "NATIVE: \t\t clusterNode\n", &(saClusterNotificationPtr->clusterNode) );
}

/**************************************************************************
 * FUNCTION:      U_printSaClusterNotification_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNotification_4(
    const char* msg,
    const SaClmClusterNotificationT_4* saClusterNotificationPtr )
{
    // BODY
    _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t(Address of saClusterNotification:%p:)\n", saClusterNotificationPtr );
    _TRACE2( "NATIVE: \t\t clusterChange:%u\n", saClusterNotificationPtr->clusterChange );
    U_printSaClusterNode_4( "NATIVE: \t\t clusterNode\n", &(saClusterNotificationPtr->clusterNode) );
}

/**************************************************************************
 * FUNCTION:      U_printSaClusterNode
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNode(
    const char* msg,
    const SaClmClusterNodeT* saClusterNodePtr )
{
    // BODY
    _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t\tnodeId:%u\n", (unsigned int) saClusterNodePtr->nodeId );
    U_printSaNodeAddress( "NATIVE: \t\t\tnodeAddress: ", &(saClusterNodePtr->nodeAddress) );
    U_printSaName( "NATIVE: \t\t\tnodeName: ", &(saClusterNodePtr->nodeName) );
    _TRACE2( "NATIVE: \t\t\tmember: %d\n", saClusterNodePtr->member );
    _TRACE2( "NATIVE: \t\t\tbootTimestamp:%lu\n", (unsigned long) saClusterNodePtr->bootTimestamp );
    _TRACE2( "NATIVE: \t\t\tinitialViewNumber:%lu\n", (unsigned long) saClusterNodePtr->initialViewNumber );
}

/**************************************************************************
 * FUNCTION:      U_printSaClusterNode_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaClusterNode_4(
    const char* msg,
    const SaClmClusterNodeT_4* saClusterNodePtr )
{
    // BODY
    _TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t\tnodeId:%u\n", (unsigned int) saClusterNodePtr->nodeId );
    U_printSaNodeAddress( "NATIVE: \t\t\tnodeAddress: ", &(saClusterNodePtr->nodeAddress) );
    U_printSaName( "NATIVE: \t\t\tnodeName: ", &(saClusterNodePtr->nodeName) );
    _TRACE2( "NATIVE: \t\t\tmember: %d\n", saClusterNodePtr->member );
    _TRACE2( "NATIVE: \t\t\tbootTimestamp:%lu\n", (unsigned long) saClusterNodePtr->bootTimestamp );
    _TRACE2( "NATIVE: \t\t\tinitialViewNumber:%lu\n", (unsigned long) saClusterNodePtr->initialViewNumber );
}

/**************************************************************************
 * FUNCTION:      U_printSaNodeAddress
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaNodeAddress(
    const char* msg,
    const SaClmNodeAddressT* saClmNodeAddressPtr )
{
    // BODY
    _TRACE2( "%s", msg );
    if( saClmNodeAddressPtr->length > SA_CLM_MAX_ADDRESS_LENGTH ){
        _TRACE_INLINE( "INVALID STRING (length: %d)\n", saClmNodeAddressPtr->length );
    }
    else{
        SaUint8T _eValue[SA_CLM_MAX_ADDRESS_LENGTH+1];
        memcpy( _eValue,
                saClmNodeAddressPtr->value,
                saClmNodeAddressPtr->length );
        _eValue[saClmNodeAddressPtr->length] = 0x00;
        _TRACE_INLINE( "%s (length: %d)\n", _eValue, saClmNodeAddressPtr->length );
    }
}

// CKPT

/**************************************************************************
 * FUNCTION:      U_printSaCheckpointDescriptor
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaCheckpointDescriptor(
    const char* msg,
    const SaCkptCheckpointDescriptorT* saCheckpointDescriptorPtr )
{
    // BODY
	_TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t(Address of saCheckpointDescriptor:%p:)\n", saCheckpointDescriptorPtr );
    _TRACE2( "NATIVE: \t\t numberOfSections:%u\n", (unsigned int) saCheckpointDescriptorPtr->numberOfSections );
    _TRACE2( "NATIVE: \t\t memoryUsed:%lu\n", (unsigned long) saCheckpointDescriptorPtr->memoryUsed );
    U_printSaCheckpointCreationAttributes( "NATIVE: \t\t checkpointCreationAttributes\n", &(saCheckpointDescriptorPtr->checkpointCreationAttributes ) );
}

/**************************************************************************
 * FUNCTION:      U_printSaCheckpointCreationAttributes
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaCheckpointCreationAttributes(
    const char* msg,
    const SaCkptCheckpointCreationAttributesT* saCheckpointCreationAttributesPtr )
{
    // BODY
	_TRACE_INLINE( "%s", msg );
    _TRACE2( "NATIVE: \t\t\tcreationFlags:%u\n", (unsigned int) saCheckpointCreationAttributesPtr->creationFlags );
    _TRACE2( "NATIVE: \t\t\tcheckpointSize: %lu\n", (unsigned long) saCheckpointCreationAttributesPtr->checkpointSize );
    _TRACE2( "NATIVE: \t\t\tretentionDuration: %lu\n", (unsigned long) saCheckpointCreationAttributesPtr->retentionDuration );
    _TRACE2( "NATIVE: \t\t\tmaxSections:%u\n", (unsigned int) saCheckpointCreationAttributesPtr->maxSections );
    _TRACE2( "NATIVE: \t\t\tmaxSectionSize: %lu\n", (unsigned long) saCheckpointCreationAttributesPtr->maxSectionSize );
    _TRACE2( "NATIVE: \t\t\tmaxSectionIdSize: %lu\n", (unsigned long) saCheckpointCreationAttributesPtr->maxSectionIdSize );
}

// AMF

/**************************************************************************
 * FUNCTION:      U_printSaAmfHAState
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfHAState(
    const char* msg,
    const SaAmfHAStateT haState )
{
	_TRACE_INLINE( "%s", msg );
    switch( haState ){
        case SA_AMF_HA_ACTIVE:
            _TRACE2( "HA ACTIVE (%d)\n", haState );
            break;
        case SA_AMF_HA_STANDBY:
            _TRACE2( "HA STANDBY (%d)\n", haState );
            break;
        case SA_AMF_HA_QUIESCED:
            _TRACE2( "HA QUIESCED (%d)\n", haState );
            break;
        case SA_AMF_HA_QUIESCING:
            _TRACE2( "HA QUIESCING (%d)\n", haState );
            break;
        default:
            _TRACE2( "HA INVALID!!! (%d)\n", haState );
            break;
    }
}

/**************************************************************************
 * FUNCTION:      U_printSaAmfCSIDescriptor
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfCSIDescriptor(
    const char* msg,
    const SaAmfHAStateT haState,
    const SaAmfCSIDescriptorT* csiDescriptorPtr )
{
	_TRACE_INLINE( "%s", msg );

    U_printSaAmfCSIFlags( "\tcsiFlags: ", csiDescriptorPtr->csiFlags );
    if( csiDescriptorPtr->csiFlags != SA_AMF_CSI_TARGET_ALL ){
        U_printSaName( "\tcsiName: ", &csiDescriptorPtr->csiName );
    }
    if( haState == SA_AMF_HA_ACTIVE ){
        U_printSaAmfCSIActiveDescriptor( "\tActive csiStateDescriptor: ",
                                         &(csiDescriptorPtr->csiStateDescriptor.activeDescriptor) );
    }
    if( haState == SA_AMF_HA_STANDBY ){
        U_printSaAmfCSIStandbyDescriptor( "\tStandby csiStateDescriptor: ",
                                          &(csiDescriptorPtr->csiStateDescriptor.standbyDescriptor) );
    }
    if( csiDescriptorPtr->csiFlags == SA_AMF_CSI_ADD_ONE ){
        U_printSaAmfCSIAttributeList( "\tcsi attributes: ", &(csiDescriptorPtr->csiAttr) );
    }

}

/**************************************************************************
 * FUNCTION:      U_printSaAmfCSIFlags
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfCSIFlags(
    const char* msg,
    const SaAmfCSIFlagsT csiFlags )
{
	_TRACE_INLINE( "%s", msg );
    switch( csiFlags ){
        case SA_AMF_CSI_ADD_ONE:
            _TRACE2( "CSI ADD ONE (%u)\n", (unsigned int) csiFlags );
            break;
        case SA_AMF_CSI_TARGET_ONE:
            _TRACE2( "CSI TARGET ONE (%u)\n", (unsigned int) csiFlags );
            break;
        case SA_AMF_CSI_TARGET_ALL:
            _TRACE2( "CSI TARGET ALL (%u)\n", (unsigned int) csiFlags );
            break;
        default:
            _TRACE2( "CSI INVALID!!! (%u)\n", (unsigned int) csiFlags );
            break;
    }
}

/**************************************************************************
 * FUNCTION:      U_printSaAmfCSIActiveDescriptor
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfCSIActiveDescriptor(
    const char* msg,
    const SaAmfCSIActiveDescriptorT* activeDescriptorPtr )
{
	_TRACE_INLINE( "%s", msg );

    _TRACE2( "\t\ttransitionDescriptor: " );
    switch( activeDescriptorPtr->transitionDescriptor ){
        case SA_AMF_CSI_NEW_ASSIGN:
            _TRACE2( "CSI NEW ASSIGN (%d)\n", activeDescriptorPtr->transitionDescriptor );
            break;
        case SA_AMF_CSI_QUIESCED:
            _TRACE2( "CSI QUIESCED (%d)\n", activeDescriptorPtr->transitionDescriptor );
            break;
        case SA_AMF_CSI_NOT_QUIESCED:
            _TRACE2( "CSI NOT QUIESCED (%d)\n", activeDescriptorPtr->transitionDescriptor );
            break;
        case SA_AMF_CSI_STILL_ACTIVE:
            _TRACE2( "CSI STILL ACTIVE (%d)\n", activeDescriptorPtr->transitionDescriptor );
            break;
        default:
            _TRACE2( "CSI INVALID!!! (%d)\n", activeDescriptorPtr->transitionDescriptor );
            break;
    }
    U_printSaName( "\t\tactiveCompName: ", &activeDescriptorPtr->activeCompName );
}

/**************************************************************************
 * FUNCTION:      U_printSaAmfCSIStandbyDescriptor
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfCSIStandbyDescriptor(
    const char* msg,
    const SaAmfCSIStandbyDescriptorT* standbyDescriptorPtr )
{
	_TRACE_INLINE( "%s", msg );
    U_printSaName( "\t\tactiveCompName: ", &standbyDescriptorPtr->activeCompName );
    _TRACE2( "\t\tstandbyRank: %u\n", (unsigned int) standbyDescriptorPtr->standbyRank );
}

/**************************************************************************
 * FUNCTION:      U_printSaAmfCSIAttributeList
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_printSaAmfCSIAttributeList(
    const char* msg,
    const SaAmfCSIAttributeListT* csiAttrListPtr )
{
    unsigned int _idx;

    _TRACE_INLINE( "%s (attr pointer: %p), (length: %u)\n",
            msg,
            (void*) csiAttrListPtr->attr,
            (unsigned int) csiAttrListPtr->number );
    // some error handling
    if( csiAttrListPtr->attr  == NULL ){
        _TRACE2( "WARNING: Pointer to attribute list NULL!!!\n" );
    	return;
    }
    if( csiAttrListPtr->number > 32 ){
        _TRACE2( "WARNING: attribute list seems to be too long!!!\n" );
    	return;
    }
    // print the attributes
    for( _idx = 0; _idx < csiAttrListPtr->number; _idx++ ){
        _TRACE2( "\t\tattribute[%u]: {%s/%s}\n",
                _idx,
                csiAttrListPtr->attr[_idx].attrName,
                csiAttrListPtr->attr[_idx].attrValue );
    }


}

