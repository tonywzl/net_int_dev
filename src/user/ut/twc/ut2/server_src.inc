SRCS += nid_log.c list_sort.c allocator.c fpc.c \
	umpk.c umpk_wc.c umpk_bwc.c umpk_dck.c umpk_crc.c umpk_doa.c umpk_drc.c \
	umpk_mrw.c umpk_twc.c umpk_sac.c umpk_pp.c umpk_sds.c umpk_drw.c \
	lstn.c fpn.c brn.c blksn.c node.c util_nw.c bit.c mpk.c mpk_subr.c ini.c lck.c rtn.c btn.c rtree.c tp.c \
	wcc.c wccg.c bwcc.c dckc.c bwccg.c dckcg.c crcc.c crccg.c twc.c twcc.c twccg.c \
	sah.c ds.c cds.c cdsg.c smc.c scg.c drcc.c drccg.c sacc.c saccg.c drwc.c drwcg.c \
	pp.c ppg.c ppc.c ppcg.c pp2.c\
	sds.c sdsg.c sdsc.c sdscg.c \
	umpk_dxa.c umpk_dxp.c \
	dxp.c dxpg.c dxpc.c dxpcg.c \
	dxa.c dxag.c dxac.c dxacg.c \
	bwc.c bwcg.c \
	io.c rio.c bio.c wc.c dck.c rc.c crc.c rw.c mrw.c mrwc.c mrwcg.c dsmgr.c drc.c \
	sacg.c doa.c doac.c doacg.c snap.c snapg.c smcg.c \
	dsbmp.c dsrec.c bfp.c cdn.c ddn.c d1an.c d2an.c d2bn.c d2cn.c bsn.c srn.c nse.c cse.c bse.c \
	bfe.c bre.c nw.c nw_service.c bfp1.c bfp2.c bfp3.c cxa.c cxag.c cxp.c cxpg.c \
	server_conf.c mrw_rn.c mrw_wn.c mrw_wsn.c rc_twc_cbn.c rc_twcn.c bfea.c bfes.c drwg.c \
	rc_bwc_cbn.c rc_bwcn.c nse_crc_cbn.c cse_crc_cbn.c nse_mdsn.c nse_mds_cbn.c cse_mds_cbn.c twcg.c crcg.c dxt.c

