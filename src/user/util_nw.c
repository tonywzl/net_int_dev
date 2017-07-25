/*
 * util_nw.c
 */

#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>

#include "util_nw.h"
#include "nid_log.h"
#include "nid.h"
#include "version.h"

#define h_addr h_addr_list[0]

#define NW_SELECT_TIMEOUT	1
int
util_nw_read_stop(int fd, char *p, uint32_t to_read, int32_t timeout, uint8_t *stop)
{
	fd_set readset;
	struct timeval tval;
	int rc, do_timeout = 0;

	if (timeout)
		do_timeout = 1;
	while (1) {
		FD_ZERO(&readset);
		tval.tv_sec = NW_SELECT_TIMEOUT;
		tval.tv_usec = 0;
		FD_SET(fd, &readset);
		rc = select(fd + 1, &readset, NULL, NULL, &tval);
		if (rc < 0 || *stop) {
			nid_log_error("read rc:[%d], error:[%d]", rc, errno);
			rc = (rc < 0) ? -1 : -2;
			goto out;
		}
		if (FD_ISSET(fd, &readset)) {
			break;
		}
		if (do_timeout && (timeout -= NW_SELECT_TIMEOUT) <= 0) {
			rc = -3;
			goto out;
		}
	}
	rc = read(fd, p, to_read);
	if (rc < 0) {
		nid_log_error("read error:[%d]", errno);
	}
out:
	return rc;
}

int
util_nw_read_n(int fd, void *buf, int n)
{
	char *log_header = "util_nw_read_n";
	char *p = buf;
	int nread , left = n;

	while (left > 0) {
		nread = read(fd, p, left);
		if (nread < 0) {
			nid_log_error("%s: read failed with rc: %d and errno: %d", log_header, nread, errno);
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nread == 0)
			return -1;	
		left -= nread;
		p += nread;
	}
	return n;
}

int
util_nw_read_n_timeout(int sfd, char *buf, uint32_t to_read, int32_t timeout)
{
	char *log_header = "util_nw_read_n_timeout";
	fd_set readset;
	struct timeval tval;
	uint32_t count = 0;
	int rc = 0;

	if (buf == NULL) {
		nid_log_error("%s: buf is NULL", log_header);
		return -1;
	}

	tval.tv_sec = timeout;
	tval.tv_usec = 0;
	while (count < to_read) {
		FD_ZERO(&readset);
		FD_SET(sfd, &readset);

		rc = select(sfd + 1, &readset, NULL, NULL, &tval);
		if (rc < 0) {
			nid_log_error("%s: select() failed. rc:%d, errno:%d",
				log_header, rc, errno);
			return -2;
		} else if (rc == 0) {
			nid_log_warning("%s: select() timeout: %d, rc:%d, errno:%d",
				log_header, timeout, rc, errno);
			break;
		} else {
			if (!FD_ISSET(sfd, &readset))
				continue;

			rc = read(sfd, buf, to_read - count);
			if (rc < 0) {
				if (errno == EWOULDBLOCK) {
					nid_log_debug("%s: read() blocked, rc:%d, errno:%d",
						log_header, rc, errno);
				} else {
					nid_log_error("%s: read() error, rc:%d, errno:%d, %p",
						log_header, rc, errno, buf);
					return -3;
				}
			} else if (rc == 0) {
				nid_log_debug("%s: read() return 0!", log_header);
				return -4;
			} else {
				buf += rc;
				count += rc;
				nid_log_debug("%s: read() count:%d, to_read:%d, %p",
					log_header, count, to_read, buf);
			}
		}
	}

	return count;
}

int
util_nw_write_timeout_stop(int sfd, char *buf, uint32_t length, int32_t timeout, uint8_t *stop)
{
	fd_set fs;
	struct timeval tv;
	int nwrite;
	int rc = 0;
	time_t tb, te;
	tb = time(NULL);
	int l, t;

	l = length;
	t = timeout;
	while (length > 0) {
		FD_ZERO(&fs);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
		FD_SET(sfd, &fs);
		if (select(sfd + 1, NULL, &fs, NULL, &tv) < 0) {
			rc = -1;
			goto out;
		}
		if (*stop) {
			rc = -2;
			goto out;
		}
		if (FD_ISSET(sfd, &fs)) {
			nwrite = write(sfd, buf, length);
			if (nwrite < 0) {
				if (errno == EWOULDBLOCK) {
					nid_log_debug("util_nw_write_timeout_stop: write blocked, nwrite:%d, errno:%d",
						nwrite, errno);
				} else {
					nid_log_error("util_nw_write_timeout_stop: write error, nwrite:%d, errno:%d",
						nwrite, errno);
					rc = -3;
					goto out;
				}
			} else {
				nid_log_debug("util_nw_write_timeout_stop: write len:%d, nwrite:%d, errno:%d",
					length, nwrite, errno);
				length -= nwrite;
				buf += nwrite;
				if (length == 0)
					break;
			}
		}
		te = time(NULL);
		if (tb + timeout < te)
			break;
		timeout -= te - tb;
		tb = te;
	}

	if (length > 0)
		rc = -4;
out:
	if (rc)
		nid_log_error("util_nw_write_timeout_stop length:%d, timeout:%d, rc:%d",
			l, t, rc);
	return rc;
}

