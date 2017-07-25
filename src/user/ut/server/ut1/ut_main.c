#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <assert.h>

#include "nid_log.h"
#include "tp_if.h"
#include "hs_if.h"
#include "nid.h"

#define NID_SERVER_PORT	10809

struct client_job {
	struct tp_jobheader	j_header;
	int			j_rsfd;
	int			j_dsfd;
	uint64_t		j_loop_num;
};

sem_t	ut_sem;

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
generate_read_request(struct nid_request *req, uint32_t len)
{
	req->cmd = NID_REQ_READ;
	req->len = htonl(len);
	req->offset = htonl(0);
}

static uint64_t d_sequence = 0;
void
generate_write_request(struct nid_request *req, uint32_t len)
{
	req->cmd = NID_REQ_WRITE;
	req->len = htonl(len);
	req->offset = htonl(0);
	req->dseq = htobe64(d_sequence);
	d_sequence += len;	
}

static void
do_response(struct tp_jobheader *job)
{
	struct client_job *job_p = (struct client_job *)job;
	char buf[4096], *p;
	int to_read, nread, total_read;
	struct nid_request *ir_p;
	uint64_t counter = 0, rcounter = 0, wcounter = 0;	

next_loop:
	p = buf;
	to_read = 4096;
	total_read = 0;
	while (to_read) {
		nread = read(job_p->j_rsfd, p, to_read);
       		if (nread <= 0) {
			nid_log_error("do_response nread:%d, errno:%d, counter:%lu", nread, errno, counter);
			return;
		}	
		p += nread;
		total_read += nread;
		to_read = NID_SIZE_REQHDR - (total_read % NID_SIZE_REQHDR);
		to_read %= NID_SIZE_REQHDR;
	}

	ir_p = (struct nid_request *)buf;
	while (total_read) {
		switch (ir_p->cmd) {
		case NID_REQ_READ: {
			char rbuf[131072];	//128K
			int len;
			len = ntohl(ir_p->len);
			p = rbuf;
			while (len) {
				nread = read(job_p->j_dsfd, p, len); 
				if (nread <= 0) {
					nid_log_error("do_response: read wrong, nread:%d, errno:%d", nread, errno);
					return;
				}
				len -= nread;
				p += nread;
			}
			rcounter++;
			break;
		}
		case NID_REQ_WRITE:
			wcounter++;
			break;
		default:
			nid_log_error("do_response: got wrong response cmd (%d)", ir_p->cmd);
			assert(0);
			break;
		}
		total_read -= NID_SIZE_REQHDR;	
		ir_p++;
		counter++;
	}
	if (counter % 100000 == 0) {
		printf("the %luth response, nread:%lu, nwrite:%lu\n", counter, rcounter, wcounter);
	}
	if (counter == job_p->j_loop_num) {
		printf("the %luth response done\n", job_p->j_loop_num);
		sem_post(&ut_sem);
		return;	
	}
	goto next_loop;

}

static void
process_response(int rsfd, int dsfd, uint64_t loop_num)
{
	struct tp_interface *tp_p;
	struct tp_setup setup = {1, 10, 1};
	struct client_job *job;

	tp_p = calloc(1, sizeof(*tp_p));
	tp_initialization(tp_p, &setup);

	job = calloc(1, sizeof(*job));
	job->j_rsfd = rsfd;
	job->j_dsfd = dsfd;
	job->j_loop_num = loop_num;
	job->j_header.jh_do = do_response;
	job->j_header.jh_free = NULL;
	tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job); 
}

static void
client(struct hs_interface *hs_p, char *ipstr, char *uuid, uint64_t loop_num)
{
	int rsfd, dsfd, data_sfd, nwrite, len;
	char buf[1024];
	struct sockaddr_in caddr;
	socklen_t clen;
	u_short port;
	struct nid_request read_req, write_req;
	uint64_t exp_size;
	uint32_t size_l, size_h;
	uint64_t i;

	nid_log_info("nw client start");

	rsfd = make_control_connection(ipstr, NID_SERVER_PORT);
	get_one_message(rsfd, buf);

	len = hs_p->h_op->h_exp_generator(hs_p, uuid, buf);
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
	process_response(rsfd, dsfd, loop_num);	

	for (i = 0; i < loop_num; i++) {
		int rw_len = 4096*((i%32)+1);
		rw_len = 4096;

		if (i%100000 == 0) {
			printf("the %luth loop\n", i);
		}
		generate_read_request(&read_req, rw_len);
#if 0
		if (write(rsfd, &read_req, NID_SIZE_REQHDR) <= 0) {
			printf("write error, errno:%d\n", errno);
			exit(1);
		}
#endif
		
		generate_write_request(&write_req, rw_len);

#if 1
		char wbuf[131072] = {'A'};	//128K
		if (write(rsfd, &write_req, NID_SIZE_REQHDR) <= 0) {
			printf("write error, errno:%d\n", errno);
			exit(1);
		}
		if (write(dsfd, wbuf, rw_len) <= 0) {
			printf("write error, errno:%d\n", errno);
			exit(1);
		}
#endif
	}
	printf("done, i:%lu\n", i);
	sem_wait(&ut_sem);
	close(dsfd);
	close(rsfd);
	nid_log_info("nw client done");
}

int
main(int argc, char* argv[])
{
	struct hs_setup hs_setup;
	struct hs_interface *hs_p;
	char *ipstr = "127.0.0.1";
	char *uuid = "export1";
	uint64_t loop_num =  1000000;

	nid_log_open();
	nid_log_info("nidc start ...");
	
	if (argc > 1) {
		ipstr = argv[1];
	}

	if (argc > 2) {
		uuid = argv[2];
	}

	if (argc > 3) {
		loop_num = atol(argv[3]);
	}
	printf("client: ipstr:%s, uuid:%s, loop_num:%lu\n", ipstr, uuid, loop_num);

	sem_init(&ut_sem, 0, 0);
	hs_p = calloc(1, sizeof(*hs_p));
	hs_initialization(hs_p, &hs_setup);
	client(hs_p, ipstr, uuid, loop_num);

	nid_log_info("nidc end...");
	nid_log_close();

	return 0;
}
