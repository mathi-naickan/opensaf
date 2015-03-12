/*          OpenSAF
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
 * Author(s): Ericsson AB
 *
 */

/* The test framework */
#include <utest.h>
#include <util.h>

#include <stdio.h>
#include <stdlib.h>
#include <mds_papi.h>
#include <ncs_mda_papi.h>
#include "ncs_main_papi.h"
#include "mdstipc.h"
#include "ncssysf_tmr.h"

#define MSG_SIZE 65479
static MDS_CLIENT_MSG_FORMAT_VER gl_set_msg_fmt_ver;

MDS_SVC_ID svc_ids[3]={2006,2007,2008};

/*****************************************************************************/
/************        SERVICE API TEST CASES   ********************************/
/*****************************************************************************/
void tet_vdest_install_thread()
{
  printf(" Inside Thread\n");
  if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          1,
                          NCSMDS_SCOPE_NONE,
                          true,
                          false)!=NCSCC_RC_SUCCESS)
  {
    printf("Install Failed");
  }
}

void  tet_svc_install_tp_01()
{
  int FAIL = 0;
  //Creating a MxN VDEST with id = 2000
  if (create_vdest(NCS_VDEST_TYPE_MxN,2000) !=  NCSCC_RC_SUCCESS) {
    printf("\nERROR: Fail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 1: Installing service 500 twice\n");
  //Installing the service 500
  if (mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                          500,
                          1,
                          NCSMDS_SCOPE_NONE,
                          true,
                          false) !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Installing the service 500\n");
  }

  //Installing the service 500 again.
  //It shall not possible to install the ALREADY installed service 500
  if (mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,500,
                          1,
                          NCSMDS_SCOPE_NONE,
                          true,
                          false) != NCSCC_RC_FAILURE) {
    printf("\npossible to install the ALREADY installed service 500\n");
    FAIL = 1;
  }

  //printf("\nUninstalling the above service\n");
  if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,500) !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Uninstalling the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  if (destroy_vdest(2000) !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_02()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 2: Installing the Internal MIN service INTMIN with NODE scope without MDS Queue ownership and failing to Retrieve");
  //Installing the Internal MIN service INTMIN with NODE scope without MDS Q ownership and failing to Retrieve
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           false,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Install the Internal MIN service INTMIN with NODE scope without MDS Q ownership\n");
    FAIL = 1;
  }

  //Retrieve shall fail
  if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL) != NCSCC_RC_FAILURE) 
  {
    printf("\nFail, Possible to retrieve\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_03()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }
  printf("\nTest case 3: Installing the External MIN service EXTMIN with CHASSIS scope");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Installing the External MIN service EXTMIN with CHASSIS scope\n");
    FAIL = 1;
  }

  //Uninstalling the above service\n");
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);

  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Uninstalling the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_05()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 5: Installing the service with MAX id:32767 with CHASSIS scope");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_MAX_SVCS,
                           1,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Installing the service with MAX id:32767 with CHASSIS scope\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_MAX_SVCS);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Uninstalling the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_06()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 6:Not able to Install the service with id > MAX:32767 with CHASSIS scope\n");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_MAX_SVCS+1,
                           1,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           false);
  if (rc != NCSCC_RC_FAILURE) {
    printf("\nPossible to Install the service with id > MAX:32767 with CHASSIS scope\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_07()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 7: Not able to Install the service with id =0");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           0,
                           1,
                           NCSMDS_SCOPE_NONE,
                           false,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to Install the service with id =0\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_08()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 8:Not able to Install the service with Invalid PWE Handle");
  rc = mds_service_install((MDS_HDL)(long)NULL,
                           100,
                           1,
                           NCSMDS_SCOPE_NONE,
                           false,
                           false);

  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to Install the service with Invalid (NULL) PWE Handle\n");
    FAIL = 1;
  }

  rc = mds_service_install(0,
                           100,
                           1,
                           NCSMDS_SCOPE_NONE,
                           false,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to Install the service with Invalid (Zero) PWE Handle\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_vdest[0].mds_vdest_hdl,
                           100,
                           1,
                           NCSMDS_SCOPE_NONE,
                           false,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to Install the service with Invalid PWE Handle\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_09()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 9:Not able to Install a Service with Svc id > External  MIN 2000 which doesn't chose MDS Q Ownership");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN+1,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           false,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nAble to Install a Service with Svc id > External  MIN 2000 which doesn't chose MDS Q Ownership\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_10()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 10:Installing the External MIN service EXTMIN in a seperate thread and Uninstalling it here\n");
  //Install thread
  rc = tet_create_task((NCS_OS_CB)tet_vdest_install_thread,
                       gl_tet_vdest[0].svc[0].task.t_handle);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Install thread\n");
    FAIL = 1;
  }

  sleep(2);
  //Now Release the Install Thread
  rc = m_NCS_TASK_RELEASE(gl_tet_vdest[0].svc[0].task.t_handle);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to release thread\n");
    FAIL = 1;
  }

  //Counter shall be != 0
  if (gl_tet_vdest[0].svc_count == 0) {
    printf("\nsvc_count == 0\n");
    FAIL = 1;
  };
 
  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Uninstalling the above service\n");
    FAIL = 1;
  }
  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_11()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case:Installation of the same Service id with different sub-part versions on the same pwe handle, must fail\n");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN ,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Installing the service\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN ,
                           2,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to install the same Service id with different sub-part versions on the same pwe handle\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstalling the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_12()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 12: Installation of the same Service id with same sub-part versions on the same pwe handle, must fail\n");  
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to create service\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to install the same Service id with same sub-part versions on the same pwe handle\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall the above service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_13()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 13: Install a service with _mds_svc_pvt_ver = 0 i_mds_svc_pvt_ver = 255 and i_mds_svc_pvt_ver = A random value, which is >0 and <255\n");
  rc = adest_get_handle();
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to get handle\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN ,
                           0,
                           NCSMDS_SCOPE_NONE,
                           true,
                           true);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_NONE\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl, NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_NONE\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN ,
                           255,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           true);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_INTRACHASSIS\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_INTRACHASSIS\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN ,
                           125,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }
  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_14()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 14: Install a service with i_fail_no_active_sends = 0 and i_fail_no_active_sends = 1\n");
  rc = adest_get_handle();
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to get handle\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_NONE,
                           true,
                           false); //i_fail_no_active_sends = 0
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_NONE\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_NONE\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           true); //i_fail_no_active_sends = 1
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_INTRACHASSIS\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_INTRACHASSIS\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_15()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 15: Installation of a service with same service id and i_fail_no_active_sends twice on the same pwe handle, must fail\n");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true); //i_fail_no_active_sends
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true); //i_fail_no_active_sends
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to install service NCSMDS_SCOPE_INTRANODE twice\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_16()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 16: Installation of a service with same service id and different i_fail_no_active_sends again on the same pwe handle, must fail\n");
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true); //i_fail_no_active_send = 1
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install the service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }

  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false); //i_fail_no_active_send = 0
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to install a service with same service id and different i_fail_no_active_sends again on the same pwe handle\n");
    FAIL = 1;
  }

  //Uninstalling the above service\n");
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall the service NCSMDS_SCOPE_INTRANODE\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void  tet_svc_install_tp_17()
{
  int FAIL = 0;
  SaUint32T rc;
  //Creating a MxN VDEST with id = 2000
  rc = create_vdest(NCS_VDEST_TYPE_MxN,2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Creating a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  printf("\nTest case 17: For MDS_INSTALL API, supply i_yr_svc_hdl = (2^32) and i_yr_svc_hdl = (2^64 -1)\n");
  gl_tet_svc.yr_svc_hdl = 4294967296ULL; 
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SVC_ID_EXTERNAL_MIN\n");
    FAIL = 1;
  }

  gl_tet_svc.yr_svc_hdl = 18446744073709551615ULL;
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_INTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           true);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service NCSMDS_SVC_ID_INTERNAL_MIN\n");
    FAIL = 1;
  }

  //Uninstalling the above services
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SVC_ID_EXTERNAL_MIN\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,NCSMDS_SVC_ID_INTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall service NCSMDS_SVC_ID_INTERNAL_MIN\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 2000
  rc = destroy_vdest(2000);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_vdest_uninstall_thread()
{
  // Inside Thread
  printf("tet_vdest_uninstall_thread\n");
  test_validate(mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,500), 
		NCSCC_RC_SUCCESS);
}

