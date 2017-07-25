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
#include "tp_if.h"
#include "hs_if.h"
#include "nid.h"

#define NID_SERVER_ADDR	"127.0.0.1"
#define NID_SERVER_PORT	10809

static int 
make_control_connection(char *ipstr, u_short port)
{
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);

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

	//nid_log_info("client connecting to (%d, %d)", saddr.sin_port, port);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		nid_log_error("cannot connect to the server (%s:%d), errno:%d",
			ipstr, port, errno);
		exit(1);
	}

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
generate_read_request(struct nid_request *req)
{
	req->cmd = NID_CMD_READ;
	req->len = htonl(10);
	req->offset_h = htonl(0);
	req->offset_l = htonl(0);
}

static int d_sequence = 0;
void
generate_write_request(struct nid_request *req)
{
	req->cmd = NID_CMD_WRITE;
	req->len = htonl(10);
	req->offset_h = htonl(0);
	req->offset_l = htonl(0);
	req->dseq_h = 0;
	req->dseq_l = htonl(d_sequence);
	d_sequence += 10;	
}

static void
client(struct hs_interface *hs_p, char *exportname)
{
	int csfd, dsfd, data_sfd, nwrite, len;
	char buf[1024];
	struct sockaddr_in caddr;
	socklen_t clen;
	u_short port;
	struct nid_request read_req, write_req;
	int i;

	nid_log_info("nw client start");

	csfd = make_control_connection(NID_SERVER_ADDR, NID_SERVER_PORT);
	get_one_message(csfd, buf);

	len = hs_p->h_op->h_exp_generator(hs_p, exportname, buf);
	nwrite = write(csfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		exit(1);
	}

	port = NID_SERVER_PORT;
	do {
		port++;
		data_sfd = make_data_connection(port);
	} while (data_sfd < 0);
	len = hs_p->h_op->h_port_generator(hs_p, "127.0.0.1", port, buf);
	nwrite = write(csfd, buf, len);
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
	for (i = 0; i < 5; i++) {
		generate_read_request(&read_req);
		write(csfd, &read_req, sizeof(read_req));		

		generate_write_request(&write_req);
		write(csfd, &write_req, sizeof(write_req));		
		write(dsfd, "0123456789", 10);
	}

	sleep(2);
	close(dsfd);
	close(csfd);
	nid_log_info("nw client done");
}

int
main(int argc, char* argv[])
{
	struct hs_setup hs_setup;
	struct hs_interface *hs_p;
	char *exportname;

	nid_log_open();
	nid_log_info("nidc start ...");
	
	if (argc > 1) {
		exportname = argv[1];
	} else {
		exportname = "export1";
	}

	hs_p = calloc(1, sizeof(*hs_p));
	hs_initialization(hs_p, &hs_setup);
	client(hs_p, exportname);

	nid_log_info("nidc end...");
	nid_log_close();

	return 0;
}
