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
 *
 */

#include "cpd.h"
#include "cpd_imm.h"
#include "immutil.h"
#include "saImm.h"
#include "cpd_log.h"

extern struct ImmutilWrapperProfile immutilWrapperProfile;
#define CPSV_IMM_IMPLEMENTER_NAME (SaImmOiImplementerNameT) "safCheckPointService"
const SaImmOiImplementerNameT implementer_name = CPSV_IMM_IMPLEMENTER_NAME;

/* IMMSv Defs */
#define CPSV_IMM_RELEASE_CODE 'A'
#define CPSV_IMM_MAJOR_VERSION 0x02
#define CPSV_IMM_MINOR_VERSION 0x01

static SaVersionT imm_version = {
	CPSV_IMM_RELEASE_CODE,
	CPSV_IMM_MAJOR_VERSION,
	CPSV_IMM_MINOR_VERSION
};

static SaAisErrorT cpd_saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
						   const SaNameT *objectName, const SaImmAttrNameT *attributeNames);
static uns32 cpd_fetch_used_size(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb);
static uns32 cpd_fetch_num_sections(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb);
static char* ckpt_replica_extract_node_name(char *src,char *key);

SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = NULL,
	.saImmOiCcbApplyCallback = NULL,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = NULL,
	.saImmOiRtAttrUpdateCallback = cpd_saImmOiRtAttrUpdateCallback
};

