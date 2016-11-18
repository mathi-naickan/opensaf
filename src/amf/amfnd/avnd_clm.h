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

  This module is the include file for handling Availability Node Directors 
  CLM structures
  
******************************************************************************
*/

#ifndef AMF_AMFND_AVND_CLM_H_
#define AMF_AMFND_AVND_CLM_H_

struct avnd_cb_tag;

extern SaAisErrorT avnd_clm_init(avnd_cb_tag* cb);
extern SaAisErrorT avnd_start_clm_init_bg();

#endif  // AMF_AMFND_AVND_CLM_H_
