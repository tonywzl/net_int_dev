.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bfp2.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid_log.c lck_if.h \
	pp_if.h wc_if.h \
	bfp_if.h bfp2_if.h bfp2.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: bfp2_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

bfp_if.h:
	ln -f -s $(BASEDIR)/bfp_if.h bfp_if.h

bfp2_if.h:
	ln -f -s $(BASEDIR)/bfp2_if.h bfp2_if.h

bfp2.c:
	ln -f -s $(BASEDIR)/bfp2.c bfp2.c
	

bfp2_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bfp2_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
