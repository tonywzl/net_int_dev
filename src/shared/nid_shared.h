/*
 * nid_shared.h
 */
#ifndef _NID_SHARED_H
#define _NID_SHARED_H

#define NID_MAGIC 0x41544C41
#define NID_MAJOR 44

/*
 * ioctl cmd
 */
#define NID_CHAN_OPEN		_IO( 0xab, 11 )
#define NID_START_CHANNEL	_IO( 0xab, 12 )
#define NID_INIT_NETLINK	_IO( 0xab, 13 )
#define NID_SET_KERN_LOG_LEVEL	_IO( 0xab, 14 )
#define NID_GET_STAT		_IO( 0xab, 15 )

#define NID_KEEPALIVE_TIMER	5

/*
 * Limits
 */
#define	NID_MAX_CHANNELS	128
#define NID_MAX_DSPAGENRSHIFT	4			// 16 pages
#define NID_MAX_DSPAGENR	(1<<NID_MAX_DSPAGENRSHIFT)
#define NID_MAX_DSPAGESZSHIFT	25			// 32M
#define	NID_MAX_DSBUF		(NID_MAX_DSPAGENRSHIFT + NID_MAX_DSPAGESZSHIFT)
#define NID_MAX_UUID		1024
#define NID_MAX_TPNAME		64
#define NID_MAX_PPNAME		64
#define NID_MAX_DEVNAME		64
#define NID_MAX_DSNAME		64
#define NID_MAX_CRCNAME		64
#define NID_MAX_RSM_CLUSTNAME	64
#define NID_MAX_RSM_CLUSTMNAME	64
#define NID_MAX_IP		16
#define NID_MAX_PATH		1024
#define NID_MAX_RESOURCE	128			// atmost number of resources in a node
#define NID_MAX_REQNODES	4096			// should not be less than NID_SIZE_REQBUF/NID_SIZE_REQHDR
#define NID_MAX_CMD		1024
#define NID_MAX_CMD_RESP	102400
#define NID_MAX_VERSION		64

/*
 * Request
 */
#define NID_REQ_READ			1
#define NID_REQ_WRITE			2
#define NID_REQ_DELETE_DEVICE		3
#define NID_REQ_FLUSH			4
#define NID_REQ_TRIM			5
#define NID_REQ_KEEPALIVE		6
#define NID_REQ_UPGRADE			7	// upgrade channel acccess level
#define NID_REQ_FUPGRADE		8	// forcibly upgrade
#define NID_REQ_HELO			9	// hello
#define NID_REQ_INIT_DEVICE		10
#define NID_REQ_IOERROR_DEVICE		12
#define NID_REQ_RECOVER			13

#define NID_REQ_GETSTAT			15
#define NID_REQ_STAT			16
#define NID_REQ_STAT_SAC		17
#define NID_REQ_STAT_WTP		18
#define NID_REQ_AGENT_STOP		19
#define NID_REQ_DELETE			21
#define NID_REQ_UPDATE			22
#define	NID_REQ_STAT_GET		23
#define NID_REQ_STAT_RESET		24
#define	NID_REQ_STOP			25
#define NID_REQ_CHECK_CONN		26
#define NID_REQ_CHECK_AGENT		27
#define NID_REQ_SET_LOG_LEVEL		28
#define NID_REQ_GET_VERSION		29
#define NID_REQ_CHECK_SERVER		30
#define NID_REQ_STAT_IO			31
#define NID_REQ_BIO_FAST_FLUSH		32
#define NID_REQ_BIO_STOP_FAST_FLUSH	33
#define NID_REQ_STAT_BOC		34
#define NID_REQ_BOC_DRIVER		35
#define NID_REQ_CHANNEL			36
#define NID_REQ_EXECUTOR		37
#define NID_REQ_SET_HOOK		38
#define NID_REQ_BIO			39
#define NID_REQ_DOA			40
#define NID_REQ_SNAP			41
#define NID_REQ_STOP_KEEPALIVE		42
#define NID_REQ_CHECK_DRIVER		43
#define NID_REQ_MAX			44


/* code for bio */
#define NID_REQ_BIO_VEC_START   	1
#define NID_REQ_BIO_VEC_STOP    	2
#define NID_REQ_BIO_VEC_STAT		3
#define NID_REQ_BIO_RELEASE_START	4
#define NID_REQ_BIO_RELEASE_STOP	5

