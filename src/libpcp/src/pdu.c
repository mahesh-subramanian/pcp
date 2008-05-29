/*
 * Copyright (c) 1995-2005 Silicon Graphics, Inc.  All Rights Reserved.
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
 * 
 * Contact information: Silicon Graphics, Inc., 1500 Crittenden Lane,
 * Mountain View, CA 94043, USA, or: http://www.sgi.com
 */

#include <signal.h>
#include "pmapi.h"
#include "impl.h"

#ifdef MALLOC_AUDIT
/*
 * PDU buffer allocations here are permanent, and do not cause any
 * memory leaks
 */
#include "no-malloc-audit.h"
#endif

extern int	__pmLastUsedFd;

int		pmDebug = 0;		/* the real McCoy */

#define ABUF_SIZE	1024	/* read buffer for reading ascii PDUs */

/*
 * Performance Instrumentation
 *  ... counts binary PDUs received and sent by low 4 bits of PDU type
 */

static unsigned int	inctrs[PDU_MAX+1] = { 0 };
static unsigned int	outctrs[PDU_MAX+1] = { 0 };
unsigned int	*__pmPDUCntIn = inctrs;
unsigned int	*__pmPDUCntOut = outctrs;

/*
 * unit of space allocation for PDU buffer ... actually defined in pdubuf.c,
 * it should have the same value here.
 */
#define PDU_CHUNK	1024

typedef struct {		/* this is like stdio for reading ascii PDUs */
    int		fd;
    int		timeout;
    int		len;
    int		next;
    char	abuf[ABUF_SIZE];
} ascii_ctl;

static int		mypid = -1;
static ascii_ctl	ac;

static int              ceiling = PDU_CHUNK * 64;

int
__pmMoreInput(int fd)
{
    return 0;
}

void
__pmNoMoreInput(int fd)
{
}

static struct timeval	def_wait = { 10, 0 };

const struct timeval *
__pmDefaultRequestTimeout(void)
{
    static int		done_default = 0;

    if (!done_default) {
	double	def_timeout;
	char	*timeout_str;
	char	*end_ptr;
	if ((timeout_str = getenv("PMCD_REQUEST_TIMEOUT")) != NULL) {
	    def_timeout = strtod(timeout_str, &end_ptr);
	    if (*end_ptr != '\0' || def_timeout < 0.0) {
		__pmNotifyErr(LOG_WARNING,
			      "ignored bad PMCD_REQUEST_TIMEOUT = '%s'\n",
			      timeout_str);
	    }
	    else {
		def_wait.tv_sec = (int)def_timeout;		/* truncate -> secs */
		if (def_timeout > (double)def_wait.tv_sec)
		    def_wait.tv_usec = (long)((def_timeout - (double)def_wait.tv_sec) * 1000000);
		else
		    def_wait.tv_usec = 0;
	    }
	}
	done_default = 1;
    }
    return (&def_wait);
}

