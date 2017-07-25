.KEEP_STATE:

BASEDIR = $(shell pwd)
MKCLEAN = make realclean
ITESTSERVER = 10.16.158.200

all: nid

nid:
	rm -f nidserver nidagent nid.ko nidmanager;\
	cd ../../src/user/build/mds;\
	$(MKCLEAN);make DEBUG=6;mv nidserver $(BASEDIR)/;\
	cd ../manager;\
	$(MKCLEAN);make;mv nidmanager $(BASEDIR)/;\
	cd ../agent;\
	$(MKCLEAN);make;mv nidagent $(BASEDIR)/;\
	cd ../../../kernel/driver;\
	$(MKCLEAN);make;mv nid.ko $(BASEDIR)/;\
	scp -r $(BASEDIR) $(ITESTSERVER):~;
	
tags:
	ctags *.[ch] 

clean:
	rm -f nidserver nidagent nid.ko nidmanager

realclean:
	make clean;\
        cd ../../src/user/build/mds;$(MKCLEAN);\
        cd ../manager;$(MKCLEAN);\
        cd ../agent;$(MKCLEAN);\
        cd ../../../kernel/driver;$(MKCLEAN);

