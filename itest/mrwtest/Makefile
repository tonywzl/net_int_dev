.KEEP_STATE:

BASEDIR = $(shell pwd)
MKCLEAN = make realclean
ITESTSERVER = 10.21.150.209
#ITESTSERVER = 10.16.150.208
#ITESTSERVER = 10.21.147.102
#ITESTSERVER = 10.21.150.255

all: nid

nid:
	rm -f nidserver nidagent nid.ko nidmanager;\
	cd ../../src/mds; rm -rf ./build/*; make DEBUG=6;\
	cd ../user/build/mds;\
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


	
	
