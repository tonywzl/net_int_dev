SRCS += nid_log.c allocator.c rsmg.c ini.c lck.c rsm.c rsun.c rsn.c node.c\

SYMLINKS += version.h nid_shared.h nid.h list.h nid_log.h nid_log.c \
	allocator.c allocator_if.h ini.c ini_if.h rsmg.c rsmg_if.h \
	rsm_if.h rsm.c lck_if.h rsg_if.h lstn_if.h node_if.h rs_if.h \
	rsn_if.h rsun_if.h rsn.c rsun.c node.c  lck.c

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h
	
allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h
	
ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c	

rsm.c:
	ln -f -s $(BASEDIR)/rs/rsm.c rsm.c

rsm_if.h:
	ln -f -s $(BASEDIR)/rs/rsm_if.h rsm_if.h

rsmg.c:
	ln -f -s $(BASEDIR)/rs/rsmg.c rsmg.c

rsmg_if.h:
	ln -f -s $(BASEDIR)/rs/rsmg_if.h rsmg_if.h

rsg_if.h:
	ln -f -s $(BASEDIR)/rs/rsg_if.h rsg_if.h

rs_if.h:
	ln -f -s $(BASEDIR)/rs/rs_if.h rs_if.h

rsn_if.h:
	ln -f -s $(BASEDIR)/mm/rsn_if.h rsn_if.h

rsun_if.h:
	ln -f -s $(BASEDIR)/mm/rsun_if.h rsun_if.h

rsn.c:
	ln -f -s $(BASEDIR)/mm/rsn.c rsn.c

rsun.c:
	ln -f -s $(BASEDIR)/mm/rsun.c rsun.c

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h


