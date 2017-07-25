/*
 * mgrrc.c
 * 	Implementation of Manager to rc Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_crc_if.h"
#include "mgrcrc_if.h"
#include "nid_shared.h"

#define RC_FREESPACE_DIST	1
#define RC_FREESPACE		2
#define RC_BSN			3
#define RC_CHECK_FP		4
#define RC_CSE_HIT		5
#define RC_DSREC_STAT		6
#define RC_SP_HEADS_SIZE	7

#define UM_HEADER_SIZE		6
#define UM_BSN_PACKET_SIZE	24
#define UM_BSN_END_PKT_SIZE	6

typedef void (*stat_unpack_func_t)(struct umessage_crc_hdr *);
stat_unpack_func_t rc_stat_unpack[NID_REQ_MAX];

struct mgrcrc_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_CRC;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d",
			ipstr, port, errno);
	} else {
		if (util_nw_write_two_byte(sfd, chan_type) != NID_SIZE_CTYPE_MSG) {
			nid_log_error("%s: failed to send chan_type, errno:%d", log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static void
_get_freespace_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_freespace_info_stat";
  	struct umessage_crc_information_resp_freespace* rc_fs_msg = (struct umessage_crc_information_resp_freespace*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	printf( "freespace for rc = %lu\n", rc_fs_msg->um_num_blocks_free );

}

static void
_get_sp_heads_size_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_sp_heads_size_stat";
  	struct umessage_crc_information_resp_sp_heads_size* rc_fs_msg = (struct umessage_crc_information_resp_sp_heads_size*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	int i;
	for (i=0; i<DSMGR_ALL_SP_HEADS; i++) {
		if (rc_fs_msg->um_sp_heads_size[i] != 0) {
			printf( "sp heads size: %d number: %d\n", i, rc_fs_msg->um_sp_heads_size[i]);
		}
	}
}

static void
_get_cse_hit_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_cse_hit_stat";
  	struct umessage_crc_information_resp_cse_hit* rc_hc_msg = (struct umessage_crc_information_resp_cse_hit*)rc_msg;
	nid_log_warning("%s: start...", log_header );
	double hit_ratio = 0;
	double total_hit = 0;
	total_hit = (double)(rc_hc_msg->um_rc_hit_counter + rc_hc_msg->um_rc_unhit_counter);

	if(total_hit > 0){
		hit_ratio = ((double)(rc_hc_msg->um_rc_hit_counter) * 100.0 ) / total_hit;
		printf( "hit counter of cse = %lu\n", rc_hc_msg->um_rc_hit_counter );
		printf( "unhit counter of cse = %lu\n", rc_hc_msg->um_rc_unhit_counter );
		printf( "hit counter of cse = %.2lf%%\n", hit_ratio);
	} else {
		printf( "cse did not work yet\n");
	}


}


static void
_get_dsrec_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_nse_hit_stat";
  	struct umessage_crc_information_resp_dsrec_stat* rc_dsrec_msg = (struct umessage_crc_information_resp_dsrec_stat*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	int i;
	for (i=0; i<NID_SIZE_FP_WGT; i++) {
		printf("lru weight: %d number: %lu\n", i, rc_dsrec_msg->um_dsrec_stat.p_lru_nr[i]);
	}
	printf("lru total number: %lu\n", rc_dsrec_msg->um_dsrec_stat.p_lru_total_nr);
	printf("lru max number: %lu\n", rc_dsrec_msg->um_dsrec_stat.p_lru_max);
	printf("lru reclaimed number: %lu\n", rc_dsrec_msg->um_dsrec_stat.p_rec_nr);
}

static void
_get_check_fp_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_check_fp_info_stat";
  	struct umessage_crc_information_resp_check_fp* rc_fs_msg = (struct umessage_crc_information_resp_check_fp*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	printf( "check_fp return code for rc = %d\n", (int)(rc_fs_msg->um_check_fp_rc) );

}

static void
_get_dsbmp_bsn_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "_get_freespace_info_stat";
  	struct umessage_crc_information_resp_dsbmp_bsn* rc_bsn_msg = (struct umessage_crc_information_resp_dsbmp_bsn*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	printf( "dsbmp node: offset: %lu, size: %lu\n", rc_bsn_msg->um_offset, rc_bsn_msg->um_size );

}

static void
_get_freespace_dist_stat( struct umessage_crc_hdr* rc_msg )
{
	char *log_header = "get_freespace_info_dist_stat_";
  	struct umessage_crc_information_resp_freespace_dist* rc_fs_msg = (struct umessage_crc_information_resp_freespace_dist*)rc_msg;
	nid_log_warning("%s: start...", log_header );

	//	int num_entries = rc_fs_msg->um_num_sizes;
	printf( "size      number of elements\n" );
	struct space_size_number* ssn = NULL, *temp_ssn = NULL;
	list_for_each_entry_safe( ssn, temp_ssn, struct space_size_number, &rc_fs_msg->um_size_list_head, um_ss_list ) {
	  	printf( "%lu      %lu\n", ssn->um_size, ssn->um_number );
		list_del( &ssn->um_ss_list );
		free( ssn );

	}

}

static int
mgrcrc_info_freespace( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_freespace";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr res, *msghdr = (struct umessage_crc_hdr *)&nid_msg;

	int sfd = -1;
	uint32_t len, resp_len;
	char msg_hdr[ 6 ];
	char buf[4096];
	int rc = 0, ctype = NID_CTYPE_CRC;
	char* msg_buf = NULL;


	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_FREE_SPACE;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {

	  //  resp_len = be32toh( *(uint32_t *)(msg_hdr + 4 ) );
		resp_len = *(uint32_t*)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, read from socket 4 bytes - still reading, resp_len = %d,no_be_conv=%d", log_header,rc_uuid, (int)resp_len, (int)resp_len );
	  	if ( resp_len > 6) {
	    		msg_buf = x_malloc( resp_len );
	    		if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
				nid_log_warning("%s: uuid:%s, read from socket -errno = %d", log_header,rc_uuid, (int)errno);
	      			rc = -1;
	      			goto out;
	    		}
			memcpy( msg_buf, msg_hdr , 6 );
		}
		nid_log_warning( "%s: after read - req = %d, req_code = %d", log_header, (int)msg_buf[ 0], (int)msg_buf[ 1] );
		nid_log_warning("%s: uuid:%s, read from socket %d bytes", log_header,rc_uuid, (int)resp_len);
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		rc_stat_unpack[RC_FREESPACE]( &res );
	}
	nid_log_warning("%s: uuid:%s, after read from socket", log_header,rc_uuid );
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_sp_heads_size( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_sp_heads_size";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	struct umessage_crc_information_resp_sp_heads_size res;

	int sfd = -1;
	uint32_t len, resp_len;
	char msg_hdr[ 6 ];
	char buf[4096];
	int rc = 0, ctype = NID_CTYPE_CRC;
	char* msg_buf = NULL;


	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_SP_HEADS_SIZE;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {

	  //  resp_len = be32toh( *(uint32_t *)(msg_hdr + 4 ) );
		resp_len = *(uint32_t*)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, read from socket 4 bytes - still reading, resp_len = %d,no_be_conv=%d", log_header,rc_uuid, (int)resp_len, (int)resp_len );
	  	if ( resp_len > 6) {
	    		msg_buf = x_malloc( resp_len );
	    		if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
				nid_log_warning("%s: uuid:%s, read from socket -errno = %d", log_header,rc_uuid, (int)errno);
	      			rc = -1;
	      			goto out;
	    		}
			memcpy( msg_buf, msg_hdr , 6 );
		}
		nid_log_warning( "%s: after read - req = %d, req_code = %d", log_header, (int)msg_buf[ 0], (int)msg_buf[ 1] );
		nid_log_warning("%s: uuid:%s, read from socket %d bytes", log_header,rc_uuid, (int)resp_len);
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		rc_stat_unpack[RC_SP_HEADS_SIZE]( (struct umessage_crc_hdr *)&res );
	}
	nid_log_warning("%s: uuid:%s, after read from socket", log_header,rc_uuid );
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_cse_hit( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_cse_hit";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr res, *msghdr = (struct umessage_crc_hdr *)&nid_msg;

	int sfd = -1;
	uint32_t len, resp_len;
	char msg_hdr[ 6 ];
	char buf[4096];
	int rc = 0, ctype = NID_CTYPE_CRC;
	char* msg_buf = NULL;


	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_CSE_HIT_CHECK;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {

	  //  resp_len = be32toh( *(uint32_t *)(msg_hdr + 4 ) );
		resp_len = *(uint32_t*)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, read from socket 4 bytes - still reading, resp_len = %d,no_be_conv=%d", log_header,rc_uuid, (int)resp_len, (int)resp_len );
	  	if ( resp_len > 6) {
	    		msg_buf = x_malloc( resp_len );
	    		if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
				nid_log_warning("%s: uuid:%s, read from socket -errno = %d", log_header,rc_uuid, (int)errno);
	      			rc = -1;
	      			goto out;
	    		}
			memcpy( msg_buf, msg_hdr , 6 );
		}
		nid_log_warning( "%s: after read - req = %d, req_code = %d", log_header, (int)msg_buf[ 0], (int)msg_buf[ 1] );
		nid_log_warning("%s: uuid:%s, read from socket %d bytes", log_header,rc_uuid, (int)resp_len);
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		rc_stat_unpack[RC_CSE_HIT]( &res );
	}
	//nid_log_warning("%s: uuid:%s, after read from socket", log_header,rc_uuid );
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_dsrec_stat(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, char *chan_uuid)
{
	char *log_header = "mgrcrc_info_dsrec_stat";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	struct umessage_crc_information_resp_dsrec_stat resp_msg;
	struct umessage_crc_hdr *resp_hdr = (struct umessage_crc_hdr *)&resp_msg;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_DSREC_STAT;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
	if (nread != UMSG_CRC_HEADER_LEN) {
		goto out;
	}
	p = buf;
	resp_hdr->um_req = *p++;
	resp_hdr->um_req_code = *p++;
	resp_hdr->um_len = *(uint32_t *)p;
	nid_log_debug ("%u", resp_hdr->um_req_code);
	assert(resp_hdr->um_req == UMSG_CRC_CMD_INFORMATION);
	assert(resp_hdr->um_req_code == UMSG_CRC_CODE_DSREC_STAT);
	assert(resp_hdr->um_len > UMSG_CRC_HEADER_LEN);

	nread = util_nw_read_n(sfd, buf + UMSG_CRC_HEADER_LEN, resp_hdr->um_len - UMSG_CRC_HEADER_LEN);
	if (nread != (int)resp_hdr->um_len - UMSG_CRC_HEADER_LEN) {
		goto out;
	}

	umpk_p->um_op->um_decode(umpk_p, buf, resp_hdr->um_len, NID_CTYPE_CRC, (void*)resp_hdr);
	rc_stat_unpack[RC_DSREC_STAT]( resp_hdr );
	nid_log_warning("%s: uuid:%s, after read from socket", log_header,rc_uuid );

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_check_fp( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_check_fp";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr res, *msghdr = (struct umessage_crc_hdr *)&nid_msg;

	int sfd = -1;
	uint32_t len, resp_len;
	char msg_hdr[ 5 ];
	char buf[4096];
	int rc = 0, ctype = NID_CTYPE_CRC;
	char* msg_buf = NULL;


	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_CHECK_FP;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {

	  //  resp_len = be32toh( *(uint32_t *)(msg_hdr + 4 ) );
		resp_len = *(uint32_t*)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, read from socket 4 bytes - still reading, resp_len = %d,no_be_conv=%d", log_header,rc_uuid, (int)resp_len, (int)resp_len );
	  	if ( resp_len > 6) {
	    		msg_buf = x_malloc( resp_len );
	    		if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
				nid_log_warning("%s: uuid:%s, read from socket -errno = %d", log_header,rc_uuid, (int)errno);
	      			rc = -1;
	      			goto out;
	    		}
			memcpy( msg_buf, msg_hdr , 6 );
		}
		nid_log_warning( "%s: after read - req = %d, req_code = %d", log_header, (int)msg_buf[ 0], (int)msg_buf[ 1] );
		nid_log_warning("%s: uuid:%s, read from socket %d bytes", log_header,rc_uuid, (int)resp_len);
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		rc_stat_unpack[RC_CHECK_FP]( &res );
	}
	nid_log_warning("%s: uuid:%s, after read from socket", log_header,rc_uuid );
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrcrc_info_freespace_dist( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_freespace_dist";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;

	int sfd = -1;
	uint32_t len;
	char msg_hdr[ 5 ];
	int rc = 0, ctype = NID_CTYPE_CRC;
	uint32_t resp_len = 0;
	char buf[ 1028 ], *msg_buf;

	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_FREE_SPACE_DIST;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {
	  	resp_len = *(uint32_t *)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, after util_nw_read_n(), resp_len:%d", log_header, rc_uuid, (int)resp_len);

	  	if ( resp_len > 6 ) {
	    		msg_buf = x_malloc( resp_len );
	    		if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
	      			rc = -1;
	      			goto out;
	    		}
			memcpy( msg_buf, msg_hdr, 6 );
		}
		struct umessage_crc_information_resp_freespace_dist res;
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		// idx = msg_buf[ 0 ];
		rc_stat_unpack[RC_FREESPACE_DIST]( (struct umessage_crc_hdr*)(&res) );
	}
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


static int
mgrcrc_info_dsbmp_rtree( struct mgrcrc_interface *mgrcrc_p, char *rc_uuid )
{
	char *log_header = "mgrcrc_info_dsbmp_rtree";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;

	int sfd = -1;
	uint32_t len;
	char msg_hdr[ 5 ];
	int rc = 0, ctype = NID_CTYPE_CRC, len_read = 0;
	uint32_t resp_len = 0;
	char buf[ 1028 ], msg_buf[ 1028 ];

	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_DSBMP_RTREE;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	int done = 0;
	char req_code = 0;
	if ( util_nw_read_n(sfd, &msg_hdr, 6) == 6 ) {
		resp_len = *(uint32_t *)(msg_hdr + 2 );
		nid_log_warning("%s: uuid:%s, after util_nw_read_n(), resp_len:%d", log_header, rc_uuid, (int)resp_len);
  		if ( ( resp_len > 6 ) && ( resp_len < 1000 ) ) {
		  	memset( msg_buf, 0, 1028 * sizeof( char ) );
    			if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
      				rc = -1;
      				goto out;
    			}
			memcpy( msg_buf, msg_hdr, 6 );
			req_code = msg_buf[ 1 ] ;
		}
		nid_log_warning( "%s: after read - req = %d, req_code = %d", log_header, (int)msg_buf[ 0], (int)req_code );
		nid_log_warning("%s: uuid:%s, read from socket %d bytes", log_header,rc_uuid, (int)resp_len );
		struct umessage_crc_information_resp_dsbmp_rtree_node res;
		umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
		printf( "offset		size\n" );
		printf( "-------------------\n" );
	}

	// begin reading the block size nodes of the dsbmp rtree
	while ( ! done ) {
	  	len_read = util_nw_read_n(sfd, &msg_hdr, UM_HEADER_SIZE);
		if ( len_read  == UM_HEADER_SIZE ) {
	  		resp_len = *(uint32_t *)(msg_hdr + 2 );
			nid_log_warning("%s: uuid:%s, after util_nw_read_n(), resp_len:%d", log_header, rc_uuid, (int)resp_len);
			memset( msg_buf, 0, 1028 * sizeof( char ) );
	  		if ( ( resp_len > 6 ) && ( resp_len < 1000 ) ) {
	    			if ( util_nw_read_n( sfd, msg_buf + 6, resp_len - 6 ) < 0 ) {
	      				rc = -1;
	      				goto out;
	    			}
				memcpy( msg_buf, msg_hdr, 6 );
			} else if  ( resp_len == 6 ) {
			  	char req = *(char*)(msg_hdr);
			  	req_code = *(char*)(msg_hdr +1);
				if ( ( req_code == UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT ) && ( req == UMSG_CRC_CMD_INFORMATION ) ) {
				  	nid_log_warning( "%s: uuid: %s, read the DSBMP_RTREE_END_RSLT", log_header, rc_uuid );
					goto out;
				} else  {
				  	nid_log_warning( "%s: uuid: %s, received invalid packet", log_header, rc_uuid );
					rc = -1;
					goto out;
				}
			} else {
			  	nid_log_warning( "%s: uuid: %s, received invalid packet", log_header, rc_uuid );
				rc = -1;
				goto out;
			}

			struct umessage_crc_information_resp_dsbmp_bsn res;
			umpk_p->um_op->um_decode( umpk_p, msg_buf, resp_len, NID_CTYPE_CRC, (void*)&res );
			rc_stat_unpack[RC_BSN]( (struct umessage_crc_hdr*)(&res) );
			printf( "%lu	%lu\n", res.um_offset, res.um_size );
		} else if ( len_read == 6 ) {
		  	// this could be the finish packet
			resp_len = *(uint32_t *)(msg_hdr + 2 );
			nid_log_warning("%s: uuid:%s, after util_nw_read_n(), resp_len:%d", log_header, rc_uuid, (int)resp_len);
			if (resp_len != 6 ) {

			}
		}

	}
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_nse_stat(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, char *chan_uuid)
{
	char *log_header = "mgrcrc_info_nse_counter";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	struct umessage_crc_information_resp_nse_stat resp_msg;
	struct umessage_crc_hdr *resp_hdr = (struct umessage_crc_hdr *)&resp_msg;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_NSE_STAT;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
	if (nread != UMSG_CRC_HEADER_LEN) {
		goto out;
	}
	p = buf;
	resp_hdr->um_req = *p++;
	resp_hdr->um_req_code = *p++;
	resp_hdr->um_len = *(uint32_t *)p;
	assert(resp_hdr->um_req == UMSG_CRC_CMD_INFORMATION);
	assert(resp_hdr->um_req_code == UMSG_CRC_CODE_RESP_NSE_STAT);
	assert(resp_hdr->um_len > UMSG_CRC_HEADER_LEN);

	nread = util_nw_read_n(sfd, buf + UMSG_CRC_HEADER_LEN, resp_hdr->um_len - UMSG_CRC_HEADER_LEN);
	if (nread != (int)resp_hdr->um_len - UMSG_CRC_HEADER_LEN) {
		goto out;
	}

	umpk_p->um_op->um_decode(umpk_p, buf, resp_hdr->um_len, NID_CTYPE_CRC, (void*)resp_hdr);

	printf("rc_uuid:%s\n", resp_msg.um_rc_uuid);
	printf("chan_uuid:%s\n", resp_msg.um_chan_uuid);
	printf("nse_stat:\n");
	printf("\tmax fp num:%lu\n", resp_msg.um_nse_stat.fp_max_num);
	printf("\tmin fp num:%lu\n", resp_msg.um_nse_stat.fp_min_num);
	printf("\tcur fp num:%lu\n", resp_msg.um_nse_stat.fp_num);
	printf("\trec fp num:%lu\n", resp_msg.um_nse_stat.fp_rec_num);
	printf("\thit num:%d\n", resp_msg.um_nse_stat.hit_num);
	printf("\tunhit num:%d\n", resp_msg.um_nse_stat.unhit_num);
	printf("\thit ratio:%.2lf%%\n", resp_msg.um_nse_stat.hit_num * 100.0 /
			(resp_msg.um_nse_stat.hit_num + resp_msg.um_nse_stat.unhit_num));
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_info_nse_detail(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, char *chan_uuid)
{
	char *log_header = "mgrcrc_info_nse_detail";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_information nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	struct umessage_crc_information_resp_nse_detail  resp_msg;
	struct umessage_crc_hdr *resp_hdr = (struct umessage_crc_hdr *)&resp_msg;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int i, nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
	msghdr->um_req_code = UMSG_CRC_CODE_NSE_DETAIL;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while (1) {
		memset(&resp_msg, 0, sizeof(resp_msg));
		nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
		if (nread != UMSG_CRC_HEADER_LEN) {
			goto out;
		}
		p = buf;
		resp_hdr->um_req = *p++;
		resp_hdr->um_req_code = *p++;
		resp_hdr->um_len = *(uint32_t *)p;
		assert(resp_hdr->um_req == UMSG_CRC_CMD_INFORMATION);
		assert(resp_hdr->um_req_code == UMSG_CRC_CODE_RESP_NSE_DETAIL);
		assert(resp_hdr->um_len > UMSG_CRC_HEADER_LEN);

		nread = util_nw_read_n(sfd, buf + UMSG_CRC_HEADER_LEN, resp_hdr->um_len - UMSG_CRC_HEADER_LEN);
		if (nread != (int)resp_hdr->um_len - UMSG_CRC_HEADER_LEN) {
			goto out;
		}

		resp_msg.um_flags = 0;
		umpk_p->um_op->um_decode(umpk_p, buf, resp_hdr->um_len, NID_CTYPE_CRC, (void*)resp_hdr);
		if (resp_msg.um_flags & UMSG_CRC_FLAG_START) {
			printf("rc_uuid:%s\n", resp_msg.um_rc_uuid);
			printf("chan_uuid:%s\n", resp_msg.um_chan_uuid);
			printf("nse_stat:\n");
			printf("\tmax fp num:%lu\n", resp_msg.um_nse_stat.fp_max_num);
			printf("\tmin fp num:%lu\n", resp_msg.um_nse_stat.fp_min_num);
			printf("\tcur fp num:%lu\n", resp_msg.um_nse_stat.fp_num);
			printf("\trec fp num:%lu\n", resp_msg.um_nse_stat.fp_rec_num);
			printf("\thit num:%d\n", resp_msg.um_nse_stat.hit_num);
			printf("\tunhit num:%d\n", resp_msg.um_nse_stat.unhit_num);
			printf("\thit ratio:%.2lf%%\n", resp_msg.um_nse_stat.hit_num * 100.0 /
					(resp_msg.um_nse_stat.hit_num + resp_msg.um_nse_stat.unhit_num));
			continue;
		} else if (resp_msg.um_flags & UMSG_CRC_FLAG_END) {
			break;
		}

		printf("block_index:%lu, fp:", resp_msg.um_block_index);
		for (i = 0; i < NID_SIZE_FP; i++) {
			printf("%x ", (uint8_t)resp_msg.um_fp[i]);
		}
		printf("\n");
	}
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_dropcache_start(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, char *chan_uuid, int do_sync)
{
	char *log_header = "mgrcrc_dropcache_start";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_dropcache nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, chan_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, chan_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_DROPCACHE;
	if (!do_sync)
		msghdr->um_req_code = UMSG_CRC_CODE_START;
	else
		msghdr->um_req_code = UMSG_CRC_CODE_START_SYNC;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, chan_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_dropcache_stop(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, char *chan_uuid)
{
	char *log_header = "mgrcrc_dropcache_stop";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_dropcache nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	char buf[4096];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_DROPCACHE;
	msghdr->um_req_code = UMSG_CRC_CODE_STOP;
	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_chan_uuid, chan_uuid);
	nid_msg.um_chan_uuid_len = strlen(chan_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_setwgt(struct mgrcrc_interface *mgrcrc_p, char *rc_uuid, int wgt)
{
	char *log_header = "mgrcrc_setwgt";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_setwgt nid_msg;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	char buf[1024];
	int sfd = -1;
	uint32_t len;
	char nothing;
	int rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_SETWGT;
	msghdr->um_req_code = UMSG_CRC_CODE_WGT;

	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	nid_msg.um_wgt = wgt;

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_add(struct mgrcrc_interface *mgrcrc_p,
		char *rc_uuid, char *pp_name, char *cache_device, int cache_size)
{
	char *log_header = "mgrcrc_add";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_add nid_msg;
	struct umessage_crc_add_resp nid_msg_resp;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&nid_msg;
	char *p, buf[1024];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_warning("%s: start (uuid:%s)...", log_header, rc_uuid);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	nid_log_warning("%s: step 1 (uuid:%s)...", log_header, rc_uuid);
	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr->um_req = UMSG_CRC_CMD_ADD;
	msghdr->um_req_code = UMSG_CRC_CODE_ADD;

	strcpy(nid_msg.um_rc_uuid, rc_uuid);
	nid_msg.um_rc_uuid_len = strlen(rc_uuid);
	strcpy(nid_msg.um_pp_name, pp_name);
	nid_msg.um_pp_name_len = strlen(pp_name);
	strcpy(nid_msg.um_cache_device, cache_device);
	nid_msg.um_cache_device_len = strlen(cache_device);
	nid_msg.um_cache_size = cache_size;

	nid_log_warning("%s: step 2 (uuid:%s)...", log_header, rc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	assert(len == msghdr->um_len);
	nid_log_warning("%s: uuid:%s, len:%d, plen:%d", log_header, rc_uuid, msghdr->um_len, *(uint32_t *)(buf+2));
	write(sfd, buf, len);

	msghdr = (struct umessage_crc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
	if (nread != UMSG_CRC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_CRC_CMD_ADD);
	assert(msghdr->um_req_code == UMSG_CRC_CODE_RESP_ADD);
	assert(msghdr->um_len >= UMSG_CRC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_CRC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CRC, msghdr);
	if (!nid_msg_resp.um_resp_code)
		printf("%s: add crc successfully\n",log_header);
	else
		printf("%s: add crc failed\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_display(struct mgrcrc_interface *mgrcrc_p, char *crc_uuid, uint8_t is_setup)
{
	char *log_header = "mgrcrc_display";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_display nid_msg;
	struct umessage_crc_display_resp nid_msg_resp;
	struct umessage_crc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_crc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_CRC_CODE_S_DISP;
	else
		msghdr->um_req_code = UMSG_CRC_CODE_W_DISP;
	strcpy(nid_msg.um_rc_uuid, crc_uuid);
	nid_msg.um_rc_uuid_len = strlen(crc_uuid);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
	msghdr = (struct umessage_crc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
	if (nread != UMSG_CRC_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, resp_len:%d, done\n",
		log_header, msghdr->um_req, msghdr->um_len);

	assert(msghdr->um_req == UMSG_CRC_CMD_DISPLAY);
	if (is_setup)
		assert(msghdr->um_req_code == UMSG_CRC_CODE_S_RESP_DISP);
	else
		assert(msghdr->um_req_code == UMSG_CRC_CODE_W_RESP_DISP);
	assert(msghdr->um_len >= UMSG_CRC_HEADER_LEN);
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_CRC_HEADER_LEN);
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CRC, msghdr);

	printf("crc %s:\n\tcache_device: %s\n\tcache_size: %d\n\tpp_name: %s\n\tblock_size: %ld\n",
		nid_msg_resp.um_rc_uuid, nid_msg_resp.um_cache_device, nid_msg_resp.um_cache_size, nid_msg_resp.um_pp_name, nid_msg_resp.um_block_size);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_display_all(struct mgrcrc_interface *mgrcrc_p, uint8_t is_setup)
{
	char *log_header = "mgrcrc_display_all";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_display nid_msg;
	struct umessage_crc_display_resp nid_msg_resp;
	struct umessage_crc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len;
	int nread, rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_crc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
	if (is_setup)
		msghdr->um_req_code = UMSG_CRC_CODE_S_DISP_ALL;
	else
		msghdr->um_req_code = UMSG_CRC_CODE_W_DISP_ALL;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	while(1) {
		memset(&nid_msg_resp, 0, sizeof(nid_msg_resp));
		msghdr = (struct umessage_crc_hdr *)&nid_msg_resp;
		nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
		if (nread != UMSG_CRC_HEADER_LEN) {
			rc = -1;
			goto out;
		}

		p = buf;
		msghdr->um_req = *p++;
		msghdr->um_req_code = *p++;
		msghdr->um_len = *(uint32_t *)p;
		p += 4;
		nid_log_debug("%s: req:%d, resp_len:%d, done\n",
			log_header, msghdr->um_req, msghdr->um_len);

		if (msghdr->um_req_code == UMSG_CRC_CODE_DISP_END)
			goto out;

		assert(msghdr->um_req == UMSG_CRC_CMD_DISPLAY);
		if(is_setup)
			assert(msghdr->um_req_code == UMSG_CRC_CODE_S_RESP_DISP_ALL);
		else
			assert(msghdr->um_req_code == UMSG_CRC_CODE_W_RESP_DISP_ALL);
		assert(msghdr->um_len >= UMSG_CRC_HEADER_LEN);
		nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_CRC_HEADER_LEN);
		umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CRC, msghdr);

		printf("crc %s:\n\tcache_device: %s\n\tcache_size: %d\n\tpp_name: %s\n\tblock_size: %ld\n",
			nid_msg_resp.um_rc_uuid, nid_msg_resp.um_cache_device, nid_msg_resp.um_cache_size, nid_msg_resp.um_pp_name, nid_msg_resp.um_block_size);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mgrcrc_hello(struct mgrcrc_interface *mgrcrc_p)
{
	char *log_header = "mgrcrc_hello";
	struct mgrcrc_private *priv_p = mgrcrc_p->mcr_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct umessage_crc_hello nid_msg, nid_msg_resp;
	struct umessage_crc_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_CRC;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_crc_hdr *)&nid_msg;
	msghdr->um_req = UMSG_CRC_CMD_HELLO;
	msghdr->um_req_code = UMSG_CRC_CODE_HELLO;
	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_crc_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_CRC_HEADER_LEN);
	if (nread != UMSG_CRC_HEADER_LEN) {
		printf("module crc is not supported.\n");
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_CRC_CMD_HELLO);
	assert(msghdr->um_req_code == UMSG_CRC_CODE_RESP_HELLO);
	assert(msghdr->um_len >= UMSG_CRC_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_CRC_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_CRC_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_CRC, msghdr);
	printf("module crc is supported.\n");
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrcrc_operations mgrcrc_op = {
	.mcr_dropcache_start = mgrcrc_dropcache_start,
	.mcr_dropcache_stop = mgrcrc_dropcache_stop,
	.mcr_set_wgt = mgrcrc_setwgt,
	.mcr_info_freespace = mgrcrc_info_freespace,
	.mcr_info_sp_heads_size = mgrcrc_info_sp_heads_size,
	.mcr_info_freespace_dist = mgrcrc_info_freespace_dist,
	.mcr_info_nse_stat = mgrcrc_info_nse_stat,
	.mcr_info_nse_detail = mgrcrc_info_nse_detail,
	.mcr_info_dsbmp_rtree = mgrcrc_info_dsbmp_rtree,
	.mcr_info_check_fp = mgrcrc_info_check_fp,
	.mcr_info_cse_hit = mgrcrc_info_cse_hit,
	.mcr_info_dsrec_stat = mgrcrc_info_dsrec_stat,
	.mcr_add = mgrcrc_add,
	.mcr_display = mgrcrc_display,
	.mcr_display_all = mgrcrc_display_all,
	.mcr_hello = mgrcrc_hello,
};

int
mgrcrc_initialization(struct mgrcrc_interface *mgrcrc_p, struct mgrcrc_setup *setup)
{
	char *log_header = "mgrcrc_initialization";
	struct mgrcrc_private *priv_p;

	nid_log_debug("%s: start ...", log_header);
	mgrcrc_p->mcr_op = &mgrcrc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrcrc_p->mcr_private = priv_p;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	rc_stat_unpack[RC_FREESPACE] = _get_freespace_stat;
	rc_stat_unpack[RC_CHECK_FP] = _get_check_fp_stat;
	rc_stat_unpack[RC_FREESPACE_DIST] = _get_freespace_dist_stat;
	rc_stat_unpack[RC_BSN] = _get_dsbmp_bsn_stat;
	rc_stat_unpack[RC_CSE_HIT] = _get_cse_hit_stat;
	rc_stat_unpack[RC_DSREC_STAT] = _get_dsrec_stat;
	rc_stat_unpack[RC_SP_HEADS_SIZE] = _get_sp_heads_size_stat;
	return 0;
}
