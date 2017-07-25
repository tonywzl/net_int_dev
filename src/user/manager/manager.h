/*
 * manager.h
 */
#ifndef _MANAGER_H
#define _MANAGER_H

#define M_MODE_DEFAULT		0
#define M_MODE_STATUS		1
#define M_MODE_CONTROL		2
#define M_MODE_DEBUG		3
#define M_MODE_MAX		4

#define M_JOB_DEFAULT		0
#define M_JOB_BIO		1
#define M_JOB_DOA		2
#define M_JOB_WC		3
#define M_JOB_RC		4
#define M_JOB_BWC		5
#define M_JOB_CRC		6
#define M_JOB_DCK		7
#define M_JOB_DRC		8
#define M_JOB_MRW		9
#define M_JOB_MAX		10

#define first_intend		"\t"
#define second_intend		"\t"

# define no_argument        0
# define required_argument  1
# define optional_argument  2


//extern void usage();
//extern void mod_ifn();


extern int manager_server_bio (int argc, char * const argv[]);
extern int manager_server_doa (int argc, char * const argv[]);
extern int manager_server_wc (int argc, char * const argv[]);
extern int manager_server_rc (int argc, char * const argv[]);
extern int manager_server_bwc (int argc, char * const argv[]);
extern int manager_server_crc (int argc, char * const argv[]);
extern int manager_server_mrw (int argc, char * const argv[]);
extern int manager_server_sac (int argc, char * const argv[]);
extern int manager_server_pp (int argc, char * const argv[]);
extern int manager_server_sds (int argc, char * const argv[]);
extern int manager_server_drw (int argc, char * const argv[]);
extern int manager_server_dx (int argc, char * const argv[]);
extern int manager_server_cx (int argc, char * const argv[]);
extern int manager_server_twc (int argc, char * const argv[]);
extern int manager_server_tp (int argc, char * const argv[]);
extern int manager_server_ini (int argc, char * const argv[]);
extern int manager_server_server (int argc, char * const argv[]);
extern int manager_server_all (int argc, char * const argv[]);
extern int manager_agent_agent (int argc, char * const argv[]);
extern int manager_driver_driver (int argc, char * const argv[]);

extern int manager_stat_dri (int argc, char * const argv[]);
extern int manager_ctrl_ser (int argc, char * const argv[]);
extern int manager_ctrl_age (int argc, char * const argv[]);
extern int manager_ctrl_dri (int argc, char * const argv[]);
extern int manager_leg_age (int argc, char * const argv[]);
extern int manager_leg_dri (int argc, char * const argv[]);
extern int manager_leg_ser (int argc, char * const argv[]);


#endif



