/*
 * scg.c
 * 	Implementation of Service Channel Guardian Module
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "list.h"
#include "util_nw.h"
#include "nid_shared.h"
#include "nid.h"
#include "nid_shared.h"
#include "server.h"
#include "nid_log.h"
#include "allocator_if.h"
#include "lstn_if.h"
#include "brn_if.h"
#include "srn_if.h"
#include "mpk_if.h"
#include "tpg_if.h"
#include "ini_if.h"
#include "sac_if.h"
#include "smcg_if.h"
#include "wccg_if.h"
#include "crccg_if.h"
#include "bwccg_if.h"
#include "mrwcg_if.h"
#include "doacg_if.h"
#include "snapg_if.h"
#include "smc_if.h"
#include "io_if.h"
#include "bwcg_if.h"
#include "rc_if.h"
#include "ppg_if.h"
#include "cdsg_if.h"
#include "sdsg_if.h"
#include "scg_if.h"
#include "fpn_if.h"
#include "crcg_if.h"
#include "twcg_if.h"
#include "twccg_if.h"
#include "bfp_if.h"
#include "sacg_if.h"
#include "saccg_if.h"
#include "ppcg_if.h"
#include "sdscg_if.h"
#include "drwcg_if.h"
#include "cxag_if.h"
#include "cxacg_if.h"
#include "cxpg_if.h"
#include "cxpcg_if.h"
#include "cxtg_if.h"
#include "dxag_if.h"
#include "dxacg_if.h"
#include "dxpg_if.h"
#include "dxtg_if.h"
#include "dxpcg_if.h"
#include "pp2_if.h"
#include "tpcg_if.h"
#include "rwg_if.h"
#include "wcg_if.h"
#include "inicg_if.h"
#include "reptg_if.h"
#include "reptcg_if.h"
#include "repsg_if.h"
#include "repscg_if.h"
#include "umpk_if.h"
#include "umpk_server_if.h"
#include "version.h"
#include "mqtt_if.h"

#define __SCG_GUARDIAN_PPG	1
#define __SCG_GUARDIAN_TPG	2

#define __SCG_GUARDIAN_CXTG	3
#define __SCG_GUARDIAN_CXAG	4
#define __SCG_GUARDIAN_CXACG	5
#define __SCG_GUARDIAN_CXPG	6
#define __SCG_GUARDIAN_CXPCG	7

#define __SCG_GUARDIAN_DXTG	8
#define __SCG_GUARDIAN_DXAG	9
#define __SCG_GUARDIAN_DXACG	10
#define __SCG_GUARDIAN_DXPG	11
#define __SCG_GUARDIAN_DXPCG	12

#define __SCG_GUARDIAN_SDSG	13
#define __SCG_GUARDIAN_CDSG	14
#define	__SCG_GUARDIAN_SDSCG	15

#define	__SCG_GUARDIAN_CRCG	19

#define	__SCG_GUARDIAN_BWCCG	20
#define	__SCG_GUARDIAN_CRCCG	21


#define	__SCG_GUARDIAN_TWCCG	22


#define	__SCG_GUARDIAN_NWC0CG	23

#define	__SCG_GUARDIAN_WCCG	24

#define __SCG_GUARDIAN_WCG	25

#define __SCG_GUARDIAN_RWG	26

#define	__SCG_GUARDIAN_DRWCG	27

#define	__SCG_GUARDIAN_MRWCG	28

#define	__SCG_GUARDIAN_SMCG	29

#define	__SCG_GUARDIAN_SACG	30
#define	__SCG_GUARDIAN_SACCG	31

#define	__SCG_GUARDIAN_TPCG	32

#define	__SCG_GUARDIAN_DOACG	33

#define	__SCG_GUARDIAN_PPCG	34

#define	__SCG_GUARDIAN_SNAPG	35

#define __SCG_GUARDIAN_INICG	36

#define __SCG_GUARDIAN_REPTG	37
#define __SCG_GUARDIAN_REPSG	38

#define __SCG_GUARDIAN_REPTCG	39
#define __SCG_GUARDIAN_REPSCG	40

#define __SCG_GUARDIAN_MAX	41

struct scg_channel {
	struct list_head        s_list;
	uint16_t		s_type;		// channel type
	void			*s_chan;	// data for this communication channel
};


/* resource channel */
struct scg_rs_chain {
	struct list_head	r_list;
	struct list_head	r_head;
};

struct scg_private {
	struct list_head		p_rs_head;		// list of all channels organized per resource
	struct scg_rs_chain		p_rs_nodes[NID_MAX_RESOURCE];
	pthread_mutex_t			p_rs_lck;
	pthread_cond_t			p_rs_cond;
	pthread_mutex_t			p_pickup_lck;
	pthread_cond_t			p_pickup_cond;
	struct list_head		p_server_keys;
	struct ini_section_desc		*p_service_sections;

	struct allocator_interface	*p_allocator;
	struct lstn_interface		*p_lstn;
	struct txn_interface		*p_txn;
	struct brn_interface		p_brn;
	void				*p_guardians[__SCG_GUARDIAN_MAX];
	struct srn_interface		p_srn;
	struct mpk_interface		*p_mpk;
	struct umpk_interface		*p_umpk;
	struct tp_interface		*p_tp;
	struct tpg_interface		*p_tpg;
	struct ini_interface		*p_ini;
	struct fpn_interface 		*p_fpn;
	struct io_interface		*p_all_io[IO_TYPE_MAX];
	struct pp2_interface		*p_pp2;
	struct mqtt_interface		*p_mqtt;
	pthread_mutex_t			p_dis_lck;

	char				p_pickup_new;
};

/*
 * Algorithm:
 * 	The first byte of a new channel is always the channel type.
 * 	Based on received channel type, the function gives this type of channel a
 * 	chance to accept a new instance
 */