/*code for lck */
#define NID_REQ_DOA_REQUEST		1
#define NID_REQ_DOA_CHECK		2
#define NID_REQ_DOA_RELEASE		3

/* code for channel */
#define NID_CODE_CHANNEL_CREATE		1
#define NID_CODE_CHANNEL_OPEN		2
#define NID_CODE_CHANNEL_DELETE		3
#define NID_CODE_CHANNEL_KEEPALIVE	4
#define NID_CODE_CHANNEL_RECOVER	5
#define NID_CODE_CHANNEL_START      	6
#define NID_CODE_CHANNEL_UPGRADE    	7
/* code for executor */
#define NID_CODE_EXECUTOR_CREATE	1
#define NID_CODE_EXECUTOR_ADD_CHAN	2
#define NID_CODE_EXECUTOR_SET		3

#define BOC_DRIVER_ADD_CHANNEL		1

/*
 * Channel type
 */
#define NID_CHANNEL_TYPE_NONE_CODE  	0
#define NID_CHANNEL_TYPE_ASC_CODE	1
#define NID_CHANNEL_TYPE_CHAN_CODE	2

/*
 * Item
 */

#define NID_ITEM_CHAN_START	1
#define NID_ITEM_UUID		(NID_ITEM_CHAN_START)
#define NID_ITEM_DEVNAME	(NID_ITEM_CHAN_START + 1)
#define NID_ITEM_ALEVEL		(NID_ITEM_CHAN_START + 2)
#define NID_ITEM_RCOUNTER	(NID_ITEM_CHAN_START + 3)
#define NID_ITEM_RREADYCOUNTER	(NID_ITEM_CHAN_START + 4)
#define NID_ITEM_WCOUNTER	(NID_ITEM_CHAN_START + 5)
#define NID_ITEM_KCOUNTER	(NID_ITEM_CHAN_START + 6)	// keepalive counter
#define NID_ITEM_RRCOUNTER	(NID_ITEM_CHAN_START + 7)	// resp read counter
#define NID_ITEM_RWCOUNTER	(NID_ITEM_CHAN_START + 8)	// resp write counter
#define NID_ITEM_RKCOUNTER	(NID_ITEM_CHAN_START + 9)	// resp keepalive counter
#define NID_ITEM_CPOS		(NID_ITEM_CHAN_START + 10)	// channel position
#define NID_ITEM_TIME		(NID_ITEM_CHAN_START + 11)	//
#define NID_ITEM_CODE		(NID_ITEM_CHAN_START + 12)
#define NID_ITEM_IP		(NID_ITEM_CHAN_START + 13)
#define NID_ITEM_RSEQUENCE	(NID_ITEM_CHAN_START + 14)	// recved sequence
#define NID_ITEM_WSEQUENCE	(NID_ITEM_CHAN_START + 15)	// waiting for sequence
#define NID_ITEM_WREADYCOUNTER	(NID_ITEM_CHAN_START + 16)
#define NID_ITEM_UPGRADE_FORCE	(NID_ITEM_CHAN_START + 17)
#define NID_ITEM_HOOK_CMD	(NID_ITEM_CHAN_START + 18)
#define NID_ITEM_SRVSTATUS	(NID_ITEM_CHAN_START + 19)
#define NID_ITEM_CHAN_END	(NID_ITEM_CHAN_START + 20)

#define NID_ITEM_TP_START	(NID_ITEM_CHAN_END + 1)
#define NID_ITEM_NUSED		(NID_ITEM_TP_START)
#define NID_ITEM_NFREE		(NID_ITEM_TP_START + 1)
#define NID_ITEM_NWORKER	(NID_ITEM_TP_START + 2)
#define NID_ITEM_NMAXWORKER	(NID_ITEM_TP_START + 3)
#define NID_ITEM_NNOFREE	(NID_ITEM_TP_START + 4)
#define NID_ITEM_TP_END		(NID_ITEM_TP_START + 4)

#define NID_ITEM_DEV_START	(NID_ITEM_TP_END + 1)
#define NID_ITEM_DEV_MINOR	NID_ITEM_DEV_START
#define NID_ITEM_DEV_END	NID_ITEM_DEV_START

#define NID_ITEM_SOCK_START	(NID_ITEM_DEV_END + 1)
#define NID_ITEM_RSOCK		NID_ITEM_SOCK_START
#define NID_ITEM_DSOCK		(NID_ITEM_SOCK_START + 1)
#define NID_ITEM_SOCK_END	(NID_ITEM_SOCK_START + 1)

