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

  This file contains macros for memory operations.
  
******************************************************************************
*/

#ifndef AMF_AMFD_DEF_H_
#define AMF_AMFD_DEF_H_

/* The value to toggle a SI.*/
typedef enum {
	AVSV_SI_TOGGLE_STABLE,
	AVSV_SI_TOGGLE_SWITCH
} SaToggleState;

/* The value to re adjust a SG.*/
typedef enum {
	AVSV_SG_STABLE,
	AVSV_SG_ADJUST
} SaAdjustState;

#endif  // AMF_AMFD_DEF_H_