/****************************************************************************
 * Name          : cpd_saImmOiRtAttrUpdateCallback
 *
 * Description   : This callback function is invoked when a OM requests for object 
 *                 information .
 *
 * Arguments     : immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
static SaAisErrorT cpd_saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
						   const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	
	int i = 0, attr_count = 0;
	CPD_CB *cb = NULL;
	CPD_CKPT_MAP_INFO *map_info = NULL;
	CPD_CKPT_INFO_NODE *ckpt_node = NULL;
	SaTimeT ckpt_ret_duration;
	SaUint32T num_users, num_readers, num_writers, num_replicas, num_sections, num_corrupt_sections;
	SaUint32T replicaIsActive = TRUE;
	SaUint64T ckpt_size, ckpt_used_size;
	SaImmAttrNameT attributeName;
	SaImmAttrModificationT_2 attr_output[9];
	const SaImmAttrModificationT_2 *attrMods[10];
	CPD_CKPT_REPLOC_INFO *rep_info = NULL;
	CPD_REP_KEY_INFO key_info;
	SaNameT replica_dn, ckptName, clm_node_name, nodeName;
	char *ckpt_name, *node_name;
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;

	memset(&replica_dn, 0, sizeof(SaNameT));
	memset(&ckptName, 0, sizeof(ckptName));
	memset(&nodeName, 0, sizeof(SaNameT));
	strcpy((char *)ckptName.value, (char *)objectName->value);
	ckptName.length = objectName->length;

	SaImmAttrValueT ckptSizeUpdateValue[] = { &ckpt_size };
	SaImmAttrValueT ckptUsedSizeUpdateValue[] = { &ckpt_used_size };
	SaImmAttrValueT ckptRetDurationUpdateValue[] = { &ckpt_ret_duration };
	SaImmAttrValueT ckptNumOpenersUpdateValue[] = { &num_users };
	SaImmAttrValueT ckptNumReadersUpdateValue[] = { &num_readers };
	SaImmAttrValueT ckptNumWritersUpdateValue[] = { &num_writers };
	SaImmAttrValueT ckptNumReplicasUpdateValue[] = { &num_replicas };
	SaImmAttrValueT ckptNumSectionsUpdateValue[] = { &num_sections };
	SaImmAttrValueT ckptNumCorruptSectionsUpdateValue[] = { &num_corrupt_sections };

	SaImmAttrValueT ckptReplicaIsActiveUpdateValue[] = { &replicaIsActive };

	/* Get CPD CB Handle. */
	m_CPD_RETRIEVE_CB(cb);
	if (cb == NULL) {
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	if (strncmp((char *)objectName->value, "safReplica=", 11) == 0) {
		ckpt_name = strchr((char *)objectName->value, ',');
		if (ckpt_name) {
			ckpt_name++;	/*escaping first ',' of the associated class DN name */
			ckpt_name = strchr((char *)ckpt_name, ',');
			if (ckpt_name) {
				ckpt_name++;
				memset(&ckptName, 0, sizeof(ckptName));
				strcpy((char *)ckptName.value, ckpt_name);
				ckptName.length = strlen(ckpt_name);
			}
		}
	node_name = (char*)malloc((objectName->length-ckptName.length)*sizeof(char));
	memset(node_name,0,(objectName->length-ckptName.length));
	node_name = (char*)memcpy(node_name,(char*)objectName->value,objectName->length-ckptName.length-1);

	node_name = ckpt_replica_extract_node_name(node_name,"\\");
	node_name = node_name + 11;	
	strcpy((char*)nodeName.value, node_name);
	nodeName.length = strlen(node_name);
	}

	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, &ckptName, &map_info);

	if (map_info) {

		cpd_ckpt_node_get(&cb->ckpt_tree, &map_info->ckpt_id, &ckpt_node);

		if (ckpt_node) {
				key_info.ckpt_name = ckpt_node->ckpt_name;
				key_info.node_name = nodeName;
				cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &rep_info);
	                
             		if (rep_info) {
				clm_node_name.length = m_NCS_OS_NTOHS(rep_info->rep_key.node_name.length);
				strncpy((char *)clm_node_name.value, (char *)rep_info->rep_key.node_name.value,
					clm_node_name.length);
				/* escapes rdn's  ',' with '\'   */
				cpd_create_association_class_dn(&clm_node_name,
								&rep_info->rep_key.ckpt_name, "safReplica",
								&replica_dn);

				if (m_CMP_HORDER_SANAMET(*objectName, replica_dn) == 0) {

					/* Walk through the attribute Name list */
					while ((attributeName = attributeNames[i]) != NULL) {
						if (strcmp(attributeName, "saCkptReplicaIsActive") == 0) {
							/* replicaIsActive  DESCRIPTION " Indicates if the node 
							   contains an active replica of the checkpoint designated 
							   by 'saCkptCheckpointNameLoc'.  Applicable only for collocated 
							   checkpoints created with either asynchronous or asynchronousweak 
							   update option. The concept of an active replica doesn't apply 
							   if the checkpoint is either non-collocated or collocated but 
							   created with a synchronous update option and hence the other 
							   INTEGER values have been provided to bring out those specific traits." */

							if (rep_info->rep_type == REP_NOTACTIVE)
								replicaIsActive = FALSE;
							attr_output[0].modType = SA_IMM_ATTR_VALUES_REPLACE;
							attr_output[0].modAttr.attrName = attributeName;
							attr_output[0].modAttr.attrValuesNumber = 1;
							attr_output[0].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
							attr_output[0].modAttr.attrValues =
							    ckptReplicaIsActiveUpdateValue;
							attrMods[0] = &attr_output[0];
							attrMods[1] = NULL;
							rc = saImmOiRtObjectUpdate_2(cb->immOiHandle, objectName, attrMods);
							if (rc != SA_AIS_OK) {
								cpd_log(NCSFL_SEV_ERROR, "saImmOiRtObjectUpdate failed for replica object: %u", rc);
							}

						}
						i++;
					}
					goto done;
				}

			}

			if (m_CMP_HORDER_SANAMET(*objectName, ckpt_node->ckpt_name) == 0) {
				/* Walk through the attribute Name list */
				while ((attributeName = attributeNames[i]) != NULL) {
					if (strcmp(attributeName, "saCkptCheckpointSize") == 0) {
						ckpt_size = ckpt_node->attributes.checkpointSize;
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
						attr_output[attr_count].modAttr.attrValues = ckptSizeUpdateValue;
						attrMods[attr_count] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointUsedSize") == 0) {
						if (cpd_fetch_used_size(ckpt_node, cb) == NCSCC_RC_FAILURE) {
							cpd_log(NCSFL_SEV_ERROR, "cpd_fetch_used_size failed");
							rc = SA_AIS_ERR_FAILED_OPERATION;
							goto done;
						}

						ckpt_used_size = ckpt_node->ckpt_used_size;
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT64T;
						attr_output[attr_count].modAttr.attrValues = ckptUsedSizeUpdateValue;
						attrMods[attr_count] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointRetDuration") == 0) {
						ckpt_ret_duration = ckpt_node->ret_time;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SATIMET;
						attr_output[attr_count].modAttr.attrValues = ckptRetDurationUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumOpeners") == 0) {
						num_users = ckpt_node->num_users;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues = ckptNumOpenersUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumReaders") == 0) {
						num_readers = ckpt_node->num_readers;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues = ckptNumReadersUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumWriters") == 0) {
						num_writers = ckpt_node->num_writers;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues = ckptNumWritersUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumReplicas") == 0) {
						num_replicas = ckpt_node->dest_cnt;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues = ckptNumReplicasUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumSections") == 0) {
						if (ckpt_node->attributes.maxSections == 1) {
							num_sections = ckpt_node->num_sections;
						}
						else {
							if (cpd_fetch_num_sections(ckpt_node, cb) == NCSCC_RC_FAILURE) {
								cpd_log(NCSFL_SEV_ERROR, "cpd_fetch_num_sections failed");
								rc = SA_AIS_ERR_FAILED_OPERATION;
								goto done;
							}

							num_sections = ckpt_node->num_sections;
						}
						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues = ckptNumSectionsUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					} else if (strcmp(attributeName, "saCkptCheckpointNumCorruptSections") == 0) {
						num_corrupt_sections = ckpt_node->num_corrupt_sections;

						attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
						attr_output[attr_count].modAttr.attrName = attributeName;
						attr_output[attr_count].modAttr.attrValuesNumber = 1;
						attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
						attr_output[attr_count].modAttr.attrValues =
						    ckptNumCorruptSectionsUpdateValue;
						attrMods[i] = &attr_output[attr_count];
						++attr_count;
					}

					i++;
				}	/* End while attributesNames() */

				attrMods[attr_count] = NULL;
				rc = saImmOiRtObjectUpdate_2(cb->immOiHandle, objectName, attrMods);
				if (rc != SA_AIS_OK) {
					cpd_log(NCSFL_SEV_ERROR, "saImmOiRtObjectUpdate failed for ckpt object: %u", rc);
				}
				goto done;
			}	/* End if  m_CMP_HORDER_SANAMET */
		}		/* end of if (ckpt_node != NULL) */
	}
	