SYMLINKS += version.h nid_shared.h nid.h list.h nid_log.h nid_log.c fpc.c fpc_if.h \
	rc_bwc_cbn.h rc_bwc_cbn.c \
	rc_bwcn.h rc_bwcn.c \
	rc_twc_cbn.h rc_twc_cbn.c \
	rc_twcn.h rc_twcn.c \
	nse_mdsn.h nse_mdsn.c \
	nse_mds_cbn.h nse_mds_cbn.c \
	cse_mds_cbn.h cse_mds_cbn.c \
	nse_crc_cbn.c nse_crc_cbn.h \
	cse_crc_cbn.c cse_crc_cbn.h \
	list_sort.h list_sort.c \
	util_nw.h util_nw.c \
	allocator_if.h  allocator.c \
	node_if.h node.c \
	lstn_if.h lstn.c \
	fpn_if.h fpn.c \
	brn_if.h brn.c \
	blksn_if.h blksn.c \
	bit_if.h bit.c \
	umpk_if.h umpk.c umpk.h \
	umpk_wc_if.h umpk_wc.c \
	umpk_bwc_if.h umpk_bwc.c \
	umpk_twc_if.h umpk_twc.c \
	umpk_mrw_if.h umpk_mrw.c \
	umpk_dck_if.h umpk_dck.c \
	umpk_crc_if.h umpk_crc.c \
	umpk_doa_if.h umpk_doa.c \
	umpk_sac_if.h umpk_sac.c \
	umpk_pp_if.h umpk_pp.c \
	umpk_sds_if.h umpk_sds.c \
	umpk_drw_if.h umpk_drw.c \
	mpk_if.h mpk.c mpk_subr.c \
	umpk_dxa_if.h umpk_dxa.c \
	umpk_dxp_if.h umpk_dxp.c \
	twcg_if.h twcg.c \
	ini_if.h ini.c \
	lck_if.h lck.c \
	atomic.h \
	rtn_if.h rtn.c \
	btn_if.h btn.c \
	rtree_if.h rtree.c \
	tp_if.h tp.c \
	ds_if.h ds.c \
	cds_if.h cds.c cdsg_if.h cdsg.c \
	sds_if.h sds.c sdsg_if.h sdsg.c \
	dxp_if.h dxp.c dxpg_if.h dxpg.c dxpc_if.h dxpc.c dxpcg_if.h dxpcg.c dxt_if.h dxt.c \
	dxa_if.h dxa.c dxag_if.h dxag.c dxac_if.h dxac.c dxacg_if.h dxacg.c \
	pp_if.h pp.c pp2_if.h pp2.c \
	ppg_if.h ppg.c\
	smc_if.h smc.c \
	wcc_if.h wcc.c \
	bwcc_if.h bwcc.c \
	twcc_if.h twcc.c \
	dckc_if.h dckc.c \
	crcc_if.h crcc.c \
	crcg_if.h crcg.c \
	cxa_if.h cxa.c \
	cxag_if.h cxag.c \
	cxp_if.h cxp.c \
	cxpg_if.h cxpg.c \
	scg_if.h scg.c \
	io_if.h io.c \
	rio_if.h rio.c \
	bfp_if.h bfp.c \
	bfp1_if.h bfp1.c \
	bfp2_if.h bfp2.c \
	bfp3_if.h bfp3.c \
	bio_if.h bio.c \
	wc_if.h wc.c \
	bwc_if.h bwc.c bwcg_if.h bwcg.c \
	twc_if.h twc.c \
	dck_if.h dck.c \
	rc_if.h rc.c \
	crc_if.h crc.c \
	rw_if.h rw.c \
	drw_if.h \
	mrw_if.h mrw.c \
	mrwc_if.h mrwc.c \
	mrwcg_if.h mrwcg.c \
	dsbmp_if.h dsbmp.c \
	dsrec_if.h dsrec.c \
	dsmgr_if.h dsmgr.c \
	ddn_if.h ddn.c \
	d1an_if.h d1an.c \
	d2an_if.h d2an.c \
	d2bn_if.h d2bn.c \
	d2cn_if.h d2cn.c \
	bsn_if.h bsn.c \
	cdn_if.h cdn.c \
	srn_if.h srn.c \
	bse_if.h bse.c \
	nse_if.h nse.c \
	cse_if.h cse.c \
	bfe_if.h bfe.c \
	bfea_if.h bfea.c \
	bfes_if.h bfes.c \
	bre_if.h bre.c \
	sah_if.h sah.c \
	nw.h nw_if.h nw.c nw_service.c \
	sac_if.h \
	smcg_if.h smcg.c \
	wccg_if.h wccg.c \
	bwccg_if.h bwccg.c \
	twccg_if.h twccg.c \
	dckcg_if.h dckcg.c \
	crccg_if.h crccg.c \
	sacg_if.h sacg.c \
	doa_if.h doa.c \
	doac_if.h doac.c \
	doacg_if.h doacg.c \
	sacc_if.h sacc.c \
	saccg_if.h saccg.c \
	ppc.c ppc_if.h \
	ppcg.c ppcg_if.h \
	sdsc.c sdsc_if.h \
	sdscg.c sdscg_if.h \
	drwc.c drwc_if.h \
	drwg_if.h drwg.c \
	drwcg.c drwcg_if.h \
	drc_if.h drc.c \
	drcc_if.h drcc.c \
	drccg_if.h drccg.c \
	umpk_drc_if.h umpk_drc.c \
	snap_if.h snap.c \
	snapg_if.h snapg.c \
	server_conf.c \
	server.h \
	mrw_rn_if.h \
	mrw_rn.c \
	mrw_wn_if.h \
	mrw_wn.c \
	mrw_wsn_if.h \
	mrw_wsn.c \

cse_mds_cbn.h:
	ln -f -s $(MM_BASEDIR)/cse_mds_cbn.h cse_mds_cbn.h

cse_mds_cbn.c:
	ln -f -s $(MM_BASEDIR)/cse_mds_cbn.c cse_mds_cbn.c

nse_mds_cbn.h:
	ln -f -s $(MM_BASEDIR)/nse_mds_cbn.h nse_mds_cbn.h

