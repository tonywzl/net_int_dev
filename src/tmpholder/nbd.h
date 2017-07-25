/*
 * 1999 Copyright (C) Pavel Machek, pavel@ucw.cz. This code is GPL.
 * 1999/11/04 Copyright (C) 1999 VMware, Inc. (Regis "HPReg" Duchesne)
 *            Made nbd_end_request() use the io_request_lock
 * 2001 Copyright (C) Steven Whitehouse
 *            New nbd_end_request() for compatibility with new linux block
 *            layer code.
 * 2003/06/24 Louis D. Langholtz <ldl@aros.net>
 *            Removed unneeded blksize_bits field from nbd_device struct.
 *            Cleanup PARANOIA usage & code.
 * 2004/02/19 Paul Clements
 *            Removed PARANOIA, plus various cleanup and comments
 */
#ifndef LINUX_NBD_H
#define LINUX_NBD_H


#include <linux/wait.h>
#include <linux/mutex.h>
#include <uapi/linux/nbd.h>

#define NBD_NUM_CHANNEL	1

struct request;
struct nbd_device {
	int flags;
	int harderror;					/* Code of hard error			*/
	struct socket *sock[NBD_NUM_CHANNEL];
	struct socket *data_sock[NBD_NUM_CHANNEL];
	struct file *file[NBD_NUM_CHANNEL]; 		/* If == NULL, device is not ready, yet	*/
	struct file *data_file[NBD_NUM_CHANNEL]; 	/* If == NULL, device is not ready, yet	*/
	int magic;
	long unsigned int sequence[NBD_NUM_CHANNEL];				

	spinlock_t queue_lock;
	struct list_head queue_head[NBD_NUM_CHANNEL];	/* Requests waiting result */
	struct request *active_req;
	struct request *cntl_req;
	wait_queue_head_t active_wq;
	struct list_head waiting_queue[NBD_NUM_CHANNEL];/* Requests to be sent */
	wait_queue_head_t waiting_wq;
	wait_queue_head_t ack_wq;
	wait_queue_head_t control_wq;

	struct mutex tx_lock;
	struct gendisk *disk;
	int blksize;
	u64 bytesize;
	pid_t pid; /* pid of nbd-client, if attached */
	int xmit_timeout;
	int disconnect; /* a disconnect has been requested by user */
	int inited;
	long int last_to_send;		/* last time (jiffies) try to send a msg */
	long int last_sent;		/* last time (jiffies) sent a msg */
	long int last_to_receive;	/* last time (jiffies) try to recvmsg */
	long int last_received;		/* last time (jiffies) received a msg */
	int waiting_len;		/* len of waiting_queue */
	int ack_len;			/* len of queue_head */
	int nr_req;
	struct task_struct *nbd_thread[NBD_NUM_CHANNEL];
	struct task_struct *nbd_do_thread[NBD_NUM_CHANNEL];
	struct task_struct *control_thread;
	struct work_struct work;
	struct nid_request *req_buf[NBD_NUM_CHANNEL];
	int chan_idx;
	char reg_done:1;
	char cntl_req_done:1;
};

#endif
