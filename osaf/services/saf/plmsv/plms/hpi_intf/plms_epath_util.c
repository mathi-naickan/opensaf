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
*                                                                            *
*  MODULE NAME:  plms_epath_util.c                                           *
*                                                                            *
*                                                                            *
*  DESCRIPTION: This module contains the utility routines to convert         *
*               entity_path to string format and vice_versa 		     *
*                                                                            *
*****************************************************************************/

/***********************************************************************
 *   INCLUDE FILES
***********************************************************************/

#include "plms.h"
#include "plms_evt.h"


/* macro definitions */
#define EPATH_BEGIN_CHAR          '{'
#define EPATH_END_CHAR            '}'
#define EPATH_DOT_SEPARATOR_CHAR        '.'
#define EPATH_COMMA_SEPARATOR_CHAR      ','

#define SAHPI_ENT_RACK_INDEX 59 
#define SAHPI_ENT_AMC_INDEX 84

/* list of entity path types */
typedef struct
{
   SaInt8T	      *etype_str;
   SaHpiEntityTypeT   etype_val;
}PLMS_ENTITY_TYPE_LIST;


/***********************************************************************
*   FUNCTION PROTOTYPES
***********************************************************************/
SaUint32T convert_entitypath_to_string(SaHpiEntityPathT *entity_path,
					SaInt8T **ent_path_str);
SaUint32T convert_string_to_epath(SaInt8T *epath_str,
                                           SaHpiEntityPathT *epath_ptr);
static void remove_spaces(SaInt8T **tok);

static SaUint32T convert_entity_types(SaHpiEntityPathT *entity_path, 
				SaInt8T *ent_path_str,
				SaUint32T  index_array[SAHPI_MAX_ENTITY_PATH]);