int
util_nw_write_n(int fd, void *buf, int n)
{
	char *p = buf;
	int nwrite , left = n;

	while (left > 0) {
		nwrite = write(fd, p, left);
		if (nwrite < 0) {
			if (errno == EINTR)
				continue;
			else
				return -1;
		} else if (nwrite == 0)
			return -1;	
		left -= nwrite;
		p += nwrite;
	}
	return n;
}

int
util_nw_write_two_byte(int fd, uint16_t ctype)
{
	char version_buf[NID_SIZE_VERSION], response_buf[NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER];
	char *part, version[] = NID_VERSION, magic[] = NID_CONN_MAGIC;
	char new_message_flag = 0x80;
	uint16_t proto_ver = NID_PROTOCOL_VER;
	int n = 0, rn = 0, mid, i;

	n += util_nw_write_n(fd, &new_message_flag, 1);
	n += util_nw_write_n(fd, magic, NID_SIZE_CONN_MAGIC);
	n += util_nw_write_n(fd, &proto_ver, NID_SIZE_PROTOCOL_VER);

	rn = util_nw_read_n(fd, response_buf, NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER);
	if (rn == NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER && !memcmp(magic, response_buf, NID_SIZE_CONN_MAGIC)) {
		proto_ver = *(uint16_t *)&response_buf[NID_SIZE_CONN_MAGIC];
		if (proto_ver < NID_PROTOCOL_RANGE_LOW || proto_ver > NID_PROTOCOL_RANGE_HIGH){
			nid_log_warning("server protocol version (%d) is not compatible with manager protocol version (%d)", proto_ver, NID_PROTOCOL_VER);
			return -1;
		}
	} else {
		return -1;
	}

	n += util_nw_write_n(fd, &ctype, 2);

	memset(version_buf, 0, NID_SIZE_VERSION);
	part = strtok(version, ".");
	if (part != NULL)
		version_buf[0] = atoi(part);

	part = strtok(NULL, ".");
	if (part != NULL)
		mid = atoi(part);

	part = strtok(NULL, ".");
	if (part != NULL) {
		i = strlen(part);
		for (;i > 0; i--)
			mid *= 10;
		mid += atoi(part);
		version_buf[1] = mid;
	}

	part = strtok(NULL, ".");
	if (part != NULL)
		*(uint16_t *)&version_buf[2] = atoi(part);

	n += util_nw_write_n(fd, version_buf, NID_SIZE_VERSION);
	return n;
}

int
util_nw_write_timeout(int sfd, char *buf, int to_write, int timeout)
{
	char *log_header = "util_nw_write_timeout";
	fd_set fs;
	struct timeval tv;
	int count = 0;
	int rc = 0;

	if (buf == NULL) {
		nid_log_error("%s: buf is NULL", log_header);
		return -1;
	}

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	while (count < to_write) {
		FD_ZERO(&fs);
		FD_SET(sfd, &fs);

		rc = select(sfd + 1, NULL, &fs, NULL, &tv);
		if ( rc < 0) {
			nid_log_error("%s: select() failed. rc:%d, errno:%d",
				log_header, rc, errno);
			return -2;
		} else if (rc == 0) {
			nid_log_warning("%s: select() timeout: %d, rc:%d, errno:%d",
				log_header, timeout, rc, errno);
			break;
		} else {
			if (!FD_ISSET(sfd, &fs))
				continue;

			rc = write(sfd, buf, to_write - count);
			if (rc < 0) {
				if (errno == EWOULDBLOCK) {
					nid_log_debug("%s: write() blocked, rc:%d, errno:%d",
						log_header, rc, errno);
				} else {
					nid_log_error("%s: write() error, rc:%d, errno:%d, %p",
						log_header, rc, errno, buf);
					return -3;
				}
			} else if (rc == 0) {
				nid_log_debug("%s: write() return 0!", log_header);
				return -4;
			} else {
				buf += rc;
				count += rc;
				nid_log_debug("%s: write() count:%d, to_write:%d, errno:%d, %p",
					log_header, count, to_write, errno, buf);
			}
		}
	}

	return count;
}

