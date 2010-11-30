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

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This file includes the macros & routines to manage the component external 
  active monitoring .
 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include "avnd.h"

/****************************************************************************
  Name          : avnd_comp_am_start
 
  Description   : This routine encapsulates the actual AM_START CLC script 
                  execution.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the COMP Struct.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_am_start(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_CLC_INFO *clc_info = &comp->clc_info;
	AVND_ERR_INFO err;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* check if its instantiated state, else exit */
	if (!m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(comp))
		goto done;

	/* check if its less than max cnt */
	if (clc_info->am_start_retry_cnt < clc_info->am_start_retry_max) {
		clc_info->am_start_retry_cnt++;

		/* exec AM_START script */
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_AMSTART);
	} else {
      /*** comp error  processing  ***/
		err.src = AVND_ERR_SRC_AM;
		err.rec_rcvr.avsv_ext = comp->err_info.def_rec;
		rc = avnd_err_process(cb, comp, &err);
	}

 done:
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_am_oper_req_process
 
  Description   : triggers the amstart/amstop if its enabled and not currently
                  running. 

  Arguments     : cb    - ptr to the AvND control block
                  comp  - ptr to the COMP struct.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_am_oper_req_process(AVND_CB *cb, AVND_COMP *comp)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	if (TRUE == comp->is_am_en)
		rc = avnd_comp_am_start(cb, comp);
	else {
		/* check if its instantiated state, else exit */
		if (m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(comp))
			rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_AMSTOP);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_amstart_clc_res_process
 
  Description   : triggers the amstart/amstop if its enabled and not currently
                  running. 

  Arguments     : cb    - ptr to the AvND control block
                  comp  - ptr to the COMP struct.
                  value - exit value of the clc command.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_amstart_clc_res_process(AVND_CB *cb, AVND_COMP *comp, NCS_OS_PROC_EXEC_STATUS value)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* reset the cmd exec context params */
	comp->clc_info.am_exec_cmd = AVND_COMP_CLC_CMD_TYPE_MAX;
	comp->clc_info.am_cmd_exec_ctxt = 0;

	if (NCS_OS_PROC_EXIT_NORMAL == value) {
		/* do nothing, just reset the count */
		comp->clc_info.am_start_retry_cnt = 0;
	} else {
		/* call amstop */
		rc = avnd_comp_clc_cmd_execute(cb, comp, AVND_COMP_CLC_CMD_TYPE_AMSTOP);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_amstop_clc_res_process
 
  Description   : triggers the amstart/amstop if its enabled and not currently
                  running. 

  Arguments     : cb    - ptr to the AvND control block
                  comp  - ptr to the COMP struct.
                  value - exit value of the clc command.
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_amstop_clc_res_process(AVND_CB *cb, AVND_COMP *comp, NCS_OS_PROC_EXEC_STATUS value)
{
	AVND_ERR_INFO err;
	uns32 rc = NCSCC_RC_SUCCESS;

	memset(&err, '\0', sizeof(AVND_ERR_INFO));

	/* reset the cmd exec context params */
	comp->clc_info.am_exec_cmd = AVND_COMP_CLC_CMD_TYPE_MAX;
	comp->clc_info.am_cmd_exec_ctxt = 0;

	if (NCS_OS_PROC_EXIT_NORMAL == value) {
		/* check for state, if inst state, call am start, else do nothing */
		if (m_AVND_COMP_PRES_STATE_IS_INSTANTIATED(comp) && comp->clc_info.am_start_retry_cnt != 0)
			rc = avnd_comp_am_start(cb, comp);
	} else {
		/* as of now, restart feature of AM_STOP is not implemented 
		   the first failure of AM_STOP is taken as critical and AMF 
		   does error processing for the component.
		   Do not process error, if the comp is already marked faulty,
		 */

		if (!m_AVND_COMP_IS_FAILED(comp)) {

	/*** process error ***/
			err.src = AVND_ERR_SRC_AM;
			err.rec_rcvr.avsv_ext = comp->err_info.def_rec;
			rc = avnd_err_process(cb, comp, &err);
		}
	}

	return rc;
}