/* HPI entity type list */
static PLMS_ENTITY_TYPE_LIST  hpi_ent_type_list[] = {
	/* Entity type string */                /* Entity type value */
	{"UNSPECIFIED",                SAHPI_ENT_UNSPECIFIED},
	{"OTHER",                      SAHPI_ENT_OTHER},
	{"UNKNOWN",                    SAHPI_ENT_UNKNOWN},
	{"PROCESSOR",                  SAHPI_ENT_PROCESSOR},
	{"DISK_BAY",                   SAHPI_ENT_DISK_BAY},
	{"PERIPHERAL_BAY",             SAHPI_ENT_PERIPHERAL_BAY},
	{"SYS_MGMNT_MODULE",           SAHPI_ENT_SYS_MGMNT_MODULE},
	{"SYSTEM_BOARD",               SAHPI_ENT_SYSTEM_BOARD},
	{"MEMORY_MODULE",              SAHPI_ENT_MEMORY_MODULE},
	{"PROCESSOR_MODULE",           SAHPI_ENT_PROCESSOR_MODULE},

	{"POWER_SUPPLY",               SAHPI_ENT_POWER_SUPPLY},
	{"ADD_IN_CARD",                SAHPI_ENT_ADD_IN_CARD},
	{"FRONT_PANEL_BOARD",          SAHPI_ENT_FRONT_PANEL_BOARD},
	{"BACK_PANEL_BOARD",           SAHPI_ENT_BACK_PANEL_BOARD},
	{"POWER_SYSTEM_BOARD",         SAHPI_ENT_POWER_SYSTEM_BOARD},
	{"DRIVE_BACKPLANE",            SAHPI_ENT_DRIVE_BACKPLANE},
	{"SYS_EXPANSION_BOARD",        SAHPI_ENT_SYS_EXPANSION_BOARD},
	{"OTHER_SYSTEM_BOARD",         SAHPI_ENT_OTHER_SYSTEM_BOARD},
	{"PROCESSOR_BOARD",            SAHPI_ENT_PROCESSOR_BOARD},
	{"POWER_UNIT",                 SAHPI_ENT_POWER_UNIT},

	{"POWER_MODULE",               SAHPI_ENT_POWER_MODULE},
	{"POWER_MGMNT",                SAHPI_ENT_POWER_MGMNT},
	{"CHASSIS_BACK_PANEL_BOARD",   SAHPI_ENT_CHASSIS_BACK_PANEL_BOARD},
	{"SYSTEM_CHASSIS",             SAHPI_ENT_SYSTEM_CHASSIS},
	{"SUB_CHASSIS",                SAHPI_ENT_SUB_CHASSIS},
	{"OTHER_CHASSIS_BOARD",        SAHPI_ENT_OTHER_CHASSIS_BOARD},
	{"DISK_DRIVE_BAY",             SAHPI_ENT_DISK_DRIVE_BAY},
	{"PERIPHERAL_BAY_2",           SAHPI_ENT_PERIPHERAL_BAY_2},
	{"DEVICE_BAY",                 SAHPI_ENT_DEVICE_BAY},
	{"COOLING_DEVICE",             SAHPI_ENT_COOLING_DEVICE},

	{"COOLING_UNIT",               SAHPI_ENT_COOLING_UNIT},
	{"INTERCONNECT",               SAHPI_ENT_INTERCONNECT},
	{"MEMORY_DEVICE",              SAHPI_ENT_MEMORY_DEVICE},
	{"SYS_MGMNT_SOFTWARE",         SAHPI_ENT_SYS_MGMNT_SOFTWARE},
	{"BIOS",                       SAHPI_ENT_BIOS},
	{"OPERATING_SYSTEM",           SAHPI_ENT_OPERATING_SYSTEM},
	{"SYSTEM_BUS",                 SAHPI_ENT_SYSTEM_BUS},
	{"GROUP",                      SAHPI_ENT_GROUP},
	{"REMOTE",                     SAHPI_ENT_REMOTE},
	{"EXTERNAL_ENVIRONMENT",       SAHPI_ENT_EXTERNAL_ENVIRONMENT},
	{"BATTERY",                    SAHPI_ENT_BATTERY},

	{"RESERVED_1",                 SAHPI_ENT_RESERVED_1},
	{"RESERVED_2",                 SAHPI_ENT_RESERVED_2},
	{"RESERVED_3",                 SAHPI_ENT_RESERVED_3},
	{"RESERVED_4",                 SAHPI_ENT_RESERVED_4},
	{"RESERVED_5",                 SAHPI_ENT_RESERVED_5},
	{"MC_FIRMWARE",                SAHPI_ENT_MC_FIRMWARE},
	{"IPMI_CHANNEL",               SAHPI_ENT_IPMI_CHANNEL},
	{"PCI_BUS",                    SAHPI_ENT_PCI_BUS},
	{"PCI_EXPRESS_BUS",            SAHPI_ENT_PCI_EXPRESS_BUS},

	{"SCSI_BUS",                   SAHPI_ENT_SCSI_BUS},
	{"SATA_BUS",                   SAHPI_ENT_SATA_BUS},
	{"PROC_FSB",                   SAHPI_ENT_PROC_FSB},
	{"CLOCK",                      SAHPI_ENT_CLOCK},
	{"SYSTEM_FIRMWARE",            SAHPI_ENT_SYSTEM_FIRMWARE},


	{"CHASSIS_SPECIFIC",           SAHPI_ENT_CHASSIS_SPECIFIC},
	{"BOARD_SET_SPECIFIC",         SAHPI_ENT_BOARD_SET_SPECIFIC},
	{"OEM_SYSINT_SPECIFIC",        SAHPI_ENT_OEM_SYSINT_SPECIFIC},
	{"ROOT",                       SAHPI_ENT_ROOT},
	{"RACK",                       SAHPI_ENT_RACK},

	{"SUBRACK",                    SAHPI_ENT_SUBRACK},
	{"COMPACTPCI_CHASSIS",         SAHPI_ENT_COMPACTPCI_CHASSIS},
	{"ADVANCEDTCA_CHASSIS",        SAHPI_ENT_ADVANCEDTCA_CHASSIS},
	{"RACK_MOUNTED_SERVER",        SAHPI_ENT_RACK_MOUNTED_SERVER},
	{"SYSTEM_BLADE",               SAHPI_ENT_SYSTEM_BLADE},
	{"SWITCH",                     SAHPI_ENT_SWITCH},
	{"SWITCH_BLADE",               SAHPI_ENT_SWITCH_BLADE},
	{"SBC_BLADE",                  SAHPI_ENT_SBC_BLADE},
	{"IO_BLADE",                   SAHPI_ENT_IO_BLADE},
	{"DISK_DRIVE",                 SAHPI_ENT_DISK_DRIVE},
	{"FAN",                        SAHPI_ENT_FAN},
	{"POWER_DISTRIBUTION_UNIT",    SAHPI_ENT_POWER_DISTRIBUTION_UNIT},
	{"SPEC_PROC_BLADE",            SAHPI_ENT_SPEC_PROC_BLADE},
	{"IO_SUBBOARD",                SAHPI_ENT_IO_SUBBOARD},
	{"SBC_SUBBOARD",               SAHPI_ENT_SBC_SUBBOARD},
	{"ALARM_MANAGER",              SAHPI_ENT_ALARM_MANAGER},
	{"SHELF_MANAGER",              SAHPI_ENT_SHELF_MANAGER},
	{"DISPLAY_PANEL",              SAHPI_ENT_DISPLAY_PANEL},
	{"SUBBOARD_CARRIER_BLADE",     SAHPI_ENT_SUBBOARD_CARRIER_BLADE},

	{"PHYSICAL_SLOT",              SAHPI_ENT_PHYSICAL_SLOT},

	{"PICMG_FRONT_BLADE",          SAHPI_ENT_PICMG_FRONT_BLADE},
	{"SYSTEM_INVENTORY_DEVICE",    SAHPI_ENT_SYSTEM_INVENTORY_DEVICE},
	{"FILTRATION_UNIT",            SAHPI_ENT_FILTRATION_UNIT},
	{"AMC",                        SAHPI_ENT_AMC},
	{"BMC",                        SAHPI_ENT_BMC},
	{"IPMC",                       SAHPI_ENT_IPMC},
	{"MMC",                        SAHPI_ENT_MMC},
	{"SHMC",                       SAHPI_ENT_SHMC},
	{"EPLD",                       SAHPI_ENT_EPLD},
	{"FPGA",                       SAHPI_ENT_FPGA},
	{"DASD",                       SAHPI_ENT_DASD},
	{"NIC",                        SAHPI_ENT_NIC},
	{"DSP",                        SAHPI_ENT_DSP},
	{"UCODE",                      SAHPI_ENT_UCODE},
	{"NPU",                        SAHPI_ENT_NPU},
	{"OEM",                        SAHPI_ENT_OEM}
};


