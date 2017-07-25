/*
 * nid.h
 */
#ifndef _NID_H
#define _NID_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>


/*
 * Size
 */
#define NID_SIZE_REQHDR		32			// sizeof(struct nid_request)
#define NID_SIZE_REQBUF	 	(32*4096)		// for istream buffer size
#define NID_SIZE_FP		32			// for finger print
#define NID_SIZE_FP_WGT		16			// for fp max weight
#define NID_SIZE_VERSION	4
#define NID_SIZE_CONN_MAGIC	3
#define NID_SIZE_PROTOCOL_VER	2
#define NID_SIZE_CTYPE_MSG	1 + NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER + 2 + NID_SIZE_VERSION // 1 byte is for new_message_flag, 2 bytes are for ctype

/*
 * Offset
 */
#define NID_OFF_COMMON_LOW	0
#define NID_OFF_COMMON_HIGH	((1<<21) - 1)		// 2M-1

/*
 * Port number
 */
#define NID_AGENT_PORT		10808
#define NID_SERVICE_PORT	10809
#define NID_SERVER_PORT		10809

/*
 * Role type
 */
#define NID_ROLE_SERVICE	1
#define NID_ROLE_AGENT		2

/*
 * Conf path
 */
#define NID_CONF_AGENT		"/etc/ilio/nidagent.conf"
#define NID_CONF_SERVICE	"/etc/ilio/nidserver.conf"

/*
 * Channel type
 */
#define	NID_CTYPE_ADM		1
#define NID_CTYPE_REG		2
#define	NID_CTYPE_DOA		4
#define	NID_CTYPE_SNAP		5
#define	NID_CTYPE_WC		6
#define	NID_CTYPE_CRC		7
#define	NID_CTYPE_BWC		8
#define NID_CTYPE_MRW		11
#define NID_CTYPE_TWC		12
#define NID_CTYPE_NWC0		13
#define NID_CTYPE_SAC		14
#define NID_CTYPE_SDS		15
#define NID_CTYPE_PP		16
#define NID_CTYPE_DRW		17
#define NID_CTYPE_CXA		18
#define NID_CTYPE_CXP		19
#define NID_CTYPE_DXA		20
#define NID_CTYPE_DXP		21
#define NID_CTYPE_TP		23
#define NID_CTYPE_INI		24
#define NID_CTYPE_REPT		25
#define NID_CTYPE_REPS		26
#define NID_CTYPE_SERVER	27
#define NID_CTYPE_AGENT		28
#define NID_CTYPE_DRIVER	29
#define NID_CTYPE_ALL		30
#define NID_CTYPE_MAX		31


/*
 * NID Magic
 */
#define NID_CONN_MAGIC		"NID"

/*
 * NID PROTOCOL
 */
#define NID_PROTOCOL_VER	10
#define NID_PROTOCOL_RANGE_LOW	10
#define NID_PROTOCOL_RANGE_HIGH	20

/*
 * Channel access level
 */
#define	NID_ALEVEL_INVALID	0
#define	NID_ALEVEL_MIN		1
#define	NID_ALEVEL_RDONLY	1
#define NID_ALEVEL_RDWR		2
#define	NID_ALEVEL_MAX		2
static inline char*
nid_alevel_to_str(int alevel)
{
	switch (alevel) {
	case NID_ALEVEL_RDONLY: return "readonly";
	case NID_ALEVEL_RDWR:	return "read write";
	default:		return "unknown";
	}
}

/*
 * Handshake message
 */
#define NID_HS_HELO		1
#define NID_HS_EXP		2
#define NID_HS_PORT		3

/*
 * timer
 */
#define NID_KEEPALIVE_TO	10	// timeout for keepalive
#define NID_KEEPALIVE_XTIME	2	// period for sending keepalive

/*
 * globe thread pool
 */
#define NID_GLOBAL_TP	"nid_gtp"
#define NID_NW_TP	"nid_nwtp"
/*
 * sizeof nid_command should be 32
 */
struct nid_request {
	int16_t		magic;
	char		code;
	char		cmd;
	uint32_t	len;		// data len
	uint64_t	offset;
	uint64_t	dseq;
	char 		handle[8];
} __attribute__((packed));

/*
 * Sockt write time out
 */
#define SOCKET_WRITE_TIME_OUT 5

static inline void* x_malloc(size_t size) {
	void *x_ptr = malloc(size);
	if (!x_ptr) {
		assert(0);
	}
	return x_ptr;
};

static inline void* x_calloc(size_t nmemb, size_t size) {
	void *x_ptr = calloc(nmemb, size);
	if (!x_ptr) {
		assert(0);
	}
	return x_ptr;
};

static inline int x_posix_memalign(void **memptr, size_t alignment, size_t size) {
	int x_ret = posix_memalign(memptr, alignment, size);
	if (x_ret != 0) {
		assert(0);
	}
	return x_ret;
}

#endif
