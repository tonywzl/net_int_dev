.KEEP_STATE:

BASEDIR = ./user

all: server agent manager

server:
	cd ./src/user/build/server;\
	make;mv nidserver ../../../../;

agent:
	cd ./src/user/build/agent;\
	make;mv nidagent ../../../../;

manager:
	cd ./src/user/build/manager;\
	make;mv nidmanager ../../../../;

kernel:
	cd ./src/kernel/driver;\
	make;mv nid.ko ../../../;
 
tags:
	ctags *.[ch] 

clean:
	rm -f nidserver nidagent nid.ko nidmanager

realclean:
	rm -f nidserver nidagent nidmanager
	cd ./src/user/build/server; make realclean;
	cd ./src/user/build/agent; make realclean;
	cd ./src/user/build/manager; make realclean;
	cd ./src/kernel/driver; make realclean;

