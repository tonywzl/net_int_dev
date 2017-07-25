/*
 * mgrdoa.c
 *
 *  Implementation of Manager to Delegation of Authority  Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_doa_if.h"
#include "mgrdoa_if.h"
#include "nid_shared.h"
#include "util_nw.h"

#define DOA_TIMEOUT 15

struct mgrdoa_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;

};
#if 0
static int
read_n(int fd, void *buf, int n)
{
	char *p = buf;
	int nread , left = n;

	while (left > 0) {
		nread = read(fd, p, left);
		if (nread <= 0) {
			nid_log_error("read_n: fd:%d, nread:%d, errno:%d", fd, nread, errno);
			return -1;
		}
		left -= nread;
		p += nread;
	}
	return n;
}
#endif

static int
make_connection(char *ipstr, u_short port)
{
	char *log_header = " make_connection";
	//struct sockaddr_in saddr;
	int sfd, nwrite = 0;
	uint16_t chan_type = NID_CTYPE_DOA;

	nid_log_debug("%s: start (ip:%s port:%d)",log_header, ipstr, port);
/*
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	sfd = socket(AF_INET, SOCK_STREAM, 0);
*/
	sfd = util_nw_make_connection(ipstr, port);
	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d",ipstr, port, errno);

	} else {
		nid_log_info("make_connection successful!!!");
		nwrite = util_nw_write_two_byte_timeout(sfd, chan_type, DOA_TIMEOUT);
		if (nwrite == NID_SIZE_CTYPE_MSG) {
			nid_log_debug("sending chan_type(SAC_CTYPE_ADM) successful!!!");
		} else {
			nid_log_error("%s: failed to send chan_type, errno:%d",log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

/*
static int
mgrdoa_stop(struct mgrdoa_interface *mgrdoa_p)
{
	struct mgrdoa_private *priv_p = mgrdoa_p->md_private;
	int sfd = -1, rc = 0;
	char mgrdoa_cmd = NID_REQ_STOP;
	char nothing;

	nid_log_debug("mgrdoa_stop %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &mgrdoa_cmd, 1);
	read(sfd, &nothing, 1);
	printf("nids stop successfully\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}
*/

static int
_get_doa_stat_check(struct umessage_doa_information *doa_msg)
{
	nid_log_debug("_get_doa_stat_check");
	char *dent = "   ";
	struct umessage_doa;
	char vid[1024];
	char lid[1024];

	memset(vid, 0, 1024);
	memset(lid, 0, 1024);
	memcpy(vid, doa_msg->um_doa_vid, doa_msg->um_doa_vid_len);
	memcpy(lid, doa_msg->um_doa_lid, doa_msg->um_doa_lid_len);
	if ( doa_msg->um_doa_lock ){
		printf("search lock successfully\n");
		printf("DOA INFORMATION:\n");
			printf("%s lock id: {%s}\n", dent, lid);
			printf("%s owner of the lock: {%s}\n", dent,vid);
			printf("%s hold time: %u\n", dent, doa_msg->um_doa_hold_time);
			printf("%s time out: %u\n", dent, doa_msg->um_doa_time_out);
		return 0;
	} else {
		printf("search lock failed\n");
		return 1;
	}

}

static int
_get_doa_stat_release(struct umessage_doa_information *nid_msg)
{
//	nid_log_debug("_get_doa_stat_release");
//	char *dent = "   ";
	if ( nid_msg->um_doa_lock ){
		printf("release lock successfully\n");
		return 0;
	} else {
		printf("release lock failed\n");
		return 1;
	}

}

static int
_get_doa_stat(struct umessage_doa_information *nid_msg)
{
	int rc;
	nid_log_debug("_get_doa_stat");
	char *dent = "   ";
	char vid[1024];
	char lid[1024];

	memset(vid, 0, 1024);
	memset(lid, 0, 1024);
	memcpy(vid, nid_msg->um_doa_vid, nid_msg->um_doa_vid_len);
	memcpy(lid, nid_msg->um_doa_lid, nid_msg->um_doa_lid_len);
	if ( nid_msg->um_doa_lock ){
		printf("grep lock successfully\n");
		rc = 0;
	} else {
		printf("grep lock failed\n");
		rc = 1;
	}
	printf("DOA INFORMATION:\n");
	printf("%s lock id: {%s}\n", dent, lid);
	printf("%s owner of the lock: {%s}\n", dent, vid);
	printf("%s hold time: %u\n", dent, nid_msg->um_doa_hold_time);
	printf("%s time out: %u\n", dent, nid_msg->um_doa_time_out);
	return rc;
}

static int
__mgrdoa_process (struct mgrdoa_private *priv_p, struct umessage_doa_information *doa_msg)
{
	char *log_header = "__mgrdoa_process";
	struct umessage_doa_hdr * msghdr = &(doa_msg->um_header);
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int rc =0, sfd = -1;
	uint32_t nwrite, nread;
	uint32_t len;

	char *p, buf[4096];
	int ctype = NID_CTYPE_DOA;
	nid_log_debug("%s:start ....",log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
		if(sfd < 0){
			rc = -1;
			nid_log_error("%s: cannot make connection", log_header);
			goto out;
		}

	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype,  (void *) doa_msg);
	assert(len == msghdr->um_len);



	nwrite= util_nw_write_timeout(sfd, buf, len, DOA_TIMEOUT);
	if (nwrite != len) {
		nid_log_error("%s: cannot write to the nid server",log_header);
		rc = -1;
		goto out;
	}
	memset(doa_msg, 0, sizeof(struct umessage_doa_information));
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	nread = util_nw_read_n_timeout(sfd, buf, UMSG_DOA_HEADER_LEN, DOA_TIMEOUT);
	if (nread == 6){
		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t*)p;
		p += 4;
		if (msghdr->um_len > 6){
			//msg_buf = x_malloc(resp_len);
			nread = util_nw_read_n_timeout(sfd, p, msghdr->um_len - UMSG_DOA_HEADER_LEN, DOA_TIMEOUT);
			if (nread < (msghdr->um_len - UMSG_DOA_HEADER_LEN)){
				rc = -1;
				nid_log_error("%s: cannot read from sfd", log_header);
				goto out;

			}

		}
	//	nid_log_error("%s: test mgrdoa.c", log_header);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, ctype, (void *)doa_msg);

		nid_log_error ("%s: vid:%s, vid_len:%u, lid:%s, lid_len:%u, hold_time:%u, get_lck:%u",
							log_header, doa_msg->um_doa_vid, doa_msg->um_doa_vid_len, doa_msg->um_doa_lid, doa_msg->um_doa_lid_len, doa_msg->um_doa_hold_time, doa_msg->um_doa_lock );
	} else {
		rc = -1;
	}

out:
	if (sfd > 0)
		close(sfd);
	return rc;

}

static int
mgrdoa_doa_request(struct mgrdoa_interface *mgrdoa_p, struct umessage_doa_information *doa_msg)
{
	char *log_header = "mgrdoa_doa_request";
	struct umessage_doa_hdr * msghdr = &(doa_msg->um_header);
	struct mgrdoa_private *priv_p = mgrdoa_p->md_private;

	int rc =0;
	nid_log_debug("%s:start ....",log_header);
	msghdr->um_req = UMSG_DOA_CMD;
	msghdr->um_req_code = UMSG_DOA_CODE_REQUEST;



	rc = __mgrdoa_process(priv_p, doa_msg);
	if (rc < 0){
		return rc;
	}

	rc =_get_doa_stat(doa_msg);


return rc;

}

static int
mgrdoa_doa_release(struct mgrdoa_interface *mgrdoa_p, struct umessage_doa_information *doa_msg)
{
	char *log_header = "mgrdoa_doa_release";
	struct umessage_doa_hdr * msghdr = &(doa_msg->um_header);
	struct mgrdoa_private *priv_p = mgrdoa_p->md_private;

	int rc =0;
	nid_log_debug("%s:start ....",log_header);
	msghdr->um_req = UMSG_DOA_CMD;
	msghdr->um_req_code = UMSG_DOA_CODE_RELEASE;



	rc = __mgrdoa_process(priv_p, doa_msg);
	if (rc < 0){
		return rc;
	}

	rc =_get_doa_stat_release(doa_msg);


return rc;

}

static int
mgrdoa_doa_check(struct mgrdoa_interface *mgrdoa_p, struct umessage_doa_information *doa_msg)
{
	char *log_header = "mgrdoa_doa_check";
	struct umessage_doa_hdr * msghdr = &(doa_msg->um_header);
	struct mgrdoa_private *priv_p = mgrdoa_p->md_private;
	nid_log_debug("%s:start ....",log_header);
	int rc =0;

	msghdr->um_req = UMSG_DOA_CMD;
	msghdr->um_req_code = UMSG_DOA_CODE_CHECK;


	rc = __mgrdoa_process(priv_p, doa_msg);
	if (rc < 0){
		return rc;
	}



	rc =_get_doa_stat_check(doa_msg);

return rc;

}

static int
mgrdoa_doa_hello(struct mgrdoa_interface *mgrdoa_p)
{
	char *log_header = "mgrdoa_hello";
	struct mgrdoa_private *priv_p = mgrdoa_p->md_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_doa_hello nid_msg, nid_msg_resp;
	struct umessage_doa_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_DOA;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_doa_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DOA_CMD_HELLO;
	msghdr->um_req_code = UMSG_DOA_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_doa_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DOA_HEADER_LEN);
	if (nread != UMSG_DOA_HEADER_LEN) {
		printf("module doa is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_DOA_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_DOA_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_DOA_HEADER_LEN);

	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DOA_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_DOA_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_DOA, msghdr);
	printf("module doa is supported.\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrdoa_operations mgrdoa_op = {
	.md_doa_request = mgrdoa_doa_request,
	.md_doa_check = mgrdoa_doa_check,
	.md_doa_release = mgrdoa_doa_release,
	.md_doa_hello = mgrdoa_doa_hello,
};

int
mgrdoa_initialization(struct mgrdoa_interface *mgrdoa_p, struct mgrdoa_setup *setup)
{
	struct mgrdoa_private *priv_p;

	nid_log_debug("mgrdoa_initialization start ...");
	if (!setup) {
		nid_log_error("mgrdoa_initialization got null setup");
		return -1;
	}

	setup->ipstr = util_nw_getip(setup->ipstr);
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrdoa_p->md_private = priv_p;
	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;

	mgrdoa_p->md_op = &mgrdoa_op;
	return 0;
}
