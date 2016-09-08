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

#include <osaf_extended_name.h>
#include <map>
#include <string>
#include "saAis.h"
#include "ncsgl_defs.h"

//
class Amf {
public:
  static std::string to_string(const SaNameT *name) {
    return osaf_extended_name_borrow(name);
  }
};

class SaNameTWrapper {
public:
  SaNameTWrapper(const std::string& str) {
    osaf_extended_name_alloc(str.c_str(), &name);
  };
  ~SaNameTWrapper() {
    clear();
  };

  // note: SaNameT* will become invalid if this SaNameTWrapper is destroyed
  operator const SaNameT*() const {
    return &name;
  };
 
  // note: SaNameT will become invalid if this SaNameTWrapper is destroyed
  operator const SaNameT() const {
    return name;
  }

  void set(const std::string& str) {
    osaf_extended_name_free(&name);
    osaf_extended_name_alloc(str.c_str(), &name);
  };

  void clear() {
    osaf_extended_name_free(&name);
  };

  SaNameTWrapper(const SaNameTWrapper&) = delete;
  SaNameTWrapper& operator=(const SaNameTWrapper&) = delete;
  SaNameTWrapper() = delete;

private:
  SaNameT name{};
};
	
//
template <typename Key, typename T>
class AmfDb {
  public:
   unsigned int insert(const Key &key, T *obj);
   void erase(const Key &key);
   void deleteAll();
   T *find(const Key &name);
   T *findNext(const Key &name);
   
   typedef std::map<Key, T*> AmfDbMap;
   typedef typename AmfDbMap::const_iterator const_iterator;
   typedef typename AmfDbMap::iterator iterator;
   typedef typename AmfDbMap::const_reverse_iterator const_reverse_iterator;

   const_iterator begin() const {return db.begin();}
   const_iterator end() const {return db.end();}
   typename AmfDbMap::size_type size() const {return db.size();}
   const_reverse_iterator rbegin() const {return db.rbegin();}
   const_reverse_iterator rend() const {return db.rend();}

   iterator erase(const iterator &it) {return db.erase(it);}

   iterator begin() {return db.begin();}
   iterator end() {return db.end();}

   const_iterator cbegin() const {return db.cbegin();}
   const_iterator cend() const {return db.cend();}

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
void AmfDb<Key, T>::deleteAll() {
  for (const auto& it: db) {
	delete it.second;
  }
  db.clear();
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

template <typename Key, typename T>
T * AmfDb<Key, T>::findNext(const Key &key) {
  typename AmfDbMap::iterator it = db.find(key);
  if (it == db.end()) {
    return 0;
  }

  it++;
  if (it == db.end())
    return 0;
  else {
    return it->second;
  }
}

#endif	/* DB_TEMPLATE_H */