void tet_svc_unstall_tp_1()
{
  int FAIL = 0;
  SaUint32T rc;
  printf("\nTest Case 1: Not able to Uninstall an ALREADY uninstalled service\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to creating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  //Installing the service 500
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           500,
                           1,
                           NCSMDS_SCOPE_INTRANODE,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install the service 500\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,500);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to uninstall the service 500\n");
    FAIL = 1;
  }

  //Not able to Uninstall an ALREADY uninstalled service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,500);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to uninstall already uninstalled service 500\n");
    FAIL = 1;
  }
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 2000\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_unstall_tp_3()
{
  int FAIL = 0;
  SaUint32T rc;
  printf("\nTest Case 3: Not able to Uninstall a Service which doesn't exist\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,600);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to uninstall a service which doesn't exist\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_unstall_tp_4()
{
  int FAIL = 0;
  SaUint32T rc;
  printf("\nTest Case 4: UnInstalling wit invalid handles\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  //Installing a service EXTMIN
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           1,
                           NCSMDS_SCOPE_INTRACHASSIS,
                           true,
                           false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install a service EXTMIN\n");
    FAIL = 1;
  }

  //Not able to Uninstall the above Service with NULL PWE handle
  rc = mds_service_uninstall((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to uninstall the Service with NULL PWE handle\n");
    FAIL = 1;
  }

  //Not able to Uninstall the above Service with ZERO PWE handle
  rc = mds_service_uninstall(0,NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to uninstall the Service with ZERO PWE handle\n");
    FAIL = 1;
  }

  //Not able to Uninstall the above Service with VDEST handle\n");
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_vdest_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_FAILURE) {
    printf("\nPossible to uninstall the Service with VDEST handle\n");
    FAIL = 1;
  }

  //Uninstalling the above service
  rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Uninstall the MDS service\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_unstall_tp_5()
{
  int FAIL = 0;
  SaUint32T rc;
  printf("\nTest Case 5: Uninstalling a service in a seperate thread\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  //Installing the service 500
  rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,500,1,
                           NCSMDS_SCOPE_INTRANODE,true,false);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to install service 500\n");
    FAIL = 1;
  }

  //Uninstalling the above service in a seperate thread
  //Uninstall thread
  rc = tet_create_task((NCS_OS_CB)tet_vdest_uninstall_thread,
                       gl_tet_vdest[0].svc[0].task.t_handle);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to create the uninstall thread\n");
    FAIL = 1;
  }

  sleep(2);

  //Now Release the Uninstall Thread
  rc = m_NCS_TASK_RELEASE(gl_tet_vdest[0].svc[0].task.t_handle);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to release the uninstall thread\n");
    FAIL = 1;
  }

  //Test gl_tet_vdest[0].svc_count == 0
  if (gl_tet_vdest[0].svc_count != 0) {
    printf("\nsvc_count is %d, should be 0\n", gl_tet_vdest[0].svc_count);
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_install_upto_MAX()
{
  int id=0;
  gl_vdest_indx=0;
  int FAIL = 0;
  SaUint32T rc;
  printf("\nTest Case 6: svc_install_upto_MAX\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nNo. of Services = %d\n",gl_tet_vdest[0].svc_count);
  fflush(stdout);
  sleep(1);
  /*With MDS Q ownership*/
  printf("\tInstalling the services from SVC_ID_INTERNAL_MIN to +500\n");
  for(id=NCSMDS_SVC_ID_INTERNAL_MIN+1; id<=NCSMDS_SVC_ID_INTERNAL_MIN+500; id++)
  {
    rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                             id,
                             1,
                             NCSMDS_SCOPE_NONE,
                             true,
                             false);
    if (rc != NCSCC_RC_SUCCESS)
      break;
  }

  printf("\tMAX number of service that can be created on a VDEST = %d",
         gl_tet_vdest[0].svc_count);
  printf("\nNo. of Services= %d\n",gl_tet_vdest[0].svc_count);fflush(stdout);
  sleep(4);
  printf("\tUninstalling the above service\n");
  for(id=gl_tet_vdest[0].svc_count-1; id>=0; id--)
  {
    rc = mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                               gl_tet_vdest[0].svc[id].svc_id);
    if (rc !=  NCSCC_RC_SUCCESS) {
      printf("\nFail to uninstall a service id %d\n", id);
      FAIL = 1;
    }
  }

  printf("\nNo. of Services = %d\n",gl_tet_vdest[0].svc_count);
  fflush(stdout);
  sleep(1);

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

static int  tet_verify_version(MDS_HDL mds_hdl,NCSMDS_SVC_ID your_scv_id,NCSMDS_SVC_ID req_svc_id,
                               MDS_SVC_PVT_SUB_PART_VER svc_pvt_ver,NCSMDS_CHG change)  
{
  int i,j,k,ret_val=0;
  if(is_service_on_adest(mds_hdl,your_scv_id)==NCSCC_RC_SUCCESS)
  { 
    for(i=0;i<gl_tet_adest.svc_count;i++)
    {
      if(gl_tet_adest.svc[i].svc_id==your_scv_id)
      {
        for(j=0;j<gl_tet_adest.svc[i].subscr_count;j++)
        {
          if(gl_tet_adest.svc[i].svcevt[j].svc_id==req_svc_id)
          {
            if((gl_tet_adest.svc[i].svcevt[j].event==change) &&
               (gl_tet_adest.svc[i].svcevt[j].rem_svc_pvt_ver == svc_pvt_ver))
              ret_val= 1; 
            else
              ret_val= 0; 
                    

          }

        }
      }
    }
  }
  else
  {
    for(i=0;i<gl_vdest_indx;i++)
    {
      for(j=0;j<gl_tet_vdest[i].svc_count;j++)
      {
        if(gl_tet_vdest[i].svc[j].svc_id==your_scv_id)
        {
          for(k=0;k<gl_tet_vdest[i].svc[j].subscr_count;k++)
          {
            if(gl_tet_vdest[i].svc[j].svcevt[k].svc_id==req_svc_id)
            {
              if((gl_tet_vdest[i].svc[j].svcevt[k].event==change) &&
                 (gl_tet_vdest[i].svc[j].svcevt[k].rem_svc_pvt_ver == svc_pvt_ver))
                ret_val= 1;
              else
                ret_val= 0;

            }
          }

        }
      }
    }
  }
  return ret_val;
}
void tet_svc_subscr_VDEST_1()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 1: 500 Subscription to:600,700 where Install scope = Subscription scope\n");

  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }
  
  printf("\nInstalling the service 500,600,700 with CHASSIS scope\n");      
  for(id=500;id<=700;id=id+100)
  {  
    rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                             id,
                             1,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             true,
                             false);
    if (rc !=  NCSCC_RC_SUCCESS) {
      printf("\nFail to install service %d\n", id);
      FAIL = 1;
    }
  }

  printf("\n500 Subscription to:600,700 where Install scope = Subscription scope");
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             2,
                             svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to subscribe service 500\n");
    FAIL = 1;
  }

  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,
                                       500,
                                       2,
                                       svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to cancel subscription service 500\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
	  if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                          gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
	  {
	    printf("\nFail mds_service_uninstall\n");
	    FAIL = 1;
	  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc != NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_2()
{
  bool FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 2: 500 Subscription to:600,700 where Install scope >Subscription scope (NODE)\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }
 
  printf("\nInstalling the service 500,600,700 with CHASSIS scope\n");      
  for(id=500;id<=700;id=id+100)
  {  
    rc = mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                             id,
                             1,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             true,
                             false);
    if (rc != NCSCC_RC_SUCCESS) {
      printf("\nFail to install service %d\n", id);
      FAIL = 1;
    }
  }

  /*install scope > subscribe scope*/
  printf("\n500 Subscription to:600,700 where Install scope > Subscription scope (NODE)\n");
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRANODE,
                             2,
                             svcids);
  if (rc != NCSCC_RC_SUCCESS) {
    printf("\nFail to subscribe service 500\n");
    FAIL = 1;
  }

  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,
                                       500,
                                       2,
                                       svcids);
  if (rc != NCSCC_RC_SUCCESS) {
    printf("\nFail to cancel subscription service 500\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
	  if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
                          gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
	  {
            printf("\nFail mds_service_uninstall\n");
            FAIL = 1;
          }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_3()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 3: Not able to: 500 Subscription to:600,700 where Install scope < Subscription scope(PWE\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail creating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                            id,
                            1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }
 
  //install scope < subscribe scope
  //Not able to: 500 Subscription to:600,700 where Install scope < Subscription scope(PWE)
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_NONE,
                             2,
                             svcids);
  if (rc ==  NCSCC_RC_SUCCESS) {
    printf("\nFail: Able to 500 Subscription to:600,700 where Install scope < Subscription scope(PWE)\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
			      gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_4()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 4: Not able to: 500 Subscription to:600,700 where Install scope < Subscription scope(PWE)\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                            id,
                            1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  //Not able to subscribe again:500 Subscribing to service 600, 700");
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             2,
                             svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to subscribe service 500\n");
    FAIL = 1;
  }

  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             1,
                             svcids);
  if (rc ==  NCSCC_RC_SUCCESS) {
    printf("\nPossible to subscribe service 500 twice\n");
    FAIL = 1;
  }


  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,500,
                                       2,svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to cancel subscription on service 500\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
	  if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
				    gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_5()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 5: Not able to Cancel the subscription which doesn't exist\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,id,1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false) != NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  //Not able to Cancel the subscription which doesn't exist
  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,
                                       500,
                                       2,
                                       svcids);
  if (rc ==  NCSCC_RC_SUCCESS) {
    printf("\nPossible to Cancel the subscription which doesn't exist\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
			      gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
} 

void tet_svc_subscr_VDEST_6()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID itself[]={500};
  printf("\nTest Case 6: A service subscribing for Itself\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                            id,
                            1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  //A service subscribing for Itself
  rc =  mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                              500,
                              NCSMDS_SCOPE_INTRACHASSIS,
                              1,
                              itself);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to subscribe for itself (service 500)");
    FAIL = 1;
  }

  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,
                                       500,
                                       1,
                                       itself);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to cancel subscription");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
			      gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_8()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 8: Able to subscribe when numer of services to be subscribed is = 0\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                            id,
                            1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  //Able to subscribe when numer of services to be subscribed is = 0\n");
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             0,
                             svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to subscribe when numer of services to be subscribed is = 0\n");
    FAIL = 1;
  }


  rc = mds_service_cancel_subscription(gl_tet_vdest[0].mds_pwe1_hdl,
                                       500,
                                       0,
                                       svcids);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to cancel subscription\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
			      gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_9()
{
  int FAIL = 0;
  SaUint32T rc;
  int id;
  printf("\nTest Case 9: Not able to subscribe when supplied services array is NULL\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nInstalling the service 500,600,700 with CHASSIS scope");      
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                            id,
                            1,
                            NCSMDS_SCOPE_INTRACHASSIS,
                            true,
                            false)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  //It shall not able to subscribe when supplied services array is NULL
  rc = mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,
                             500,
                             NCSMDS_SCOPE_INTRACHASSIS,
                             1,
                             NULL);
  if (rc ==  NCSCC_RC_SUCCESS) {
    printf("\nPossible to subscribe when supplied services array is NULL\n");
    FAIL = 1;
  }

  //clean up
  //Uninstalling all the services on this VDEST
  for(id=gl_tet_vdest[0].svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,
			      gl_tet_vdest[0].svc[id].svc_id) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail mds_service_uninstall\n");
      FAIL = 1;
    }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_7()
{
  int FAIL = 0;
  SaUint32T rc;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 7: A Service which is NOT installed; trying to subscribe for 600, 700\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  if( mds_service_subscribe(gl_tet_vdest[0].mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_10()
{
  bool FAIL = false;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  printf("\nTest Case 10: Cross check whether i_rem_svc_pvt_ver returned in service event callback, matches the service private sub-part version of the remote service (subscribee)\n");
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  //Cross check whether i_rem_svc_pvt_ver returned in service event callback, matches the service private sub-part version of the remote service (subscribee)
  //Getting an ADEST handle
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to get handle\n");
    FAIL = 1;
  }

  gl_tet_adest.svc_count=0; /*fix for blocking regression*/
  //Installing the services
  if(mds_service_install(gl_tet_adest.mds_pwe1_hdl,500,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to install service 500\n");
    FAIL = 1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,600,1,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to install service 600\n");
    FAIL = 1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,700,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to install service 700\n");
    FAIL = 1;
  }

  //Subscribing for the services
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to subscribe service 500\n");
    FAIL = 1;
  }

  //verifying the rem svc ver from 600 and 700
  //Changing the role of vdest to active
  if(vdest_change_role(1001,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to change role\n");
    FAIL = 1;
  }
  //Retrieving the events
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nRetrieve servive 500 Fail\n");
    FAIL = 1;
  }

  //Verifying for the versions for UP event
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_UP))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,2,NCSMDS_UP)!=1))
  {
    printf("\nFail to verifying for the versions for UP event\n");
    FAIL = 1;
  }
  //Changing the role of vdest role to quiesced
  if(vdest_change_role(1001,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to change the role of vdest role to quiesced\n");
    FAIL = 1;
  }
  //Retrieving the events
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to retreive events\n");
    FAIL = 1;
  }
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_NO_ACTIVE))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,2,NCSMDS_NO_ACTIVE)!=1))
  {
    printf("\nFail to verifying for the versions for NO_ACTIVE event\n");
    FAIL = 1;
  }
  //Changing the role of vdest role to standby
  if(vdest_change_role(1001,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to change the role of vdest role to standby\n");
    FAIL = 1;
  }
  printf(" \n Sleeping for 3 minutes \n");
  sleep(200);
  //Retrieving the events
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nRetrieve Fail\n");
    FAIL = 1;
  }
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,0,NCSMDS_DOWN))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,0,NCSMDS_DOWN)!=1))
  {
    printf("\nFail to verifying for the versions for DOWN event\n");
    FAIL = 1;
  }

  printf("\tUninstalling 600 and 700\n");
  for(id=600;id<=700;id=id+100)
  {
    if(  mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,id)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail to uninstall service %d\n", id);
      FAIL = 1;
    }
  }

  //Cancelling the subscription
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail cancel subscription 500\n");
    FAIL = 1;
  }
  if(  mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,500)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to uninstall service 500\n");
    FAIL = 1;
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_11()
{
  bool FAIL = 0;
  SaUint32T rc;
  int id;
  MDS_SVC_ID svcids[]={600,700};
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nTest Case 11: When the Subscribee comes again with different sub-part version, cross check these changes are properly returned in the service event callback\n");
  printf("/nGetting an ADEST handle");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to get handle\n");
    FAIL=1;
  }

  gl_tet_adest.svc_count=0; /*fix for blocking regression*/
  printf("\nInstalling the services");
  if(mds_service_install(gl_tet_adest.mds_pwe1_hdl,500,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to installing the servic 500s\n");
    FAIL=1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,600,1,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to installing the servic 600s\n");
    FAIL=1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,700,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to installing the servic 700s\n");
    FAIL=1;
  }
  printf("\nSubscribing for the services");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail to subscribing for the service 500\n");
    FAIL=1;
  }
  /* verifying the rem svc ver from 600 and 700*/
  printf("\nChanging the role of vdest to active");
  if(vdest_change_role(1001,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\nRetrieving the events\n");
  sleep(1);
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nVerifying for the versions for UP event\n");
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_UP))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,2,NCSMDS_UP)!=1))
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nUninstalling 600 and 700\n");
  for(id=600;id<=700;id=id+100)
  {
    if(  mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,id)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,600,3,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,700,4,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  sleep(1);
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("Retrieve Fail\n");
    FAIL=1;
  }

  printf("\tVerifying for the versions for second  UP event\n");
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,3,NCSMDS_NEW_ACTIVE))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,4,NCSMDS_NEW_ACTIVE)!=1))
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\nUninstalling 600 and 700\n");
  for(id=600;id<=700;id=id+100)
  {
    if(  mds_service_uninstall(gl_tet_vdest[0].mds_pwe1_hdl,id)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
  }

  if(  mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,500)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_VDEST_12()
{
  int FAIL = 0;
  SaUint32T rc;
  int i;
  MDS_SVC_ID svc_id_sixhd[]={600};
  //Creating a N-way VDEST with id =1001
  rc = create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nCreating a N-way VDEST with id =1001\n");
    FAIL = 1;
  }

  printf("\nTest case 12: In the NO_ACTIVE event notification, the remote service subpart version is set to the last active instance.s remote-service sub-part version\n");
  printf("\nGetting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  gl_tet_adest.svc_count=0; /*fix for blocking regression*/
  printf("\nAction: Creating a N-way VDEST with id =1001\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,1002)!=
     NCSCC_RC_SUCCESS)
  {    printf("\nFail\n");FAIL=1;  }

  printf("\nAction: Installing the services\n");
  if(mds_service_install(gl_tet_adest.mds_pwe1_hdl,500,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,600,1,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_install(gl_tet_vdest[1].mds_pwe1_hdl,600,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Subscribing for the services\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,1,
                            svc_id_sixhd)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  /* verifying the rem svc ver from 600 and 700*/
  printf("\nAction: Changing the role of vdest 1001 to active\n");
  if(vdest_change_role(1001,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Retrieving the events\n");
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("Retrieve Fail\n");
    FAIL=1;
  }

  printf("\nAction: Verifying for the versions for UP event\n");
  if( tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_UP)!=1) 
   
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Changing the role of vdest 1002 to active\n");
  if(vdest_change_role(1002,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\tChanging the role of vdest 1001 to standby\n");
  if(vdest_change_role(1001,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Changing the role of vdest 1002 to standby\n");
  if(vdest_change_role(1002,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  sleep(1);
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("Retrieve Fail\n");
    FAIL=1;
  }
  printf("\nAction: Verifying for the versions for NO ACTIVE event\n");
  if( tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,2,NCSMDS_NO_ACTIVE)!=1)

  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Uninstalling 600 \n");
  for(i=0; i<=1;i++)
  {
    if(  mds_service_uninstall(gl_tet_vdest[i].mds_pwe1_hdl,600)!=NCSCC_RC_SUCCESS)

    {
      printf("\nFail\n");
      FAIL=1;
    }
  } 

  printf("\nAction: Cancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,1,
                                     svc_id_sixhd)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(  mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,500)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\nAction: Destroying the VDEST 1002\n");
  if(destroy_vdest(1002)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  //Destroying a MxN VDEST with id = 1001
  rc = destroy_vdest(1001);
  if (rc !=  NCSCC_RC_SUCCESS) {
    printf("\nFail to Destroying a MxN VDEST with id = 1001\n");
    FAIL = 1;
  }

  test_validate(FAIL, 0);
}

void tet_adest_cancel_thread()
{
  MDS_SVC_ID svcids[]={600,700};
  printf(" Inside Thread\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("Cancel Failed\n");
  }
}

void tet_adest_retrieve_thread()
{
  printf(" Inside Thread\n");
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)==NCSCC_RC_FAILURE)
  {
    printf("Retrieve Failed\n");
  }
}

bool prepare_ADEST_tc()
{
  int FAIL=0;
  int id;
  
  printf("\nGetting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  gl_tet_adest.svc_count=0; /*fix for blocking regression*/
  
  printf("\nInstalling the services 500,600,700 with CHASSIS scope\n");
  for(id=500;id<=700;id=id+100)
  {  
    if( mds_service_install(gl_tet_adest.mds_pwe1_hdl,id,1,
                            NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  if (FAIL == 1)
    return false;
  else
    return true;
}

void cleanup_ADEST_srv()
{
  int id;
  printf("\nUninstalling all the services on this ADESt\n");
  for(id=gl_tet_adest.svc_count-1;id>=0;id--)
    if (mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,
			      gl_tet_adest.svc[id].svc_id) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail mds_service_uninstall\n");
    }
}

void tet_svc_subscr_ADEST_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  printf("Test Case 1: 500 Subscription to:600,700 where Install scope = Subscription scope\n");
  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nAction: Retrieve only ONE event\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
  {
    printf("\nAction: Retrieve only ONE event\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                            SA_DISPATCH_ONE)!=NCSCC_RC_SUCCESS)
    {
      printf("Fail, retrieve ONE\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

    printf("\nAction: Retrieve ALL event\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("Retrieve ALL FAIL\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

    printf("\nAction: Cancel subscription 500\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nSuccess\n");
  }

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  /*install scope > subscribe scope*/
  printf("Test Case 2: 500 Subscription to:600,700 where Install scope > Subscription scope (NODE)\n");

  printf("\nAction: Subscribe srv 500\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRANODE,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
  {
    printf("\nAction: Retrieve with  Invalid NULL/0 PWE Handle\n");
    if(mds_service_retrieve((MDS_HDL)(long)NULL,500,SA_DISPATCH_ALL)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n"); 
      FAIL=1;
    }
    if(mds_service_retrieve(0,500,SA_DISPATCH_ALL)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    printf("\nAction: Not able to Retrieve Events for a Service which is not installed\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,100,
                            SA_DISPATCH_ALL)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail to mds_service_retrieve\n");
      FAIL=1; 
    }

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nSuccess\n");
  }
  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  /*install scope < subscribe scope*/
  printf("\nTest Case 3: Not able to: 500 Subscription to:600,700 where install scope < Subscription scope(PWE)\n");
  printf("\nAction: Subscribe srv 500\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_NONE,2,svcids)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else  
    printf("\nSuccess\n");

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nTest Case 4: Not able to subscribe again:500 Subscribing to service 600\n");

  printf("\nAction: Subscribe srv 500\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  printf("\nAction: Subscribe srv 500 again\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,1,
                            svcids)==NCSCC_RC_SUCCESS)
  {    
     printf("\nFail\n");
     FAIL=1; 
  }
  else
  {
    printf("\nAction: Retreive service\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS) 
    {
      printf("\nFail to mds_service_retrieve\n");
      FAIL=1; 
    }

    printf("\nAction: Cancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nSuccess\n");
  } 

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nTest Case 5: Not able to Cancel the subscription which doesn't exist\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                     svcids)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");  

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_6()
{
  int FAIL=0;
  MDS_SVC_ID itself[]={500};

  if (prepare_ADEST_tc() == false)
    FAIL=1;
  printf("\nTest Case 6: A service subscribing for Itself\n");
  printf("\n Action: Subscribe srv 500\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,1,
                            itself)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
  {
    printf("\n Action: Not able to Retrieve with Invalid Dispatch Flag\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                            4)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    printf("\n Action: Retreive service 500\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail mds_service_retrieve\n");
      FAIL=1; 
    }

    printf("\n Action: Cancel subscription 500\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,1,
                                       itself)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }  
    printf("\nSuccess\n");
  }

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};
 
  printf("\nTest Case 7 : A Service which is NOT installed; trying to subscribe for 600, 700\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRANODE,2,
                            svcids)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_8()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  /*install scope = subscribe scope*/
  printf("\nTest Case 8: 500 Subscription to:600,700 Cancel this Subscription in a seperate thread\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  else
  {
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                            SA_DISPATCH_ALL)==NCSCC_RC_FAILURE)
    {
      printf("Retrieve Fail\n");
      FAIL=1;
    }
    printf("\nAction: Cancel in a seperate thread\n");
    if(tet_create_task((NCS_OS_CB)tet_adest_cancel_thread,
                       gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    sleep(2);
    fflush(stdout);
    printf("\nAction: Release the Cancel Thread\n");
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nTASK is released\n");
    }
    if(gl_tet_adest.svc[0].subscr_count)
    {
      printf("Cancel Fail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
  }

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_9()
{
  int FAIL=0;
  MDS_SVC_ID one[]={600};
  MDS_SVC_ID two[]={700};
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nTest Case 9: 500 Subscription to:600,700 in two seperate Subscription calls but Cancels both in a single cancellation call\n");
  printf("\nAction: Subscribe 500 to 600\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,1,
                            one)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\nAction: Subscribe 500 to 700\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,1,
                            two)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  else
  {
    printf("\nAction: Retreive three times, third shall fail\n");
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,SA_DISPATCH_ONE) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
      FAIL=1;
    }

    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,SA_DISPATCH_ONE) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
      FAIL=1;
    }

    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                            SA_DISPATCH_ONE)==NCSCC_RC_SUCCESS)
    {
      printf("Retrieve Fail\n");
      FAIL=1;
    }

    printf("\nAction: Cancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    printf("\nSuccess\n");
  }

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_10()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nTest Case 10: 500 Subscription to:600,700 Cancel this Subscription in a seperate thread\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  else
  {
    printf("\nAction: Retrieve in a seperate thread\n");
    /*Retrieve thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_retrieve_thread,
                       gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    sleep(2);
    fflush(stdout);
    printf("\nAction: Release the Retrieve Thread\n");
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nTASK is released\n");
    }

    printf("\nAction: Cancel subscription 500\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    printf("\nSuccess\n");
  }

  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

void tet_svc_subscr_ADEST_11()
{
  int FAIL=0;
  int id;
  MDS_SVC_ID svcids[]={600,700};

  if (prepare_ADEST_tc() == false)
    FAIL=1;

  printf("\nTest Case 11: Cross check whether i_rem_svc_pvt_ver returned in service event callback, matches the service private sub-part version of the remote service (subscribee)\n");
  if (mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,700) != NCSCC_RC_SUCCESS) 
  {
    printf("\nFail mds_service_uninstall\n");
    FAIL = 1;
  }
  printf("\nAction: Installing serive 700\n");
  if(mds_service_install(gl_tet_adest.mds_pwe1_hdl,700,2,
                         NCSMDS_SCOPE_INTRACHASSIS,true,false)!=NCSCC_RC_SUCCESS)
  {
    printf("Fail \n");
    FAIL=1;
  }
  printf("\nAction: Subscribing for the services\n");
  if( mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,500,
                            NCSMDS_SCOPE_INTRACHASSIS,2,
                            svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("Fail \n");
    FAIL=1;
  }
  sleep(1);
  printf("\nAction: Retreive the services\n");
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("Retrieve Fail\n");
    FAIL=1;
  }
     
  printf("\nAction: Verifying the svc pvt version for UP events\n");
  if( (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_UP))!=1 ||
      (tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,2,NCSMDS_UP)!=1))
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\nAction: Uninstalling 600 and 700\n");
  for(id=600;id<=700;id=id+100)
  {
    if(  mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,id)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
  }
  sleep(1);
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,500,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("Retrieve Fail\n");
    FAIL=1;
  }
  printf("\nAction: Verifying the svc pvt version for DOWN events\n");

  if(tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,600,1,NCSMDS_DOWN)!=1 ||
     tet_verify_version(gl_tet_adest.mds_pwe1_hdl,500,700,2,NCSMDS_DOWN)!=1)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("\nAction: Cancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,500,2,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  //clean up
  cleanup_ADEST_srv();

  test_validate(FAIL, 0);
}

uint32_t tet_initialise_setup(bool fail_no_active_sends)
{
  int i,FAIL=0;
  gl_vdest_indx=0;
  printf("/ntet_initialise_setup: Get an ADEST handle,Create PWE=2 on ADEST,Install EXTMIN and INTMIN svc\
 on ADEST,Install INTMIN,EXTMIN services on ADEST's PWE=2,\nCreate VDEST 100\
 and VDEST 200,Change the role of VDEST 200 to ACTIVE, \n\tInstall EXTMIN \
 service on VDEST 100,Install INTMIN, EXTMIN services on VDEST 200 \n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail adest_get_handle\n");
    FAIL=1; 
  }
  
  if(create_vdest(NCS_VDEST_TYPE_MxN,100)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  if(vdest_change_role(gl_tet_vdest[1].vdest,
                       V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  if(create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,2)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  for(i=NCSMDS_SVC_ID_INTERNAL_MIN;i<=NCSMDS_SVC_ID_EXTERNAL_MIN;
      i=i+(NCSMDS_SVC_ID_EXTERNAL_MIN-NCSMDS_SVC_ID_INTERNAL_MIN))
    if(mds_service_install(gl_tet_adest.mds_pwe1_hdl,i,3,NCSMDS_SCOPE_NONE,
                           true,fail_no_active_sends)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
  
  for(i=NCSMDS_SVC_ID_INTERNAL_MIN;i<=NCSMDS_SVC_ID_EXTERNAL_MIN;
      i=i+(NCSMDS_SVC_ID_EXTERNAL_MIN-NCSMDS_SVC_ID_INTERNAL_MIN))
    if(mds_service_install(gl_tet_adest.pwe[0].mds_pwe_hdl,i,3,NCSMDS_SCOPE_NONE,
                           true,fail_no_active_sends)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  
  if(mds_service_install(gl_tet_vdest[0].mds_pwe1_hdl,
                         NCSMDS_SVC_ID_EXTERNAL_MIN,2,NCSMDS_SCOPE_NONE,
                         true,fail_no_active_sends)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  if(mds_service_install(gl_tet_vdest[1].mds_pwe1_hdl,
                         NCSMDS_SVC_ID_INTERNAL_MIN,1,NCSMDS_SCOPE_NONE,
                         true,fail_no_active_sends)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  if(mds_service_install(gl_tet_vdest[1].mds_pwe1_hdl,
                         NCSMDS_SVC_ID_EXTERNAL_MIN,1,NCSMDS_SCOPE_NONE,
                         true,fail_no_active_sends)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  return FAIL;
}

uint32_t tet_cleanup_setup()
{
  int i,id,FAIL=0;
  printf("\tUninstalling the services on both VDESTs and ADEST\n");
  printf("\n UnInstalling the Services on both the VDESTs\n");
  for(i=gl_vdest_indx-1;i>=0;i--)
  {  
    for(id=gl_tet_vdest[i].svc_count-1;id>=0;id--)
    {
      if (mds_service_retrieve(gl_tet_vdest[i].mds_pwe1_hdl,
                               gl_tet_vdest[i].svc[id].svc_id,
                               SA_DISPATCH_ALL) 
	  !=NCSCC_RC_SUCCESS)
      {  
        printf("Fail mds_service_retrieve\n");  
        FAIL=1; 
      }

      if(mds_service_uninstall(gl_tet_vdest[i].mds_pwe1_hdl,
                               gl_tet_vdest[i].svc[id].svc_id)
         !=NCSCC_RC_SUCCESS)
      {  
        printf("Fail Vdest Svc Uninstall\n");  
        FAIL=1; 
      }  
    }
  }
  fflush(stdout);
  printf("\nDestroying the VDESTS\n"); 
  printf("\tDestroying both the VDESTs and PWE=2 on ADEST\n");
  for(i=gl_vdest_indx-1;i>=0;i--)
  {
    /*changing its role to Standby*/
    if(vdest_change_role(gl_tet_vdest[i].vdest,V_DEST_RL_STANDBY)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
    }
    /*destroy all VDESTs */
    if(destroy_vdest(gl_tet_vdest[i].vdest)!=NCSCC_RC_SUCCESS)
    {    
      printf("VDEST DESTROY Fail\n");
      FAIL=1; 
    }
  }
#if 1
  /*Uninstalling the 2000/INTMIN service on PWE1 ADEST*/
  for(i=NCSMDS_SVC_ID_EXTERNAL_MIN;i>=NCSMDS_SVC_ID_INTERNAL_MIN;i=i-NCSMDS_SVC_ID_INTERNAL_MIN)
  {
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,i,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("Adest Svc  Retrieve Fail\n");
      FAIL=1;
    }
    if(mds_service_uninstall(gl_tet_adest.mds_pwe1_hdl,i)!=
       NCSCC_RC_SUCCESS)
    {
      printf("Adest Svc  Uninstall Fail\n");
      FAIL=1;
    }
  }
  /*Uninstalling the 2000/INTMIN on PWE2 of ADEST*/
  printf("\n ADEST : PWE 2 : Uninstalling Services 2000/INTMIN\n"); 
  for(i=NCSMDS_SVC_ID_EXTERNAL_MIN;i>=NCSMDS_SVC_ID_INTERNAL_MIN;i=i-NCSMDS_SVC_ID_INTERNAL_MIN)
  {
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,i,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("Adest Svc PWE 2 Retrieve Fail\n");
      FAIL=1; 
    }  
    if(mds_service_uninstall(gl_tet_adest.pwe[0].mds_pwe_hdl,i)!=
       NCSCC_RC_SUCCESS)
    {    
      printf("Adest Svc PWE 2 Uninstall Fail\n");
      FAIL=1; 
    }
  }
#endif
  printf("\nADEST PWE2 Destroyed\n"); 
  if(destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl)!=NCSCC_RC_SUCCESS)
  {    
    printf("ADEST PWE2 Destroy Fail\n");
    FAIL=1; 
  }
  
  return FAIL;
}

void tet_just_send_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 1: Send Low, Medium, High and Very High Priority messages to Svc EXTMIN on Active Vdest in sequence\n");

    printf("\nAction: Subscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nAction: Retreive\n");
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nAction: Send LOW\n");
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else {
      printf("LOW Success\n");
    }

    printf("\nAction: Send MEDIUM\n");
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_MEDIUM,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    } else {
      printf("MEDIUM Success\n");
    }

    printf("\nAction: Send HIGH\n");
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_HIGH,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    } else {
      printf("HIGH Success\n");
    }

    printf("\nAction: Send VERY HIGH\n");
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_VERY_HIGH,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    } else {
      printf("VERY HIGH Success\n");
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }


  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 2: Not able to send a message to Svc EXTMIN with Invalid pwe handle\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send(gl_tet_adest.pwe[0].mds_pwe_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 3: Not able to send a message to Svc EXTMIN with NULL pwe handle\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,gl_tet_vdest[1].vdest,
                     MDS_SEND_PRIORITY_LOW,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 4: Not able to send a message to Svc EXTMIN with  Wrong DEST\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,0,
                     MDS_SEND_PRIORITY_LOW,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 5: Not able to send a message to service which is Not installed\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,1800,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_6()
{
  int FAIL=0;
  int id;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 6: Able to send a messages 1000 times  to Svc 2000 on Active Vdest\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    for(id=1;id<=1000;id++)
      if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                       mesg)!=NCSCC_RC_SUCCESS)
      {      
        printf("\nFail\n");
        FAIL=1;
        break; 
      }
    if(id==1001)
    {
      printf("\nALL THE MESSAGES GOT DELIVERED\n");
      sleep(1);
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  MDS_SVC_ID svc_ids[]={NCSMDS_SVC_ID_INTERNAL_MIN};
 gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 7: Send a message to unsubscribed Svc INTMIN on Active Vdest:Implicit/Explicit Combination\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_INTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_INTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\tCancelling the subscription svc_ids");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\tCancelling the subscription svcids");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("Fail");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_8()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 8: Not able to send a message to Svc EXTMIN with Improper Priority\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,5,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_9()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 9: Not able to send aNULL message to Svc EXTMIN on Active Vdest\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     NULL)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_10()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 10: Now send a message( > MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    memset(mesg->send_data,'S',2*MSG_SIZE+2 );
    mesg->send_len=2*MSG_SIZE+2;
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_11()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 11: Not able to Send a message with Invalid Send type\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.i_op=MDS_SEND;
    svc_to_mds_info.info.svc_send.i_msg=mesg;
    svc_to_mds_info.info.svc_send.i_to_svc=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_send.i_priority=MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_send.i_sendtype=12; /*wrong send type*/
    svc_to_mds_info.info.svc_send.info.snd.i_to_dest=
        gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_12()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 12: While Await Active timer is On: send a  message to Svc EXTMIN Vdest=200\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/ 
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_13()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("\nTest Case 13: Send a message to Svc EXTMIN on QUIESCED Vdest=200\n");
    /*--------------------------------------------------------------------*/
    printf("\nSubscribe\n");
   if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_HIGH,
                     mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_just_send_tp_14()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  gl_vdest_indx=0;
  
  char tmp[]=" Hi Receiver ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } 
  else
  {
    printf("Test Case 14: Copy Callback returning Failure: Send Fails\n");
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*Returning Failure at Copy CallBack*/
    gl_COPY_CB_FAIL=1;
    if(mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Resetting it*/
    gl_COPY_CB_FAIL=0;

    printf("\tCancelling the subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

bool setup_send_ack_tp()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*--------SEND ACKNOWLEDGE-------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  if (FAIL != 0) 
  {
    return false;
  }
  return true;
}

void tet_send_ack_tp_1()
{
  int FAIL=0;
  //  int id;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  //  MDS_SVC_ID svc_ids[]={NCSMDS_SVC_ID_INTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
  
    printf("\nTest Case 1: Now send LOW,MEDIUM,HIGH and VERY HIGH Priority messages with ACK to Svc on ACtive VDEST in sequence\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("LOW Success\n");

    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,
                        MDS_SEND_PRIORITY_MEDIUM,mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("MEDIUM Success\n");

    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_HIGH,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("HIGH Success\n");

    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,
                        MDS_SEND_PRIORITY_VERY_HIGH,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("VERY HIGH Success\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
  
    printf("\nTest Case 2: Not able to send a message with ACK to 1700 with Invalid pwe handle\n");
    if(mds_send_get_ack(gl_tet_adest.pwe[0].mds_pwe_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 3: Not able to send a message with ACK to 1700 with NULL pwe handle\n");
    if(mds_send_get_ack((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,
                        MDS_SEND_PRIORITY_LOW,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 4: Not able to send a message with ACK to 1700 with Wrong DEST\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,0,100,
                        MDS_SEND_PRIORITY_LOW,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 5: Not able to send a message with ACK to service which is Not installed\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,1800,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
 }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 6: Not able to send a message with ACK to OAC service on this DEST\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_7()
{
  int FAIL=0;
  int id;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 7: Able to send a message with ACK 1000 times to service 1700\n");
    for(id=1;id<=1000;id++)
      if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          gl_tet_vdest[1].vdest,100,
                          MDS_SEND_PRIORITY_LOW,mesg)!=NCSCC_RC_SUCCESS)
      {      printf("\nFail\n");FAIL=1;break;    }
    if(id==1001)
    {
      printf("\n\n\t ALL THE MESSAGES GOT DELIVERED\n\n");
      sleep(1);
      printf("\nSuccess\n");
    }

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_8()
{
  int FAIL=0;
  //  int id;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  MDS_SVC_ID svc_ids[]={NCSMDS_SVC_ID_INTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);

    printf("\nTest Case 8: Send a message with ACK to unsubscribed service 1600\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_INTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_INTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_9()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
    printf("\nTest Case 9: Not able to send_ack a message with Improper Priority to 1700\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,0,
                        mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_10()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
    printf("\nTest Case 10: Not able to send a NULL message with ACK to 1700\n");
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        NULL)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_11()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
    printf("Test Case 11: Now send a message( > MDS_DIRECT_BUF_MAXSIZE) to 1700\n");
    memset(mesg->send_data,'S',2*MSG_SIZE+2 );
    mesg->send_len=2*MSG_SIZE+2;
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_12()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
    printf("Test Case 12: While Await Active timer is On: send_ack a message to Svc EXTMIN Vdest=200\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,1000,
                        MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_send_ack_tp_13()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};

  if (setup_send_ack_tp() == true) 
  {
    char tmp[]=" Hi Receiver! Are you there? ";
    TET_MDS_MSG *mesg;
    mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
    memset(mesg, 0, sizeof(TET_MDS_MSG));
    memcpy(mesg->send_data,tmp,sizeof(tmp));
    mesg->send_len=sizeof(tmp);
    printf("Test Case 13: Send_ack a message to Svc EXTMIN on QUIESCED Vdest=200\n");
    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        NCSMDS_SVC_ID_EXTERNAL_MIN,
                        gl_tet_vdest[1].vdest,100,MDS_SEND_PRIORITY_LOW,
                        mesg)!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    free(mesg);
  }
  else
  {
    FAIL=1; 
  }
  /*------------------------------------------------------------------*/
  /*clean up*/
  printf("\nCancelling the subscription\n");
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {    
    printf("Cancelling the subscription Failed\n");
    FAIL=1; 
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_query_pwe_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest case 1: Get the details of all the Services on this core\n");
    if(mds_service_query_for_pwe(gl_tet_adest.mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN)
       !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN)
       !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(gl_tet_vdest[0].mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN)
       !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(gl_tet_vdest[1].mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN)
       !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(gl_tet_vdest[1].mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_INTERNAL_MIN)
       !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
      
    if (mds_service_redundant_subscribe(gl_tet_adest.mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,
                                    NCSMDS_SCOPE_NONE,1,
                                    svcids) !=NCSCC_RC_SUCCESS)
    { 
      printf("\nFail mds_service_redundant_subscribe\n");
      FAIL=1;
    }

    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) 
	!=NCSCC_RC_SUCCESS)
    { 
      printf("\nFail mds_service_retrieve\n");
      FAIL=1;
    }
      
    /*mds_query_vdest_for_anchor(gl_tet_adest.mds_pwe1_hdl,2000,100,
      2000,0);*/
      
    if (mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    svcids) != NCSCC_RC_SUCCESS)
    { 
      printf("\nmds_service_cancel_subscription\n");
      FAIL=1;
    }
      
    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }
 
  test_validate(FAIL, 0);
}

void tet_query_pwe_tp_2()
{
  int FAIL=0;
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 2: It shall not be able to query PWE with Invalid PWE Handle\n");
    if(mds_service_query_for_pwe((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(0,NCSMDS_SVC_ID_EXTERNAL_MIN)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_query_for_pwe(gl_tet_vdest[0].mds_vdest_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }
 
  test_validate(FAIL, 0);
}

void tet_query_pwe_tp_3()
{
  int FAIL=0;
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 3: ?????mds_query_vdest_for_anchor\n");
    if (mds_query_vdest_for_anchor(gl_tet_adest.mds_pwe1_hdl,
                                   NCSMDS_SVC_ID_EXTERNAL_MIN,100,
                                   NCSMDS_SVC_ID_EXTERNAL_MIN,0) == NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    FAIL=1;
  }
 
  test_validate(FAIL, 0);
}

void tet_adest_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  int FAIL=0;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
  
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(3)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                         NCSMDS_SVC_ID_EXTERNAL_MIN,
                         SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");
      FAIL=1;    
    }

    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_send_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,mesg)
         !=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
        FAIL=1;    
      }
      else
        printf("Response Success\n");
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_adest_rcvr_svc_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
  
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(0)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    sleep(1);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {     
      printf("Fail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_send_response(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           mesg)!=NCSCC_RC_SUCCESS)
      {     
        printf("Response Fail\n");
      }
      else
        printf("Response Success\n");
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }

  free(mesg);
}

void tet_vdest_rcvr_resp_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);    
  
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {     
      printf("Fail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_send_response(gl_tet_vdest[1].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           mesg)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
      }
      else
        printf("Response Success\n");

    }
  }

  free(mesg);
}

void tet_send_response_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 1: Svc INTMIN on PWE=2 of ADEST sends a LOW  Priority message to Svc NCSMDS_SVC_ID_EXTERNAL_MIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 2: Svc INTMIN on PWE=2 of ADEST sends a MEDIUM Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_MEDIUM,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}


void tet_send_response_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 3: Svc INTMIN on Active VEST=200 sends a HIGH Priority message to Svc EXTERNAL_MIN on ADEST and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_svc_thread,
                       gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_HIGH,
                             mesg)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[0].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 4: Svc EXTMIN of ADEST sends a VERYHIGH Priority message to Svc EXTMIN on Active Vdest=200 and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_resp_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_vdest[1].vdest,500,
                             MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nTASK is released\n");
    }
    fflush(stdout); 
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 5: While Await ActiveTimer ON:SvcEXTMIN of ADEST sends a message to Svc EXTMIN on Active Vdest=200 and Times out\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_resp_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_vdest[1].vdest,500,
                             MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    fflush(stdout); 
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 6: Svc EXTMIN of ADEST sends a VERYHIGH Priority message to Svc EXTMIN on QUIESCED Vdest=200 and Times out\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_resp_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);         
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_vdest[1].vdest,500,
                             MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    fflush(stdout);
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    printf("\nTest Case 7: Implicit Subscription:Svc INTMIN on PWE=2 of ADEST sends a LOW Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }
    fflush(stdout);
    /*Implicit explicit combination*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\nTASK is released\n");
    }
    fflush(stdout);

    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_8()
{
  int FAIL=0;
  //  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    printf("\nTest Case 8: Not able to send a message to a Service which doesn't exist\n");
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,3000,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,mesg)
       ==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_9()
{
  int FAIL=0;
  //MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    printf("\nTest Case 9: Not able to send_response a message to Svc 2000 with Improper Priority\n");
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,100,6,
                             mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 

    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_10()
{
  int FAIL=0;
  //MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    printf("Test Case 10 : Not able to send_response a NULL message to Svc EXTMIN on Active Vdest\n");
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,
                             NULL)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 

    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_tp_11()
{
  int FAIL=0;
  //MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {    
    printf("\nTest Case 11: Now send_response a message(> MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN on Active Vdest\n");
    memset(mesg->send_data,'S',2*MSG_SIZE+2 );
    mesg->send_len=2*MSG_SIZE+2;
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             gl_tet_adest.adest,500,
                             MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
    {
      printf("\tTASK is released\n");
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}

#if 0
TODO: Check this testcase, it was outcomment already in the "tet"-files
    void tet_send_response_tp_12()
{
  int FAIL=0;
  int id;
  //MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_adest_rcvr_thread();
  
  char tmp[]=" Hi Receiver! What is your Name? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {   
    printf("\nTest Case 12 : Able to send a messages 200 times to Svc  2000 on Active Vdest\n");
    for(id=1;id<=200;id++)
    {
      /*Receiver thread*/
      if(tet_create_task((NCS_OS_CB)tet_adest_rcvr_thread,
                         gl_tet_adest.svc[2].task.t_handle)
         ==NCSCC_RC_SUCCESS )
      {
        printf("\tTask has been Created\n");fflush(stdout);
      }
      /*Sender*/           
      if(mds_send_get_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               2000,gl_tet_adest.adest,500,
                               MDS_SEND_PRIORITY_LOW,
                               mesg)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
        FAIL=1;
      }
    }
    if(id==201)
    {
      printf("\n\n\t ALL THE MESSAGES GOT DELIVERED\n\n");
      sleep(1);
      printf("\nSuccess\n");
    }

    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nClenaup failed\n");
      FAIL=1;
    }
  }

  free(mesg);
  test_validate(FAIL, 0);
}
#endif