nse_mds_cbn.c:
	ln -f -s $(MM_BASEDIR)/nse_mds_cbn.c nse_mds_cbn.c

nse_mdsn.h:
	ln -f -s $(MM_BASEDIR)/nse_mdsn.h nse_mdsn.h

nse_mdsn.c:
	ln -f -s $(MM_BASEDIR)/nse_mdsn.c nse_mdsn.c

rc_bwcn.h:
	ln -f -s $(MM_BASEDIR)/rc_bwcn.h rc_bwcn.h

rc_bwcn.c:
	ln -f -s $(MM_BASEDIR)/rc_bwcn.c rc_bwcn.c

rc_twcn.h:
	ln -f -s $(MM_BASEDIR)/rc_twcn.h rc_twcn.h

rc_twcn.c:
	ln -f -s $(MM_BASEDIR)/rc_twcn.c rc_twcn.c

cse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.h cse_crc_cbn.h

cse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.c cse_crc_cbn.c

nse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.h nse_crc_cbn.h

nse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.c nse_crc_cbn.c

rc_bwc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_bwc_cbn.h rc_bwc_cbn.h

rc_bwc_cbn.c:
	ln -f -s $(MM_BASEDIR)/rc_bwc_cbn.c rc_bwc_cbn.c

rc_twc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_twc_cbn.h rc_twc_cbn.h

rc_twc_cbn.c:
	ln -f -s $(MM_BASEDIR)/rc_twc_cbn.c rc_twc_cbn.c

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h
	
allocator.c:
	ln -f -s $(MM_BASEDIR)/allocator.c allocator.c

atomic.h:
	ln -f -s $(BASEDIR)/atomic.h atomic.h

bfe_if.h:
	ln -f -s $(BASEDIR)/bfe_if.h bfe_if.h
	
bfe.c:
	ln -f -s $(BASEDIR)/bfe.c bfe.c

bfea_if.h:
	ln -f -s $(BASEDIR)/bfea_if.h bfea_if.h

bfea.c:
	ln -f -s $(BASEDIR)/bfea.c bfea.c

bfes_if.h:
	ln -f -s $(BASEDIR)/bfes_if.h bfes_if.h

bfes.c:
	ln -f -s $(BASEDIR)/bfes.c bfes.c

bfp_if.h:
	ln -f -s $(BASEDIR)/bfp_if.h bfp_if.h
	
bfp.c:
	ln -f -s $(BASEDIR)/bfp.c bfp.c
	
bfp1_if.h:
	ln -f -s $(BASEDIR)/bfp1_if.h bfp1_if.h

bfp1.c:
	ln -f -s $(BASEDIR)/bfp1.c bfp1.c	

bfp2_if.h:
	ln -f -s $(BASEDIR)/bfp2_if.h bfp2_if.h

bfp2.c:
	ln -f -s $(BASEDIR)/bfp2.c bfp2.c

bfp3_if.h:
	ln -f -s $(BASEDIR)/bfp3_if.h bfp3_if.h

bfp3.c:
	ln -f -s $(BASEDIR)/bfp3.c bfp3.c

bio_if.h:
	ln -f -s $(BASEDIR)/bio_if.h bio_if.h
	
bio.c:
	ln -f -s $(BASEDIR)/bio.c bio.c

bit_if.h:
	ln -f -s $(BASEDIR)/bit_if.h bit_if.h
	
bit.c:
	ln -f -s $(BASEDIR)/bit.c bit.c
	
blksn_if.h:
	ln -f -s $(MM_BASEDIR)/blksn_if.h blksn_if.h
	
blksn.c:
	ln -f -s $(MM_BASEDIR)/blksn.c blksn.c
	
bre_if.h:
	ln -f -s $(BASEDIR)/bre_if.h bre_if.h
	
bre.c:
	ln -f -s $(BASEDIR)/bre.c bre.c
	
brn_if.h:
	ln -f -s $(MM_BASEDIR)/brn_if.h brn_if.h
	
brn.c:
	ln -f -s $(MM_BASEDIR)/brn.c brn.c

bse_if.h:
	ln -f -s $(BASEDIR)/bse_if.h bse_if.h
	
bse.c:
	ln -f -s $(BASEDIR)/bse.c bse.c

bsn_if.h:
	ln -f -s $(MM_BASEDIR)/bsn_if.h bsn_if.h
	
