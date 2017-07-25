SRCS =  util_nw.c umpk.c umpka.c umpk_wc.c umpk_crc.c umpk_dxa.c umpk_dxp.c umpk_cxa.c umpk_cxp.c umpk_doa.c mpk.c umpk_bwc.c umpk_twc.c umpk_mrw.c umpk_sac.c umpk_pp.c umpk_sds.c umpk_drw.c mpk_subr.c umpk_tp.c umpk_ini.c umpk_server.c umpk_agent.c umpk_driver.c\
	msc.c mac.c mgrwc.c mgrbwc.c mgrcrc.c mgrdoa.c mgrmrw.c mgrdx.c mgrcx.c mgrsac.c mgrpp.c mgrsds.c mgrdrw.c mgrtp.c mgrini.c mgrserver.c mgragent.c mgrdriver.c mgrall.c manager.c manager_bio.c manager_doa.c \
	manager_wc.c manager_crc.c manager_bwc.c manager_mrw.c manager_dx.c  manager_cx.c manager_sac.c manager_pp.c manager_sds.c manager_drw.c manager_twc.c mgrtwc.c pp2.c lck.c \
	 manager_tp.c manager_ini.c tx_shared.c manager_server.c manager_agent.c manager_driver.c manager_all.c

SYMLINKS = version.h list.h util_nw.h util_nw.c nid_shared.h nid.h nid_log.h allocator_if.h \
	dsm_if.h \
	mpk_if.h umpk_if.h mpk.c  mpk_subr.c \
	umpk.h umpk_if.h umpk.c \
	umpka.h umpka_if.h umpka.c \
	umpk_wc_if.h umpk_wc.c \
	umpk_crc_if.h umpk_crc.c \
	umpk_bwc_if.h umpk_bwc.c \
	umpk_twc_if.h umpk_twc.c \
	umpk_dxa_if.h umpk_dxa.c \
	umpk_dxp_if.h umpk_dxp.c \
	umpk_cxa_if.h umpk_cxa.c \
	umpk_cxp_if.h umpk_cxp.c \
	umpk_mrw_if.h umpk_mrw.c \
	umpk_doa_if.h umpk_doa.c \
	umpk_sac_if.h umpk_sac.c \
	umpk_pp_if.h umpk_pp.c \
	umpk_sds_if.h umpk_sds.c \
	umpk_drw_if.h umpk_drw.c \
	umpk_tp_if.h umpk_tp.c \
	umpk_ini_if.h umpk_ini.c \
	umpk_server_if.h umpk_server.c\
	umpk_agent_if.h umpk_agent.c \
	umpk_driver_if.h umpk_driver.c \
	msc_if.h msc.c \
	mac_if.h mac.c \
	mgrwc_if.h mgrwc.c \
	mgrdx_if.h mgrdx.c \
	mgrcx_if.h mgrcx.c \
	mgrbwc_if.h mgrbwc.c \
	mgrtwc_if.h mgrtwc.c \
	mgrmrw_if.h mgrmrw.c \
	mgrcrc_if.h mgrcrc.c \
	mgrdoa_if.h mgrdoa.c \
	mgrsac_if.h mgrsac.c \
	mgrpp_if.h mgrpp.c \
	mgrsds_if.h mgrsds.c \
	mgrdrw_if.h mgrdrw.c \
	mgrtp_if.h mgrtp.c \
	mgrini_if.h mgrini.c \
	mgrserver_if.h mgrserver.c \
	mgragent_if.h mgragent.c \
	mgrdriver_if.h mgrdriver.c \
	mgrall_if.h mgrall.c \
	manager.h manager.c \
	manager_bwc.c manager_mrw.c manager_sac.c manager_pp.c manager_sds.c \
	manager_drw.c manager_bio.c manager_doa.c manager_wc.c manager_crc.c manager_twc.c manager_dx.c manager_cx.c \
	manager_tp.c manager_ini.c manager_server.c manager_agent.c manager_driver.c\
	manager_all.c \
	pp2_if.h pp2.c \
	lstn_if.h \
	node_if.h \
	lck_if.h lck.c \
	tx_shared.h tx_shared.c

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h

dsm_if.h:
	ln -f -s $(BASEDIR)/dsm_if.h dsm_if.h

msc_if.h:
	ln -f -s $(BASEDIR)/msc_if.h msc_if.h

manager_bwc.c:
	ln -f -s $(BASEDIR)/manager/manager_bwc.c manager_bwc.c

manager_mrw.c:
	ln -f -s $(BASEDIR)/manager/manager_mrw.c manager_mrw.c

manager_sac.c:
	ln -f -s $(BASEDIR)/manager/manager_sac.c manager_sac.c

manager_pp.c:
	ln -f -s $(BASEDIR)/manager/manager_pp.c manager_pp.c

manager_sds.c:
	ln -f -s $(BASEDIR)/manager/manager_sds.c manager_sds.c

manager_drw.c:
	ln -f -s $(BASEDIR)/manager/manager_drw.c manager_drw.c

manager_tp.c:
	ln -f -s $(BASEDIR)/manager/manager_tp.c manager_tp.c

