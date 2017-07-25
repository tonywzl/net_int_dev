TX_SRC = crc.c crcg.c crcc.c crccg.c \
	cxa.c cxag.c cxpc.c cxpcg.c \
	cxp.c cxpg.c cxac.c cxacg.c \
	cxt.c cxtg.c \
	dxp.c dxpg.c dxpc.c dxpcg.c \
	dxa.c dxag.c dxac.c dxacg.c \
	dxt.c dxtg.c \
	mq.c  tx_shared.c

TX_SYMLINKS = cxa_if.h cxa.c cxag_if.h cxag.c cxac_if.h cxac.c cxacg_if.h cxacg.c \
	cxp_if.h cxp.c cxpg_if.h cxpg.c cxpc_if.h cxpc.c cxpcg_if.h cxpcg.c \
	cxt_if.h cxt.c cxtg_if.h cxtg.c \
	dxp_if.h dxp.c dxpg_if.h dxpg.c dxpc_if.h dxpc.c dxpcg_if.h dxpcg.c \
	dxa_if.h dxa.c dxag_if.h dxag.c dxac_if.h dxac.c dxacg_if.h dxacg.c \
	dxt_if.h dxt.c dxtg_if.h dxtg.c \
	mq_if.h mq.c \
	tx_shared.h tx_shared.c

cxa_if.h:
	ln -f -s $(TX_BASEDIR)/cxa_if.h cxa_if.h
	
cxa.c:
	ln -f -s $(TX_BASEDIR)/cxa.c cxa.c

cxag_if.h:
	ln -f -s $(TX_BASEDIR)/cxag_if.h cxag_if.h
	
cxag.c:
	ln -f -s $(TX_BASEDIR)/cxag.c cxag.c

cxac_if.h:
	ln -f -s $(TX_BASEDIR)/cxac_if.h cxac_if.h
	
cxac.c:
	ln -f -s $(TX_BASEDIR)/cxac.c cxac.c

cxacg_if.h:
	ln -f -s $(TX_BASEDIR)/cxacg_if.h cxacg_if.h
	
cxacg.c:
	ln -f -s $(TX_BASEDIR)/cxacg.c cxacg.c

cxp_if.h:
	ln -f -s $(TX_BASEDIR)/cxp_if.h cxp_if.h
	
cxp.c:
	ln -f -s $(TX_BASEDIR)/cxp.c cxp.c

cxpg_if.h:
	ln -f -s $(TX_BASEDIR)/cxpg_if.h cxpg_if.h
	
cxpg.c:
	ln -f -s $(TX_BASEDIR)/cxpg.c cxpg.c

cxpc_if.h:
	ln -f -s $(TX_BASEDIR)/cxpc_if.h cxpc_if.h
	
cxpc.c:
	ln -f -s $(TX_BASEDIR)/cxpc.c cxpc.c

cxpcg_if.h:
	ln -f -s $(TX_BASEDIR)/cxpcg_if.h cxpcg_if.h
	
cxpcg.c:
	ln -f -s $(TX_BASEDIR)/cxpcg.c cxpcg.c

cxt_if.h:
	ln -f -s $(TX_BASEDIR)/cxt_if.h cxt_if.h

cxt.c:
	ln -f -s $(TX_BASEDIR)/cxt.c cxt.c

cxtg_if.h:
	ln -f -s $(TX_BASEDIR)/cxtg_if.h cxtg_if.h

cxtg.c:
	ln -f -s $(TX_BASEDIR)/cxtg.c cxtg.c

dxp_if.h:
	ln -f -s $(TX_BASEDIR)/dxp_if.h dxp_if.h
	
dxp.c:
	ln -f -s $(TX_BASEDIR)/dxp.c dxp.c

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

dxt_if.h:
	ln -f -s $(TX_BASEDIR)/dxt_if.h dxt_if.h
	
dxt.c:
	ln -f -s $(TX_BASEDIR)/dxt.c dxt.c

dxtg_if.h:
	ln -f -s $(TX_BASEDIR)/dxtg_if.h dxtg_if.h
	
dxtg.c:
	ln -f -s $(TX_BASEDIR)/dxtg.c dxtg.c

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

mq.c:
	ln -f -s $(BASEDIR)/mq.c mq.c

tx_shared.h:
	ln -f -s $(TX_BASEDIR)/tx_shared.h tx_shared.h

tx_shared.c:
	ln -f -s $(TX_BASEDIR)/tx_shared.c tx_shared.c
