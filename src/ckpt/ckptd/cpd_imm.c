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

#include "ckpt/ckptd/cpd.h"
#include "ckpt/ckptd/cpd_imm.h"
#include "osaf/immutil/immutil.h"
#include "imm/saf/saImm.h"
#include "base/saf_error.h"

extern const SaImmOiImplementerNameT implementer_name;
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
static uint32_t cpd_fetch_used_size(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb);
static uint32_t cpd_fetch_num_sections(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb);
static void extract_node_name_from_replica_name(const char *replica_name, const char *ckpt_name, char **node_name);

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
	SaUint32T replicaIsActive = true;
	SaUint64T ckpt_size, ckpt_used_size;
	SaImmAttrNameT attributeName;
	SaImmAttrModificationT_2 attr_output[9];
	const SaImmAttrModificationT_2 *attrMods[10];
	CPD_CKPT_REPLOC_INFO *rep_info = NULL;
	CPD_REP_KEY_INFO key_info;
	char *replica_dn = NULL, *ckpt_name = NULL, *node_name = NULL; 
	SaConstStringT object_name;
	SaAisErrorT rc = SA_AIS_ERR_FAILED_OPERATION;

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

	TRACE_ENTER();

	/* Get CPD CB Handle. */
	m_CPD_RETRIEVE_CB(cb);
	if (cb == NULL) {
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	object_name = osaf_extended_name_borrow(objectName);

	/* Extract ckpt_name and node_name */
	if (strncmp(object_name, "safReplica=", 11) == 0) {
		/* Extract ckpt_name */
		char *p_char = strchr(object_name, ',');
		if (p_char) {
			p_char++;	/* escaping first ',' of the associated class DN name */
			p_char = strchr(p_char, ',');
			if (p_char) {
				p_char++;
				ckpt_name = malloc(strlen(p_char) + 1); /*1 extra byte for \0 char*/
				strcpy(ckpt_name, p_char);
			}
		}

		/* Extract node_name */
		extract_node_name_from_replica_name(object_name, ckpt_name, &node_name);
	} else {
		ckpt_name = strdup(object_name);
	}

	TRACE_4("ckpt_name: %s", ckpt_name);
	TRACE_4("node_name: %s", node_name);

	cpd_ckpt_map_node_get(&cb->ckpt_map_tree, ckpt_name, &map_info);

	if (map_info) {

		cpd_ckpt_node_get(&cb->ckpt_tree, &map_info->ckpt_id, &ckpt_node);

		if (ckpt_node) {
			key_info.ckpt_name = ckpt_node->ckpt_name;
			key_info.node_name = node_name;
			cpd_ckpt_reploc_get(&cb->ckpt_reploc_tree, &key_info, &rep_info);
	                
             		if (rep_info) {
				/* escapes rdn's  ',' with '\'   */
				cpd_create_association_class_dn(rep_info->rep_key.node_name, 
								rep_info->rep_key.ckpt_name, "safReplica",
								&replica_dn);

				TRACE("replica_dn: %s", replica_dn);
				if (strcmp(object_name, replica_dn) == 0){

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
								replicaIsActive = false;
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
								LOG_ER("saImmOiRtObjectUpdate failed for replica object: %u", rc);
							}

						}
						i++;
					}
					goto done;
				}

			}

			if (strcmp(object_name, ckpt_node->ckpt_name) == 0){
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
							LOG_ER("cpd_fetch_used_size failed");
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
						else 
						{
							if (cpd_fetch_num_sections(ckpt_node, cb) == NCSCC_RC_FAILURE) {
								 LOG_ER("cpd_fetch_num_sections failed");
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
					 LOG_ER("saImmOiRtObjectUpdate failed for ckpt object: %u", rc);
				}
				goto done;
			}	/* End if  m_CMP_HORDER_SANAMET */
		}		/* end of if (ckpt_node != NULL) */
	}
	
