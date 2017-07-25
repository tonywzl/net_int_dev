#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "btn_if.h"

static struct btn_setup btn_setup;
static struct btn_interface ut_btn;

int
main()
{
	struct btn_node *np[1025];
	int i;

	nid_log_open();
	nid_log_info("btn ut module start ...");

	btn_initialization(&ut_btn, &btn_setup);

	for (i = 0; i < 1025; i++) {
		np[i] = ut_btn.bt_op->bt_get_node(&ut_btn);
		assert(np[i]);
	}
	for (i = 0; i < 1025; i++) {
		ut_btn.bt_op->bt_put_node(&ut_btn, np[i]);
	}

	nid_log_info("btn ut module end...");
	nid_log_close();

	return 0;
}
