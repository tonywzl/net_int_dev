#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

#include "version.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "nid.h"
#include "tp_if.h"
#include "nw_if.h"
#include "mq_if.h"
#include "nl_if.h"
#include "adt_if.h"
#include "ini_if.h"
#include "acg_if.h"
#include "agent.h"
#include "allocator_if.h"
#include "pp2_if.h"
#include "lstn_if.h"
#include "tpg_if.h"
#include "umpka_if.h"
#include "mqtt_if.h"

static pthread_mutex_t agent_lck;
static pthread_cond_t agent_cond;
static int __agent_stop = 0;

void
agent_stop()
{
	pthread_mutex_lock(&agent_lck);
	__agent_stop = 1;
	pthread_cond_signal(&agent_cond);
	pthread_mutex_unlock(&agent_lck);
}

int
agent_is_stop()
{
	return __agent_stop ? 1 : 0;
}

static int
agent_alive()
{
	int rc;
	rc = system("/usr/local/bin/nidmanager -r a > /dev/null");
	return rc ? 0 : 1;
}

int
main()
{
	struct ini_setup ini_setup;
	struct ini_interface *ini_p;
	struct tp_setup tp_setup;
	struct tp_interface *tp_p;
	struct nw_setup nw_setup;
	struct nw_interface *nw_p;
	struct acg_setup acg_setup;
	struct acg_interface *acg_p;
	struct rlimit rlim;
	struct allocator_setup allocator_setup;
	struct allocator_interface *allocator_p;
	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p, *dyn_pp2_p, *dyn2_pp2_p;
	struct lstn_setup lstn_setup;
	struct lstn_interface *lstn_p;
	struct list_head conf_key_set;
	struct list_node *conf_key_node;
	struct ini_key_desc *conf_key;
	struct tpg_interface *tpg_p;
	struct tpg_setup tpg_setup;
	struct umpka_setup umpka_setup;
	struct umpka_interface *umpka_p;
	struct sigaction sa;
	struct mqtt_interface *mqtt_p;
	struct mqtt_setup mqtt_setup;
	struct mqtt_message msg;
    struct ini_section_content *global_sect;
    struct ini_key_content *the_key;

	if (agent_alive()) {
		printf("agent already exists, exiting ...\n");
		return -1;
	}

	daemon(1, 0);
	nid_log_open();
	nid_log_warning("nidagent (version:%s) start ...", NID_VERSION);

	if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
			nid_log_error("nid server setrlimit failed");
		}
	}

	/*init pp2*/
	allocator_p = x_calloc(1, sizeof(*allocator_p));
	allocator_setup.a_role = ALLOCATOR_ROLE_AGENT;
	allocator_initialization(allocator_p, &allocator_setup);
	pp2_p = x_calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "agent_pp2";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_AGENT_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 1;
	pp2_initialization(pp2_p, &pp2_setup);

	/*init dynamic pp2*/
	dyn_pp2_p = x_calloc(1, sizeof(*dyn_pp2_p));
	pp2_setup.name = "agent_dynamic_pp2";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_AGENT_DYN_PP2;
	pp2_setup.page_size = 1;
	pp2_setup.pool_size = 0;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(dyn_pp2_p, &pp2_setup);

	/*init dynamic pp2 2*/
	dyn2_pp2_p = x_calloc(1, sizeof(*dyn2_pp2_p));
	pp2_setup.name = "agent_dynamic2_pp2";
	pp2_setup.allocator = allocator_p;
	pp2_setup.set_id = ALLOCATOR_SET_AGENT_DYN2_PP2;
	pp2_setup.page_size = 8;
	pp2_setup.pool_size = 16;
	pp2_setup.get_zero = 0;
	pp2_setup.put_zero = 0;
	pp2_initialization(dyn2_pp2_p, &pp2_setup);


	pthread_mutex_init(&agent_lck, NULL);
	pthread_cond_init(&agent_cond, NULL);

	lstn_setup.allocator = allocator_p;
	lstn_setup.set_id = ALLOCATOR_SET_AGENT_LSTN;
	lstn_setup.seg_size = 128;
	lstn_p = x_calloc(1, sizeof(*lstn_p));
	lstn_initialization(lstn_p, &lstn_setup);


	INIT_LIST_HEAD(&conf_key_set);
	agent_generate_conf_tmplate(&conf_key_set, lstn_p);
	list_for_each_entry(conf_key_node, struct list_node, &conf_key_set, ln_list) {
		conf_key = (struct ini_key_desc *)conf_key_node->ln_data;
		while (conf_key->k_name) {
			if (!conf_key->k_description) {
				nid_log_error("agent_initialization: key: %s has no description.", conf_key->k_name);
				return 0;
			}
			conf_key++;
		}
	}
	memset(&ini_setup, 0, sizeof(ini_setup));
	ini_setup.path = NID_CONF_AGENT;
	ini_setup.key_set = conf_key_set;
	ini_p = (struct ini_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*ini_p));
	ini_initialization(ini_p, &ini_setup);
	ini_p->i_op->i_parse(ini_p);

	global_sect = ini_p->i_op->i_search_section(ini_p, "global");
	assert(global_sect);
	the_key = ini_p->i_op->i_search_key(ini_p, global_sect, "log_level");
	nid_log_set_level(*(int *)the_key->k_value);

	strcpy(tp_setup.name, "agent_tp");
	tp_setup.pp2 = pp2_p;
	tp_setup.min_workers = 1;
	tp_setup.max_workers = 0;
	tp_setup.extend = 1;
	tp_setup.delay = 1;
	tp_p = (struct tp_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*tp_p));
	tp_initialization(tp_p, &tp_setup);

	/* init umpka */
	memset(&umpka_setup, 0, sizeof(umpka_setup));
	umpka_p = (struct umpka_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*umpka_p));
	umpka_initialization(umpka_p, &umpka_setup);

	/*
	 * init mqtt
	 */
	memset(&mqtt_setup, 0, sizeof(mqtt_setup));
	mqtt_setup.client_id = "nidagent";
	mqtt_setup.clean_session = 0;
	mqtt_setup.pp2 = pp2_p;
	mqtt_setup.role = NID_ROLE_AGENT;
	mqtt_p = calloc(1, sizeof(*mqtt_p));
	mqtt_initialization(mqtt_p, &mqtt_setup);

	msg.type = "nidagent";
	msg.module = "NIDAgent";
	msg.message = "NID agent started";
	msg.status = "OK";
	strcpy(msg.uuid, "NULL");
	strcpy(msg.ip, "NULL");
	mqtt_p->mt_op->mt_publish(mqtt_p, &msg);

	/* init acg */
	acg_p = (struct acg_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*acg_p));
	acg_setup.ini = ini_p;
	acg_setup.tp = tp_p;
	acg_setup.pp2 = pp2_p;
	acg_setup.dyn_pp2 = dyn_pp2_p;
	acg_setup.dyn2_pp2 = dyn2_pp2_p;
	acg_setup.agent_keys = conf_key_set;
	acg_setup.umpka = umpka_p;
	acg_initialization(acg_p, &acg_setup);

	/*
	 * Create thread pool for guardian
	 */
	memset(&tpg_setup, 0, sizeof(tpg_setup));
	tpg_setup.ini = ini_p;
	tpg_setup.pp2 = pp2_p;
	tpg_p = x_calloc(1, sizeof(*tpg_p));
	tpg_initialization(tpg_p, &tpg_setup);

	memset(&nw_setup, 0, sizeof(nw_setup));
	nw_setup.cg = acg_p;
	nw_setup.type = NID_ROLE_AGENT;
	nw_setup.port = NID_AGENT_PORT;
	nw_setup.pp2 = pp2_p;
	nw_setup.dyn_pp2 = dyn_pp2_p;
	nw_setup.tpg = tpg_p;
	nw_p = (struct nw_interface *)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*nw_p));
	nw_initialization(nw_p, &nw_setup);

	sa.sa_handler = SIG_IGN;
	if(sigaction(SIGTERM, &sa, NULL) == -1) {
		exit(1);
	}

	sa.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &sa, NULL) == -1) {
		exit(1);
	}

	pthread_mutex_lock(&agent_lck);
	while (!__agent_stop) {
		pthread_cond_wait(&agent_cond, &agent_lck);
	}
	pthread_mutex_unlock(&agent_lck);

	while (!acg_p->ag_op->ag_is_stop(acg_p)) {
		sleep(1);
	}

	ini_p->i_op->i_cleanup(ini_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)ini_p);

	umpka_p->um_op->um_cleanup(umpka_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)umpka_p);

	acg_p->ag_op->ag_cleanup(acg_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)acg_p);

	tp_p->t_op->t_destroy(tp_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)tp_p);

	nw_p->n_op->n_cleanup(nw_p);
	pp2_p->pp2_op->pp2_put(pp2_p, (void *)nw_p);

	tpg_p->tg_op->tpg_cleanup_tp(tpg_p);

	msg.type = "nidagent";
	msg.module = "NIDAgent";
	msg.message = "NID agent stopped";
	msg.status = "OK";
	strcpy(msg.uuid, "NULL");
	strcpy(msg.ip, "NULL");
	mqtt_p->mt_op->mt_publish(mqtt_p, &msg);

	mqtt_p->mt_op->mt_cleanup(mqtt_p);

	pp2_p->pp2_op->pp2_display(pp2_p);
	pp2_p->pp2_op->pp2_cleanup(pp2_p);
	dyn_pp2_p->pp2_op->pp2_display(dyn_pp2_p);
	dyn_pp2_p->pp2_op->pp2_cleanup(dyn_pp2_p);
	dyn2_pp2_p->pp2_op->pp2_display(dyn2_pp2_p);
	dyn2_pp2_p->pp2_op->pp2_cleanup(dyn2_pp2_p);
	allocator_p->a_op->a_display(allocator_p);
	nid_log_info("nidagent end...");
	nid_log_close();

	return 0;
}