static int
pduread(int fd, char *buf, int len, int mode, int timeout)
{
    /*
     * handle short reads that may spilt a PDU ...
     */
    int			status = 0;
    int			have = 0;
    fd_set		onefd;

    while (len) {
	if (timeout == GETPDU_ASYNC) {
	    /*
	     * no grabbing more than you need ... read header to get
	     * length and then read body.
	     * This assumes you are either willing to block, or have
	     * already done a select() and are pretty confident that
	     * you will not block.
	     * Also assumes buf is aligned on a __pmPDU boundary.
	     */
	    __pmPDU	*lp;
	    status = (int)recv(fd, buf, (int)sizeof(__pmPDU), 0);
	    __pmLastUsedFd = fd;
	    if (status <= 0)
		/* EOF or error */
		return status;
	    else if (status != sizeof(__pmPDU))
		/* short read, bad error! */
		return PM_ERR_IPC;
	    lp = (__pmPDU *)buf;
	    have = ntohl(*lp);
	    status = (int)recv(fd, &buf[sizeof(__pmPDU)], have - (int)sizeof(__pmPDU), 0);
	    if (status <= 0)
		/* EOF or error */
		return status;
	    else if (status != have - (int)sizeof(__pmPDU))
		/* short read, bad error! */
		return PM_ERR_IPC;
	    break;
	}
	else {
	    struct timeval	wait;
	    /*
	     * either never timeout (i.e. block forever), or timeout
	     */
	    if (timeout != TIMEOUT_NEVER) {
		if (timeout > 0) {
		    wait.tv_sec = timeout;
		    wait.tv_usec = 0;
		}
		else
		    wait = def_wait;
		FD_ZERO(&onefd);
		FD_SET(fd, &onefd);
		status = select(fd+1, &onefd, NULL, NULL, &wait);
		if (status == 0) {
		    if (__pmGetInternalState() != PM_STATE_APPL) {
			/* special for PMCD and friends 
			 * Note, on Linux select would return 'time remaing' in
			 * timeout value, so report the expected timeout
			 */
			int tosec, tomsec;

			if ( timeout != TIMEOUT_NEVER && timeout > 0 ) {
			    tosec = (int)timeout;
			    tomsec = 0;
			} else {
			    tosec = (int)def_wait.tv_sec;
			    tomsec = 1000*(int)def_wait.tv_usec;
			}

			__pmNotifyErr(LOG_WARNING, 
				      "pduread: timeout (after %d.%03d "
				      "sec) while attempting to read %d "
				      "bytes out of %d in mode %d on fd=%d",
				      tosec, tomsec, len - have, len, 
				      mode, fd);
		    }
		    return PM_ERR_TIMEOUT;
		}
		else if (status < 0) {
		    __pmNotifyErr(LOG_ERR, "pduread: select() on fd=%d: %s",
			    fd, strerror(errno));
		    return status;
		}
	    }
	    status = (int)recv(fd, buf, len, 0);
	    __pmLastUsedFd = fd;
	    if (status <= 0 || mode == PDU_ASCII)
		/* ASCII, EOF or error */
		return status;
	    if (mode == -1)
		/* special case, see __pmGetPDU */
		return status;
	    have += status;
	    buf += status;
	    len -= status;
	}
    }

    return have;
}

const char *
__pmPDUTypeStr(int type)
{
    /* libpcp PDU names */
    if (type == PDU_ERROR) return "ERROR";
    else if (type == PDU_RESULT) return "RESULT";
    else if (type == PDU_PROFILE) return "PROFILE";
    else if (type == PDU_FETCH) return "FETCH";
    else if (type == PDU_DESC_REQ) return "DESC_REQ";
    else if (type == PDU_DESC) return "DESC";
    else if (type == PDU_INSTANCE_REQ) return "INSTANCE_REQ";
    else if (type == PDU_INSTANCE) return "INSTANCE";
    else if (type == PDU_TEXT_REQ) return "TEXT_REQ";
    else if (type == PDU_TEXT) return "TEXT";
    else if (type == PDU_CONTROL_REQ) return "CONTROL_REQ";
    else if (type == PDU_DATA_X) return "DATA_X";
    else if (type == PDU_CREDS) return "CREDS";
    else if (type == PDU_PMNS_IDS) return "PMNS_IDS";
    else if (type == PDU_PMNS_NAMES) return "PMNS_NAMES";
    else if (type == PDU_PMNS_CHILD) return "PMNS_CHILD";
    else if (type == PDU_PMNS_TRAVERSE) return "PMNS_TRAVERSE";
    /* pmlc <-> pmlogger PDU names from libpcp_dev */
    else if (type == PDU_LOG_CONTROL) return "LOG_CONTROL";
    else if (type == PDU_LOG_STATUS) return "LOG_STATUS";
    else if (type == PDU_LOG_REQUEST) return "LOG_REQUEST";
    /* and anything else ... */
    else {
	static char	buf[20];
	snprintf(buf, sizeof(buf), "TYPE-%d?", type);
	return buf;
    }
}

