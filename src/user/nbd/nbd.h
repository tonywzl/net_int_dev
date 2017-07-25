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

//#include <linux/types.h>
#include "list.h"

#define NBD_SET_SOCK	_IO( 0xab, 0 )
#define NBD_SET_BLKSIZE	_IO( 0xab, 1 )
#define NBD_SET_SIZE	_IO( 0xab, 2 )
#define NBD_DO_IT	_IO( 0xab, 3 )
#define NBD_CLEAR_SOCK	_IO( 0xab, 4 )
#define NBD_CLEAR_QUE	_IO( 0xab, 5 )
#define NBD_PRINT_DEBUG	_IO( 0xab, 6 )
#define NBD_SET_SIZE_BLOCKS	_IO( 0xab, 7 )
#define NBD_DISCONNECT  _IO( 0xab, 8 )
#define NBD_SET_TIMEOUT _IO( 0xab, 9 )
#define NBD_SET_FLAGS _IO( 0xab, 10 )
#define NBD_CONTROL _IO( 0xab, 11 )
#define NBD_SET_DATA_SOCK _IO( 0xab, 12 )


enum {
	NBD_CMD_READ = 0,
	NBD_CMD_WRITE = 1,
	NBD_CMD_DISC = 2,
	NBD_CMD_FLUSH = 3,
	NBD_CMD_TRIM = 4,
	NBD_CMD_CONTROL = 5,
};

enum {
	NBD_CNTL_KEEPALIVE = 1,
	NBD_CNTL_GETSTAT = 2,
	NBD_CNTL_SETSTAT = 3,
};

struct nbd_control {
	int len;
	char cmd;
	char pad[3];
	char data[0];
};

#define NBD_CMD_MASK_COMMAND 0x0000ffff
#define NBD_CMD_FLAG_FUA (1<<16)

/* values for flags field */
#define NBD_FLAG_HAS_FLAGS	(1 << 0)	/* Flags are there */
#define NBD_FLAG_READ_ONLY	(1 << 1)	/* Device is read-only */
#define NBD_FLAG_SEND_FLUSH	(1 << 2)	/* Send FLUSH */
#define NBD_FLAG_SEND_FUA	(1 << 3)	/* Send FUA (Force Unit Access) */
#define NBD_FLAG_ROTATIONAL	(1 << 4)	/* Use elevator algorithm - rotational media */
#define NBD_FLAG_SEND_TRIM	(1 << 5)	/* Send TRIM (discard) */

#define NBD_FLAG_PERSISTENT		(1 << 10)	/* ATLAS-COMMENT: for persistent mode, as defined in nbd.h from kernel */
#define NBD_FLAG_PERSISTENT_HANG	(1 << 11)	/* ATLAS-COMMENT: for persistent hang mode, as defined in nbd.h from kernel */
#define NBD_VERSION "this-is-atlas-version"

#define nbd_cmd(req) ((req)->cmd[0])

/* userspace doesn't need the nbd_device structure */

/* These are sent over the network in the request/reply magic fields */

#define NBD_REQUEST_MAGIC 0x25609513
#define NBD_REPLY_MAGIC 0x67446698
/* Do *not* use magics: 0x12560953 0x96744668. */


struct nbd_stat_info {
	int c_waiting_len;
	int c_ack_len;
	int c_nr_req;
	int s_recv_len;
	int s_reply_len;
	int s_busy_workers;
	int s_free_workers;
};
#endif
