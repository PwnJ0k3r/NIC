#ifndef IP_H
#define IP_H

#include <stdint.h>
#include "interface.h"

void ip_send_packet(
    nic_driver_t *drv,
    nic_device_t *device,
    uint32_t dst_ip,
    uint8_t protocol,
    const uint8_t *payload,
    uint16_t payload_len
);

#endif