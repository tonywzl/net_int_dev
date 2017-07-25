CACHE_SRC = wcc.c wccg.c \
	twc.c twcc.c twccg.c \
	bwc.c bwc-rec.c bwc-rec2.c bwc-dwr.c bwc-snap.c bwc-ff.c bwcc.c bwccg.c bwcg.c \
	twcg.c wc.c \
	bfe.c bfe-async.c bfe-sync.c bre.c \
	bfp.c bfp1.c bfp2.c bfp3.c bfp4.c bfp5.c \
	bse.c rc.c \
	dsbmp.c dsrec.c nse.c cse.c \
	dsmgr.c wcg.c 

CACHE_SYMLINKS = wcc_if.h wcc.c \
	twcc_if.h twcc.c \
	bwcc_if.h bwcc.c \
	bfp_if.h bfp.c \
	bfp1_if.h bfp1.c \
	bfp2_if.h bfp2.c \
	bfp3_if.h bfp3.c \
	bfp4_if.h bfp4.c \
	bfp5_if.h bfp5.c \
	wc_if.h wc.c \
	wcg_if.h wcg.c \
	twc_if.h twc.c twcg_if.h twcg.c \
	bwc_if.h bwc.c bwc.h bwc-rec.c bwc-rec2.c bwc-dwr.c bwc-snap.c bwc-ff.c bwcg_if.h bwcg.c \
	bse_if.h bse.c \
	bfe_if.h bfe.c \
	bfe-async.c bfe-sync.c \
	bre_if.h bre.c \
	wccg_if.h wccg.c \
	twccg_if.h twccg.c \
	bwccg_if.h bwccg.c \
	rc_if.h rc.c \
	crc_if.h crc.c crcg_if.h crcg.c crcc_if.h crcc.c crccg_if.h crccg.c \
 	dsbmp_if.h dsbmp.c \
	dsrec_if.h dsrec.c \
	dsmgr_if.h dsmgr.c \
	nse_if.h nse.c \
	cse_if.h cse.c 



wc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wc_if.h wc_if.h
	
wc.c:
	ln -f -s $(CACHE_BASEDIR)/wc.c wc.c

wcg_if.h:
	ln -f -s $(CACHE_BASEDIR)/wcg_if.h wcg_if.h
	
wcg.c:
	ln -f -s $(CACHE_BASEDIR)/wcg.c wcg.c

wcc_if.h:
	ln -f -s $(CACHE_BASEDIR)/wcc_if.h wcc_if.h
	
wcc.c:
	ln -f -s $(CACHE_BASEDIR)/wcc.c wcc.c	
	
wccg_if.h:
	ln -f -s $(CACHE_BASEDIR)/wccg_if.h wccg_if.h
	
wccg.c:
	ln -f -s $(CACHE_BASEDIR)/wccg.c wccg.c

twcc_if.h:
	ln -f -s $(CACHE_BASEDIR)/twcc_if.h twcc_if.h

twcc.c:
	ln -f -s $(CACHE_BASEDIR)/twcc.c twcc.c

twc_if.h:
	ln -f -s $(CACHE_BASEDIR)/twc_if.h twc_if.h

twc.c:
	ln -f -s $(CACHE_BASEDIR)/twc.c twc.c

twcg_if.h:
	ln -f -s $(CACHE_BASEDIR)/twcg_if.h twcg_if.h

twcg.c:
	ln -f -s $(CACHE_BASEDIR)/twcg.c twcg.c

twccg_if.h:
	ln -f -s $(CACHE_BASEDIR)/twccg_if.h twccg_if.h

twccg.c:
	ln -f -s $(CACHE_BASEDIR)/twccg.c twccg.c

bwcc_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwcc_if.h bwcc_if.h

bwcc.c:
	ln -f -s $(CACHE_BASEDIR)/bwcc.c bwcc.c

bwc_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwc_if.h bwc_if.h

bwc.c:
	ln -f -s $(CACHE_BASEDIR)/bwc.c bwc.c

bwc.h:
	ln -f -s $(CACHE_BASEDIR)/bwc.h bwc.h

bwc-rec.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-rec.c bwc-rec.c

bwc-rec2.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-rec2.c bwc-rec2.c

bwc-dwr.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-dwr.c bwc-dwr.c

bwc-snap.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-snap.c bwc-snap.c

bwc-ff.c:
	ln -f -s $(CACHE_BASEDIR)/bwc-ff.c bwc-ff.c

