.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS = util_nw.c nid_log.c  scg.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = util_nw.c util_nw.h list.h nid_log.h nid_log.c lck_if.h mpk_if.h nid.h nid_shared.h allocator_if.h server.h \
	node_if.h lstn_if.h brn_if.h srn_if.h fpn_if.h \
	tp_if.h tpg_if.h ini_if.h cc_if.h sah_if.h sac_if.h \
	sacg_if.h doag_if.h doacg_if.h snapg_if.h smc_if.h smcg_if.h wccg_if.h bwccg_if.h crc_if.h crccg_if.h \
	ds_if.h pp_if.h \
	io_if.h rw_if.h wc_if.h rc_if.h \
	scg_if.h scg.c

BASEDIR = ../../..

all: scg_unit
	
util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

list.h:
	ln -f -s ../../../list.h list.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

nid_log.c:
	ln -f -s ../../../nid_log.c nid_log.c

lck_if.h:
	ln -f -s ../../../lck_if.h lck_if.h

nid.h:
	ln -f -s ../../../nid.h nid.h

allocator_if.h:
	ln -f -s ../../../mm/allocator_if.h allocator_if.h

server.h:
	ln -f -s ../../../server.h server.h

node_if.h:
	ln -f -s ../../../mm/node_if.h node_if.h

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

brn_if.h:
	ln -f -s $(BASEDIR)/mm/brn_if.h brn_if.h

srn_if.h:
	ln -f -s ../../../mm/srn_if.h srn_if.h

fpn_if.h:
	ln -f -s ../../../mm/fpn_if.h fpn_if.h

tp_if.h:
	ln -f -s ../../../tp_if.h tp_if.h

tpg_if.h:
	ln -f -s ../../../tpg_if.h tpg_if.h

ini_if.h:
	ln -f -s ../../../ini_if.h ini_if.h

cc_if.h:
	ln -f -s $(BASEDIR)/cc_if.h cc_if.h

sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

sacg_if.h:
	ln -f -s $(BASEDIR)/sacg_if.h sacg_if.h

doag_if.h:
	ln -f -s $(BASEDIR)/doag_if.h doag_if.h

doacg_if.h:
	ln -f -s $(BASEDIR)/doacg_if.h doacg_if.h

snapg_if.h:
	ln -f -s $(BASEDIR)/snapg_if.h snapg_if.h

smcg_if.h:
	ln -f -s $(BASEDIR)/smcg_if.h smcg_if.h

wccg_if.h:
	ln -f -s $(BASEDIR)/wccg_if.h wccg_if.h

bwccg_if.h:
	ln -f -s $(BASEDIR)/bwccg_if.h bwccg_if.h

crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h

crccg_if.h:
	ln -f -s $(BASEDIR)/crccg_if.h crccg_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h

scg.c:
	ln -f -s $(BASEDIR)/scg.c scg.c

scg_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f scg_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
