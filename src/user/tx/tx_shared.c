/*
 * tx_shared.c
 * common function for tx module
 */

#include <string.h>
#include <assert.h>

#include "tx_shared.h"
#include "lstn_if.h"

char*
encode_tx_hdr(char *out_buf, uint32_t *out_len, struct umessage_tx_hdr *msghdr_p)
{
	char *p = out_buf;
	uint32_t len = 0;

	memcpy(p, &msghdr_p->um_req, sizeof(msghdr_p->um_req));
	p += sizeof(msghdr_p->um_req);
	len += sizeof(msghdr_p->um_req);

	memcpy(p, &msghdr_p->um_req_code, sizeof(msghdr_p->um_req_code));
	p += sizeof(msghdr_p->um_req_code);
	len += sizeof(msghdr_p->um_req_code);

	memcpy(p, &msghdr_p->um_level, sizeof(msghdr_p->um_level));
	p += sizeof(msghdr_p->um_level);
	len += sizeof(msghdr_p->um_level);

	memcpy(p, &msghdr_p->um_type, sizeof(msghdr_p->um_type));
	p += sizeof(msghdr_p->um_type);
	len += sizeof(msghdr_p->um_type);

	memcpy(p, &msghdr_p->um_len, sizeof(msghdr_p->um_len));
	p += sizeof(msghdr_p->um_len);
	len += sizeof(msghdr_p->um_len);

	memcpy(p, &msghdr_p->um_dlen, sizeof(msghdr_p->um_dlen));
	p += sizeof(msghdr_p->um_dlen);
	len += sizeof(msghdr_p->um_dlen);

	memcpy(p, &msghdr_p->um_seq, sizeof(msghdr_p->um_seq));
	p += sizeof(msghdr_p->um_seq);
	len += sizeof(msghdr_p->um_seq);

	memcpy(p, &msghdr_p->um_dseq, sizeof(msghdr_p->um_dseq));
	p += sizeof(msghdr_p->um_dseq);
	len += sizeof(msghdr_p->um_dseq);

	memcpy(p, &msghdr_p->um_dummy, sizeof(msghdr_p->um_dummy));
	p += sizeof(msghdr_p->um_dummy);
	len += sizeof(msghdr_p->um_dummy);

	*out_len = len;

	return p;
}

char*
decode_tx_hdr(char *p, uint32_t *len, struct umessage_tx_hdr *msghdr_p)
{
	assert(*len >= UMSG_TX_HEADER_LEN);

	memcpy(&msghdr_p->um_req, p, sizeof(msghdr_p->um_req));
	p += sizeof(msghdr_p->um_req);

	memcpy(&msghdr_p->um_req_code, p, sizeof(msghdr_p->um_req_code));
	p += sizeof(msghdr_p->um_req_code);

	memcpy(&msghdr_p->um_level, p, sizeof(msghdr_p->um_level));
	p += sizeof(msghdr_p->um_level);

	memcpy(&msghdr_p->um_type, p, sizeof(msghdr_p->um_type));
	p += sizeof(msghdr_p->um_type);

	memcpy(&msghdr_p->um_len, p, sizeof(msghdr_p->um_len));
	p += sizeof(msghdr_p->um_len);

	memcpy(&msghdr_p->um_dlen, p, sizeof(msghdr_p->um_dlen));
	p += sizeof(msghdr_p->um_dlen);

	memcpy(&msghdr_p->um_seq, p, sizeof(msghdr_p->um_seq));
	p += sizeof(msghdr_p->um_seq);

	memcpy(&msghdr_p->um_dseq, p, sizeof(msghdr_p->um_dseq));
	p += sizeof(msghdr_p->um_dseq);

	memcpy(&msghdr_p->um_dummy, p, sizeof(msghdr_p->um_dummy));
	p += sizeof(msghdr_p->um_dummy);

	*len -= UMSG_TX_HEADER_LEN;

	return p;
}

uint8_t
__tx_get_pkg_level(struct umessage_tx_hdr *msg_hdr) {
	return msg_hdr->um_level;
}
