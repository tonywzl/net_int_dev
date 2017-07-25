MM_SRC = txn.c cse_mds_cbn.c nse_mds_cbn.c nse_mdsn.c rc_wcn.c cse_crc_cbn.c nse_crc_cbn.c rsn.c \
	 rc_wc_cbn.c allocator.c blksn.c brn.c bsn.c btn.c cdn.c d1an.c d2an.c d2bn.c d2cn.c \
	ddn.c fpn.c lstn.c mrw_rn.c mrw_wn.c mrw_wsn.c node.c pp.c pp2.c rtn.c srn.c ppg.c ppc.c ppcg.c \
	bwc_rbn.c rsun.c rdwn.c spn.c rsugn.c

MM_SYMLINKS = txn_if.h txn.c \
	cse_mds_cbn.h cse_mds_cbn.c \
	nse_mds_cbn.h nse_mds_cbn.c \
	nse_mdsn.h nse_mdsn.c \
	rc_wcn.h rc_wcn.c \
	rsn_if.h rsn.c \
	cse_crc_cbn.h cse_crc_cbn.c \
	nse_crc_cbn.h nse_crc_cbn.c \
	rc_wc_cbn.h rc_wc_cbn.c \
	allocator_if.h allocator.c \
	blksn_if.h blksn.c \
	brn_if.h brn.c \
	bsn_if.h bsn.c \
	btn_if.h btn.c \
	cdn_if.h cdn.c \
	d1an_if.h d1an.c \
	d2an_if.h d2an.c \
	d2bn_if.h d2bn.c \
	d2cn_if.h d2cn.c \
	ddn_if.h ddn.c \
	fpn_if.h fpn.c \
	lstn_if.h lstn.c \
	mrw_rn_if.h mrw_rn.c \
	mrw_wn_if.h mrw_wn.c \
	mrw_wsn_if.h mrw_wsn.c \
	node_if.h node.c pp_if.h pp.c \
	spn_if.h spn.c \
	pp2_if.h pp2.c \
	rtn_if.h rtn.c \
	srn_if.h srn.c \
	ppg_if.h ppg.c \
	ppc.c ppc_if.h \
	rsun_if.h rsun.c rsn_if.h \
	rdwn_if.h rdwn.c \
	ppcg.c ppcg_if.h \
	bwc_rbn.c bwc_rbn_if.h \
	rsugn.c rsugn_if.h \

rsun_if.h:
	ln -f -s $(MM_BASEDIR)/rsun_if.h rsun_if.h

rsun.c:
	ln -f -s $(MM_BASEDIR)/rsun.c rsun.c

rsn_if.h:
	ln -f -s $(MM_BASEDIR)/rsn_if.h rsn_if.h

rdwn_if.h:
	ln -f -s $(MM_BASEDIR)/rdwn_if.h rdwn_if.h

rdwn.c:
	ln -f -s $(MM_BASEDIR)/rdwn.c rdwn.c

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

rc_wcn.h:
	ln -f -s $(MM_BASEDIR)/rc_wcn.h rc_wcn.h

rc_wcn.c:
	ln -f -s $(MM_BASEDIR)/rc_wcn.c rc_wcn.c

rsn.c:
	ln -f -s $(MM_BASEDIR)/rsn.c rsn.c

cse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.h cse_crc_cbn.h

cse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/cse_crc_cbn.c cse_crc_cbn.c

nse_crc_cbn.h:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.h nse_crc_cbn.h

nse_crc_cbn.c:
	ln -f -s $(MM_BASEDIR)/nse_crc_cbn.c nse_crc_cbn.c

rc_wc_cbn.h:
	ln -f -s $(MM_BASEDIR)/rc_wc_cbn.h rc_wc_cbn.h

rc_wc_cbn.c:
	ln -f -s $(MM_BASEDIR)/rc_wc_cbn.c rc_wc_cbn.c

allocator_if.h:
	ln -f -s $(MM_BASEDIR)/allocator_if.h allocator_if.h
	
