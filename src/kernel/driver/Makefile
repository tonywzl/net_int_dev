# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERN_VERSION := $(shell uname -r)
#KERNDIR ?= "/build/image"
#KERNDIR ?= "/build/src/image"
KERNDIR ?= "/lib/modules/${KERN_VERSION}/build"
 
SYMLINKS = version.h nid_log.h nid_shared.h \
	   mpk_if.h mpk_subr.c \
	   mpk_if.h mpk.c \
	   dat_if.h dat.c \
	   mq_if.h mq.c \
	   nl_if.h nl.c \
	   ddg_if.h ddg.c \
	   dev_if.h dev.c \
	   nid.h \
	   drv_if.h drv.c \
	   nid_mod.c

BASEDIR = ..

all: $(SYMLINKS)
	@echo "Using kernel module directory: $(value KERNDIR)"
	make -C $(KERNDIR) M=`pwd` CC=$(CC) modules
version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h
nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h


mpk_subr.c:
	ln -f -s $(BASEDIR)/../shared/mpk_subr.c mpk_subr.c

mpk.c:
	ln -f -s $(BASEDIR)/../shared/mpk.c mpk.c

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

ddg_if.h:
	ln -f -s $(BASEDIR)/ddg_if.h ddg_if.h

ddg.c:
	ln -f -s $(BASEDIR)/ddg.c ddg.c

dev_if.h:
	ln -f -s $(BASEDIR)/dev_if.h dev_if.h

dev.c:
	ln -f -s $(BASEDIR)/dev.c dev.c

dat_if.h:
	ln -f -s $(BASEDIR)/dat_if.h dat_if.h

dat.c:
	ln -f -s $(BASEDIR)/dat.c dat.c

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

mq.c:
	ln -f -s $(BASEDIR)/mq.c mq.c

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

nl.c:
	ln -f -s $(BASEDIR)/nl.c nl.c

drv_if.h:
	ln -f -s $(BASEDIR)/drv_if.h drv_if.h

drv.c:
	ln -f -s $(BASEDIR)/drv.c drv.c

nid_mod.c:
	ln -f -s $(BASEDIR)/nid_mod.c nid_mod.c

clean:
	@echo "Using kernel module directory: $(value KERNDIR)"
	make -C $(KERNDIR) M=`pwd` CC=$(CC) clean

realclean: clean
	rm -rf $(SYMLINKS) nid.ko *.o
