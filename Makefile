TARGET = eReader
OBJS = bg.o bookmark.o charsets.o conf.o ctrl.o display.o fat.o fs.o html.o image.o main.o mp3.o power.o scene.o usb.o win.o\
	./common/log.o ./common/utils.o

INCDIR = $(PSPSDK)/../include

CFLAGS = -O2 -G0 -Wall -DFONT12 #-D_DEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LIBS = ./lib/unzip.a ./lib/libchm.a ./lib/libpng.a ./lib/libgif.a ./lib/libjpeg.a ./lib/libbmp.a ./lib/libtga.a ./lib/libmad.a ./lib/libid3tag.a ./lib/libz.a \
	-lc -lm -lpsppower -lpspaudiolib -lpspaudio -lpspusb -lpspusbstor

#LIBDIR =
#LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = eReader
#PSP_EBOOT_ICON = ICON0.png

include $(PSPSDK)/lib/build.mak
