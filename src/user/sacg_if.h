/*
 * sacg_if.h
 * 	Interface of Server Agent Channel (sac) Guardian Module
 */
#ifndef NID_SACG_IF_H
#define NID_SACG_IF_H

struct sacg_interface;
struct sac_interface;
struct umessage_sac_list_stat_resp;

struct sacg_operations {
	void*			(*sag_accept_new_channel)(struct sacg_interface *, int, char *);
	void			(*sag_do_channel)(struct sacg_interface *, void *);
	int			(*sag_job_dispatcher)(struct sacg_interface *);
	void			(*sag_get_stat)(struct sacg_interface *, struct list_head *);
	void			(*sag_request_coming)(struct sacg_interface *);
	void			(*sag_reset_stat)(struct sacg_interface *);
	void			(*sag_update)(struct sacg_interface *);
	int			(*sag_check_conn)(struct sacg_interface *, char *);
	int			(*sag_upgrade_alevel)(struct sacg_interface *, struct sac_interface *);
	int			(*sag_fupgrade_alevel)(struct sacg_interface *, struct sac_interface *);
	void			(*sag_stop)(struct sacg_interface *);
	int			(*sag_add_sac)(struct sacg_interface *, char *, char, char, char, int, char *, char *, char *, uint32_t, char *);
	int			(*sag_delete_sac)(struct sacg_interface *, char *);
	int			(*sag_switch_sac)(struct sacg_interface *, char *, char *, int);
	struct sac_info*	(*sag_get_sac_info)(struct sacg_interface *, char *);
	int                     (*sag_set_keepalive_sac)(struct sacg_interface *, char *, uint16_t, uint16_t);
	struct sac_info*	(*sag_get_all_sac_info)(struct sacg_interface *, int *);
	char**			(*sag_get_working_sac_name)(struct sacg_interface *, int *);
	struct sac_interface *	(*sag_get_sac)(struct sacg_interface *, char *);
	int			(*sag_start_fast_release)(struct sacg_interface *, char *);
	int			(*sag_stop_fast_release)(struct sacg_interface *, char *);
	int			(*sag_get_counter)(struct sacg_interface *, char *, uint32_t *, uint32_t *);
	int			(*sag_get_list_stat)(struct sacg_interface *, char *, struct umessage_sac_list_stat_resp *);
};

struct sacg_interface {
	void			*sag_private;
	struct sacg_operations	*sag_op;
};

struct sacg_setup {
	struct scg_interface	*scg;
	struct sdsg_interface	*sdsg;
	struct cdsg_interface	*cdsg;
	struct wcg_interface	*wcg;
	struct crcg_interface	*crcg;
	struct rwg_interface	*rwg;
	struct tpg_interface	*tpg;
	struct tp_interface	*wtp;
	struct ini_interface	**ini;
	struct io_interface	**all_io;
//	pthread_mutex_t		*ash_lck;
//	struct ini_key_desc	*server_keys;
	struct list_head	server_keys;
	struct mqtt_interface	*mqtt;
};

extern int sacg_initialization(struct sacg_interface *, struct sacg_setup *);
#endif
