.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0/ -I/usr/lib/glib-2.0/include/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread -lcrypto

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  nid_log.c fpc.c ut_main.c allocator.c lck.c node.c fpn.c server_conf.c hash.c ini.c agent_conf.c \
	asc_conf.c cds_conf.c crc_conf.c cxa_conf.c cxp_conf.c cxt_conf.c dck_conf.c drc_conf.c \
	drw_conf.c dxa_conf.c dxp_conf.c dxt_conf.c mrw_conf.c bwc_conf.c  \
	pp_conf.c rgdev_conf.c rsa_conf.c rsm_conf.c sac_conf.c sarw_conf.c scg_conf.c \
	sds_conf.c server_global_conf.c shm_conf.c tp_conf.c twc_conf.c shr_conf.c lstn.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = nid_log.h nid_log.c fpc_if.h fpc.c allocator_if.h allocator.c lck_if.h lck.c \
	   ini_if.h hash_if.h nid.h  nid_log.c hash.c list.h ini_if.h ini.c server.h server_conf.c \
	   node_if.h node.c fpn_if.h fpn.c lstn_if.h lstn.c agent_conf.c \
	asc_conf.c cds_conf.c crc_conf.c cxa_conf.c cxp_conf.c cxt_conf.c dck_conf.c drc_conf.c \
	drw_conf.c dxa_conf.c dxp_conf.c dxt_conf.c mrw_conf.c bwc_conf.c \
	pp_conf.c rgdev_conf.c rsa_conf.c rsm_conf.c sac_conf.c sarw_conf.c scg_conf.c \
	sds_conf.c server_global_conf.c shm_conf.c tp_conf.c twc_conf.c agent.h shr_conf.c

BASEDIR = ../../..
SHARED_BASEDIR = ../../../../shared

all: hash_fpc

fpn_if.h:
	ln -f -s $(BASEDIR)/mm/fpn_if.h fpn_if.h

fpn.c:
	ln -f -s $(BASEDIR)/mm/fpn.c fpn.c

agent.h:
	ln -f -s $(BASEDIR)/agent.h agent.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h
	
nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

lstn.c:
	ln -f -s $(BASEDIR)/mm/lstn.c lstn.c

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h

fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

hash_if.h:
	ln -f -s $(BASEDIR)/hash_if.h hash_if.h	

hash.c:
	ln -f -s $(BASEDIR)/hash.c hash.c

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

server_conf.c:
	ln -f -s $(BASEDIR)/conf/server_conf.c server_conf.c

agent_conf.c:
	ln -f -s $(BASEDIR)/conf/agent_conf.c agent_conf.c

asc_conf.c:
	ln -f -s $(BASEDIR)/conf/asc_conf.c asc_conf.c

cds_conf.c:
	ln -f -s $(BASEDIR)/conf/cds_conf.c cds_conf.c

crc_conf.c:
	ln -f -s $(BASEDIR)/conf/crc_conf.c crc_conf.c

cxa_conf.c:
	ln -f -s $(BASEDIR)/conf/cxa_conf.c cxa_conf.c

cxp_conf.c:
	ln -f -s $(BASEDIR)/conf/cxp_conf.c cxp_conf.c

cxt_conf.c:
	ln -f -s $(BASEDIR)/conf/cxt_conf.c cxt_conf.c

dck_conf.c:
	ln -f -s $(BASEDIR)/conf/dck_conf.c dck_conf.c

drc_conf.c:
	ln -f -s $(BASEDIR)/conf/drc_conf.c drc_conf.c

drw_conf.c:
	ln -f -s $(BASEDIR)/conf/drw_conf.c drw_conf.c

dxa_conf.c:
	ln -f -s $(BASEDIR)/conf/dxa_conf.c dxa_conf.c

dxp_conf.c:
	ln -f -s $(BASEDIR)/conf/dxp_conf.c dxp_conf.c

dxt_conf.c:
	ln -f -s $(BASEDIR)/conf/dxt_conf.c dxt_conf.c

mrw_conf.c:
	ln -f -s $(BASEDIR)/conf/mrw_conf.c mrw_conf.c

bwc_conf.c:
	ln -f -s $(BASEDIR)/conf/bwc_conf.c bwc_conf.c

pp_conf.c:
	ln -f -s $(BASEDIR)/conf/pp_conf.c pp_conf.c

rgdev_conf.c:
	ln -f -s $(BASEDIR)/conf/rgdev_conf.c rgdev_conf.c

rsa_conf.c:
	ln -f -s $(BASEDIR)/conf/rsa_conf.c rsa_conf.c

rsm_conf.c:
	ln -f -s $(BASEDIR)/conf/rsm_conf.c rsm_conf.c

sac_conf.c:
	ln -f -s $(BASEDIR)/conf/sac_conf.c sac_conf.c

sarw_conf.c:
	ln -f -s $(BASEDIR)/conf/sarw_conf.c sarw_conf.c

scg_conf.c:
	ln -f -s $(BASEDIR)/conf/scg_conf.c scg_conf.c

sds_conf.c:
	ln -f -s $(BASEDIR)/conf/sds_conf.c sds_conf.c

server_global_conf.c:
	ln -f -s $(BASEDIR)/conf/server_global_conf.c server_global_conf.c

shm_conf.c:
	ln -f -s $(BASEDIR)/conf/shm_conf.c shm_conf.c

shr_conf.c:
	ln -f -s $(BASEDIR)/conf/shr_conf.c shr_conf.c

tp_conf.c:
	ln -f -s $(BASEDIR)/conf/tp_conf.c tp_conf.c

twc_conf.c:
	ln -f -s $(BASEDIR)/conf/twc_conf.c twc_conf.c
	
hash_fpc: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(SYSLIBS) $(LIBPATH)

tags:
	ctags *.[ch] 

clean:
	rm -f hash_fpc *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