/***********************************************************************
 * Name          : convert_entitypath_to_string
 *
 *
 * Description   : This function converts entitypath to string format
 *
 * Arguments     : SaHpiEntityPathT*    - Pointer to entity path
 *                 uns8*  - Pointer to destination string
 *
 * Return Values : NCSCC_RC_SUCCESS
 *                 NCSCC_RC_FAILURE.
 *
 * Notes         : None.
 ************************************************************************/
SaUint32T convert_entitypath_to_string(SaHpiEntityPathT *entity_path, 
			SaInt8T **ent_path_str)
{
        SaUint32T  i     = 0;
        SaUint32T  len = 0;
        SaUint32T  rc = NCSCC_RC_SUCCESS;
	SaUint32T  temp_index = 0;
	SaUint32T  temp_value = 0;
	SaUint32T  num_entity_types; 
	SaUint32T  index_array[SAHPI_MAX_ENTITY_PATH]={};
	SaUint32T  found = FALSE;

	if(NULL == entity_path){
		LOG_ER("Invalid entity_path arg argument");
		return NCSCC_RC_FAILURE;
	}
	
	num_entity_types = sizeof(hpi_ent_type_list)/
			sizeof(PLMS_ENTITY_TYPE_LIST);

	/* Allocate memory for entity_path_str */
	for(i = 0; i < SAHPI_MAX_ENTITY_PATH; i++){


		if(entity_path->Entry[i].EntityType <SAHPI_ENT_SYSTEM_FIRMWARE){
			temp_index = entity_path->Entry[i].EntityType;
			found = TRUE;
		}
#if 0
		if(entity_path->Entry[i].EntityType > SAHPI_ENT_SYSTEM_FIRMWARE
			&& entity_path->Entry[i].EntityType <= SAHPI_ENT_RACK){
			temp_value = entity_path->Entry[i].EntityType
				- SAHPI_ENT_SYSTEM_FIRMWARE;
				
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE + temp_value; 
			found = TRUE;
		}
#endif
		if(SAHPI_ENT_CHASSIS_SPECIFIC ==
			entity_path->Entry[i].EntityType){
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE +1;
			found = TRUE;
		}
		if(SAHPI_ENT_BOARD_SET_SPECIFIC ==	
			entity_path->Entry[i].EntityType){
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE +2;
			found = TRUE;
		}
		if(SAHPI_ENT_OEM_SYSINT_SPECIFIC ==
			entity_path->Entry[i].EntityType){
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE +3;
			found = TRUE;
		}
		if(SAHPI_ENT_ROOT ==
			entity_path->Entry[i].EntityType){
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE +4;
			found = TRUE;
		}
		if(SAHPI_ENT_RACK ==
			entity_path->Entry[i].EntityType){
			temp_index = SAHPI_ENT_SYSTEM_FIRMWARE +5;
			found = TRUE;
		}

		
		if(entity_path->Entry[i].EntityType > SAHPI_ENT_RACK &&
			entity_path->Entry[i].EntityType <= SAHPI_ENT_AMC ){
			temp_value = entity_path->Entry[i].EntityType
				- SAHPI_ENT_RACK;
				
			temp_index = SAHPI_ENT_RACK_INDEX + temp_value;
			found = TRUE;
		}

		if((entity_path->Entry[i].EntityType > SAHPI_ENT_AMC) && 
		(entity_path->Entry[i].EntityType <= SAHPI_ENT_OEM)){
			temp_value = entity_path->Entry[i].EntityType -
				SAHPI_ENT_AMC;
			temp_index = SAHPI_ENT_AMC_INDEX + temp_value;
			found = TRUE;
		}
		if(found == FALSE || temp_index > num_entity_types){
			LOG_ER("Invalid entity_path arg argument");
			return NCSCC_RC_FAILURE;
		}

		index_array[i] = temp_index;
			
		len += strlen(hpi_ent_type_list[temp_index].etype_str);
         	len += 7;
		if(entity_path->Entry[i].EntityType == SAHPI_ENT_ROOT)
			break;

		temp_index = 0;
	}
	if(len != 0){
		*ent_path_str = (SaInt8T *)malloc(len);
		if(NULL == *ent_path_str)
		{
			LOG_CR("Failed to allocate memory error: %s",
						strerror(errno));
			assert(0);
		}
	}else{
		LOG_ER("Invalia entity_path arg");
		return NCSCC_RC_FAILURE;
	}
	convert_entity_types(entity_path, *ent_path_str, index_array);

	*((*ent_path_str) + len - 1) = '\0';

        return rc;
}
/****************************************************************************
* Name          : convert_entity_types 
*
* Description   : 
*
* Arguments     : 
*
* Return Values :NSCC_RC_SUCCESS
*                NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
static SaUint32T convert_entity_types(SaHpiEntityPathT *entity_path, 
				SaInt8T *ent_path_str,
				SaUint32T  index_array[SAHPI_MAX_ENTITY_PATH])
{
	SaUint32T i     = 0;
	SaUint32T index = 0;
	SaUint32T count;
	SaUint32T rc = NCSCC_RC_SUCCESS;

	if(NULL == ent_path_str || NULL == entity_path){
		LOG_ER("Invalia args to convert_entity_types");
		return NCSCC_RC_FAILURE;
	}

        for(i = 0; i < SAHPI_MAX_ENTITY_PATH; i++)
        {
		index = entity_path->Entry[i].EntityLocation;

		memcpy(ent_path_str,hpi_ent_type_list[index_array[i]].etype_str,
			strlen(hpi_ent_type_list[index_array[i]].etype_str));

		ent_path_str +=
		strlen(hpi_ent_type_list[index_array[i]].etype_str);

		*(ent_path_str++) = '.';

		count = sprintf(ent_path_str, "%d",entity_path->Entry[i].EntityLocation); 
		ent_path_str += count;

		if(entity_path->Entry[i+1].EntityType == SAHPI_ENT_ROOT)
			break;

		*(ent_path_str++)= ',';

        }

        /* Must null-terminate this string */
        *ent_path_str = '\0';
	return rc;
}


