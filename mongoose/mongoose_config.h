#ifndef __MONGOOSE_CONFIG_H__
#define __MONGOOSE_CONFIG_H__

#include "CO_driver_target.h"

#define MG_ARCH	MG_ARCH_NEWLIB
#define MG_TLS	MG_TLS_NONE

#define MG_ENABLE_TCPIP			1
#define MG_ENABLE_CUSTOM_MILLIS	1
#define MG_ENABLE_CUSTOM_RANDOM	1
#define MG_ENABLE_DRIVER_W5500	1
#define MG_ENABLE_PACKED_FS		1

#define MIP_TCP_KEEPALIVE_MS	30000
#define MG_IO_SIZE				256

// For static IP configuration, define MG_TCPIP_{IP, MASK, GW}
// By default, those are set to zero, meaning that DHCP is used
#define MG_TCPIP_IP		MG_IPV4(192, 168,   1, 20) // IP
#define MG_TCPIP_MASK	MG_IPV4(255, 255, 255,  0) // Netmask
#define MG_TCPIP_GW		MG_IPV4(192, 168,   1,  1) // Gateway

#define MG_SET_MAC_ADDRESS(mac)      \
  do {                               \
    mac[0] = 2;                      \
    mac[1] = MGUID[0] & 255;         \
    mac[2] = (MGUID[1] >> 10) & 255; \
    mac[3] = (MGUID[1] >> 19) & 255; \
    mac[4] = MGUID[2] & 255;         \
    mac[5] = MGUID[3] & 255;         \
  } while (0)

#endif