void tet_vdest_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Yes Sender! I am in. Message Delivered?";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);    

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
        printf("Fail mds_service_retrieve\n");
    }

    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_sendrsp_getack(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,0,
                            mesg)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
      }
      else
        printf("Response Ack Success\n");
      gl_rcvdmsginfo.rsp_reqd=0;
      /*if(mds_send_redrsp_getack(gl_tet_vdest[1].mds_pwe1_hdl,
        2000,300)!=NCSCC_RC_SUCCESS)
        {      printf("Response Fail\n");FAIL=1;    }
        else
        printf("Red Response Success\n");      */
    }
  }
  fflush(stdout);
  free(mesg);
}

void tet_Dadest_all_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,
			     SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
    }

    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      printf("i am here\n");
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      if(mds_direct_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_RSP,0)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
  free(mesg);
}

/*Adest sending direct response to vdest*/
void tet_Dadest_all_chgrole_rcvr_thread()
{
  MDS_SVC_ID svc_id;

  printf("\nInside CHG ROLE ADEST direct Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    sleep(2);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,
			     SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      if(mds_direct_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN, gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_RSP, 1500)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }

      sleep(10);
      if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }

      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
}

void tet_adest_all_chgrole_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside CHG ROLE ADEST Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    sleep(1);
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\tFail\n");
    }
    else
    {
      /*after that send a response to the sender, if it expects*/
      if(gl_rcvdmsginfo.rsp_reqd)
      {
        if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
        {
          printf("\nFail\n");
        }
        if(mds_send_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,mesg)!=NCSCC_RC_SUCCESS)
        {
          printf("\nFail\n");
        }

               
        sleep(10);
        if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
        {
          printf("\nFail\n");
        }

        gl_rcvdmsginfo.rsp_reqd=0;
      }
    }
  }
  fflush(stdout);
  free(mesg);
}
void tet_vdest_all_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside VDEST  Receiver Thread\n");fflush(stdout);
  sleep(10);
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
  }

  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    sleep(1);
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,
			     SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_send_response(gl_tet_vdest[1].mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,mesg)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
  free(mesg);
}
void tet_adest_all_rcvrack_thread()
{
  MDS_SVC_ID svc_id;
  uint32_t rs;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,
			     SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFailmds_service_retrieve \n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }

      rs=mds_sendrsp_getack(gl_tet_adest.mds_pwe1_hdl, NCSMDS_SVC_ID_EXTERNAL_MIN,
                            0,mesg);
               
      printf("\nResponse code is %d",rs);
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
  free(mesg);
}

