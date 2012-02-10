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

#include "ImmAttrValue.hh"
#include "string.h"
#include "immnd.h"


ImmAttrValue::ImmAttrValue() : 
    mValue(0), 
    mValueSize(0) 
{
}

ImmAttrValue::ImmAttrValue(const ImmAttrValue& b) :
    mValue(NULL), 
    mValueSize(b.mValueSize) 
{
    if(mValueSize) {
        mValue = new char[b.mValueSize];
        (void)::memcpy(mValue, b.mValue, b.mValueSize);
    }
}

ImmAttrValue::~ImmAttrValue() 
{
    delete [] mValue;
    mValue=0;
    mValueSize=0;
}

void
ImmAttrValue::printSimpleValue() const
{
    //printf("ImmAttrValue::printSimpleValue size: %u %p\n", mValueSize, mValue);
}

void
ImmAttrValue::print() const
{
    this->printSimpleValue();
}

ImmAttrValue&
ImmAttrValue::operator= (const ImmAttrValue& b)
{
    if (this != &b)
    {
        delete [] mValue;

        mValueSize = b.mValueSize;
        if(mValueSize) {
            mValue = new char[b.mValueSize];
            (void)::memcpy(mValue, b.mValue, b.mValueSize);
        }
    }

    return *this;
}

void
ImmAttrValue::setValue(const IMMSV_OCTET_STRING& in) 
{
    if(mValue) {
        if((in.size == mValueSize) && 
            (memcmp(mValue, in.buf, mValueSize) == 0)) {return;} //Already equal
        delete [] mValue;
        mValue=0;
    }

    mValueSize = (size_t)in.size;
    if(mValueSize) {
        mValue = new char[mValueSize];
        (void)::memcpy(mValue, in.buf, mValueSize);
    }
}

void
ImmAttrValue::discardValues() 
{
    if(mValue) {
        delete [] mValue;
        mValue=0;
        mValueSize=0;
    }
}

void
ImmAttrValue::setValue_int(int i)
{
    if(mValue && mValueSize != sizeof(int)) {
        delete [] mValue;
        mValue=0;
        mValueSize=0;
    }

    if(!mValue) {
        mValueSize = sizeof(int);
        mValue = new char[mValueSize];
    }

    *((int *) mValue) = i;
}

int
ImmAttrValue::getValue_int() const
{
    if(mValueSize != sizeof(int)) {return 0;}
    
    return *((int *) mValue);
}

void
ImmAttrValue::setValueC_str(const char* str)
{
    if(mValue) {
        if(str) {
            if(strncmp(mValue, str, mValueSize) == 0) {return;} //Already equal
        }
        delete [] mValue;
        mValue=0;
        mValueSize=0;
    }
    
    if(str) {
        mValueSize=  (unsigned int) strlen(str) + 1;
        mValue = new char[mValueSize];
        strncpy(mValue, str, mValueSize);
    }
}

const char* 
ImmAttrValue::getValueC_str() const
{
    return mValue;
}

void
ImmAttrValue::copyValueToEdu(IMMSV_EDU_ATTR_VAL* out, SaImmValueTypeT t) const
{
    if (!mValueSize) {
        memset(out, 0, sizeof(IMMSV_EDU_ATTR_VAL));
        return;
    }
    
    osafassert(mValue);
    
    switch(t)
    {
        case SA_IMM_ATTR_SAINT32T: 
            out->val.saint32 = *((SaInt32T *) mValue);
            return;
        case SA_IMM_ATTR_SAUINT32T: 
            out->val.sauint32 = *((SaUint32T *) mValue);
            return;
        case SA_IMM_ATTR_SAINT64T: 
            out->val.saint64 = *((SaInt64T *) mValue);
            return;
        case SA_IMM_ATTR_SAUINT64T: 
            out->val.sauint64 = *((SaUint64T *) mValue);
            return;
        case SA_IMM_ATTR_SATIMET:
            out->val.satime = *((SaTimeT *) mValue);
            return;
        case SA_IMM_ATTR_SAFLOATT:
            out->val.safloat = *((SaFloatT *) mValue);
            return;
        case SA_IMM_ATTR_SADOUBLET:
            out->val.sadouble = *((SaDoubleT *) mValue);
            return;
            
        case SA_IMM_ATTR_SASTRINGT:
        case SA_IMM_ATTR_SAANYT:
        case SA_IMM_ATTR_SANAMET:
            out->val.x.size = mValueSize;
            out->val.x.buf = (char *) malloc(mValueSize);
            memcpy(out->val.x.buf, mValue, mValueSize);
            
            break;
            
        default:
            abort();
    }
}


