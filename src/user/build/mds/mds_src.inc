SYMLINKS += ms_intf.h msret.h ioflag.h

ms_intf.h:	
	ln -f -s $(BASEDIR)/../metaserver/include/ms_intf.h ms_intf.h

msret.h:
	ln -f -s $(BASEDIR)/../metaserver/include/msret.h msret.h
	
ioflag.h:
	ln -f -s $(BASEDIR)/../metaserver/include/ioflag.h ioflag.h
	
