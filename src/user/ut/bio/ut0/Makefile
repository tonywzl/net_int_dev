.KEEP_STATE:

INCLUDES = -I./ 
LIBPATH = 
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c bio.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid.h nid_log.h nid_log.c nid_shared.h lck_if.h tp_if.h ini_if.h smc_if.h \
	io_if.h sac_if.h ds_if.h rw_if.h pp_if.h bse_if.h bfp_if.h bfe_if.h bre_if.h ddn_if.h srn_if.h wc_if.h \
	bio_if.h bio.c

BASEDIR = ../../..

all: bio_unit
	
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

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h

io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h

rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h

pp_if.h:
	ln -f -s $(BASEDIR)/mm/pp_if.h pp_if.h

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h

bfp_if.h:
	ln -f -s $(BASEDIR)/bfp_if.h bfp_if.h

bfe_if.h:
	ln -f -s $(BASEDIR)/bfe_if.h bfe_if.h

bre_if.h:
	ln -f -s $(BASEDIR)/bre_if.h bre_if.h

ddn_if.h:
	ln -f -s $(BASEDIR)/mm/ddn_if.h ddn_if.h

srn_if.h:
	ln -f -s $(BASEDIR)/mm/srn_if.h srn_if.h

wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h

bio.c:
	ln -f -s $(BASEDIR)/bio.c bio.c
	

bio_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f bio_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
