SRCS = nid_log.c mpk.c mpk_subr.c util_nw.c mq.c tp.c tpg.c nw.c nw_agent.c ini.c ash.c dsm.c asc.c asc-sm.c asc_conf.c \
       mwl.c acg.c ascg.c amc.c nl.c adt.c drv.c lstn.c node.c agent_conf.c agent.c allocator.c pp2.c lck.c agent_global_conf.c spn.c umpka.c umpk_agent.c umpk_driver.c mqtt.c

SYMLINKS = version.h nid.h nid_shared.h list.h nid_log.h nid_log.c \
	mpk_if.h mpk.c mpk_subr.c \
	nw.h nw_if.h nw.c nw_agent.c \
	util_nw.h util_nw.c \
	ini_if.h ini.c \
	ash_if.h ash.c \
	dsm_if.h dsm.c \
	asc_if.h asc.h asc.c asc-sm.c asc_conf.c\
	mwl_if.h mwl.c \
	asc_if.h asc.h asc.c asc-sm.c\
	acg_if.h acg.c \
	ascg_if.h ascg.c \
	amc_if.h amc.c \
	tp_if.h tp.c \
	tpg_if.h tpg.c \
	mq_if.h mq.c \
	nl_if.h nl.c \
	lstn_if.h lstn.c \
	node_if.h node.c \
	spn_if.h spn.c \
	adt_if.h adt.c \
	drv_if.h drv.c \
	agent.h agent_conf.c agent.c \
	allocator_if.h allocator.c \
	pp2_if.h pp2.c \
	lck_if.h lck.c \
	agent_global_conf.c \
	umpk_agent_if.h umpk_agent.c \
	umpk_driver_if.h umpk_driver.c \
	umpka_if.h umpka.h umpka.c \
	mqtt_if.h mqtt.c

version.h:
	ln -f -s $(BASEDIR)/../../build/version.h version.h

mpk_if.h:
	ln -f -s $(BASEDIR)/../shared/mpk_if.h mpk_if.h

mpk.c:
	ln -f -s $(BASEDIR)/../shared/mpk.c mpk.c

mpk_subr.c:
	ln -f -s $(BASEDIR)/../shared/mpk_subr.c mpk_subr.c
	
nid_shared.h:
	ln -f -s $(BASEDIR)/../shared/nid_shared.h nid_shared.h

nid.h:
	ln -f -s $(BASEDIR)/nid.h nid.h

list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

nid_log.c:
	ln -f -s $(BASEDIR)/nid_log.c nid_log.c

util_nw.h:
	ln -f -s $(BASEDIR)/util_nw.h util_nw.h

util_nw.c:
	ln -f -s $(BASEDIR)/util_nw.c util_nw.c

tp_if.h:
	ln -f -s $(BASEDIR)/tp_if.h tp_if.h
	
tp.c:
	ln -f -s $(BASEDIR)/tp.c tp.c

tpg_if.h:
	ln -f -s $(BASEDIR)/tpg_if.h tpg_if.h

tpg.c:
	ln -f -s $(BASEDIR)/tpg.c tpg.c

mq_if.h:
	ln -f -s $(BASEDIR)/mq_if.h mq_if.h

mq.c:
	ln -f -s $(BASEDIR)/mq.c mq.c

nw.h:
	ln -f -s $(BASEDIR)/nw.h nw.h
	
nw_if.h:
	ln -f -s $(BASEDIR)/nw_if.h nw_if.h
	
nw.c:
	ln -f -s $(BASEDIR)/nw.c nw.c

nw_agent.c:
	ln -f -s $(BASEDIR)/nw_agent.c nw_agent.c

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

lstn_if.h:
	ln -f -s $(BASEDIR)/mm/lstn_if.h lstn_if.h

lstn.c:
	ln -f -s $(BASEDIR)/mm/lstn.c lstn.c

node_if.h:
	ln -f -s $(BASEDIR)/mm/node_if.h node_if.h

node.c:
	ln -f -s $(BASEDIR)/mm/node.c node.c

spn_if.h:
	ln -f -s $(BASEDIR)/mm/spn_if.h spn_if.h