done:
	ncshm_give_hdl(cb->cpd_hdl);
	return rc;
}

/****************************************************************************
 * Name          : create_runtime_replica_object
 *
 * Description   : This function is invoked to create a replica runtime object 
 *
 * Arguments     : rname            - DN of resource
 *                 ckpt_reploc_node - Checkpoint reploc node 
 *                 immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT create_runtime_replica_object(CPD_CKPT_REPLOC_INFO *ckpt_reploc_node, SaImmOiHandleT immOiHandle)
{
	SaImmAttrValueT dn[1];
	SaImmAttrValuesT_2 replica_dn;
	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrValuesT_2 *attrValues[2];
	SaNameT replica_rdn;
	SaNameT ckpt_name;
	memset(&ckpt_name, 0, sizeof(SaNameT));

	/* escapes rdn's  ',' with '\'   */
	cpd_create_association_class_dn(&ckpt_reploc_node->rep_key.node_name, NULL, "safReplica", &replica_rdn);

	dn[0] = &replica_rdn;
	replica_dn.attrName = "safReplica";
	replica_dn.attrValueType = SA_IMM_ATTR_SANAMET;
	replica_dn.attrValuesNumber = 1;
	replica_dn.attrValues = dn;

	attrValues[0] = &replica_dn;
	attrValues[1] = NULL;

	strcpy((char *)ckpt_name.value, (char *)ckpt_reploc_node->rep_key.ckpt_name.value);
	ckpt_name.length = strlen((char *)ckpt_name.value);

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaCkptReplica", &ckpt_name, attrValues);
	return rc;
}

