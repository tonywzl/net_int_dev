.KEEP_STATE:

INCLUDES = -I./ 
LIppATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIppATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c sds.c ut_main.c pp.c allocator.c lck.c io.c rio.c bio.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid.h nid_log.h nid_log.c nid_shared.h list.h \
	lck_if.h lck.c \
	pp_if.h pp.c allocator.c allocator_if.h node_if.h \
	ds_if.h sds_if.h sds.c sdsg_if.h cdsg_if.h \
	rio_if.h rio.c bio_if.h bio.c io_if.h io.c \
	rw_if.h wc_if.h
	

BASEDIR = ../../..
MM_BASEDIR = ../../../mm
CACHE_BASEDIR = ../../../cache

all: sds_unit

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

list.c:
	ln -f -s $(BASEDIR)/list.c list.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(MM_BASEDIR)/allocator.c allocator.c

rw_if.h:
	ln -f -s $(BASEDIR)/rw/rw_if.h rw_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

io.c:
	ln -f -s $(BASEDIR)/io.c io.c

rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h

rio.c:
	ln -f -s $(BASEDIR)/rio.c rio.c

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

bio.c:
	ln -f -s $(BASEDIR)/bio.c bio.c

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h

sds.c:
	ln -f -s $(BASEDIR)/sds.c sds.c

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h	

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h

sds_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sds_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