spn.c:
	ln -f -s $(BASEDIR)/mm/spn.c spn.c

ash_if.h:
	ln -f -s $(BASEDIR)/ash_if.h ash_if.h

ash.c:
	ln -f -s $(BASEDIR)/ash.c ash.c

dsm_if.h:
	ln -f -s $(BASEDIR)/dsm_if.h dsm_if.h

dsm.c:
	ln -f -s $(BASEDIR)/dsm.c dsm.c

asc_if.h:
	ln -f -s $(BASEDIR)/asc_if.h asc_if.h

asc.h:
	ln -f -s $(BASEDIR)/asc.h asc.h

asc.c:
	ln -f -s $(BASEDIR)/asc.c asc.c

asc-sm.c:
	ln -f -s $(BASEDIR)/asc-sm.c asc-sm.c

amc_if.h:
	ln -f -s $(BASEDIR)/amc_if.h amc_if.h

amc.c:
	ln -f -s $(BASEDIR)/amc.c amc.c

mwl_if.h:
	ln -f -s $(BASEDIR)/mwl_if.h mwl_if.h

mwl.c:
	ln -f -s $(BASEDIR)/mwl.c mwl.c

acg_if.h:
	ln -f -s $(BASEDIR)/acg_if.h acg_if.h

acg.c:
	ln -f -s $(BASEDIR)/acg.c acg.c

ascg_if.h:
	ln -f -s $(BASEDIR)/ascg_if.h ascg_if.h

ascg.c:
	ln -f -s $(BASEDIR)/ascg.c ascg.c

nl_if.h:
	ln -f -s $(BASEDIR)/nl_if.h nl_if.h

nl.c:
	ln -f -s $(BASEDIR)/nl.c nl.c

adt_if.h:
	ln -f -s $(BASEDIR)/adt_if.h adt_if.h

adt.c:
	ln -f -s $(BASEDIR)/adt.c adt.c

drv_if.h:
	ln -f -s $(BASEDIR)/drv_if.h drv_if.h

drv.c:
	ln -f -s $(BASEDIR)/drv.c drv.c

agent.h:
	ln -f -s $(BASEDIR)/agent.h agent.h

agent_conf.c:
	ln -f -s $(CONF_BASEDIR)/agent_conf.c agent_conf.c

agent_global_conf.c:
	ln -f -s $(CONF_BASEDIR)/agent_global_conf.c agent_global_conf.c

asc_conf.c:
	ln -f -s $(CONF_BASEDIR)/asc_conf.c asc_conf.c

agent.c:
	ln -f -s $(BASEDIR)/agent.c agent.c
allocator_if.h:
	ln -f -s $(BASEDIR)/mm/allocator_if.h allocator_if.h
allocator.c:
	ln -f -s $(BASEDIR)/mm/allocator.c allocator.c
pp2_if.h:
	ln -f -s $(BASEDIR)/mm/pp2_if.h pp2_if.h
pp2.c:
	ln -f -s $(BASEDIR)/mm/pp2.c pp2.c
lck_if.h:
	ln -f -s $(BASEDIR)/lck_if.h lck_if.h
lck.c:
	ln -f -s $(BASEDIR)/lck.c lck.c

umpka_if.h:
	ln -f -s $(BASEDIR)/umpk/umpka_if.h umpka_if.h

umpka.h:
	ln -f -s $(BASEDIR)/umpk/umpka.h umpka.h

umpka.c:
	ln -f -s $(BASEDIR)/umpk/umpka.c umpka.c

umpk_agent_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_agent_if.h umpk_agent_if.h

umpk_agent.c:
	ln -f -s $(BASEDIR)/umpk/umpk_agent.c umpk_agent.c

umpk_driver_if.h:
	ln -f -s $(BASEDIR)/umpk/umpk_driver_if.h umpk_driver_if.h

umpk_driver.c:
	ln -f -s $(BASEDIR)/umpk/umpk_driver.c umpk_driver.c

mqtt_if.h:
	ln -f -s $(BASEDIR)/mqtt_if.h mqtt_if.h

mqtt.c:
	ln -f -s $(BASEDIR)/mqtt.c mqtt.c
