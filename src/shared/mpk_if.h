/*
 * mpk_if.h
 * 	Interface of Message Packaging Module
 */
#ifndef _NID_MPK_IF_H
#define _NID_MPK_IF_H

#ifdef __KERNEL__
#define x_be16toh	be16_to_cpu
#define x_be32toh	be32_to_cpu
#define x_be64toh	be64_to_cpu
#define x_htobe16	cpu_to_be16
#define x_htobe32	cpu_to_be32
#define x_htobe64	cpu_to_be64

#else
#define x_be16toh	be16toh
#define x_be32toh	be32toh
#define x_be64toh	be64toh
#define x_htobe16	htobe16
#define x_htobe32	htobe32
#define x_htobe64	htobe64
#endif

#include "nid_shared.h"

struct nid_message_hdr {
	uint8_t		m_req;
	uint8_t		m_req_code;
};

struct nid_message_bio_release {
	uint8_t		m_req;
	uint8_t		m_req_code;
	uint16_t	m_len;
	char		m_chan_uuid[NID_MAX_UUID];
};

struct nid_message_bio_vec {
	uint8_t		m_req;
	uint8_t		m_req_code;
	uint32_t	m_vec_flushnum;
	uint64_t	m_vec_vecnum;
	uint64_t	m_vec_writesz;
};

struct nid_message_doa {
	uint8_t		m_req;
	uint8_t		m_req_code;
	char*		m_doa_vid;
	uint16_t	m_doa_vid_len;
	char*		m_doa_lid;
	uint16_t	m_doa_lid_len;
	uint32_t	m_doa_hold_time;
	uint32_t 	m_doa_lock;
	uint32_t	m_doa_time_out;

};

struct nid_message {
	uint8_t		m_req;
	uint8_t		m_req_code;
	int		m_devminor;
#define m_chan_id	m_devminor
	char		*m_devname;
	char		*m_uuid;
#define m_bundle_id	m_uuid
#define m_channel_uuid	m_uuid
	char		*m_ip;
	uint16_t	m_devnamelen;
	uint16_t	m_uuidlen;
	uint8_t		m_iplen;
	uint8_t		m_state;
	uint8_t		m_alevel;
	uint64_t	m_size;
	uint32_t	m_blksize;
	int		m_rsfd;
#define m_rex_id	m_rsfd
	int		m_dsfd;
#define m_pos		m_dsfd
	uint16_t	m_nfree;
	uint16_t	m_nnofree;
	uint16_t	m_nused;
	uint16_t	m_nworker;
	uint16_t	m_nmaxworker;
	uint64_t	m_kcounter;
	uint64_t	m_r_kcounter;
	uint64_t	m_rcounter;
	uint64_t	m_rreadycounter;
	uint64_t	m_r_rcounter;
	uint64_t	m_wcounter;
	uint64_t	m_wreadycounter;
	uint64_t	m_r_wcounter;
	uint64_t	m_recv_sequence;
	uint64_t	m_wait_sequence;
	uint32_t	m_srv_status;
	uint32_t	m_live_time;
	char 		m_code;
	char		m_upgrade_force;
	uint32_t	m_rtree_segsz;
	uint32_t	m_rtree_nseg;
	uint32_t	m_rtree_nfree;
	uint32_t	m_rtree_nused;
	uint32_t	m_btn_segsz;
	uint32_t	m_btn_nseg;
	uint32_t	m_btn_nfree;
	uint32_t	m_btn_nused;
	uint8_t		m_io_type_bio;
#define	m_chan_type	m_io_type_bio
	uint16_t	m_block_occupied;
	uint16_t	m_flush_nblocks;
	uint16_t	m_cur_write_block;
	uint16_t	m_cur_flush_block;
	uint64_t	m_seq_flushed;
	uint64_t	m_rec_seq_flushed;
	uint64_t	m_seq_assigned;
	uint32_t	m_buf_avail;
	uint8_t		m_io_type_rio;
	uint32_t	m_cmd;
	char*		m_version;
	uint8_t		m_versionlen;
	uint32_t	m_sfd;

	char		m_has_devminor;
#define m_has_chan_id	m_has_devminor
	char		m_has_devname;
	char		m_has_uuid;
#define m_has_bundle_id	m_has_uuid
#define m_has_channel_uuid m_has_uuid
	char		m_has_ip;
	char		m_has_size;
	char		m_has_blksize;
	char		m_has_rsfd;
#define m_has_rex_id	m_has_rsfd
	char		m_has_dsfd;
#define m_has_pos	m_has_dsfd
	char		m_has_state;
	char		m_has_alevel;
	char		m_has_nfree;
	char		m_has_nnofree;
	char		m_has_nused;
	char		m_has_nworker;
	char		m_has_nmaxworker;
	char		m_has_kcounter;
	char		m_has_r_kcounter;
	char		m_has_rcounter;
	char		m_has_rreadycounter;
	char		m_has_r_rcounter;
	char		m_has_wcounter;
	char		m_has_wreadycounter;
	char		m_has_r_wcounter;
	char		m_has_recv_sequence;
	char		m_has_wait_sequence;
	char		m_has_srv_status;
	char		m_has_live_time;
	char		m_has_code;
	char		m_has_upgrade_force;
	char		m_has_rtree_segsz;
	char		m_has_rtree_nseg;
	char		m_has_rtree_nfree;
	char		m_has_rtree_nused;
	char		m_has_btn_segsz;
	char		m_has_btn_nseg;
	char		m_has_btn_nfree;
	char		m_has_btn_nused;
	char		m_has_io_type_bio;
#define m_has_chan_type	m_has_io_type_bio
	char		m_has_block_occupied;
	char		m_has_flush_nblocks;
	char		m_has_cur_write_block;
	char		m_has_cur_flush_block;
	char		m_has_seq_flushed;
	char		m_has_rec_seq_flushed;
	char		m_has_seq_assigned;
	char		m_has_buf_avail;
	char		m_has_io_type_rio;
	char		m_has_cmd;
	char		m_has_version;
	char		m_has_sfd;
};

struct mpk_interface;
struct mpk_operations {
	int	(*m_decode)(struct mpk_interface *, char *, int *, struct nid_message_hdr *);
	int	(*m_encode)(struct mpk_interface *, char *, int *, struct nid_message_hdr *);
	void	(*m_cleanup)(struct mpk_interface *);
};

struct mpk_interface {
	void			*m_private;
	struct mpk_operations	*m_op;
};

struct mpk_setup {
	int			type;
#ifndef __KERNEL_
	char			do_pp2;
	struct pp2_interface	*pp2;
#endif
};
extern int encode_type_bio (char * p, int *len, struct nid_message_hdr * msghdr_p);
extern int decode_type_bio (char * p, int *left, struct nid_message_hdr * reshdr_p);
extern int encode_type_doa (char * p, int *len, struct nid_message_hdr * msghdr_p);
extern int decode_type_doa (char * p, int *len, struct nid_message_hdr * msghdr_p);
extern int mpk_initialization(struct mpk_interface *, struct mpk_setup *);
#endif
