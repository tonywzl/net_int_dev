/*
 * crcg_if.h
 * 	Interface of CAS (Content-Addressed Storage) Read Cache Module Guardian Module
 */
#ifndef NID_CRCG_IF_H
#define NID_CRCG_IF_H

#define	CRCG_MAX_CRC	2

struct nse_stat;
struct crcg_interface;
struct crcg_operations {
	struct rc_interface*					(*rg_search_crc)(struct crcg_interface *, char *);
	struct rc_interface*					(*rg_search_and_create_crc)(struct crcg_interface *, char *);
	int							(*rg_add_crc)(struct crcg_interface *, char *, char *, char *, int);
	void							(*rg_dropcache_start)(struct crcg_interface *, char *, char *, int);
	void							(*rg_dropcache_stop)(struct crcg_interface *, char *, char *);
	void							(*rg_set_fp_wgt)(struct crcg_interface *, char *, int);
	uint64_t						(*rg_freespace)(struct crcg_interface *, char *);
	int*							(*rg_sp_heads_size)(struct crcg_interface *, char *);
	void							(*rg_cse_hit)(struct crcg_interface *, char *, int *, int *);
	unsigned char 						(*rg_check_fp)(struct crcg_interface *, char *);
	struct umessage_crc_information_resp_freespace_dist*	(*rg_freespace_dist)(struct crcg_interface *, char *);
	int							(*rg_get_nse_stat)(struct crcg_interface *, char *, char *, struct nse_stat *);
	void*							(*rg_nse_traverse_start)(struct crcg_interface *, char *, char *);
	int							(*rg_nse_traverse_next)(struct crcg_interface *, char *, void *, char *, uint64_t *);
	void							(*rg_nse_traverse_stop)(struct crcg_interface *, char *, void *);
	struct list_head* 					(*rg_dsbmp_rtree_list)(struct crcg_interface *, char *);
	struct dsrec_stat 					(*rg_get_dsrec_stat)(struct crcg_interface *, char *);
	struct crc_setup*					(*rg_get_all_crc_setup)(struct crcg_interface *, int *);
	int*							(*rg_get_working_crc_index)(struct crcg_interface *, int *);
};

struct crcg_interface {
	void			*rg_private;
	struct crcg_operations	*rg_op;
};

struct ini_interface;
struct ppg_interface;
struct tpg_interface;
struct crcg_setup {
	struct ini_interface		**ini;
	struct ppg_interface		*ppg;
	struct allocator_interface	*allocator;
	struct fpn_interface		*fpn;
	struct brn_interface		*brn;
	struct srn_interface		*srn;
	struct tpg_interface		*tpg;
};

extern int crcg_initialization(struct crcg_interface *, struct crcg_setup *);
#endif