void tet_adest_all_rcvrack_chgrole_thread()
{
  MDS_SVC_ID svc_id;
  uint32_t rs;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,
			     SA_DISPATCH_ALL) !=NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      if(tet_create_task((NCS_OS_CB)tet_adest_all_chgrole_rcvr_thread,
                         gl_tet_adest.svc[1].task.t_handle)
         == NCSCC_RC_SUCCESS )
      {
        printf("\tTask has been Created\n");fflush(stdout); 
      }
      rs=mds_sendrsp_getack(gl_tet_adest.mds_pwe1_hdl, NCSMDS_SVC_ID_EXTERNAL_MIN,
                            0,mesg);
      if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
         ==NCSCC_RC_SUCCESS)
        printf("\tTASK is released\n");
      fflush(stdout);

      printf("\nResponse code is %d",rs);
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
  free(mesg);
}

void tet_Dadest_all_rcvrack_chgrole_thread()
{
  MDS_SVC_ID svc_id;
  uint32_t rs=0;

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                         NCSMDS_SVC_ID_EXTERNAL_MIN,
                         SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      if(tet_create_task((NCS_OS_CB)tet_adest_all_chgrole_rcvr_thread,
                         gl_tet_adest.svc[1].task.t_handle)== NCSCC_RC_SUCCESS )
      {
        printf("\tTask has been Created\n");fflush(stdout);
      }
      if(mds_direct_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN, gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_SNDRACK, 0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
      }
      else
        printf("Response Success\n");
      if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)==NCSCC_RC_SUCCESS)
      {
        printf("\tTASK is released\n");
      }

      fflush(stdout);

      printf("\nResponse code is %d",rs);
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
}

void tet_change_role_thread()
{
  sleep(10);
  printf("\nInside Chnage role Thread\n");fflush(stdout);
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
  }

  fflush(stdout);
}

void tet_adest_all_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  char tmp[]=" Hi Sender! My Name is RECEIVER ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(1)) )
  {
    printf("\n\t Got the message: trying to retreive it\n");fflush(stdout);
    if (mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                         NCSMDS_SVC_ID_EXTERNAL_MIN,
                         SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      if(mds_send_response(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,mesg)!=NCSCC_RC_SUCCESS)
      {
        printf("\nFail\n");
      }
      gl_rcvdmsginfo.rsp_reqd=0;
    }
  }
  fflush(stdout);
  free(mesg);
}

void tet_send_all_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();
  void tet_adest_all_rcvr_thread();
  void tet_vdest_all_rcvr_thread();
  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
  
  /*--------------------------------------------------------------------*/

  printf("\nTest Case 1: Sender service installed with i_fail_no_active_sends = true and there is no-active instance of the receiver service\n");

  printf("\tSetting up the setup\n");
  if(tet_initialise_setup(true))
  {
    printf("\nSetup Initialisation has Failed \n");
      
    FAIL=1;
  }
  if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           NCSMDS_SCOPE_NONE,1,
                           svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  /*SND*/
  printf("\t Sending the message to no active instance\n");
  if( (mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=MDS_RC_MSG_NO_BUFFERING) )
  {
    printf("\nFail\n");
    FAIL=1;
  }
  else
    printf("\nSuccess\n");
  /*SNDACK*/

  printf("\nSendack to the no active instance\n");
  if((mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       gl_tet_vdest[1].vdest,
                       0,MDS_SEND_PRIORITY_LOW,
                       mesg)!=MDS_RC_MSG_NO_BUFFERING) )
  {
    printf("\nFail\n");
    FAIL=1;
  }
  else
    printf("\nSuccess\n");
  /*SNDRSP*/
  printf("\nSend response to the no active instance\n");
  if((mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_vdest[1].vdest,
                            0,MDS_SEND_PRIORITY_LOW,
                            mesg)!=MDS_RC_MSG_NO_BUFFERING) )
  {
    printf("\nFail\n");
    FAIL=1;
  }
  /*RSP*/
  printf("\nChange role to active\n"); 
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  if(tet_create_task((NCS_OS_CB)tet_adest_all_rcvr_thread,
                     gl_tet_adest.svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\nTask has been Created\n");fflush(stdout);
  }


  if((mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_adest.adest,
                            0,MDS_SEND_PRIORITY_LOW,
                            mesg)!= NCSCC_RC_SUCCESS) )
  {
    printf("\nFail\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n"); 

  /*Now Stop and Release the Receiver Thread*/
  if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");                
  fflush(stdout);                
  /*SNDRACK*/
#if 0
  printf("\tChange role to active\n"); 
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  if(tet_create_task((NCS_OS_CB)tet_adest_all_rcvrack_thread,
                     gl_tet_adest.svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\tTask has been Created\n");fflush(stdout);
  }

  printf("\tSending msg with rsp ack\n");
  if((mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_adest.adest,
                            0,MDS_SEND_PRIORITY_LOW,
                            mesg)!= NCSCC_RC_SUCCESS) )
  {
    printf("Fail from response\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n"); 

  /*Now Stop and Release the Receiver Thread*/
  if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");                
  fflush(stdout);                
#endif
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  fflush(stdout);

  /*clean up*/
  if(tet_cleanup_setup())
  {
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_all_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();
  void tet_adest_all_rcvr_thread();
  void tet_vdest_all_rcvr_thread();
  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
  
  /*--------------------------------------------------------------------*/

  printf("\nTest Case 2: Sender service installed with i_fail_no_active_sends = false and there is no-active instance of the receiver service\n");

  if(tet_initialise_setup(false))
  {
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                           NCSMDS_SVC_ID_EXTERNAL_MIN,
                           NCSMDS_SCOPE_NONE,1,
                           svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

 
  if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  printf("\n Sending the message to no active instance\n");
  if( (mds_just_send(gl_tet_adest.mds_pwe1_hdl,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     NCSMDS_SVC_ID_EXTERNAL_MIN,
                     gl_tet_vdest[1].vdest,MDS_SEND_PRIORITY_LOW,
                     mesg)!=NCSCC_RC_SUCCESS) && gl_rcvdmsginfo.msg_fmt_ver != 3 )
  {
    printf("\nFail\n");
    FAIL=1;
  }
  sleep(10);
  printf("\nChange role to active\n");
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail mds_service_retrieve\n");
    FAIL=1;
  }
  /*SNDACK*/
  printf("\nChange role to standby\n");
  if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n");
  /*Create thread to change role*/

  if(tet_create_task((NCS_OS_CB)tet_change_role_thread,
                     gl_tet_adest.svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\nTask has been Created\n");fflush(stdout);
  }

  printf("\n Sendack to the no active instance\n");
  if((mds_send_get_ack(gl_tet_adest.mds_pwe1_hdl,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       NCSMDS_SVC_ID_EXTERNAL_MIN,
                       gl_tet_vdest[1].vdest,
                       1500,MDS_SEND_PRIORITY_LOW,
                       mesg)!=NCSCC_RC_SUCCESS) && (gl_rcvdmsginfo.msg_fmt_ver != 3))
  {
    printf("\nFail\n");
    FAIL=1;

  }

  else
    printf("\nSuccess\n");

  if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");
  fflush(stdout);
  /*SNDRSP*/

  printf("\nChange role to standby\n");
  if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                          NCSMDS_SVC_ID_EXTERNAL_MIN,
                          SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n");
  if(tet_create_task((NCS_OS_CB)tet_vdest_all_rcvr_thread,
                     gl_tet_vdest[1].svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\nTask has been Created\n");fflush(stdout);
  }
  printf("\n Sendrsp to the no active instance\n");
  if((mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_vdest[1].vdest,
                            1500,MDS_SEND_PRIORITY_LOW,
                            mesg)!= NCSCC_RC_SUCCESS)&& (gl_rcvdmsginfo.msg_fmt_ver != 3) )
  {
    printf("msg fmt ver is %d",gl_rcvdmsginfo.msg_fmt_ver);
    printf("Fail frm rsp\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n");
  if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");
  /*RSP*/
  if(tet_create_task((NCS_OS_CB)tet_adest_all_chgrole_rcvr_thread,
                     gl_tet_adest.svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\nTask has been Created\n");fflush(stdout);
  }

  printf("\n rsp to the no active instance\n");
  if((mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_adest.adest,
                            2500,MDS_SEND_PRIORITY_LOW,
                            mesg)!= NCSCC_RC_SUCCESS )&&gl_rcvdmsginfo.msg_fmt_ver != 3) 
  {
    printf("\nFail\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n"); 

  /*Now Stop and Release the Receiver Thread*/
  if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");                
  fflush(stdout);                
#if 0
  /*SNDRACK*/

  if(tet_create_task((NCS_OS_CB)tet_adest_all_rcvrack_chgrole_thread,
                     gl_tet_adest.svc[1].task.t_handle)
     == NCSCC_RC_SUCCESS )
  {
    printf("\nTask has been Created\n");fflush(stdout);
  }
  printf("\n Send ack being sent\n");

  if((mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            gl_tet_adest.adest,
                            1500,MDS_SEND_PRIORITY_LOW,
                            mesg)!= NCSCC_RC_SUCCESS) )
  {
    printf("\nFail\n");
    FAIL=1;
  }

  else
    printf("\nSuccess\n"); 

  /*Now Stop and Release the Receiver Thread*/
  if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
     ==NCSCC_RC_SUCCESS)
    printf("\nTASK is released\n");                
  fflush(stdout);                

#endif
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  fflush(stdout);

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("Setup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 1: Svc EXTMIN on ADEST sends a LOW Priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {      printf("Send get Response Fail\n");FAIL=1;    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");                
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 2: Svc EXTMIN on ADEST sends a MEDIUM Priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_MEDIUM,
                             mesg)!=NCSCC_RC_SUCCESS)
    {      printf("Send get Response Fail\n");FAIL=1;    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");                
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 3: Svc EXTMIN on ADEST sends a HIGH Priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_HIGH,
                             mesg)!=NCSCC_RC_SUCCESS)
    {      printf("Send get Response Fail\n");FAIL=1;    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS) {
      printf("\tTASK is released\n");
    }
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 4: Svc EXTMIN on ADEST sends a VERYHIGH Priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_SUCCESS)
    {      printf("Send get Response Fail\n");FAIL=1;    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");                
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*--------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 5: While Await Active Timer ON:SvcEXTMIN of ADEST sends a message to Svc EXTMIN on Active Vdest=200 and Times out\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);         
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             300,MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_REQ_TIMOUT)
    { 
      printf("Send get Response Fail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS) {
      printf("\tTASK is released\n");
    }

    fflush(stdout);                
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  } 
  else 
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 6: SvcEXTMIN of ADEST sends message to SvcEXTMIN on QUIESCED Vdest=200 and Times out\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);         
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             300,MDS_SEND_PRIORITY_VERY_HIGH,
                             mesg)!=NCSCC_RC_REQ_TIMOUT)
    {
      /*printf("Send get Response Fail\n");*/
      printf("Send get Response is not Timing OUT: Why\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");                
    fflush(stdout);                
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if (tet_cleanup_setup()) {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  } else {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 7: Implicit Subscription: Svc EXTL_MIN on ADEST sends a LOWPriority message to Svc EXTMIN on VDEST=200 and expects Response\n");
    /*Receiver thread*/
    if (tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                        gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS ) {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if (mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                              NCSMDS_SVC_ID_EXTERNAL_MIN,
                              NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                              900,MDS_SEND_PRIORITY_LOW,
                              mesg) != NCSCC_RC_SUCCESS) {      
      printf("Send get Response Fail\n");FAIL=1;    
    } else {
      printf("\nSuccess\n");
    }

    /*Now Stop and Release the Receiver Thread*/
    if (m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS) {
      printf("\tTASK is released\n");
    }          
  
    fflush(stdout);                
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS)
    {      printf("Send get Response Fail\n");FAIL=1;    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");                
    fflush(stdout);                
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*--------------------------------------------------------------------*/
  /*clean up*/
  if (tet_cleanup_setup()) {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_send_response_ack_tp_8()
{
  int FAIL=0;
  void tet_vdest_rcvr_thread();

  char tmp[]=" Hi Receiver! Are you there? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  } else {
    /*--------------------------------------------------------------------*/
    printf("\nTest Case 8: Now send_response a message( > MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN on Active Vdest\n");
    memset(mesg->send_data,'S',2*MSG_SIZE+2 );
    mesg->send_len=2*MSG_SIZE+2;
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_vdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS ) {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_send_get_response(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,200,
                             900,MDS_SEND_PRIORITY_LOW,
                             mesg)!=NCSCC_RC_SUCCESS) {      
      printf("Send get Response Fail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS) {
      printf("\nTASK is released\n");
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup()) {
    printf("\nCleanup has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_vdest_Srcvr_thread()
{
  MDS_SVC_ID svc_id;
  
  char tmp[]=" Yes Sender! I am STANDBY? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));  
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
  
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      gl_rcvdmsginfo.rsp_reqd=0;
    }
    /*      if(mds_send_redrsp_getack(gl_tet_vdest[0].mds_pwe1_hdl,2000,
            300)!=NCSCC_RC_SUCCESS)
            {      printf("Response Ack Fail\n");FAIL=1;    }
            else
            printf("Red Response Ack Success\n");      */
  }
  fflush(stdout);
  free(mesg);
}

void tet_vdest_Arcvr_thread()
{
  MDS_SVC_ID svc_id;
    
  char tmp[]=" Yes Man! I am in: Message Delivered? ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));  
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);
        
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("\nFail mds_service_retrieve\n");
    }
#if 0 /*AVM RE-CHECk*/
    /*after that send a response to the sender, if it expects*/ 
    if(gl_rcvdmsginfo.rsp_reqd)
    {
      if(mds_send_redrsp_getack(gl_tet_vdest[0].mds_pwe1_hdl,
                                NCSMDS_SVC_ID_EXTERNAL_MIN,0,
                                mesg)!=NCSCC_RC_SUCCESS)
      {      printf("Response Ack Fail\n");FAIL=1;    }
      else
        printf("Red Response Ack Success\n");      
      gl_rcvdmsginfo.rsp_reqd=0;
    }          
#endif
  }
  fflush(stdout);
  free(mesg);
}

void tet_broadcast_to_svc_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*--------------------------------------------------------------------*/
    printf("\nCase 1: Svc INTMIN on VDEST=200 Broadcasting a LOW Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_broadcast_to_svc(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SCOPE_NONE,MDS_SEND_PRIORITY_LOW,
                            mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_broadcast_to_svc_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 2: Svc INTMIN on ADEST Broadcasting a MEDIUM Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_broadcast_to_svc(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SCOPE_NONE,MDS_SEND_PRIORITY_MEDIUM,
                            mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");     
    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}



void tet_broadcast_to_svc_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 3: Svc INTMIN on VDEST=200 Redundant Broadcasting a HIGH priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n\t Setup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_broadcast_to_svc_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 4: Svc INTMIN on VDEST=200 Not able to Broadcast a message with Invalid Priority\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_broadcast_to_svc(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SCOPE_NONE,5,mesg)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_broadcast_to_svc_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 5: Svc INTMIN on VDEST=200 Not able to Broadcast a NULL message\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_broadcast_to_svc(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SCOPE_NONE,MDS_SEND_PRIORITY_LOW,
                            NULL)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n\t Setup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

void tet_broadcast_to_svc_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
    
  char tmp[]=" Hi All ";
  TET_MDS_MSG *mesg;
  mesg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
  memset(mesg, 0, sizeof(TET_MDS_MSG));   
  memcpy(mesg->send_data,tmp,sizeof(tmp));
  mesg->send_len=sizeof(tmp);

  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    printf("\nTest Case 6: Svc INTMIN on VDEST=200 Broadcasting a VERY HIGH Priority message (>MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN\n");
    memset(mesg->send_data,'S',MSG_SIZE+512 );
    mesg->send_len=MSG_SIZE+512;
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_broadcast_to_svc(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            NCSMDS_SCOPE_NONE,
                            MDS_SEND_PRIORITY_VERY_HIGH,
                            mesg)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*---------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n\t Setup Clean Up has Failed \n");
    FAIL=1;
  }

  free(mesg);
  test_validate(FAIL, 0);
}

/*---------------- DIRECT SEND TEST CASES--------------------------------*/