bsn.c:
	ln -f -s $(MM_BASEDIR)/bsn.c bsn.c

btn_if.h:
	ln -f -s $(MM_BASEDIR)/btn_if.h btn_if.h
	
btn.c:
	ln -f -s $(MM_BASEDIR)/btn.c btn.c

cb.h:
	ln -f -s $(BASEDIR)/../mds/src/include/cb.h cb.h

cdn_if.h:
	ln -f -s $(MM_BASEDIR)/cdn_if.h cdn_if.h
	
cdn.c:
	ln -f -s $(MM_BASEDIR)/cdn.c cdn.c

cds_if.h:
	ln -f -s $(BASEDIR)/cds_if.h cds_if.h
	
cds.c:
	ln -f -s $(BASEDIR)/cds.c cds.c
	
crc_if.h:
	ln -f -s $(BASEDIR)/crc_if.h crc_if.h
	
crc.c:
	ln -f -s $(BASEDIR)/crc.c crc.c	

crcc_if.h:
	ln -f -s $(BASEDIR)/crcc_if.h crcc_if.h
	
crcc.c:
	ln -f -s $(BASEDIR)/crcc.c crcc.c

crcg_if.h:
	ln -f -s $(BASEDIR)/crcg_if.h crcg_if.h

crcg.c:
	ln -f -s $(BASEDIR)/crcg.c crcg.c

crccg_if.h:
	ln -f -s $(BASEDIR)/crccg_if.h crccg_if.h
	
crccg.c:
	ln -f -s $(BASEDIR)/crccg.c crccg.c

cse_if.h:
	ln -f -s $(BASEDIR)/cse_if.h cse_if.h
	
cse.c:
	ln -f -s $(BASEDIR)/cse.c cse.c

cxa_if.h:
	ln -f -s $(TX_BASEDIR)/cxa_if.h cxa_if.h

cxa.c:
	ln -f -s $(TX_BASEDIR)/cxa.c cxa.c

cxag_if.h:
	ln -f -s $(TX_BASEDIR)/cxag_if.h cxag_if.h

cxag.c:
	ln -f -s $(TX_BASEDIR)/cxag.c cxag.c

cxp_if.h:
	ln -f -s $(TX_BASEDIR)/cxp_if.h cxp_if.h

cxp.c:
	ln -f -s $(TX_BASEDIR)/cxp.c cxp.c

cxpg_if.h:
	ln -f -s $(TX_BASEDIR)/cxpg_if.h cxpg_if.h

cxpg.c:
	ln -f -s $(TX_BASEDIR)/cxpg.c cxpg.c
	
d1an_if.h:
	ln -f -s $(MM_BASEDIR)/d1an_if.h d1an_if.h
	
d1an.c:
	ln -f -s $(MM_BASEDIR)/d1an.c d1an.c
	
d2an_if.h:
	ln -f -s $(MM_BASEDIR)/d2an_if.h d2an_if.h
	
d2an.c:
	ln -f -s $(MM_BASEDIR)/d2an.c d2an.c

d2bn_if.h:
	ln -f -s $(MM_BASEDIR)/d2bn_if.h d2bn_if.h
	
d2bn.c:
	ln -f -s $(MM_BASEDIR)/d2bn.c d2bn.c

d2cn_if.h:
	ln -f -s $(MM_BASEDIR)/d2cn_if.h d2cn_if.h
	
d2cn.c:
	ln -f -s $(MM_BASEDIR)/d2cn.c d2cn.c
	
ddn_if.h:
	ln -f -s $(MM_BASEDIR)/ddn_if.h ddn_if.h
	
ddn.c:
	ln -f -s $(MM_BASEDIR)/ddn.c ddn.c

doa_if.h:
	ln -f -s $(BASEDIR)/doa_if.h doa_if.h
	
doa.c:
	ln -f -s $(BASEDIR)/doa.c doa.c

doac_if.h:
	ln -f -s $(BASEDIR)/doac_if.h doac_if.h
	
doac.c:
	ln -f -s $(BASEDIR)/doac.c doac.c

doacg_if.h:
	ln -f -s $(BASEDIR)/doacg_if.h doacg_if.h
	
doacg.c:
	ln -f -s $(BASEDIR)/doacg.c doacg.c

