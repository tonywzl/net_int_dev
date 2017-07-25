/*
 * manager-pp.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrpp_if.h"
#include "manager.h"
#include "allocator_if.h"


#define CMD_STAT		1
#define CMD_ADD			2
#define CMD_DELETE		3
#define CMD_DISPLAY		4
#define CMD_HELLO		5
#define	CMD_MAX			6

#define CODE_NONE		0
#define CODE_HELLO		1

/* code of display */
#define CODE_S_DIS		1
#define CODE_S_DIS_ALL		2
#define CODE_W_DIS		3
#define CODE_W_DIS_ALL		4

static void usage_pp();

struct pp_setid_map {
	char	*owner_type;
	int	set_id;
};

static struct pp_setid_map pp_setid_table[] = {
		{"scg",	ALLOCATOR_SET_SCG_PP},
		{"rc",	ALLOCATOR_SET_RC_PP},
		{"crc",	ALLOCATOR_SET_CRC_PP},
		{"sds",	ALLOCATOR_SET_SDS_PP},
		{NULL,	0},
};

static int __get_pp_setid(char *owner_type)
{
	struct pp_setid_map *mp = pp_setid_table;

	while (mp->owner_type) {
		if (!strcmp(owner_type, mp->owner_type)) {
			break;
		}
		mp++;
	}
	return mp->set_id;
}

static int
__pp_stat_func(int argc, char* const argv[], char *pp_name, struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "__pp_stat_func";
	char *warn_str = "";
	int rc = -1;

	assert(argc);
	assert(argv);	

	if (pp_name == NULL) {
		rc = mgrpp_p->pp_op->pp_stat_all(mgrpp_p);
	} else {
		if (strlen(pp_name) >= NID_MAX_PPNAME) {
                        warn_str = "wrong pp_name!";
                        goto out;
		}
		rc = mgrpp_p->pp_op->pp_stat(mgrpp_p, pp_name);
	}

out:
	if (rc == -1) {
		usage_pp();
		nid_log_warning("%s: pp_name:%s: %s",
			log_header, pp_name, warn_str);
	} else if (rc == 1) {
		printf("pp: %s not found.\n",pp_name);
	}
	return rc;
}

static int
__pp_add_func(int argc, char* const argv[], char *pp_name, struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "__pp_add_func";
	char *pp_conf;
	char args[4][1024];
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";
	int n;
	char *owner_type;
	uint8_t set_id;
	int pool_size, page_size;

	if (!pp_name) {
		goto out;
	}

	if (strlen(pp_name) >= NID_MAX_PPNAME) {
		warn_str = "wrong pp_name!";
		goto out;
	}

	if (optind >= argc) {
		warn_str = "got empty pp configuration!";
		goto out;
	}

	pp_conf = argv[optind++];
	n = sscanf(pp_conf, "%[^:]:%[^:]:%[^:]:%[^:]",
			args[0], args[1], args[2], args[3]);

	if (n != 3) {
		warn_str = "wrong number of pp configuration items!";
		goto out;
	}

	owner_type = args[0];
	set_id = (uint8_t)__get_pp_setid(owner_type);
	if (!set_id) {
		warn_str = "wrong owner_type configuration!";
		goto out;
	}

	pool_size = atoi(args[1]);

	page_size = atoi(args[2]);

	nid_log_warning("%s: cmd_code=%d, optind:%d pp_name:%s",
				log_header, cmd_code, optind, pp_name);
	rc = mgrpp_p->pp_op->pp_add(mgrpp_p, pp_name, set_id, (uint32_t)pool_size, (uint32_t)page_size);
	return rc;

out:
	if (rc) {
		usage_pp();
		nid_log_warning("%s: pp_name:%s: %s",
			log_header, pp_name, warn_str);
	}
	return rc;
}

