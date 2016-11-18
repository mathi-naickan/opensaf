/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 *
 */

/*****************************************************************************
  DESCRIPTION:
  This file contains HPI IDR  definitions used by PLMS
******************************************************************************/

#ifndef PLM_PLMS_HPI_H_
#define PLM_PLMS_HPI_H_

#include <SaHpi.h>

typedef struct
{
	SaUint8T    chassis_area_offset;
	SaUint8T    board_area_offset;
	SaUint8T    product_area_offset;
} PLMS_IDR_HEADER; 

typedef struct
{
	SaUint8T  format_version;
	SaHpiTextBufferT manufacturer_name;
	SaHpiTextBufferT  product_name;
	SaHpiTextBufferT serial_no;
	SaHpiTextBufferT part_no;
	SaHpiTextBufferT product_version;
	SaHpiTextBufferT asset_tag;
	SaHpiTextBufferT fru_field_id;

} PLMS_PRODUCT_INFO_AREA ;

typedef struct
{
	SaUint8T  version;
	SaHpiTextBufferT manufacturer_name;
	SaHpiTextBufferT  product_name;
	SaHpiTextBufferT  serial_no;
	SaHpiTextBufferT  part_no;
	SaHpiTextBufferT fru_field_id;
} PLMS_BOARD_INFO_AREA;	


typedef struct
{
	SaUint8T  format_version;
	SaUint8T  chassis_type;
	SaHpiTextBufferT  serial_no;
	SaHpiTextBufferT  part_no;
} PLMS_CHASSIS_INFO_AREA;	
 
typedef struct {
	PLMS_IDR_HEADER            hdr;
	PLMS_CHASSIS_INFO_AREA     chassis_area;
	PLMS_BOARD_INFO_AREA       board_area;
	PLMS_PRODUCT_INFO_AREA     product_area;
}PLMS_INV_DATA;

#endif  // PLM_PLMS_HPI_H_
