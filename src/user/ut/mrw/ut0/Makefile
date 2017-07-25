.KEEP_STATE:

BASEDIR = ../../../
INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/lib/glib-2.0/include 

META_PATH=$(BASEDIR)/../mds

LIBPATH = -L$(META_PATH)/build -lglib-2.0 -L$(META_PATH)/opensrc
SYSLIBS = -lpthread -lcrypto -lms -lcom -lrados -lcassandra -lpthread -lstdc++ -lm -lboost_program_options -lboost_thread -lboost_system -lnss3 -lnssutil3 -lsmime3 -lssl3 -lplds4 -lplc4 -lnspr4 -ldl -luuid -llttng-ust -luv -lssh2
CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wno-cast-qual -Wshadow -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=1 -DMETASERVER

SRCS = ut_main.c nid_log.c mrw.c fpc.c fpn.c node.c allocator.c lck.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = allocator_if.h allocator.c rw_if.h list.h nid_log.h nid_log.c mrw_if.h mrw.c fpc_if.h fpc.c fpn_if.h fpn.c \
	node_if.h node.c ms_intf.h msret.h cb.h pub.h vector.h ioflag.h lck_if.h lck.c

all: ut0

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h
	
lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c
	
mrw_if.h:
	ln -f -s $(BASEDIR)/mrw_if.h mrw_if.h
	
mrw.c:
	ln -f -s $(BASEDIR)/mrw.c mrw.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h
	
fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h
	
fpn.c:
	ln -f -s $(BASEDIR)/mm/fpn.c fpn.c
	
fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h
	
fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c
	
node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h
	
node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c
	
ms_intf.h:
	ln -f -s $(BASEDIR)/../mds/src/include/ms_intf.h ms_intf.h

msret.h:
	ln -f -s $(BASEDIR)/../mds/src/include/msret.h msret.h
	
cb.h:
	ln -f -s $(BASEDIR)/../mds/src/include/cb.h cb.h

pub.h:
	ln -f -s $(BASEDIR)/../mds/src/include/pub.h pub.h

vector.h:
	ln -f -s $(BASEDIR)/../mds/src/include/vector.h vector.h
	
ioflag.h:
	ln -f -s $(BASEDIR)/../mds/src/include/ioflag.h ioflag.h

mkmeta:
	cd $(META_PATH); make; cd -;

ut0: mkmeta $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f ut0 *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
