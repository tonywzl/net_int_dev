REP_SRC = reps.c rept.c reptg.c repsg.c reptc.c reptcg.c repsc.c repscg.c

REP_SYMLINKS = nid_log.h nid_log.c list.h \
	reps_if.h reps.c rept_if.h rept.c \
	reptg_if.h reptg.c repsg_if.h repsg.c \
	reptc_if.h reptc.c reptcg_if.h reptcg.c \
	repsc_if.h repsc.c repscg_if.h repscg.c

reps_if.h:
	ln -f -s $(REP_BASEDIR)/reps_if.h reps_if.h

reps.c:
	ln -f -s $(REP_BASEDIR)/reps.c reps.c

rept_if.h:
	ln -f -s $(REP_BASEDIR)/rept_if.h rept_if.h

rept.c:
	ln -f -s $(REP_BASEDIR)/rept.c rept.c
	
reptg_if.h:
	ln -f -s $(REP_BASEDIR)/reptg_if.h reptg_if.h

reptg.c:
	ln -f -s $(REP_BASEDIR)/reptg.c reptg.c

repsg_if.h:
	ln -f -s $(REP_BASEDIR)/repsg_if.h repsg_if.h

repsg.c:
	ln -f -s $(REP_BASEDIR)/repsg.c repsg.c

reptc_if.h:
	ln -f -s $(REP_BASEDIR)/reptc_if.h reptc_if.h

reptc.c:
	ln -f -s $(REP_BASEDIR)/reptc.c reptc.c

reptcg_if.h:
	ln -f -s $(REP_BASEDIR)/reptcg_if.h reptcg_if.h

reptcg.c:
	ln -f -s $(REP_BASEDIR)/reptcg.c reptcg.c

repsc_if.h:
	ln -f -s $(REP_BASEDIR)/repsc_if.h repsc_if.h

repsc.c:
	ln -f -s $(REP_BASEDIR)/repsc.c repsc.c

repscg_if.h:
	ln -f -s $(REP_BASEDIR)/repscg_if.h repscg_if.h

repscg.c:
	ln -f -s $(REP_BASEDIR)/repscg.c repscg.c
