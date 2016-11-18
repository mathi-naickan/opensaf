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
 * Author(s): Ericsson AB
 *
 */


#ifndef NTF_NTFD_NTFREADER_H_
#define NTF_NTFD_NTFREADER_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include "ntf/ntfd/NtfNotification.h"
#include "ntf/ntfd/NtfFilter.h"
/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */
typedef std::deque<NtfSmartPtr> readerNotificationListT;
typedef std::deque<NtfSmartPtr>::iterator readerNotificationListTIter;
typedef std::deque<NtfSmartPtr>::reverse_iterator readerNotReverseIterT;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
class NtfLogger;
class NtfCriteriaFilter;

class NtfReader
{
  friend class NtfCriteriaFilter;
 public:
  NtfReader(NtfLogger& ntfLogger, unsigned int readerId);
  NtfReader(NtfLogger& ntfLogger,
            unsigned int readerId,
            SaNtfSearchCriteriaT searchCriteria,
            ntfsv_filter_ptrs_t *f_rec);
  ~NtfReader();
  void filterCacheList(NtfLogger& ntfLogger);
  NtfSmartPtr next(SaNtfSearchDirectionT direction,
                   SaAisErrorT* error);
  unsigned int getId();

 private:
  readerNotificationListT coll_;
  readerNotificationListTIter ffIter;
  FilterMap filterMap;
  unsigned int readerId_;
  SaNtfSearchCriteriaT searchCriteria_;
  NtfCriteriaFilter* c_filter_;
  bool firstRead;
};

class NtfCriteriaFilter
{
 public:
  NtfCriteriaFilter(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  virtual ~NtfCriteriaFilter();
  static NtfCriteriaFilter* getCriteriaFilter(SaNtfSearchCriteriaT& sc, NtfReader* r);
  virtual bool filter(NtfSmartPtr& n)=0;
  void add(NtfSmartPtr& n);
  void setIndexCurrent();
  virtual void finalize();
  void removeTail();
  void removeHead();
  void convertToIter();
 protected:
  SaNtfSearchCriteriaT searchCriteria_;
  NtfReader* reader_;
  int i_;
  int startIdx_;
};

class NtfBeforeAtTime:public NtfCriteriaFilter
{
 public:
  NtfBeforeAtTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
  void finalize();
  bool eTimeFound_;
};

class AtTime:public NtfCriteriaFilter
{
 public:
  AtTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
};

class NtfAtOrAfterTime:public NtfCriteriaFilter
{
 public:
  NtfAtOrAfterTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
  void finalize();
 private:
  bool indexSaved_;
  bool eTimeFound_;
};

class NtfBeforeTime:public NtfCriteriaFilter
{
 public:
  NtfBeforeTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
  void finalize();

 private:
  bool indexSaved_;
  bool eTimeFound_;
};

class NtfAfterTime:public NtfCriteriaFilter
{
 public:
  NtfAfterTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
  void finalize();
 private:
  bool indexSaved_;
  bool eTimeFound_;
};

class NtfOnlyFilter:public NtfCriteriaFilter
{
 public:
  NtfOnlyFilter(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
};

class NtfIdSearch:public NtfCriteriaFilter
{
 public:
  NtfIdSearch(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader);
  bool filter(NtfSmartPtr& n);
};

#endif  // NTF_NTFD_NTFREADER_H_

