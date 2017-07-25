RW_SRC = mrw.c mrwc.c mrwg.c mrwcg.c \
	drw.c drwg.c drwc.c drwcg.c \
	rw.c rwg.c

RW_SYMLINKS = drw_if.h drw.c drwg_if.h drwg.c drwc_if.h drwc.c drwcg_if.h drwcg.c \
	mrw_if.h mrw.c mrwg_if.h mrwg.c mrwc_if.h mrwc.c mrwcg_if.h mrwcg.c \
	rw_if.h rw.c rwg.c rwg_if.h

drwc.c:
	ln -f -s $(RW_BASEDIR)/drwc.c drwc.c

drwc_if.h:
	ln -f -s $(RW_BASEDIR)/drwc_if.h drwc_if.h

drwcg.c:
	ln -f -s $(RW_BASEDIR)/drwcg.c drwcg.c

drwcg_if.h:
	ln -f -s $(RW_BASEDIR)/drwcg_if.h drwcg_if.h

drw_if.h:
	ln -f -s $(RW_BASEDIR)/drw_if.h drw_if.h
	
drw.c:
	ln -f -s $(RW_BASEDIR)/drw.c drw.c

drwg_if.h:
	ln -f -s $(RW_BASEDIR)/drwg_if.h drwg_if.h
	
drwg.c:
	ln -f -s $(RW_BASEDIR)/drwg.c drwg.c

mrw_if.h:
	ln -f -s $(RW_BASEDIR)/mrw_if.h mrw_if.h
	
mrw.c:
	ln -f -s $(RW_BASEDIR)/mrw.c mrw.c

mrwg_if.h:
	ln -f -s $(RW_BASEDIR)/mrwg_if.h mrwg_if.h

mrwg.c:
	ln -f -s $(RW_BASEDIR)/mrwg.c mrwg.c

mrwc_if.h:
	ln -f -s $(RW_BASEDIR)/mrwc_if.h mrwc_if.h
	
mrwc.c:
	ln -f -s $(RW_BASEDIR)/mrwc.c mrwc.c

mrwcg_if.h:
	ln -f -s $(RW_BASEDIR)/mrwcg_if.h mrwcg_if.h
	
mrwcg.c:
	ln -f -s $(RW_BASEDIR)/mrwcg.c mrwcg.c

rw_if.h:
	ln -f -s $(RW_BASEDIR)/rw_if.h rw_if.h
	
rw.c:
	ln -f -s $(RW_BASEDIR)/rw.c rw.c	

rwg_if.h:
	ln -f -s $(RW_BASEDIR)/rwg_if.h rwg_if.h

rwg.c:
	ln -f -s $(RW_BASEDIR)/rwg.c rwg.c


