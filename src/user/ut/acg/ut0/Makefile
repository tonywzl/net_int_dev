.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c acg.c ut_main.c

OBJS = $(SRCS:.c=.o)
BASEDIR = ../../..

SYMLINKS = list.h nid_log.h nid_log.c nid_shared.h mq_if.h nid.h agent.h \
	   mpk_if.h \
	   tp_if.h \
	   ini_if.h \
	   adt_if.h \
	   cc_if.h \
	   asc_if.h \
	   ash_if.h \
	   amc_if.h \
	   drv_if.h \
	   devg_if.h \
	   ascg_if.h acg_if.h acg.c

all: acg_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

agent.h:
	ln -f -s $(BASEDIR)/agent.h agent.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

adt_if.h:
	ln -f -s $(BASEDIR)/adt_if.h adt_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

cc_if.h:
	ln -f -s $(BASEDIR)/cc_if.h cc_if.h

ash_if.h:
	ln -f -s $(BASEDIR)/ash_if.h ash_if.h

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

amc_if.h:
	ln -f -s $(BASEDIR)/amc_if.h amc_if.h

mwl_if.h:
	ln -f -s $(BASEDIR)/mwl_if.h mwl_if.h

drv_if.h:
	ln -f -s $(BASEDIR)/drv_if.h drv_if.h

devg_if.h:
	ln -f -s $(BASEDIR)/devg_if.h devg_if.h

ascg_if.h:
	ln -f -s $(BASEDIR)/ascg_if.h ascg_if.h

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

acg.c:
	ln -f -s $(BASEDIR)/acg.c acg.c

acg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f acg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