/****************************************************************************
* Name          : convert_string_to_epath
*
* Description   : This function is a utility function used to convert the
*                 canonical string of entity path to the SAF HPI defined
*                 entity path structure 'SaHpiEntityPathT'.
*
* Arguments     : epath_str (Input)canonical entity path string.
*                 epath_len (Input)
*                 epath_ptr (Output) Pointer to HPI's entity path structure
*
* Return Values :iNSCC_RC_SUCCESS
*                NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
SaUint32T convert_string_to_epath(SaInt8T *epath_str,
                                           SaHpiEntityPathT *epath_ptr)
{
        SaInt8T 	*tok, *ptr, *end_char;
        SaInt8T 	*epath;
        SaUint32T 	rc = NCSCC_RC_SUCCESS;
	SaUint32T 	entity_index = 0;
	SaUint32T 	entity_type, entity_type_index;
	SaUint32T 	num_entity_types; 
        SaUint32T 	epath_len;

	TRACE_ENTER();
        /* verify the arguments */
        if ( NULL == epath_str )
        {
                LOG_ER("Invalid arguments to string2entitypath");
                return NCSCC_RC_FAILURE;
        }
        memset(epath_ptr, 0, sizeof(SaHpiEntityPathT));

	epath_len = strlen(epath_str);
        /* allocate memory to make a duplicate of epath_str */
        if (NULL == (epath = (SaInt8T *)malloc(epath_len)))
        {
		LOG_CR("Failed to allocate memory error: %s",
						strerror(errno));
                return NCSCC_RC_FAILURE;
        }

        /* copy epath_str to epath */
        memcpy(epath, epath_str, epath_len);
        /* set the pointers */
        tok = epath;
        end_char = epath + epath_len;

	 /* get the tokens from epath and fill the epath_ptr structure */
        while (tok != NULL)
        {
                /* gets the entity type */
                if (NULL == (ptr = strchr(tok, EPATH_DOT_SEPARATOR_CHAR)))
                        break;
                *ptr = '\0';
                remove_spaces(&tok);

		 /** compare the token with available entity type and return the
		 ** corresponding value for a matching entity type.
    		 **/
		num_entity_types = 
			sizeof(hpi_ent_type_list)/sizeof(PLMS_ENTITY_TYPE_LIST);
		for (entity_type_index = 0; entity_type_index < num_entity_types; entity_type_index++){
			if (0 == strcmp(tok, hpi_ent_type_list[entity_type_index].etype_str))
			break;
		}
		
		if (num_entity_types == entity_type_index){
			entity_type = SAHPI_ENT_UNSPECIFIED;
			rc = NCSCC_RC_FAILURE;
			break;
		}else{
			entity_type = hpi_ent_type_list[entity_type_index].etype_val;
		}

                /* fill corresponding value of entity type in the structure */
                epath_ptr->Entry[entity_index].EntityType =entity_type; 
                if ((tok = ptr+1) >= end_char)
                        break;
                /* get the entity instance value */
                if (NULL == (ptr = strchr(tok, EPATH_COMMA_SEPARATOR_CHAR)))
                        ptr = strchr(tok, '\0');
                *ptr = '\0';

                remove_spaces(&tok);

                /* put the entity instance value in the epath_ptr structure */
                sscanf(tok, "%d", &epath_ptr->Entry[entity_index].EntityLocation);
                if ((tok = ptr+1) >= end_char)
                        break;
                if (++entity_index >= SAHPI_MAX_ENTITY_PATH)
                        break;
        }

	epath_ptr->Entry[entity_index].EntityType = SAHPI_ENT_ROOT;
        epath_ptr->Entry[entity_index].EntityLocation = 0;

        /* free the duplicate string allocated for entity path */
        free(epath);
	epath = NULL;
	TRACE_LEAVE();
        return rc;
}
/****************************************************************************
 * Name          : remove_spaces
 *
 * Description   : This function used to remove the white spaces around
 *                 the token string read from entity path string.
 *
 * Arguments     : tok - pointer to token string.
 *
 * Return Values : string free of white spaces.
 *
 * Notes         : None.
 *****************************************************************************/
static void remove_spaces(SaInt8T **tok)
{
	SaInt8T *str = *tok;
	if (str == NULL)
	return;

	/* remove spaces before token */
	while ((*str != '\0') && (*str == ' '))
	str++;

	/* mark the first non-space character as beginning of token */
	*tok = str;

	/* move to end of token */
	while ((*str!= '\0') && (*str!= ' '))
	str++;

	/* terminate at the end of token, ignore rest of spaces if any */
	if (*str== ' ')
	*str= '\0';

	return;
}
