.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = nid_log.c dxacg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h nid_log.h nid_log.c nid_shared.h \
	pp2_if.h \
	lstn_if.h node_if.h list.h \
	dxa_if.h \
	dxacg_if.h dxacg.c dxac_if.h \
	scg_if.h \
	io_if.h

BASEDIR = ../../..
DX_BASEDIR = ../../../tx
MM_BASEDIR = ../../../mm
SHARED_BASEDIR = ../../../../shared
UMPK_BASEDIR = ../../../umpk

all: dxacg_unit
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(SHARED_BASEDIR)/nid_shared.h nid_shared.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

dxa_if.h:
	ln -f -s $(DX_BASEDIR)/dxa_if.h dxa_if.h

dxac_if.h:
	ln -f -s $(DX_BASEDIR)/dxac_if.h dxac_if.h

dxacg_if.h:
	ln -f -s $(DX_BASEDIR)/dxacg_if.h dxacg_if.h

dxacg.c:
	ln -f -s $(DX_BASEDIR)/dxacg.c dxacg.c

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h	pp2_if.h

dxacg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dxacg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