done:
	if (ckpt_name != NULL)
		free(ckpt_name);

	if (node_name != NULL)
		free(node_name);

	if (replica_dn != NULL)
		free(replica_dn);

	ncshm_give_hdl(cb->cpd_hdl);
	TRACE_LEAVE();
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
	char* replica_name = NULL;
	SaNameT replica_rdn;
	SaNameT ckpt_name;
	memset(&ckpt_name, 0, sizeof(SaNameT));

	TRACE_ENTER();
	/* escapes rdn's  ',' with '\'   */
	cpd_create_association_class_dn(ckpt_reploc_node->rep_key.node_name, NULL, "safReplica", &replica_name);

	osaf_extended_name_lend(replica_name, &replica_rdn);

	dn[0] = &replica_rdn;
	replica_dn.attrName = "safReplica";
	replica_dn.attrValueType = SA_IMM_ATTR_SANAMET;
	replica_dn.attrValuesNumber = 1;
	replica_dn.attrValues = dn;

	attrValues[0] = &replica_dn;
	attrValues[1] = NULL;

	osaf_extended_name_lend(ckpt_reploc_node->rep_key.ckpt_name, &ckpt_name);

	rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, "SaCkptReplica", &ckpt_name, attrValues);
	if (rc != SA_AIS_OK)
		LOG_ER("create_runtime_replica_object - saImmOiRtObjectCreate_2 failed with error = %u", rc);

	free(replica_name);
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;
}

/****************************************************************************
 * Name          : delete_runtime_replica_object
 *
 * Description   : This function is invoked to delete a replica runtime object 
 *
 * Arguments     : ckpt_reploc_node - Checkpoint reploc node 
 *                 immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT delete_runtime_replica_object(CPD_CKPT_REPLOC_INFO *ckpt_reploc_node, SaImmOiHandleT immOiHandle)
{
	SaNameT replica_name;
	char* replica_dn = NULL;
	SaAisErrorT rc;

	TRACE_ENTER();

	cpd_create_association_class_dn(ckpt_reploc_node->rep_key.node_name,
					ckpt_reploc_node->rep_key.ckpt_name, "safReplica", &replica_dn);

	osaf_extended_name_lend(replica_dn, &replica_name);
	rc = immutil_saImmOiRtObjectDelete(immOiHandle, &replica_name); 
	if (rc != SA_AIS_OK) {
		LOG_ER("Deleting run time object %s Failed - rc = %d",replica_dn, rc);
	} else {
		TRACE("Deleting run time object %s Success - rc = %d",replica_dn, rc);
	}

	free(replica_dn);

	TRACE_LEAVE();
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
	char *dndup = strdup(ckpt_node->ckpt_name);
	char *parent_name = strchr(ckpt_node->ckpt_name, ',');
	char *rdnstr;
	const SaImmAttrValuesT_2 *attrValues[7];
	SaImmAttrValueT dn[1], create_time[1], creat_flags[1], max_sections[1],
	    max_section_size[1], max_section_id_size[1];
	SaImmAttrValuesT_2 ckpt_dn, ckpt_creat_time, ckpt_creat_flags,
	    ckpt_max_sections, ckpt_max_section_size, ckpt_max_section_id_size;
	SaTimeT create_time_sec;

	TRACE_ENTER();
	memset(&parentName, 0, sizeof(SaNameT));

	if (parent_name != NULL) {
		rdnstr = strtok(dndup, ",");
		parent_name++;

		osaf_extended_name_lend(parent_name, &parentName);
	} else
		rdnstr = (char *)ckpt_node->ckpt_name;

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
		LOG_ER("create_runtime_ckpt_object - saImmOiRtObjectCreate_2 failed with error = %u", rc);

	free(dndup);
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;

}	/* End create_runtime_object() */

