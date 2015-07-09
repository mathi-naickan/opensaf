############################################################################
#
# (C) Copyright 2014 The OpenSAF Foundation
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
# under the GNU Lesser General Public License Version 2.1, February 1999.
# The complete license can be accessed from the following location:
# http://opensource.org/licenses/lgpl-license.php
# See the Copying file included with the OpenSAF distribution for full
# licensing terms.
#
# Author(s): Ericsson
#
############################################################################
'''
Class representing an IMM CCB.
'''

import time
from ctypes import c_void_p, pointer, cast

from pyosaf.saAis import eSaAisErrorT, SaNameT, SaStringT, \
    SaDoubleT, SaTimeT, SaUint64T, SaInt64T, SaUint32T, SaInt32T, SaFloatT, \
    marshalNullArray

from pyosaf.saImm import eSaImmScopeT, eSaImmValueTypeT, SaImmAttrValuesT_2, \
    SaImmValueTypeMap

from pyosaf import saImm
from pyosaf import saImmOm
from pyosaf import saAis

import pyosaf.utils.immom
from pyosaf.utils.immom.common import SafException

TRYAGAIN_CNT = 60

def _value_to_ctype_ptr(value_type, value):
    ''' convert a value to a ctypes value ptr '''
    if value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAINT32T:
        ctypeptr = cast(pointer(SaInt32T(value)), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAUINT32T:
        ctypeptr = cast(pointer(SaUint32T(long(value))), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAINT64T:
        ctypeptr = cast(pointer(SaInt64T(long(value))), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAUINT64T:
        ctypeptr = cast(pointer(SaUint64T(long(value))), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SATIMET:
        ctypeptr = cast(pointer(SaTimeT(long(value))), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SANAMET:
        ctypeptr = cast(pointer(SaNameT(value)), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAFLOATT:
        ctypeptr = cast(pointer(SaFloatT(value)), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SADOUBLET:
        ctypeptr = cast(pointer(SaDoubleT(value)), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SASTRINGT:
        ctypeptr = cast(pointer(SaStringT(value)), c_void_p)
    elif value_type is eSaImmValueTypeT.SA_IMM_ATTR_SAANYT:
        assert 0
    else:
        assert 0

    return ctypeptr

def marshal_c_array(value_type, value_list):
    ''' creates a C array initialized with values from value_list '''
    c_array_type = saImm.SaImmAttrValueT * len(value_list)
    c_array = c_array_type()
    for i in range(len(value_list)):
        c_array[i] = _value_to_ctype_ptr(value_type, value_list[i])
    return c_array

class Ccb(object):
    def __init__(self):
        self.owner_handle = saImmOm.SaImmAdminOwnerHandleT()

        owner_name = saImmOm.SaImmAdminOwnerNameT("DummyName")
        one_sec_sleeps = 0
        err = saImmOm.saImmOmAdminOwnerInitialize(pyosaf.utils.immom.HANDLE, owner_name,
                saAis.eSaBoolT.SA_TRUE, self.owner_handle)
        while err == eSaAisErrorT.SA_AIS_ERR_TRY_AGAIN:
            if one_sec_sleeps == TRYAGAIN_CNT:
                break
            time.sleep(1)
            one_sec_sleeps += 1
            err = saImmOm.saImmOmAdminOwnerInitialize(pyosaf.utils.immom.HANDLE, owner_name,
                saAis.eSaBoolT.SA_TRUE, self.owner_handle)

        if err != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmAdminOwnerInitialize: %s" % eSaAisErrorT.whatis(err)
            raise SafException(err)

        self.ccb_handle = saImmOm.SaImmCcbHandleT()

        ccb_flags = saImmOm.SaImmCcbFlagsT(
            saImm.saImm.SA_IMM_CCB_REGISTERED_OI)
        one_sec_sleeps = 0
        err = saImmOm.saImmOmCcbInitialize(self.owner_handle, ccb_flags,
                                           self.ccb_handle)
        while err == eSaAisErrorT.SA_AIS_ERR_TRY_AGAIN:
            if one_sec_sleeps == TRYAGAIN_CNT:
                break
            time.sleep(1)
            one_sec_sleeps += 1
            err = saImmOm.saImmOmCcbInitialize(self.owner_handle, ccb_flags,
                                               self.ccb_handle)

        if err != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmCcbInitialize: %s" % eSaAisErrorT.whatis(err)
            raise SafException(err)

    def __del__(self):
        one_sec_sleeps = 0
        error = saImmOm.saImmOmAdminOwnerFinalize(self.owner_handle)
        while error == eSaAisErrorT.SA_AIS_ERR_TRY_AGAIN:
            if one_sec_sleeps == TRYAGAIN_CNT:
                break
            time.sleep(1)
            one_sec_sleeps += 1
            error = saImmOm.saImmOmAdminOwnerFinalize(self.owner_handle)

        if error != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmAdminOwnerFinalize: %s" % eSaAisErrorT.whatis(error)
            raise SafException(error)

    def create(self, obj, _parent_name=None):
        ''' add to the CCB the object 'obj' '''
        if _parent_name is not None:
            parent_name = SaNameT(_parent_name)
            object_names = [parent_name]
            err = saImmOm.saImmOmAdminOwnerSet(self.owner_handle, object_names,
                eSaImmScopeT.SA_IMM_SUBTREE)
            if err != eSaAisErrorT.SA_AIS_OK:
                msg = "saImmOmAdminOwnerSet: %s - obj:%s" % \
                    (_parent_name, eSaAisErrorT.whatis(err))
                raise SafException(err, msg)
        else:
            parent_name = None

        attr_values = []
        for attr_name, type_values in obj.attrs.iteritems():
            values = type_values[1]
            attr = SaImmAttrValuesT_2()
            attr.attrName = attr_name
            attr.attrValueType = type_values[0]
            attr.attrValuesNumber = len(values)
            attr.attrValues = marshal_c_array(attr.attrValueType, values)
            attr_values.append(attr)

        err = saImmOm.saImmOmCcbObjectCreate_2(self.ccb_handle,
                                               obj.class_name,
                                               parent_name,
                                               attr_values)
        if err != eSaAisErrorT.SA_AIS_OK:
            msg = "saImmOmCcbObjectCreate_2: %s, parent:%s, class:%s" % (
                eSaAisErrorT.whatis(err), _parent_name, obj.class_name)
            raise SafException(err, msg)

    def delete(self, _object_name):
        if _object_name is None:
            raise SafException(eSaAisErrorT.SA_AIS_ERR_NOT_EXIST)

        object_name = SaNameT(_object_name)
        object_names = [object_name]

        err = saImmOm.saImmOmAdminOwnerSet(self.owner_handle, object_names,
            eSaImmScopeT.SA_IMM_SUBTREE)
        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err, "saImmOmAdminOwnerSet: %s - %s" % \
                (_object_name, eSaAisErrorT.whatis(err)))

        err = saImmOm.saImmOmCcbObjectDelete(self.ccb_handle, object_name)
        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err, "saImmOmCcbObjectDelete: %s - %s" % \
                (_object_name, eSaAisErrorT.whatis(err)))

    def modify_value_add(self, object_name, attr_name, values):
        ''' add to the CCB an ADD modification of an existing object '''

        assert object_name

        # first get class name to read class description to get value type...
        try:
            obj = pyosaf.utils.immom.get(object_name)
        except SafException as err:
            print "failed: %s" % err
            return

        object_names = [SaNameT(object_name)]
        class_name = obj.SaImmAttrClassName
        value_type = None
        attr_def_list = pyosaf.utils.immom.class_description_get(class_name)
        for attr_def in attr_def_list:
            if attr_def.attrName == attr_name:
                value_type = attr_def.attrValueType
                break

        if value_type is None:
            # means attribute name is invalid
            raise SafException(eSaAisErrorT.SA_AIS_ERR_NOT_EXIST,
                "attribute '%s' does not exist" % attr_name)

        err = saImmOm.saImmOmAdminOwnerSet(self.owner_handle, object_names,
            eSaImmScopeT.SA_IMM_ONE)
        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err, "saImmOmAdminOwnerSet: %s - %s" % \
                (object_name, eSaAisErrorT.whatis(err)))

        attr_mods = []

        attr_mod = saImmOm.SaImmAttrModificationT_2()
        attr_mod.modType = \
            saImm.eSaImmAttrModificationTypeT.SA_IMM_ATTR_VALUES_ADD
        attr_mod.modAttr = SaImmAttrValuesT_2()
        attr_mod.modAttr.attrName = attr_name
        attr_mod.modAttr.attrValueType = value_type
        attr_mod.modAttr.attrValuesNumber = len(values)
        attr_mod.modAttr.attrValues = marshal_c_array(value_type, values)

        attr_mods.append(attr_mod)

        err = saImmOm.saImmOmCcbObjectModify_2(self.ccb_handle,
            object_names[0], attr_mods)
        if err != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmCcbObjectModify_2: %s, %s" % (eSaAisErrorT.whatis(err), object_name)
            raise SafException(err)

    def modify_value_replace(self, object_name, attr_name, values):
        ''' add to the CCB an REPLACE modification of an existing object '''

        assert object_name

        # first get class name to read class description to get value type...
        try:
            obj = pyosaf.utils.immom.get(object_name)
        except SafException as err:
            print "failed: %s" % err
            return

        object_names = [SaNameT(object_name)]
        class_name = obj.SaImmAttrClassName
        value_type = None
        attr_def_list = pyosaf.utils.immom.class_description_get(class_name)
        for attr_def in attr_def_list:
            if attr_def.attrName == attr_name:
                value_type = attr_def.attrValueType
                break

        if value_type is None:
            # means attribute name is invalid
            raise SafException(eSaAisErrorT.SA_AIS_ERR_NOT_EXIST,
                "attribute '%s' does not exist" % attr_name)

        err = saImmOm.saImmOmAdminOwnerSet(self.owner_handle, object_names,
            eSaImmScopeT.SA_IMM_ONE)
        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err, "saImmOmAdminOwnerSet: %s - %s" % \
                (object_name, eSaAisErrorT.whatis(err)))

        attr_mods = []
        attr_mod = saImmOm.SaImmAttrModificationT_2()
        attr_mod.modType = \
            saImm.eSaImmAttrModificationTypeT.SA_IMM_ATTR_VALUES_REPLACE
        attr_mod.modAttr = SaImmAttrValuesT_2()
        attr_mod.modAttr.attrName = attr_name
        attr_mod.modAttr.attrValueType = value_type
        attr_mod.modAttr.attrValuesNumber = len(values)
        attr_mod.modAttr.attrValues = marshal_c_array(value_type, values)

        attr_mods.append(attr_mod)

        err = saImmOm.saImmOmCcbObjectModify_2(self.ccb_handle,
            object_names[0], attr_mods)
        if err != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmCcbObjectModify_2: %s, %s" % (eSaAisErrorT.whatis(err), object_name)
            raise SafException(err)

    def modify_value_delete(self, object_name, attr_name, values):
        ''' add to the CCB an DELETE modification of an existing object '''

        assert object_name

        # first get class name to read class description to get value type...
        try:
            obj = pyosaf.utils.immom.get(object_name)
        except SafException as err:
            print "failed: %s" % err
            return

        object_names = [SaNameT(object_name)]
        class_name = obj.SaImmAttrClassName
        value_type = None
        attr_def_list = pyosaf.utils.immom.class_description_get(class_name)
        for attr_def in attr_def_list:
            if attr_def.attrName == attr_name:
                value_type = attr_def.attrValueType
                break

        if value_type is None:
            # means attribute name is invalid
            raise SafException(eSaAisErrorT.SA_AIS_ERR_NOT_EXIST,
                "attribute '%s' does not exist" % attr_name)

        err = saImmOm.saImmOmAdminOwnerSet(self.owner_handle, object_names,
            eSaImmScopeT.SA_IMM_ONE)
        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err, "saImmOmAdminOwnerSet: %s - %s" % \
                (object_name, eSaAisErrorT.whatis(err)))

        attr_mods = []
        attr_mod = saImmOm.SaImmAttrModificationT_2()
        attr_mod.modType = \
            saImm.eSaImmAttrModificationTypeT.SA_IMM_ATTR_VALUES_DELETE
        attr_mod.modAttr = SaImmAttrValuesT_2()
        attr_mod.modAttr.attrName = attr_name
        attr_mod.modAttr.attrValueType = value_type
        attr_mod.modAttr.attrValuesNumber = len(values)
        attr_mod.modAttr.attrValues = marshal_c_array(value_type, values)

        attr_mods.append(attr_mod)

        err = saImmOm.saImmOmCcbObjectModify_2(self.ccb_handle,
            object_names[0], attr_mods)
        if err != eSaAisErrorT.SA_AIS_OK:
            print "saImmOmCcbObjectModify_2: %s, %s" % (eSaAisErrorT.whatis(err), object_name)
            raise SafException(err)

    def apply(self):
        ''' Apply the CCB '''

        one_sec_sleeps = 0
        err = saImmOm.saImmOmCcbApply(self.ccb_handle)
        while err == eSaAisErrorT.SA_AIS_ERR_TRY_AGAIN:
            if one_sec_sleeps == TRYAGAIN_CNT:
                break
            time.sleep(1)
            one_sec_sleeps += 1
            err = saImmOm.saImmOmCcbApply(self.ccb_handle)

        if err != eSaAisErrorT.SA_AIS_OK:
            raise SafException(err)

def test():
    ccb = Ccb()
    ccb.modify("safAmfCluster=myAmfCluster", "saAmfClusterStartupTimeout",
               10000000000)
    ccb.apply()
    del ccb

if __name__ == '__main__':
    test()
