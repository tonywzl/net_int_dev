#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "cxa_if.h"

static struct cxa_setup cxa_setup;
static struct cxa_interface ut_cxa;

struct umessage_tx_hdr;

char*
encode_tx_hdr(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
        assert(out_buf && out_len && msghdr_p);
        return NULL;
}

char*
decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
        assert(p && len && msghdr_p);
        return NULL;
}

int
main()
{
	nid_log_open();
	nid_log_info("cxa ut module start ...");

	cxa_initialization(&ut_cxa, &cxa_setup);

	nid_log_info("cxa ut module end...");
	nid_log_close();

	return 0;
}