#if defined(HAVE_SIGPIPE)
/*
 * Because the default handler for SIGPIPE is to exit, we always want a handler
 * installed to override that so that the write() just returns an error.  The
 * problem is that the user might have installed one prior to the first write()
 * or may install one at some later stage.  This doesn't matter.  As long as a
 * handler other than SIG_DFL is there, all will be well.  The first time that
 * __pmXmitPDU is called, install SIG_IGN as the handler for SIGPIPE.  If the
 * user had already changed the handler from SIG_DFL, put back what was there
 * before.
 */
static int sigpipe_done = 0;		/* First time check for installation of
					   non-default SIGPIPE handler */
static void setup_sigpipe()
{
    if (!sigpipe_done) {       /* Make sure SIGPIPE is handled */
	SIG_PF  user_onpipe;
	user_onpipe = signal(SIGPIPE, SIG_IGN);
	if (user_onpipe != SIG_DFL)     /* Put user handler back */
	     signal(SIGPIPE, user_onpipe);
	sigpipe_done = 1;
    }
}
#else
static void setup_sigpipe() { }
#endif

int
__pmXmitPDU(int fd, __pmPDU *pdubuf)
{
    int		off = 0;
    int		len;
    __pmPDUHdr	*php = (__pmPDUHdr *)pdubuf;

    /* assume PDU_BINARY ... should not be here, otherwise */

    setup_sigpipe();

    if (mypid == -1)
	mypid = getpid();
    php->from = mypid;
#ifdef PCP_DEBUG
    if (pmDebug & DBG_TRACE_PDU) {
	int	j;
	char	*p;
	int	jend = (php->len+(int)sizeof(__pmPDU)-1)/(int)sizeof(__pmPDU);

	/* for Purify ... */
	p = (char *)pdubuf + php->len;
	while (p < (char *)pdubuf + jend*sizeof(__pmPDU))
	    *p++ = '~';	/* buffer end */

	fprintf(stderr, "[%d]pmXmitPDU: %s fd=%d len=%d",
		mypid, __pmPDUTypeStr(php->type), fd, php->len);
	for (j = 0; j < jend; j++) {
	    if ((j % 8) == 0)
		fprintf(stderr, "\n%03d: ", j);
	    fprintf(stderr, "%8x ", pdubuf[j]);
	}
	putc('\n', stderr);
    }
#endif
    len = php->len;

    php->len = htonl(php->len);
    php->from = htonl(php->from);
    php->type = htonl(php->type);
    while (off < len) {
	char *p = (char *)pdubuf;
	int n;

	p += off;

	if ((n = (int)send(fd, p, len-off, 0)) < 0) {
	    break;
	}
	off += n;
    }
    php->len = ntohl(php->len);
    php->from = ntohl(php->from);
    php->type = ntohl(php->type);

    if (off != len)
	return (errno) ? -errno : PM_ERR_IPC;

    __pmLastUsedFd = fd;
    if (php->type >= PDU_START && php->type <= PDU_FINISH)
	__pmPDUCntOut[php->type-PDU_START]++;

    return off;
}

int
__pmXmitAscii(int fd, const char *buf, int nbytes)
{
#ifdef PCP_DEBUG
    if (pmDebug & DBG_TRACE_PDU) {
	fprintf(stderr, "[%d]pmXmitPDU: fd=%d ASCII (%d bytes): %s",
	    mypid, fd, nbytes, buf);
	if (buf[nbytes-1] != '\n')
	    putc('\n', stderr);
    }
#endif

    setup_sigpipe();

    if (send(fd, buf, nbytes, 0) != nbytes)
	return -errno;

    __pmLastUsedFd = fd;
    return 0;
}