unsigned int
ImmAttrValue::extraValues() const //virtual
{
    return 0;
}

bool
ImmAttrValue::isMultiValued() const //virtual
{
    return false;
}

void
ImmAttrValue::removeValue(const IMMSV_OCTET_STRING& match) //virtual
{
    if((mValueSize ==  match.size) &&
        (bcmp((const void *) mValue, (const void *) match.buf, 
            mValueSize) == 0)) {
        this->discardValues();
    }
}

bool
ImmAttrValue::hasMatchingValue(const IMMSV_OCTET_STRING& match) const //virtual
{
    if(mValueSize != match.size) return false;
    
    return bcmp((const void *) mValue, (const void *) match.buf, 
        mValueSize) == 0 ;
}

ImmAttrMultiValue::ImmAttrMultiValue() : 
    ImmAttrValue(),
    mNext(NULL)
{
}

ImmAttrMultiValue::ImmAttrMultiValue(const ImmAttrMultiValue& b) :
    ImmAttrValue(b)
{
    if(b.mNext) {
        mNext = new ImmAttrMultiValue(*(b.mNext));
    } else {
        mNext=NULL;
    }
}

ImmAttrMultiValue::ImmAttrMultiValue(const ImmAttrValue& b) :
    ImmAttrValue(b)
{
    mNext = NULL;
}

ImmAttrMultiValue::~ImmAttrMultiValue() 
{
    if(mValue) {
        delete [] mValue;
        mValue=0;
        mValueSize=0;
    }
    if(mNext) {
        delete mNext;
        mNext=0;
    }
}

ImmAttrMultiValue&
ImmAttrMultiValue::operator= (const ImmAttrMultiValue& b)
{
    if (this != &b)
    {
        delete [] mValue;
        mValue=0;
        mValueSize = b.mValueSize;
        if(mValueSize) {
            mValue = new char[b.mValueSize];
            (void)::memcpy(mValue, b.mValue, b.mValueSize);
        }
    }
    
    if(mNext) {
        delete mNext;
        mNext=NULL;
    }
    
    if(b.mNext) {
        mNext = new ImmAttrMultiValue(*(b.mNext));
    }
    return *this;
}

void
ImmAttrMultiValue::discardValues() //virtual
{
    if(mValue) {
        delete [] mValue;
        mValue=0;
        mValueSize=0;
    }
    
    if(mNext) {
        delete mNext;
        mNext=NULL;
    }
}

void
ImmAttrMultiValue::removeValue(const IMMSV_OCTET_STRING& match) //virtual
{
    //Remove ALL matching occurences of the value `match'
    bool tryRemoveHead=true;
    do {
        while(!mValueSize && mNext) { //Empty head => shift up an extra.
            ImmAttrMultiValue* tmp = mNext;
            
            mValue = tmp->mValue;
            tmp->mValue=NULL;
            
            mValueSize = tmp->mValueSize;
            tmp->mValueSize=0;
            
            mNext=tmp->mNext;
            tmp->mNext = NULL;
            delete tmp;
        }
        
        if(mValueSize && (mValueSize ==  match.size) &&
            (bcmp((const void *) mValue, (const void *) match.buf, 
                mValueSize) == 0)) {
            //match!
            delete [] mValue;
            mValue=0;
            mValueSize=0;
            //Head is now empty because it matched.
        } else {
            tryRemoveHead = false;
        }
        
    } while(tryRemoveHead);
    
    //We now have a head that does not match, possibly an empty head.
    //If head is empty then there can not be any tail.
    osafassert(mValueSize || !mNext); 
    
    if(mNext) {
        mNext->removeValue(match); //TODO: make non-recursive!

        if(!(mNext->mValueSize)) {
            ImmAttrMultiValue* tmp = mNext;
            mNext = tmp->mNext;
            tmp->mNext = NULL;
            delete tmp;
        }
    }
}

