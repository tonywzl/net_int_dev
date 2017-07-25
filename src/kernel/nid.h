#ifndef _NID_H
#define _NID_H

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/bio.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <net/sock.h>
#include <linux/net.h>
#include <linux/kthread.h>

#include <asm/uaccess.h>
#include <asm/types.h>

#include "nid_shared.h"

#define NID_NAME_SIZE		128

/* values for flags field */
#define NID_FLAG_HAS_FLAGS    (1 << 0) /* nbd-server supports flags */
#define NID_FLAG_READ_ONLY    (1 << 1) /* device is read-only */
#define NID_FLAG_SEND_FLUSH   (1 << 2) /* can flush writeback cache */
/* there is a gap here to match userspace */
#define NID_FLAG_SEND_TRIM    (1 << 5) /* send trim/discard */

#define nid_cmd(req) ((req)->cmd[0])

#define NID_REQUEST_MAGIC 0x35698712
#define NID_REPLY_MAGIC 0x68344281

#include <linux/types.h>

enum {
        NID_CMD_READ = 0,
        NID_CMD_WRITE = 1,
        NID_CMD_DISC = 2,
        NID_CMD_FLUSH = 3,
        NID_CMD_TRIM = 4,
        NID_CMD_CONTROL = 5,
};

enum {
        NID_CNTL_KEEPALIVE = 1,
        NID_CNTL_GETSTAT = 2,
        NID_CNTL_SETSTAT = 3,
};

/*
 * sizeof nid_request should be 32
 */
struct nid_request {
	__be16		magic;
	char		code;
	char		cmd;
	__be32		len;		// data len
	__be64		offset;
	__be64		dseq;		// input data sequence number, high 32 bits
	char 		handle[8];
};

struct nid_control {
	int len;
	char cmd;
	char pad[3];
	char data[0];
};

struct nid_cntl_request {
	struct request req;
	struct nid_control cntl_info;
};

struct nid_device {
	//dev_t			dev;
	int			magic;
	int			flags;
	int			harderror;
	unsigned long		sequence;
	unsigned long		last_keepalive;

	pid_t			pid;
	int			xmit_timeout;
	int			disconnect;
	int			inited;

	struct mutex		*nid_mutex;
	struct task_struct	*nid_ctl_snd_thread;
	struct task_struct	*nid_dat_snd_thread;
	struct task_struct	*nid_ctl_rcv_thread;
	struct task_struct	*nid_dat_rcv_thread;

	struct file		*ctl_file;
	struct file		*data_file;
	struct socket		*ctl_sock;
	struct socket		*data_sock;

	spinlock_t		queue_lock;
	struct list_head	ctl_snd_queue;
	int			ctl_snd_len;
	unsigned long		ctl_snd_len_total;
	wait_queue_head_t	ctl_snd_wq;

	struct list_head	dat_snd_queue;
	int			dat_snd_len;
	unsigned long		dat_snd_len_total;
	unsigned long		dat_snd_err_total;
	unsigned long		dat_snd_done_total;
	wait_queue_head_t	dat_snd_wq;

	struct list_head	ctl_rcv_queue;
	int			ctl_rcv_len;
	unsigned long		ctl_rcv_len_total;
	wait_queue_head_t	ctl_rcv_wq;

	struct list_head	dat_rcv_queue;
	int			dat_rcv_len;
	unsigned long		dat_rcv_len_total;
	wait_queue_head_t	dat_rcv_wq;

	int			nr_req;
	unsigned long		nr_req_total;
	unsigned long		nr_bad_req_total;

	struct request		*curr_req;
	wait_queue_head_t	curr_wq;

	struct request_queue	*nid_queue;
	struct gendisk		*disk;
	int			blksize;
	u64			bytesize;

	struct work_struct	work;
	char			reg_done;
	int			error_now;
	struct nid_request	*req_buf;

	struct request		*cntl_req;;
	char			cntl_req_done;
	struct completion	wait; //DEBUG

	uint64_t		kcounter;	//send keeplive
	uint64_t		r_kcounter;	//receive keeplive
	uint64_t		rcounter;	//send read
	uint64_t		r_rcounter;	//receive read
	uint64_t		wcounter;	//send write
	uint64_t		r_wcounter;	//receive write
	uint64_t		tcounter;	//send trim
	uint64_t		r_tcounter;	//receive trim
	uint64_t		dat_snd_sequence;	//send data in bytes
	uint64_t		dat_rcv_sequence;	//receive data in bytes
};

struct drv_interface;
extern void nid_driver_stop(struct drv_interface *drv_p);
extern int nid_device_send_keepalive(int minor);
extern int nid_device_init(int minor, uint64_t size, uint32_t blksize);
extern int nid_device_start(int minor, int rsfd, int dsfd);
extern void nid_device_set_ioerror(int minor);
extern int nid_device_set_sock(int minor, int rsfd);
extern int nid_device_set_dsock(int minor, int dsfd );
extern void nid_device_to_delete(int minor);
extern void nid_device_to_recover(int minor);
extern int nid_device_send_upgrade(int minor, char force);

#endif