int
__pmGetPDU(int fd, int mode, int timeout, __pmPDU **result)
{
    int			need;
    int			len;
    static int		maxsize = PDU_CHUNK;
    char		*handle;
    __pmPDU		*pdubuf;
    __pmPDU		*pdubuf_prev;
    __pmPDUHdr		*php;

    if (mypid == -1)
	mypid = getpid();
    if ( (mode == PDU_BINARY) || (mode == PDU_CLIENT) ) {
	if ((pdubuf = __pmFindPDUBuf(maxsize)) == NULL)
	    return -errno;

	/* First read - try to read the header */
	len = pduread(fd, (void *)pdubuf, sizeof(__pmPDUHdr), -1, timeout);
	php = (__pmPDUHdr *)pdubuf;

	if (len < (int)sizeof(__pmPDUHdr)) {
	    if (len == -1) {
		if (errno == ECONNRESET || errno == EPIPE || 
		    errno == ETIMEDOUT || errno == ENETDOWN ||
		    errno == ENETUNREACH || errno == EHOSTDOWN ||
		    errno == EHOSTUNREACH || errno == ECONNREFUSED)
		    /*
		     * Treat this like end of file on input.
		     *
		     * failed as a result of pmcd exiting and the connection
		     * being reset, or as a result of the kernel ripping
		     * down the connection (most likely becuase the host at
		     * the other end just took a dive)
		     *
		     *
		     * from irix/kern/fs/nfs/bds.c seems like all of the
		     * following are peers here:
		     *  ECONNRESET (pmcd terminated?)
		     *  ETIMEDOUT ENETDOWN ENETUNREACH EHOSTDOWN EHOSTUNREACH
		     *  ECONNREFUSED
		     * peers for bds but not here:
		     *  ENETRESET ENONET ESHUTDOWN (cache_fs only?)
		     *  ECONNABORTED (accept, user req only?)
		     *  ENOTCONN (udp?)
		     *  EPIPE EAGAIN (nfs, bds & ..., but not ip or tcp?)
		     */
		    len = 0;
		else
		    __pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d hdr read: len=%d: %s", fd, len, pmErrStr(-errno));
	    }
	    else if (len >= (int)sizeof(php->len)) {
		/*
		 * Have part of a PDU header.  Enough for the "len"
		 * field to be valid, but not yet all of it - save
		 * what we have received and try to read some more.
		 * Note this can only happen once per PDU, so the
		 * ntohl() below will _only_ be done once per PDU.
		 */
		goto check_read_len;	/* continue, do not return */
	    }
	    else if (len == PM_ERR_TIMEOUT)
		return PM_ERR_TIMEOUT;
	    else if (len < 0) {
		__pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d hdr read: len=%d: %s", fd, len, pmErrStr(len));
		return PM_ERR_IPC;
	    }
	    else if (len > 0) {
		__pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d hdr read: bad len=%d", fd, len);
		return PM_ERR_IPC;
	    }

	    /*
	     * end-of-file with no data
	     */
	    return 0;
	}

check_read_len:
	php->len = ntohl(php->len);
	if (php->len < (int)sizeof(__pmPDUHdr)) {
	    /*
	     * PDU length indicates insufficient bytes for a PDU header
	     * ... looks like DOS attack like PV 935490
	     */
	    __pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d illegal PDU len=%d in hdr", fd, php->len);
	    return PM_ERR_IPC;
	} else if ( mode == PDU_CLIENT && php->len > ceiling ) {
	    /*
	     * Guard against denial of service attack ... don't accept PDUs
	     * from clients that are larger than 64 Kbytes (ceiling)
	     * (note, pmcd and pmdas have to be able to _send_ large PDUs,
	     * e.g. for a pmResult or instance domain enquiry)
	     */
	    __pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d bad PDU len=%d in hdr exceeds maximum client PDU size (%d)",
    			  fd, php->len, ceiling);

	    return PM_ERR_TOOBIG;
	}

	if (len < php->len) {
	    /*
	     * need to read more ...
	     */
	    int		tmpsize;
	    int		have = len;

	    if (php->len > maxsize) {
		tmpsize = PDU_CHUNK * ( 1 + php->len / PDU_CHUNK);
	    } else {
		tmpsize = maxsize;
	    }

	    __pmPinPDUBuf(pdubuf);
	    pdubuf_prev = pdubuf;
	    if ((pdubuf = __pmFindPDUBuf(tmpsize)) == NULL) {
		__pmUnpinPDUBuf(pdubuf_prev);
		return -errno;
	    }

	    maxsize = tmpsize;

	    memmove((void *)pdubuf, (void *)php, len);
	    __pmUnpinPDUBuf(pdubuf_prev);

	    php = (__pmPDUHdr *)pdubuf;
	    need = php->len - have;
	    handle = (char *)pdubuf;
	    /* block until all of the PDU is received this time */
	    len = pduread(fd, (void *)&handle[len], need, PDU_BINARY, timeout);
	    if (len != need) {
		if (len == PM_ERR_TIMEOUT)
		    return PM_ERR_TIMEOUT;
		else if (len < 0)
		    __pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d data read: len=%d: %s", fd, len, pmErrStr(-errno));
		else
		    __pmNotifyErr(LOG_ERR, "__pmGetPDU: fd=%d data read: have %d, want %d, got %d", fd, have, need, len);
		/*
		 * only report header fields if you've read enough bytes
		 */
		if (len > 0)
		    have += len;
		if (have >= (int)(sizeof(php->len)+sizeof(php->type)+sizeof(php->from)))
		    __pmNotifyErr(LOG_ERR, "__pmGetPDU: PDU hdr: len=0x%x type=0x%x from=0x%x", php->len, ntohl(php->type), ntohl(php->from));
		else if (have >= (int)(sizeof(php->len)+sizeof(php->type)))
		    __pmNotifyErr(LOG_ERR, "__pmGetPDU: PDU hdr: len=0x%x type=0x%x", php->len, ntohl(php->type));
		return PM_ERR_IPC;
	    }
	}

	*result = (__pmPDU *)php;
	php->type = ntohl((unsigned int)php->type);
	php->from = ntohl((unsigned int)php->from);
#ifdef PCP_DEBUG
	if (pmDebug & DBG_TRACE_PDU) {
	    int		j;
	    char	*p;
	    int		jend = (php->len+(int)sizeof(__pmPDU)-1)/(int)sizeof(__pmPDU);

	    /* for Purify ... */
	    p = (char *)*result + php->len;
	    while (p < (char *)*result + jend*sizeof(__pmPDU))
		*p++ = '~';	/* buffer end */

	    fprintf(stderr, "[%d]pmGetPDU: %s fd=%d len=%d from=%d",
		    mypid, __pmPDUTypeStr(php->type), fd, php->len, php->from);
	    for (j = 0; j < jend; j++) {
		if ((j % 8) == 0)
		    fprintf(stderr, "\n%03d: ", j);
		fprintf(stderr, "%8x ", (*result)[j]);
	    }
	    putc('\n', stderr);
	}
#endif
	if (php->type >= PDU_START && php->type <= PDU_FINISH)
	    __pmPDUCntIn[php->type-PDU_START]++;

	return php->type;
    }
    else {
	/* assume PDU_ASCII */
	if (ac.len && ac.next < ac.len) {
	    __pmNotifyErr(LOG_WARNING, "__pmGetPDU: fd=%d extra ASCII PDU text discarded \"%s\"", fd, &ac.abuf[ac.next]);
	}
	ac.fd = fd;
	ac.timeout = timeout;
	ac.len = pduread(fd, ac.abuf, ABUF_SIZE-1, PDU_ASCII, timeout);
	ac.next = 0;

	/* expect at least one line, with magic PDU cookie at the start */
	if (ac.len == PM_ERR_IPC || ac.len == PM_ERR_TIMEOUT)
	    /* errno not always set */
	    return ac.len;
	else if (ac.len < 0)
	    return -errno;
	else if (ac.len == 0)
	    return PM_ERR_EOF;
	ac.abuf[ac.len] = '\0';
#ifdef PCP_DEBUG
	if (pmDebug & DBG_TRACE_PDU) {
	    fprintf(stderr, "[%d]pmGetPDU: fd=%d ASCII (%d bytes): %s",
		mypid, fd, ac.len, ac.abuf);
	    if (ac.abuf[ac.len-1] != '\n')
		putc('\n', stderr);
	}
#endif

	*result = (__pmPDU *)&ac;		/* ugly, but caller sees this as opaque */

	if (strncmp(ac.abuf, "INSTANCE_REQ", 12) == 0)
	    return PDU_INSTANCE_REQ;
	else if (strncmp(ac.abuf, "CONTROL_REQ", 11) == 0)
	    return PDU_CONTROL_REQ;
	else if (strncmp(ac.abuf, "DESC_REQ", 8) == 0)
	    return PDU_DESC_REQ;
	else if (strncmp(ac.abuf, "INSTANCE", 8) == 0)
	    return PDU_INSTANCE;
	else if (strncmp(ac.abuf, "DESC_REQ", 8) == 0)
	    return PDU_DESC_REQ;
	else if (strncmp(ac.abuf, "TEXT_REQ", 8) == 0)
	    return PDU_TEXT_REQ;
	else if (strncmp(ac.abuf, "PROFILE", 7) == 0)
	    return PDU_PROFILE;
	else if (strncmp(ac.abuf, "RESULT", 6) == 0)
	    return PDU_RESULT;
	else if (strncmp(ac.abuf, "ERROR", 5) == 0)
	    return PDU_ERROR;
	else if (strncmp(ac.abuf, "FETCH", 5) == 0)
	    return PDU_FETCH;
	else if (strncmp(ac.abuf, "DESC", 4) == 0)
	    return PDU_DESC;
	else if (strncmp(ac.abuf, "TEXT", 4) == 0)
	    return PDU_TEXT;
	else {
	    char	*p;

	    for (p = ac.abuf; *p && *p != ' ' && *p != '\n'; p++)
		;
	    *p = '\0';
	    __pmNotifyErr(LOG_WARNING, "__pmGetPDU: fd=%d ASCII type=\"%s\"?", fd, ac.abuf);
	    return PM_ERR_IPC;
	}
    }
}