int
util_nw_write_timeout_rc(int sfd, char *buf, int to_write, int timeout, int *rc_code)
{
	char *log_header = "util_nw_write_timeout";
	fd_set fs;
	struct timeval tv;
	int count = 0;
	int rc = 0;

	if (buf == NULL) {
		nid_log_error("%s: buf is NULL", log_header);
		return -1;
	}

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	while (count < to_write) {
		FD_ZERO(&fs);
		FD_SET(sfd, &fs);

		rc = select(sfd + 1, NULL, &fs, NULL, &tv);
		if ( rc < 0) {
			nid_log_error("%s: select() failed. rc:%d, errno:%d",
				log_header, rc, errno);
			rc = -2;
			break;
		} else if (rc == 0) {
			nid_log_warning("%s: select() timeout: %d, rc:%d, errno:%d",
				log_header, timeout, rc, errno);
			break;
		} else {
			if (!FD_ISSET(sfd, &fs))
				continue;

			rc = write(sfd, buf, to_write - count);
			if (rc < 0) {
				if (errno == EWOULDBLOCK) {
					nid_log_debug("%s: write() blocked, rc:%d, errno:%d",
						log_header, rc, errno);
				} else {
					nid_log_error("%s: write() error, rc:%d, errno:%d, %p",
						log_header, rc, errno, buf);
					break;
				}
			} else if (rc == 0) {
				nid_log_debug("%s: write() return 0!", log_header);
				rc = -4;
				break;
			} else {
				buf += rc;
				count += rc;
				nid_log_debug("%s: write() count:%d, to_write:%d, errno:%d, %p",
					log_header, count, to_write, errno, buf);
			}
		}
	}

	*rc_code = rc;
	return count;
}

int
util_nw_write_two_byte_timeout(int fd, uint16_t ctype, int timeout)
{
	char version_buf[NID_SIZE_VERSION], response_buf[NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER];
	char *part, version[] = NID_VERSION, magic[] = NID_CONN_MAGIC;;
	char new_message_flag = 0x80;
	uint16_t proto_ver = NID_PROTOCOL_VER;
	int n = 0, rn = 0, mid, i;

	n += util_nw_write_timeout(fd, &new_message_flag, 1, timeout);
	n += util_nw_write_timeout(fd, magic, NID_SIZE_CONN_MAGIC, timeout);
	n += util_nw_write_timeout(fd, (char *)&proto_ver, NID_SIZE_PROTOCOL_VER, timeout);

	rn = util_nw_read_n_timeout(fd, response_buf, NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER, timeout);
	if (rn == NID_SIZE_CONN_MAGIC + NID_SIZE_PROTOCOL_VER && !memcmp(magic, response_buf, NID_SIZE_CONN_MAGIC)) {
		proto_ver = *(uint16_t *)&response_buf[NID_SIZE_CONN_MAGIC];
		if (proto_ver < NID_PROTOCOL_RANGE_LOW || proto_ver > NID_PROTOCOL_RANGE_HIGH){
			nid_log_warning("server protocol version (%d) is not compatible with manager protocol version (%d)", proto_ver, NID_PROTOCOL_VER);
			return -1;
		}
	} else {
		return -1;
	}

	n += util_nw_write_timeout(fd, (char *)&ctype, 2, timeout);

	memset(version_buf, 0, NID_SIZE_VERSION);
	part = strtok(version, ".");
	if (part != NULL)
		version_buf[0] = atoi(part);

	part = strtok(NULL, ".");
	if (part != NULL)
		mid = atoi(part);

	part = strtok(NULL, ".");
	if (part != NULL) {
		i = strlen(part);
		for (;i > 0; i--)
			mid *= 10;
		mid += atoi(part);
		version_buf[1] = mid;
	}

	part = strtok(NULL, ".");
	if (part != NULL)
		*(uint16_t *)&version_buf[2] = atoi(part);

	n += util_nw_write_timeout(fd, version_buf, NID_SIZE_VERSION, timeout);
	return n;
}

char *
util_nw_getip(char *dn_or_ip)
{
struct hostent *host;
	if (dn_or_ip == NULL)
		return NULL;
	if (strcmp(dn_or_ip, "localhost") == 0) {
		return "127.0.0.1";
	} else {
		host = gethostbyname(dn_or_ip);
		if (host == NULL)
			return dn_or_ip;
		dn_or_ip = (char *)inet_ntoa(*(struct in_addr *)(host->h_addr));
	}
	return dn_or_ip;
}


