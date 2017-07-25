.KEEP_STATE:

BASEDIR = ../..
BUILDDIR = ..
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 

META_PATH=../../../metaserver
ifdef DEBUG
LIBPATH = -L../../../metaserver/debug
else
LIBPATH = -L../../../metaserver/release
endif

LIBPATH += -lglib-2.0 -L../../../metaserver/opensrc

SYSLIBS = -lpthread -lcrypto -lms -lcom -lrados -lcassandra -lpthread -lstdc++ -lm -lboost_program_options -lboost_thread -lboost_system -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -ldl -luuid -llttng-ust -luv -lssh2
CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wno-cast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=4 -DMETASERVER 
#CFLAGS += -DMETASERVER_LOCAL_TEST

OBJS = $(SRCS:.c=.o)

all: nidserver	

SRCS = server.c 

SYMLINKS = server.c server.h

include ../server/server_src.inc
include mds_src.inc

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

server.c:
	ln -f -s $(BASEDIR)/server.c server.c

mkmeta:
	cd $(META_PATH); cd -;

nidserver: mkmeta $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f nidserver *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
