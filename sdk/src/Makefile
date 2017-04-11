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
include ../../hexitec.inc

ifneq ($(HEXITEC_DUMMY),0)
hexitecApi-objs = HexitecDummy.o
else
hexitecApi-objs = HexitecApi.o GigE.o
endif

SRCS = $(hexitecApi-objs:.o=.cpp) 

CXXFLAGS += -I../include \
			-I../tools/src \
			-I$(PLEORA_ROOT)/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/include \
			-fPIC -g -std=c++11

ifneq ($(HEXITEC_DUMMY),0)
CXXFLAGS += -DCOMPILE_HEXITEC_DUMMY
endif

all:	HexitecAPI.o

HexitecAPI.o:	$(hexitecApi-objs)
	$(LD) -o $@ -r $+

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
