CONF_SRC = server_conf.c server_global_conf.c cds_conf.c crc_conf.c cxa_conf.c cxp_conf.c cxt_conf.c \
	dxa_conf.c dxp_conf.c dxt_conf.c bwc_conf.c pp_conf.c sac_conf.c scg_conf.c sds_conf.c\
	tp_conf.c twc_conf.c \
	rgdev_conf.c \
	drw_conf.c mrw_conf.c sarw_conf.c reps_conf.c rept_conf.c

CONF_SYMLINKS = server_conf.c server_global_conf.c cds_conf.c crc_conf.c cxa_conf.c cxp_conf.c cxt_conf.c \
        dxa_conf.c dxp_conf.c dxt_conf.c bwc_conf.c pp_conf.c sac_conf.c scg_conf.c sds_conf.c\
        tp_conf.c twc_conf.c \
	rgdev_conf.c \
	drw_conf.c mrw_conf.c sarw_conf.c reps_conf.c rept_conf.c

server_conf.c:
	ln -f -s $(CONF_BASEDIR)/server_conf.c server_conf.c

server_global_conf.c:
	ln -f -s $(CONF_BASEDIR)/server_global_conf.c server_global_conf.c

cds_conf.c:
	ln -f -s $(CONF_BASEDIR)/cds_conf.c cds_conf.c

crc_conf.c:
	ln -f -s $(CONF_BASEDIR)/crc_conf.c crc_conf.c

cxa_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxa_conf.c cxa_conf.c

cxp_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxp_conf.c cxp_conf.c

cxt_conf.c:
	ln -f -s $(CONF_BASEDIR)/cxt_conf.c cxt_conf.c

dxa_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxa_conf.c dxa_conf.c

dxp_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxp_conf.c dxp_conf.c

dxt_conf.c:
	ln -f -s $(CONF_BASEDIR)/dxt_conf.c dxt_conf.c

bwc_conf.c:
	ln -f -s $(CONF_BASEDIR)/bwc_conf.c bwc_conf.c

pp_conf.c:
	ln -f -s $(CONF_BASEDIR)/pp_conf.c pp_conf.c

sac_conf.c:
	ln -f -s $(CONF_BASEDIR)/sac_conf.c sac_conf.c

scg_conf.c:
	ln -f -s $(CONF_BASEDIR)/scg_conf.c scg_conf.c

sds_conf.c:
	ln -f -s $(CONF_BASEDIR)/sds_conf.c sds_conf.c

tp_conf.c:
	ln -f -s $(CONF_BASEDIR)/tp_conf.c tp_conf.c

twc_conf.c:
	ln -f -s $(CONF_BASEDIR)/twc_conf.c twc_conf.c

rgdev_conf.c:
	ln -f -s $(CONF_BASEDIR)/rgdev_conf.c rgdev_conf.c

drw_conf.c:
	ln -f -s $(CONF_BASEDIR)/drw_conf.c drw_conf.c

mrw_conf.c:
	ln -f -s $(CONF_BASEDIR)/mrw_conf.c mrw_conf.c

sarw_conf.c:
	ln -f -s $(CONF_BASEDIR)/sarw_conf.c sarw_conf.c

reps_conf.c:
	ln -f -s $(CONF_BASEDIR)/reps_conf.c reps_conf.c

rept_conf.c:
	ln -f -s $(CONF_BASEDIR)/rept_conf.c rept_conf.c
