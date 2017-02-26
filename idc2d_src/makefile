CXX			= g++.exe
CC			= gcc.exe
WINDRES	= windres.exe

EXEEXT		= .exe
.SUFFIXES: .o .c .cpp .cxx .h $(EXEEXT)

RES		= i_current_private.res
RC		= i_current_private.rc
OBJS	= browser_log.o button_callbacks.o i_current-2.o i_dc.o  main.o o_image.o Fl_Image_Display.o $(RES)
SRCS	= browser_log.cpp button_callbacks.cpp i_current-2.cxx i_dc.cpp main.cpp o_image.cpp Fl_Image_Display.cxx
LIBS	= -L/usr/local/lib -lfltk -lfltk_images -lfltk_png -lfltk_z -lfltk_jpeg -lole32 -lwsock32 -lcomctl32 -luuid -lgomp -lpthread
INCS	= -I/usr/local/include -I/usr/local/include/FL/images
DEFS	= -DWIN32
BIN		= icd2d$(EXEEXT)
LDFLAGS		= -mwindows  -fopenmp
CXXFLAGS	= $(INCS) $(DEFS) $(LDFLAGS) -O2 -Wall -fno-strict-aliasing
#CXXFLAGS	= $(INCS) $(DEFS) $(LDFLAGS)

RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	@echo Cleaning...
	${RM} $(OBJS) $(BIN) $(RES)

$(BIN):$(OBJS)
	@echo Linking $@...
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	@echo Compiling...
	$(CXX) $(CXXFLAGS) -c $<

.cxx.o:
	@echo Compiling...
	$(CXX) $(CXXFLAGS) -c $<

$(RES):$(RC)
	@echo Compiling $<...
	$(WINDRES) -i $(RC) --input-format=rc -o $(RES) -O coff 
