/*
 * umpka.c
 * 	Implementation of Userland Message Packaging Agent Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nid_log.h"
#include "nid.h"
#include "umpka.h"
#include "umpka_if.h"

struct umpka_private {
};

/*
 * Input:
 * 	ctype: channel type
 *	data: data to encode
 * output:
 * 	umpka_buf: encoded data, ready to transfer through network
 * 	len: encoded data length
 * 	data: may be updated
 */
static int
umpka_encode(struct umpka_interface *umpka_p, char *umpka_buf, uint32_t *len, int ctype, void *data)
{
	char *log_header = "umpka_encode";
	char *p = umpka_buf;
	int rc = 0;

	assert(umpka_p);
	switch (ctype) {
	case NID_CTYPE_AGENT:
		rc = umpk_encode_agent(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_DRIVER:
		rc = umpk_encode_driver(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	default:
		nid_log_error("%s: got unknown ctype (%d)", log_header, ctype);
		len = 0;
		rc = -1;
		break;
	}

	if (rc)
		nid_log_error("%s: rc:%d", log_header, rc);
	return rc;
}

/*
 * Input:
 * 	ctype: channel type
 *	umpka_buf: encoded data
 *	len: length of encoded data
 * output:
 * 	data: output structure
 */
static int
umpka_decode(struct umpka_interface *umpka_p, char *umpka_buf, uint32_t len, int ctype, void *data)
{
	char *log_header = "umpka_decode";
	char *p = umpka_buf;
	int rc = 0;

	assert(umpka_p);
	switch (ctype) {
	case NID_CTYPE_AGENT:
		rc = umpk_decode_agent(p, len, data);
		break;

	case NID_CTYPE_DRIVER:
		rc = umpk_decode_driver(p, len, data);
		break;

	default:
		rc = -2;
		nid_log_error("%s: got unknown ctype (%d)", log_header, ctype);
	}

	if (rc)
		nid_log_error("%s: error, rc:%d", log_header, rc);

	return rc;
}

static void
umpka_cleanup(struct umpka_interface *umpka_p)
{
	struct umpka_private *priv_p = (struct umpka_private *)umpka_p->um_private;
	free(priv_p);
	umpka_p->um_private = NULL;
}

static struct umpka_operations umpka_op = {
	.um_decode = umpka_decode,
	.um_encode = umpka_encode,
	.um_cleanup = umpka_cleanup,
};

int
umpka_initialization(struct umpka_interface *umpka_p, struct umpka_setup *setup)
{
	char *log_header = "umpka_initialization";
	struct umpka_private *priv_p;

	nid_log_debug("%s: start ...", log_header);
	assert(setup);
	priv_p = calloc(1, sizeof(*priv_p));
	umpka_p->um_private = priv_p;
	umpka_p->um_op = &umpka_op;

	return 0;
}
