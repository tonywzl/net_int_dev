UMPK_SRC = umpk.c umpk_wc.c umpk_bwc.c umpk_crc.c umpk_doa.c umpk_drc.c umpk_mrw.c umpk_twc.c \
 	umpk_sac.c umpk_pp.c umpk_sds.c umpk_drw.c \
	umpk_cxa.c umpk_cxp.c \
        umpk_dxa.c umpk_dxp.c \
	umpk_tp.c \
	umpk_ini.c \
	umpk_rept.c umpk_reps.c \
	umpk_server.c 

UMPK_SYMLINKS = umpk_if.h umpk.c umpk.h \
        umpk_wc_if.h umpk_wc.c \
        umpk_bwc_if.h umpk_bwc.c \
        umpk_twc_if.h umpk_twc.c \
        umpk_mrw_if.h umpk_mrw.c \
        umpk_crc_if.h umpk_crc.c \
        umpk_doa_if.h umpk_doa.c \
        umpk_sac_if.h umpk_sac.c \
        umpk_pp_if.h umpk_pp.c \
        umpk_sds_if.h umpk_sds.c \
        umpk_drw_if.h umpk_drw.c \
	umpk_drc_if.h umpk_drc.c \
        umpk_cxa_if.h umpk_cxa.c \
        umpk_cxp_if.h umpk_cxp.c \
        umpk_dxa_if.h umpk_dxa.c \
        umpk_dxp_if.h umpk_dxp.c \
	umpk_tp_if.h umpk_tp.c \
	umpk_ini_if.h umpk_ini.c \
	umpk_rept_if.h umpk_rept.c \
	umpk_reps_if.h umpk_reps.c \
	umpk_server_if.h umpk_server.c

umpk_cxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa_if.h umpk_cxa_if.h
	
umpk_cxa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxa.c umpk_cxa.c

umpk_cxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp_if.h umpk_cxp_if.h
	
umpk_cxp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_cxp.c umpk_cxp.c

umpk_dxa_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa_if.h umpk_dxa_if.h
	
umpk_dxa.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxa.c umpk_dxa.c

umpk_dxp_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp_if.h umpk_dxp_if.h
	
umpk_dxp.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_dxp.c umpk_dxp.c

umpk_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_if.h umpk_if.h
	
umpk.h:
	ln -f -s $(BASEDIR)/umpk/umpk.h umpk.h
	
umpk.c:
	ln -f -s $(BASEDIR)/umpk/umpk.c umpk.c

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

umpk_crc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_crc_if.h umpk_crc_if.h
	
umpk_crc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_crc.c umpk_crc.c

umpk_drc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_drc.c umpk_drc.c

umpk_drc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_drc_if.h umpk_drc_if.h

umpk_wc_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_wc_if.h umpk_wc_if.h
	
umpk_wc.c:
	ln -f -s $(BASEDIR)/umpk/umpk_wc.c umpk_wc.c

umpk_doa_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_doa_if.h umpk_doa_if.h
	
umpk_doa.c:
	ln -f -s $(BASEDIR)/umpk/umpk_doa.c umpk_doa.c

umpk_sac_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_sac_if.h umpk_sac_if.h

umpk_sac.c:
	ln -f -s $(BASEDIR)/umpk/umpk_sac.c umpk_sac.c

umpk_pp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_pp_if.h umpk_pp_if.h

umpk_pp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_pp.c umpk_pp.c

umpk_sds_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_sds_if.h umpk_sds_if.h

umpk_sds.c:
	ln -f -s $(BASEDIR)/umpk/umpk_sds.c umpk_sds.c

umpk_drw_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_drw_if.h umpk_drw_if.h

umpk_drw.c:
	ln -f -s $(BASEDIR)/umpk/umpk_drw.c umpk_drw.c

umpk_tp.c:
	ln -f -s $(BASEDIR)/umpk/umpk_tp.c umpk_tp.c

umpk_tp_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_tp_if.h umpk_tp_if.h

umpk_ini_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_ini_if.h umpk_ini_if.h

umpk_ini.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_ini.c umpk_ini.c

umpk_rept_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_rept_if.h umpk_rept_if.h

umpk_rept.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_rept.c umpk_rept.c

umpk_reps_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_reps_if.h umpk_reps_if.h

umpk_reps.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_reps.c umpk_reps.c

umpk_server_if.h:
	ln -f -s $(UMPK_BASEDIR)/umpk_server_if.h umpk_server_if.h

umpk_server.c:
	ln -f -s $(UMPK_BASEDIR)/umpk_server.c umpk_server.c
