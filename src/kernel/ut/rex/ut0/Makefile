# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_shared.h nid.h nid_log.h drg_if.h \
	rex.h rex_if.h rex-policy0.c rex.c

BASEDIR = ../../..

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

drg_if.h:
	ln -f -s ../../../drg_if.h drg_if.h

rex.h:
	ln -f -s ../../../rex.h rex.h

rex_if.h:
	ln -f -s ../../../rex_if.h rex_if.h

rex-policy0.c:
	ln -f -s ../../../rex-policy0.c rex-policy0.c

rex.c:
	ln -f -s ../../../rex.c rex.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
