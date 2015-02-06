

#ifndef lint
static const char rcsid[] _U_ =
    "@(#) $Header: /tcpdump/master/tcpdump/print-null.c,v 1.53.2.4 2007/02/26 13:31:33 hannes Exp $ (LBL)";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tcpdump-stdinc.h>

#include <pcap.h>
#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "addrtoname.h"
#include "af.h"

#include "ip.h"
#ifdef INET6
#include "ip6.h"
#endif

#define	NULL_HDRLEN 4


#define	SWAPLONG(y) \
((((y)&0xff)<<24) | (((y)&0xff00)<<8) | (((y)&0xff0000)>>8) | (((y)>>24)&0xff))

static inline void
null_hdr_print(u_int family, u_int length)
{
	if (!qflag) {
		(void)printf("AF %s (%u)",
			tok2str(bsd_af_values,"Unknown",family),family);
	} else {
		(void)printf("%s",
			tok2str(bsd_af_values,"Unknown AF %u",family));
	}

	(void)printf(", length %u: ", length);
}

u_int
null_if_print(const struct pcap_pkthdr *h, const u_char *p)
{
	u_int length = h->len;
	u_int caplen = h->caplen;
	u_int family;

	if (caplen < NULL_HDRLEN) {
		printf("[|null]");
		return (NULL_HDRLEN);
	}

	memcpy((char *)&family, (char *)p, sizeof(family));

	/*
	 * This isn't necessarily in our host byte order; if this is
	 * a DLT_LOOP capture, it's in network byte order, and if
	 * this is a DLT_NULL capture from a machine with the opposite
	 * byte-order, it's in the opposite byte order from ours.
	 *
	 * If the upper 16 bits aren't all zero, assume it's byte-swapped.
	 */
	if ((family & 0xFFFF0000) != 0)
		family = SWAPLONG(family);

	if (eflag)
		null_hdr_print(family, length);

	length -= NULL_HDRLEN;
	caplen -= NULL_HDRLEN;
	p += NULL_HDRLEN;

	switch (family) {

	case BSD_AFNUM_INET:
		ip_print(gndo, p, length);
		break;

#ifdef INET6
	case BSD_AFNUM_INET6_BSD:
	case BSD_AFNUM_INET6_FREEBSD:
	case BSD_AFNUM_INET6_DARWIN:
		ip6_print(p, length);
		break;
#endif

	case BSD_AFNUM_ISO:
		isoclns_print(p, length, caplen);
		break;

	case BSD_AFNUM_APPLETALK:
		atalk_print(p, length);
		break;

	case BSD_AFNUM_IPX:
		ipx_print(p, length);
		break;

	default:
		/* unknown AF_ value */
		if (!eflag)
			null_hdr_print(family, length + NULL_HDRLEN);
		if (!suppress_default_print)
			default_print(p, caplen);
	}

	return (NULL_HDRLEN);
}

