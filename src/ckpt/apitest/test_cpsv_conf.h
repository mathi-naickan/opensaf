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
 */

#ifndef CKPT_APITEST_TEST_CPSV_CONF_H_
#define CKPT_APITEST_TEST_CPSV_CONF_H_

typedef enum {
  TEST_NONCONFIG_MODE = 0,
  TEST_CONFIG_MODE,
  TEST_USER_DEFINED_MODE,
}CONFIG_FLAG;

typedef enum {
  TEST_NDRESTART = 1,
  TEST_SWITCHOVER,
  TEST_FAILOVER,
  TEST_MULTI_SO,
  TEST_MULTI_FO,
  TEST_MANUAL,
  TEST_NDRESTART_MANUAL,
  TEST_SOS_MANUAL,
  TEST_FAILOVER_MANUAL,
}RED_FLAG;
                                                                                                                             
typedef enum {
   CPSV_CLEAN_INIT_SUCCESS_T,
   CPSV_CLEAN_INIT_NULL_CBKS_T,
   CPSV_CLEAN_INIT_OPEN_NULL_CBK_T,
   CPSV_CLEAN_INIT_SYNC_NULL_CBK_T,
   CPSV_CLEAN_INIT_NULL_CBKS2_T,
   CPSV_CLEAN_INIT_SUCCESS_HDL2_T,
}CPSV_CLEANUP_TC_TYPE;

typedef enum {
   CPSV_CLEAN_ASYNC_ALL_MODES_SUCCESS_T,
   CPSV_CLEAN_ALL_REPLICAS_CKPT,
   CPSV_CLEAN_ACTIVE_REPLICAS_CKPT,
   CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT,
   CPSV_CLEAN_ALL_COLLOCATED_REPLICAS_CKPT,
   CPSV_CLEAN_WEAK_COLLOCATED_REPLICAS_CKPT,
   CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT,
   CPSV_CLEAN_ASYNC_ACTIVE_REPLICAS_CKPT,
   CPSV_CLEAN_WEAK_REPLICAS_CKPT,
   CPSV_CLEAN_MULTI_VECTOR_CKPT,
   CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT_EXTENDED_NAME,
}CPSV_CLEANUP_CKPT_TC_TYPE;


struct cpsv_testcase_data
{
  NCSCONTEXT thread_handle,thread_handle1,thread_handle2,thread_handle3,thread_handle4,thread_handle5;

  SaCkptHandleT ckptHandle,ckptHandle2,ckptHandle3,ckptHandle4,ckptHandle5,uninitHandle,junkHandle1,junkHandle2,junkHandle3;
  SaCkptCallbacksT general_callbks,open_null_callbk,synchronize_null_callbk,null_callbks;
  SaVersionT version_supported,version_err,version_err2;
  SaSelectionObjectT selobj;

  SaCkptCheckpointCreationAttributesT all_replicas,smoke_test_replica,active_replica,replica_weak,collocated;
  SaCkptCheckpointCreationAttributesT ckpt_all_collocated_replica,ckpt_weak_collocated_replica,multi_io_replica;
  SaCkptCheckpointCreationAttributesT invalid,invalid2,invalid3,invalid4,invalid_collocated,my_app;

  SaCkptCheckpointHandleT all_replicas_Createhdl,all_replicas_Writehdl,all_replicas_Readhdl,all_replicas_create_after_unlink;
  SaCkptCheckpointHandleT active_replica_Createhdl,active_replica_Writehdl,active_replica_Readhdl,weak_replica_Createhdl,weak_replica_Writehdl;
  SaCkptCheckpointHandleT weak_replica_Readhdl,active_colloc_Createhdl,active_colloc_create_test,active_colloc_sync_nullcb_Writehdl;
  SaCkptCheckpointHandleT active_colloc_Readhdl,active_colloc_Writehdl,allflags_set_hdl,all_collocated_Createhdl,all_collocated_Writehdl;
  SaCkptCheckpointHandleT all_collocated_Readhdl,weak_collocated_Createhdl,weak_collocated_Writehdl,weak_collocated_Readhdl;
  SaCkptCheckpointHandleT multi_io_hdl,uninitckptHandle,testHandle,async_all_replicas_hdl,open_clbk_hdl;

  SaNameT all_replicas_ckpt,active_replica_ckpt,weak_replica_ckpt,collocated_ckpt,async_all_replicas_ckpt,async_active_replica_ckpt;
  SaNameT smoketest_ckpt,all_collocated_ckpt,weak_collocated_ckpt,non_existing_ckpt,multi_vector_ckpt;
  SaNameT all_replicas_ckpt_large,active_replica_ckpt_large,weak_replica_ckpt_large,collocated_ckpt_large;
  SaNameT all_replicas_ckpt_with_valid_extended_name_length;
  SaNameT all_replicas_ckpt_with_invalid_extended_name_length;

  SaCkptSectionIdT section1,section2,section3,section4,section5,section6,section7,invalid_sec,invalidsection,gen_sec,invalidSection,gen_sec_del;
  SaCkptSectionIdT long_section_id;
  SaCkptSectionIdT too_long_section_id;
  SaCkptSectionCreationAttributesT general_attr,expiration_attr,section_attr,special_attr,special_attr2,special_attr3,invalid_attr,multi_attr;
  SaCkptSectionCreationAttributesT section_attr_with_long_id;
  SaCkptSectionCreationAttributesT section_attr_with_too_long_id;
  char data1[14],data2[14],data3[14];
  SaSizeT size,size_zero;

