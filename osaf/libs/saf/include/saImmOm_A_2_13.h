/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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
 */

/*
 * DESCRIPTION:
 *   This file provides the suggested additions to the C language binding for 
 *   the Service Availability(TM) Forum Information Model Management Service (IMM).
 *   It contains only the prototypes and type definitions that are part of this
 *   proposed addition. 
 *   These additions are currently NON STANDARD. But the intention is to get these
 *   additions approved formally by SAF in the future.
 *
 *   For detailed explanation of the new API, see osaf/services/saf/immsv/README.
 */


#ifndef _SA_IMM_OM_A_2_13_H
#define _SA_IMM_OM_A_2_13_H

#ifdef  __cplusplus
extern "C" {
#endif


	/* 4.2.5 SaImmAttrFlagsT */
	/*
#define SA_IMM_ATTR_MULTI_VALUE   0x00000001
#define SA_IMM_ATTR_RDN           0x00000002
#define SA_IMM_ATTR_CONFIG        0x00000100
#define SA_IMM_ATTR_WRITABLE      0x00000200
#define SA_IMM_ATTR_INITIALIZED   0x00000400
#define SA_IMM_ATTR_RUNTIME       0x00010000
#define SA_IMM_ATTR_PERSISTENT    0x00020000
#define SA_IMM_ATTR_CACHED        0x00040000
#define SA_IMM_ATTR_NO_DUPLICATES 0x0000000001000000 / * See: http://devel.opensaf.org/ticket/1545
							Supported in OpenSaf 4.3 * /
#define SA_IMM_ATTR_NOTIFY        0x0000000002000000 / * See: http://devel.opensaf.org/ticket/2883
							Supported in OpenSaf 4.3 * /
	*/
#define SA_IMM_ATTR_NO_DANGLING   0x0000000004000000    /* See: https://sourceforge.net/p/opensaf/tickets/49/
                                                         Supported in OpenSaf 4.4 */


    /* 4.2.12 SaImmSearchOptionsT */
    /*
#define SA_IMM_SEARCH_ONE_ATTR       0x0001
#define SA_IMM_SEARCH_GET_ALL_ATTR   0x0100
#define SA_IMM_SEARCH_GET_NO_ATTR    0x0200
#define SA_IMM_SEARCH_GET_SOME_ATTR  0x0400
#define SA_IMM_SEARCH_GET_CONFIG_ATTR 0x0000000000010000   / * See: http://devel.opensaf.org/ticket/1897
                          Supported in OpenSaf 4.3 * /
    */
#define SA_IMM_SEARCH_NO_DANGLING_DEPENDENTS  0x0000000100000000    /* See: https://sourceforge.net/p/opensaf/tickets/50/
                                                                       Supported in OpenSaf 4.4 */


#ifdef  __cplusplus
}
#endif

#endif   /* _SA_IMM_OM_A_2_13_H */
