IDIR := src
SRCDIR := src
ODIR := obj
LDIR :=

BLACKTDIR = blackt
LIBSATDIR = libsat

AR=ar
CXX=g++
# Compile only
CDEFINES = -DBLACKT_ENABLE_LIBPNG
#CLIBS = -lpng
CFLAGS = -std=gnu++11 -O2 -Wall -L${LIBSATDIR} -lsat -L${BLACKTDIR} -lblackt -lpng
CINCLUDES = -I${BLACKTDIR}/src -I${LIBSATDIR}/src
CXXFLAGS=$(CFLAGS) $(CDEFINES) $(CINCLUDES) -I$(IDIR)

LIBDEPS := $(LIBSATDIR)/libsat.a $(BLACKTDIR)/libblackt.a

TOOLSRCS := $(wildcard $(SRCDIR)/*.cpp)
TOOLSINDIR := $(patsubst %.cpp,%,$(TOOLSRCS))
TOOLS := $(notdir $(TOOLSINDIR))

.SECONDEXPANSION:

all: $(BLACKTDIR)/libblackt.a $(LIBSATDIR)/libsat.a $(TOOLS)

$(BLACKTDIR)/libblackt.a: $(BLACKTDIR)/src/**/*.cpp
	make -C ${BLACKTDIR} all

$(LIBSATDIR)/libsat.a: $(LIBSATDIR)/src/**/*.cpp
	make -C ${LIBSATDIR} all

$(TOOLS): $(SRCDIR)/$$@.cpp $(LIBDEPS) $(BLACKTDIR)/libblackt.a
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $(notdir $@) $(CXXFLAGS)

.PHONY: cleanme clean

cleanme:
	rm -f $(TOOLS)

clean: cleanme
#	rm -f $(LIB)
#	rm -rf $(ODIR)
	make -C ${BLACKTDIR} clean
	make -C ${LIBSATDIR} clean