bwcg_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwcg_if.h bwcg_if.h

bwcg.c:
	ln -f -s $(CACHE_BASEDIR)/bwcg.c bwcg.c

bwccg_if.h:
	ln -f -s $(CACHE_BASEDIR)/bwccg_if.h bwccg_if.h

bwccg.c:
	ln -f -s $(CACHE_BASEDIR)/bwccg.c bwccg.c

bre_if.h:
	ln -f -s $(CACHE_BASEDIR)/bre_if.h bre_if.h
	
bre.c:
	ln -f -s $(CACHE_BASEDIR)/bre.c bre.c
	
bse_if.h:
	ln -f -s $(CACHE_BASEDIR)/bse_if.h bse_if.h
	
bse.c:
	ln -f -s $(CACHE_BASEDIR)/bse.c bse.c

bfe_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfe_if.h bfe_if.h
	
bfe.c:
	ln -f -s $(CACHE_BASEDIR)/bfe.c bfe.c

bfe-async.c:
	ln -f -s $(CACHE_BASEDIR)/bfe-async.c bfe-async.c

bfe-sync.c:
	ln -f -s $(CACHE_BASEDIR)/bfe-sync.c bfe-sync.c

bfp_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp_if.h bfp_if.h
	
bfp.c:
	ln -f -s $(CACHE_BASEDIR)/bfp.c bfp.c
	
bfp1_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp1_if.h bfp1_if.h

bfp1.c:
	ln -f -s $(CACHE_BASEDIR)/bfp1.c bfp1.c	

bfp2_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp2_if.h bfp2_if.h

bfp2.c:
	ln -f -s $(CACHE_BASEDIR)/bfp2.c bfp2.c

bfp3_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp3_if.h bfp3_if.h

bfp3.c:
	ln -f -s $(CACHE_BASEDIR)/bfp3.c bfp3.c

bfp4_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp4_if.h bfp4_if.h

bfp4.c:
	ln -f -s $(CACHE_BASEDIR)/bfp4.c bfp4.c

bfp5_if.h:
	ln -f -s $(CACHE_BASEDIR)/bfp5_if.h bfp5_if.h

bfp5.c:
	ln -f -s $(CACHE_BASEDIR)/bfp5.c bfp5.c

crc_if.h:
	ln -f -s $(CACHE_BASEDIR)/crc_if.h crc_if.h
	
crc.c:
	ln -f -s $(CACHE_BASEDIR)/crc.c crc.c	

crcg_if.h:
	ln -f -s $(CACHE_BASEDIR)/crcg_if.h crcg_if.h
	
crcg.c:
	ln -f -s $(CACHE_BASEDIR)/crcg.c crcg.c	

crcc_if.h:
	ln -f -s $(CACHE_BASEDIR)/crcc_if.h crcc_if.h
	
crcc.c:
	ln -f -s $(CACHE_BASEDIR)/crcc.c crcc.c

crccg_if.h:
	ln -f -s $(CACHE_BASEDIR)/crccg_if.h crccg_if.h
	
crccg.c:
	ln -f -s $(CACHE_BASEDIR)/crccg.c crccg.c

rc_if.h:
	ln -f -s $(CACHE_BASEDIR)/rc_if.h rc_if.h
	
rc.c:
	ln -f -s $(CACHE_BASEDIR)/rc.c rc.c

dsbmp_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsbmp_if.h dsbmp_if.h
	
dsbmp.c:
	ln -f -s $(CACHE_BASEDIR)/dsbmp.c dsbmp.c

dsmgr_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsmgr_if.h dsmgr_if.h
	
dsmgr.c:
	ln -f -s $(CACHE_BASEDIR)/dsmgr.c dsmgr.c
	
dsrec_if.h:
	ln -f -s $(CACHE_BASEDIR)/dsrec_if.h dsrec_if.h
	
dsrec.c:
	ln -f -s $(CACHE_BASEDIR)/dsrec.c dsrec.c	

cse_if.h:
	ln -f -s $(CACHE_BASEDIR)/cse_if.h cse_if.h
	
cse.c:
	ln -f -s $(CACHE_BASEDIR)/cse.c cse.c

nse_if.h:
	ln -f -s $(CACHE_BASEDIR)/nse_if.h nse_if.h
	
nse.c:
	ln -f -s $(CACHE_BASEDIR)/nse.c nse.c


