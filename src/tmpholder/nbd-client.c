/*
 * Open connection for network block device
 *
 * Copyright 1997,1998 Pavel Machek, distribute under GPL
 *  <pavel@atrey.karlin.mff.cuni.cz>
 * Copyright (c) 2002 - 2011 Wouter Verhelst <w@uter.be>
 *
 * Version 1.0 - 64bit issues should be fixed, now
 * Version 1.1 - added bs (blocksize) option (Alexey Guzeev, aga@permonline.ru)
 * Version 1.2 - I added new option '-d' to send the disconnect request
 * Version 2.0 - Version synchronised with server
 * Version 2.1 - Check for disconnection before INIT_PASSWD is received
 * 	to make errormsg a bit more helpful in case the server can't
 * 	open the exported file.
 * 16/03/2010 - Add IPv6 support.
 * 	Kitt Tientanopajai <kitt@kitty.in.th>
 *	Neutron Soutmun <neo.neutron@gmail.com>
 *	Suriya Soutmun <darksolar@gmail.com>
 */

#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>


#include <linux/ioctl.h>

#include "nbd.h"
#include "nid_log.h"
#include "nid.h"
#include "hs_if.h"

int check_conn(char* devname, int do_print) {
	char buf[256];
	char* p;
	int fd;
	int len;

	if( (p=strrchr(devname, '/')) ) {
		devname=p+1;
	}
	if((p=strchr(devname, 'p'))) {
		/* We can't do checks on partitions. */
		*p='\0';
	}
	snprintf(buf, 256, "/sys/block/%s/pid", devname);
	if((fd=open(buf, O_RDONLY))<0) {
		if(errno==ENOENT) {
			return 1;
		} else {
			return 2;
		}
	}
	len=read(fd, buf, 256);
	buf[len-1]='\0';
	if(do_print) printf("%s\n", buf);
	return 0;
}

int
do_nbd_control(char* cmd, char* devname)
{
	nid_log_debug("do_nbd_control: %s:%s", cmd, devname);
#if 0
	int nbd;
	struct nbd_control *nbd_job;
	struct nbd_stat_info *stat_info;

	nbd_job = calloc(1, sizeof(*nbd_job) + 1024);

	if (!strcmp(cmd, "get_stat")) {
		nbd_job->cmd = NBD_CNTL_GETSTAT;
		nbd_job->len = 0;
	} else {
		nid_log_error("invalid control cmd(%s)\n", cmd);
		exit(EXIT_FAILURE);
	}

	nbd = open(devname, O_RDWR);
	if (nbd < 0) {
		nid_log_error("Cannot open NBD device %s\n", devname);
		exit(1);
	}

	if (ioctl(nbd, NBD_CONTROL, nbd_job) < 0) {
		nid_log_error("ioctl NBD_CONTROL failed"); 
		exit(1);
	}

	stat_info = (struct nbd_stat_info *)nbd_job->data;
	nid_log_debug("do_nbd_control got resp. c_waiting_len:%d, c_ack_len:%d, c_nr_req:%d,"
		"s_recv_len:%d, s_reply_len:%d, s_busy_workers:%d, s_free_workers:%d\n",
		stat_info->c_waiting_len, stat_info->c_ack_len, stat_info->c_nr_req,
		stat_info->s_recv_len, stat_info->s_reply_len,
		stat_info->s_busy_workers, stat_info->s_free_workers); 
	printf("c_waiting_len:%d, c_ack_len:%d, c_nr_req:%d,"
		"s_recv_len:%d, s_reply_len:%d, s_busy_workers:%d, s_free_workers:%d\n",
		stat_info->c_waiting_len, stat_info->c_ack_len, stat_info->c_nr_req,
		stat_info->s_recv_len, stat_info->s_reply_len,
		stat_info->s_busy_workers, stat_info->s_free_workers); 
#endif
	return 0;
}

static int
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
	if (nread <= 0) {
		nid_log_error("get_one_message: read error, errno:%d", errno);
		return -1;
	}

	nid_log_debug("get_one_message: got cmd:%d, len:%d, msg:{%s}", cmd, msg_len, p); 
	return 0;
}

