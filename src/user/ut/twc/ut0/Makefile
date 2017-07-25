.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c twc.c ut_main.c rc_twc_cbn.c rc_twcn.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid.h nid_log.h nid_log.c nid_shared.h umpk_wc_if.h umpk_twc_if.h lck_if.h atomic.h tp_if.h ini_if.h smc_if.h allocator_if.h \
	node_if.h ddn_if.h d2an_if.h d2bn_if.h srn_if.h fpn_if.h bre_if.h \
	io_if.h sac_if.h ds_if.h rw_if.h pp_if.h bse_if.h rc_if.h wc_if.h lstn_if.h rc_twc_cbn.h rc_twcn.h\
	twc_if.h twc.c rc_twc_cbn.c rc_twcn.c

BASEDIR = ../../..
MM_BASEDIR = $(BASEDIR)/mm
UMPK_BASEDIR = $(BASEDIR)/umpk

all: twc_unit
	
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

umpk_wc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc_if.h umpk_wc_if.h

umpk_twc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc_if.h umpk_twc_if.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

ddn_if.h:
	ln -f -s $(MM_BASEDIR)/ddn_if.h ddn_if.h

d2an_if.h:
	ln -f -s $(MM_BASEDIR)/d2an_if.h d2an_if.h

d2bn_if.h:
	ln -f -s $(MM_BASEDIR)/d2bn_if.h d2bn_if.h

srn_if.h:
	ln -f -s $(MM_BASEDIR)/srn_if.h srn_if.h

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h
	
lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h
	
rc_twc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_twc_cbn.h rc_twc_cbn.h

rc_twc_cbn.c:
	ln -f -s $(MM_BASEDIR)/rc_twc_cbn.c rc_twc_cbn.c

rc_twcn.h:
	ln -f -s $(MM_BASEDIR)/rc_twcn.h rc_twcn.h

rc_twcn.c:
	ln -f -s $(MM_BASEDIR)/rc_twcn.c rc_twcn.c

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h

rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

twc_if.h:
	ln -f -s $(BASEDIR)/twc_if.h twc_if.h

twc.c:
	ln -f -s $(BASEDIR)/twc.c twc.c

bre_if.h:
	ln -f -s $(BASEDIR)/bre_if.h bre_if.h
	

twc_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f twc_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