drc.c:
	ln -f -s $(BASEDIR)/drc.c drc.c

drc_if.h:
	ln -f -s $(BASEDIR)/drc_if.h drc_if.h

drcc.c:
	ln -f -s $(BASEDIR)/drcc.c drcc.c

drcc_if.h:
	ln -f -s $(BASEDIR)/drcc_if.h drcc_if.h	

drccg.c:
	ln -f -s $(BASEDIR)/drccg.c drccg.c

drccg_if.h:
	ln -f -s $(BASEDIR)/drccg_if.h drccg_if.h
	
sacc.c:
	ln -f -s $(BASEDIR)/sacc.c sacc.c

sacc_if.h:
	ln -f -s $(BASEDIR)/sacc_if.h sacc_if.h

saccg.c:
	ln -f -s $(BASEDIR)/saccg.c saccg.c

saccg_if.h:
	ln -f -s $(BASEDIR)/saccg_if.h saccg_if.h

ppc.c:
	ln -f -s $(BASEDIR)/ppc.c ppc.c

ppc_if.h:
	ln -f -s $(BASEDIR)/ppc_if.h ppc_if.h

ppcg.c:
	ln -f -s $(BASEDIR)/ppcg.c ppcg.c

ppcg_if.h:
	ln -f -s $(BASEDIR)/ppcg_if.h ppcg_if.h

sdsc.c:
	ln -f -s $(BASEDIR)/sdsc.c sdsc.c

sdsc_if.h:
	ln -f -s $(BASEDIR)/sdsc_if.h sdsc_if.h

sdscg.c:
	ln -f -s $(BASEDIR)/sdscg.c sdscg.c

sdscg_if.h:
	ln -f -s $(BASEDIR)/sdscg_if.h sdscg_if.h

drwc.c:
	ln -f -s $(BASEDIR)/drwc.c drwc.c

drwc_if.h:
	ln -f -s $(BASEDIR)/drwc_if.h drwc_if.h

drwg_if.h:
	ln -f -s $(BASEDIR)/drwg_if.h drwg_if.h

drwg.c:
	ln -f -s $(BASEDIR)/drwg.c drwg.c

drwcg.c:
	ln -f -s $(BASEDIR)/drwcg.c drwcg.c

drwcg_if.h:
	ln -f -s $(BASEDIR)/drwcg_if.h drwcg_if.h

drw_if.h:
	ln -f -s $(BASEDIR)/drw_if.h drw_if.h
	
dsbmp_if.h:
	ln -f -s $(BASEDIR)/dsbmp_if.h dsbmp_if.h
	
dsbmp.c:
	ln -f -s $(BASEDIR)/dsbmp.c dsbmp.c

ds_if.h:
	ln -f -s $(BASEDIR)/ds_if.h ds_if.h
	
ds.c:
	ln -f -s $(BASEDIR)/ds.c ds.c	

dsmgr_if.h:
	ln -f -s $(BASEDIR)/dsmgr_if.h dsmgr_if.h
	
dsmgr.c:
	ln -f -s $(BASEDIR)/dsmgr.c dsmgr.c
	
dsrec_if.h:
	ln -f -s $(BASEDIR)/dsrec_if.h dsrec_if.h
	
dsrec.c:
	ln -f -s $(BASEDIR)/dsrec.c dsrec.c	

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h
	
fpn.c:
	ln -f -s $(MM_BASEDIR)/fpn.c fpn.c

fpc_if.h:
	ln -f -s $(BASEDIR)/fpc_if.h fpc_if.h
	
fpc.c:
	ln -f -s $(BASEDIR)/fpc.c fpc.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h
	
ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

ioflag.h:
	ln -f -s $(BASEDIR)/../mds/src/include/ioflag.h ioflag.h
	
io_if.h:
	ln -f -s $(BASEDIR)/io_if.h io_if.h
	
io.c:
	ln -f -s $(BASEDIR)/io.c io.c	

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h
	
lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

list_sort.h:
	ln -f -s $(BASEDIR)/list_sort.h list_sort.h

list_sort.c:
	ln -f -s $(BASEDIR)/list_sort.c list_sort.c	
	
lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h
	
lstn.c:
	ln -f -s $(MM_BASEDIR)/lstn.c lstn.c

