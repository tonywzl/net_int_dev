.KEEP_STATE:

BASEDIR = $(shell pwd)
MKCLEAN = make realclean
#ITESTSERVER = 10.16.150.208
ITESTSERVER = 127.0.0.1

all: nid

nid:
	make clean;\
	cd ../../src/user/build/server;\
	$(MKCLEAN);make;mv nidserver $(BASEDIR)/;\
	cd ../manager;\
	$(MKCLEAN);make;mv nidmanager $(BASEDIR)/;\
	cd ../agent;\
	$(MKCLEAN);make;mv nidagent $(BASEDIR)/;\
	cd ../../../kernel/driver;\
	$(MKCLEAN);make;mv nid.ko $(BASEDIR)/;\
	cd ..; scp -r $(BASEDIR) $(ITESTSERVER):~;

tags:
	ctags *.[ch]

clean:
	rm -f nidserver nidagent nid.ko nidmanager core*

realclean:
	make clean;\
        cd ../../src/user/build/server;$(MKCLEAN);\
        cd ../manager;$(MKCLEAN);\
        cd ../agent;$(MKCLEAN);\
        cd ../../../kernel/driver;$(MKCLEAN);