#define NID_ITEM_SIZE_START	(NID_ITEM_SOCK_END + 1)
#define NID_ITEM_SIZE		(NID_ITEM_SIZE_START)
#define NID_ITEM_SIZE_BLOCK	(NID_ITEM_SIZE_START + 1)
#define NID_ITEM_SIZE_END	(NID_ITEM_SIZE_START + 1)

#define NID_ITEM_RTREE_START	(NID_ITEM_SIZE_END + 1)
#define NID_ITEM_RTREE_SEGSZ	(NID_ITEM_RTREE_START)
#define NID_ITEM_RTREE_NSEG	(NID_ITEM_RTREE_START + 1)
#define NID_ITEM_RTREE_NFREE	(NID_ITEM_RTREE_START + 2)
#define NID_ITEM_RTREE_NUSED	(NID_ITEM_RTREE_START + 3)
#define NID_ITEM_RTREE_END	(NID_ITEM_RTREE_START + 3)

#define NID_ITEM_BTN_START	(NID_ITEM_RTREE_END + 1)
#define NID_ITEM_BTN_SEGSZ	(NID_ITEM_BTN_START)
#define NID_ITEM_BTN_NSEG	(NID_ITEM_BTN_START + 1)
#define NID_ITEM_BTN_NFREE	(NID_ITEM_BTN_START + 2)
#define NID_ITEM_BTN_NUSED	(NID_ITEM_BTN_START + 3)
#define NID_ITEM_BTN_END	(NID_ITEM_BTN_START + 3)

#define NID_ITEM_IO_START	(NID_ITEM_BTN_END + 1)
#define NID_ITEM_IO_BIO		(NID_ITEM_IO_START)
#define NID_ITEM_IO_OCCUPIED	(NID_ITEM_IO_START + 1)
#define NID_ITEM_IO_WBLOCK	(NID_ITEM_IO_START + 2)
#define NID_ITEM_IO_FBLOCK	(NID_ITEM_IO_START + 3)
#define NID_ITEM_IO_SFLUSHED	(NID_ITEM_IO_START + 4)
#define NID_ITEM_IO_SASSIGNED	(NID_ITEM_IO_START + 5)
#define NID_ITEM_BUF_AVAIL	(NID_ITEM_IO_START + 6)
#define NID_ITEM_IO_RIO		(NID_ITEM_IO_START + 7)
#define NID_ITEM_IO_NBLOCKS	(NID_ITEM_IO_START + 8)
#define NID_ITEM_IO_RECSFLUSHED	(NID_ITEM_IO_START + 9)
#define NID_ITEM_IO_END		(NID_ITEM_IO_START + 9)

#define NID_ITEM_VERSION_START	(NID_ITEM_IO_END + 1)
#define NID_ITEM_VERSION	(NID_ITEM_VERSION_START)
#define NID_ITEM_VERSION_SFD	(NID_ITEM_VERSION_START + 1)
#define NID_ITEM_VERSION_END	(NID_ITEM_VERSION_START + 1)
/*
 * BIO items
 */
#define NID_ITEM_BIO_CHANNEL	1



#define NID_ITEM_BIO_VEC_START		0
#define NID_ITEM_BIO_VEC_FLUSHNUM 	(NID_ITEM_BIO_VEC_START)
#define NID_ITEM_BIO_VEC_VECNUM   	(NID_ITEM_BIO_VEC_START + 1)
#define NID_ITEM_BIO_VEC_WRITESZ  	(NID_ITEM_BIO_VEC_START + 2)
#define NID_ITEM_BIO_VEC_END    	(NID_ITEM_BIO_VEC_START + 2)

#define NID_ITEM_DOA_START	0
#define NID_ITEM_DOA_VID	(NID_ITEM_DOA_START)
#define NID_ITEM_DOA_LID	(NID_ITEM_DOA_START + 1)
#define NID_ITEM_DOA_TIME_OUT	(NID_ITEM_DOA_START + 2)
#define NID_ITEM_DOA_HOLD_TIME	(NID_ITEM_DOA_START + 3)
#define NID_ITEM_DOA_LOCK	(NID_ITEM_DOA_START + 4)
#define NID_ITEM_DOA_END      	(NID_ITEM_DOA_START + 4)

/*
 * stat
 * */
#define NID_STAT_ACTIVE		1
#define NID_STAT_INACTIVE	2

#endif
