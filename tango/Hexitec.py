# ###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2017
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
# ###########################################################################
# #############################################################################
# license :
# =============================================================================
#
# file :        Hexitec.py
#
# description : Python source for the Hexitec and its commands.
#                The class is derived from Device. It represents the
#                CORBA servant object which will be accessed from the
#                network. All commands which can be executed on the
#                Hexitec are implemented in this file.
#
# project :     TANGO Device Server
#
# copyleft :    European Synchrotron Radiation Facility
#               BP 220, Grenoble 38043
#               FRANCE
#
# =============================================================================
# (c) - Bliss - ESRF
# =============================================================================
#
import PyTango
import AttrHelper
from Lima import Core
from AttrHelper import get_attr_string_value_list
from Lima import Hexitec as HexitecAcq


class Hexitec(PyTango.Device_4Impl):

    Core.DEB_CLASS(Core.DebModApplication, 'LimaCCDs')

    def __init__(self, cl, name):
        PyTango.Device_4Impl.__init__(self, cl, name)

        self.__ProcessType = {'RAW': HexitecAcq.Camera.RAW,
                              'SORT': HexitecAcq.Camera.SORT,
                              'CSA': HexitecAcq.Camera.CSA,
                              'CSD': HexitecAcq.Camera.CSD,
                              'CSA_NF': HexitecAcq.Camera.CSA_NF,
                              'CSD_NF': HexitecAcq.Camera.CSD_NF}

        self.__SaveOpt = {'SaveRaw': 1,
                          'SaveProcessed': 2,
                          'SaveHistogram': 4,
                          'SaveSummed': 8,
                          }

        self.init_device()

    def init_device(self):
        self.set_state(PyTango.DevState.ON)
        self.get_device_properties(self.get_device_class())

# ------------------------------------------------------------------
#    getAttrStringValueList command:
#
#    Description: return a list of authorized values if any
#    argout: DevVarStringArray
# ------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return get_attr_string_value_list(self, attr_name)