#if 1
int
util_nw_make_connection(char *ipstr, u_short port)
{
	char *log_header = "util_nw_make_connection";
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);
	int flags, ret, error = 0;
	int nsec = 1;
	fd_set rset, wset;
	struct timeval	tval;
	struct timeval timeout;

	nid_log_debug("%s: start (%s, %d)", log_header, ipstr, port);
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("%s: cannot open a socket (%s, %d)", log_header, ipstr, port);
		return -1;
	}
	timeout.tv_sec = SOCKET_WRITE_TIME_OUT;
	timeout.tv_usec = 0;
	if (setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
		nid_log_error("set SO_SNDTIMEO failed, errno:%d", errno);
		close(sfd);
		return -1;
	}

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
		nid_log_error("%s: cannot do TCP_NODELAY, errno:%d", log_header, errno);
		close(sfd);
		return -1;
	}
	flags = fcntl(sfd, F_GETFL, 0);
	ret = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
	if(ret < 0) {
		nid_log_error("%s: cannot set socket to NONBLOCK mode, errno:%d", log_header, errno);
		close(sfd);
		return -1;
	}

	ret = connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret == 0) {
		goto done;
	}
	if (ret < 0) {
		// must store errno before compare it with EINPROGRESS, because log function
		// will change its value
		error = errno;
		nid_log_warning("%s: connect return %d  (errno:%d)", log_header, ret, error);
		if (error != EINPROGRESS) {
			close(sfd);
			return -1;
		}
		// restore error value
		error = 0;
	}

	nid_log_warning("%s: got error EINPROGRESS", log_header);
	FD_ZERO(&rset);
	FD_SET(sfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ((ret = select(sfd + 1, &rset, &wset, NULL, nsec ? &tval : NULL)) <= 0) {
		nid_log_error("%s: cannot connect to the server (%s:%d), errno:%d",
				log_header, ipstr, port, errno);
		close(sfd);		/* timeout */
		errno = ETIMEDOUT;
		return -1;
	}

	if (FD_ISSET(sfd, &rset) || FD_ISSET(sfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			nid_log_error("%s: getsockopt return error: %d.", log_header, errno);
			close(sfd);
			return -1;			/* Solaris pending error */
		}
	} else {
		nid_log_error("%s: select error: sockfd not set", log_header);
		close(sfd);
		return -1;
	}

done:
	fcntl(sfd, F_SETFL, flags);	/* restore file status flags */
	if (error) {
		nid_log_error("%s: error:%d", log_header, error);
		close(sfd);		/* just in case */
		errno = error;
		return -1;
	}

	return sfd;
}
#else
int
util_nw_make_connection(char *ipstr, u_short port)
{
	char *log_header = "util_nw_make_connection";
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);
	int sfd;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);

	memset(&saddr, 0, sizeof(saddr));
	ipstr = GetIp(ipstr);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("%s: cannot open socket, errno:%d", log_header, errno);
		return -1;
	}

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
		nid_log_error("%s: cannot do TCP_NODELAY, errno:%d", log_header, errno);
	}

	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("%s: cannot connect to the client (%s:%d), errno:%d",
				log_header, ipstr, port, errno);
		close(sfd);
		return -2;
	}
	return sfd;
}
#endif

int
util_nw_make_connection2(char *ipstr, u_short port)
{
	char *log_header = "util_nw_make_connection2";
	struct sockaddr_in saddr;
	int sfd;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("%s: cannot open socket, errno:%d", log_header, errno);
		return -1;
	}

	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("%s: cannot connect to the client (%s:%d), errno:%d",
			log_header, ipstr, port, errno);
		close(sfd);
		return -2;
	}
	return sfd;
}

int
util_nw_connection(char *ipstr, u_short port, int sfd)
{
	char *log_header = "util_nw_make_connection";
	struct sockaddr_in saddr;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("%s: cannot connect to the client (%s:%d), errno:%d",
			log_header, ipstr, port, errno);
		return -1;
	}
	return 0;
}

int
util_nw_open()
{
	char *log_header = "util_nw_open";
	int sfd;

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		nid_log_error("%s: cannot open socket, errno:%d", log_header, errno);
	}
	return sfd;
}

int
util_nw_bind(int sfd, u_short port)
{
	char *log_header = "util_nw_bind";
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);
	if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("%s: cannot bind socket, errno:%d", log_header, errno);
		close(sfd);
		return -1;
	}

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (listen(sfd, 5) < 0) {
		close(sfd);
		return -1;
	}
	return sfd;
}

