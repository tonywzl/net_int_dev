.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  util_nw.c amc.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h nid.h nid_shared.h util_nw.h ini_if.h\
	   mpk_if.h \
	   util_nw.h util_nw.c \
	   acg_if.h asc_if.h \
	   amc_if.h amc.c

BASEDIR = ../../..

all: amc_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

amc_if.h:
	ln -f -s $(BASEDIR)/amc_if.h amc_if.h

amc.c:
	ln -f -s $(BASEDIR)/amc.c amc.c

amc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f amc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
