/*
 * manager-all.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "mgrall_if.h"
#include "mgrtp_if.h"
#include "mgrpp_if.h"
#include "mgrbwc_if.h"
#include "mgrtwc_if.h"
#include "mgrcrc_if.h"
#include "mgrsds_if.h"
#include "mgrmrw_if.h"
#include "mgrdrw_if.h"
#include "mgrdx_if.h"
#include "mgrcx_if.h"
#include "mgrsac_if.h"
#include "manager.h"

#define CMD_DISPLAY		1
#define CMD_MAX			2

#define CODE_NONE		0

/* code of display */
#define CODE_DISPLAY		1

static void usage_all();

static int
__all_display_func(struct mgrall_interface *mgrall_p,
			struct mgrtp_interface *mgrtp_p,
			struct mgrpp_interface *mgrpp_p,
			struct mgrbwc_interface *mgrbwc_p,
			struct mgrtwc_interface *mgrtwc_p,
			struct mgrcrc_interface *mgrcrc_p,
			struct mgrsds_interface *mgrsds_p,
			struct mgrmrw_interface *mgrmrw_p,
			struct mgrdrw_interface *mgrdrw_p,
			struct mgrdx_interface *mgrdx_p,
			struct mgrcx_interface *mgrcx_p,
			struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "__all_display_func";
	int rc = -1, cmd_code = CODE_NONE;

	cmd_code = CODE_DISPLAY;
	nid_log_warning("%s: cmd_code=%d, optind:%d: display:",
		log_header, cmd_code, optind);
	rc = mgrall_p->al_op->al_display(mgrall_p, mgrtp_p, mgrpp_p, mgrbwc_p, mgrtwc_p, mgrcrc_p, mgrsds_p,
				mgrmrw_p, mgrdrw_p, mgrdx_p, mgrcx_p, mgrsac_p);

	return rc;
}

typedef int (*all_operation_func)(struct mgrall_interface *,
					struct mgrtp_interface *,
					struct mgrpp_interface *,
					struct mgrbwc_interface *,
					struct mgrtwc_interface *,
					struct mgrcrc_interface *,
					struct mgrsds_interface *,
					struct mgrmrw_interface *,
					struct mgrdrw_interface *,
					struct mgrdx_interface *,
					struct mgrcx_interface *,
					struct mgrsac_interface *);

struct manager_all_task {
	char			*cmd_long_name;
	char			*cmd_short_name;
	int			cmd_index;
	all_operation_func	func;
	char			*description;
	char			*detail;
};

static struct manager_all_task all_task_table[] = {
	{"display",		"disp",		CMD_DISPLAY,			__all_display_func,		NULL,		NULL},
	{NULL,			NULL,		CMD_MAX,			NULL,				NULL,		NULL},
};

static void
__setup_display_setup_description(struct manager_all_task *disp_mtask)
{
	disp_mtask->description = "display:	show the configuration of all modules\n";

	disp_mtask->detail = x_malloc(1024);
	sprintf(disp_mtask->detail, "%s",
		first_intend"			sample:nidmanager -r s -m all display\n");
}

static void
setup_descriptions()
{
	struct manager_all_task *mtask;

	mtask = &all_task_table[0];
	while (mtask->cmd_index < CMD_MAX) {
		if (mtask->cmd_index == CMD_DISPLAY)
			__setup_display_setup_description(mtask);
		else
			assert(0);
		mtask++;
	}
}

