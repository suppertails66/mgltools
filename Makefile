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

SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(patsubst $(SRCDIR)/%,$(ODIR)/%,$(patsubst %.cpp,%.o,$(SRC)))
DEP := $(patsubst %.o,%.d,$(OBJ))
#LIB := libcopycat.a

PREFIX := /usr/local
BINDIR := $(PREFIX)/bin
INSTALL := install

tools = sjis_srch mgl_strtab_extr mgl_str_insr mgl_str_fmtconv mgl_img_decmp mgl_img_cmp mgl_img_inject mgl_grp_conv mgl_img_extr mgl_img_insr mgl_fieldbod_extr mgl_script_extr mgl_dlog_insr mgl_credits_extr mgl_credits_insr mgl_ascii_extr mgl_ascii_build mgl_tileimg_extr mgl_titleimg_extr mgl_colorize

all: blackt $(tools)

-include $(DEP)

$(ODIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -MMD -MP -MF $(@:.o=.d) -o $@ $< $(CXXFLAGS)
	
sjis_srch: blackt $(OBJ)
	$(CXX) $(ODIR)/sjis_srch.o -o sjis_srch $(CXXFLAGS)
	
mgl_strtab_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/csv_utf8.o $(ODIR)/mgl_transtxt.o $(ODIR)/mgl_strtab_extr.o -o mgl_strtab_extr $(CXXFLAGS)
	
mgl_str_insr: blackt $(OBJ)
	$(CXX) $(ODIR)/csv_utf8.o $(ODIR)/mgl_transtxt.o $(ODIR)/mgl_str_insr.o -o mgl_str_insr $(CXXFLAGS)
	
mgl_str_fmtconv: blackt $(OBJ)
	$(CXX) $(ODIR)/csv_utf8.o $(ODIR)/mgl_transtxt.o $(ODIR)/mgl_str_fmtconv.o -o mgl_str_fmtconv $(CXXFLAGS)
	
mgl_img_decmp: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_img_decmp.o -o mgl_img_decmp $(CXXFLAGS)
	
mgl_img_cmp: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_img_cmp.o -o mgl_img_cmp $(CXXFLAGS)
	
mgl_img_inject: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_img_inject.o -o mgl_img_inject $(CXXFLAGS)
	
mgl_grp_conv: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_grp_conv.o -o mgl_grp_conv $(CXXFLAGS)
	
mgl_img_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_img_extr.o -o mgl_img_extr $(CXXFLAGS)
	
mgl_img_insr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_img_insr.o -o mgl_img_insr $(CXXFLAGS)
	
mgl_fieldbod_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_fieldbod_extr.o -o mgl_fieldbod_extr $(CXXFLAGS)
	
mgl_script_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_script_extr.o -o mgl_script_extr $(CXXFLAGS)
	
mgl_dlog_insr: blackt $(OBJ)
	$(CXX) $(ODIR)/csv_utf8.o $(ODIR)/mgl_dlog_insr.o -o mgl_dlog_insr $(CXXFLAGS)
	
mgl_credits_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_credits_extr.o -o mgl_credits_extr $(CXXFLAGS)
	
mgl_credits_insr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_credits_insr.o -o mgl_credits_insr $(CXXFLAGS)
	
mgl_ascii_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_ascii_extr.o -o mgl_ascii_extr $(CXXFLAGS)
	
mgl_ascii_build: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_ascii_build.o -o mgl_ascii_build $(CXXFLAGS)
	
mgl_tileimg_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_tileimg_extr.o -o mgl_tileimg_extr $(CXXFLAGS)
	
mgl_titleimg_extr: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_titleimg_extr.o -o mgl_titleimg_extr $(CXXFLAGS)
	
mgl_colorize: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_colorize.o -o mgl_colorize $(CXXFLAGS)
	
mgl_tileimg_build: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_tileimg_build.o -o mgl_tileimg_build $(CXXFLAGS)
	
mgl_filepatch: blackt $(OBJ)
	$(CXX) $(ODIR)/mgl_filepatch.o -o mgl_filepatch $(CXXFLAGS)

blackt:
	$(MAKE) -C blackt

.PHONY: blackt clean cleanme install

# Clean mgltools
cleanme:
	rm -f $(tools)
	rm -rf $(ODIR)
	rm -f grp

# Clean mgltools and libraries
clean: cleanme
	$(MAKE) -C blackt clean

install: all
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) $(tools) $(BINDIR)
