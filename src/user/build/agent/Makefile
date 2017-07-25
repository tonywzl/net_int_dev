.KEEP_STATE:

BASEDIR = ../../
CONF_BASEDIR = $(BASEDIR)/conf
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

mosquitto=$(notdir $(wildcard /usr/include/mosquitto.h))
ifeq ($(mosquitto), mosquitto.h)
LIBPATH += -lmosquitto
endif

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=4

ifeq ($(mosquitto), mosquitto.h)
CFLAGS += -DHAS_MQTT
endif

OBJS = $(SRCS:.c=.o)

all: nidagent

include agent_src.inc
	
nidagent: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)
ifneq ($(mosquitto), mosquitto.h)
	@echo "Mosquitto lib is not installed. If you want to enable MQTT, please follow the steps in Enable_mqtt to install mosquitto lib."
endif

tags:
	ctags *.[ch] 

clean:
	rm -f nidagent *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