mgrwc_if.h:
	ln -f -s $(BASEDIR)/manager/mgrwc_if.h mgrwc_if.h

mgrwc.c:
	ln -f -s $(BASEDIR)/manager/mgrwc.c mgrwc.c

mgrbwc_if.h:
	ln -f -s $(BASEDIR)/manager/mgrbwc_if.h mgrbwc_if.h

mgrbwc.c:
	ln -f -s $(BASEDIR)/manager/mgrbwc.c mgrbwc.c

mgrdx_if.h:
	ln -f -s $(BASEDIR)/manager/mgrdx_if.h mgrdx_if.h

mgrdx.c:
	ln -f -s $(BASEDIR)/manager/mgrdx.c mgrdx.c

mgrcx_if.h:
	ln -f -s $(BASEDIR)/manager/mgrcx_if.h mgrcx_if.h

mgrcx.c:
	ln -f -s $(BASEDIR)/manager/mgrcx.c mgrcx.c

mgrtwc_if.h:
	ln -f -s $(BASEDIR)/manager/mgrtwc_if.h mgrtwc_if.h

mgrtwc.c:
	ln -f -s $(BASEDIR)/manager/mgrtwc.c mgrtwc.c

mgrmrw_if.h:
	ln -f -s $(BASEDIR)/manager/mgrmrw_if.h mgrmrw_if.h

mgrmrw.c:
	ln -f -s $(BASEDIR)/manager/mgrmrw.c mgrmrw.c

mgrcrc_if.h:
	ln -f -s $(BASEDIR)/manager/mgrcrc_if.h mgrcrc_if.h

mgrcrc.c:
	ln -f -s $(BASEDIR)/manager/mgrcrc.c mgrcrc.c

mgrsac_if.h:
	ln -f -s $(BASEDIR)/manager/mgrsac_if.h mgrsac_if.h

mgrsac.c:
	ln -f -s $(BASEDIR)/manager/mgrsac.c mgrsac.c

mgrpp_if.h:
	ln -f -s $(BASEDIR)/manager/mgrpp_if.h mgrpp_if.h

mgrpp.c:
	ln -f -s $(BASEDIR)/manager/mgrpp.c mgrpp.c

mgrsds_if.h:
	ln -f -s $(BASEDIR)/manager/mgrsds_if.h mgrsds_if.h

mgrsds.c:
	ln -f -s $(BASEDIR)/manager/mgrsds.c mgrsds.c

mgrdrw_if.h:
	ln -f -s $(BASEDIR)/manager/mgrdrw_if.h mgrdrw_if.h

mgrdrw.c:
	ln -f -s $(BASEDIR)/manager/mgrdrw.c mgrdrw.c

mgrtp.c:
	ln -f -s $(BASEDIR)/manager/mgrtp.c mgrtp.c

mgrtp_if.h:
	ln -f -s $(BASEDIR)/manager/mgrtp_if.h mgrtp_if.h

mgrini.c:
	ln -f -s $(BASEDIR)/manager/mgrini.c mgrini.c

mgrini_if.h:
	ln -f -s $(BASEDIR)/manager/mgrini_if.h mgrini_if.h

mgrserver.c:
	ln -f -s $(BASEDIR)/manager/mgrserver.c mgrserver.c

mgrserver_if.h:
	ln -f -s $(BASEDIR)/manager/mgrserver_if.h mgrserver_if.h

mgragent.c:
	ln -f -s $(BASEDIR)/manager/mgragent.c mgragent.c

mgragent_if.h:
	ln -f -s $(BASEDIR)/manager/mgragent_if.h mgragent_if.h

mgrdriver.c:
	ln -f -s $(BASEDIR)/manager/mgrdriver.c mgrdriver.c

mgrdriver_if.h:
	ln -f -s $(BASEDIR)/manager/mgrdriver_if.h mgrdriver_if.h

mgrall.c:
	ln -f -s $(BASEDIR)/manager/mgrall.c mgrall.c

mgrall_if.h:
	ln -f -s $(BASEDIR)/manager/mgrall_if.h mgrall_if.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mpk.c:
	ln -f -s $(BASEDIR)/../shared/mpk.c mpk.c

mpk_subr.c:
	ln -f -s $(BASEDIR)/../shared/mpk_subr.c mpk_subr.c 

umpk.h:
	ln -f -s $(BASEDIR)/umpk/umpk.h umpk.h

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_if.h umpk_if.h

umpk.c:
	ln -f -s $(BASEDIR)/umpk/umpk.c umpk.c

umpka.h:
	ln -f -s $(BASEDIR)/umpk/umpka.h umpka.h

umpka_if.h:
	ln -f -s $(BASEDIR)/umpk/umpka_if.h umpka_if.h

umpka.c:
	ln -f -s $(BASEDIR)/umpk/umpka.c umpka.c

umpk_wc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_wc_if.h umpk_wc_if.h

umpk_wc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_wc.c umpk_wc.c

umpk_dxa_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_dxa_if.h umpk_dxa_if.h

umpk_dxa.c:
	ln -f -s $(BASEDIR)/umpk/umpk_dxa.c umpk_dxa.c

