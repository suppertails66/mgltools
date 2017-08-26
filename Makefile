IDIR := src
SRCDIR := src
ODIR := obj
LDIR :=

AR=ar
CXX=g++
CDEFINES = -DBLACKT_ENABLE_LIBPNG
CLIBS = -L./blackt -lblackt -lpng
CFLAGS = -std=gnu++11 -O2 -Wall
CINCLUDES = -I./blackt/src
CXXFLAGS=$(CFLAGS) $(CDEFINES) $(CINCLUDES) -I$(IDIR) $(CLIBS)

SRC := $(wildcard $(SRCDIR)/*/*.cpp)
OBJ := $(patsubst $(SRCDIR)/%,$(ODIR)/%,$(patsubst %.cpp,%.o,$(SRC)))
DEP := $(patsubst %.o,%.d,$(OBJ))
LIB := libcopycat.a

PREFIX := /usr/local
BINDIR := $(PREFIX)/bin
INSTALL := install

tools = sjis_srch mgl_strtab_extr mgl_str_insr mgl_str_fmtconv mgl_img_decmp mgl_img_cmp mgl_img_inject mgl_grp_conv mgl_img_extr mgl_img_insr mgl_fieldbod_extr mgl_script_extr mgl_dlog_insr mgl_credits_extr mgl_credits_insr mgl_ascii_extr mgl_ascii_build mgl_tileimg_extr mgl_titleimg_extr

all: blackt $(OBJ) $(tools)
	
sjis_srch: blackt $(OBJ)
	$(CXX) $(OBJ) src/sjis_srch.cpp -o sjis_srch $(CXXFLAGS)
	
mgl_strtab_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_transtxt.cpp src/mgl_strtab_extr.cpp -o mgl_strtab_extr $(CXXFLAGS)
	
mgl_str_insr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_transtxt.cpp src/mgl_str_insr.cpp -o mgl_str_insr $(CXXFLAGS)
	
mgl_str_fmtconv: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_transtxt.cpp src/mgl_str_fmtconv.cpp -o mgl_str_fmtconv $(CXXFLAGS)
	
mgl_img_decmp: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_img_decmp.cpp -o mgl_img_decmp $(CXXFLAGS)
	
mgl_img_cmp: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_img_cmp.cpp -o mgl_img_cmp $(CXXFLAGS)
	
mgl_img_inject: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_img_inject.cpp -o mgl_img_inject $(CXXFLAGS)
	
mgl_grp_conv: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_grp_conv.cpp -o mgl_grp_conv $(CXXFLAGS)
	
mgl_img_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_img_extr.cpp -o mgl_img_extr $(CXXFLAGS)
	
mgl_img_insr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_img_insr.cpp -o mgl_img_insr $(CXXFLAGS)
	
mgl_fieldbod_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_fieldbod_extr.cpp -o mgl_fieldbod_extr $(CXXFLAGS)
	
mgl_script_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_script_extr.cpp -o mgl_script_extr $(CXXFLAGS)
	
mgl_dlog_insr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_dlog_insr.cpp -o mgl_dlog_insr $(CXXFLAGS)
	
mgl_credits_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_credits_extr.cpp -o mgl_credits_extr $(CXXFLAGS)
	
mgl_credits_insr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_credits_insr.cpp -o mgl_credits_insr $(CXXFLAGS)
	
mgl_ascii_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_ascii_extr.cpp -o mgl_ascii_extr $(CXXFLAGS)
	
mgl_ascii_build: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_ascii_build.cpp -o mgl_ascii_build $(CXXFLAGS)
	
mgl_tileimg_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_tileimg_extr.cpp -o mgl_tileimg_extr $(CXXFLAGS)
	
mgl_titleimg_extr: blackt $(OBJ)
	$(CXX) $(OBJ) src/mgl_titleimg_extr.cpp -o mgl_titleimg_extr $(CXXFLAGS)

-include $(DEP)

$(ODIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -MMD -MP -MF $(@:.o=.d) -o $@ $< $(CXXFLAGS)

blackt: blackt/libblackt.a

blackt/libblackt.a:
	cd ./blackt && $(MAKE) && cd $(CURDIR)

.PHONY: clean cleanme install

# Clean wdtools
cleanme:
	rm -f $(tools)
	rm -rf $(ODIR)
	rm -f grp

# Clean wdtools and libraries
clean: cleanme
	cd ./blackt && $(MAKE) clean && cd $(CURDIR)

install: all
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) $(tools) $(BINDIR)