mpk.c:
	ln -f -s $(BASEDIR)/../shared/mpk.c mpk.c

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mpk_subr.c:
	ln -f -s $(BASEDIR)/../shared/mpk_subr.c mpk_subr.c 
	
mrw_if.h:
	ln -f -s $(BASEDIR)/mrw_if.h mrw_if.h
	
mrw.c:
	ln -f -s $(BASEDIR)/mrw.c mrw.c

mrwc_if.h:
	ln -f -s $(BASEDIR)/mrwc_if.h mrwc_if.h
	
mrwc.c:
	ln -f -s $(BASEDIR)/mrwc.c mrwc.c

mrwcg_if.h:
	ln -f -s $(BASEDIR)/mrwcg_if.h mrwcg_if.h
	
mrwcg.c:
	ln -f -s $(BASEDIR)/mrwcg.c mrwcg.c

mrw_rn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_rn_if.h mrw_rn_if.h

mrw_rn.c:
	ln -f -s $(MM_BASEDIR)/mrw_rn.c mrw_rn.c

mrw_wn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_wn_if.h mrw_wn_if.h

mrw_wn.c:
	ln -f -s $(MM_BASEDIR)/mrw_wn.c mrw_wn.c

mrw_wsn_if.h:
	ln -f -s $(MM_BASEDIR)/mrw_wsn_if.h mrw_wsn_if.h

mrw_wsn.c:
	ln -f -s $(MM_BASEDIR)/mrw_wsn.c mrw_wsn.c

ms_intf.h:	
	ln -f -s $(BASEDIR)/../mds/src/include/ms_intf.h ms_intf.h

msret.h:
	ln -f -s $(BASEDIR)/../mds/src/include/msret.h msret.h
	
bwc_if.h:
	ln -f -s $(BASEDIR)/bwc_if.h bwc_if.h
	
bwc.c:
	ln -f -s $(BASEDIR)/bwc.c bwc.c
	
bwcg_if.h:
	ln -f -s $(BASEDIR)/bwcg_if.h bwcg_if.h
	
bwcg.c:
	ln -f -s $(BASEDIR)/bwcg.c bwcg.c
	
twc_if.h:
	ln -f -s $(BASEDIR)/twc_if.h twc_if.h

twc.c:
	ln -f -s $(BASEDIR)/twc.c twc.c

twcg_if.h:
	ln -f -s $(BASEDIR)/twcg_if.h twcg_if.h

twcg.c:
	ln -f -s $(BASEDIR)/twcg.c twcg.c


dck_if.h:
	ln -f -s $(BASEDIR)/dck_if.h dck_if.h

dck.c:
	ln -f -s $(BASEDIR)/dck.c dck.c

bwccg_if.h:
	ln -f -s $(BASEDIR)/bwccg_if.h bwccg_if.h
	
bwccg.c:
	ln -f -s $(BASEDIR)/bwccg.c bwccg.c

twccg_if.h:
	ln -f -s $(BASEDIR)/twccg_if.h twccg_if.h

twccg.c:
	ln -f -s $(BASEDIR)/twccg.c twccg.c

dckcg_if.h:
	ln -f -s $(BASEDIR)/dckcg_if.h dckcg_if.h

dckcg.c:
	ln -f -s $(BASEDIR)/dckcg.c dckcg.c
	
bwcc_if.h:
	ln -f -s $(BASEDIR)/bwcc_if.h bwcc_if.h
	
bwcc.c:
	ln -f -s $(BASEDIR)/bwcc.c bwcc.c

twcc_if.h:
	ln -f -s $(BASEDIR)/twcc_if.h twcc_if.h

twcc.c:
	ln -f -s $(BASEDIR)/twcc.c twcc.c

dckc_if.h:
	ln -f -s $(BASEDIR)/dckc_if.h dckc_if.h

dckc.c:
	ln -f -s $(BASEDIR)/dckc.c dckc.c

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h
	
node.c:
	ln -f -s $(MM_BASEDIR)/node.c node.c
	
nse_if.h:
	ln -f -s $(BASEDIR)/nse_if.h nse_if.h
	
nse.c:
	ln -f -s $(BASEDIR)/nse.c nse.c

nw_if.h:
	ln -f -s $(BASEDIR)/nw_if.h nw_if.h

