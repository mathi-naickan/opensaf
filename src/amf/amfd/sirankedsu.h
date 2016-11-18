/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
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

#ifndef AMF_AMFD_SIRANKEDSU_H_
#define AMF_AMFD_SIRANKEDSU_H_

class AVD_SIRANKEDSU {
 public:
  AVD_SIRANKEDSU(const std::string &name, uint32_t rank) : suname(name), saAmfRank(rank) {}
  const std::string& get_suname() const {return suname;}
  uint32_t get_sa_amf_rank() const {return saAmfRank;}

 private:
  std::string suname;
  uint32_t saAmfRank;
  // disallow copy and assign.
  AVD_SIRANKEDSU(const AVD_SIRANKEDSU&);
  void operator=(const AVD_SIRANKEDSU&);

};

#endif  // AMF_AMFD_SIRANKEDSU_H_