void tet_direct_just_send_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 1: Now Direct send Low,Medium,High and Very High Priority messages to Svc EXTMIN on Active Vdest=200\n");
    /*Low*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Medium*/

    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_MEDIUM,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*High*/

    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_HIGH,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Very High*/

    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_VERY_HIGH,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nTest Case 2: Not able to send a message to 2000 with Invalid pwe handle\n");
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 3: Not able to send a message to 2000 with NULL pwe handle\n");
    if(mds_direct_send_message((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,
                               gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*--------------------------------------------------------------------*/
    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 4: Not able to send a message to Svc EXTMIN on Vdest=200 with Wrong DEST\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,0,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }
  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 5: Not able to send a message to service which is Not installed\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,3000,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_6()
{
  int FAIL=0;
  int id;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 6 : Able to send a message 1000 times  to Svc EXTMIN on Active Vdest=200\n");
    for(id=1;id<=1000;id++)
    {
      if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                 MDS_SENDTYPE_SND,
                                 gl_tet_vdest[1].vdest,
                                 0,MDS_SEND_PRIORITY_LOW,
                                 message)!=NCSCC_RC_SUCCESS)
      {    
        printf("\nFail\n");
        FAIL=1; 
      }
    }
    if(id==1001)
    {
      printf("\nALL THE MESSAGES GOT DELIVERED\n\n");
      sleep(1);
      printf("\nSuccess\n");
    }

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  MDS_SVC_ID svc_ids[]={NCSMDS_SVC_ID_INTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 7: Implicit Subscription: Direct send a message to unsubscribed Svc INTMIN\n");
    /*Implicit Subscription*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_INTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Explicit subscription*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svc_ids) != NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_INTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }

  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_8()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 8: Not able to send a message to Svc EXTMIN with Improper Priority\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               5,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_9()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 9: Not able to send a message to Svc EXTMIN with Direct Buff as NULL\n");
    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=NULL;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len = 
        strlen(message)+1;
    svc_to_mds_info.info.svc_direct_send.i_to_svc=
        NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=1;
    svc_to_mds_info.info.svc_direct_send.i_priority=
        MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=MDS_SENDTYPE_SND;
    svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=
        gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    { printf("\nFail\n");FAIL=1;    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_10()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 10: Not able to send a message with Invalid Sendtype(<0 or >11)\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               -1,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_11()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 11: Not able to send a message with Invalid Message length\n");
    direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(message)+1);
    memset(direct_buff, 0, strlen(message)+1);
    if(direct_buff==NULL)
      printf("Direct Buffer not allocated properly\n");
    memcpy(direct_buff,(uint8_t *)message,strlen(message)+1);
    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_EXTERNAL_MIN;
          
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len = -1;
    /*wrong length*/
    svc_to_mds_info.info.svc_direct_send.i_to_svc=
        NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=1;
    svc_to_mds_info.info.svc_direct_send.i_priority=
        MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=MDS_SENDTYPE_SND;
    svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=
        gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    { printf("\nFail\n");FAIL=1;    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }
  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_12()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 12: While Await Active Timer ON: Direct send a Low Priority message to Svc EXTMIN on Vdest=200\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_13()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 13: Direct send a Medium Priority message to Svc EXTMIN on QUIESCED Vdest=200\n");
    printf("\ntet_direct_just_send_tp_13\n");
    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    int rc;
    if((rc=mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_MEDIUM,
                                   message))!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail rc=%d\n", rc);
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_14()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char big_message[8000];

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 14: Not able to send a message of size >(MDS_DIRECT_BUF_MAXSIZE) to 2000\n");

    memset(big_message, 's', 8000);
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_adest.adest,0,
                               MDS_SEND_PRIORITY_LOW,
                               big_message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_just_send_tp_15()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char big_message[8000];

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 15: Able to send a message of size =(MDS_DIRECT_BUF_MAXSIZE) to 2000\n");
          
    memset(big_message, '\0', 8000);
    memset(big_message, 's', 7999);
          
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SND,gl_tet_adest.adest,0,
                               MDS_SEND_PRIORITY_LOW,
                               big_message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nCancel subscription\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_all_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------DIRECT SEND --------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=0;
    printf("\nTest Case 1: Direct send a message with i_msg_fmt_ver = 0 for all send types\n");
    /*SND*/
    printf("\nDirect sending the message\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*SNDACK*/
    printf("\nDirect send with ack\n"); 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*SNDRSP*/
    printf("\nDirect send with rsp\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");

    fflush(stdout);
    /*SNDRACK*/
    printf("\n Direct send with response ack\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_rack_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle) == NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
 
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_all_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=3;
    printf("\nTest Case 2: Direct send a message with i_msg_fmt_ver = i_rem_svc_pvt_ver for all send types\n");
    /*SND*/
    printf("\nDirect sending the message\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*SNDACK*/
    printf("\nDirect send with ack\n"); 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*SNDRSP*/
    printf("\nDirect send with rsp\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);
    /*SNDRACK*/
    printf("\n Direct send with response ack\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_rack_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);         
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_all_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {       
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=4;
    printf("\nTest Case 3: Direct send a message with i_msg_fmt_ver > i_rem_svc_pvt_ver for all send types\n");
    /*SND*/
    printf("\nDirect sending the message\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*SNDACK*/
    printf("\nDirect send with ack\n"); 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*SNDRSP*/
    printf("\nDirect send with rsp\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);
    /*SNDRACK*/
    printf("\n Direct send with response ack\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_rack_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);         
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_all_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=2;
    printf("\nTest Case 4: Direct send a message with i_msg_fmt_ver < i_rem_svc_pvt_ver for all send types\n");
    /*SND*/
    printf("\nDirect sending the message\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*SNDACK*/
    printf("\nDirect send with ack\n"); 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*SNDRSP*/
    printf("\nDirect send with rsp\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);
    /*SNDRACK*/
    printf("\n Direct send with response ack\n");
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_rack_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);         
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_all_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=1;
    printf("\nTest Case 5 : Direct send when Sender service installed with i_fail_no_active_sends = false and there is no-active instance of the receiver service\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    printf("\n Sending the message to no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,
                               gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    sleep(10);
    printf("\nChange role to active\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    /*SNDACK*/
    printf("\nChange role to standby\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n");
    /*Create thread to change role*/

    if(tet_create_task((NCS_OS_CB)tet_change_role_thread,
                       gl_tet_adest.svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }

    printf("\n Sendack to the no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)

    {
      printf("\nFail\n");
      FAIL=1;

    }

    else
      printf("\nSuccess\n");

    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);
    /*SNDRSP*/
    printf("\nChange role to standby\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n");

    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_all_chg_role_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }

    printf("\nSendrsp to the no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)


    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n");
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");


    /*RSP*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_all_chgrole_rcvr_thread,
                       gl_tet_adest.svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }


    printf("\n Sendrsp to the no active instance\n");
    if(mds_direct_send_message(gl_tet_vdest[1].mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,1500,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)


    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");
    fflush(stdout);

#if 0
    printf("\nChange role to standby\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n");

    /*SNDRACK*/

    if(tet_create_task((NCS_OS_CB)tet_Dadest_all_rcvrack_chgrole_thread,
                       gl_tet_adest.svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    printf("\nSend rsp ack being sent\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,3000,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)


    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n"); 

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");                
    fflush(stdout);                

#endif
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}


void tet_direct_send_all_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_all_thread();
  void tet_Dvdest_rcvr_all_rack_thread();
  void tet_Dvdest_rcvr_all_chg_role_thread();
  void tet_Dadest_all_rcvrack_chgrole_thread();
  void tet_Dadest_all_chgrole_rcvr_thread();
  void tet_Dadest_all_rcvr_thread();

  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail");
      FAIL=1; 
    }

    gl_set_msg_fmt_ver=1;
    printf("\nTest Case 6: Direct send when Sender service installed with i_fail_no_active_sends = true and there is no-active instance of the receiver service\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    /*Unistalling setup to install again with req params*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
    if(tet_initialise_setup(true))
    { 
      printf("\nSetup Initialisation has Failed \n");
      FAIL=1;
    }

    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
                            
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*SND*/
    printf("\nDirect Sending the message to no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SND,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=MDS_RC_MSG_NO_BUFFERING)

    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

    /*SNDACK*/
    printf("\nDirect  Sendack to the no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=MDS_RC_MSG_NO_BUFFERING)

    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

    /*SNDRSP*/
    printf("\n Send response to the no active instance\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_vdest[1].vdest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=MDS_RC_MSG_NO_BUFFERING)

    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");


    /*RSP*/
    printf("\nChange role to active\n"); 
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    if(tet_create_task((NCS_OS_CB)tet_Dadest_all_rcvr_thread,
                       gl_tet_adest.svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }


    if(mds_direct_send_message(gl_tet_vdest[1].mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)

    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n"); 

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");                
    fflush(stdout);                
    /*SNDRACK*/
#if 0
    printf("\nChange role to active\n"); 
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }

    if(tet_create_task((NCS_OS_CB)tet_adest_all_rcvrack_thread,
                       gl_tet_adest.svc[1].task.t_handle)
       == NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }


    if((mds_send_get_response(gl_tet_vdest[1].mds_pwe1_hdl,
                              NCSMDS_SVC_ID_EXTERNAL_MIN,
                              NCSMDS_SVC_ID_EXTERNAL_MIN,
                              gl_tet_adest.adest,
                              3000,MDS_SEND_PRIORITY_LOW,
                              mesg)!= NCSCC_RC_SUCCESS) )
    {
      printf("\nFail\n");
      FAIL=1;
    }

    else
      printf("\nSuccess\n"); 

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");                
    fflush(stdout);                
#endif
  }
  gl_set_msg_fmt_ver=0;
  if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                     NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                     svcids)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
  }
  fflush(stdout);
  /*--------------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\nSetup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
 
    printf("\nTest Case 1: Now Direct send_ack Low,Medium,High and Very High Priority message to Svc EXTMIN on Active Vdest=200 in sequence\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*Medium*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_MEDIUM,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*High*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_HIGH,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    /*Very High*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_VERY_HIGH,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 2: Not able to send_ack a message to 2000 with Invalid pwe handle\n");
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,
                               gl_tet_vdest[1].vdest,20,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nTest Case 3: Not able to send_ack a message to EXTMIN with NULL pwe handle\n");
    if(mds_direct_send_message((MDS_HDL)(long)NULL,NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,
                               gl_tet_vdest[1].vdest,20,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 4: Not able to send_ack a message to Svc EXTMIN on Vdest=200 with Wrong DEST\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,0,20,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 5: Not able to send_ack a message to service which is Not installed\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,3000,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_6()
{
  int FAIL=0;
  int id=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 6: Able to send_ack a message 1000 times  to Svc EXTMIN on Active Vdest=200\n");
    for(id=1;id<=1000;id++)
    {
      if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN,
                                 NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                 MDS_SENDTYPE_SNDACK,
                                 gl_tet_vdest[1].vdest,20,
                                 MDS_SEND_PRIORITY_LOW,
                                 message)!=NCSCC_RC_SUCCESS)
      {    
        printf("\nFail\n");
        FAIL=1; 
      }
    }
    if(id==1001)
    {
      printf("\n\n\t ALL THE MESSAGES GOT DELIVERED\n\n");
      sleep(1);
      printf("\nSuccess\n");
    }

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  MDS_SVC_ID svc_ids[]={NCSMDS_SVC_ID_INTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 7: Implicit Subscription: Direct send_ack a message to unsubscribed Svc INTMIN\n");
    /*Implicit Subscription*/
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_INTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    printf("\nNow doing Explicit Subscription\n");
    /*Explicit Subscription*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svc_ids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_INTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svc_ids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_8()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 8: Not able to send_ack a message to Svc EXTMIN with Improper Priority\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               0,5,message)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids )!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_9()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 9: Not able to send_ack a message with Invalid Sendtype(<0 or >11)\n");
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               12,gl_tet_vdest[1].vdest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_SUCCESS)
    {
      printf("\nFail\n");
      FAIL=1;
    }
    else
      printf("\nSuccess\n");

#if 0
    /*Allocating memory for the direct buffer*/
    direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(message)+1);
    memset(direct_buff, 0, sizeof(direct_buff));
    if(direct_buff==NULL)
      printf("Direct Buffer not allocated properly\n");
    memcpy(direct_buff,(uint8_t *)message,strlen(message)+1);
    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=2000;
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=
        strlen(message)+1;
    svc_to_mds_info.info.svc_direct_send.i_to_svc=2000;
    svc_to_mds_info.info.svc_direct_send.i_priority=
        MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=12;/*wrong sendtype*/
    svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=
        gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    { printf("\nFail\n");FAIL=1;    }
    else
    {
      printf("\nSuccess\n");
      /*Freeing Direct Buffer*/
      m_MDS_FREE_DIRECT_BUFF(direct_buff);
    }
#endif

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_10()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 10: Not able to send_ack a message to Svc EXTMIN with Direct Buff as NULL\n");
    /*Allocating memory for the direct buffer*/
    direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(message)+1);
    memset(direct_buff, 0, strlen(message)+1);
    if(direct_buff==NULL)
      printf("Direct Buffer not allocated properly\n");
    memcpy(direct_buff,(uint8_t *)message,strlen(message)+1);
    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=NULL;/*wrong*/
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=
        strlen(message)+1;
    svc_to_mds_info.info.svc_direct_send.i_to_svc=
        NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=1;
    svc_to_mds_info.info.svc_direct_send.i_priority=
        MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=MDS_SENDTYPE_SNDACK;
    svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=
        gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS) { 
      printf("\nFail\n");
      FAIL=1;    
    } else {
      printf("\nSuccess\n");
      /*Freeing Direct Buffer*/
      m_MDS_FREE_DIRECT_BUFF(direct_buff);
    }

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_11()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 11 : Not able to send_ack a message with Invalid Message length\n");
    /*Allocating memory for the direct buffer*/
    direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(message)+1);
    memset(direct_buff, 0, strlen(message)+1);
    if(direct_buff==NULL)
      printf("Direct Buffer not allocated properly\n");
    memcpy(direct_buff,(uint8_t *)message,strlen(message)+1);
    svc_to_mds_info.i_mds_hdl=gl_tet_adest.mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len= -1;/*wrong length*/
    svc_to_mds_info.info.svc_direct_send.i_to_svc=NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=1;
    svc_to_mds_info.info.svc_direct_send.i_priority=MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=MDS_SENDTYPE_SNDACK;
    svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=gl_tet_vdest[1].vdest;
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    { printf("\nFail\n");FAIL=1;    }
    else
      printf("\nSuccess\n");

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_12()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 12: While Await Active Timer ON: Direct send_ack a message to Svc EXTMIN on Vdest=200\n");
    if(vdest_change_role(200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,
                               MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_ack_tp_13()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  /*start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }
  else
  {
    /*----------DIRECT SEND ----------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 13: Direct send a Medium Priority message to Svc EXTMIN on QUIESCED Vdest=200\n");
    printf("\ntet_direct_send_ack_tp_13\n");
    if(vdest_change_role(200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    sleep(2);
    int rc;
    if((rc=mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDACK,gl_tet_vdest[1].vdest,
                               20,
                               MDS_SEND_PRIORITY_MEDIUM,
                                   message))!=NCSCC_RC_REQ_TIMOUT)
    {    
      printf("\nFail rc=%d\n", rc);
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");
    if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    fflush(stdout);
    /*--------------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_Dadest_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_adest_sel_obj_found(3)) )
  {
    /*Will this if leads to any problems*/
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)==NCSCC_RC_SUCCESS)
    {
      /*after that send a response to the sender, if it expects*/ 
      if(gl_direct_rcvmsginfo.rsp_reqd)
      {
        if(mds_direct_response(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_RSP,0)==NCSCC_RC_FAILURE)
        {      
          printf("Direct Response Fail\n");
        }
        else
          printf("\nDirect Response Success/TimeOUT\n");
      }
    }
  }
  fflush(stdout);  
}

void tet_Dvdest_rcvr_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
     if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			      NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) 
	 != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail mds_service_retrieve\n");
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                             MDS_SENDTYPE_SNDRACK,0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");   
      }
      else
        printf("Response Ack Success\n");
    }
  }
  fflush(stdout);  
}
void tet_Dvdest_rcvr_all_rack_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_SNDRACK,0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");    
      }
      else
        printf("Response Ack Success\n");
    }
  }
  fflush(stdout);
}

void tet_Dvdest_rcvr_all_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN, gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_RSP, 0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
      }
      else
        printf("Response Success\n");
    }
  }
  fflush(stdout);
}
void tet_Dvdest_rcvr_all_chg_role_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  sleep(10);
  if(vdest_change_role(200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
  }
  if( (svc_id=is_vdest_sel_obj_found(1,1)) )
  {
    sleep(2);
    if (mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN, gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_RSP, 0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Response Fail\n");
      }
      else
        printf("Response Success\n");
    }
  }
  fflush(stdout);
}

void tet_Dvdest_Srcvr_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                             MDS_SENDTYPE_RRSP,200)!=NCSCC_RC_SUCCESS)
      {      
        printf("Red Response Fail\n");
      }
      else
        printf("Red Response Success\n");
    }
  }
  fflush(stdout);  
}
void tet_Dvdest_Srcvr_all_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
     if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			      NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_RRSP,200)!=NCSCC_RC_SUCCESS)
      {      
        printf("Red Response Fail\n");
      }
      else
        printf("Red Response Success\n");
    }

  }
  fflush(stdout);
}

void tet_Dvdest_Arcvr_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/ 
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                             MDS_SENDTYPE_REDRACK,0)!=NCSCC_RC_SUCCESS)
      {     
        printf("Red Response Ack Fail\n");
      }
      else
        printf("Red Response Ack Success\n");      
    }          
  }
  fflush(stdout);  
}

void tet_Dvdest_Arcvr_all_thread()
{
  MDS_SVC_ID svc_id;
  printf("\nInside Receiver Thread\n");fflush(stdout);
  if( (svc_id=is_vdest_sel_obj_found(0,0)) )
  {
    if (mds_service_retrieve(gl_tet_vdest[0].mds_pwe1_hdl,
			     NCSMDS_SVC_ID_EXTERNAL_MIN,SA_DISPATCH_ALL) != NCSCC_RC_SUCCESS)
    {      
      printf("Fail mds_service_retrieve\n");    
    }
    /*after that send a response to the sender, if it expects*/
    if(gl_direct_rcvmsginfo.rsp_reqd)
    {
      if(mds_direct_response(gl_tet_vdest[0].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,gl_set_msg_fmt_ver,
                             MDS_SENDTYPE_REDRACK,0)!=NCSCC_RC_SUCCESS)
      {      
        printf("Red Response Ack Fail\n");
      }
      else
        printf("Red Response Ack Success\n");
    }
  }
  fflush(stdout);
}