# ==================================================================
#
#    Hexitec read/write attribute methods
#
# ==================================================================
    @Core.DEB_MEMBER_FUNCT
    def read_environmentValues(self, attr):
        returnList = []
        ev = _HexitecCamera.getEnvironmentalValues()
        returnList.append(ev.humidity)
        returnList.append(ev.ambientTemperature)
        returnList.append(ev.asicTemperature)
        returnList.append(ev.adcTemperature)
        returnList.append(ev.ntcTemperature)
        attr.set_value(returnList)

    @Core.DEB_MEMBER_FUNCT
    def read_operatingValues(self, attr):
        returnList = []
        ov = _HexitecCamera.getOperatingValues()
        returnList.append(ov.v3_3)
        returnList.append(ov.hvMon)
        returnList.append(ov.hvOut)
        returnList.append(ov.v1_2)
        returnList.append(ov.v1_8)
        returnList.append(ov.v3)
        returnList.append(ov.v2_5)
        returnList.append(ov.v3_3ln)
        returnList.append(ov.v1_65ln)
        returnList.append(ov.v1_8ana)
        returnList.append(ov.v3_8ana)
        returnList.append(ov.peltierCurrent)
        returnList.append(ov.ntcTemperature)
        attr.set_value(returnList)

    @Core.DEB_MEMBER_FUNCT
    def read_collectDcTimeout(self, attr):
        attr.set_value(_HexitecCamera.getCollectDcTimeout())

    @Core.DEB_MEMBER_FUNCT
    def write_collectDcTimeout(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setCollectDcTimeout(data)

    @Core.DEB_MEMBER_FUNCT
    def read_frameTimeout(self, attr):
        attr.set_value(_HexitecCamera.getFrameTimeout())

    @Core.DEB_MEMBER_FUNCT
    def write_frameTimeout(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setFrameTimeout(data)

    @Core.DEB_MEMBER_FUNCT
    def read_type(self, attr):
        type = _HexitecCamera.getType()
        attr.set_value(AttrHelper._getDictKey(self.__ProcessType, type))

    @Core.DEB_MEMBER_FUNCT
    def write_type(self, attr):
        data = attr.get_write_value()
        type = AttrHelper._getDictValue(self.__ProcessType, data)
        _HexitecCamera.setType(type)

    @Core.DEB_MEMBER_FUNCT
    def read_binWidth(self, attr):
        attr.set_value(_HexitecCamera.getBinWidth())

    @Core.DEB_MEMBER_FUNCT
    def write_binWidth(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setBinWidth(data)

    @Core.DEB_MEMBER_FUNCT
    def read_specLen(self, attr):
        attr.set_value(_HexitecCamera.getSpecLen())

    @Core.DEB_MEMBER_FUNCT
    def write_specLen(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setSpecLen(data)

    @Core.DEB_MEMBER_FUNCT
    def read_lowThreshold(self, attr):
        attr.set_value(_HexitecCamera.getLowThreshold())

    @Core.DEB_MEMBER_FUNCT
    def write_lowThreshold(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setLowThreshold(data)

    @Core.DEB_MEMBER_FUNCT
    def read_highThreshold(self, attr):
        attr.set_value(_HexitecCamera.getHighThreshold())

    @Core.DEB_MEMBER_FUNCT
    def write_highThreshold(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setHighThreshold(data)

    @Core.DEB_MEMBER_FUNCT
    def read_frameRate(self, attr):
        attr.set_value(_HexitecCamera.getFrameRate())

    @Core.DEB_MEMBER_FUNCT
    def read_saveOpt(self, attr):
        attr.set_value(_HexitecCamera.getSaveOpt())

    @Core.DEB_MEMBER_FUNCT
    def write_saveOpt(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setSaveOpt(data)

    @Core.DEB_MEMBER_FUNCT
    def read_biasVoltageRefreshInterval(self, attr):
        attr.set_value(_HexitecCamera.getBiasVoltageRefreshInterval())

    @Core.DEB_MEMBER_FUNCT
    def write_biasVoltageRefreshInterval(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setBiasVoltageRefreshInterval(data)

    @Core.DEB_MEMBER_FUNCT
    def read_biasVoltageRefreshTime(self, attr):
        attr.set_value(_HexitecCamera.getBiasVoltageRefreshTime())

    @Core.DEB_MEMBER_FUNCT
    def write_biasVoltageRefreshTime(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setBiasVoltageRefreshTime(data)

    @Core.DEB_MEMBER_FUNCT
    def read_biasVoltageSettleTime(self, attr):
        attr.set_value(_HexitecCamera.getBiasVoltageSettleTime())

    @Core.DEB_MEMBER_FUNCT
    def write_biasVoltageSettleTime(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setBiasVoltageSettleTime(data)

    @Core.DEB_MEMBER_FUNCT
    def read_biasVoltage(self, attr):
        attr.set_value(_HexitecCamera.getBiasVoltage())

    @Core.DEB_MEMBER_FUNCT
    def write_biasVoltage(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setBiasVoltage(data)

    @Core.DEB_MEMBER_FUNCT
    def read_refreshVoltage(self, attr):
        attr.set_value(_HexitecCamera.getRefreshVoltage())

    @Core.DEB_MEMBER_FUNCT
    def write_refreshVoltage(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setRefreshVoltage(data)

    @Core.DEB_MEMBER_FUNCT
    def read_framesPerTrigger(self, attr):
        attr.set_value(_HexitecCamera.getframesPerTrigger())

    @Core.DEB_MEMBER_FUNCT
    def write_framesPerTrigger(self, attr):
        data = attr.get_write_value()
        _HexitecCamera.setFramesPerTrigger(data)

    @Core.DEB_MEMBER_FUNCT
    def read_skippedFrameCount(self, attr):
        attr.set_value(_HexitecCamera.getSkippedFrameCount())
        
# ==================================================================
#
#    Hexitec command methods
#
# ==================================================================
    @Core.DEB_MEMBER_FUNCT
    def CollectOffsetValues(self):
        _HexitecCamera.collectOffsetValues()

    @Core.DEB_MEMBER_FUNCT
    def HvBiasOn(self):
        _HexitecCamera.setHvBiasOn()

    @Core.DEB_MEMBER_FUNCT
    def HvBiasOff(self):
        _HexitecCamera.setBiasOff()


# ==================================================================
#
#    HexitecClass class definition
#
# ==================================================================
class HexitecClass(PyTango.DeviceClass):

    #    Class Properties
    class_property_list = {
        }

    #    Device Properties
    device_property_list = {
        'IPaddress':
            [PyTango.DevString,
             "Hexitec IP address (e.g. 192.168.0.1)", []],
        'configFilename':
            [PyTango.DevString,
             "Hexitec configuration filename", []],
        'bufferCount':
            [PyTango.DevLong,
             "Number of buffer available for Hexitec", []],
        'timeout':
            [PyTango.DevLong,
             "Timeout on Hexitec operations (milliseconds)", []],
        'asicPitch':
            [PyTango.DevLong,
             "Hexitec asic pitch", []],
        }

    #    Command definitions
    cmd_list = {
        'getAttrStringValueList':
            [[PyTango.DevString, "Attribute name"],
             [PyTango.DevVarStringArray, "Authorized String value list"]],
        'CollectOffsetValues':
            [[PyTango.DevVoid, "none"],
             [PyTango.DevVoid, "none"]],
        'HvBiasOn':
            [[PyTango.DevVoid, "none"],
             [PyTango.DevVoid, "none"]],
        'HvBiasOff':
            [[PyTango.DevVoid, "none"],
             [PyTango.DevVoid, "none"]],
        }

    #    Attribute definitions
    attr_list = {
        'environmentValues':
            [[PyTango.DevDouble,
              PyTango.SPECTRUM,
              PyTango.READ_WRITE, 5]],
        'operatingValues':
            [[PyTango.DevDouble,
              PyTango.SPECTRUM,
              PyTango.READ_WRITE, 13]],
        'collectDcTimeout':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'frameTimeout':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'type':
            [[PyTango.DevString,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'binWidth':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'specLen':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'lowThreshold':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'highThreshold':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'frameRate':
            [[PyTango.DevDouble,
              PyTango.SCALAR,
              PyTango.READ]],
        'saveOpt':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'biasVoltageRefreshInterval':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'biasVoltageRefreshTime':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'biasVoltageSettleTime':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'biasVoltage':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'refreshVoltage':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'framePerTrigger':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ_WRITE]],
        'skippedFrameCount':
            [[PyTango.DevLong,
              PyTango.SCALAR,
              PyTango.READ]],
        }

    def __init__(self, name):
        PyTango.DeviceClass.__init__(self, name)
        self.set_type(name)


_HexitecInterface = None
_HexitecCamera = None


def get_control(IPaddress="0", configFilename="0", bufferCount=50, timeout=600, asicPitch=250, **keys):
    global _HexitecInterface
    global _HexitecCamera
#    Core.DebParams.setTypeFlags(Core.DebParams.AllFlags)
    if _HexitecInterface is None:
        print "IPaddress ", IPaddress
        print "full path config file ", configFilename
        _HexitecCamera = HexitecAcq.Camera(IPaddress, configFilename, int(bufferCount),
                                           int(timeout), int(asicPitch))
        _HexitecInterface = HexitecAcq.Interface(_HexitecCamera)
    ct = Core.CtControl(_HexitecInterface)
    print "Core.Control done"
    return ct


def get_tango_specific_class_n_device():
    return HexitecClass, Hexitec