/****************************************************************************
 * Name          : delete_runtime_ckpt_object
 *
 * Description   : This function is invoked to delete a checkpoint runtime object 
 *
 * Arguments     : ckpt_node        - Checkpoint Node 
 *                 immOiHandle      - IMM handle
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
SaAisErrorT delete_runtime_ckpt_object(CPD_CKPT_INFO_NODE *ckpt_node, SaImmOiHandleT immOiHandle)
{
	SaNameT ckpt_name;
	SaAisErrorT rc;

	osaf_extended_name_lend(ckpt_node->ckpt_name, &ckpt_name);

	rc =  immutil_saImmOiRtObjectDelete(immOiHandle, &ckpt_name);
	if (rc != SA_AIS_OK) {
		LOG_ER("Deleting run time object %s failed - rc = %d", ckpt_node->ckpt_name, rc);
	} else {
		TRACE("Deleting run time object %s success - rc = %d", ckpt_node->ckpt_name, rc);
	}
	return rc;
}

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
SaAisErrorT cpd_imm_init(SaImmOiHandleT* immOiHandle, SaSelectionObjectT* imm_sel_obj)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOiInitialize_2(immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK) {
		rc = immutil_saImmOiSelectionObjectGet(*immOiHandle, imm_sel_obj);
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
void cpd_imm_declare_implementer(SaImmOiHandleT* immOiHandle, SaSelectionObjectT* imm_sel_obj)
{
	SaAisErrorT error = SA_AIS_OK;

	TRACE_ENTER();
	static const unsigned sleep_delay_ms = 400;
	static const unsigned max_waiting_time_ms = 60 * 1000;
	unsigned msecs_waited = 0;
	for (;;) {
		if (msecs_waited >= max_waiting_time_ms) {
			LOG_ER("Timeout in cpd_imm_declare_implementer");
			exit(EXIT_FAILURE);
		}

		error = saImmOiImplementerSet(*immOiHandle, implementer_name);
		while (error == SA_AIS_ERR_TRY_AGAIN && msecs_waited < max_waiting_time_ms) {
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			error = saImmOiImplementerSet(*immOiHandle, implementer_name);
		}
		if (error == SA_AIS_ERR_TIMEOUT || error == SA_AIS_ERR_BAD_HANDLE) {
			LOG_WA("saImmOiImplementerSet returned %u", (unsigned) error);
			usleep(sleep_delay_ms * 1000);
			msecs_waited += sleep_delay_ms;
			saImmOiFinalize(*immOiHandle);
			*immOiHandle = 0;
			*imm_sel_obj = -1;
			if ((error = cpd_imm_init(immOiHandle, imm_sel_obj)) != SA_AIS_OK) {
				LOG_ER("cpd_imm_init FAILED, rc = %u", (unsigned) error);
				exit(EXIT_FAILURE);
			}
			continue;
		}
		if (error != SA_AIS_OK) {
			LOG_ER("saImmOiImplementerSet FAILED, rc = %u", (unsigned) error);
			exit(EXIT_FAILURE);
		}
		break;
	}
	TRACE_LEAVE();
}

/****************************************************************************
 * Name          : cpd_create_association_class_dn
 *
 * Description   : This function is invoked to create a 
 *                 dn = rdn_tag + '=' + child_dn + parent_dn 
 *                 User must free() the dn after using
 *
 * Arguments     : 
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
void cpd_create_association_class_dn(const char *child_dn, const char *parent_dn,
				     const char *rdn_tag, char **dn)
{
	int i;
	size_t child_dn_length = 0;
	size_t parent_dn_length = 0;
	size_t rdn_tag_length = 0;
	size_t class_dn_length = 0;

	if (child_dn != NULL)
		child_dn_length = strlen(child_dn);

	if (parent_dn != NULL)
		parent_dn_length = strlen(parent_dn);

	if (rdn_tag != NULL)
		rdn_tag_length = strlen(rdn_tag);

	class_dn_length = child_dn_length + parent_dn_length + rdn_tag_length + 10;

	char *class_dn = malloc(class_dn_length);
	memset(class_dn, 0, class_dn_length);

	char *p = class_dn;

	p += sprintf(class_dn, "%s=", rdn_tag);

	/* copy child DN and escape commas */
	for (i = 0; i < child_dn_length; i++) {
		if (child_dn[i] == ',')
			*p++ = 0x5c;	/* backslash */

		*p++ = child_dn[i];
	}

	if (parent_dn != NULL) {
		*p++ = ',';
		strcpy(p, parent_dn);
	}

	*dn = class_dn;
}