void tet_direct_send_response_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dadest_rcvr_thread();
  void tet_Dvdest_rcvr_thread();
  void tet_Dvdest_Srcvr_thread();
  void tet_Dvdest_Arcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
 
    printf("\nTest Case 1: Svc INTMIN on PWE=2 of ADEST sends a LOW Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_LOW,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");               
    fflush(stdout); 

    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nSubscription Cancellation Fail\n");
      FAIL=1; 
    }
  }

  fflush(stdout);    
  /*-------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dadest_rcvr_thread();
  void tet_Dvdest_rcvr_thread();
  void tet_Dvdest_Srcvr_thread();
  void tet_Dvdest_Arcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
 
    printf("\nTest Case 2: Svc INTMIN on PWE=2 of ADEST sends a MEDIUM Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_MEDIUM,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
  }

  /*-------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dadest_rcvr_thread();
  void tet_Dvdest_rcvr_thread();
  void tet_Dvdest_Srcvr_thread();
  void tet_Dvdest_Arcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 3: Svc INTMIN on PWE=2 of ADEST sends a HIGH Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_HIGH,
                               message)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
  }

  /*-------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }
  test_validate(FAIL, 0);
}

void tet_direct_send_response_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dadest_rcvr_thread();
  void tet_Dvdest_rcvr_thread();
  void tet_Dvdest_Srcvr_thread();
  void tet_Dvdest_Arcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nTest Case 4: Svc INTMIN on PWE=2 of ADEST sends a VERYHIGH Priority message to Svc EXTMIN and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_VERY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("High Priority Direct Sent Fail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
  }

  /*-------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dadest_rcvr_thread();
  void tet_Dvdest_rcvr_thread();
  void tet_Dvdest_Srcvr_thread();
  void tet_Dvdest_Arcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\n\t Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-------------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nTest Case 5: Implicit/Explicit Svc INTMIN on PWE=2 of ADEST sends a message to Svc EXTMIN and expects a Response\n");
    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nSubscription Cancellation Fail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_VERY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nHigh Priority Direct Sent Fail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
    sleep(3);
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dadest_rcvr_thread,
                       gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender*/  
    if(mds_direct_send_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                               NCSMDS_SVC_ID_INTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,gl_tet_adest.adest,
                               0,MDS_SEND_PRIORITY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("High Priority Direct Sent Fail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");

    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_adest.svc[2].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 

    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("Subscription Cancellation Fail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }
  /*-------------------------------------------------------------*/
  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_ack_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\n Setup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-----------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
         

    printf("\nTest Case 1: Svc EXTMIN on ADEST sends a LOW priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_LOW,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 

    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout);

    /*-----------------------------------------------------------*/    
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_ack_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-----------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
         
    printf("\nTest Case 2: Svc EXTMIN on ADEST sends a MEDIUM priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");
      fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_MEDIUM,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");            
    fflush(stdout); 

    /*-----------------------------------------------------------*/    
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_ack_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_thread();
  printf("\nCase 3: Svc EXTMIN on ADEST sends a HIGH priority message to Svc EXTMIN on VDEST=200 and expects a Response\n");

 /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-----------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

   
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 

    /*-----------------------------------------------------------*/    
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_ack_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-----------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nTest Case 4: Svc EXTMIN on ADEST sends a VERYHIGH priority  message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_VERY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    sleep(2); /*in 16.0.2*/
    /*Now Stop and Release the Receiver Thread*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
    /*-----------------------------------------------------------*/    
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_send_response_ack_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  char message[]="Direct Message";
  void tet_Dvdest_rcvr_thread();
  /*Start up*/
  if(tet_initialise_setup(false))
  {
    printf("\nSetup Initialisation has Failed \n");
    FAIL=1;
  }

  else
  {
    /*-----------------------------------------------------------------*/
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }

    printf("\nTest Case 5: Implicit/Explicit Svc EXTMIN on ADEST sends a message to Svc EXTMIN on VDEST=200 and expects a Response\n");
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\tTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_VERY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    sleep(2); /*in 16.0.2*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\tTASK is released\n");               
    fflush(stdout); 
    if(mds_service_subscribe(gl_tet_adest.mds_pwe1_hdl,
                             NCSMDS_SVC_ID_EXTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.mds_pwe1_hdl,
                            NCSMDS_SVC_ID_EXTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    /*Receiver thread*/
    if(tet_create_task((NCS_OS_CB)tet_Dvdest_rcvr_thread,
                       gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS )
    {
      printf("\nTask has been Created\n");fflush(stdout);
    }
    /*Sender */ 
    if(mds_direct_send_message(gl_tet_adest.mds_pwe1_hdl,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,
                               NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                               MDS_SENDTYPE_SNDRSP,200,0,
                               MDS_SEND_PRIORITY_HIGH,
                               message)==NCSCC_RC_FAILURE)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n"); 
    /*Now Stop and Release the Receiver Thread*/
    sleep(2); /*in 16.0.2*/
    if(m_NCS_TASK_RELEASE(gl_tet_vdest[1].svc[1].task.t_handle)
       ==NCSCC_RC_SUCCESS)
      printf("\nTASK is released\n");               
    fflush(stdout); 
 
    /*-----------------------------------------------------------*/    
    if(mds_service_cancel_subscription(gl_tet_adest.mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                       svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
  }

  /*clean up*/
  if(tet_cleanup_setup())
  {
    printf("\n Setup Clean Up has Failed \n");
    FAIL=1;
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_1()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
 
    printf("\nTest Case 1 : Svc INTMIN on VDEST=200 Broadcasting a Low Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_LOW)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);

    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n\nt Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_2()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 2 :Svc INTMIN on VDEST=200 Broadcasting a Medium Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_MEDIUM)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_3()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n\t Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 3: Svc INTMIN on VDEST=200 Broadcasting a High Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,
                             svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_HIGH)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_4()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n\t Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 4: Svc INTMIN on VDEST=200 Broadcasting a Very High Priority message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_VERY_HIGH)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_5()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\nSetup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 5: Svc INTMIN on ADEST Broadcasting a message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_adest.pwe[0].mds_pwe_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_adest.pwe[0].mds_pwe_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_LOW)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");     
    if(mds_service_cancel_subscription(gl_tet_adest.pwe[0].mds_pwe_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_6()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 6: Svc INTMIN on VDEST=200 Redundant Broadcasting  a message to Svc EXTMIN\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_RBCAST,
                                    NCSMDS_SCOPE_NONE,
                                    MDS_SEND_PRIORITY_LOW)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");     
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\nSetup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_7()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { 
    printf("\nSetup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 7: Svc INTMIN on VDEST=200 Not able to Broadcast a message with Invalid Priority\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_direct_broadcast_message(gl_tet_vdest[1].mds_pwe1_hdl,
                                    NCSMDS_SVC_ID_INTERNAL_MIN,
                                    NCSMDS_SVC_ID_EXTERNAL_MIN,1,
                                    MDS_SENDTYPE_BCAST,
                                    NCSMDS_SCOPE_NONE,0)
       ==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   
    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n\nt Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

void tet_direct_broadcast_to_svc_tp_8()
{
  int FAIL=0;
  MDS_SVC_ID svcids[]={NCSMDS_SVC_ID_EXTERNAL_MIN};
  /*Start up*/
  if(tet_initialise_setup(false))
  { printf("\n Setup Initialisation has Failed \n");}
  else
  {
    /*-------------------------------------------------------------*/
    printf("\nTest Case 8: Svc INTMIN on VDEST=200 Not able to Broadcast a message with NULL Direct Buff\n");
    if(mds_service_subscribe(gl_tet_vdest[1].mds_pwe1_hdl,
                             NCSMDS_SVC_ID_INTERNAL_MIN,
                             NCSMDS_SCOPE_NONE,1,svcids)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    if(mds_service_retrieve(gl_tet_vdest[1].mds_pwe1_hdl,
                            NCSMDS_SVC_ID_INTERNAL_MIN,
                            SA_DISPATCH_ALL)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    svc_to_mds_info.i_mds_hdl=gl_tet_vdest[1].mds_pwe1_hdl;
    svc_to_mds_info.i_svc_id=NCSMDS_SVC_ID_INTERNAL_MIN;
    svc_to_mds_info.i_op=MDS_DIRECT_SEND;    
  
    svc_to_mds_info.info.svc_direct_send.i_direct_buff=NULL;
    svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=20;
    svc_to_mds_info.info.svc_direct_send.i_to_svc=
        NCSMDS_SVC_ID_EXTERNAL_MIN;
    svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=1;
    svc_to_mds_info.info.svc_direct_send.i_priority
        = MDS_SEND_PRIORITY_LOW;
    svc_to_mds_info.info.svc_direct_send.i_sendtype=MDS_SENDTYPE_BCAST;
    svc_to_mds_info.info.svc_direct_send.info.bcast.i_bcast_scope
        = NCSMDS_SCOPE_NONE;
  
    if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    else
      printf("\nSuccess\n");   

    if(mds_service_cancel_subscription(gl_tet_vdest[1].mds_pwe1_hdl,
                                       NCSMDS_SVC_ID_INTERNAL_MIN,
                                       1,svcids)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    fflush(stdout);
    /*----------------------------------------------------------------*/
    /*clean up*/
    if(tet_cleanup_setup())
    {
      printf("\n\nt Setup Clean Up has Failed \n");
      FAIL=1;
    }
  }

  test_validate(FAIL, 0);
}

/****************************************************************/
/**************             ADEST TEST PUPOSES      *************/
/****************************************************************/
void tet_destroy_PWE_ADEST_twice_tp()
{
  int FAIL=0;
  printf("\nTest Case 1: Destroy PWE ADEST twice\n");
  /*start up*/
  memset(&gl_tet_adest,'\0', sizeof(gl_tet_adest)); /*zeroizing*/
    
    
  printf("Getting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Creating a PWE=2 on this ADEST\n");
  if(create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,2)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Destroying the PWE=2 on this ADEST\n");
  if(destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Not able to Destroy ALREADY destroyed PWE=2 on this ADEST\n");
  if(destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_PWE_ADEST_twice_tp()
{
  int FAIL=0;
  printf("\nTest case 2: Create PWE ADEST twice\n");
  /*start up*/
  memset(&gl_tet_adest,'\0', sizeof(gl_tet_adest)); /*zeroizing*/
    
    
  printf("Getting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Creating a PWE=2 on this ADEST\n");
  if(create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,2)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Able Create ALREADY created PWE=2 on this ADEST\n");
  if(create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,2)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
  {
    if (destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail destroy_pwe_on_adest\n");
      FAIL=1; 
    }
  }
    
  /*clean up*/
  printf("Destroying the PWE=2 on this ADEST\n");
  if(destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_default_PWE_ADEST_tp()
{
  int FAIL=0;
  printf("\nTest case 3: Create default PWE ADEST\n");
  /*start up*/
  memset(&gl_tet_adest,'\0', sizeof(gl_tet_adest)); /*zeroizing*/
    
  printf("Getting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("Able to Create a DEFUALT PWE=1 on this ADEST\n");
  if(create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,1)!=NCSCC_RC_SUCCESS)
  {      printf("\nFail\n");
    FAIL=1;
  }
  else
  {
    if(destroy_pwe_on_adest(gl_tet_adest.pwe[0].mds_pwe_hdl)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
    printf("\nSuccess\n");       
  }

  test_validate(FAIL, 0);
}

void tet_create_PWE_upto_MAX_tp()
{
  int id;
  int FAIL=0;
  printf("\nTest Case 4: Create PWE upto MAX\n");
  printf("\n**********tet_create_PWE_upto_MAX_tp****************\n");     
  /*start up*/
  memset(&gl_tet_adest,'\0', sizeof(gl_tet_adest)); /*zeroizing*/
    
  printf("Getting an ADEST handle\n");
  if(adest_get_handle()!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("\n No. of PWEs = %d \t",gl_tet_adest.pwe_count);
  fflush(stdout);sleep(1);
  printf("Creating PWEs with pwe_id from 2 to 1999\n");
  for(id=2;id<=NCSMDS_MAX_PWES;id++)
  {
    if (create_pwe_on_adest(gl_tet_adest.mds_adest_hdl,id) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail create_pwe_on_adest\n");
      FAIL=1; 
    }
  }
  printf("\n No. of PWEs = %d \t",gl_tet_adest.pwe_count);
  fflush(stdout);sleep(1);
  if(gl_tet_adest.pwe_count==NCSMDS_MAX_PWES-1)
    printf("Success: %d PWEs got created",gl_tet_adest.pwe_count);
  else
  {
    printf("Failed to Create the PWE=2000\n");
    FAIL=1; 
  }
  printf("Destroying all the PWEs from 2 to 1999\n");
  for(id=gl_tet_adest.pwe_count-1;id>=0;id--)
  {
    if (destroy_pwe_on_adest(gl_tet_adest.pwe[id].mds_pwe_hdl) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail destroy_pwe_on_adest\n");
      FAIL=1; 
    }
  }
  printf("\n No. of PWEs = %d \t",gl_tet_adest.pwe_count);

  if(gl_tet_adest.pwe_count==0)   
  {
    printf("Success: All the PWEs got Destroyed\n");
  }
  else
  {
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

/********************************************************************/
/**************       VDEST TEST PUPOSES                  ***********/
/********************************************************************/
void tet_create_MxN_VDEST_1()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 1: Creating a VDEST in MXN model with MIN vdest_id\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,
                  NCSVDA_EXTERNAL_UNNAMED_MIN)==NCSCC_RC_SUCCESS)
  {
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }  
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_MxN_VDEST_2()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 2: Not able to create a VDEST with vdest_id above the MAX RANGE\n");
  //NCSVDA_UNNAMED_MAX = NCSMDS_MAX_VDEST = 32767
  if(create_vdest(NCS_VDEST_TYPE_MxN,
                  NCSVDA_UNNAMED_MAX+1)==NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_MxN_VDEST_3()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 3: Not able to create a VDEST with vdest_id = 0\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,0)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_MxN_VDEST_4()
{
  int FAIL=0;
  printf("          tet_create_MxN_VDEST\n");
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 4: Create a VDEST with vdest_id below the MIN RANGE\n");   
  if(create_vdest(NCS_VDEST_TYPE_MxN,
                  NCSVDA_EXTERNAL_UNNAMED_MIN-1)==NCSCC_RC_SUCCESS)
  {
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN-1)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }    
 
  test_validate(FAIL, 0);
}

void tet_create_Nway_VDEST_1()
{
  int FAIL=0;
  printf("          tet_create_Nway_VDEST\n");
  gl_vdest_indx=0;    
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 1: Creating a VDEST in N-way model with MAX vdest_id\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  NCSVDA_EXTERNAL_UNNAMED_MAX)==NCSCC_RC_SUCCESS)
  {
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MAX)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }  
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }  

  test_validate(FAIL, 0);
}

void tet_create_Nway_VDEST_2()
{
  int FAIL=0;
  printf("          tet_create_Nway_VDEST\n");
  gl_vdest_indx=0;    
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 2: Not able to create a VDEST with vdest_id above the MAX RANGE\n");
  //NCSVDA_UNNAMED_MAX = NCSMDS_MAX_VDEST = 32767
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  NCSVDA_UNNAMED_MAX+1)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }  

  test_validate(FAIL, 0);
}

void tet_create_Nway_VDEST_3()
{
  int FAIL=0;
  printf("          tet_create_Nway_VDEST\n");
  gl_vdest_indx=0;    
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 3: Not able to create a VDEST with vdest_id = 0\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  0)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_Nway_VDEST_4()
{
  int FAIL=0;
  printf("          tet_create_Nway_VDEST\n");
  gl_vdest_indx=0;    
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 4: Create a VDEST with vdest_id below the MIN RANGE\n");   
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  NCSVDA_EXTERNAL_UNNAMED_MIN-1)==NCSCC_RC_SUCCESS)
  {
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN-1)!=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  }
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }    

  test_validate(FAIL, 0);
}

void tet_create_OAC_VDEST_tp_1()
{
  int FAIL=0;
  printf("          tet_create_OAC_VDEST_tp\n");
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("Test Case 1: Creating a OAC service on VDEST in MXN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,
                  NCSVDA_EXTERNAL_UNNAMED_MIN)==NCSCC_RC_SUCCESS)
  {
    sleep(2);/*Introduced in Build 12*/
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN)!=NCSCC_RC_SUCCESS)
    {    
      printf("\n  VDEST_DESTROY is FAILED\n");
      fflush(stdout);
      FAIL=1; 
    }
  }  
  else
    FAIL=1;

  test_validate(FAIL, 0);
}

void tet_create_OAC_VDEST_tp_2()
{
  int FAIL=0;
  printf("          tet_create_OAC_VDEST_tp\n");
  gl_vdest_indx=0;
  /*Start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 2: Creating a a OAC service on VDEST in N-way model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  NCSVDA_EXTERNAL_UNNAMED_MAX)==NCSCC_RC_SUCCESS)
  {
    sleep(2);/*Introduced in Build 12*/
    if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MAX)!=NCSCC_RC_SUCCESS)
    {    
      printf("\n  VDEST_DESTROY is FAILED\n");
      fflush(stdout);
      FAIL=1; 
    }
  }  
  else
    FAIL=1;    

  test_validate(FAIL, 0);
}

void tet_destroy_VDEST_twice_tp_1()
{
  int FAIL=0;
  printf("                tet_destroy_VDEST_twice_tp\n");
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 1: Creating a VDEST in N-WAY model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  NCSVDA_EXTERNAL_UNNAMED_MIN)!=NCSCC_RC_SUCCESS)
  {      printf("\nFail\n");FAIL=1;    }
  
  /*clean up*/   
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN)!=NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");FAIL=1;    
  }
 
  test_validate(FAIL, 0);
}

