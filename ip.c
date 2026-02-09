#include "ip.h"
#include "arp_table.h"
#include "arp.h"
#include "config.h"
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

#define ETH_P_IP 0x0800

struct eth_hdr {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed));

struct ip_hdr {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed));

static uint16_t ip_checksum(void *vdata, size_t length) {
    uint32_t acc = 0;
    uint16_t *data = vdata;

    for (; length > 1; length -= 2)
        acc += *data++;

    if (length)
        acc += *(uint8_t *)data;

    while (acc >> 16)
        acc = (acc & 0xFFFF) + (acc >> 16);

    return htons(~acc);
}

void ip_send_packet(
    nic_driver_t *drv,
    nic_device_t *device,
    uint32_t dst_ip,
    uint8_t protocol,
    const uint8_t *payload,
    uint16_t payload_len
) {
    uint8_t *dst_mac = arp_table_lookup(dst_ip);

    if (!dst_mac) {
        printf("[IP] MAC unknown for %d.%d.%d.%d → ARP request sent\n",
            (dst_ip>>24)&0xFF,(dst_ip>>16)&0xFF,
            (dst_ip>>8)&0xFF,dst_ip&0xFF);

        arp_send_request(drv, device, dst_ip);
        return;
    }

    uint8_t buf[1500];
    struct eth_hdr *eth = (void*)buf;
    struct ip_hdr  *ip  = (void*)(buf + sizeof(*eth));

    memcpy(eth->dst, dst_mac, 6);
    memcpy(eth->src, device->mac_address, 6);
    eth->type = htons(ETH_P_IP);

    ip->ver_ihl  = 0x45;
    ip->tos      = 0;
    ip->tot_len = htons(sizeof(*ip) + payload_len);
    ip->id       = htons(1);
    ip->frag_off = htons(0);
    ip->ttl      = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->saddr    = htonl(MY_IP);
    ip->daddr    = htonl(dst_ip);

    ip->checksum = ip_checksum(ip, sizeof(*ip));

    memcpy(buf + sizeof(*eth) + sizeof(*ip), payload, payload_len);

    drv->send_packet(device, buf,
        sizeof(*eth) + sizeof(*ip) + payload_len);

    printf("[IP] Sent IPv4 packet to %d.%d.%d.%d\n",
        (dst_ip>>24)&0xFF,(dst_ip>>16)&0xFF,
        (dst_ip>>8)&0xFF,dst_ip&0xFF);
}