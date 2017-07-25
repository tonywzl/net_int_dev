# $Id $
#ident "$Source $"

# $Copyright $

CC = gcc
KERNDIR := $(shell uname -r)
KERNDIR ?= "3.10.3"
 
SYMLINKS = nid_log.h ml_if.h \
	nl_if.h nl.c

all:	$(SYMLINKS)
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) modules

nid_log.h:
	ln -f -s ../../../nid_log.h nid_log.h

ml_if.h:
	ln -f -s ../../../ml_if.h ml_if.h

nl_if.h:
	ln -f -s ../../../nl_if.h nl_if.h

nl.c:
	ln -f -s ../../../nl.c nl.c

clean:
	@echo "Using kernel module directory: /lib/modules/$(value KERNDIR)"
	make -C /lib/modules/$(KERNDIR)/build M=`pwd` CC=$(CC) clean

realclean: clean
	rm -f $(SYMLINKS)