static int
__pp_del_func(int argc, char * const argv[], char *pp_name, struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "__pp_del_func";
	int rc = -1, cmd_code = CODE_NONE;
	char *warn_str = "";

	assert(argv);
	if (!pp_name) {
		warn_str = "pp name cannot be null!";
		goto out;
	}

	if (optind < argc) {
		warn_str = "got too many arguments!";
		goto out;

	}

	if (strlen(pp_name) >= NID_MAX_UUID) {
		warn_str = "wrong pp name!";
		goto out;
	}

	nid_log_warning("%s: cmd_code=%d, optind:%d, pp_name:%s: del",
		log_header, cmd_code, optind, pp_name);
	rc = mgrpp_p->pp_op->pp_delete(mgrpp_p, pp_name);

out:
	if (rc) {
		usage_pp();
		nid_log_warning("%s: pp_name:%s: %s",
			log_header, pp_name, warn_str);
	}
	return rc;
}

static int
__pp_display_func(int argc, char* const argv[], char *pp_name, struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "__pp_display_func";
	char *cmd_str = NULL;
	int rc = -1, cmd_code = CODE_NONE;
	uint8_t is_setup;
	char *warn_str = "";

	if (optind >= argc) {
		warn_str = "got empty sub command!";
		usage_pp();
		goto out;
	}

	cmd_str = argv[optind++];

	if (pp_name != NULL && cmd_str != NULL) {
		if (strlen(pp_name) > NID_MAX_PPNAME) {
			warn_str = "wrong pp_name!";
			usage_pp();
			goto out;
		}
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_W_DIS;
			is_setup = 0;
		}
	}
	else if (pp_name == NULL && cmd_str != NULL){
		if (!strcmp(cmd_str, "setup")) {
			cmd_code = CODE_S_DIS_ALL;
			is_setup = 1;
		}
		else if (!strcmp(cmd_str, "working")) {
			cmd_code = CODE_W_DIS_ALL;
			is_setup = 0;
		}
	}
	else if (cmd_str == NULL) {
		usage_pp();
		goto out;
	}

	if (cmd_code == CODE_S_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, pp_name:%s: display",
			log_header, cmd_code, optind, pp_name);
		rc = mgrpp_p->pp_op->pp_display(mgrpp_p, pp_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_S_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, pp_name:%s: display all",
			log_header, cmd_code, optind, pp_name);
		rc = mgrpp_p->pp_op->pp_display_all(mgrpp_p, is_setup);
	}
	else if (cmd_code == CODE_W_DIS) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, pp_name:%s: display",
			log_header, cmd_code, optind, pp_name);
		rc = mgrpp_p->pp_op->pp_display(mgrpp_p, pp_name, is_setup);
		if (rc)
			warn_str = "not found!";
	}
	else if (cmd_code == CODE_W_DIS_ALL) {
		nid_log_warning("%s: cmd_code=%d, optind:%d, pp_name:%s: display",
			log_header, cmd_code, optind, pp_name);
		rc = mgrpp_p->pp_op->pp_display_all(mgrpp_p, is_setup);
	}
	else {
		nid_log_warning("%s: cmd_code=%d, optind=%d, pp_name=%s: got wrong cmd_code",
			log_header, cmd_code, optind, pp_name);
	}

out:
	if (rc) {
		nid_log_warning("%s: pp_name:%s: %s",
			log_header, pp_name, warn_str);
	}
	return rc;
}

static int
__pp_hello_func(int argc, char* const argv[], char *pp_name, struct mgrpp_interface *mgrpp_p)
{
	char *log_header = "__pp_stat_func";
	int cmd_code = CODE_NONE;
	int rc = -1;

	assert(&pp_name);
	assert(argv);	

	if (optind > argc) {
		usage_pp();
		goto out;
	}

	cmd_code = CMD_HELLO;

	nid_log_warning("%s: cmd_code=%d, optind=%d: hello",
			log_header, cmd_code, optind);
	rc = mgrpp_p->pp_op->pp_hello(mgrpp_p);

out:
	return rc;
}

typedef int (*pp_operation_func)(int, char* const *, char *, struct mgrpp_interface *);
struct manager_pp_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	pp_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_pp_task pp_task_table[] = {
	{"stat",	"stat",		CMD_STAT,		__pp_stat_func,		NULL,	NULL},
	{"add",		"add",		CMD_ADD,		__pp_add_func,		NULL,	NULL},
	{"delete",	"del",		CMD_DELETE,		__pp_del_func,		NULL,	NULL},
	{"display",	"disp",		CMD_DISPLAY,		__pp_display_func,	NULL,	NULL},
	{"hello",	"hello",	CMD_HELLO,		__pp_hello_func,	NULL,	NULL},
	{NULL,		NULL,		CMD_MAX,		NULL,			NULL,	NULL},
};