void tet_destroy_VDEST_twice_tp_2()
{
  int FAIL=0;
  printf("                tet_destroy_VDEST_twice_tp\n");
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 2: Not able to Destroy ALREADY destroyed VDEST\n");
  if(destroy_vdest(NCSVDA_EXTERNAL_UNNAMED_MIN)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
 
  test_validate(FAIL, 0);
}

void tet_destroy_ACTIVE_MxN_VDEST_tp_1()
{
  int FAIL=0;
    
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 1: Destroy ACTIVE MxN VDEST\n");
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1400)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }   
  printf("Changing VDEST role to ACTIVE\n");
  if(vdest_change_role(1400,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1400)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_destroy_ACTIVE_MxN_VDEST_tp_2()
{
  int FAIL=0;
    
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

 
  printf("\nTest Case 2: Destroy QUIESCED MxN VDEST\n");
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1400)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }   
  if(vdest_change_role(1400,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Changing VDEST role to QUIESCED\n");    
  if(vdest_change_role(1400,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }   
  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1400)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_chg_standby_to_queisced_tp_1()
{
  int FAIL=0;
  printf("\nTest Case 1: Change_standby to queisced (ACTIVE->ACTIVE)\n");
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
  printf("Creating a VDEST in N-way model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  printf("\nTest Case 1: Changing VDEST role to ACTIVE twice\n");
  printf("Changing VDEST role to ACTIVE\n");
  if(vdest_change_role(1200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
        
  printf("Again Changing VDEST role to ACTIVE\n");
  if(vdest_change_role(1200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_chg_standby_to_queisced_tp_2()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
  printf("Creating a VDEST in N-way model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  printf("\nTest Case 2: Changing VDEST role from ACTIVE to QUIESCED\n");
  printf("Changing VDEST role to ACTIVE\n");
  if(vdest_change_role(1200,V_DEST_RL_ACTIVE)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
        
  printf("Changing VDEST role from ACTIVE to QUIESCED\n");
  if(vdest_change_role(1200,V_DEST_RL_QUIESCED)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_chg_standby_to_queisced_tp_3()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
  printf("Creating a VDEST in N-way model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
 
  printf("\nTest Case 3: Changing VDEST role from QUIESCED to STANDBY\n");
  if(vdest_change_role(1200,V_DEST_RL_STANDBY)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_chg_standby_to_queisced_tp_4()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
  printf("Creating a VDEST in N-way model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
 
  printf("\nTest Case 4: Not able to Change VDEST role from STANDBY to QUIESCED\n");
  if(vdest_change_role(1200,V_DEST_RL_QUIESCED)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  
  /*clean up*/ 
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

#if 0
***********************************************
The "named" test cases are not woking since the NCSVDA_VDEST_CREATE_NAMED is not supported
***********************************************
void tet_create_named_VDEST_1()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nTest Case 1: Not able to Create a Named VDEST in MxN model with Persistence Flag= true\n");
  if(create_named_vdest(true,NCS_VDEST_TYPE_MxN,
                        vname)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_2()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nTest Case 2: Not able to Create a Named VDEST in N-Way model with Persistence Flag= true\n");
  if(create_named_vdest(true,NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                        vname)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_3()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  MDS_DEST vdest_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nTest Case 3: Able to Create a Named VDEST in MxN model with Persistence Flag=false\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_MxN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");

  printf("Looking for VDEST id associated with this Named VDEST\n");
  vdest_id=vdest_lookup(vname);
  if(vdest_id)
    printf("Named VDEST= %s : %d Vdest id",vname,(int)vdest_id);    
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Destroying the above Named VDEST in MxN model\n");
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_4()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  MDS_DEST vdest_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nTest Case 4: Able to Create a Named VDEST in MxN model with OAC Flag= true\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_MxN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");
  printf("Looking for VDEST id associated with this Named VDEST\n");
  vdest_id=vdest_lookup(vname);
  if(vdest_id)
    printf("Success: Named VDEST= %s : %d Vdest id",vname,(int)vdest_id);   
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Destroying the above Named VDEST in MxN model\n");
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_5()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  MDS_DEST vdest_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nTest Case 5: Able to Create a Named VDEST in N-Way model with Persistence Flag=false\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");
  printf("Looking for VDEST id associated with this Named VDEST\n");
  vdest_id=vdest_lookup(vname);
  if(vdest_id)
    printf("Success: Named VDEST= %s : %d Vdest id",vname,(int)vdest_id);   
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Destroying the above Named VDEST in MxN model\n");
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_6()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  MDS_DEST vdest_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 6: Able to Create a Named VDEST in N-Way model with OAC Flag= true\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");
  printf("Looking for VDEST id associated with this Named VDEST\n");
  vdest_id=vdest_lookup(vname);
  if(vdest_id)
    printf("Success: Named VDEST= %s : %d Vdest id",vname,(int)vdest_id);   
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Destroying the above Named VDEST in MxN model\n");
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_7()
{
  int FAIL=0;
  char vname[10]="MY_VDEST";
  MDS_DEST vdest_id,V_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 7: Check the vdest_id returned: if we Create a Named VDEST in N-Way model for the second time\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  vdest_id=vdest_lookup(vname);
  printf("First time : Named VDEST= %s : %d Vdest id",vname,(int)vdest_id); 
  if(create_named_vdest(false,NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                        vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  V_id=vdest_lookup(vname);
  printf("Second time: Named VDEST= %s : %d Vdest id",vname,(int)V_id);     
  if(vdest_id==V_id)
    printf("\nSuccess\n");
  else
  {
    printf("Not The Same Vdest_id returned\n");  
    FAIL=1;
  }  
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  if(destroy_named_vdest(false,vdest_id,vname)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }   
 
  test_validate(FAIL, 0);
}
      
void tet_create_named_VDEST_8()
{
  int FAIL=0;
  char null[1]="\0";
  MDS_DEST V_id;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
 
  printf("\nTest Case 8: Looking for VDEST id associated with a NULL name\n");
  V_id=vdest_lookup(NULL);
  if(V_id==NCSCC_RC_FAILURE)
    printf("\nSuccess\n");
  else
  {
    printf("\nFail\n");
    FAIL=1;
  }

  printf("Named VDEST= %s : %d Vdest id",null,(int)V_id);
 
  test_validate(FAIL, 0);
}

void tet_create_named_VDEST_9()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*start up*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/

  printf("\nTest Case 9: Not able to Create a Named VDEST in MxN model with NULL name\n");
  if(create_named_vdest(false,NCS_VDEST_TYPE_MxN,
                        NULL)==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  else
    printf("\nSuccess\n");
 
  test_validate(FAIL, 0);
}
#endif

void tet_test_PWE_VDEST_tp_1()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  printf("\nTest Case 1: Creating a PWE with MAX PWE_id= 2000 on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,NCSMDS_MAX_PWES)==NCSCC_RC_SUCCESS)
  {
    destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl);
  } 
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  } 

  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_test_PWE_VDEST_tp_2()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  printf("\nTest Case 2: Not able to Create a PWE with PWE_id > MAX i.e > 2000 on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,NCSMDS_MAX_PWES+1)==NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");FAIL=1;
    destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl);
  } 

  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_test_PWE_VDEST_tp_3()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("\nTest Case 3: Not able to Create a PWE with Invalid PWE_id = 0 on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,0)==NCSCC_RC_SUCCESS)
  {
    printf("\nFail\n");
    FAIL=1;
    if(destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl)
       !=NCSCC_RC_SUCCESS)
    {    
      printf("\nFail\n");
      FAIL=1; 
    }
  } 

  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_test_PWE_VDEST_tp_4()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("\nTest Case 4: Creating a PWE with PWE_id= 10 on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,10)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
        
  printf("Destroying the PWE=10 on this VDEST\n");    
  if(destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl)
     !=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_test_PWE_VDEST_tp_5()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("\nTest Case 5: Not able to Destroy an Already Destroyed PWE on this VDEST\n");
  if(destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl)
     ==NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  } 

  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_test_PWE_VDEST_tp_6()
{
  int FAIL=0;
  gl_vdest_indx=0;
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("\nTest Case 6: Creating a PWE with PWE_id= 20 and OAC service on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,20)==NCSCC_RC_SUCCESS)
  {
    sleep(2);
    destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl);
  }    
  else
  {    
    printf("\nFail\n");
    FAIL=1; 
  }   
 
  /*clean up*/
  printf("\nDestroying the above VDEST\n");
  if(destroy_vdest(1250)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  test_validate(FAIL, 0);
}

void tet_create_PWE_upto_MAX_VDEST()
{
  int FAIL=0;
  int id=0;
  gl_vdest_indx=0;
  printf("\nTest Case 7: tet_create_PWE_upto_MAX_VDEST\n");
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("\nCreating a VDEST in MxN model\n");
  if(create_vdest(NCS_VDEST_TYPE_MxN,1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
    
  printf("\nCreating PWEs from 2 to 2000\n");
  gl_tet_vdest[0].pwe_count=0;/*FIX THIS*/
  printf("\npwe_count = %d",gl_tet_vdest[0].pwe_count);
  fflush(stdout);
  sleep(1);
  for(id=2;id<=NCSMDS_MAX_PWES;id++)
  {
    if (create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,id) != NCSCC_RC_SUCCESS)
    {    
      printf("\nFail create_pwe_on_vdest\n");
      FAIL=1; 
    }
    printf("\tpwe_count = %d\t",gl_tet_vdest[0].pwe_count);
  }   
  fflush(stdout);sleep(1);
  if(gl_tet_vdest[0].pwe_count==NCSMDS_MAX_PWES-1)
    printf("Success: %d",gl_tet_vdest[0].pwe_count);
  else
    printf("\nFail\n");
  printf("Destroying PWEs from NCSMDS_MAX_PWES to 2\n");
  for(id=gl_tet_vdest[0].pwe_count-1;id>=0;id--)
  {
    destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[id].mds_pwe_hdl);
  }
  if(gl_tet_vdest[0].pwe_count!=0)
  {      printf("\nFail\n");FAIL=1;  }    
  /*clean up*/
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(1200)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void tet_create_default_PWE_VDEST_tp()
{
  int FAIL=0;
  gl_vdest_indx=0;
  printf("\nTest Case 8: tet_create_default_PWE_VDEST_tp\n");
  /*Start UP*/
  memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
    
  printf("Creating a VDEST in N-WAY model\n");
  if(create_vdest(NCS_VDEST_TYPE_N_WAY_ROUND_ROBIN,
                  2000)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  printf("Creating DEFAULT  PWE=1 on this VDEST\n");
  if(create_pwe_on_vdest(gl_tet_vdest[0].mds_vdest_hdl,1)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }
  destroy_pwe_on_vdest(gl_tet_vdest[0].pwe[0].mds_pwe_hdl);

  /*clean up*/
  printf("Destroying the above VDEST\n");
  if(destroy_vdest(2000)!=NCSCC_RC_SUCCESS)
  {    
    printf("\nFail\n");
    FAIL=1; 
  }

  test_validate(FAIL, 0);
}

void Print_return_status(uint32_t rs)
{
  switch(rs)
  {
    case SA_AIS_OK:
      printf("API is Successfull\n");break;
    case SA_AIS_ERR_LIBRARY:
      printf("Problem with the Library\n");break;
    case SA_AIS_ERR_TIMEOUT:
      printf("Time OUT\n");break;
    case SA_AIS_ERR_TRY_AGAIN:
      printf("The service can't be provided\n");break;
    case SA_AIS_ERR_INVALID_PARAM:
      printf("A parameter is not set properly\n");break;
    case SA_AIS_ERR_NO_MEMORY:
      printf("Out of Memory\n");break;
    case SA_AIS_ERR_NO_RESOURCES:
      printf("The system out of resources\n");break;
    case SA_AIS_ERR_VERSION:
      printf("Version parameter not compatible\n");break;
  }
}

#if 0
TODO: What to do with this
                     void tet_VDS(int choice)
    {
      int FAIL=0,i=0,j;
      char active_ip[15];
      char snpset[100];
      char *vname[]={"MY_VDEST1","MY_VDEST2","MY_VDEST3","MY_VDEST4","MY_VDEST5"};
      MDS_DEST vdest_id[5],v_id;
      gl_vdest_indx=0;
      char *tmpprt=NULL;
      tmpprt= (char *) getenv("ACTIVE_CB_IP\n");
      strcpy(active_ip,tmpprt);
      printf("\n*************************tet_VDS**************************\n");
      /*start up*/
      memset(&gl_tet_vdest,'\0', sizeof(gl_tet_vdest)); /*zeroizing*/
  
      printf("Creating a Named VDEST in MxN model\n");
      for(j=0;j<5;j++)
      {
        if(create_named_vdest(false,NCS_VDEST_TYPE_MxN,
                              vname[j])!=NCSCC_RC_SUCCESS)
        {    
          printf("\nFail\n");
          FAIL=1; 
        }
      }
      printf("\n vdest count = %d \n",gl_vdest_indx);
  
      printf("Looking for VDEST id associated with this Named VDEST\n");
      for(j=0,i=0;j<5;j++)
      {
        vdest_id[i]=vdest_lookup(vname[j]);
        printf("Named VDEST= %s : %d Vdest id",vname[j],(int)vdest_id[i++]);
      }
      /*-----------------------------------------------------------------------*/
      switch(choice)
      {
        case 1:  
          printf("pkill vds.out\n");
          system("pkill vds\n");
          break;
        case 2:  
          printf("snmpswithover\n");
          printf("\nActive SCXB IP = %s\n",active_ip);
          sprintf(snpset,"snmpset -v2c -m /usr/share/snmp/mibs/NCS-AVM-MIB -c public %s ncsAvmAdmSwitch.0 i 1",active_ip);
          printf("\n %s\n",snpset);
          system(snpset);
          break;
        case 3: printf("error report\n");
          SaAmfHandleT amfHandle;
          SaVersionT   version={'B',1,0};
          SaNameT      compName;
          char name[]="safComp=CompT_VDS,safSu=SuT_NCS_SCXB,safNode=Node1_SCXB";
          uint32_t returnvalue;

          returnvalue=saAmfInitialize(&amfHandle,NULL,&version);
          if(SA_AIS_OK==returnvalue)
          {
            printf("saAmfInitialize Successful\n");
            /*error report*/
            compName.length=(uint16_t)strlen(name);
            memcpy(compName.value,name,(uint16_t)strlen(name));
            if(SA_AIS_OK==saAmfComponentErrorReport(amfHandle,&compName,0,
                                                    SA_AMF_COMPONENT_FAILOVER,0))
              printf("saAmfComponentErrorReport Successful\n");
            else
              printf("saAmfComponentErrorReport FAILED\n");
            
            /*finalize*/
            if(SA_AIS_OK==saAmfFinalize(amfHandle))
              printf("saAmfFinalize Successful\n");
          }
          else
            Print_return_status(returnvalue);
          break;
        default: printf(" 1: Pkill VDS\n 2:Snmp switchover \n 3: Error Report \n");
          break;  
      }
      /*------------------------------------------------------------------------*/
      printf("\nPlease Enter any Key\n");
      sleep(5);
      fflush(stdout);
      getchar();
      /*checking*/
      for(j=0;j<5;j++)
      {
        v_id=vdest_lookup(vname[j]);
        if(vdest_id[j]!=v_id)
        {
          printf("\n Not Matched %d",j);
          FAIL=1;
          break;
        }
      }
      if(!FAIL)
        printf("\nSuccess\n");
    
      printf("\nDestroying the above Named VDEST in MxN model\n");
      for(j=4;j>=0;j--)
      {
        if(destroy_named_vdest(false,vdest_id[gl_vdest_indx-1],
                               vname[j])!=NCSCC_RC_SUCCESS)
        {    
          printf("\nFail\n");
          FAIL=1; 
        }        
      }

      test_validate(FAIL, 0);
    }
#endif

__attribute__ ((constructor)) static void mdsTipcAPI_constructor(void) {
  test_suite_add(1, "MDS TIPC API install");
  test_case_add(1, tet_svc_install_tp_01, "Installing service 500 twice");
  test_case_add(1, tet_svc_install_tp_02, "Installing the Internal MIN service INTMIN with NODE scope without MDS Queue ownership and failing to Retrieve");
  test_case_add(1, tet_svc_install_tp_03, "Installing the External MIN service EXTMIN with CHASSIS scope");
  test_case_add(1, tet_svc_install_tp_05, "Installing the service with MAX id:32767 with CHASSIS scope");
  test_case_add(1, tet_svc_install_tp_06, "Not able to Install the service with id > MAX:32767 with CHASSIS scope");
  test_case_add(1, tet_svc_install_tp_07, "Not able to Install the service with id = 0");
  test_case_add(1, tet_svc_install_tp_08, "Not able to Install the service with Invalid PWE Handle");
  test_case_add(1, tet_svc_install_tp_09, "Not able to Install a Service with Svc id > External MIN 2000 which doesn't chose MDS Q Ownership");
  test_case_add(1, tet_svc_install_tp_10, "Installing the External MIN service EXTMIN in a seperate thread and Uninstalling it here");
  test_case_add(1, tet_svc_install_tp_11, "Installation of the same Service id with different sub-part versions on the same pwe handle, must fail");
  test_case_add(1, tet_svc_install_tp_12, "Installation of the same Service id with same sub-part versions on the same pwe handle, must fail");
  test_case_add(1, tet_svc_install_tp_13, "Install a service with _mds_svc_pvt_ver = 0 i_mds_svc_pvt_ver = 255 and i_mds_svc_pvt_ver = A random value, which is >0 and <255");
  test_case_add(1, tet_svc_install_tp_14, "Install a service with i_fail_no_active_sends = 0 and i_fail_no_active_sends = 1");
  test_case_add(1, tet_svc_install_tp_15, "Installation of a service with same service id and i_fail_no_active_sends again on the same pwe handle, must fail");
  test_case_add(1, tet_svc_install_tp_16, "Installation of a service with same service id and different i_fail_no_active_sends again on the same pwe handle, must fail");
  test_case_add(1, tet_svc_install_tp_17, "For MDS_INSTALL API, supply i_yr_svc_hdl = (2^32) and i_yr_svc_hdl = (2^64 -1)");

  test_suite_add(2, "MDS TIPC API uninstall");
  test_case_add(2, tet_svc_unstall_tp_1, "Not able to Uninstall an ALREADY uninstalled service");
  test_case_add(2, tet_svc_unstall_tp_3, "Not able to Uninstall a Service which doesn't exist");
  test_case_add(2, tet_svc_unstall_tp_4, "UnInstalling wit invalid handles");
  test_case_add(2, tet_svc_unstall_tp_5, "Uninstalling a service in a seperate thread");

  test_suite_add(3, "Install sv up to MAX");
  test_case_add(3, tet_svc_install_upto_MAX, "tet_svc_install_upto_MAX");

  test_suite_add(4, "Subscribe VDEST");
  test_case_add(4, tet_svc_subscr_VDEST_1, "500 Subscription to:600,700 where Install scope = Subscription scope");
  test_case_add(4, tet_svc_subscr_VDEST_2, "500 Subscription to:600,700 where Install scope >Subscription scope (NODE)");
  test_case_add(4, tet_svc_subscr_VDEST_3, "Not able to: 500 Subscription to:600,700 where Install scope < Subscription scope(PWE)");
  test_case_add(4, tet_svc_subscr_VDEST_4, "Not able to: 500 Subscription to:600,700 where Install scope < Subscription scope(PWE)");
  test_case_add(4, tet_svc_subscr_VDEST_5, "Not able to Cancel the subscription which doesn't exist");
  test_case_add(4, tet_svc_subscr_VDEST_6, "A service subscribing for Itself");
  test_case_add(4, tet_svc_subscr_VDEST_7, "A Service which is NOT installed; trying to subscribe for 600, 700");
  test_case_add(4, tet_svc_subscr_VDEST_8, "Able to subscribe when numer of services to be subscribed is = 0");
  test_case_add(4, tet_svc_subscr_VDEST_9, "Not able to subscribe when supplied services array is NULL");
  test_case_add(4, tet_svc_subscr_VDEST_10, "Cross check whether i_rem_svc_pvt_ver returned in service event callback, matches the service private sub-part version of the remote service (subscribee)");
  test_case_add(4, tet_svc_subscr_VDEST_11, "When the Subscribee comes again with different sub-part version, cross check these changes are properly returned in the service event callback");
  test_case_add(4, tet_svc_subscr_VDEST_12, "In the NO_ACTIVE event notification, the remote service subpart version is set to the last active instance.s remote-service sub-part version");

  test_suite_add(5, "Subscribe ADEST");
  test_case_add(5, tet_svc_subscr_ADEST_1, "In the NO_ACTIVE event notification, the remote service subpart version is set to the last active instance.s remote-service sub-part version");
  test_case_add(5, tet_svc_subscr_ADEST_2, "Subscription to:600,700 where Install scope > Subscription scope (NODE)");
  test_case_add(5, tet_svc_subscr_ADEST_3, "Not able to: 500 Subscription to:600,700 where install scope < Subscription scope(PWE)");
  test_case_add(5, tet_svc_subscr_ADEST_4, "Not able to subscribe again:500 Subscribing to service 600");
  test_case_add(5, tet_svc_subscr_ADEST_5, "Not able to Cancel the subscription which doesn't exist");
  test_case_add(5, tet_svc_subscr_ADEST_6, "A service subscribing for Itself");
  test_case_add(5, tet_svc_subscr_ADEST_7, "A Service which is NOT installed; trying to subscribe for 600, 700");
  test_case_add(5, tet_svc_subscr_ADEST_8, "500 Subscription to:600,700 Cancel this Subscription in a seperate thread");
  test_case_add(5, tet_svc_subscr_ADEST_9, "500 Subscription to:600,700 in two seperate Subscription calls but Cancels both in a single cancellation call");
  test_case_add(5, tet_svc_subscr_ADEST_10, "500 Subscription to:600,700 Cancel this Subscription in a seperate thread");
  test_case_add(5, tet_svc_subscr_ADEST_11, "Cross check whether i_rem_svc_pvt_ver returned in service event callback, matches the service private sub-part version of the remote service (subscribee)");

  test_suite_add(6, "SIMPLE SEND TEST CASES");
  test_case_add(6, tet_just_send_tp_1, "Now send Low, Medium, High and Very High Priority messages to Svc EXTMIN on Active Vdest in sequence");
  test_case_add(6, tet_just_send_tp_2, "Not able to send a message to Svc EXTMIN with Invalid pwe handle");
  test_case_add(6, tet_just_send_tp_3, "Not able to send a message to Svc EXTMIN with NULL pwe handle");
  test_case_add(6, tet_just_send_tp_4, "Not able to send a message to Svc EXTMIN with  Wrong DEST");
  test_case_add(6, tet_just_send_tp_5, "Not able to send a message to service which is Not installed");
  test_case_add(6, tet_just_send_tp_6, "Able to send a messages 1000 times  to Svc 2000 on Active Vdest");
  test_case_add(6, tet_just_send_tp_7, "Send a message to unsubscribed Svc INTMIN on Active Vdest:Implicit/Explicit Combination");
  test_case_add(6, tet_just_send_tp_8, "Not able to send a message to Svc EXTMIN with Improper Priority");
  test_case_add(6, tet_just_send_tp_9, "Not able to send aNULL message to Svc EXTMIN on Active Vdest");
  test_case_add(6, tet_just_send_tp_10, "Now send a message( > MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN");
  test_case_add(6, tet_just_send_tp_11, "Not able to Send a message with Invalid Send type");
  test_case_add(6, tet_just_send_tp_12, "While Await Active timer is On: send a  message to Svc EXTMIN Vdest=200");
  test_case_add(6, tet_just_send_tp_13, "Send a message to Svc EXTMIN on QUIESCED Vdest=200");
  test_case_add(6, tet_just_send_tp_14, "Copy Callback returning Failure: Send Fails");

  test_suite_add(7, "SEND ACK test cases");
  test_case_add(7, tet_send_ack_tp_1, "Send LOW,MEDIUM,HIGH and VERY HIGH Priority messages with ACK to Svc on ACtive VDEST in sequence");
  test_case_add(7, tet_send_ack_tp_2, "Not able to send a message with ACK to 1700 with Invalid pwe handle");
  test_case_add(7, tet_send_ack_tp_3, "Not able to send a message with ACK to 1700 with NULL pwe handle");
  test_case_add(7, tet_send_ack_tp_4, "Not able to send a message with ACK to 1700 with Wrong DEST");
  test_case_add(7, tet_send_ack_tp_5, "Not able to send a message with ACK to service which is Not installed");
  test_case_add(7, tet_send_ack_tp_6, "Not able to send a message with ACK to OAC service on this DEST");
  test_case_add(7, tet_send_ack_tp_7, "Able to send a message with ACK 1000 times to service 1700");
  test_case_add(7, tet_send_ack_tp_8, "Send a message with ACK to unsubscribed service 1600");
  test_case_add(7, tet_send_ack_tp_9, "Not able to send_ack a message with Improper Priority to 1700");
  test_case_add(7, tet_send_ack_tp_10, "Not able to send a NULL message with ACK to 1700");
  test_case_add(7, tet_send_ack_tp_11, "Send a message( > MDS_DIRECT_BUF_MAXSIZE) to 1700");
  test_case_add(7, tet_send_ack_tp_12, "While Await Active timer is On: send_ack a message to Svc EXTMIN Vdest=200");
  test_case_add(7, tet_send_ack_tp_13, "Send_ack a message to Svc EXTMIN on QUIESCED Vdest=200");
 
  test_suite_add(8, "Query PWE test cases");
  test_case_add(8, tet_query_pwe_tp_1, "Get the details of all the Services on this core");
  test_case_add(8, tet_query_pwe_tp_2, "It shall not be able to query PWE with Invalid PWE Handle");
  test_case_add(8, tet_query_pwe_tp_3, "?????mds_query_vdest_for_anchor");

  test_suite_add(9, "Send Response test cases");
  test_case_add(9, tet_send_response_tp_1, "Svc INTMIN on PWE=2 of ADEST sends a LOW  Priority message to Svc NCSMDS_SVC_ID_EXTERNAL_MIN and expects a Response");
  test_case_add(9, tet_send_response_tp_2, "Svc INTMIN on PWE=2 of ADEST sends a MEDIUM Priority message to Svc EXTMIN and expects a Response");
  test_case_add(9, tet_send_response_tp_3, "Svc INTMIN on Active VEST=200 sends a HIGH Priority message to Svc EXTERNAL_MIN on ADEST and expects a Response");
  test_case_add(9, tet_send_response_tp_4, "Svc EXTMIN of ADEST sends a VERYHIGH Priority message to Svc EXTMIN on Active Vdest=200 and expects a Response");
  test_case_add(9, tet_send_response_tp_5, "While Await ActiveTimer ON:SvcEXTMIN of ADEST sends a message to Svc EXTMIN on Active Vdest=200 and Times out");
  test_case_add(9, tet_send_response_tp_6, "Svc EXTMIN of ADEST sends a VERYHIGH Priority message to Svc EXTMIN on QUIESCED Vdest=200 and Times out");
  test_case_add(9, tet_send_response_tp_7, "Implicit Subscription:Svc INTMIN on PWE=2 of ADEST sends a LOW Priority message to Svc EXTMIN and expects a Response");
  test_case_add(9, tet_send_response_tp_8, "Not able to send a message to a Service which doesn't exist");
  test_case_add(9, tet_send_response_tp_9, "Not able to send_response a message to Svc 2000 with Improper Priority");
  test_case_add(9, tet_send_response_tp_10, "Not able to send_response a NULL message to Svc EXTMIN on Active Vdest");
  test_case_add(9, tet_send_response_tp_11, "Now send_response a message(> MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN on Active Vdest");
  //TODO: Check this testcase
  //  test_case_add(9, tet_send_response_tp_12, "Able to send a messages 200 times to Svc  2000 on Active Vdest");

  test_suite_add(10, "Send All test cases");
  test_case_add(10, tet_send_all_tp_1, "Sender service installed with i_fail_no_active_sends = true and there is no-active instance of the receiver service");
  test_case_add(10, tet_send_all_tp_2, "Sender service installed with i_fail_no_active_sends = false and there is no-active instance of the receiver service");

  test_suite_add(11, "Send Responce Ack test cases");
  test_case_add(11, tet_send_response_ack_tp_1, "Svc EXTMIN on ADEST sends a LOW Priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(11, tet_send_response_ack_tp_2, "Svc EXTMIN on ADEST sends a MEDIUM Priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(11, tet_send_response_ack_tp_3, "Svc EXTMIN on ADEST sends a HIGH Priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(11, tet_send_response_ack_tp_4, "Svc EXTMIN on ADEST sends a VERYHIGH Priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(11, tet_send_response_ack_tp_5, "While Await Active Timer ON:SvcEXTMIN of ADEST sends a message to Svc EXTMIN on Active Vdest=200 and Times out");
  test_case_add(11, tet_send_response_ack_tp_6, "SvcEXTMIN of ADEST sends message to SvcEXTMIN on QUIESCED Vdest=200 and Times out");
  test_case_add(11, tet_send_response_ack_tp_7, "Implicit Subscription: Svc EXTL_MIN on ADEST sends a LOWPriority message to Svc EXTMIN on VDEST=200 and expects Response");
  test_case_add(11, tet_send_response_ack_tp_8, "send_response a message(> MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN on Active Vdest");

  test_suite_add(12, "Send Broadcast To SVC test cases");
  test_case_add(12, tet_broadcast_to_svc_tp_1, "Svc INTMIN on VDEST=200 Broadcasting a LOW Priority message to Svc EXTMIN");
  test_case_add(12, tet_broadcast_to_svc_tp_2, "Svc INTMIN on ADEST Broadcasting a MEDIUM Priority message to Svc EXTMIN");
  test_case_add(12, tet_broadcast_to_svc_tp_3, "Svc INTMIN on VDEST=200 Redundant Broadcasting a HIGH priority message to Svc EXTMIN");
  test_case_add(12, tet_broadcast_to_svc_tp_4, "Svc INTMIN on VDEST=200 Not able to Broadcast a message with Invalid Priority");
  test_case_add(12, tet_broadcast_to_svc_tp_5, "Svc INTMIN on VDEST=200 Not able to Broadcast a NULL message");
  test_case_add(12, tet_broadcast_to_svc_tp_6, "Svc INTMIN on VDEST=200 Broadcasting a VERY HIGH Priority message (>MDS_DIRECT_BUF_MAXSIZE) to Svc EXTMIN");

  test_suite_add(13, "Direct Just Send test cases");
  test_case_add(13, tet_direct_just_send_tp_1, "Test Case 1: Now Direct send Low,Medium,High and Very High Priority messages to Svc EXTMIN on Active Vdest=200");
  test_case_add(13, tet_direct_just_send_tp_2, "Not able to send a message to 2000 with Invalid pwe handle");
  test_case_add(13, tet_direct_just_send_tp_3, " Not able to send a message to 2000 with NULL pwe handle");
  test_case_add(13, tet_direct_just_send_tp_4, "Test Case 4: Not able to send a message to Svc EXTMIN on Vdest=200 with Wrong DEST");
  test_case_add(13, tet_direct_just_send_tp_5, " Not able to send a message to service which is Not installed");
  test_case_add(13, tet_direct_just_send_tp_6, "Able to send a message 1000 times  to Svc EXTMIN on Active Vdest=200");
  test_case_add(13, tet_direct_just_send_tp_7, "Implicit Subscription: Direct send a message to unsubscribed Svc INTMIN");
  test_case_add(13, tet_direct_just_send_tp_8, "Not able to send a message to Svc EXTMIN with Improper Priority");
  test_case_add(13, tet_direct_just_send_tp_9, "Not able to send a message to Svc EXTMIN with Direct Buff as NULL");
  test_case_add(13, tet_direct_just_send_tp_10, "Not able to send a message with Invalid Sendtype(<0 or >11)");
  test_case_add(13, tet_direct_just_send_tp_11, " Not able to send a message with Invalid Message length");
  test_case_add(13, tet_direct_just_send_tp_12, "While Await Active Timer ON: Direct send a Low Priority message to Svc EXTMIN on Vdest=200");
  test_case_add(13, tet_direct_just_send_tp_13, "Direct send a Medium Priority message to Svc EXTMIN on QUIESCED Vdest=200");
  test_case_add(13, tet_direct_just_send_tp_14, "Not able to send a message of size >(MDS_DIRECT_BUF_MAXSIZE) to 2000");
  test_case_add(13, tet_direct_just_send_tp_15, "Able to send a message of size =(MDS_DIRECT_BUF_MAXSIZE) to 200");

  test_suite_add(14, "Direct Send All test cases");
  test_case_add(14, tet_direct_send_all_tp_1, "Direct send a message with i_msg_fmt_ver = 0 for all send types");
 test_case_add(14, tet_direct_send_all_tp_2, "Direct send a message with i_msg_fmt_ver = i_rem_svc_pvt_ver for all send types");
 test_case_add(14, tet_direct_send_all_tp_3, "Direct send a message with i_msg_fmt_ver > i_rem_svc_pvt_ver for all send types");
test_case_add(14, tet_direct_send_all_tp_4, "Direct send a message with i_msg_fmt_ver < i_rem_svc_pvt_ver for all send types");
test_case_add(14, tet_direct_send_all_tp_5, "Direct send when Sender service installed with i_fail_no_active_sends = false and there is no-active instance of the receiver service");
test_case_add(14, tet_direct_send_all_tp_6, "Direct send when Sender service installed with i_fail_no_active_sends = true and there is no-active instance of the receiver service");

  test_suite_add(15, "Direct Send Ack test cases");
test_case_add(15,tet_direct_send_ack_tp_1, "Direct send_ack Low,Medium,High and Very High Priority message to Svc EXTMIN on Active Vdest=200 in sequence");
  test_case_add(15,tet_direct_send_ack_tp_2, "Not able to send_ack a message to 2000 with Invalid pwe handle");
 test_case_add(15,tet_direct_send_ack_tp_3, "Not able to send_ack a message to EXTMIN with NULL pwe handle");
  test_case_add(15,tet_direct_send_ack_tp_4, "Not able to send_ack a message to Svc EXTMIN on Vdest=200 with Wrong DEST");
  test_case_add(15,tet_direct_send_ack_tp_5, "Not able to send_ack a message to service which is Not installed");
  test_case_add(15,tet_direct_send_ack_tp_6, "Able to send_ack a message 1000 times  to Svc EXTMIN on Active Vdest=200");
  test_case_add(15,tet_direct_send_ack_tp_7, "Implicit Subscription: Direct send_ack a message to unsubscribed Svc INTMIN");
  test_case_add(15,tet_direct_send_ack_tp_8, "Not able to send_ack a message to Svc EXTMIN with Improper Priority");
  test_case_add(15,tet_direct_send_ack_tp_9, "Not able to send_ack a message with Invalid Sendtype(<0 or >11)");
  test_case_add(15,tet_direct_send_ack_tp_10, "Not able to send_ack a message to Svc EXTMIN with Direct Buff as NULL");
 test_case_add(15,tet_direct_send_ack_tp_11, "Not able to send_ack a message with Invalid Message length");
//TODO: For some strange reasons the tests 12 and 13 sometimes fail
 //test_case_add(15,tet_direct_send_ack_tp_12, "While Await Active Timer ON: Direct send_ack a message to Svc EXTMIN on Vdest=200");
//  test_case_add(15,tet_direct_send_ack_tp_13, "Direct send a Medium Priority message to Svc EXTMIN on QUIESCED Vdest=200");

  test_suite_add(16, "Direct Send Response test cases");
  test_case_add(16,tet_direct_send_response_tp_1, "Svc INTMIN on PWE=2 of ADEST sends a LOW Priority message to Svc EXTMIN and expects a Response");
  test_case_add(16,tet_direct_send_response_tp_2, "Svc INTMIN on PWE=2 of ADEST sends a MEDIUM Priority message to Svc EXTMIN and expects a Response");
  test_case_add(16,tet_direct_send_response_tp_3, "Svc INTMIN on PWE=2 of ADEST sends a HIGH Priority message to Svc EXTMIN and expects a Response");
  test_case_add(16,tet_direct_send_response_tp_4, "Svc INTMIN on PWE=2 of ADEST sends a VERYHIGH Priority message to Svc EXTMIN and expects a Response");
  test_case_add(16,tet_direct_send_response_tp_5, "Implicit/Explicit Svc INTMIN on PWE=2 of ADEST sends a message to Svc EXTMIN and expects a Response");
 
  test_suite_add(17, "Direct Send Response Ack test cases");
  test_case_add(17,tet_direct_send_response_ack_tp_1, "Svc EXTMIN on ADEST sends a LOW priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(17,tet_direct_send_response_ack_tp_2, "Svc EXTMIN on ADEST sends a MEDIUM priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(17,tet_direct_send_response_ack_tp_3, "Svc EXTMIN on ADEST sends a HIGH priority message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(17,tet_direct_send_response_ack_tp_4, " Svc EXTMIN on ADEST sends a VERYHIGH priority  message to Svc EXTMIN on VDEST=200 and expects a Response");
  test_case_add(17,tet_direct_send_response_ack_tp_5, "Implicit/Explicit Svc EXTMIN on ADEST sends a message to Svc EXTMIN on VDEST=200 and expects a Response");

  test_suite_add(18, "Direct Broadcast To SVC test cases");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_1, "Svc INTMIN on VDEST=200 Broadcasting a Low Priority message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_2, "Svc INTMIN on VDEST=200 Broadcasting a Medium Priority message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_3, "Svc INTMIN on VDEST=200 Broadcasting a High Priority message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_4, "Svc INTMIN on VDEST=200 Broadcasting a Very High Priority message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_5, "Svc INTMIN on ADEST Broadcasting a message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_6, "Svc INTMIN on VDEST=200 Redundant Broadcasting  a message to Svc EXTMIN");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_7, "Svc INTMIN on VDEST=200 Not able to Broadcast a message with Invalid Priority");
  test_case_add(18,tet_direct_broadcast_to_svc_tp_8, "Svc INTMIN on VDEST=200 Not able to Broadcast a message with NULL Direct Buff");
 
  test_suite_add(19, "ADEST test cases");
  test_case_add(19, tet_destroy_PWE_ADEST_twice_tp, "Destroy PWE ADEST twice");
  test_case_add(19, tet_create_PWE_ADEST_twice_tp, "Create PWE ADEST twice");
  test_case_add(19, tet_create_default_PWE_ADEST_tp , "Create default PWE ADEST");
  test_case_add(19, tet_create_PWE_upto_MAX_tp, "Create PWE upto MAX");
 
  test_suite_add(20, "VDEST test cases");
  test_case_add(20,tet_create_MxN_VDEST_1, "Creating a VDEST in MXN model with MIN vdest_id");
  test_case_add(20,tet_create_MxN_VDEST_2, "Not able to create a VDEST with vdest_id above the MAX RANGE");
  test_case_add(20,tet_create_MxN_VDEST_3, "Not able to create a VDEST with vdest_id = 0");
  test_case_add(20,tet_create_MxN_VDEST_4, "Create a VDEST with vdest_id below the MIN RANGE");

  test_suite_add(21, "Create N-Way VDEST test cases");
  test_case_add(21, tet_create_Nway_VDEST_1, "Creating a VDEST in N-way model with MAX vdest_id");
  test_case_add(21, tet_create_Nway_VDEST_2, "Not able to create a VDEST with vdest_id above the MAX RANGE");
  test_case_add(21, tet_create_Nway_VDEST_3, "Not able to create a VDEST with vdest_id = 0");
  test_case_add(21, tet_create_Nway_VDEST_4, "Create a VDEST with vdest_id below the MIN RANGE");

  test_suite_add(22, "Create OAC VDEST test cases");
  test_case_add(22, tet_create_OAC_VDEST_tp_1, "Creating a OAC service on VDEST in MXN model");
  test_case_add(22, tet_create_OAC_VDEST_tp_2, "Creating a a OAC service on VDEST in N-way model");

  test_suite_add(23, "Destroy VDEST twice test cases");
  test_case_add(23, tet_destroy_VDEST_twice_tp_1, "Destroy ACTIVE MxN VDEST");
  test_case_add(23, tet_destroy_VDEST_twice_tp_2, "Destroy QUIESCED MxN VDEST");

  test_suite_add(24, "Change standby to queisced test cases");
  test_case_add(24, tet_chg_standby_to_queisced_tp_1, "Changing VDEST role to ACTIVE twice");
  test_case_add(24, tet_chg_standby_to_queisced_tp_2, "Changing VDEST role from ACTIVE to QUIESCED");
  test_case_add(24, tet_chg_standby_to_queisced_tp_3, "Changing VDEST role from QUIESCED to STANDBY");
  test_case_add(24, tet_chg_standby_to_queisced_tp_4, "Not able to Change VDEST role from STANDBY to QUIESCED");


  //Named VDEST currently not supported by MDS
  test_suite_add(25, "Create Named VDEST test cases (Named VDEST currently not supported by MDS (test cases outcomment)");
#if 0
  test_case_add(25, tet_create_named_VDEST_1, "Not able to Create a Named VDEST in MxN model with Persistence Flag= true");
  test_case_add(25, tet_create_named_VDEST_2, " Not able to Create a Named VDEST in N-Way model with Persistence Flag= true");
  test_case_add(25, tet_create_named_VDEST_3, "Able to Create a Named VDEST in MxN model with Persistence Flag=false");
  test_case_add(25, tet_create_named_VDEST_4, "Able to Create a Named VDEST in MxN model with OAC Flag= true");
  test_case_add(25, tet_create_named_VDEST_5, "Able to Create a Named VDEST in N-Way model with Persistence Flag=false");
  test_case_add(25, tet_create_named_VDEST_6, "Able to Create a Named VDEST in N-Way model with OAC Flag= true");
  test_case_add(25, tet_create_named_VDEST_7, "Check the vdest_id returned: if we Create a Named VDEST in N-Way model for the second time");
  test_case_add(25, tet_create_named_VDEST_8, " Looking for VDEST id associated with a NULL name");
  test_case_add(25, tet_create_named_VDEST_9, "Not able to Create a Named VDEST in MxN model with NULL name");
#endif

  test_suite_add(26, "PWE VDEST test cases");
  test_case_add(26, tet_test_PWE_VDEST_tp_1, "Creating a PWE with MAX PWE_id= 2000 on this VDEST");
  test_case_add(26, tet_test_PWE_VDEST_tp_2, "Not able to Create a PWE with PWE_id > MAX i.e > 2000 on this VDEST");
  test_case_add(26, tet_test_PWE_VDEST_tp_3, "Not able to Create a PWE with Invalid PWE_id = 0 on this VDEST");
  test_case_add(26, tet_test_PWE_VDEST_tp_4, "Creating a PWE with PWE_id= 10 on this VDEST");
  test_case_add(26, tet_test_PWE_VDEST_tp_5, "Not able to Destroy an Already Destroyed PWE on this VDEST");
  test_case_add(26, tet_test_PWE_VDEST_tp_6, "Creating a PWE with PWE_id= 20 and OAC service on this VDEST");
  test_case_add(26, tet_create_PWE_upto_MAX_VDEST, "create_PWE_upto_MAX_VDEST");
  test_case_add(26, tet_create_default_PWE_VDEST_tp, "create_default_PWE_VDEST");


}
