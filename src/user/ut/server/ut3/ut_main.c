#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "nid_log.h"
#include "tp_if.h"
#include "hs_if.h"
#include "nid.h"

//#define NID_SERVER_ADDR	"10.121.138.40"
#define NID_SERVER_ADDR	"127.0.0.1"
#define NID_SERVER_PORT	10809

struct client_job {
	struct tp_jobheader	j_header;
	int			j_rsfd;
	int			j_dsfd;
};

static int 
make_control_connection(char *ipstr, u_short port)
{
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);
	char chan_type = CC_CTYPE_REG;

	//nid_log_info("client make_control_connection start (%s, %d)", ipstr, port); 
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("cannot open a socket");
		exit(1);
	}
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len);

	nid_log_info("client connecting to (%s:%d, %d)", ipstr, saddr.sin_port, port);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		nid_log_error("cannot connect to the server (%s:%d), errno:%d",
			ipstr, port, errno);
		exit(1);
	}

	write(sfd, &chan_type, 1);
	return sfd;
}

static int
make_data_connection(u_short port)
{
	struct sockaddr_in saddr;
	int sfd;

	//nid_log_info("client make_data_connection start (port:%d)", port); 
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		return -1;
	}
	
	listen(sfd, 5);
	return sfd;
}

void
get_one_message(int sfd, char *buf)
{
	int nread;
	uint16_t msg_len;
	char *p = buf, cmd;

	memset(buf, 0, 128);
	nread = read(sfd, p, 3);

	cmd = *p++;
	msg_len = (*p++) << 8;
	msg_len += *p++;
        nread = read(sfd, p, msg_len);	
	if (nread < 0) {
		nid_log_error("get_one_message: read error, errno:%d", errno);
	}

	nid_log_debug("get_one_message: got cmd:%d, len:%d, msg:{%s}", cmd, msg_len, p); 
}

void
generate_keepalive_request(struct nid_request *req)
{
	req->cmd = NID_REQ_KEEPALIVE;
}


static void
client(struct hs_interface *hs_p, char *ipstr, char *exportname, int loop_num)
{
	int rsfd, dsfd, data_sfd, nwrite, len;
	char buf[1024];
	struct sockaddr_in caddr;
	socklen_t clen;
	u_short port;
	struct nid_request ka_req;
	uint64_t exp_size;
	uint32_t size_l, size_h;
	int i;

	nid_log_info("nw client start");

	rsfd = make_control_connection(ipstr, NID_SERVER_PORT);
	get_one_message(rsfd, buf);

	len = hs_p->h_op->h_exp_generator(hs_p, exportname, buf);
	nwrite = write(rsfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		exit(1);
	}

	/* setup size */
	assert(read(rsfd, &size_h, 4) == 4);
	assert(read(rsfd, &size_l, 4) == 4);
	exp_size = ntohl(size_h);
	exp_size <<= 32;
	exp_size += ntohl(size_l);
	nid_log_debug("client: size:%lu", exp_size);

	port = NID_SERVER_PORT;
	do {
		port++;
		data_sfd = make_data_connection(port);
	} while (data_sfd < 0);

	clen = sizeof(caddr);
	getsockname(rsfd, (struct sockaddr *)&caddr, &clen);
	nid_log_debug("channel_establish: my ipstr:%s, port:%d, clen:%d",
		inet_ntoa(caddr.sin_addr), caddr.sin_port,  clen);
	len = hs_p->h_op->h_port_generator(hs_p, inet_ntoa(caddr.sin_addr), port, buf);
	nwrite = write(rsfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		exit(1);
	}

	clen = sizeof(caddr);
	dsfd = accept(data_sfd, (struct sockaddr *)&caddr, &clen);
	nid_log_info("client: got connect from ths server, fd:%d", dsfd);

	/*
	 * Now, we got both control and data connections
	 */	

	for (i = 0; i < loop_num; i++) {
		generate_keepalive_request(&ka_req);
		if (write(rsfd, &ka_req, NID_SIZE_REQHDR) != NID_SIZE_REQHDR) {
			printf("cannot wirte: errno:%d\n", errno);
			break;
		}
		sleep(55);
	}
	printf("done, i:%d\n", i);
	close(dsfd);
	close(rsfd);
	nid_log_info("nw client done");
}

int
main(int argc, char* argv[])
{
	struct hs_setup hs_setup;
	struct hs_interface *hs_p;
	int loop_num = 10;
	char *ipstr = "127.0.0.1";
	char *exportname = "export1";

	nid_log_open();
	nid_log_info("nidc start ...");

	if (argc > 1) {
		ipstr = argv[1];
	}
	if (argc > 2) {
		exportname = argv[2];
	}
	if (argc > 3) {
		loop_num = atoi(argv[3]);
	}

	hs_p = calloc(1, sizeof(*hs_p));
	hs_initialization(hs_p, &hs_setup);
	client(hs_p, ipstr, exportname, loop_num);

	nid_log_info("nidc end...");
	nid_log_close();

	return 0;
}
