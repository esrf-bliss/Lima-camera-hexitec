############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2013
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
############################################################################
include ../hexitec.inc

hexitec-objs = HexitecCamera.o \
	HexitecInterface.o \
	HexitecDetInfoCtrlObj.o \
	HexitecSyncCtrlObj.o \
	HexitecSavingCtrlObj.o \
	HexitecProcessingTask.o \
	HexitecSavingTask.o \

SRCS = $(hexitec-objs:.o=.cpp) 

CXXFLAGS += -I../include \
			-I../sdk/include \
			-I../../../hardware/include \
			-I../../../common/include \
			-I../../../control/include \
			-I../../../control/software_operation/include \
			-I../../../third-party/Processlib/core/include \
			-I$(PLEORA_ROOT)/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/include \
			-fPIC -g -Wno-unknown-pragmas -std=c++11

ifneq ($(COMPILE_HDF5_SAVING),0)
CXXFLAGS += -I../../../third-party/hdf5/src
CXXFLAGS += -I../../../third-party/hdf5/c++/src
CXXFLAGS += -DWITH_HDF5_SAVING
endif

all:	Hexitec.o

Hexitec.o:	$(hexitec-objs)
	$(LD) -o $@ -r $+ ../sdk/src/HexitecAPI.o ../sdk/tools/src/libINIReader.o

clean:
	rm -f *.o *.P

%.o : %.cpp
	$(COMPILE.cpp) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(SRCS:.cpp=.P)

.PHONY: check-syntax
check-syntax:
	$(CXX) -Wall -Wextra -fsyntax-only $(CXXFLAGS) $(CHK_SOURCES)
