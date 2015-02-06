
/* @(#) $Header: /tcpdump/master/tcpdump/tftp.h,v 1.1.2.1 2007/09/14 01:03:12 guy Exp $ (LBL) */

#ifndef _TFTP_H_
#define	_TFTP_H_

#define	SEGSIZE		512		/* data segment size */

#define	RRQ	01			/* read request */
#define	WRQ	02			/* write request */
#define	DATA	03			/* data packet */
#define	ACK	04			/* acknowledgement */
#define	ERROR	05			/* error code */
#define OACK	06			/* option acknowledgement */

struct	tftphdr {
	unsigned short	th_opcode;		/* packet type */
	union {
		unsigned short	tu_block;	/* block # */
		unsigned short	tu_code;	/* error code */
		char	tu_stuff[1];	/* request packet stuff */
	} th_u;
	char	th_data[1];		/* data or error string */
};

#define	th_block	th_u.tu_block
#define	th_code		th_u.tu_code
#define	th_stuff	th_u.tu_stuff
#define	th_msg		th_data

#define	EUNDEF		0		/* not defined */
#define	ENOTFOUND	1		/* file not found */
#define	EACCESS		2		/* access violation */
#define	ENOSPACE	3		/* disk full or allocation exceeded */
#define	EBADOP		4		/* illegal TFTP operation */
#define	EBADID		5		/* unknown transfer ID */
#define	EEXISTS		6		/* file already exists */
#define	ENOUSER		7		/* no such user */

#endif /* !_TFTP_H_ */