static void
__setup_stat_setup_description(struct manager_pp_task *add_mtask)
{
	add_mtask->description = "stat:	stat operation command\n";

	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"-i 			specify the pp_name\n\n"
		first_intend"			show all pp general stat\n"
		first_intend"			sample:nidmanager -r s -m pp stat \n\n"
		first_intend"			show one pp general stat\n"
		first_intend"			sample:nidmanager -r s -m pp -i pp_name stat\n");
}

static void
__setup_add_setup_description(struct manager_pp_task *add_mtask)
{
	add_mtask->description = "add:	add operation command\n";

	add_mtask->detail = x_malloc (1024*1024);
	sprintf(add_mtask->detail, "%s",
		first_intend"[pp_name] add [owner_type]:[pool_size]:[page_size]"
			"		pp configurations\n"
		first_intend"			sample:nidmanager -r s -m pp -i pp_name add sds:512:8 \n");
}

static void
__setup_del_setup_description(struct manager_pp_task *del_mtask)
{
	del_mtask->description = "del:	delete operation command\n";

	del_mtask->detail = x_malloc(1024);
	sprintf(del_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m pp -i pp_name del\n");
}

static void
__setup_display_setup_description(struct manager_pp_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of pp";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"setup:			show pp configuration\n"
		first_intend"			sample:nidmanager -r s -m pp display setup\n"
		first_intend"			sample:nidmanager -r s -m pp -i pp_name display setup\n"
		first_intend"working:		show working crc configuration\n"
		first_intend"			sample:nidmanager -r s -m pp display working\n"
		first_intend"			sample:nidmanager -r s -m pp -i pp_name display working\n");
}

static void
__setup_hello_setup_description(struct manager_pp_task *hello_mtask)
{
	hello_mtask->description = "hello:	show this module is supported or not\n";

	hello_mtask->detail = x_malloc(1024);
	sprintf(hello_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m pp hello\n");
}

static void
setup_descriptions()
{
	struct manager_pp_task *mtask;

	mtask = &pp_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_STAT)
			__setup_stat_setup_description(mtask);
		else if (mtask->cmd_index == CMD_ADD)
			__setup_add_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DELETE)
			__setup_del_setup_description(mtask);
		else if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else if (mtask->cmd_index == CMD_HELLO)
			__setup_hello_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_pp()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_pp_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m pp ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &pp_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (pp_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &pp_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s\n",task->description );
		printf("%s\n",task->detail);

	}
	printf ("\n");

	return;
}

static char *optstr_ss = "a:i:";
static struct option long_options_ss[] = {
	{"vec", required_argument, NULL, 'v'},
	{0, 0, 0, 0},
};

int
manager_server_pp(int argc, char * const argv[])
{
	char *log_header = "manager_server_pp";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	char *pp_name = NULL;
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrpp_interface *mgrpp_p;
	struct mgrpp_setup setup;
	struct manager_pp_task *mtask = &pp_task_table[0];
	int rc = 0;


	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...\n", log_header);
	while ((c = getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		case 'i':
			pp_name = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_pp();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrpp_p = x_calloc(1, sizeof(*mgrpp_p));
	mgrpp_initialization(mgrpp_p, &setup);

	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_pp();
		rc = -1;
		goto out;
	}

	cmd_index = CMD_MAX;
	while (mtask->cmd_index < CMD_MAX) {
		if (!strcmp(cmd_str, mtask->cmd_long_name) || !strcmp(cmd_str, mtask->cmd_short_name)) {
			cmd_index = mtask->cmd_index;
			break;
		}
		mtask++;
	}

	if (cmd_index == CMD_MAX) {
		usage_pp();
		rc = -1;
		goto out;
	}

	mtask->func(argc, argv, pp_name, mgrpp_p);

out:
	return rc;
}
