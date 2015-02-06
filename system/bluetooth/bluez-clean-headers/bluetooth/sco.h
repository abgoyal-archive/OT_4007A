
#ifndef __SCO_H
#define __SCO_H

#ifdef __cplusplus
#endif
#define SCO_DEFAULT_MTU 500
#define SCO_DEFAULT_FLUSH_TO 0xFFFF
#define SCO_CONN_TIMEOUT (HZ * 40)
#define SCO_DISCONN_TIMEOUT (HZ * 2)
#define SCO_CONN_IDLE_TIMEOUT (HZ * 60)
#define SCO_OPTIONS 0x01
#define SCO_CONNINFO 0x02
#ifdef __cplusplus
#endif
struct sockaddr_sco {
	sa_family_t	sco_family;
	bdaddr_t	sco_bdaddr;
	uint16_t	sco_pkt_type;
};
#endif