  SaUint8T *sec_id1, *sec_id2, *sec_id3, *sec_id4, *inv_sec;
  char filedata1[600],filedata2[600],filedata3[600];
  char buffer[200],buffer_large[400];
  char buffer1[200],buffer2[200],buffer3[200];

  SaCkptIOVectorElementT generate_write,generate_write1,generate_write_large,general_write,general_write2,multi_vector[3],invalid_write,default_write,invalid_write2,
                           err_no_res;
  SaCkptIOVectorElementT generate_read,generate_read_large,general_read,default_read,multi_read_vector[3],invalid_read,invalid_read2,invalid_read3,buffer_null;

  SaUint32T nOfE,nOfE2;
  SaCkptSectionDescriptorT secDesc;
  SaCkptCheckpointDescriptorT status;
  SaSizeT memLeft;
  SaUint32T ind;
  uint32_t nodeId;
  RED_FLAG redFlag;
  int inst_num;
  char overdata[100];
  SaSizeT size1,size2;

  SaCkptSectionIterationHandleT secIterationHandle, secIterationHandle2, secjunkHandle,badHandle;
  SaCkptSectionsChosenT sec_forever,sec_any,exp_leq,exp_geq,sec_corrupt,sec_invalid;
  SaNameT invalidName2,invalidName;
  SaInvocationT open_clbk_invo;
  SaInvocationT sync_clbk_invo;
  SaAisErrorT open_clbk_err;
  SaAisErrorT sync_clbk_err;
  int arr_clbk_flag;
  int arr_clbk_err;
};

struct cpsv_testcase_data tcd;

extern int test_ckptInitialize(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptSelectionObject(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptDispatch(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptFinalize(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptOpen(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptOpenAsync(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptSectionCreate(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptReplicaSet(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptStatusGet(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptSectionDelete(int i,CONFIG_FLAG cfg_flg);
extern int test_saCkptSectionIdFree(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptUnlink(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptClose(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptExpirationSet(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptDurationSet(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptWrite(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptRead(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptOverwrite(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptSynchronize(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptSynchronizeAsync(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptIterationInit(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptIterationNext(int i,CONFIG_FLAG cfg_flg);
extern int test_ckptIterationFin(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptInitialize(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptSelectionObject(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptDispatch(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptFinalize(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptOpen(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptOpenAsync(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptSectionCreate(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptReplicaSet(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptStatusGet(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptSectionDelete(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptUnlink(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptClose(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptExpirationSet(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptDurationSet(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptWrite(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptRead(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptOverwrite(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptSynchronize(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptSynchronizeAsync(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptIterationInit(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptIterationNext(int i,CONFIG_FLAG cfg_flg);
extern int test_red_ckptIterationFin(int i,CONFIG_FLAG cfg_flg);
extern void selection_thread_blocking(NCSCONTEXT arg);
extern void cpsv_createthread(SaCkptHandleT *cl_hdl);

extern void printHead(char *str);
extern void cpsv_clean_clbk_params();
extern void printResult(int result);
extern void AppCkptArrivalCallback(const SaCkptCheckpointHandleT ckptArrivalHandle, SaCkptIOVectorElementT *ioVector, SaUint32T num_of_elem);
extern void test_cpsv_cleanup(CPSV_CLEANUP_TC_TYPE i);
extern void test_ckpt_cleanup(CPSV_CLEANUP_CKPT_TC_TYPE i);
extern void test_red_cpsv_cleanup(CPSV_CLEANUP_TC_TYPE i);
extern void test_red_ckpt_cleanup(CPSV_CLEANUP_CKPT_TC_TYPE i);
extern int cpsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,CONFIG_FLAG flg);
extern void fill_ckpt_attri(SaCkptCheckpointCreationAttributesT *cr_attr,SaCkptCheckpointCreationFlagsT cr_flags,SaSizeT ckptSize, SaTimeT ret,
                                                         SaUint32T max_sec, SaSizeT max_sec_size, SaSizeT max_sec_id_size);

extern void fill_write_iovec(SaCkptIOVectorElementT *iovec,SaCkptSectionIdT sec,char *buf,SaSizeT size,SaOffsetT offset,SaSizeT read_size);
extern void fill_read_iovec(SaCkptIOVectorElementT *iovec,SaCkptSectionIdT sec,char *buf,SaSizeT size,SaOffsetT offset,SaSizeT read_size);

extern void cpsv_it_expset_02();
extern void cpsv_redundancy_func(RED_FLAG i);
extern SaAisErrorT ncsCkptRegisterCkptArrivalCallback(SaCkptHandleT lckHandle,ncsCkptCkptArrivalCallbackT arrivalCallbck);

extern int gl_sync_pointnum;
extern int fill_syncparameters(int vote);






#endif  // CKPT_APITEST_TEST_CPSV_CONF_H_

