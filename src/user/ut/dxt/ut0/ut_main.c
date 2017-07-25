#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dxt_if.h"

static struct dxt_setup dxt_setup;
static struct dxt_interface ut_dxt;

uint8_t __tx_get_pkg_level(struct list_node *lnp)
{
	assert(lnp);
	return 0;
}

int
util_nw_write_timeout_rc(int sfd, char *buf, int to_write, int timeout, int *rc_code)
{
	assert(sfd && buf && to_write && timeout && rc_code);
	return 0;
}

int
util_nw_read_stop(int fd, char *p, uint32_t to_read, int32_t timeout, uint8_t *stop)
{
	assert(fd && p && to_read && timeout && stop);
	return 0;
}

int
main()
{
	nid_log_open();
	nid_log_info("dxt ut module start ...");

	dxt_initialization(&ut_dxt, &dxt_setup);

	nid_log_info("dxt ut module end...");
	nid_log_close();

	return 0;
}
