.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c dit.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c list.h ini_if.h tp_if.h \
	dcn_if.h rdg_if.h olap_if.h \
	dit_if.h dit.c

BASEDIR = ../../../..
SHARED_BASEDIR = ../../../../../shared
TESTBASEDIR = ../../..

all: dit_ut
	
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

dcn_if.h:
	ln -f -s $(BASEDIR)/dcn_if.h dcn_if.h

rdg_if.h:
	ln -f -s $(TESTBASEDIR)/rdg_if.h rdg_if.h

olap_if.h:
	ln -f -s $(BASEDIR)/olap_if.h olap_if.h

dit_if.h:
	ln -f -s $(TESTBASEDIR)/dit_if.h dit_if.h

dit.c:
	ln -f -s $(TESTBASEDIR)/dit.c dit.c
	

dit_ut: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f dit_ut *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
