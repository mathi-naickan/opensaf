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


/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include "ntf/ntfd/NtfReader.h"
#include "ntf/ntfd/NtfLogger.h"
#include <iostream>
#include "base/logtrace.h"
#include "ntf/ntfd/NtfNotification.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */


/**
 *   NtfReader object creates a copy of the current cashed
 *   notifications. The private forward iterator ffIter is set
 *   to first element.
 *   @param ntfLogger
 *   @param readerId
 *
 */
NtfReader::NtfReader(NtfLogger& ntfLogger, unsigned int readerId):
    coll_(ntfLogger.coll_),
    ffIter(coll_.begin()),
    readerId_(readerId),
    c_filter_(0),
    firstRead(true) {
  searchCriteria_.eventTime = 0;
  searchCriteria_.notificationId = 0;
  searchCriteria_.searchMode = SA_NTF_SEARCH_NOTIFICATION_ID;
  TRACE_3("ntfLogger.coll_.size: %u", (unsigned int)ntfLogger.coll_.size());
}

/**
 *   NtfReader object creates a copy of the current cashed
 *   notifications that matches the filter and the search
 *   criteria. The private forward iterator ffIter is set
 *   according to the searchCriteria. This constructor is used
 *   when filter is provided.
 *
 *   @param ntfLogger
 *   @param readerId
 *   @param searchCriteria
 *   @param f_rec - filter record
 *   @param firstRead
 */
NtfReader::NtfReader(NtfLogger& ntfLogger,
                     unsigned int readerId,
                     SaNtfSearchCriteriaT searchCriteria,
                     ntfsv_filter_ptrs_t *f_rec):readerId_(readerId),searchCriteria_(searchCriteria),c_filter_(NtfCriteriaFilter::getCriteriaFilter(searchCriteria, this)),firstRead(true)
{
  TRACE_3("New NtfReader with filter, ntfLogger.coll_.size: %u", (unsigned int)ntfLogger.coll_.size());
  if (f_rec->alarm_filter) {
    NtfFilter* filter = new NtfAlarmFilter(f_rec->alarm_filter);
    filterMap[filter->type()] = filter;
  }
  if (f_rec->sec_al_filter) {
    NtfFilter* filter = new NtfSecurityAlarmFilter(f_rec->sec_al_filter);
    filterMap[filter->type()] = filter;
  }
  filterCacheList(ntfLogger);
}

NtfReader::~NtfReader() {
  FilterMap::iterator posN = filterMap.begin();
  while (posN != filterMap.end()) {
    NtfFilter* filter = posN->second;
    TRACE_2("Delete filter type %#x", (int)filter->type());
    delete filter;
    filterMap.erase(posN++);
  }
  delete c_filter_;
}

/**
 * This method is called to check which notifications that
 * matches the filtering criterias.
 *
 * For those notifications that matches the different
 * notification type filter criterias (only alarms and security
 * alarms notification types is possible) a help object derived
 * from NtfCriteriaFilter class will save notifcations in the
 * NtfReader collection that matches the saNtfSearchCriteria
 * that has been set.
 *
 *   @param ntfLogger
 */
void NtfReader::filterCacheList(NtfLogger& ntfLogger) {
  readerNotificationListT::iterator rpos;
  for (rpos = ntfLogger.coll_.begin(); rpos != ntfLogger.coll_.end(); rpos++) {
    NtfSmartPtr n (*rpos);
    bool rv = false;
    FilterMap::iterator pos = filterMap.find(n->getNotificationType());
    if (pos != filterMap.end()) {
      NtfFilter* filter = pos->second;
      osafassert(filter);
      rv = filter->checkFilter(n);
    }
    if (rv){
      if (!c_filter_->filter(n))
        break;
    }
  }
  c_filter_->finalize();
}

/**
 *   This method returns the notification at the current
 *   position of the iterator ffIter if search direction is
 *   SA_NTF_SEARCH_YOUNGER. The first search will give the same
 *   result for both search directions.
 *
 *   @param searchDirection
 *   @param error - outparam tells if the operation succeded
 *   @return NtfNotification - an empty non initialized
 *           notification will be returned if error != SA_AIS_OK
 */
