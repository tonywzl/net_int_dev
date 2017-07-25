/*
 * nid.h
 */
#ifndef _NID_H
#define _NID_H

#include <stdint.h>

#define SERVER_PORT		10809
#define SERVER_CONF_PATH	"/etc/ilio/nids.conf"

#define CC_REQ_BUFSIZE	 	(32*4096)		// for istream buffer size	
#define CC_REQ_SIZE		32			// sizeof(struct nid_request)
#define CC_REQ_MAXNODES		32768			// should not be less than CC_REQ_BUFSIZE/CC_REQ_SIZE		

#define	CC_CTYPE_ADM		1
#define CC_CTYPE_REG		2

#define DS_BUFSHIFT		25			// 32M
#define DS_BUFSIZE 		(1 << DS_BUFSHIFT)
#define DS_BUFMASK		(~(DS_BUFSIZE - 1))
#define DS_BUFRMASK		(DS_BUFSIZE - 1)
#define DS_PAGENRSHIFT		2			// 4 pages
#define	DS_PAGENR		(1 << DS_PAGENRSHIFT)	
#define DS_PAGESHIFT		23			// 8M
#define DS_PAGESIZE		(1 << DS_PAGESHIFT)
#define DS_PAGEMASK		(~(DS_PAGESIZE - 1))
#define DS_PAGERMASK		(DS_PAGESIZE - 1)

/*
 * for server admin command
 */
#define SA_CMD_STOP		1
#define SA_CMD_STAT_RESET	2
#define SA_CMD_STAT_GET		3
#define SA_CMD_UPDATE		4


#define NID_NAME_SIZE		128
#define NID_REQ_READ            1
#define NID_REQ_WRITE           2
#define NID_REQ_DISC            3
#define NID_REQ_FLUSH           4
#define NID_REQ_TRIM            5

#define NID_NUM_CHANNEL		1

/*
 * sizeof nid_command should be 32
 */
struct nid_request {
	int16_t		magic;
	char		type;
	char		cmd;
	uint32_t	len;		// data len
	uint64_t	offset;
	uint64_t	dseq;
	char 		handle[8];
} __attribute__((packed));

#endif
