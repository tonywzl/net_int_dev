.KEEP_STATE:

BASEDIR = $(shell pwd)
MKCLEAN = make realclean
ITESTSERVER = 127.0.0.1

all: nid

nid:
	rm -f nidserver nidmanager;\
	cd ../../src/user/build/server;\
	$(MKCLEAN);make;mv nidserver $(BASEDIR)/;\
	cd ../manager;\
	$(MKCLEAN);make;mv nidmanager $(BASEDIR)/;\
	cd ../../../../itest/; scp -r $(BASEDIR) $(ITESTSERVER):~;
	
tags:
	ctags *.[ch] 

clean:
	rm -f nidserver nidmanager

realclean:
	make clean;\
        cd ../../src/user/build/server;$(MKCLEAN);\
        cd ../../src/user/build/manager;$(MKCLEAN);\

