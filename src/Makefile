#
# $Id$
#

CC=gcc
CXX=g++
CPP=cpp

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share

UI=sdl
UILIBS=-L/usr/local/lib -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/lib -Wl,-rpath,/usr/lib/arm-linux-gnueabihf -Wl,-rpath,/lib/arm-linux-gnueabihf -Wl,-rpath,/opt/vc/lib -Wl,-rpath,/opt/vc/lib -L/usr/lib -Wl,-rpath,/usr/lib -L/mingw32/lib -Wl,-rpath,/mingw32/lib -lSDL -lpthread -L/opt/vc/lib -lSDL_mixer
UIFLAGS=-I/usr/local/include/SDL -D_REENTRANT

LIBS=-Wl,--as-needed -Wl,--gc-sections -Wl,--no-seh -Wl,--large-address-aware -L/usr/lib -L/usr/lib/arm-linux-gnueabihf -L/lib/arm-linux-gnueabihf $(UILIBS) -lz -lpng -lxml2
# CCPREFIX=~/raspberry/tools/arm-bcm2708/arm-bcm2708hardfp-linux-gnueabi/bin/arm-bcm2708hardfp-linux-gnueabi-
# CCPREFIX=~/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
FEATURES=-UHAVE_BACKTRACE -DHAVE_VARIADIC_MACROS=1
#DEBUGCXXFLAGS=-O2 -g -fno-lto -DNPERF -marm -mfpu=vfp -march=armv6zk -mtune=arm1176jzf-s -mfloat-abi=hard -UDEBUG -DNDEBUG -DRASB_PI -DNTSC_TV -fno-exceptions -fno-non-call-exceptions -fno-common
DEBUGCXXFLAGS=-O2 -march=pentium4 -mtune=generic -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector-strong -D__USE_MINGW_ANSI_STDIO=1 -UPERF -DNPERF -UNDEBUG -DDEBUG -URASB_PI
COMMON_FLAGS=-mconsole $(FEATURES) -Wall -Wextra -Werror -Wmissing-declarations -I. -I/mingw32/include/libxml2 -I/mingw32/include/SDL $(UIFLAGS) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\" $(DEBUGCXXFLAGS) -D_GNU_SOURCE=1 $(LIBS)
CFLAGS=-std=gnu90 -fanalyzer $(COMMON_FLAGS)
CXXFLAGS=-std=gnu++11 $(COMMON_FLAGS) -Weffc++ -Wold-style-cast
INSTALL=install

ifeq ($(STATIC_GCC_LIBS),true)
    LDFLAGS+=-L. -static-libgcc -static-libstdc++
endif

include Makefile.common

install-prog::
	$(INSTALL) -D -s $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D -s coord $(libdir)/u4g/coord
	$(INSTALL) -D -s dumpsavegame $(libdir)/u4g/dumpsavegame
	$(INSTALL) -D -s tlkconv $(libdir)/u4g/tlkconv
	$(INSTALL) -D -s u4dec $(libdir)/u4g/u4dec
	$(INSTALL) -D -s u4enc $(libdir)/u4g/u4enc
	$(INSTALL) -D -s u4unpackexe $(libdir)/u4g/u4unpackexe
	$(INSTALL) -D -s xor $(libdir)/u4g/xor
	$(INSTALL) -D -s xorbin $(libdir)/u4g/xorbin

install-conf::
	$(INSTALL) ../conf/*.xml $(libdir)/u4g
	mkdir -p $(libdir)/u4g/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4g/dtd

install:: install-prog install-conf
	$(INSTALL) -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4g/music
#	$(INSTALL) ../mid/*.mid $(libdir)/u4g/music
#	$(INSTALL) ../mid/*.it $(libdir)/u4g/music
	$(INSTALL) ../music/*.wav $(libdir)/u4g/music
	mkdir -p $(libdir)/u4g/sound
#	$(INSTALL) ../sound/*.ogg $(libdir)/u4g/sound
	$(INSTALL) ../sound/*.wav $(libdir)/u4g/sound
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

include .depends