umpk_dxp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_dxp_if.h umpk_dxp_if.h

umpk_dxp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_dxp.c umpk_dxp.c

umpk_cxa_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_cxa_if.h umpk_cxa_if.h

umpk_cxa.c:
	ln -f -s $(BASEDIR)/umpk/umpk_cxa.c umpk_cxa.c

umpk_cxp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_cxp_if.h umpk_cxp_if.h

umpk_cxp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_cxp.c umpk_cxp.c

umpk_crc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_crc_if.h umpk_crc_if.h

umpk_crc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_crc.c umpk_crc.c

umpk_bwc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_bwc_if.h umpk_bwc_if.h

umpk_bwc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_bwc.c umpk_bwc.c

umpk_twc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_twc_if.h umpk_twc_if.h

umpk_twc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_twc.c umpk_twc.c

umpk_mrw_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_mrw_if.h umpk_mrw_if.h

umpk_mrw.c:
	ln -f -s $(BASEDIR)/umpk/umpk_mrw.c umpk_mrw.c

umpk_doa_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_doa_if.h umpk_doa_if.h

umpk_doa.c:
	ln -f -s $(BASEDIR)/umpk/umpk_doa.c umpk_doa.c

umpk_sac.c:
	ln -f -s $(BASEDIR)/umpk/umpk_sac.c umpk_sac.c

umpk_sac_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_sac_if.h umpk_sac_if.h

umpk_pp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_pp.c umpk_pp.c

umpk_pp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_pp_if.h umpk_pp_if.h

umpk_sds.c:
	ln -f -s $(BASEDIR)/umpk/umpk_sds.c umpk_sds.c

umpk_sds_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_sds_if.h umpk_sds_if.h

umpk_drw.c:
	ln -f -s $(BASEDIR)/umpk/umpk_drw.c umpk_drw.c

umpk_drw_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_drw_if.h umpk_drw_if.h

umpk_tp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_tp.c umpk_tp.c

umpk_tp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_tp_if.h umpk_tp_if.h

umpk_ini.c:
	ln -f -s $(BASEDIR)/umpk/umpk_ini.c umpk_ini.c

umpk_ini_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_ini_if.h umpk_ini_if.h

umpk_server.c:
	ln -f -s $(BASEDIR)/umpk/umpk_server.c umpk_server.c

umpk_server_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_server_if.h umpk_server_if.h

umpk_agent.c:
	ln -f -s $(BASEDIR)/umpk/umpk_agent.c umpk_agent.c

umpk_agent_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_agent_if.h umpk_agent_if.h

umpk_driver.c:
	ln -f -s $(BASEDIR)/umpk/umpk_driver.c umpk_driver.c

umpk_driver_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_driver_if.h umpk_driver_if.h

msc.c:
	ln -f -s $(BASEDIR)/msc.c msc.c

mac_if.h:
	ln -f -s $(BASEDIR)/mac_if.h mac_if.h

mac.c:
	ln -f -s $(BASEDIR)/mac.c mac.c

mgrdoa_if.h:
	ln -f -s $(BASEDIR)/manager/mgrdoa_if.h mgrdoa_if.h

mgrdoa.c:
	ln -f -s $(BASEDIR)/manager/mgrdoa.c mgrdoa.c

manager.h:
	ln -f -s $(BASEDIR)/manager/manager.h manager.h

manager.c:
	ln -f -s $(BASEDIR)/manager/manager.c manager.c

manager_bio.c:
	ln -f -s $(BASEDIR)/manager/manager_bio.c manager_bio.c

manager_doa.c:
	ln -f -s $(BASEDIR)/manager/manager_doa.c manager_doa.c

manager_wc.c:
	ln -f -s $(BASEDIR)/manager/manager_wc.c manager_wc.c

manager_crc.c:
	ln -f -s $(BASEDIR)/manager/manager_crc.c manager_crc.c

manager_dx.c:
	ln -f -s $(BASEDIR)/manager/manager_dx.c manager_dx.c

manager_cx.c:
	ln -f -s $(BASEDIR)/manager/manager_cx.c manager_cx.c

manager_twc.c:
	ln -f -s $(BASEDIR)/manager/manager_twc.c manager_twc.c

manager_ini.c:
	ln -f -s $(BASEDIR)/manager/manager_ini.c manager_ini.c

manager_server.c:
	ln -f -s $(BASEDIR)/manager/manager_server.c manager_server.c

manager_agent.c:
	ln -f -s $(BASEDIR)/manager/manager_agent.c manager_agent.c

manager_driver.c:
	ln -f -s $(BASEDIR)/manager/manager_driver.c manager_driver.c

manager_all.c:
	ln -f -s $(BASEDIR)/manager/manager_all.c manager_all.c

pp2_if.h:
	ln -f -s $(BASEDIR)/mm/pp2_if.h pp2_if.h

pp2.c:
	ln -f -s $(BASEDIR)/mm/pp2.c pp2.c

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h

lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

tx_shared.h:
	ln -f -s $(BASEDIR)/tx/tx_shared.h tx_shared.h

tx_shared.c:
	ln -f -s $(BASEDIR)/tx/tx_shared.c tx_shared.c