/****************************************************************************
 * Name          : create_runtime_ckpt_object
 *
 * Description   : This function is invoked to create a checkpoint runtime object 
 *
 * Arguments     : rname            - DN of resource
 *                 ckpt_node        - Checkpoint Node 
 *                 immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT create_runtime_ckpt_object(CPD_CKPT_INFO_NODE *ckpt_node, SaImmOiHandleT immOiHandle)
{
	SaNameT parentName;
	SaAisErrorT rc = SA_AIS_OK;
	char *dndup = strdup((char *)ckpt_node->ckpt_name.value);
	char *parent_name = strchr((char *)ckpt_node->ckpt_name.value, ',');
	char *rdnstr;
	const SaImmAttrValuesT_2 *attrValues[7];
	SaImmAttrValueT dn[1], create_time[1], creat_flags[1], max_sections[1],
	    max_section_size[1], max_section_id_size[1];
	SaImmAttrValuesT_2 ckpt_dn, ckpt_creat_time, ckpt_creat_flags,
	    ckpt_max_sections, ckpt_max_section_size, ckpt_max_section_id_size;
	SaTimeT create_time_sec;

	memset(&parentName, 0, sizeof(SaNameT));

	if (parent_name != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;
		strcpy((char *)parentName.value, parent_name);
		parentName.length = strlen((char *)parent_name);
	} else
		rdnstr = (char *)ckpt_node->ckpt_name.value;

	dn[0] = &rdnstr;
	create_time_sec = ckpt_node->create_time * SA_TIME_ONE_SECOND;
	create_time[0] = &create_time_sec;
	creat_flags[0] = &ckpt_node->attributes.creationFlags;
	max_sections[0] = &ckpt_node->attributes.maxSections;
	max_section_size[0] = &ckpt_node->attributes.maxSectionSize;
	max_section_id_size[0] = &ckpt_node->attributes.maxSectionIdSize;

	ckpt_dn.attrName = "safCkpt";
	ckpt_dn.attrValueType = SA_IMM_ATTR_SASTRINGT;
	ckpt_dn.attrValuesNumber = 1;
	ckpt_dn.attrValues = dn;

	ckpt_creat_time.attrName = "saCkptCheckpointCreationTimestamp";
	ckpt_creat_time.attrValueType = SA_IMM_ATTR_SATIMET;
	ckpt_creat_time.attrValuesNumber = 1;
	ckpt_creat_time.attrValues = create_time;

	ckpt_creat_flags.attrName = "saCkptCheckpointCreationFlags";
	ckpt_creat_flags.attrValueType = SA_IMM_ATTR_SAUINT32T;
	ckpt_creat_flags.attrValuesNumber = 1;
	ckpt_creat_flags.attrValues = creat_flags;

	ckpt_max_sections.attrName = "saCkptCheckpointMaxSections";
	ckpt_max_sections.attrValueType = SA_IMM_ATTR_SAUINT32T;
	ckpt_max_sections.attrValuesNumber = 1;
	ckpt_max_sections.attrValues = max_sections;

	ckpt_max_section_size.attrName = "saCkptCheckpointMaxSectionSize";
	ckpt_max_section_size.attrValueType = SA_IMM_ATTR_SAUINT64T;
	ckpt_max_section_size.attrValuesNumber = 1;
	ckpt_max_section_size.attrValues = max_section_size;

	ckpt_max_section_id_size.attrName = "saCkptCheckpointMaxSectionIdSize";
	ckpt_max_section_id_size.attrValueType = SA_IMM_ATTR_SAUINT64T;
	ckpt_max_section_id_size.attrValuesNumber = 1;
	ckpt_max_section_id_size.attrValues = max_section_id_size;

	attrValues[0] = &ckpt_dn;
	attrValues[1] = &ckpt_creat_time;
	attrValues[2] = &ckpt_creat_flags;
	attrValues[3] = &ckpt_max_sections;
	attrValues[4] = &ckpt_max_section_size;
	attrValues[5] = &ckpt_max_section_id_size;
	attrValues[6] = NULL;

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaCkptCheckpoint", &parentName, attrValues);
	if (rc != SA_AIS_OK)
		cpd_log(NCSFL_SEV_ERROR, " saImmOiRtObjectCreate_2 failed with error = %u", rc);

	free(dndup);
	return rc;

}	/* End create_runtime_object() */

/****************************************************************************
 * Name          : cpd_imm_init
 *
 * Description   : Initialize the OI and get selection object  
 *
 * Arguments     : cb            
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT cpd_imm_init(CPD_CB *cb)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;
	rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK) {
		rc = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
	}
	return rc;
}

/****************************************************************************
 * Name          : _cpd_imm_declare_implementer
 *
 * Description   : Become a OI implementer  
 *
 * Arguments     : cb            
 *
 * Return Values : None 
 *
 * Notes         : None.
 *****************************************************************************/
static void *_cpd_imm_declare_implementer(void *cb)
{
	SaAisErrorT error = SA_AIS_OK;
	CPD_CB *cpd_cb = (CPD_CB *)cb;
	error = saImmOiImplementerSet(cpd_cb->immOiHandle, implementer_name);
	unsigned int nTries = 1;
	while (error == SA_AIS_ERR_TRY_AGAIN && nTries < 25) {
		usleep(400 * 1000);
		error = saImmOiImplementerSet(cpd_cb->immOiHandle, implementer_name);
		nTries++;
	}
	if (error != SA_AIS_OK) {
		cpd_log(NCSFL_SEV_ERROR, "saImmOiImplementerSet FAILED, rc = %u", error);
		exit(EXIT_FAILURE);
	}
	return NULL;
}


void cpd_create_association_class_dn(const SaNameT *child_dn, const SaNameT *parent_dn,
				     const char *rdn_tag, SaNameT *dn)
{
	char *p = (char *)dn->value;
	int i;

	memset(dn, 0, sizeof(SaNameT));

	p += sprintf((char *)dn->value, "%s=", rdn_tag);

	/* copy child DN and escape commas */
	for (i = 0; i < child_dn->length; i++) {
		if (child_dn->value[i] == ',')
			*p++ = 0x5c;	/* backslash */

		*p++ = child_dn->value[i];
	}

	if (parent_dn != NULL) {
		*p++ = ',';
		strcpy(p, (char *)parent_dn->value);
	}

	dn->length = strlen((char *)dn->value);
}