static void *
scg_accept_new_channel(struct scg_interface *scg_p, int sfd, char *cip)
{
	char *log_header = "scg_accept_new_channel";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct scg_channel *chan_p = NULL;
	int nread;
	char chan_type_buf[2], magic_buf[NID_SIZE_CONN_MAGIC + 1];
	char version_buf[NID_SIZE_VERSION];
	char magic[] = NID_CONN_MAGIC;
	uint16_t chan_type, proto_ver, my_proto_ver = NID_PROTOCOL_VER;
	uint8_t is_old_command = 0;

	nid_log_info("%s: establish channel from %s", log_header, cip);

	/*
	 * first of all, figure out the channel type
	 */
	nread = util_nw_read_n(sfd, &chan_type_buf[0], 1);
       	if (nread != 1) {
		nid_log_error("%s: read failed, nread:%d, errno:%d",
			log_header, nread, errno);
		goto out;
	}
	if (chan_type_buf[0] & 0x80) {
		nread = util_nw_read_n(sfd, &chan_type_buf[1], 1);
		if (nread != 1) {
			nid_log_error("%s: read failed, nread:%d, errno:%d",
				log_header, nread, errno);
			goto out;
		}

		if (chan_type_buf[1] == 'I'){
			magic_buf[0] = chan_type_buf[1];
			nread = util_nw_read_n(sfd, &magic_buf[1], NID_SIZE_CONN_MAGIC - 1);
			if (nread != NID_SIZE_CONN_MAGIC - 1) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
			magic_buf[NID_SIZE_CONN_MAGIC] = '\0';
			/* not valid NID message, close the socket*/
			if (strcmp(magic_buf, magic)) {
				close(sfd);
				goto out;
			}

			nread = util_nw_read_n(sfd, &proto_ver, NID_SIZE_PROTOCOL_VER);
			if (nread != NID_SIZE_PROTOCOL_VER) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
			/* will send magic and protocol version back, no matter this message is supported or not, so send it now */
			util_nw_write_n(sfd, &magic, NID_SIZE_CONN_MAGIC);
			util_nw_write_n(sfd, &my_proto_ver, NID_SIZE_PROTOCOL_VER);

			if (proto_ver > NID_PROTOCOL_RANGE_HIGH || proto_ver < NID_PROTOCOL_RANGE_LOW) {
				nid_log_warning("%s: the NID_PROTOCOL_VER (%d) is not supported by server (%s)",
					log_header, proto_ver, NID_VERSION);
				close(sfd);
				goto out;
			}

			nread = util_nw_read_n(sfd, &chan_type, 2);
			if (nread != NID_SIZE_PROTOCOL_VER) {
				nid_log_error("%s: read failed, nread:%d, errno:%d",
					log_header, nread, errno);
				goto out;
			}
		} else {
			chan_type = (uint8_t)chan_type_buf[1] + ((chan_type_buf[0] & 0x7f) << 8);
			nid_log_warning("%s: get 3.5.0 style command", log_header);
		}
		nid_log_warning("First receive: command:%u ", chan_type);
		/* get version informtion */
		nread = util_nw_read_n(sfd, version_buf, NID_SIZE_VERSION);
		if (nread != NID_SIZE_VERSION) {
			nid_log_error("%s: read failed, nread:%d, errno:%d",
				log_header, nread, errno);
			goto out;
		}
		nid_log_warning("%s: version:%d.%d.%d", log_header, (uint8_t)version_buf[0], (uint8_t)version_buf[1],*(uint16_t *)&version_buf[2]);
	} else {
		chan_type = chan_type_buf[0];
		is_old_command = 1;
		nid_log_warning("%s: get legacy command", log_header);
	}

	chan_p = calloc(1, sizeof(*chan_p));
	chan_p->s_type = chan_type;
	nid_log_warning("%s: get command:%u", log_header, chan_type);
	switch (chan_type) {
	case NID_CTYPE_ADM: {
		struct smcg_interface *smcg_p = priv_p->p_guardians[__SCG_GUARDIAN_SMCG];
		nid_log_debug("%s: a new admin channel", log_header);
		chan_p->s_chan = smcg_p->smg_op->smg_accept_new_channel(smcg_p, sfd, cip);
		break;
	}

	case NID_CTYPE_CXA: {
		struct cxacg_interface *cxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXACG];
		if(cxacg_p)
			chan_p->s_chan = cxacg_p->cxcg_op->cxcg_accept_new_channel(cxacg_p, sfd);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_CXP: {
		struct cxpcg_interface *cxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXPCG];
		if(cxpcg_p)
			chan_p->s_chan = cxpcg_p->cxcg_op->cxcg_accept_new_channel(cxpcg_p, sfd);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_DXA: {
		struct dxacg_interface *dxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXACG];
		if(dxacg_p)
			chan_p->s_chan = dxacg_p->dxcg_op->dxcg_accept_new_channel(dxacg_p, sfd);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_DXP: {
		struct dxpcg_interface *dxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXPCG];
		if(dxpcg_p)
			chan_p->s_chan = dxpcg_p->dxcg_op->dxcg_accept_new_channel(dxpcg_p, sfd);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_DOA: {
		struct doacg_interface *doacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DOACG];
		if(doacg_p)
			chan_p->s_chan = doacg_p->dg_op->dg_accept_new_channel(doacg_p, sfd, is_old_command);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_WC: {
		struct wccg_interface *wccg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCCG];
		if(wccg_p)
			chan_p->s_chan = wccg_p->wcg_op->wcg_accept_new_channel(wccg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_BWC: {
		struct bwccg_interface *bwccg_p = priv_p->p_guardians[__SCG_GUARDIAN_BWCCG];
		if(bwccg_p)
			chan_p->s_chan = bwccg_p->wcg_op->wcg_accept_new_channel(bwccg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_TWC: {
		struct twccg_interface *twccg_p = priv_p->p_guardians[__SCG_GUARDIAN_TWCCG];
		if(twccg_p)
			chan_p->s_chan = twccg_p->wcg_op->wcg_accept_new_channel(twccg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_MRW: {
		struct mrwcg_interface *mrwcg_p =  priv_p->p_guardians[__SCG_GUARDIAN_MRWCG];
		if(mrwcg_p)
			chan_p->s_chan = mrwcg_p->mrwcg_op->mrwcg_accept_new_channel(mrwcg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}


	case NID_CTYPE_CRC: {
		struct crccg_interface *crccg_p = priv_p->p_guardians[__SCG_GUARDIAN_CRCCG];
		if(crccg_p)
			chan_p->s_chan = crccg_p->crg_op->crg_accept_new_channel(crccg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_REG: {
		struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];

		/*
		 * Now we have async bwc recovery. The p_recovered of scg is no long used
		 * to check all if he bwcs are recovered and ready to accept channels.
		 * Instead, the recovered is checked per bwc after channel accepted.
		 */
		if(sacg_p)
			chan_p->s_chan = sacg_p->sag_op->sag_accept_new_channel(sacg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_SNAP: {
		struct snapg_interface *snapg_p = priv_p->p_guardians[__SCG_GUARDIAN_SNAPG];
		chan_p->s_chan = snapg_p->ssg_op->ssg_accept_new_channel(snapg_p, sfd);
		break;
	}

	case NID_CTYPE_TP: {
		struct tpcg_interface *tpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_TPCG];
		if(tpcg_p)
			chan_p->s_chan = tpcg_p->tcg_op->tcg_accept_new_channel(tpcg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_SAC: {
		struct saccg_interface *saccg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACCG];
		if (saccg_p)
			chan_p->s_chan = saccg_p->saccg_op->saccg_accept_new_channel(saccg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_SDS: {
		struct sdscg_interface *sdscg_p = priv_p->p_guardians[__SCG_GUARDIAN_SDSCG];
		if (sdscg_p)
			chan_p->s_chan = sdscg_p->sdscg_op->sdscg_accept_new_channel(sdscg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_PP: {
		struct ppcg_interface *ppcg_p = priv_p->p_guardians[__SCG_GUARDIAN_PPCG];
		if (ppcg_p)
			chan_p->s_chan = ppcg_p->ppcg_op->ppcg_accept_new_channel(ppcg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%u but according support is not configured!",
					log_header, chan_type);
		break;
	}

	case NID_CTYPE_DRW: {
		struct drwcg_interface *drwcg_p =  priv_p->p_guardians[__SCG_GUARDIAN_DRWCG];
		if (drwcg_p)
			chan_p->s_chan = drwcg_p->drwcg_op->drwcg_accept_new_channel(drwcg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%d but according support not configured!",
					log_header, NID_CTYPE_DRW);
		break;
	}

	case NID_CTYPE_INI: {
		struct inicg_interface *inicg_p = priv_p->p_guardians[__SCG_GUARDIAN_INICG];
		if (inicg_p)
			chan_p->s_chan = inicg_p->icg_op->icg_accept_new_channel(inicg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%d but according support not configured!",
					log_header, NID_CTYPE_INI);
		break;
	}

	case NID_CTYPE_REPT: {
		struct reptcg_interface *reptcg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPTCG];
		if (reptcg_p)
			chan_p->s_chan = reptcg_p->r_op->rtcg_accept_new_channel(reptcg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%d but according support not configured!",
					log_header, NID_CTYPE_REPT);
		break;
	}

	case NID_CTYPE_REPS: {
		struct repscg_interface *repscg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPSCG];
		if (repscg_p)
			chan_p->s_chan = repscg_p->r_op->rscg_accept_new_channel(repscg_p, sfd, cip);
		else
			nid_log_warning("%s: received channel type:%d but according support not configured!",
					log_header, NID_CTYPE_REPS);
		break;
	}

	case NID_CTYPE_SERVER: {
		nid_log_debug("%s: doing NID_CTYPE_SERVER", log_header);
		char msg_buf[64], *p = msg_buf, ver_buf[128];
		int rc;
		struct umessage_server_version nid_msg;
		struct umessage_server_version_resp resp_msg;
		struct umpk_interface *umpk_p = priv_p->p_umpk;
		uint32_t len;
		char nothing_back;

		util_nw_read_n(sfd, msg_buf, UMSG_SERVER_HEADER_LEN);
		nid_msg.um_header.um_req = *p++;
		nid_msg.um_header.um_req_code = *p++;
		nid_msg.um_header.um_len = *(uint32_t *)p;
		rc = umpk_p->um_op->um_decode(umpk_p, msg_buf,nid_msg.um_header.um_len, NID_CTYPE_SERVER, &nid_msg);
		if (rc)
			goto out;
		assert(nid_msg.um_header.um_req == UMSG_SERVER_CMD_VERSION);

		memset(&resp_msg, 0, sizeof(resp_msg));
		resp_msg.um_header.um_req = UMSG_SERVER_CMD_VERSION;
		resp_msg.um_header.um_req_code = UMSG_SERVER_CODE_RESP_VERSION;
		strcpy(ver_buf, NID_VERSION);
#ifdef HAS_MQTT
		strcat(ver_buf, "\nMQTT Enabled");
#endif
		resp_msg.um_version_len = strlen(ver_buf);
		memcpy(resp_msg.um_version, ver_buf, resp_msg.um_version_len);
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &len, NID_CTYPE_SERVER, &resp_msg);
		if (rc)
			goto out;
		write(sfd, msg_buf, len);
		read(sfd, &nothing_back, 1);

		chan_p = NULL;

		break;
	}

	case NID_CTYPE_ALL:{
		char buf = 0;
		pthread_mutex_lock(&priv_p->p_dis_lck);
		write(sfd, &buf, 1);
		read(sfd, &buf, 1);
		pthread_mutex_unlock(&priv_p->p_dis_lck);
		break;
	}

	default:
		nid_log_error("%s: got unknown channel type :%d", log_header, chan_type);
		close(sfd);
		free(chan_p);
		chan_p = NULL;
		break;
	}

out:
	if (chan_p && !chan_p->s_chan) {
		close(sfd);
		free(chan_p);
		chan_p = NULL;

	}
	return (void *)chan_p;
}

static void
scg_do_channel(struct scg_interface *scg_p, void *data)
{
	char *log_header = "scg_do_channel";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	struct scg_channel *chan_p = (struct scg_channel *)data;
	uint16_t chan_type = chan_p->s_type;

	nid_log_warning("%s: start (%p), do channel command:%u", log_header, data, chan_type);

	switch (chan_type) {
	case NID_CTYPE_ADM: {
		struct smcg_interface *smcg_p = priv_p->p_guardians[__SCG_GUARDIAN_SMCG];
		nid_log_debug("%s: do admin channel", log_header);
		smcg_p->smg_op->smg_do_channel(smcg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_CXA: {
		struct cxacg_interface *cxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXACG];
		cxacg_p->cxcg_op->cxcg_do_channel(cxacg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_CXP: {
		struct cxpcg_interface *cxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXPCG];
		cxpcg_p->cxcg_op->cxcg_do_channel(cxpcg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_DXA: {
		struct dxacg_interface *dxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXACG];
		dxacg_p->dxcg_op->dxcg_do_channel(dxacg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_DXP: {
		struct dxpcg_interface *dxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXPCG];
		dxpcg_p->dxcg_op->dxcg_do_channel(dxpcg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_WC: {
		struct wccg_interface *wccg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCCG];
		wccg_p->wcg_op->wcg_do_channel(wccg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_BWC: {
		struct bwccg_interface *bwccg_p = priv_p->p_guardians[__SCG_GUARDIAN_BWCCG];
		bwccg_p->wcg_op->wcg_do_channel(bwccg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_TWC: {
		struct twccg_interface *twccg_p = priv_p->p_guardians[__SCG_GUARDIAN_TWCCG];
		twccg_p->wcg_op->wcg_do_channel(twccg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_MRW: {
		struct mrwcg_interface *mrwcg_p = priv_p->p_guardians[__SCG_GUARDIAN_MRWCG];
		mrwcg_p->mrwcg_op->mrwcg_do_channel(mrwcg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_CRC: {
		struct crccg_interface *crccg_p = priv_p->p_guardians[__SCG_GUARDIAN_CRCCG];
		crccg_p->crg_op->crg_do_channel(crccg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_REG:
		nid_log_debug("%s: do regular channel", log_header);
		sacg_p->sag_op->sag_do_channel(sacg_p, chan_p->s_chan);
		break;

	case NID_CTYPE_DOA: {
		struct doacg_interface *doacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DOACG];
		doacg_p->dg_op->dg_do_channel(doacg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_SAC: {
		struct saccg_interface *saccg = priv_p->p_guardians[__SCG_GUARDIAN_SACCG];
		saccg->saccg_op->saccg_do_channel(saccg, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_SDS: {
		struct sdscg_interface *sdscg_p = priv_p->p_guardians[__SCG_GUARDIAN_SDSCG];
		sdscg_p->sdscg_op->sdscg_do_channel(sdscg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_PP: {
		struct ppcg_interface *ppcg_p = priv_p->p_guardians[__SCG_GUARDIAN_PPCG];
		ppcg_p->ppcg_op->ppcg_do_channel(ppcg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_DRW: {
		struct drwcg_interface *drwcg_p = priv_p->p_guardians[__SCG_GUARDIAN_DRWCG];
		drwcg_p->drwcg_op->drwcg_do_channel(drwcg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_TP: {
		struct tpcg_interface *tpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_TPCG];
		tpcg_p->tcg_op->tcg_do_channel(tpcg_p, chan_p->s_chan, scg_p);
		break;
	}

	case NID_CTYPE_INI: {
		struct inicg_interface *inicg_p = priv_p->p_guardians[__SCG_GUARDIAN_INICG];
		inicg_p->icg_op->icg_do_channel(inicg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_REPT: {
		struct reptcg_interface *reptcg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPTCG];
		reptcg_p->r_op->rtcg_do_channel(reptcg_p, chan_p->s_chan);
		break;
	}

	case NID_CTYPE_REPS: {
		struct repscg_interface *repscg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPSCG];
		repscg_p->r_op->rscg_do_channel(repscg_p, chan_p->s_chan);
		break;
	}

	default:
		nid_log_error("%s: got unknown channel type :%d", log_header, chan_type);
		break;
	}
	if (NID_CTYPE_REG != chan_type)
		free(chan_p->s_chan);
	free(chan_p);
}

static void
scg_get_stat(struct scg_interface *scg_p, struct list_head *out_head)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	struct ini_interface *ini_p = priv_p->p_ini;
	struct io_interface *io_p;
	struct io_stat *io_stat;
	struct smc_stat_record *sr_p;
	struct bwcg_interface *bwcg_p;
	struct twcg_interface *twcg_p;
	struct wcg_interface *wcg_p;


	nid_log_debug("scg_get_stat: start...");
	ini_p->i_op->i_display(ini_p);	// display configuration
	sacg_p->sag_op->sag_get_stat(sacg_p, out_head);

	if ((io_p = priv_p->p_all_io[IO_TYPE_RESOURCE])) {
		io_stat = calloc(1, sizeof(*io_stat));
		sr_p = calloc(1, sizeof(*sr_p));
		sr_p->r_type = NID_REQ_STAT_IO;
		sr_p->r_data = (void *)io_stat;
		list_add_tail(&sr_p->r_list, out_head);
		io_p->io_op->io_get_stat(io_p, io_stat);
	}

	if ((io_p = priv_p->p_all_io[IO_TYPE_BUFFER])) {
		wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
		bwcg_p = wcg_p->wg_op->wg_get_bwcg(wcg_p);
		if (bwcg_p) {
			bwcg_p->wg_op->wg_get_all_bwc_stat(bwcg_p, out_head);
		}
		twcg_p = wcg_p->wg_op->wg_get_twcg(wcg_p);
		if (twcg_p) {
			twcg_p->wg_op->wg_get_all_twc_stat(twcg_p, out_head);
		}
	}
}

static void
scg_reset_stat(struct scg_interface *scg_p)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];

	nid_log_debug("scg_reset_stat: start...");
	sacg_p->sag_op->sag_reset_stat(sacg_p);
}

static void
scg_update(struct scg_interface *scg_p)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	nid_log_debug("scg_update: start ...");
	sacg_p->sag_op->sag_update(sacg_p);
}

/*
 * return:
 * 	1: found the channel
 * 	0: otherwise
 */
static int
scg_check_conn(struct scg_interface *scg_p, char *uuid)
{
	char *log_header = "scg_check_conn";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	nid_log_debug("%s: start (uuid:%s)...", log_header, uuid);
	return sacg_p->sag_op->sag_check_conn(sacg_p, uuid);
}

static int
scg_upgrade_alevel(struct scg_interface *scg_p, struct sac_interface *target)
{
	char *log_header = "scg_upgrade_alevel";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	nid_log_debug("%s: start ...", log_header);
	return sacg_p->sag_op->sag_upgrade_alevel(sacg_p, target);
}

static int
scg_fupgrade_alevel(struct scg_interface *scg_p, struct sac_interface *target)
{
	char *log_header = "scg_fupgrade_alevel";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	nid_log_debug("%s: start ...", log_header);
	return sacg_p->sag_op->sag_fupgrade_alevel(sacg_p, target);
}

static int
scg_bio_fast_flush(struct scg_interface *scg_p, char *resource)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct bwcg_interface *bwcg_p;
	struct wcg_interface *wcg_p;

	assert(!resource);
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	bwcg_p = wcg_p->wg_op->wg_get_bwcg(wcg_p);
	if (bwcg_p) {
		bwcg_p->wg_op->wg_fflush_start_all(bwcg_p);
	}

	return 0;
}

static int
scg_bio_stop_fast_flush(struct scg_interface *scg_p, char *resource)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct bwcg_interface *bwcg_p;
	struct wcg_interface *wcg_p;

	assert(!resource);
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	bwcg_p = wcg_p->wg_op->wg_get_bwcg(wcg_p);
	if (bwcg_p) {
		bwcg_p->wg_op->wg_fflush_stop_all(bwcg_p);
	}

	return 0;
}

static int
scg_bio_vec_start(struct scg_interface *scg_p){
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct io_interface *io_p = priv_p->p_all_io[IO_TYPE_BUFFER];
	assert(io_p);
	return io_p->io_op->io_vec_start(io_p);

}

static int
scg_bio_vec_stop(struct scg_interface *scg_p){
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct io_interface *io_p = priv_p->p_all_io[IO_TYPE_BUFFER];
	assert(io_p);
	return io_p->io_op->io_vec_stop(io_p);

}

static void
scg_bio_vec_stat(struct scg_interface *scg_p, struct list_head *out_head){
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	char *log_header = "scg_bio_vec_stat";
	struct io_vec_stat  *io_vec_stat;
	struct smc_stat_record *sr_p;
	struct sac_interface *sac_p;
	struct scg_rs_chain *chain_p;
	struct scg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);

	pthread_mutex_lock(&priv_p->p_rs_lck);
	list_for_each_entry(chain_p, struct scg_rs_chain, &priv_p->p_rs_head, r_list) {
		list_for_each_entry(chan_p, struct scg_channel, &chain_p->r_head, s_list) {
			sac_p = (struct sac_interface *)chan_p->s_chan;
			io_vec_stat = calloc(1,sizeof(*io_vec_stat));
			sr_p = calloc(1, sizeof(*sr_p));
			sr_p->r_type = NID_REQ_BIO_VEC_STOP;
			sr_p->r_data = (void *)io_vec_stat;
			list_add_tail(&sr_p->r_list, out_head);
			sac_p->sa_op->sa_get_vec_stat(sac_p, io_vec_stat);
		}
	}
	pthread_mutex_unlock(&priv_p->p_rs_lck);

}

static void
scg_set_max_workers(struct scg_interface *scg_p, uint16_t max_workers)
{
	assert(scg_p);
	struct tp_interface *tp_p = NULL;
	tp_p->t_op->t_set_max_workers(tp_p, max_workers);
}

static void
scg_stop(struct scg_interface *scg_p)
{
	char *log_header = "scg_stop";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_interface *sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	struct io_interface *io_p;

	nid_log_notice("%s: start ...", log_header);
	sacg_p->sag_op->sag_stop(sacg_p);

	if ((io_p = priv_p->p_all_io[IO_TYPE_RESOURCE]))
		io_p->io_op->io_stop(io_p);
	if ((io_p = priv_p->p_all_io[IO_TYPE_BUFFER]))
		io_p->io_op->io_stop(io_p);
}

static void
scg_lock_dis_lck(struct scg_interface *scg_p)
{
	char *log_header = "scg_lock_dis_lck";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;

	nid_log_debug("%s: start ...", log_header);
	pthread_mutex_lock(&priv_p->p_dis_lck);
}

static void
scg_unlock_dis_lck(struct scg_interface *scg_p)
{
	char *log_header = "scg_lock_dis_lck";
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;

	nid_log_debug("%s: start ...", log_header);
	pthread_mutex_unlock(&priv_p->p_dis_lck);

}

struct scg_operations scg_op = {
	.sg_accept_new_channel = scg_accept_new_channel,
	.sg_do_channel = scg_do_channel,
	.sg_get_stat = scg_get_stat,
	.sg_reset_stat = scg_reset_stat,
	.sg_update = scg_update,
	.sg_upgrade_alevel = scg_upgrade_alevel,
	.sg_fupgrade_alevel = scg_fupgrade_alevel,
	.sg_check_conn = scg_check_conn,
	.sg_stop = scg_stop,
	.sg_set_max_workers = scg_set_max_workers,
	.sg_bio_fast_flush = scg_bio_fast_flush,
	.sg_bio_stop_fast_flush = scg_bio_stop_fast_flush,
	.sg_bio_vec_start = scg_bio_vec_start,
	.sg_bio_vec_stop = scg_bio_vec_stop,
	.sg_bio_vec_stat = scg_bio_vec_stat,
	.sg_lock_dis_lck = scg_lock_dis_lck,
	.sg_unlock_dis_lck = scg_unlock_dis_lck,
};

static struct ppg_interface *
__scg_create_ppg(struct scg_private *priv_p)
{
	struct ppg_interface *ppg_p;

	ppg_p = calloc(1, sizeof(*ppg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_PPG] = (void *)ppg_p;
	return ppg_p;
}

static struct sdsg_interface *
__scg_create_sdsg(struct scg_private *priv_p)
{
	struct sdsg_interface *sdsg_p;

	sdsg_p = calloc(1, sizeof(*sdsg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SDSG] = (void *)sdsg_p;
	return sdsg_p;
}

static struct sdscg_interface *
__scg_create_sdscg(struct scg_private *priv_p)
{
	struct sdscg_interface *sdscg_p;

	sdscg_p = calloc(1, sizeof(*sdscg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SDSCG] = (void *)sdscg_p;
	return sdscg_p;
}

static struct cdsg_interface *
__scg_create_cdsg(struct scg_private *priv_p)
{
	struct cdsg_interface *cdsg_p;

	cdsg_p = calloc(1, sizeof(*cdsg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CDSG] = (void *)cdsg_p;
	return cdsg_p;
}

static struct cxtg_interface *
__scg_create_cxtg(struct scg_private *priv_p)
{
	struct cxtg_interface *cxtg_p = NULL;

	cxtg_p = calloc(1, sizeof(*cxtg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CXTG] = cxtg_p;
	return cxtg_p;
}

static struct cxag_interface *
__scg_create_cxag(struct scg_private *priv_p)
{
	struct cxag_interface *cxag_p = NULL;

	cxag_p = calloc(1, sizeof(*cxag_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CXAG] = cxag_p;
	return cxag_p;
}

static struct cxacg_interface *
__scg_create_cxacg(struct scg_private *priv_p)
{
	struct cxacg_interface *cxacg_p = NULL;

	cxacg_p = calloc(1, sizeof(*cxacg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CXACG] = cxacg_p;
	return cxacg_p;
}

static struct cxpg_interface *
__scg_create_cxpg(struct scg_private *priv_p)
{
	struct cxpg_interface *cxpg_p = NULL;

	cxpg_p = calloc(1, sizeof(*cxpg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CXPG] = cxpg_p;
	return cxpg_p;
}

static struct cxpcg_interface *
__scg_create_cxpcg(struct scg_private *priv_p)
{
	struct cxpcg_interface *cxpcg_p = NULL;

	cxpcg_p = calloc(1, sizeof(*cxpcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CXPCG] = cxpcg_p;
	return cxpcg_p;
}

static struct dxtg_interface *
__scg_create_dxtg(struct scg_private *priv_p)
{
	struct dxtg_interface *dxtg_p = NULL;

	dxtg_p = calloc(1, sizeof(*dxtg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DXTG] = dxtg_p;
	return dxtg_p;
}

static struct dxag_interface *
__scg_create_dxag(struct scg_private *priv_p)
{
	struct dxag_interface *dxag_p = NULL;

	dxag_p = calloc(1, sizeof(*dxag_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DXAG] = dxag_p;
	return dxag_p;
}

static struct dxacg_interface *
__scg_create_dxacg(struct scg_private *priv_p)
{
	struct dxacg_interface *dxacg_p = NULL;

	dxacg_p = calloc(1, sizeof(*dxacg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DXACG] = dxacg_p;
	return dxacg_p;
}

static struct dxpg_interface *
__scg_create_dxpg(struct scg_private *priv_p)
{
	struct dxpg_interface *dxpg_p = NULL;

	dxpg_p = calloc(1, sizeof(*dxpg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DXPG] = dxpg_p;
	return dxpg_p;
}

static struct dxpcg_interface *
__scg_create_dxpcg(struct scg_private *priv_p)
{
	struct dxpcg_interface *dxpcg_p = NULL;

	dxpcg_p = calloc(1, sizeof(*dxpcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DXPCG] = dxpcg_p;
	return dxpcg_p;
}

static struct wcg_interface *
__scg_create_wcg(struct scg_private *priv_p)
{
	struct wcg_interface *wcg_p = NULL;

	wcg_p = calloc(1, sizeof(*wcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_WCG] = wcg_p;
	return wcg_p;
}

static struct bwccg_interface *
__scg_create_bwccg(struct scg_private *priv_p)
{
	struct bwccg_interface *bwccg_p = NULL;

	bwccg_p = calloc(1, sizeof(*bwccg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_BWCCG] = bwccg_p;
	return bwccg_p;
}

static struct crcg_interface *
__scg_create_crcg(struct scg_private *priv_p)
{
	struct crcg_interface *crcg_p = NULL;

	crcg_p = calloc(1, sizeof(*crcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CRCG] = crcg_p;
	return crcg_p;
}

static struct crccg_interface *
__scg_create_crccg(struct scg_private *priv_p)
{
	struct crccg_interface *crccg_p = NULL;

	crccg_p = calloc(1, sizeof(*crccg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_CRCCG] = crccg_p;
	return crccg_p;
}


static struct twccg_interface *
__scg_create_twccg(struct scg_private *priv_p)
{
	struct twccg_interface *twccg_p = NULL;

	twccg_p = calloc(1, sizeof(*twccg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_TWCCG] = twccg_p;
	return twccg_p;
}

static struct wccg_interface *
__scg_create_wccg(struct scg_private *priv_p)
{
	struct wccg_interface *wccg_p = NULL;

	wccg_p = calloc(1, sizeof(*wccg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_WCCG] = wccg_p;
	return wccg_p;
}

static struct rwg_interface *
__scg_create_rwg(struct scg_private *priv_p)
{
	struct rwg_interface *rwg_p = NULL;

	rwg_p = calloc(1, sizeof(*rwg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_RWG] = rwg_p;
	return rwg_p;
}

static struct drwcg_interface *
__scg_create_drwcg(struct scg_private *priv_p)
{
	struct drwcg_interface *drwcg_p = NULL;

	drwcg_p = calloc(1, sizeof(*drwcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DRWCG] = drwcg_p;
	return drwcg_p;
}

static struct mrwcg_interface *
__scg_create_mrwcg(struct scg_private *priv_p)
{
	struct mrwcg_interface *mrwcg_p = NULL;

	mrwcg_p = calloc(1, sizeof(*mrwcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_MRWCG] = mrwcg_p;
	return mrwcg_p;
}

static struct smcg_interface *
__scg_create_smcg(struct scg_private *priv_p)
{
	struct smcg_interface *smcg_p = NULL;

	smcg_p = calloc(1, sizeof(*smcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SMCG] = smcg_p;
	return smcg_p;
}

static struct sacg_interface *
__scg_create_sacg(struct scg_private *priv_p)
{
	struct sacg_interface *sacg_p = NULL;

	sacg_p = calloc(1, sizeof(*sacg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SACG] = sacg_p;
	return sacg_p;
}

static struct tpcg_interface *
__scg_create_tpcg(struct scg_private *priv_p)
{
	struct tpcg_interface *tpcg_p = NULL;

	tpcg_p = calloc(1, sizeof(*tpcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_TPCG] = tpcg_p;
	return tpcg_p;
}

static struct saccg_interface *
__scg_create_saccg(struct scg_private *priv_p)
{
	struct saccg_interface *saccg_p = NULL;

	saccg_p = calloc(1, sizeof(*saccg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SACCG] = saccg_p;
	return saccg_p;
}

static struct doacg_interface *
__scg_create_doacg(struct scg_private *priv_p)
{
	struct doacg_interface *doacg_p = NULL;

	doacg_p = calloc(1, sizeof(*doacg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_DOACG] = doacg_p;
	return doacg_p;
}

static struct ppcg_interface *
__scg_create_ppcg(struct scg_private *priv_p)
{
	struct ppcg_interface *ppcg_p = NULL;

	ppcg_p = calloc(1, sizeof(*ppcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_PPCG] = ppcg_p;
	return ppcg_p;
}

static struct snapg_interface *
__scg_create_snapg(struct scg_private *priv_p)
{
	struct snapg_interface *snapg_p = NULL;

	snapg_p = calloc(1, sizeof(*snapg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_SNAPG] = snapg_p;
	return snapg_p;
}

static struct inicg_interface *
__scg_create_inicg(struct scg_private *priv_p)
{
	struct inicg_interface *inicg_p = NULL;

	inicg_p = calloc(1, sizeof(*inicg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_INICG] = inicg_p;
	return inicg_p;
}

static struct reptg_interface *
__scg_create_reptg(struct scg_private *priv_p)
{
	struct reptg_interface *rtg_p = NULL;

	rtg_p = calloc(1, sizeof(*rtg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_REPTG] = rtg_p;
	return rtg_p;
}

static struct repsg_interface *
__scg_create_repsg(struct scg_private *priv_p)
{
	struct repsg_interface *rsg_p = NULL;

	rsg_p = calloc(1, sizeof(*rsg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_REPSG] = rsg_p;
	return rsg_p;
}

static struct reptcg_interface *
__scg_create_reptcg(struct scg_private *priv_p)
{
	struct reptcg_interface *rtcg_p = NULL;

	rtcg_p = calloc(1, sizeof(*rtcg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_REPTCG] = rtcg_p;
	return rtcg_p;
}

static struct repscg_interface *
__scg_create_repscg(struct scg_private *priv_p)
{
	struct repscg_interface *rscg_p = NULL;

	rscg_p = calloc(1, sizeof(*rscg_p));
	priv_p->p_guardians[__SCG_GUARDIAN_REPSCG] = rscg_p;
	return rscg_p;
}

static void
__scg_create_guardians(struct scg_private *priv_p, struct scg_setup *setup)
{
	void *ret_obj;

	ret_obj = (void *)__scg_create_ppg(priv_p);
	assert(ret_obj);

	ret_obj = (void *)__scg_create_snapg(priv_p);
	assert(ret_obj);


	if (setup->support_sds) {
		ret_obj = (void *)__scg_create_sdsg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_sdsc) {
		ret_obj = (void *)__scg_create_sdscg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cds) {
		ret_obj = (void *)__scg_create_cdsg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cxt) {
		ret_obj = (void *)__scg_create_cxtg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cxa) {
		ret_obj = (void *)__scg_create_cxag(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cxac) {
		ret_obj = (void *)__scg_create_cxacg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cxp) {
		ret_obj = (void *)__scg_create_cxpg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_cxpc) {
		ret_obj = (void *)__scg_create_cxpcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_dxt) {
		ret_obj = (void *)__scg_create_dxtg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_dxa) {
		ret_obj = (void *)__scg_create_dxag(priv_p);
		assert(ret_obj);
	}

	if (setup->support_dxac) {
		ret_obj = (void *)__scg_create_dxacg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_dxp) {
		ret_obj = (void *)__scg_create_dxpg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_dxpc) {
		ret_obj = (void *)__scg_create_dxpcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_bwc || setup->support_twc) {
		ret_obj = (void *)__scg_create_wcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_bwcc) {
		ret_obj = (void *)__scg_create_bwccg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_crc) {
		ret_obj = (void *)__scg_create_crcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_crcc) {
		ret_obj = (void *)__scg_create_crccg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_twcc) {
		ret_obj = (void *)__scg_create_twccg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_wcc) {
		ret_obj = (void *)__scg_create_wccg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_drw || setup->support_mrw) {
		ret_obj = (void *)__scg_create_rwg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_drwc) {
		ret_obj = (void *)__scg_create_drwcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_mrwc) {
		ret_obj = (void *)__scg_create_mrwcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_smc) {
		ret_obj = (void *)__scg_create_smcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_sac) {
		ret_obj = (void *)__scg_create_sacg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_tpc) {
		ret_obj = (void *)__scg_create_tpcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_sacc) {
		ret_obj = (void *)__scg_create_saccg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_doac) {
		ret_obj = (void *)__scg_create_doacg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_ppc) {
		ret_obj = (void *)__scg_create_ppcg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_inic) {
		ret_obj = (void *)__scg_create_inicg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_rept) {
		ret_obj = (void *)__scg_create_reptg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_reps) {
		ret_obj = (void *)__scg_create_repsg(priv_p);
		assert(ret_obj);
	}

	if (setup->support_reptc) {
		ret_obj = (void *)__scg_create_reptcg(priv_p);
		assert(ret_obj);
	}
	if (setup->support_repsc) {
		ret_obj = (void *)__scg_create_repscg(priv_p);
		assert(ret_obj);
	}

}

static int
__scg_init_ppg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct ppg_setup ppg_setup;
	struct ppg_interface *ppg_p;

	assert(setup);
	ppg_p = priv_p->p_guardians[__SCG_GUARDIAN_PPG];
	ppg_setup.allocator = priv_p->p_allocator;
	ppg_setup.ini = priv_p->p_ini;
	return ppg_initialization(ppg_p, &ppg_setup);
}

static int
__scg_init_sdsg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sdsg_setup sdsg_setup;
	struct sdsg_interface *sdsg_p;

	assert(setup);
	sdsg_p = priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	sdsg_setup.allocator = priv_p->p_allocator;
	sdsg_setup.ini = priv_p->p_ini;
	sdsg_setup.ppg = (struct ppg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_PPG];
	return sdsg_initialization(sdsg_p, &sdsg_setup);
}

static int
__scg_init_sdscg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sdscg_setup sdscg_setup;
	struct sdscg_interface *sdscg_p;

	assert(setup);
	sdscg_p = priv_p->p_guardians[__SCG_GUARDIAN_SDSCG];
	sdscg_setup.umpk = priv_p->p_umpk;
	sdscg_setup.sdsg = (struct sdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	return sdscg_initialization(sdscg_p, &sdscg_setup);
}

static int
__scg_init_cdsg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cdsg_setup cdsg_setup;
	struct cdsg_interface *cdsg_p;

	assert(setup);
	cdsg_p = priv_p->p_guardians[__SCG_GUARDIAN_CDSG];
	cdsg_setup.allocator = priv_p->p_allocator;
	cdsg_setup.ini = priv_p->p_ini;
	return cdsg_initialization(cdsg_p, &cdsg_setup);
}

static int
__scg_init_cxtg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cxtg_setup cxtg_setup;
	struct cxtg_interface *cxtg_p;

	assert(setup);
	cxtg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXTG];
	cxtg_setup.ini = priv_p->p_ini;
	cxtg_setup.lstn = priv_p->p_lstn;
	cxtg_setup.txn = priv_p->p_txn;
	cxtg_setup.pp2 = priv_p->p_pp2;
	cxtg_setup.cdsg_p = (struct cdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CDSG];
	cxtg_setup.sdsg_p = (struct sdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	return cxtg_initialization(cxtg_p, &cxtg_setup);
}

static int
__scg_init_cxag(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cxag_setup cxag_setup;
	struct cxag_interface *cxag_p;

	assert(setup);
	cxag_p = priv_p->p_guardians[__SCG_GUARDIAN_CXAG];
	cxag_setup.lstn = priv_p->p_lstn;
	cxag_setup.ini = priv_p->p_ini;
	cxag_setup.umpk = priv_p->p_umpk;
	cxag_setup.pp2 = priv_p->p_pp2;
	cxag_setup.cxtg = (struct cxtg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CXTG];
	return cxag_initialization(cxag_p, &cxag_setup);
}

static int
__scg_init_cxacg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cxacg_setup cxacg_setup;
	struct cxacg_interface *cxacg_p;

	assert(setup);
	cxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXACG];
	cxacg_setup.umpk = priv_p->p_umpk;
	cxacg_setup.cxag = (struct cxag_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CXAG];
	return cxacg_initialization(cxacg_p, &cxacg_setup);
}

static int
__scg_init_cxpg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cxpg_setup cxpg_setup;
	struct cxpg_interface *cxpg_p;

	assert(setup);
	cxpg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXPG];
	cxpg_setup.lstn = priv_p->p_lstn;
	cxpg_setup.ini = priv_p->p_ini;
	cxpg_setup.cxtg = priv_p->p_guardians[__SCG_GUARDIAN_CXTG];
	return cxpg_initialization(cxpg_p, &cxpg_setup);
}

static int
__scg_init_cxpcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct cxpcg_setup cxpcg_setup;
	struct cxpcg_interface *cxpcg_p;

	assert(setup);
	cxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_CXPCG];
	cxpcg_setup.umpk = priv_p->p_umpk;
	cxpcg_setup.cxpg = (struct cxpg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CXPG];
	return cxpcg_initialization(cxpcg_p, &cxpcg_setup);
}

static int
__scg_init_dxtg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct dxtg_setup dxtg_setup;
	struct dxtg_interface *dxtg_p;

	assert(setup);
	dxtg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXTG];
	dxtg_setup.txn = priv_p->p_txn;
	dxtg_setup.ini = priv_p->p_ini;
	dxtg_setup.lstn = priv_p->p_lstn;
	dxtg_setup.pp2 = priv_p->p_pp2;
	dxtg_setup.cdsg_p = (struct cdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CDSG];
	dxtg_setup.sdsg_p = (struct sdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	return dxtg_initialization(dxtg_p, &dxtg_setup);
}

static int
__scg_init_dxag(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct dxag_setup dxag_setup;
	struct dxag_interface *dxag_p;

	assert(setup);
	dxag_p = priv_p->p_guardians[__SCG_GUARDIAN_DXAG];
	dxag_setup.lstn = priv_p->p_lstn;
	dxag_setup.ini = priv_p->p_ini;
	dxag_setup.umpk = priv_p->p_umpk;
	dxag_setup.pp2 = priv_p->p_pp2;
	dxag_setup.dxtg = (struct dxtg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_DXTG];
	return dxag_initialization(dxag_p, &dxag_setup);
}

static int
__scg_init_dxacg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct dxacg_setup dxacg_setup;
	struct dxacg_interface *dxacg_p;

	assert(setup);
	dxacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXACG];
	dxacg_setup.umpk = priv_p->p_umpk;
	dxacg_setup.dxag = (struct dxag_interface *)priv_p->p_guardians[__SCG_GUARDIAN_DXAG];
	return dxacg_initialization(dxacg_p, &dxacg_setup);
}

static int
__scg_init_dxpg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct dxpg_setup dxpg_setup;
	struct dxpg_interface *dxpg_p;

	assert(setup);
	dxpg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXPG];
	dxpg_setup.lstn = priv_p->p_lstn;
	dxpg_setup.ini = priv_p->p_ini;
	dxpg_setup.umpk = priv_p->p_umpk;
	dxpg_setup.dxtg = (struct dxtg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_DXTG];
	return dxpg_initialization(dxpg_p, &dxpg_setup);
}

static int
__scg_init_dxpcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct dxpcg_setup dxpcg_setup;
	struct dxpcg_interface *dxpcg_p;

	assert(setup);
	dxpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_DXPCG];
	dxpcg_setup.umpk = priv_p->p_umpk;
	dxpcg_setup.dxpg = (struct dxpg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_DXPG];
	return dxpcg_initialization(dxpcg_p, &dxpcg_setup);
}

static int
__scg_init_wcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct wcg_setup wcg_setup;
	struct wcg_interface *wcg_p;


	assert(setup);
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	wcg_setup.support_bwc = setup->support_bwc;
	wcg_setup.support_twc = setup->support_twc;
	wcg_setup.ini = priv_p->p_ini;
	wcg_setup.sdsg = (struct sdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	wcg_setup.srn = &priv_p->p_srn;
	wcg_setup.allocator = priv_p->p_allocator;
	wcg_setup.lstn = priv_p->p_lstn;
	wcg_setup.io = priv_p->p_all_io[IO_TYPE_BUFFER];
	wcg_setup.tpg = (struct tpg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_TPG];
	wcg_setup.mqtt = priv_p->p_mqtt;
	return wcg_initialization(wcg_p, &wcg_setup);
}

static int
__scg_init_bwccg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct bwccg_setup bwccg_setup;
	struct bwccg_interface *bwccg_p;
	struct wcg_interface *wcg_p;

	assert(setup);
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	bwccg_p = priv_p->p_guardians[__SCG_GUARDIAN_BWCCG];
	bwccg_setup.bwcg = wcg_p->wg_op->wg_get_bwcg(wcg_p);
	bwccg_setup.umpk = priv_p->p_umpk;

	return bwccg_initialization(bwccg_p, &bwccg_setup);
}

static int
__scg_init_twccg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct twccg_setup twccg_setup;
	struct twccg_interface *twccg_p;
	struct wcg_interface *wcg_p;

	assert(setup);
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	twccg_p = priv_p->p_guardians[__SCG_GUARDIAN_TWCCG];
	twccg_setup.twcg = wcg_p->wg_op->wg_get_twcg(wcg_p);
	twccg_setup.umpk = priv_p->p_umpk;

	return twccg_initialization(twccg_p, &twccg_setup);
}

static int
__scg_init_crcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct crcg_setup crcg_setup;
	struct crcg_interface *crcg_p;

	assert(setup);
	crcg_p = priv_p->p_guardians[__SCG_GUARDIAN_CRCG];
	crcg_setup.ini = &priv_p->p_ini;
	crcg_setup.ppg = (struct ppg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_PPG];
	crcg_setup.fpn = priv_p->p_fpn;
	crcg_setup.srn = &priv_p->p_srn;
	crcg_setup.allocator = priv_p->p_allocator;
	crcg_setup.brn = &priv_p->p_brn;
	crcg_setup.tpg = priv_p->p_tpg;
	return crcg_initialization(crcg_p, &crcg_setup);
}

static int
__scg_init_crccg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct crccg_setup crccg_setup;
	struct crccg_interface *crccg_p;

	assert(setup);
	crccg_p = priv_p->p_guardians[__SCG_GUARDIAN_CRCCG];
	crccg_setup.crcg = (struct crcg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CRCG];
	crccg_setup.umpk = priv_p->p_umpk;

	return crccg_initialization(crccg_p, &crccg_setup);
}


static int
__scg_init_wccg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct wccg_setup wccg_setup;
	struct wccg_interface *wccg_p;
	struct wcg_interface *wcg_p;

	assert(setup);
	wccg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCCG];
	wcg_p = priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	wccg_setup.bwcg = wcg_p->wg_op->wg_get_bwcg(wcg_p);
	wccg_setup.twcg = wcg_p->wg_op->wg_get_twcg(wcg_p);
	wccg_setup.umpk = priv_p->p_umpk;

	return wccg_initialization(wccg_p, &wccg_setup);
}

static int
__scg_init_rwg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct rwg_setup rwg_setup;
	struct rwg_interface *rwg_p;

	assert(setup);
	rwg_p = priv_p->p_guardians[__SCG_GUARDIAN_RWG];
	rwg_setup.support_mrw = setup->support_mrw;
	rwg_setup.support_drw = setup->support_drw;
	rwg_setup.allocator = priv_p->p_allocator;
	rwg_setup.fpn = priv_p->p_fpn;
	rwg_setup.ini = &priv_p->p_ini;

	return rwg_initialization(rwg_p, &rwg_setup);
}

static int
__scg_init_drwcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct drwcg_setup drwcg_setup;
	struct drwcg_interface *drwcg_p;
	struct rwg_interface *rwg_p;

	assert(setup);
	drwcg_p = priv_p->p_guardians[__SCG_GUARDIAN_DRWCG];
	rwg_p = (struct rwg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_RWG];
	drwcg_setup.drwg = rwg_p->rwg_op->rwg_get_drwg(rwg_p);
	drwcg_setup.umpk = priv_p->p_umpk;

	return drwcg_initialization(drwcg_p, &drwcg_setup);
}

static int
__scg_init_mrwcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct mrwcg_setup mrwcg_setup;
	struct mrwcg_interface *mrwcg_p;
	struct rwg_interface *rwg_p;

	assert(setup);
	mrwcg_p = priv_p->p_guardians[__SCG_GUARDIAN_MRWCG];
	rwg_p = (struct rwg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_RWG];
	mrwcg_setup.mrwg = rwg_p->rwg_op->rwg_get_mrwg(rwg_p);
	mrwcg_setup.umpk = priv_p->p_umpk;

	return mrwcg_initialization(mrwcg_p, &mrwcg_setup);
}

static int
__scg_init_smcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct smcg_setup smcg_setup;
	struct smcg_interface *smcg_p;

	assert(setup);
	smcg_p = priv_p->p_guardians[__SCG_GUARDIAN_SMCG];
	smcg_setup.scg = scg_p;
	smcg_setup.mpk = priv_p->p_mpk;
	smcg_setup.alloc = priv_p->p_allocator;
	smcg_setup.ini = priv_p->p_ini;
	smcg_setup.mqtt = priv_p->p_mqtt;

	return smcg_initialization(smcg_p, &smcg_setup);
}

static int
__scg_init_tpcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct tpcg_setup tpcg_setup;
	struct tpcg_interface *tpcg_p;

	assert(setup);
	tpcg_p = priv_p->p_guardians[__SCG_GUARDIAN_TPCG];
	tpcg_setup.tpg = (struct tpg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_TPG];
	tpcg_setup.umpk = priv_p->p_umpk;

	return tpcg_initialization(tpcg_p, &tpcg_setup);
}

static int
__scg_init_sacg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct sacg_setup sacg_setup;
	struct sacg_interface *sacg_p;

	assert(setup);
	sacg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	sacg_setup.scg = scg_p;
	sacg_setup.sdsg = (struct sdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_SDSG];
	sacg_setup.cdsg = (struct cdsg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CDSG];
	sacg_setup.wcg = (struct wcg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	sacg_setup.crcg = (struct crcg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_CRCG];
	sacg_setup.rwg = (struct rwg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_RWG];
	sacg_setup.tpg = priv_p->p_tpg;
	sacg_setup.ini = &priv_p->p_ini;
	sacg_setup.all_io = priv_p->p_all_io;
	sacg_setup.server_keys = priv_p->p_server_keys;
	sacg_setup.mqtt = priv_p->p_mqtt;

	return sacg_initialization(sacg_p, &sacg_setup);
}

static int
__scg_init_saccg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct saccg_setup saccg_setup;
	struct saccg_interface *saccg_p;

	assert(setup);
	saccg_p = priv_p->p_guardians[__SCG_GUARDIAN_SACCG];
	saccg_setup.sacg = priv_p->p_guardians[__SCG_GUARDIAN_SACG];
	saccg_setup.umpk = priv_p->p_umpk;

	return saccg_initialization(saccg_p, &saccg_setup);
}

static int
__scg_init_ppcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct ppcg_setup ppcg_setup;
	struct ppcg_interface *ppcg_p;

	assert(setup);
	ppcg_p = priv_p->p_guardians[__SCG_GUARDIAN_PPCG];
	ppcg_setup.ppg = priv_p->p_guardians[__SCG_GUARDIAN_PPG];
	ppcg_setup.umpk = priv_p->p_umpk;

	return ppcg_initialization(ppcg_p, &ppcg_setup);
}


static int
__scg_init_snapg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct snapg_setup snapg_setup;
	struct snapg_interface *snapg_p;

	assert(setup);
	snapg_p = priv_p->p_guardians[__SCG_GUARDIAN_SNAPG];

	return snapg_initialization(snapg_p, &snapg_setup);
}

static int
__scg_init_doacg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct doacg_setup doacg_setup;
	struct doacg_interface *doacg_p;

	assert(setup);
	doacg_p = priv_p->p_guardians[__SCG_GUARDIAN_DOACG];
	doacg_setup.scg = scg_p;
	doacg_setup.umpk = priv_p->p_umpk;

	return doacg_initialization(doacg_p, &doacg_setup);
}

static int
__scg_init_inicg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct inicg_setup inicg_setup;
	struct inicg_interface *inicg_p;

	assert(setup);
	inicg_p = priv_p->p_guardians[__SCG_GUARDIAN_INICG];
	inicg_setup.ini = priv_p->p_ini;
	inicg_setup.umpk = priv_p->p_umpk;

	return inicg_initialization(inicg_p, &inicg_setup);
}

static int
__scg_init_reptg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct reptg_setup rtg_setup;
	struct reptg_interface *rtg_p;

	assert(setup);
	rtg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPTG];
	rtg_setup.ini = priv_p->p_ini;
	rtg_setup.tpg = priv_p->p_tpg;
	rtg_setup.allocator = priv_p->p_allocator;
	rtg_setup.dxpg = priv_p->p_guardians[__SCG_GUARDIAN_DXPG];

	return reptg_initialization(rtg_p, &rtg_setup);
}

static int
__scg_init_repsg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct repsg_setup rsg_setup;
	struct repsg_interface *rsg_p;

	assert(setup);
	rsg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPSG];
	rsg_setup.ini = priv_p->p_ini;
	rsg_setup.tpg = priv_p->p_tpg;
	rsg_setup.allocator = priv_p->p_allocator;
	rsg_setup.umpk = priv_p->p_umpk;
	rsg_setup.dxag = priv_p->p_guardians[__SCG_GUARDIAN_DXAG];

	return repsg_initialization(rsg_p, &rsg_setup);
}

static int
__scg_init_reptcg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct reptcg_setup rtcg_setup;
	struct reptcg_interface *rtcg_p;

	assert(setup);
	rtcg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPTCG];
	rtcg_setup.umpk = priv_p->p_umpk;
	rtcg_setup.reptg = priv_p->p_guardians[__SCG_GUARDIAN_REPTG];

	return reptcg_initialization(rtcg_p, &rtcg_setup);
}

static int
__scg_init_repscg(struct scg_interface *scg_p, struct scg_setup *setup)
{
	struct scg_private *priv_p = (struct scg_private *)scg_p->sg_private;
	struct repscg_setup rscg_setup;
	struct repscg_interface *rscg_p;

	assert(setup);
	rscg_p = priv_p->p_guardians[__SCG_GUARDIAN_REPSCG];
	rscg_setup.umpk = priv_p->p_umpk;
	rscg_setup.repsg = priv_p->p_guardians[__SCG_GUARDIAN_REPSG];

	return repscg_initialization(rscg_p, &rscg_setup);
}

static void
__scg_init_guardians(struct scg_interface *scg_p, struct scg_setup *setup)
{
	int rc;

	rc = __scg_init_ppg(scg_p, setup);
	assert(!rc);

	rc = __scg_init_snapg(scg_p, setup);
	assert(!rc);


	if (setup->support_sds) {
		rc = __scg_init_sdsg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_sdsc) {
		rc = __scg_init_sdscg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cds) {
		rc = __scg_init_cdsg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cxt) {
		rc = __scg_init_cxtg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cxa) {
		rc = __scg_init_cxag(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cxac) {
		rc = __scg_init_cxacg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cxp) {
		rc = __scg_init_cxpg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_cxpc) {
		rc = __scg_init_cxpcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_dxt) {
		rc = __scg_init_dxtg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_dxa) {
		rc = __scg_init_dxag(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_dxac) {
		rc = __scg_init_dxacg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_dxp) {
		rc = __scg_init_dxpg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_dxpc) {
		rc = __scg_init_dxpcg(scg_p, setup);
		assert(!rc);
	}

// All module guardains like wcg should init before control guardains like bwccg and sac module
	if (setup->support_bwc || setup->support_twc) {
		rc = __scg_init_wcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_crc) {
		rc = __scg_init_crcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_bwcc) {
		rc = __scg_init_bwccg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_crcc) {
		rc = __scg_init_crccg(scg_p, setup);
		assert(!rc);
	}


	if (setup->support_twcc) {
		rc = __scg_init_twccg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_wcc) {
		rc = __scg_init_wccg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_drw || setup->support_mrw) {
		rc = __scg_init_rwg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_drwc) {
		rc = __scg_init_drwcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_mrwc) {
		rc = __scg_init_mrwcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_smc) {
		rc = __scg_init_smcg(scg_p, setup);
		assert(!rc);
	}



	if (setup->support_tpc) {
		rc = __scg_init_tpcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_sacc) {
		rc = __scg_init_saccg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_doac) {
		rc = __scg_init_doacg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_ppc) {
		rc = __scg_init_ppcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_sac) {
		rc = __scg_init_sacg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_inic) {
		rc = __scg_init_inicg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_rept) {
		rc = __scg_init_reptg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_reps) {
		rc = __scg_init_repsg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_reptc) {
		rc = __scg_init_reptcg(scg_p, setup);
		assert(!rc);
	}

	if (setup->support_repsc) {
		rc = __scg_init_repscg(scg_p, setup);
		assert(!rc);
	}
}

int
scg_initialization(struct scg_interface *scg_p, struct scg_setup *setup)
{
	char *log_header = "scg_initialization";
	struct scg_private *priv_p;
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	struct lstn_interface *lstn_p = setup->lstn;
	struct txn_interface *txn_p = setup->txn;
	struct brn_interface *brn_p;
	struct brn_setup brn_setup;
	struct srn_interface *srn_p;
	struct srn_setup srn_setup;
	struct io_interface *io_p;
	struct io_setup io_setup;

	struct fpn_setup fpn_setup;
	struct fpn_interface *fpn_p = NULL;

	struct wcg_interface *wcg_p;

	char *sect_type;

	nid_log_warning("%s: start ...", log_header);
	if (!setup) {
		nid_log_error("%s: got null setup", log_header);
		return -1;
	}

	scg_p->sg_op = &scg_op;
	priv_p = calloc(1, sizeof(*priv_p));
	scg_p->sg_private = priv_p;


	INIT_LIST_HEAD(&priv_p->p_rs_head);

	pthread_mutex_init(&priv_p->p_rs_lck, NULL);
	pthread_cond_init(&priv_p->p_rs_cond, NULL);
	pthread_mutex_init(&priv_p->p_pickup_lck, NULL);
	pthread_cond_init(&priv_p->p_pickup_cond, NULL);
	pthread_mutex_init(&priv_p->p_dis_lck, NULL);

	priv_p->p_allocator = setup->allocator;
	priv_p->p_mpk = setup->mpk;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_tpg = setup->tpg;
	priv_p->p_ini = setup->ini;
	priv_p->p_server_keys = setup->server_keys;
	priv_p->p_lstn = lstn_p;
	priv_p->p_txn  = txn_p;
	priv_p->p_pp2 = setup->pp2;
	priv_p->p_mqtt = setup->mqtt;

	// create thread pool interface
	ini_p = priv_p->p_ini;
	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		if(!the_key)
			continue;

		sect_type = (char *)(the_key->k_value);
		if (strcmp(sect_type, "scg"))
			continue;

		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "wtp_name");
		if(!the_key) {
			nid_log_error("%s: no wtp_name in scg section", log_header);
			priv_p->p_tp = NULL;
		} else {
			char *tp_nm = (char *)(the_key->k_value);
			struct tpg_interface *tpg_p = priv_p->p_tpg;
			priv_p->p_tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_nm);
			if(!priv_p->p_tp) {
				nid_log_error("%s: failed to create thread pool %s instance", log_header, tp_nm);
			}
		}

		if(!priv_p->p_tp) {
			assert(0);
		}
		break;
	}

	/*
	 * create fpn
	 */
	fpn_p = calloc(1, sizeof(*fpn_p));
	fpn_setup.fp_size = NID_SIZE_FP;
	fpn_setup.allocator = priv_p->p_allocator;
	fpn_setup.set_id = ALLOCATOR_SET_SCG_FPN;
	fpn_setup.seg_size = 4096;
	fpn_initialization(fpn_p, &fpn_setup);
	priv_p->p_fpn = fpn_p;

	/*
	 * create brn
	 */
	brn_p = &priv_p->p_brn;
	brn_setup.allocator = priv_p->p_allocator;
	brn_setup.set_id = ALLOCATOR_SET_SCG_BRN;
	brn_setup.seg_size = 4096;
	brn_initialization(brn_p, &brn_setup);

	/*
	 * create srn
	 */
	srn_p = &priv_p->p_srn;
	srn_setup.allocator = priv_p->p_allocator;
	srn_setup.set_id = ALLOCATOR_SET_SCG_SRN;
	srn_setup.seg_size = 1024;
	srn_initialization(srn_p, &srn_setup);

	/*
	 * create rio
	 */
	if (setup->support_rio) {
		io_p = calloc(1, sizeof(*io_p));
		priv_p->p_all_io[IO_TYPE_RESOURCE] = io_p;
		io_setup.io_type = IO_TYPE_RESOURCE;
		io_setup.ini = &priv_p->p_ini;
		io_initialization(io_p, &io_setup);
	}

	/*
	 * create bio
	 */
	if (setup->support_bio) {
		assert(setup->support_sds);
		io_p = calloc(1, sizeof(*io_p));
		priv_p->p_all_io[IO_TYPE_BUFFER] = io_p;
		io_setup.io_type = IO_TYPE_BUFFER;
		io_setup.ini = &priv_p->p_ini;
		io_initialization(io_p, &io_setup);
	}

	priv_p->p_guardians[__SCG_GUARDIAN_TPG] = priv_p->p_tpg;
	__scg_create_guardians(priv_p, setup);
	__scg_init_guardians(scg_p, setup);

	/*
	 * all non-global sections other than sac since sac channels rely on
	 * other type of sections parsing results
	 */
	ini_p = priv_p->p_ini;
	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "type");
		sect_type = (char *)(the_key->k_value);

		if (!strcmp(sect_type, "scg")) {
		}
	}


	/*
	 * now the wc channels required by wc recovery are there added by __scg_add_all_sac().
	 * time to start the bwc recovery threads.
	 */
	wcg_p = (struct wcg_interface *)priv_p->p_guardians[__SCG_GUARDIAN_WCG];
	if (wcg_p)
		wcg_p->wg_op->wg_recover_all_wc(wcg_p);

	return 0;
}


