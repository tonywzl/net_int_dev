.KEEP_STATE:

BASEDIR = ../..
BUILDDIR = ..
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto -lm

mosquitto=$(notdir $(wildcard /usr/include/mosquitto.h))
ifeq ($(mosquitto), mosquitto.h)
LIBPATH += -lmosquitto
endif

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wcast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=4

ifeq ($(mosquitto), mosquitto.h)
CFLAGS += -DHAS_MQTT
endif

OBJS1 = $(SRCS1:.c=.o)
OBJS2 = $(SRCS2:.c=.o)

all: nidserver bwc_ver_cmp

BASEDIR = ../..

include server_src.inc

SRCS1 = $(SRCS) server.c
SRCS2 = $(SRCS) bwc_ver_cmp.c 

SYMLINKS1 = $(SYMLINKS) server.c
SYMLINKS2 = $(SYMLINKS) bwc_ver_cmp.c

server.c:
	ln -f -s $(BASEDIR)/server.c server.c
	
bwc_ver_cmp.c:
	ln -f -s $(BASEDIR)/cache/bwc_ver_cmp.c bwc_ver_cmp.c

nidserver: $(SYMLINKS1) $(OBJS1)
	$(LINK.c) -o $@ $(OBJS1) $(LIBPATH) $(SYSLIBS)
ifneq ($(mosquitto), mosquitto.h)
	@echo "Mosquitto lib is not installed. If you want to enable MQTT, please follow the steps in Enable_mqtt to install mosquitto lib."
endif

bwc_ver_cmp: $(SYMLINKS2) $(OBJS2)
	$(LINK.c) -o $@ $(OBJS2) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nidserver bwc_ver_cmp *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS1)
	rm -f $(SYMLINKS2)
