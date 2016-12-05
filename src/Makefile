hexitec-objs = HexitecCamera.o \
	HexitecInterface.o \
	HexitecDetInfoCtrlObj.o \
	HexitecSyncCtrlObj.o \
	HexitecSavingCtrlObj.o \
#	HexitecProcessingTask.o 

SRCS = $(hexitec-objs:.o=.cpp) 

CXXFLAGS += -I../include \
			-I../../../hardware/include \
			-I../../../common/include \
			-I../../../control/include \
			-I../../../control/software_operation/include \
			-I../sdk/include \
			-I../sdk/GigE \
			-I../../../third-party/Processlib/core/include \
			-I/opt/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/include \
			-fPIC -g -Wno-unknown-pragmas -std=c++11

#ifneq ($(COMPILE_HDF5_SAVING),0)
CXXFLAGS += -I../../../third-party/hdf5/src
CXXFLAGS += -I../../../third-party/hdf5/c++/src
CXXFLAGS += -DWITH_HDF5_SAVING
#endif

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
