#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include "config.h" 
#include "interface.h"

// Tipos de ARP
#define ARP_REQUEST 1
#define ARP_REPLY   2
#define ETH_P_ARP   0x0806

// Funciones públicas
void arp_send_request(nic_driver_t *drv,nic_device_t *device,uint32_t target_ip);
void arp_send_reply(nic_driver_t *drv,nic_device_t *device,uint8_t *target_mac, uint32_t target_ip);

#endif // ARP_H