nw.h:
	ln -f -s $(BASEDIR)/nw.h nw.h

nw.c:
	ln -f -s $(BASEDIR)/nw.c nw.c

nw_service.c:
	ln -f -s $(BASEDIR)/nw_service.c nw_service.c	

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(MM_BASEDIR)/pp2.c pp2.c

ppg_if.h:
	ln -f -s $(BASEDIR)/ppg_if.h ppg_if.h

ppg.c:
	ln -f -s $(BASEDIR)/ppg.c ppg.c

pub.h:	
	ln -f -s $(BASEDIR)/../mds/src/include/pub.h pub.h
	
rc_if.h:
	ln -f -s $(BASEDIR)/rc_if.h rc_if.h
	
rc.c:
	ln -f -s $(BASEDIR)/rc.c rc.c
		
rio_if.h:
	ln -f -s $(BASEDIR)/rio_if.h rio_if.h
	
rio.c:
	ln -f -s $(BASEDIR)/rio.c rio.c		

rtn_if.h:
	ln -f -s $(MM_BASEDIR)/rtn_if.h rtn_if.h
	
rtn.c:
	ln -f -s $(MM_BASEDIR)/rtn.c rtn.c

rtree_if.h:
	ln -f -s $(BASEDIR)/rtree_if.h rtree_if.h
	
rtree.c:
	ln -f -s $(BASEDIR)/rtree.c rtree.c
	
rw_if.h:
	ln -f -s $(BASEDIR)/rw_if.h rw_if.h
	
rw.c:
	ln -f -s $(BASEDIR)/rw.c rw.c	

sac_if.h:
	ln -f -s $(BASEDIR)/sac_if.h sac_if.h
	
	
sacg_if.h:
	ln -f -s $(BASEDIR)/sacg_if.h sacg_if.h
	
sacg.c:
	ln -f -s $(BASEDIR)/sacg.c sacg.c

sah_if.h:
	ln -f -s $(BASEDIR)/sah_if.h sah_if.h
	
sah.c:
	ln -f -s $(BASEDIR)/sah.c sah.c	
	
scg_if.h:
	ln -f -s $(BASEDIR)/scg_if.h scg_if.h
	
scg.c:
	ln -f -s $(BASEDIR)/scg.c scg.c		

sds_if.h:
	ln -f -s $(BASEDIR)/sds_if.h sds_if.h
	
sds.c:
	ln -f -s $(BASEDIR)/sds.c sds.c

sdsg_if.h:
	ln -f -s $(BASEDIR)/sdsg_if.h sdsg_if.h
	
sdsg.c:
	ln -f -s $(BASEDIR)/sdsg.c sdsg.c

cdsg_if.h:
	ln -f -s $(BASEDIR)/cdsg_if.h cdsg_if.h

cdsg.c:
	ln -f -s $(BASEDIR)/cdsg.c cdsg.c

dxp_if.h:
	ln -f -s $(TX_BASEDIR)/dxp_if.h dxp_if.h
	
dxp.c:
	ln -f -s $(TX_BASEDIR)/dxp.c dxp.c

dxt_if.h:
	ln -f -s $(TX_BASEDIR)/dxt_if.h dxt_if.h

dxt.c:
	ln -f -s $(TX_BASEDIR)/dxt.c dxt.c

dxpg_if.h:
	ln -f -s $(TX_BASEDIR)/dxpg_if.h dxpg_if.h
	
dxpg.c:
	ln -f -s $(TX_BASEDIR)/dxpg.c dxpg.c

dxpc_if.h:
	ln -f -s $(TX_BASEDIR)/dxpc_if.h dxpc_if.h
	
dxpc.c:
	ln -f -s $(TX_BASEDIR)/dxpc.c dxpc.c

dxpcg_if.h:
	ln -f -s $(TX_BASEDIR)/dxpcg_if.h dxpcg_if.h
	
dxpcg.c:
	ln -f -s $(TX_BASEDIR)/dxpcg.c dxpcg.c

dxa_if.h:
	ln -f -s $(TX_BASEDIR)/dxa_if.h dxa_if.h
	
dxa.c:
	ln -f -s $(TX_BASEDIR)/dxa.c dxa.c

dxag_if.h:
	ln -f -s $(TX_BASEDIR)/dxag_if.h dxag_if.h
	