void
usage_all()
{
	char *log_header = "usage";
	char *tab = "    ";
	struct manager_all_task *task;
	int i;

	setup_descriptions();

	printf("%s: nidmanager ", log_header);
	printf("%s -r s -m all ", tab);
	for (i = 0 ; i < CMD_MAX; i++){
		task = &all_task_table[i];
		if (task->cmd_long_name == NULL)
			break;
		printf("%s",task->cmd_short_name);
		if (all_task_table[i+1].cmd_long_name != NULL)
			printf ("|");
	}
	printf(" ...\n");
	for (i = 0; i < CMD_MAX; i++){
		task = &all_task_table[i];
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
	{0, 0, 0, 0},
};

int
manager_server_all(int argc, char * const argv[])
{
	char *log_header = "manager_server_all";
	char c;
	int i;
	int cmd_index;
	char *cmd_str = NULL;
	char *ipstr = "127.0.0.1";	// by default
	struct umpk_setup umpk_setup;
	struct umpk_interface *umpk_p;
	struct mgrall_interface *mgrall_p;
	struct mgrall_setup setup;
	struct mgrtp_interface *mgrtp_p;
	struct mgrpp_interface *mgrpp_p;
	struct mgrbwc_interface *mgrbwc_p;
	struct mgrtwc_interface *mgrtwc_p;
	struct mgrcrc_interface *mgrcrc_p;
	struct mgrsds_interface *mgrsds_p;
	struct mgrmrw_interface *mgrmrw_p;
	struct mgrdrw_interface *mgrdrw_p;
	struct mgrdx_interface *mgrdx_p;
	struct mgrcx_interface *mgrcx_p;
	struct mgrsac_interface *mgrsac_p;
	struct manager_all_task *mtask = &all_task_table[0];
	int rc = 0;


	umpk_p = x_calloc(1, sizeof(*umpk_p));
	umpk_initialization(umpk_p, &umpk_setup);

	nid_log_debug("%s: start ...\n", log_header);
	while ((c = getopt_long(argc, argv, optstr_ss, long_options_ss, &i)) >= 0) {
		switch (c) {
		case 'a':
			ipstr = optarg;
			break;

		default:
			printf("the option has not been create\n");
			usage_all();
			return 0;
		}
	}

	setup.ipstr = ipstr;
	setup.port = NID_SERVICE_PORT;
	setup.umpk = umpk_p;
	mgrall_p = x_calloc(1, sizeof(*mgrall_p));
	mgrall_initialization(mgrall_p, &setup);

	mgrtp_p = x_calloc(1, sizeof(*mgrtp_p));
	mgrtp_initialization(mgrtp_p, (struct mgrtp_setup *)&setup);

	mgrpp_p = x_calloc(1, sizeof(*mgrpp_p));
	mgrpp_initialization(mgrpp_p, (struct mgrpp_setup *)&setup);

	mgrbwc_p = x_calloc(1, sizeof(*mgrbwc_p));
	mgrbwc_initialization(mgrbwc_p, (struct mgrbwc_setup *)&setup);

	mgrtwc_p = x_calloc(1, sizeof(*mgrtwc_p));
	mgrtwc_initialization(mgrtwc_p, (struct mgrtwc_setup *)&setup);

	mgrcrc_p = x_calloc(1, sizeof(*mgrcrc_p));
	mgrcrc_initialization(mgrcrc_p, (struct mgrcrc_setup *)&setup);

	mgrsds_p = x_calloc(1, sizeof(*mgrsds_p));
	mgrsds_initialization(mgrsds_p, (struct mgrsds_setup *)&setup);

	mgrmrw_p = x_calloc(1, sizeof(*mgrmrw_p));
	mgrmrw_initialization(mgrmrw_p, (struct mgrmrw_setup *)&setup);

	mgrdrw_p = x_calloc(1, sizeof(*mgrdrw_p));
	mgrdrw_initialization(mgrdrw_p, (struct mgrdrw_setup *)&setup);

	mgrdx_p = x_calloc(1, sizeof(*mgrdx_p));
	mgrdx_initialization(mgrdx_p, (struct mgrdx_setup *)&setup);

	mgrcx_p = x_calloc(1, sizeof(*mgrcx_p));
	mgrcx_initialization(mgrcx_p, (struct mgrcx_setup *)&setup);

	mgrsac_p = x_calloc(1, sizeof(*mgrsac_p));
	mgrsac_initialization(mgrsac_p, (struct mgrsac_setup *)&setup);
	/*
	 *  get command
	 */
	cmd_str = argv[optind++];
	if (cmd_str == NULL) {
		usage_all();
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
		usage_all();
		rc = -1;
		goto out;
	}

	mtask->func(mgrall_p, mgrtp_p, mgrpp_p, mgrbwc_p, mgrtwc_p, mgrcrc_p, mgrsds_p, mgrmrw_p, mgrdrw_p,
			mgrdx_p, mgrcx_p, mgrsac_p);

out:
	return rc;
}