NtfSmartPtr
NtfReader::next(SaNtfSearchDirectionT direction,
                SaAisErrorT* error) {
  TRACE_ENTER();

  *error = SA_AIS_ERR_NOT_EXIST;

  if (direction == SA_NTF_SEARCH_YOUNGER ||
      searchCriteria_.searchMode == SA_NTF_SEARCH_AT_TIME ||
      searchCriteria_.searchMode == SA_NTF_SEARCH_ONLY_FILTER) {
    if (ffIter >= coll_.end()) {
      *error = SA_AIS_ERR_NOT_EXIST;
      TRACE_LEAVE();
      NtfSmartPtr notif;
      firstRead = false;
      return notif;
    }
    NtfSmartPtr notif(*ffIter);
    ffIter++;
    *error = SA_AIS_OK;
    TRACE_LEAVE();
    firstRead = false;
    return notif;
  }
  else  // SA_NTF_SEARCH_OLDER
  {
    readerNotReverseIterT rIter(ffIter);
    if (firstRead)
      rIter--;
    else
      rIter++;

    if (rIter >= coll_.rend() || rIter < coll_.rbegin()) {
      *error = SA_AIS_ERR_NOT_EXIST;
      ffIter = rIter.base();
      TRACE_LEAVE();
      NtfSmartPtr notif;
      firstRead = false;
      return notif;
    }
    NtfSmartPtr notif(*rIter);
    ffIter = rIter.base();
    *error = SA_AIS_OK;
    TRACE_LEAVE();
    firstRead = false;
    return notif;
  }
}

unsigned int NtfReader::getId() {
  return readerId_;
}

/**
 *   NtfCriteriaFilter is a base class for all searchCriteria
 *   filters. NtfCriteriaFilter and its derived classes are help
 *   classes to the NtfReader class. The derived classes must
 *   implementer the filter() method.
 *
 *   @param searchCriteria
 *   @param reader
 */
NtfCriteriaFilter::NtfCriteriaFilter(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):
    searchCriteria_(searchCriteria), reader_(reader), i_(-1), startIdx_(0) {
}

NtfCriteriaFilter::~NtfCriteriaFilter() {
}

void NtfCriteriaFilter::add(NtfSmartPtr& n) {
  reader_->coll_.push_back(n);
  i_++;
}

void NtfCriteriaFilter::setIndexCurrent() {
  if (reader_->coll_.size()) {
    startIdx_=i_;
  }
}

void NtfCriteriaFilter::removeTail() {
  if (reader_->coll_.size()) {
    reader_->coll_.erase(reader_->coll_.begin() +  startIdx_ + 1, reader_->coll_.end());
    i_= startIdx_;
  };
}

void NtfCriteriaFilter::removeHead() {
  if (reader_->coll_.size()) {
    reader_->coll_.erase(reader_->coll_.begin(), reader_->coll_.begin() + startIdx_);
    i_ -= startIdx_;
    startIdx_ = 0;
  }
}

void NtfCriteriaFilter::finalize() {
  startIdx_=0;
  convertToIter();
}

void NtfCriteriaFilter::convertToIter() {
  reader_->ffIter = reader_->coll_.begin() + startIdx_;
}

NtfBeforeAtTime::NtfBeforeAtTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader), eTimeFound_(false) {
  TRACE_3("NtfBeforeAtTime constructor");
}

bool NtfBeforeAtTime::filter(NtfSmartPtr& n) {
  add(n);
  if (*n->header()->eventTime == searchCriteria_.eventTime) {
    setIndexCurrent();
    eTimeFound_ = true;
  }
  return true;
}

void NtfBeforeAtTime::finalize() {
  TRACE_3("NtfBeforeAtTime finalize");
  if(eTimeFound_) {
    removeTail();
  }
  setIndexCurrent();
  convertToIter();
}

AtTime::AtTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader) {
  TRACE_3("AtTime constructor");
}

bool AtTime::filter(NtfSmartPtr& n) {
  if (*n->header()->eventTime == searchCriteria_.eventTime) {
    add(n);
  }
  return true;
}

NtfAtOrAfterTime::NtfAtOrAfterTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader),indexSaved_(false), eTimeFound_(false) {
  TRACE_3("NtfAtOrAfterTime constructor");
}

bool NtfAtOrAfterTime::filter(NtfSmartPtr& n) {
  add(n);
  if (*n->header()->eventTime == searchCriteria_.eventTime) {
    if (!eTimeFound_) {
      setIndexCurrent();
      indexSaved_ = true;
      eTimeFound_ = true;
    }
  } else if (!eTimeFound_ && (*n->header()->eventTime > searchCriteria_.eventTime)) {
    if (!indexSaved_) {
      setIndexCurrent();
      indexSaved_ = true;
    }
  }
  return true;
}

