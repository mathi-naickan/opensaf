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

/*
  ImmAttrValue.hh  should be immnd_attr_value.hh ??
  Only included by the IMMND.
*/

#ifndef IMM_IMMND_IMMATTRVALUE_H_
#define IMM_IMMND_IMMATTRVALUE_H_ 1

#include "imm/immsv_evt_model.h"


/**
 *
 */
class ImmAttrValue
{
public:
    ImmAttrValue();
    ImmAttrValue(const ImmAttrValue& b);
    virtual         ~ImmAttrValue();
    virtual bool     isMultiValued() const;
    virtual unsigned int extraValues() const;
    virtual void     discardValues();
    virtual void     print() const;
    virtual void     removeValue(const IMMSV_OCTET_STRING& match);
    virtual bool     hasMatchingValue(const IMMSV_OCTET_STRING& match) const;
    virtual bool     hasDuplicates() const;

    void printSimpleValue() const;

    ImmAttrValue&   operator= (const ImmAttrValue& b);

    void            setValue_int(int i);
    int             getValue_int() const;
    void            setValueC_str(const char* str);
    const char*     getValueC_str() const;
    void            setValue(const IMMSV_OCTET_STRING& in);
    void            copyValueToEdu(IMMSV_EDU_ATTR_VAL* out, 
        SaImmValueTypeT t) const;
    bool            empty() const {return !mValueSize;}
    
protected:
    char*           mValue;
    unsigned int    mValueSize;
    
};

class ImmAttrMultiValue: public ImmAttrValue
{
public:
    ImmAttrMultiValue();
    ImmAttrMultiValue(const ImmAttrMultiValue& b);
    ImmAttrMultiValue(const ImmAttrValue& b);
    virtual         ~ImmAttrMultiValue();
    virtual bool     isMultiValued() const;
    virtual unsigned int extraValues() const;
    virtual void     discardValues();
    virtual void     print() const;
    virtual void     removeValue(const IMMSV_OCTET_STRING& match);
    virtual bool     hasMatchingValue(const IMMSV_OCTET_STRING& match) const;
    virtual bool     hasDuplicates() const;
    
    void printMultiValue() const;
    
    ImmAttrMultiValue&   operator= (const ImmAttrMultiValue& b);
    
    void            setExtraValue(const IMMSV_OCTET_STRING& in);
    void            setExtraValueC_str(const char* str);
    void            copyExtraValuesToEdu(IMMSV_EDU_ATTR_VAL_LIST** out,
        SaImmValueTypeT t) const;
    bool            hasExtraValueC_str(const char* str) const;
    bool            removeExtraValueC_str(const char* str);
    
    ImmAttrMultiValue *getNextAttrValue() const {return mNext;};

private:
    
    ImmAttrMultiValue* mNext;  //Singly linked list
};



#endif  // IMM_IMMND_IMMATTRVALUE_H_
