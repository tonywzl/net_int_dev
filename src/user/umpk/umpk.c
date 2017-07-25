/*
 * umpk.c
 * 	Implementation of Userland Message Packaging Module
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "nid_log.h"
#include "nid.h"
#include "umpk.h"
#include "umpk_if.h"

struct umpk_private {
};

/*
 * Input:
 * 	ctype: channel type
 *	data: data to encode
 * output:
 * 	umpk_buf: encoded data, ready to transfer through network
 * 	len: encoded data length
 * 	data: may be updated
 */
static int
umpk_encode(struct umpk_interface *umpk_p, char *umpk_buf, uint32_t *len, int ctype, void *data)
{
	char *log_header = "umpk_encode";
	char *p = umpk_buf;
	int rc = 0;

	assert(umpk_p);
	switch (ctype) {
	case NID_CTYPE_WC:
		rc = umpk_encode_wc(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_BWC:
		rc = umpk_encode_bwc(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_TWC:
		rc = umpk_encode_twc(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_MRW:
		rc = umpk_encode_mrw(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_DXA:
		rc = umpk_encode_dxa(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_DXP:
		rc = umpk_encode_dxp(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_CXA:
		rc = umpk_encode_cxa(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_CXP:
		rc = umpk_encode_cxp(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_CRC:
		rc = umpk_encode_crc(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_DOA:
		rc = umpk_encode_doa(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_SAC:
		rc = umpk_encode_sac(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_PP:
		rc = umpk_encode_pp(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_SDS:
		rc = umpk_encode_sds(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_DRW:
		rc = umpk_encode_drw(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;
	
	case NID_CTYPE_TP:
		rc = umpk_encode_tp(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_INI:
		rc = umpk_encode_ini(p, len, data);
		nid_log_warning("%s: len:%d", log_header, *len);
		break;

	case NID_CTYPE_SERVER:
		rc = umpk_encode_server(p, len, data);
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
 *	umpk_buf: encoded data
 *	len: length of encoded data
 * output:
 * 	data: output structure
 */
static int
umpk_decode(struct umpk_interface *umpk_p, char *umpk_buf, uint32_t len, int ctype, void *data)
{
	char *log_header = "umpk_decode";
	char *p = umpk_buf;
	int rc = 0;

	assert(umpk_p);
	switch (ctype) {
	case NID_CTYPE_WC:
		rc = umpk_decode_wc(p, len, data);
		break;

	case NID_CTYPE_BWC:
		rc = umpk_decode_bwc(p, len, data);
		break;

	case NID_CTYPE_TWC:
		rc = umpk_decode_twc(p, len, data);
		break;

	case NID_CTYPE_MRW:
		rc = umpk_decode_mrw(p, len, data);
		break;

	case NID_CTYPE_CRC:
		rc = umpk_decode_crc(p, len, data);
		break;

	case NID_CTYPE_DOA:
		rc = umpk_decode_doa(p, len, data);
		break;

	case NID_CTYPE_DXA:
		rc = umpk_decode_dxa(p, len, data);
		break;

	case NID_CTYPE_DXP:
		rc = umpk_decode_dxp(p, len, data);
		break;

	case NID_CTYPE_CXA:
		rc = umpk_decode_cxa(p, len, data);
		break;

	case NID_CTYPE_CXP:
		rc = umpk_decode_cxp(p, len, data);
		break;

	case NID_CTYPE_SAC:
		rc = umpk_decode_sac(p, len, data);
		break;

	case NID_CTYPE_PP:
		rc = umpk_decode_pp(p, len, data);
		break;

	case NID_CTYPE_SDS:
		rc = umpk_decode_sds(p, len, data);
		break;

	case NID_CTYPE_DRW:
		rc = umpk_decode_drw(p, len, data);
		break;

	case NID_CTYPE_TP:
		rc = umpk_decode_tp(p, len, data);
		break;

	case NID_CTYPE_INI:
		rc = umpk_decode_ini(p, len, data);
		break;

	case NID_CTYPE_SERVER:
		rc = umpk_decode_server(p, len, data);
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
umpk_cleanup(struct umpk_interface *umpk_p)
{
	struct umpk_private *priv_p = (struct umpk_private *)umpk_p->um_private;
	free(priv_p);
	umpk_p->um_private = NULL;
}

static struct umpk_operations umpk_op = {
	.um_decode = umpk_decode,
	.um_encode = umpk_encode,
	.um_cleanup = umpk_cleanup,
};

int
umpk_initialization(struct umpk_interface *umpk_p, struct umpk_setup *setup)
{
	char *log_header = "umpk_initialization";
	struct umpk_private *priv_p;

	nid_log_debug("%s: start ...", log_header);
	assert(setup);
	priv_p = calloc(1, sizeof(*priv_p));
	umpk_p->um_private = priv_p;
	umpk_p->um_op = &umpk_op;

	return 0;
}
