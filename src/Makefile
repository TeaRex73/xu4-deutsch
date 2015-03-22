#
# $Id$
#

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share

UI=sdl
UILIBS=-L/usr/local/lib -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/lib -Wl,-rpath,/usr/lib/arm-linux-gnueabihf -Wl,-rpath,/lib/arm-linux-gnueabihf -Wl,-rpath,/opt/vc/lib -lSDL -lpthread -lSDL_mixer
UIFLAGS=-I/usr/local/include/SDL -D_REENTRANT

LIBS=-L/usr/lib -L/usr/lib/arm-linux-gnueabihf -L/lib/arm-linux-gnueabihf $(UILIBS) -lxml2 -lpng -lz
# CCPREFIX=~/raspberry/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-
# CCPREFIX=~/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
CC=$(CCPREFIX)gcc
CXX=$(CCPREFIX)g++
FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCXXFLAGS=-Og -marm -mfpu=vfp -march=native -mtune=native -mfloat-abi=hard -DDEBUG -g -DRASB_PI
# DEBUGCXXFLAGS=-Ofast -march=native -mtune=native -DNDEBUG -URASB_PI -fno-exceptions -fno-non-call-exceptions -fno-common
CXXFLAGS=$(FEATURES) -Wall -pipe -I. -I/usr/include/libxml2 -I/usr/include/SDL -I/usr/include -I/usr/local/include -I/include -I/usr/include/arm-linux-gnueabihf $(UIFLAGS) -I/usr/include/libxml2 -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\" -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS) $(LIBS)
CFLAGS=$(CXXFLAGS)
INSTALL=install

ifeq ($(STATIC_GCC_LIBS),true)
    LDFLAGS+=-L. -static-libgcc
endif

include Makefile.common

install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4g/music
	$(INSTALL) ../mid/*.mid $(libdir)/u4g/music
	$(INSTALL) ../mid/*.it $(libdir)/u4g/music
	mkdir -p $(libdir)/u4g/sound
	$(INSTALL) ../sound/*.ogg $(libdir)/u4g/sound
	$(INSTALL) -D coord $(libdir)/u4g/coord
	$(INSTALL) -D dumpsavegame $(libdir)/u4g/dumpsavegame
	$(INSTALL) -D tlkconv $(libdir)/u4g/tlkconv
	$(INSTALL) -D u4dec $(libdir)/u4g/u4dec
	$(INSTALL) -D u4enc $(libdir)/u4g/u4enc
	$(INSTALL) -D u4unpackexe $(libdir)/u4g/u4unpackexe
	$(INSTALL) ../conf/*.xml $(libdir)/u4g
	mkdir -p $(libdir)/u4g/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4g/dtd
	mkdir -p $(libdir)/u4g/graphics
	mkdir -p $(libdir)/u4g/graphics/ega
	mkdir -p $(libdir)/u4g/graphics/hires
	mkdir -p $(libdir)/u4g/graphics/png
	mkdir -p $(libdir)/u4g/graphics/vga
	mkdir -p $(libdir)/u4g/graphics/vga2
	mkdir -p $(libdir)/u4g/graphics/new
	$(INSTALL) ../graphics/ega/*.png $(libdir)/u4g/graphics/ega
	$(INSTALL) ../graphics/hires/*.png $(libdir)/u4g/graphics/hires
	$(INSTALL) ../graphics/hires/*.vga $(libdir)/u4g/graphics/hires
	$(INSTALL) ../graphics/png/*.png $(libdir)/u4g/graphics/png
	$(INSTALL) ../graphics/vga/*.png $(libdir)/u4g/graphics/vga
	$(INSTALL) ../graphics/vga2/*.png $(libdir)/u4g/graphics/vga2
	$(INSTALL) ../graphics/new/* $(libdir)/u4g/graphics/new
	$(INSTALL) -D ../u4.desktop $(datadir)/applications/u4.desktop


# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

# a target to build without dependencies on libstdc++, libgcc_s
all.static_gcc_libs:
	ln -s `$(CXX) -print-file-name=libstdc++.a` .
	$(MAKE) STATIC_GCC_LIBS=true
	rm -f libstdc++.a

-include .depends

