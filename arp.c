#include "arp.h"
#include "interface.h" 
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

// Ethernet frame header
struct ethernet_frame { //14 bytes
    uint8_t dst_mac[6]; // Destination MAC
    uint8_t src_mac[6]; // Source MAC
    uint16_t type; // Ethernet type (e.g., 0x0800 for IPv4, 0x0806 for ARP)
} __attribute__((packed));


// ARP packet structure
struct arp_packet { //28 bytes
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
} __attribute__((packed));

/****************** TX Functions ******************/

void arp_send_request(nic_driver_t *drv, nic_device_t *device, uint32_t target_ip) {
    uint8_t buf[42];
    struct ethernet_frame *eth = (void*)buf;
    struct arp_packet *arp = (void*)(buf + sizeof(*eth));

    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    memcpy(eth->dst_mac, broadcast, 6);
    memcpy(eth->src_mac, device->mac_address, 6);
    eth->type = htons(ETH_P_ARP);

    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(ARP_REQUEST);
    memcpy(arp->sha, device->mac_address, 6);
    arp->spa = htonl(MY_IP);
    memset(arp->tha, 0, 6);
    arp->tpa = htonl(target_ip);

    drv->send_packet(device, buf, sizeof(buf));

    printf("[ARP] Request: Who has %d.%d.%d.%d?\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF);
}

void arp_send_reply(nic_driver_t *drv, nic_device_t *device, uint8_t *target_mac, uint32_t target_ip) {
    uint8_t buf[42];
    struct ethernet_frame *eth = (void*)buf;
    struct arp_packet *arp = (void*)(buf + sizeof(*eth));

    memcpy(eth->dst_mac, target_mac, 6);
    memcpy(eth->src_mac, device->mac_address, 6);
    eth->type = htons(ETH_P_ARP);

    arp->htype = htons(1);
    arp->ptype = htons(0x0800);
    arp->hlen  = 6;
    arp->plen  = 4;
    arp->oper  = htons(ARP_REPLY);
    memcpy(arp->sha, device->mac_address, 6);
    arp->spa = htonl(MY_IP);
    memcpy(arp->tha, target_mac, 6);
    arp->tpa = htonl(target_ip);

    drv->send_packet(device, buf, sizeof(buf));

    printf("[ARP] Reply: %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
        (target_ip>>24)&0xFF,(target_ip>>16)&0xFF,
        (target_ip>>8)&0xFF,target_ip&0xFF,
        target_mac[0],target_mac[1],target_mac[2],
        target_mac[3],target_mac[4],target_mac[5]);

    arp_table_add(target_ip, target_mac);
}


/****************** RX Function ******************/
/*
void arp_rx(nic_device_t *nic, nic_driver_t *drv,
            const uint8_t *frame, size_t len)
{
    struct arp_hdr *arp = (struct arp_hdr *)(frame + ETH_HDR_LEN);

    if (ntohs(arp->oper) != ARP_REQUEST)
        return;

    if (arp->target_ip != htonl(MY_IP))
        return;

    arp_send_reply(
        drv,
        nic,
        arp->sender_mac,
        ntohl(arp->sender_ip)
    );
}
*/
void arp_rx(uint8_t *buf, unsigned int len) {
    if (len < sizeof(struct ethernet_frame) + sizeof(struct arp_packet)) return;

    struct ethernet_frame *eth = (void*)buf; // apunta a la cabecera Ethernet al inicio del buffer.
    struct arp_packet *arp = (void*)(buf + sizeof(*eth)); // apunta justo después de la cabecera Ethernet, donde empieza el paquete ARP.

    if (ntohs(eth->type) != ETH_P_ARP) return;

    uint16_t oper = ntohs(arp->oper);

    // Solo procesamos Reply
    if (oper == ARP_REPLY) {
        uint32_t sender_ip = ntohl(arp->spa);
        uint8_t *sender_mac = arp->sha;

        arp_table_add(sender_ip, sender_mac);

        printf("[ARP RX] Reply from %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
            (sender_ip>>24)&0xFF,(sender_ip>>16)&0xFF,
            (sender_ip>>8)&0xFF,sender_ip&0xFF,
            sender_mac[0],sender_mac[1],sender_mac[2],
            sender_mac[3],sender_mac[4],sender_mac[5]);
    }
}