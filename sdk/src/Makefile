hexitecApi-objs = HexitecApi.o GigE.o

SRCS = $(hexitecApi-objs:.o=.cpp) 

CXXFLAGS += -I../include \
			-I../tools/src \
			-I/opt/pleora/ebus_sdk/CentOS-RHEL-7-x86_64/include \
			-fPIC -g -std=c++11

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