static uint32_t cpd_fetch_used_size(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb)
{
	CPSV_EVT send_evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;
	memset(&send_evt, 0, sizeof(CPSV_EVT));

	TRACE_ENTER();
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_SIZE;
	send_evt.info.cpnd.info.ckpt_mem_size.ckpt_id = ckpt_node->ckpt_id;

	if (ckpt_node->active_dest == 0) {
		ckpt_node->ckpt_used_size = 0;
	} else {
		rc = cpd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, ckpt_node->active_dest, &send_evt, &out_evt,
					   CPSV_WAIT_TIME);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("cpd mds send fail for active dest: %"PRIu64,ckpt_node->active_dest);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if (out_evt == NULL) {
			TRACE_4("cpd mds send fail for active dest :%"PRIu64"with out_evt NULL",ckpt_node->active_dest);
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
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;
}

static uint32_t cpd_fetch_num_sections(CPD_CKPT_INFO_NODE *ckpt_node, CPD_CB *cb)
{
	CPSV_EVT send_evt;
	CPSV_EVT *out_evt = NULL;
	uint32_t rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();
	memset(&send_evt, 0, sizeof(CPSV_EVT));
	send_evt.type = CPSV_EVT_TYPE_CPND;
	send_evt.info.cpnd.type = CPND_EVT_D2ND_CKPT_NUM_SECTIONS;
	send_evt.info.cpnd.info.ckpt_sections.ckpt_id = ckpt_node->ckpt_id;

	if (ckpt_node->active_dest == 0) {
		ckpt_node->num_sections = 0;
		TRACE_4("cpd cpnd node does not exist for active dest :%"PRIu64,ckpt_node->active_dest);
	} else {
		rc = cpd_mds_msg_sync_send(cb, NCSMDS_SVC_ID_CPND, ckpt_node->active_dest, &send_evt, &out_evt,
					   CPSV_WAIT_TIME);
		if (rc != NCSCC_RC_SUCCESS) {

			TRACE_4("cpd mds send fail in fetch num sections for active dest :%"PRIu64,ckpt_node->active_dest);
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

		if (out_evt == NULL) {
			TRACE_4("cpd mds send fail with out_evt as NULL for active dest :%"PRIu64,ckpt_node->active_dest);
			rc = NCSCC_RC_FAILURE;
			goto done;
		} else {
			if (out_evt->info.cpd.info.ckpt_created_sections.error == SA_AIS_ERR_NOT_EXIST) {
	
				TRACE_4("cpd cpnd node doest not exist for active dest:%"PRIu64,ckpt_node->active_dest);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}

			ckpt_node->num_sections = out_evt->info.cpd.info.ckpt_created_sections.ckpt_num_sections;
			m_MMGR_FREE_CPSV_EVT(out_evt, NCS_SERVICE_ID_CPD);
		}
	}

 done:
	TRACE_LEAVE2("Ret val %d",rc);
	return rc;
}

/****************************************************************************
 * Name          : extract_node_name_from_replica_name
 *
 * Description   : This function extract node_name without '/' from replica
 *                 name. The user must free() the node_name after using
 *
 * Arguments     : 
 *
 * Return Values : SaAisErrorT 
 *
 * Notes         : None.
 *****************************************************************************/
static void extract_node_name_from_replica_name(const char *replica_name, const char *ckpt_name, char **node_name)
{
	char *dest = NULL;
	SaUint32T i = 0, k = 0;
	
	if (replica_name == NULL || ckpt_name == NULL)
		return;

	/* Remove slash '/' , the ',' right before the ckpt_name and ckpt_name */ 
	int node_name_length = strlen(replica_name) - strlen(ckpt_name) - strlen("safReplica=") - 1;
	dest = malloc(node_name_length);
	memset(dest, 0, node_name_length);

	const char* src = replica_name + strlen("safReplica=");

	for (i = 0; i < node_name_length; i++) {
		if (src[i] != '\\') {
			dest[k] = src[i];
			k++;
		}
	}

	*node_name = dest;

	return;
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
	SaImmOiHandleT immOiHandle;
	SaSelectionObjectT imm_sel_obj;
	TRACE_ENTER();
	/* Reinitiate IMM */
	error = cpd_imm_init(&immOiHandle, &imm_sel_obj);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE)
			cpd_imm_declare_implementer(&immOiHandle, &imm_sel_obj);
		cb->imm_sel_obj = imm_sel_obj;
		cb->immOiHandle = immOiHandle;
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

/**
 * @Brief
 * Search for old runtime checkpoint objects and delete them
 *
 * If the checkpoint server on both SC nodes is stopped, runtime IMM
 * objects for application checkpoints may be left.
 * This has to be cleaned up when the checkpoint server is started again.
 *
 *
 */
SaAisErrorT cpd_clean_checkpoint_objects(CPD_CB *cb)
{
       SaAisErrorT rc = SA_AIS_OK;
       SaImmHandleT immOmHandle;
       SaImmSearchHandleT immSearchHandle;

       TRACE_ENTER();

       /* Save immutil settings and reconfigure */
       struct ImmutilWrapperProfile tmp_immutilWrapperProfile;
       tmp_immutilWrapperProfile.errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
       tmp_immutilWrapperProfile.nTries = immutilWrapperProfile.nTries;
       tmp_immutilWrapperProfile.retryInterval = immutilWrapperProfile.retryInterval;

       immutilWrapperProfile.errorsAreFatal = 0;
       immutilWrapperProfile.nTries = 500;
       immutilWrapperProfile.retryInterval = 1000;

       /* Intialize Om API
        */
       rc = immutil_saImmOmInitialize(&immOmHandle, NULL, &imm_version);
       if (rc != SA_AIS_OK) {
               LOG_ER("%s saImmOmInitialize FAIL %d", __FUNCTION__, rc);
               goto done;
       }

       cpd_imm_declare_implementer(&cb->immOiHandle, &cb->imm_sel_obj);

       /* Initialize search for application checkpoint runtime objects
        * Search for all objects of class 'SaCkptCheckpoint'
        * Search criteria:
        * Attribute SaImmAttrClassName == SaCkptCheckpoint
        */
       SaImmSearchParametersT_2 searchParam;
       const char* class_name = "SaCkptCheckpoint";
       searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
       searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
       searchParam.searchOneAttr.attrValue = &class_name;
       SaNameT root_name;
       osaf_extended_name_lend("\0", &root_name);

       rc = immutil_saImmOmSearchInitialize_2(
                       immOmHandle,
                       &root_name,
                       SA_IMM_SUBTREE,
                       SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR,
                       &searchParam,
                       NULL,
                       &immSearchHandle);
       if (rc != SA_AIS_OK) {
               LOG_ER("%s saImmOmSearchInitialize FAIL %d", __FUNCTION__, rc);
               goto done;
       }

       /* Iterate the search until all objects found
        * When an object is found it is deleted
        */
       SaNameT object_name;
       SaImmAttrValuesT_2 **attributes;

       while ((rc = immutil_saImmOmSearchNext_2(immSearchHandle, &object_name, &attributes)) == SA_AIS_OK) {
               /* Delete the runtime object and its children. */
               rc = immutil_saImmOiRtObjectDelete(cb->immOiHandle, &object_name);
               if (rc == SA_AIS_OK) {
		       TRACE("saImmOiRtObjectDelete \"%s\" deleted Successfully", (char *) osaf_extended_name_borrow(&object_name));
	       } else {
		       LOG_ER("%s saImmOiRtObjectDelete for \"%s\" FAILED %d",
				       __FUNCTION__, (char *) osaf_extended_name_borrow(&object_name), rc);
	       }
       }

       if (rc != SA_AIS_ERR_NOT_EXIST) {
               LOG_ER("%s saImmOmSearchNext FAILED %d", __FUNCTION__, rc);
       }

done:
       rc = immutil_saImmOiImplementerClear(cb->immOiHandle);
       if (rc != SA_AIS_OK) {
               LOG_ER("saImmOiImplementerClear FAIL %d", rc);
       }

       /* Finalize the Om API
        */
       rc = immutil_saImmOmFinalize(immOmHandle);
       if (rc != SA_AIS_OK) {
               LOG_ER("saImmOmFinalize FAIL %d", rc);
       }

       /* Restore immutil settings */
       immutilWrapperProfile.errorsAreFatal = tmp_immutilWrapperProfile.errorsAreFatal;
       immutilWrapperProfile.nTries = tmp_immutilWrapperProfile.nTries;
       immutilWrapperProfile.retryInterval = tmp_immutilWrapperProfile.retryInterval;

       TRACE_LEAVE();
       return rc;
}

/****************************************************************************************
 * Name          : cpd_get_scAbsenceAllowed_attr
 *
 * Description   : This function gets scAbsenceAllowed attribute
 *
 * Arguments     : -
 * 
 * Return Values : scAbsenceAllowed attribute (0 = not allowed)
 *****************************************************************************************/
SaUint32T cpd_get_scAbsenceAllowed_attr()
{
	SaUint32T rc_attr_val = 0;
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAccessorHandleT accessorHandle;
	SaImmHandleT immOmHandle;
	SaImmAttrValuesT_2 *attribute;
	SaImmAttrValuesT_2 **attributes;

	TRACE_ENTER();

	char *attribute_names[] = {
		"scAbsenceAllowed",
		NULL
	};
	char object_name_str[] = "opensafImm=opensafImm,safApp=safImmService";

	SaNameT object_name;
	osaf_extended_name_lend(object_name_str, &object_name);

	/* Save immutil settings and reconfigure */
	struct ImmutilWrapperProfile tmp_immutilWrapperProfile;
	tmp_immutilWrapperProfile.errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	tmp_immutilWrapperProfile.nTries = immutilWrapperProfile.nTries;
	tmp_immutilWrapperProfile.retryInterval = immutilWrapperProfile.retryInterval;

	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.nTries = 500;
	immutilWrapperProfile.retryInterval = 1000;

	/* Initialize Om API */
	rc = immutil_saImmOmInitialize(&immOmHandle, NULL, &imm_version);
	if (rc != SA_AIS_OK) {
		LOG_ER("%s saImmOmInitialize FAIL %d", __FUNCTION__, rc);
		goto done;
	}

	/* Initialize accessor for reading attributes */
	rc = immutil_saImmOmAccessorInitialize(immOmHandle, &accessorHandle);
	if (rc != SA_AIS_OK) {
		LOG_ER("%s saImmOmAccessorInitialize Fail %s", __FUNCTION__, saf_error(rc));
		goto done;
	}


	rc = immutil_saImmOmAccessorGet_2(accessorHandle, &object_name, attribute_names, &attributes);
	if (rc != SA_AIS_OK) {
		TRACE("%s saImmOmAccessorGet_2 Fail '%s'", __FUNCTION__, saf_error(rc));
		goto done_fin_Om;
	}

	void *value;

	/* Handle the global scAbsenceAllowed_flag */
	attribute = attributes[0];
	TRACE("%s attrName \"%s\"",__FUNCTION__,attribute?attribute->attrName:"");
	if ((attribute != NULL) && (attribute->attrValuesNumber != 0)) {
		/* scAbsenceAllowed has value. Get the value */
		value = attribute->attrValues[0];
		rc_attr_val = *((SaUint32T *) value);
	}

	done_fin_Om:
	/* Free Om resources */
	rc = immutil_saImmOmFinalize(immOmHandle);
	if (rc != SA_AIS_OK) {
		TRACE("%s saImmOmFinalize Fail '%s'", __FUNCTION__, saf_error(rc));
	}

	done:
	/* Restore immutil settings */
	immutilWrapperProfile.errorsAreFatal = tmp_immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.nTries = tmp_immutilWrapperProfile.nTries;
	immutilWrapperProfile.retryInterval = tmp_immutilWrapperProfile.retryInterval;

	TRACE_LEAVE();
	return rc_attr_val;
}