allocator.c:
	ln -f -s $(MM_BASEDIR)/allocator.c allocator.c

blksn_if.h:
	ln -f -s $(MM_BASEDIR)/blksn_if.h blksn_if.h
	
blksn.c:
	ln -f -s $(MM_BASEDIR)/blksn.c blksn.c
	
brn_if.h:
	ln -f -s $(MM_BASEDIR)/brn_if.h brn_if.h
	
brn.c:
	ln -f -s $(MM_BASEDIR)/brn.c brn.c

bsn_if.h:
	ln -f -s $(MM_BASEDIR)/bsn_if.h bsn_if.h
	
bsn.c:
	ln -f -s $(MM_BASEDIR)/bsn.c bsn.c

btn_if.h:
	ln -f -s $(MM_BASEDIR)/btn_if.h btn_if.h
	
btn.c:
	ln -f -s $(MM_BASEDIR)/btn.c btn.c

cdn_if.h:
	ln -f -s $(MM_BASEDIR)/cdn_if.h cdn_if.h
	
cdn.c:
	ln -f -s $(MM_BASEDIR)/cdn.c cdn.c

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

fpn_if.h:
	ln -f -s $(MM_BASEDIR)/fpn_if.h fpn_if.h
	
fpn.c:
	ln -f -s $(MM_BASEDIR)/fpn.c fpn.c

txn_if.h:
	ln -f -s $(MM_BASEDIR)/txn_if.h txn_if.h
	
txn.c:
	ln -f -s $(MM_BASEDIR)/txn.c txn.c

lstn_if.h:
	ln -f -s $(MM_BASEDIR)/lstn_if.h lstn_if.h
	
lstn.c:
	ln -f -s $(MM_BASEDIR)/lstn.c lstn.c

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

node_if.h:
	ln -f -s $(MM_BASEDIR)/node_if.h node_if.h

node.c:
	ln -f -s $(MM_BASEDIR)/node.c node.c

spn_if.h:
	ln -f -s $(MM_BASEDIR)/spn_if.h spn_if.h

spn.c:
	ln -f -s $(MM_BASEDIR)/spn.c spn.c

pp_if.h:
	ln -f -s $(MM_BASEDIR)/pp_if.h pp_if.h

pp.c:
	ln -f -s $(MM_BASEDIR)/pp.c pp.c

pp2_if.h:
	ln -f -s $(MM_BASEDIR)/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(MM_BASEDIR)/pp2.c pp2.c

rtn_if.h:
	ln -f -s $(MM_BASEDIR)/rtn_if.h rtn_if.h
	
rtn.c:
	ln -f -s $(MM_BASEDIR)/rtn.c rtn.c

srn_if.h:
	ln -f -s $(MM_BASEDIR)/srn_if.h srn_if.h
	
srn.c:
	ln -f -s $(MM_BASEDIR)/srn.c srn.c

ppc.c:
	ln -f -s $(MM_BASEDIR)/ppc.c ppc.c

ppc_if.h:
	ln -f -s $(MM_BASEDIR)/ppc_if.h ppc_if.h

ppcg.c:
	ln -f -s $(MM_BASEDIR)/ppcg.c ppcg.c

ppcg_if.h:
	ln -f -s $(MM_BASEDIR)/ppcg_if.h ppcg_if.h

ppg_if.h:
	ln -f -s $(MM_BASEDIR)/ppg_if.h ppg_if.h

ppg.c:
	ln -f -s $(MM_BASEDIR)/ppg.c ppg.c

bwc_rbn_if.h:
	ln -f -s $(MM_BASEDIR)/bwc_rbn_if.h bwc_rbn_if.h

bwc_rbn.c:
	ln -f -s $(MM_BASEDIR)/bwc_rbn.c bwc_rbn.c

rsugn_if.h:
	ln -f -s $(MM_BASEDIR)/rsugn_if.h rsugn_if.h

rsugn.c:
	ln -f -s $(MM_BASEDIR)/rsugn.c rsugn.c