/*
 * deprecated old interface ...
 */
int
__pmRecvPDU(int fd, int mode, __pmPDU **result)
{
    return __pmGetPDU(fd, mode, TIMEOUT_NEVER, result);
}

/*
 * Fetch next line of input from an ASCII format PDU stream.
 *
 * Note: pdubuf is really an opaque pointer to the (ascii_ctl *) value
 * returned from an earlier call to __pmGetPDU.
 */
int
__pmRecvLine(__pmPDU *pdubuf, int maxch, char *lbuf)
{
    ascii_ctl	*ap = (ascii_ctl *)pdubuf;
    char	*p = lbuf;
    char	*lend = &lbuf[maxch];

    for ( ; ; ) {
	if (ap->next >= ap->len) {
	    ap->len = pduread(ap->fd, ap->abuf, ABUF_SIZE, PDU_ASCII, ap->timeout);
	    if (ap->len == PM_ERR_IPC || ap->len == PM_ERR_TIMEOUT)
		/* errno not always set */
		return ap->len;
	    else if (ap->len < 0)
		return -errno;
	    else if (ap->len == 0)
		return PM_ERR_EOF;
	    ap->abuf[ac.len] = '\0';
#ifdef PCP_DEBUG
	    if (pmDebug & DBG_TRACE_PDU) {
		fprintf(stderr, "[%d]pmRecvLine: ASCII (%d bytes): %s", mypid, ap->len, ap->abuf);
		if (ap->abuf[ap->len-1] != '\n')
		    putc('\n', stderr);
	    }
#endif
	    ap->next = 0;
	}
	if (p >= lend)
	    return PM_ERR_TOOBIG;
	*p = ap->abuf[ap->next++];
	if (*p == '\n')
	    break;
	p++;
    }
    *p = '\0';

    return (int)(p - lbuf);
}

int
__pmGetPDUCeiling (void)
{
    return ceiling;
}

int
__pmSetPDUCeiling (int newceiling)
{
    if ( newceiling > 0 ) {
	return (ceiling = newceiling);
    } else {
	return (ceiling);
    }
}

void
__pmSetPDUCntBuf(unsigned *in, unsigned *out)
{
    __pmPDUCntIn = in;
    __pmPDUCntOut = out;
}