bool
ImmAttrMultiValue::hasMatchingValue(const IMMSV_OCTET_STRING& match) const//virtual
{
    if((mValueSize == match.size) &&
        bcmp((const void *) mValue, (const void *) match.buf, mValueSize) == 0) {
        return true;
    }
    
    //TODO: make non-recursive.
    return mNext?(mNext->hasMatchingValue(match)):false;
}

//Note ImmAttrMultiValue::setValue(const IMMSV_OCTET_STRING& in)  
//is inherited non virtual from ImmAttrValue

void
ImmAttrMultiValue::setExtraValue(const IMMSV_OCTET_STRING& in) 
{
    //NOTE: values need not be unique, this is a "bag".
    ImmAttrMultiValue* next = new ImmAttrMultiValue();
    next->setValue(in);
    //Push the new value on over old (stack model)  
    next->mNext = mNext;
    mNext = next;
}

void
ImmAttrMultiValue::setExtraValueC_str(const char* str) 
{
    //NOTE: values need not be unique, this is a "bag".
    ImmAttrMultiValue* next = new ImmAttrMultiValue();
    next->setValueC_str(str);
    //Push the new value on over old (stack model)  
    next->mNext = mNext;
    mNext = next;
}

bool
ImmAttrMultiValue::hasExtraValueC_str(const char* str) const
{
    const ImmAttrMultiValue* mval = this;
    while(mval) {
        if(strncmp(str, mval->mValue, mval->mValueSize)==0) {
            return true;
        }
        mval=mval->mNext;
    }
    return false;
}

bool
ImmAttrMultiValue::removeExtraValueC_str(const char* str)
{
    ImmAttrMultiValue* mval = this;
    while(mval->mNext) {
        if(strncmp(str, mval->mNext->mValue, mval->mNext->mValueSize)==0) {
            ImmAttrMultiValue* tmp = mval->mNext;
            mval->mNext=mval->mNext->mNext;
            tmp->mNext=NULL;
            delete tmp;
            return true;
        }
        mval=mval->mNext;
    }
    return false;
}

void
ImmAttrMultiValue::copyExtraValuesToEdu(IMMSV_EDU_ATTR_VAL_LIST** amvOut,
    SaImmValueTypeT t) const
{
    IMMSV_EDU_ATTR_VAL_LIST** prevp = amvOut;
    ImmAttrMultiValue* tmpNext = mNext;
    while(tmpNext) {
        
        *prevp = (IMMSV_EDU_ATTR_VAL_LIST*) 
            calloc(1, sizeof(IMMSV_EDU_ATTR_VAL_LIST));
        
        tmpNext->copyValueToEdu(&((*prevp)->n), t);
        (*prevp)->next = NULL;
        prevp = &((*prevp)->next);
        tmpNext = tmpNext->mNext;
    }
}


unsigned int
ImmAttrMultiValue::extraValues() const //virtual
{ 
    unsigned int extras = 0;
    ImmAttrMultiValue* mval = mNext;
    while(mval) {
        ++extras;
        mval=mval->mNext;
    }
    return extras;
}

bool
ImmAttrMultiValue::isMultiValued() const //virtual
{
    return true;
}

void
ImmAttrMultiValue::printMultiValue() const
{
    this->printSimpleValue();
    //printf("ImmAttrMultiValue::printMultiValue() mNext: %p\n", mNext);
    if(mNext) mNext->printMultiValue();
}

void
ImmAttrMultiValue::print() const
{
    this->printMultiValue();
}