dxag.c:
	ln -f -s $(TX_BASEDIR)/dxag.c dxag.c

dxac_if.h:
	ln -f -s $(TX_BASEDIR)/dxac_if.h dxac_if.h
	
dxac.c:
	ln -f -s $(TX_BASEDIR)/dxac.c dxac.c

dxacg_if.h:
	ln -f -s $(TX_BASEDIR)/dxacg_if.h dxacg_if.h
	
dxacg.c:
	ln -f -s $(TX_BASEDIR)/dxacg.c dxacg.c

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h
	
umpk_dxa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa.c umpk_dxa.c

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h
	
umpk_dxp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp.c umpk_dxp.c

server_conf.c:
	ln -f -s $(BASEDIR)/server_conf.c server_conf.c

server.h:
	ln -f -s $(BASEDIR)/server.h server.h

smc_if.h:
	ln -f -s $(BASEDIR)/smc_if.h smc_if.h
	
smc.c:
	ln -f -s $(BASEDIR)/smc.c smc.c	

smcg_if.h:
	ln -f -s $(BASEDIR)/smcg_if.h smcg_if.h
	
smcg.c:
	ln -f -s $(BASEDIR)/smcg.c smcg.c

snap_if.h:
	ln -f -s $(BASEDIR)/snap_if.h snap_if.h
	
snap.c:
	ln -f -s $(BASEDIR)/snap.c snap.c

snapg_if.h:
	ln -f -s $(BASEDIR)/snapg_if.h snapg_if.h
	
snapg.c:
	ln -f -s $(BASEDIR)/snapg.c snapg.c
	
srn_if.h:
	ln -f -s $(MM_BASEDIR)/srn_if.h srn_if.h
	
srn.c:
	ln -f -s $(MM_BASEDIR)/srn.c srn.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

umpk_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_if.h umpk_if.h
	
umpk.h:
	ln -f -s $(UMPK_BASEDIR)/umpk.h umpk.h
	
umpk.c:
	ln -f -s $(UMPK_BASEDIR)/umpk.c umpk.c

umpk_bwc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc_if.h umpk_bwc_if.h
	
umpk_bwc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_bwc.c umpk_bwc.c

umpk_twc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc_if.h umpk_twc_if.h

umpk_twc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_twc.c umpk_twc.c

umpk_mrw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_mrw_if.h umpk_mrw_if.h
	
umpk_mrw.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_mrw.c umpk_mrw.c

umpk_dck_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dck_if.h umpk_dck_if.h

umpk_dck.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dck.c umpk_dck.c

umpk_crc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc_if.h umpk_crc_if.h
	
umpk_crc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_crc.c umpk_crc.c

umpk_drc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_drc.c umpk_drc.c

umpk_drc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drc_if.h umpk_drc_if.h

umpk_wc_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc_if.h umpk_wc_if.h
	
umpk_wc.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_wc.c umpk_wc.c

umpk_doa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_doa_if.h umpk_doa_if.h
	
umpk_doa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_doa.c umpk_doa.c

umpk_sac_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sac_if.h umpk_sac_if.h

umpk_sac.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_sac.c umpk_sac.c

umpk_pp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_pp_if.h umpk_pp_if.h

umpk_pp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_pp.c umpk_pp.c

umpk_sds_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_sds_if.h umpk_sds_if.h

umpk_sds.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_sds.c umpk_sds.c

umpk_drw_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_drw_if.h umpk_drw_if.h

umpk_drw.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_drw.c umpk_drw.c

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h
	
util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c
	
vector.h:
	ln -f -s $(BASEDIR)/../mds/src/include/vector.h vector.h

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h
	
wc_if.h:
	ln -f -s $(BASEDIR)/wc_if.h wc_if.h
	
wc.c:
	ln -f -s $(BASEDIR)/wc.c wc.c

wcc_if.h:
	ln -f -s $(BASEDIR)/wcc_if.h wcc_if.h
	
wcc.c:
	ln -f -s $(BASEDIR)/wcc.c wcc.c	
	
wccg_if.h:
	ln -f -s $(BASEDIR)/wccg_if.h wccg_if.h

wccg.c:
	ln -f -s $(BASEDIR)/wccg.c wccg.c
	
	
	
