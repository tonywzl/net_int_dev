.KEEP_STATE:

BASEDIR = $(shell pwd)
MKCLEAN = make realclean
#ITESTSERVER = 10.16.150.208
#ITESTSERVER = 10.21.150.209
#ITESTSERVER = 10.16.99.99
ITESTSERVER = 10.21.147.106
#ITESTSERVER = 10.21.111.134

all: nid

nid:
	rm -f nidserver bwc_ver_cmp nidagent nid.ko nidmanager;\
	cd ../../src/user/build/server;\
	$(MKCLEAN);make;mv nidserver bwc_ver_cmp $(BASEDIR)/;\
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
	rm -f nidserver nidagent nid.ko nidmanager bwc_ver_cmp

realclean:
	make clean;\
        cd ../../src/user/build/server;$(MKCLEAN);\
        cd ../manager;$(MKCLEAN);\
        cd ../agent;$(MKCLEAN);\
        cd ../../../kernel/driver;$(MKCLEAN);