static int 
make_request_connection(char *ipstr, u_short port)
{
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);
	char chan_type = CC_CTYPE_REG;

	nid_log_info("client make_request_connection start (%s, %d)", ipstr, port); 
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("cannot open a socket");
		return -1;
	}
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
		nid_log_error("cannot do TCP_NODELAY, errno:%d", errno);
	}

	//nid_log_info("client connecting to (%d, %d)", saddr.sin_port, port);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		nid_log_error("cannot connect to the server (%s:%d), errno:%d",
			ipstr, port, errno);
		close(sfd);
		return -1;
	}

	write(sfd, &chan_type, 1);
	return sfd;
}

static int
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
		nid_log_info("client make_data_connection failed to bind (port:%d)", port);
		return -1;
	}

	listen(sfd, 5);
	return sfd;
}

static int 
channel_establish(struct hs_interface *hs_p, char *uuid, char *server_ipstr, u_short port, int *data_sfd, uint64_t *size)
{
	int rsfd, dsfd, a_sfd, nwrite, len;
	char buf[1024];
	struct sockaddr_in caddr;
	socklen_t clen;
	uint32_t size_h, size_l;
	uint64_t exp_size;
	int rc, opt = 1;

	nid_log_info("nw client start");

	rsfd = make_request_connection(server_ipstr, port);
	if (rsfd < 0) 
		return -1;
	rc = get_one_message(rsfd, buf);
	if (rc < 0)
		return -1;

	len = hs_p->h_op->h_exp_generator(hs_p, uuid, buf);
	nwrite = write(rsfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		close(rsfd);
		return -1;
	}

	/* setup size */
	assert(read(rsfd, &size_h, 4) == 4);
	assert(read(rsfd, &size_l, 4) == 4);
	nid_log_debug("channel_establish:  size_h:%u, size_l:%u",  size_h, size_l);
	size_l = ntohl(size_l);
	size_h = ntohl(size_h);
	exp_size = size_h;
	exp_size <<= 32;
	exp_size += size_l;
	nid_log_debug("channel_establish: size:%lu, size_h:%u, size_l:%u", exp_size, size_h, size_l);
	*size = exp_size;

	do {
		port++;
		a_sfd = make_data_connection(port);
	} while (a_sfd < 0);

	clen = sizeof(caddr);
	getsockname(rsfd, (struct sockaddr *)&caddr, &clen);
	nid_log_debug("channel_establish: my ipstr:%s, clen:%d, errno:%d", inet_ntoa(caddr.sin_addr), clen, errno);	

	len = hs_p->h_op->h_port_generator(hs_p, inet_ntoa(caddr.sin_addr), port, buf);
	nwrite = write(rsfd, buf, len);
	if (nwrite != len) {
		nid_log_error("client: write(%s) incorrectly (len:%d, nwrite:%d",
			buf, len, nwrite);
		exit(1);
	}

	clen = sizeof(caddr);
	dsfd = accept(a_sfd, (struct sockaddr *)&caddr, &clen);
	nid_log_info("client: got connect from ths server, fd:%d", dsfd);
	*data_sfd = dsfd;
	close(a_sfd);

	if (setsockopt(dsfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		nid_log_error("cannot do TCP_NODELAY, errno:%d", errno);
	}
	return rsfd;
}

void setsizes(int nbd, uint64_t size64, int blocksize, uint32_t flags) {
	unsigned long size;
	int read_only = 0;

	if (size64>>12 > (uint64_t)~0UL)
		nid_log_error("Device too large.\n");
	else {
		if (ioctl(nbd, NBD_SET_BLKSIZE, 4096UL) < 0)
			nid_log_error("Ioctl/1.1a failed: %m\n");
		size = (unsigned long)(size64>>12);
		if (ioctl(nbd, NBD_SET_SIZE_BLOCKS, size) < 0)
			nid_log_error("Ioctl/1.1b failed:\n");
		if (ioctl(nbd, NBD_SET_BLKSIZE, (unsigned long)blocksize) < 0)
			nid_log_error("Ioctl/1.1c failed: \n");
		fprintf(stderr, "bs=%d, sz=%llu bytes\n", blocksize, 4096ULL*size);
	}

	ioctl(nbd, NBD_CLEAR_SOCK);

	/* ignore error as kernel may not support */
	ioctl(nbd, NBD_SET_FLAGS, (unsigned long) flags);

	if (ioctl(nbd, BLKROSET, (unsigned long) &read_only) < 0)
		nid_log_error("Unable to set read-only attribute for device");
}

void set_timeout(int nbd, int timeout) {
	if (timeout) {
		if (ioctl(nbd, NBD_SET_TIMEOUT, (unsigned long)timeout) < 0)
			nid_log_error("Ioctl NBD_SET_TIMEOUT failed\n");
		fprintf(stderr, "timeout=%d\n", timeout);
	}
}

void finish_sock(int sock, int data_sock, int nbd) {
	if (ioctl(nbd, NBD_SET_SOCK, sock) < 0)
		nid_log_error("Ioctl NBD_SET_SOCK failed\n");
	if (ioctl(nbd, NBD_SET_DATA_SOCK, data_sock) < 0)
		nid_log_error("Ioctl NBD_SET_DATA_SOCK failed\n");
}


void usage(char* errmsg, ...) {
	if(errmsg) {
		char tmp[256];
		va_list ap;
		va_start(ap, errmsg);
		snprintf(tmp, 256, "ERROR: %s\n\n", errmsg);
		vfprintf(stderr, tmp, ap);
		va_end(ap);
	}
	fprintf(stderr, "Usage: nbd-client host port nbd_device [-block-size|-b block size] [-timeout|-t timeout] [-sdp|-S] [-persist|-p]|[-persist-hang|-P]\n");
	fprintf(stderr, "Or   : nbd-client -name|-N name host [port] nbd_device [-block-size|-b block size] [-timeout|-t timeout] [-sdp|-S] [-persist|-p]|[-persist-hang|-P]\n");
	fprintf(stderr, "Or   : nbd-client -d nbd_device\n");
	fprintf(stderr, "Or   : nbd-client -c nbd_device\n");
	fprintf(stderr, "Or   : nbd-client -h|--help\n");
	fprintf(stderr, "Or   : nbd-client -l|--list host\n");
	fprintf(stderr, "Default value for blocksize is 1024 (recommended for ethernet)\n");
	fprintf(stderr, "Allowed values for blocksize are 512,1024,2048,4096\n"); /* will be checked in kernel :) */
	fprintf(stderr, "Note, that kernel 2.4.2 and older ones do not work correctly with\n");
	fprintf(stderr, "blocksizes other than 1024 without patches\n");
	fprintf(stderr, "Default value for port with -N is 10809. Note that port must always be numeric\n");
}

void disconnect(char* device) {
	int nbd;

	nid_log_info("Enter disconnect %s\n", device);

	nbd = open(device, O_RDWR);
	if (nbd < 0) {
		nid_log_error("Cannot open NBD device %s\n", device);
		exit(1);
	}

	nid_log_info("disconnect: trying to NBD_DISCONNECT\n");
	if (ioctl(nbd, NBD_DISCONNECT)<0) {
		nid_log_info("disconnect: NBD_DISCONNECT retrun error");
		//ioctl(nbd, NBD_DO_IT);	// try to cleanup
		//exit(1);
	}

	nid_log_info("disconnect: trying to NBD_CLEAR_SOCK\n");
	if (ioctl(nbd, NBD_CLEAR_SOCK)<0) {
		nid_log_error("ioctl NBD_CLEAR_SOCK failed");
		exit(1);
	}

	nid_log_info("disconnect success!\n");
}

void persistent_post_do(char *cmd)
{
	FILE *fp;
	char buff[1024];
	int pid;

	if (!cmd) {
		nid_log_error("persistent_post_do got NULL cmd\n");
		return;
	}

	if ((pid = fork())) {
		if (pid < 0) {
			nid_log_error("cannot fork, exit ...\n");
			exit(1);
		}	

		/* in parent */
		return;
	}

	/* in child */
	
	fp = popen(cmd, "r");  	
	while (fgets(buff, sizeof(buff), fp) != NULL) {
		nid_log_debug("%s", buff);
	}
	pclose(fp);
	exit(0);
}

int main(int argc, char *argv[]) {
	u_short port = SERVER_PORT;
	int nbd;
	int blocksize=1024;
	char *server_ipstr=NULL;
	char *nbddev=NULL;
	int cont=0;
	//int hang=0;
	int timeout=0;
	char *control_cmd = NULL;
	uint64_t size64;
	uint32_t flags = 0;
	int c;
	int nonspecial=0;
	char* uuid=NULL;
	sigset_t block, old;
	struct sigaction sa;
	int persistent_postrun = 0;
	char *persistent_postcmd = NULL;
	struct option long_options[] = {
		{ "block-size", required_argument, NULL, 'b' },
		{ "check", required_argument, NULL, 'c' },
		{ "control", required_argument, NULL, 'C' },
		{ "disconnect", required_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{ "uuid", required_argument, NULL, 'N' },
		{ "persist", no_argument, NULL, 'p' },
		//{ "persist-hang", no_argument, NULL, 'P' },
		{ "persist-postrun", required_argument, NULL, 'x' },
		{ "sdp", no_argument, NULL, 'S' },
		{ "timeout", required_argument, NULL, 't' },
		{ 0, 0, 0, 0 }, 
	};
	struct hs_setup hs_setup;
	struct hs_interface *hs_p;
	int req_sfd[NID_NUM_CHANNEL], data_sfd[NID_NUM_CHANNEL];
	int chan_index;

	//logging();
	nid_log_open();
	nid_log_info("start with ver (%s)\n", NBD_VERSION);


	while((c=getopt_long_only(argc, argv, "-b:c:C:d:hN:pPx:St:", long_options, NULL))>=0) {
		switch(c) {
		case 1:
			printf("in case 1: optarg:%s\n", optarg);
			// non-option argument
			if(strchr(optarg, '=')) {
				// old-style 'bs=' or 'timeout='
				// argument
				fprintf(stderr, "WARNING: old-style command-line argument encountered. This is deprecated.\n");
				if(!strncmp(optarg, "bs=", 3)) {
					optarg+=3;
					goto blocksize;
				}
				if(!strncmp(optarg, "timeout=", 8)) {
					optarg+=8;
					goto timeout;
				}
				usage("unknown option %s encountered", optarg);
				exit(EXIT_FAILURE);
			}
			switch(nonspecial++) {
				case 0:
					if (control_cmd) {
						nbddev = optarg;
					} else {
						// host
						server_ipstr=optarg;
					}
					break;
				case 1:
					// port
					if(!strtol(optarg, NULL, 0)) {
						// not parseable as a number, assume it's the device and we have a name
						nbddev = optarg;
						nonspecial++;
					} else {
						port = atoi(optarg);
					}
					break;
				case 2:
					// device
					nbddev = optarg;
					break;
				default:
					usage("too many non-option arguments specified");
					exit(EXIT_FAILURE);
			}
			break;
		case 'b':
		      blocksize:
			blocksize=(int)strtol(optarg, NULL, 0);
			break;
		case 'c':
			return check_conn(optarg, 1);
		case 'C':
			control_cmd = optarg;
			printf("control_cmd:%s\n", control_cmd);
			break; 
		case 'd':
			disconnect(optarg);
			exit(EXIT_SUCCESS);
		case 'h':
			usage(NULL);
			exit(EXIT_SUCCESS);
		case 'N':
			uuid=optarg;
			break;
		case 'p':
			cont=1;
			break;
		//case 'P':
		//	cont=1;
		//	hang = 1;
		//	break;
		case 'x':
			persistent_postrun = 1;
			persistent_postcmd = optarg;
			break;
		case 't':
		      timeout:
			timeout=strtol(optarg, NULL, 0);
			break;
		default:
			fprintf(stderr, "E: option eaten by 42 mice\n");
			exit(EXIT_FAILURE);
		}
	}

	if (control_cmd) {
		printf("this is a control cmd:%s, nbd:%s\n", control_cmd, nbddev);
		if (!nbddev) {
			usage("not enough information specified");
			exit(EXIT_FAILURE);
		}
		return do_nbd_control(control_cmd, nbddev);
	}

	if((!port && !uuid) || !server_ipstr || !nbddev) {
		usage("not enough information specified");
		exit(EXIT_FAILURE);
	}

	hs_p = calloc(1, sizeof(*hs_p));
	hs_initialization(hs_p, &hs_setup);
	for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
		req_sfd[chan_index] = -1;
		data_sfd[chan_index] = -1;
	}

	for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
		req_sfd[chan_index] = channel_establish(hs_p, uuid, server_ipstr, port, &data_sfd[chan_index], &size64);
		if (req_sfd[chan_index] < 0) {
			nid_log_error("cnanot establish channel to the server, %s:%d", server_ipstr, port);
			exit(1);
		}
	}
	// ATLAS-COMMENT
	if (cont) {
		flags |= NBD_FLAG_PERSISTENT;
		//if (hang)
		//	flags |= NBD_FLAG_PERSISTENT_HANG;
	}

	nbd = open(nbddev, O_RDWR);
	if (nbd < 0) {
		nid_log_error("Cannot open NBD device :%s, errno:%d\n", nbddev, errno);
		exit(1);
	}

	setsizes(nbd, size64, blocksize, flags);
	// set_timeout(nbd, timeout);
	for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
		finish_sock(req_sfd[chan_index], data_sfd[chan_index], nbd);
	}

	/* Go daemon */
	
	if (daemon(0,0) < 0)
		nid_log_error("Cannot detach from terminal");

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	do {

		sigfillset(&block);
		sigdelset(&block, SIGKILL);
		sigdelset(&block, SIGTERM);
		sigdelset(&block, SIGPIPE);
		sigprocmask(SIG_SETMASK, &block, &old);

		int rc = 0;
		if ((rc=ioctl(nbd, NBD_DO_IT)) < 0) {
		        int error = errno;
			nid_log_warning("nbd,%d: Kernel call returned with errno:%d, rc:%d", getpid(), error, rc);
			if(error==EBADR) {
				/* The user probably did 'nbd-client -d' on us.
				 * quit */
				cont=0;
			} else {
				if(cont) {
					int io_counter;
					int do_postrun = 0;
					int need_stop = 0;

					for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
						close(req_sfd[chan_index]); // close(nbd); ATLAS-COMMENT:
						close(data_sfd[chan_index]);
					}
					io_counter = (timeout > 0) ? timeout : -1;
					for (;;) {
						nid_log_debug("timeout:%d, retry channel_establish...\n", timeout);
						for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
							req_sfd[chan_index] = channel_establish(hs_p, uuid, server_ipstr, port, &data_sfd[chan_index], &size64);
							if (req_sfd[chan_index] < 0)
								break;
						}
						if (chan_index == NID_NUM_CHANNEL) {
							break;
						}

						sleep (1);
						if (io_counter > 0) {
							io_counter--;
						} else if (io_counter == 0) {
							nid_log_debug("after timeout(%d), clear the queue\n", timeout);
							if (ioctl(nbd, NBD_CLEAR_QUE) < 0) {
								nid_log_error("ioctl NBD_CLEAR_QUE failed");
								exit(1);
							}
							do_postrun = 1;
							io_counter = (timeout > 0) ? timeout : -1;
						}
					}
					if (need_stop) {
						ioctl(nbd, NBD_DISCONNECT);
						ioctl(nbd, NBD_DO_IT); // to cleanup leftover, should return immediately
						goto done;
					}
					for (chan_index = 0; chan_index < NID_NUM_CHANNEL; chan_index++) {
						finish_sock(req_sfd[chan_index], data_sfd[chan_index], nbd);
					}
					if (persistent_postrun && do_postrun) {
						nid_log_debug("persistent_postcmd is (%s)\n", persistent_postcmd);
						persistent_post_do(persistent_postcmd);
					}
				}
			}
		} else {
			/* We're on 2.4. It's not clearly defined what exactly
			 * happened at this point. Probably best to quit, now
			 */
			nid_log_warning("Kernel call returned.\n");
			cont=0;
		}
	} while(cont);
done:
	nid_log_info("Done\n");
	return 0;
}
