.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c mdoa.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS =  nid_log.c nid_log.h nid.h nid_shared.h mpk_if.h\
	    mdoa_if.h mdoa.c

BASEDIR = ../../..
all: mdoa_unit
	
nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_log.c:
	ln -f -s ../../../nid_log.c nid_log.c

nid.h:
	ln -f -s ../../../nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mdoa_if.h:
	ln -f -s ../../../mdoa_if.h mdoa_if.h

mdoa.c:
	ln -f -s ../../../mdoa.c mdoa.c

mdoa_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f mdoa_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