static uns32 cpd_fetch_used_size(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb)
{
	CPSV_EVT send_evt;
	CPSV_EVT *out_evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&send_evt, 0, sizeof(CPSV_EVT));

	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_SIZE;
	send_evt.info.cpnd.info.ckpt_mem_size.ckpt_id = ckpt_node->ckpt_id;

	if (ckpt_node->active_dest == 0) {
		ckpt_node->ckpt_used_size = 0;
	} else {
		rc = cpd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, ckpt_node->active_dest, &send_evt, &out_evt,
					   CPSV_WAIT_TIME);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_FCL(CPD_MDS_SEND_FAIL, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->active_dest, __FILE__,
				      __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if (out_evt == NULL) {
			m_LOG_CPD_FCL(CPD_MDS_SEND_FAIL, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->active_dest, __FILE__,
				      __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		if (out_evt->info.cpd.info.ckpt_mem_used.error != SA_AIS_OK) {
			m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPD);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
		ckpt_node->ckpt_used_size = out_evt->info.cpd.info.ckpt_mem_used.ckpt_used_size;
		m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPD);
	}

 done:
	return rc;
}

static uns32 cpd_fetch_num_sections(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb)
{
	CPSV_EVT send_evt;
	CPSV_EVT *out_evt = NULL;
	uns32 rc = NCSCC_RC_SUCCESS;
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_NUM_SECTIONS;
	send_evt.info.cpnd.info.ckpt_sections.ckpt_id = ckpt_node->ckpt_id;

	if (ckpt_node->active_dest == 0) {
		ckpt_node->num_sections = 0;
		m_LOG_CPD_FCL(CPD_CPND_NODE_DOES_NOT_EXIST, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->active_dest,
			      __FILE__, __LINE__);
	} else {
		rc = cpd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, ckpt_node->active_dest, &send_evt, &out_evt,
					   CPSV_WAIT_TIME);
		if (rc != NCSCC_RC_SUCCESS) {
			m_LOG_CPD_FCL(CPD_MDS_SEND_FAIL, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->active_dest, __FILE__,
				      __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if (out_evt == NULL) {
			m_LOG_CPD_FCL(CPD_MDS_SEND_FAIL, CPD_FC_HDLN, NCSFL_SEV_ERROR, ckpt_node->active_dest, __FILE__,
				      __LINE__);
			rc = NCSCC_RC_FAILURE;
			goto done;
		} else {
			if (out_evt->info.cpd.info.ckpt_created_sections.error == SA_AIS_ERR_NOT_EXIST) {
				m_LOG_CPD_FCL(CPD_CPND_NODE_DOES_NOT_EXIST, CPD_FC_HDLN, NCSFL_SEV_ERROR,
					      ckpt_node->active_dest, __FILE__, __LINE__);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			ckpt_node->num_sections = out_evt->info.cpd.info.ckpt_created_sections.ckpt_num_sections;
			m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPD);
		}
	}

 done:
	return rc;
}



static char *ckpt_replica_extract_node_name(char *src,char *key)
{
	char *dest = NULL;
	SaUint32T len_src = 0, len_key = 0;
	SaUint32T i = 0, k = 0;
	len_src = strlen( src );
	len_key = strlen( key );
	dest = (char *) malloc( sizeof( char ) * len_src + 1 );
	memset( dest, 0, sizeof( char ) * len_src + 1 );
	for ( i = 0; i < len_src; i++ ) {
                if ( src[i] != '\\' ) {
                        dest[k] = src[i];
                        k++;
                }
	}
	return dest;
}

/**
 * Initialize the OI interface and get a selection object. 
 * @param cb
 * 
 * @return SaAisErrorT
 */
static void  *cpd_imm_reinit_thread(void * _cb)
{
	SaAisErrorT error = SA_AIS_OK;
	CPD_CB *cb = (CPD_CB *)_cb;
	TRACE_ENTER();
	/* Reinitiate IMM */
	error = cpd_imm_init(cb);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE)
			_cpd_imm_declare_implementer(cb);
	}
	else
	{

		LOG_ER("cpd_imm_init FAILED: %s", strerror(error));
		exit(EXIT_FAILURE);

	}
	TRACE_LEAVE();
	return NULL;
}


/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void cpd_imm_reinit_bg(CPD_CB * cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();

	if (pthread_create(&thread, &attr, cpd_imm_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

