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
include ../../../config.inc
include ../hexitec.inc

SRCS = test2.cpp test4.cpp

ifneq ($(HEXITEC_DUMMY),0)
LDFLAGS = -pthread -L../../../build  -L../../../third-party/Processlib/build -L/usr/lib64 
LDLIBS = -llimacore  -lprocesslib
else
LDFLAGS = -pthread -L../../../build  -L../../../third-party/Processlib/build -L../sdk/tools/src -L$(PLEORA_ROOT)/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/lib -L/usr/lib64 
LDLIBS = -llimacore  -lprocesslib -lPvDevice -lPvStream -lPvBuffer -lPvGenICam -lPvBase -lPvSerial -lPvSystem -lEbUtilsLib -ludev
endif

CXXFLAGS = -I../include \
	       -I../../../hardware/include \
	       -I../../../common/include \
           -I../../../control/include \
           -I../../../control/software_operation/include \
	       -I../../../third-party/Processlib/core/include \
	       -I../../../third-party/Processlib/tasks/include \
	       -I../../../third-party/Sps/Include \
	       -I../../../third-party/hdf5/src \
	       -I../../../third-party/hdf5/c++/src \
	       -I../sdk/include \
	       -I$(PLEORA_ROOT)/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/include \
	       -std=c++11 -Wall -Wno-unknown-pragmas -pthread -g

ifneq ($(HEXITEC_DUMMY),0)
CXXFLAGS += -DCOMPILE_HEXITEC_DUMMY
endif

ifneq ($(COMPILE_CONFIG),0)
CXXFLAGS += -DWITH_CONFIG -I../../third-party/libconfig/lib
LDLIBS += -L../../../third-party/libconfig/lib/.libs -lconfig
endif

ifneq ($(COMPILE_SPS_IMAGE),0)
CXXFLAGS += -DWITH_SPS_IMAGE -I../../third-party/libconfig/lib
LDLIBS += -L../../../third-party/sps/lib/.libs -lconfig
endif

HDF5_LDFLAGS := -L../../../third-party/hdf5/c++/src/.libs -L../../../third-party/hdf5/src/.libs -L../../../install/Lima/lib
HDF5_LDLIBS := -lhdf5_cpp -lhdf5

test-progs = test4

all: 	$(test-progs)

#test2:		test2.o ../src/Hexitec.o
#	$(CXX) $(LDFLAGS) -o $@ $+  $(HDF5_LDFLAGS) $(HDF5_LDLIBS) $(LDLIBS)

test4:		test4.o ../src/Hexitec.o
	$(CXX) $(LDFLAGS) -o $@ $+  $(HDF5_LDFLAGS) $(HDF5_LDLIBS) $(LDLIBS)

clean:
	rm -f *.o *.P test2 test4

%.o : %.cpp
	$(COMPILE.cpp) -MD $(CXXFLAGS) -o $@ $<
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d

-include $(SRCS:.cpp=.P)