void NtfAtOrAfterTime::finalize() {
  if (indexSaved_) {
    removeHead();
  }
  convertToIter();
}

NtfBeforeTime::NtfBeforeTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader),indexSaved_(false), eTimeFound_(false) {
  TRACE_3("NtfBeforeTime constructor");
}

bool NtfBeforeTime::filter(NtfSmartPtr& n) {
  if (*n->header()->eventTime == searchCriteria_.eventTime) {
    if (!indexSaved_) {
      setIndexCurrent();
      indexSaved_ = true;
      eTimeFound_     = true;
    }
    return false;
  } else if (!eTimeFound_ && (*n->header()->eventTime > searchCriteria_.eventTime)) {
    if (!indexSaved_) {
      setIndexCurrent();
      indexSaved_ = true;
    }
  }
  add(n);
  return true;
}

void NtfBeforeTime::finalize() {
  if (!indexSaved_) {
    setIndexCurrent();
  } else {
    if(eTimeFound_) {
      removeTail();
    }
    setIndexCurrent();
  }
  convertToIter();
}

NtfAfterTime::NtfAfterTime(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader),indexSaved_(false), eTimeFound_(false) {
  TRACE_3("NtfAfterTime constructor");
}

bool NtfAfterTime::filter(NtfSmartPtr& n) {
  add(n);
  if (*n->header()->eventTime == searchCriteria_.eventTime) {
    if (!eTimeFound_) {
      indexSaved_ = true;
      eTimeFound_ = true;
    }
    setIndexCurrent();
  } else if (eTimeFound_ && (*n->header()->eventTime > searchCriteria_.eventTime)) {
    if (!indexSaved_) {
      setIndexCurrent();
      indexSaved_ = true;
    }
  }
  return true;
}

void NtfAfterTime::finalize() {
  if (indexSaved_) {
    startIdx_++;
    removeHead();
  }
  convertToIter();
}

NtfOnlyFilter::NtfOnlyFilter(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader) {
  TRACE_3("NtfOnlyFilter constructor");
}


bool NtfOnlyFilter::filter(NtfSmartPtr& n) {
  unsigned int nId =  n->getNotificationId();
  TRACE_3("nId: %u added to readerList", nId);
  add(n);
  return true;
}

NtfIdSearch::NtfIdSearch(SaNtfSearchCriteriaT& searchCriteria, NtfReader* reader):NtfCriteriaFilter(searchCriteria,reader) {
  TRACE_3("NtfIdSearch constructor");
}

bool NtfIdSearch::filter(NtfSmartPtr& n) {
  if (*n->header()->notificationId == searchCriteria_.notificationId) {
    add(n);
    return false;
  }
  return true;
}

NtfCriteriaFilter* NtfCriteriaFilter::getCriteriaFilter(SaNtfSearchCriteriaT& sc, NtfReader* r) {
  NtfCriteriaFilter* f;
  switch (sc.searchMode) {
    case SA_NTF_SEARCH_BEFORE_OR_AT_TIME:
      f = (NtfCriteriaFilter*) new NtfBeforeAtTime(sc, r);
      break;
    case SA_NTF_SEARCH_AT_TIME:
      f = (NtfCriteriaFilter*) new AtTime(sc, r);
      break;
    case    SA_NTF_SEARCH_AT_OR_AFTER_TIME:
      f = (NtfCriteriaFilter*) new NtfAtOrAfterTime(sc, r);
      break;
    case SA_NTF_SEARCH_BEFORE_TIME:
      f = (NtfCriteriaFilter*) new NtfBeforeTime(sc, r);
      break;
    case SA_NTF_SEARCH_AFTER_TIME:
      f = (NtfCriteriaFilter*) new NtfAfterTime(sc, r);
      break;
    case SA_NTF_SEARCH_NOTIFICATION_ID:
      f = (NtfCriteriaFilter*) new NtfIdSearch(sc, r);
      break;
    case SA_NTF_SEARCH_ONLY_FILTER:
    default:
      f= (NtfCriteriaFilter*) new NtfOnlyFilter(sc, r);
      break;
  }
  return f;
}

