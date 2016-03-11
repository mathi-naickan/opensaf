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
 * Author(s): Ericsson AB
 *
 */
#ifndef DB_TEMPLATE_H
#define	DB_TEMPLATE_H

#include <map>
#include <string>
#include "saAis.h"
#include "ncsgl_defs.h"

//
class Amf {
public:
  static std::string to_string(const SaNameT *name) {
	  return std::string((char*)name->value, name->length);
  }
};

//
template <typename Key, typename T>
class AmfDb {
  public:
   unsigned int insert(const Key &key, T *obj);
   void erase(const Key &key);
   T *find(const Key &name);
   
   typedef std::map<Key, T*> AmfDbMap;
   typedef typename AmfDbMap::const_iterator const_iterator;
   typedef typename AmfDbMap::const_reverse_iterator const_reverse_iterator;

   const_iterator begin() const {return db.begin();}
   const_iterator end() const {return db.end();}
   const_reverse_iterator rbegin() const {return db.rbegin();}
   const_reverse_iterator rend() const {return db.rend();}

  private:
   AmfDbMap db;
};

//
template <typename Key, typename T>
unsigned int AmfDb<Key, T>::insert(const Key &key, T *obj) {
  osafassert(obj);
  
  if (db.insert(std::make_pair(key, obj)).second) {
    return 1; // NCSCC_RC_SUCCESS
  }
   else {
      return 2; // Duplicate (NCSCC_RC_FAILURE)
    }
 }

//
template <typename Key, typename T>
void AmfDb<Key, T>::erase(const Key &key) {
  db.erase(key);
}

//
template <typename Key, typename T>
T *AmfDb<Key, T>::find(const Key &key) {
  typename AmfDbMap::iterator it = db.find(key);
  if (it == db.end())
    return 0;
  else
    return it->second;
}

#endif	/* DB_TEMPLATE_H */
