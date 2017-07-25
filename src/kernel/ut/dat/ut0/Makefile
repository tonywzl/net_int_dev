# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_log.h \
	nl_if.h mq_if.h \
	dat_if.h dat.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

dat_if.h:
	ln -f -s $(BASEDIR)/dat_if.h dat_if.h

dat.c:
	ln -f -s $(BASEDIR)/dat.c dat.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
