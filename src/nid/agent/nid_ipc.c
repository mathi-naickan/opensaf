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

/************************************************************************
*                                                                       *
* Module Name: NIDFIFO                                                  *
*                                                                       *
* Purpose: Provides intarfaces for opening, closing,                    *
* reading and writing using FIFO as IPC mechanism                       *
* between nodeinitd and services spawned by it.                         *
* This module is not expected to be a shared                            *
* library.                                                              *
************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include "osaf/configmake.h"

#include "nid/agent/nid_api.h"

/****************************************************************
*       FIFO Used To communicate to nodeinitd                  *
****************************************************************/
#define NID_FIFO	PKGLOCALSTATEDIR "/nodeinit.fifo"

int32_t fifo_fd = -1;

/****************************************************************************
 * Name          : nid_create_ipc                                           *
 *                                                                          *
 * Description   : Creates FIFO with predefined file name.                  *
 *                                                                          *
 * Arguments     : strbuf - To return a descriptive error message if any    * 
 *                                                                          *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
uint32_t nid_create_ipc(char *strbuf)
{
	mode_t mask;

	/* Lets Remove any such file if it already exists */
	unlink(NID_FIFO);

	mask = umask(0);

	/* Create nid fifo */
	if (mkfifo(NID_FIFO, 0666) < 0) {
		sprintf(strbuf, " FAILURE: Unable To Create FIFO Error:%s\n", strerror(errno));
		umask(mask);
		return NCSCC_RC_FAILURE;
	}

	umask(mask);
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : nid_open_ipc                                             *
 *                                                                          *
 * Description   : Opens FIFO with predefined file name  and returns the fd *
 *                                                                          *
 * Arguments     : fd -  To return fifo_fd                                  *
 *                 strbuf - To return descriptive error message if any.     *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
uint32_t nid_open_ipc(int32_t *fd, char *strbuf)
{
	/* Try to open FIFO if its not already open */
	if (fifo_fd < 0) {
		if ((fifo_fd = open(NID_FIFO, O_RDWR | O_NONBLOCK)) < 0) {
			sprintf(strbuf, "NID FAILURE: Unable To Open FIFO Error:%s\n", strerror(errno));
			return NCSCC_RC_FAILURE;
		}
	}

	*fd = fifo_fd;
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : nid_close_ipc                                            *
 *                                                                          *
 * Description   : Closes the fifo.                                         *
 *                                                                          *
 * Arguments     : None.                                                    *
 *                                                                          *
 * Return Values : None.                                                    *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
void nid_close_ipc(void)
{
	close(fifo_fd);
	fifo_fd = -1;
}

/****************************************************************************
 * Name          : nid_is_ipcopen                                           *
 *                                                                          *
 * Description   : Checks if FIFO is open.                                  *
 *                                                                          *
 * Arguments     : None.                                                    *
 *                                                                          *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.                       *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
uint32_t nid_is_ipcopen(void)
{
	if (fifo_fd < 0)
		return NCSCC_RC_FAILURE;

	return NCSCC_RC_SUCCESS;
}
