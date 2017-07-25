#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#include "nid_log.h"
#include "nid.h"
#include "server.h"
#include "tp_if.h"
#include "hs_if.h"
#include "cm_if.h"
#include "nw_if.h"

#define NID_SERVER_ADDR	"127.0.0.1"
#define NID_SERVER_PORT	10809

static struct nw_interface ut_nw;

int 
opennet(char *ipstr, u_short port)
{
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);

	nid_log_info("client opennet start (%s, %d)", ipstr, port); 
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

	nid_log_info("client connecting to (%d, %d)", saddr.sin_port, port);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		nid_log_error("cannot connect to the server (%s:%d)", ipstr, port);
		exit(1);
	}

	return sfd;
}

int
make_data_connection(u_short port)
{
	struct sockaddr_in saddr;
	int sfd;

	nid_log_info("client make_data_connection start (port:%d)", port); 
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("client make_data_connection: cannot bind to %d", port);
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


int
client(struct hs_interface *hs_p)
{
	int sfd, data_sfd, new_sfd, nwrite, len;
	char buf[1024];
	struct sockaddr_in caddr;
	socklen_t clen;
	u_short port;

	nid_log_info("nw client start");
	sfd = opennet(NID_SERVER_ADDR, NID_SERVER_PORT);
	nid_log_info("got connected with the server, sfd:%d", sfd);
	get_one_message(sfd, buf);

	port = NID_SERVER_PORT;
	do {
		port++;
		data_sfd = make_data_connection(port);
	} while (data_sfd < 0);
	len = hs_p->h_op->h_port_generator(hs_p, "127.0.0.1", port, buf);
	nwrite = write(sfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		exit(1);
	}

	clen = sizeof(caddr);
	new_sfd = accept(data_sfd, (struct sockaddr *)&caddr, &clen);
	nid_log_info("client: got connect from ths server, fd:%d", new_sfd);
	
	close(sfd);
	nid_log_info("nw client done");
	return 0;
}

int
main(int argc, char* argv[])
{
	struct tp_setup tp_setup = {10, 100, 5, 0};
	struct tp_interface *tp_p;
	struct tp_setup wtp_setup = {8, 8, 0, 0};
	struct tp_interface *wtp_p;
	struct hs_setup hs_setup;
	struct hs_interface *hs_p;
	struct cm_setup cm_setup;
	struct cm_interface *cm_p;
	struct nw_setup nw_setup;

	nid_log_open();
	nid_log_info("ut start ...");
	
	hs_p = calloc(1, sizeof(*hs_p));
	hs_initialization(hs_p, &hs_setup);
	if (argc > 1 && !strcmp(argv[1], "c")) {
		nid_log_info("ut run as client");
		return client(hs_p);
	}	       

	tp_p = calloc(1, sizeof(*tp_p));
	tp_initialization(tp_p, &tp_setup);
	wtp_p = calloc(1, sizeof(*wtp_p));
	tp_initialization(wtp_p, &wtp_setup);

	cm_setup.tp = tp_p;
	cm_setup.wtp = wtp_p;
	cm_setup.hs = hs_p;
	cm_p = calloc(1, sizeof(*cm_p));
	cm_initialization(cm_p, &cm_setup);

	memset(&nw_setup, 0, sizeof(nw_setup));
	nw_setup.type = NID_TYPE_SERVICE;
	nw_setup.tp = tp_p;
	nw_setup.cg = (void*)cm_p;
	nw_setup.port = NID_SERVER_PORT;
	nw_setup.ipstr = NID_SERVER_ADDR;
	nw_initialization(&ut_nw, &nw_setup);
	sleep(10);

	nid_log_info("ut end...");
	nid_log_close();

	return 0;
}
