#include "arp.h"
#include "interface.h"   // NIC interface from your base project
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

// Store our MAC/IP from config
static uint8_t MY_MAC_ADDR[6] = MY_MAC;
static uint32_t MY_IP_ADDR = MY_IP;

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

// Function to send an ARP request
void arp_send_request(nic_driver_t *drv,nic_device_t *device,uint32_t target_ip) {
    uint8_t buf[42]; // Ethernet (14 bytes) + ARP (28 bytes)
    struct ethernet_frame *eth = (void*)buf; // Ethernet header
    struct arp_packet *arp = (void*)(buf + sizeof(*eth)); // ARP packet starts after Ethernet header

    // Ethernet broadcast
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // Broadcast MAC address
    memcpy(eth->dst_mac, broadcast, 6);// Destination MAC: Broadcast
    memcpy(eth->src_mac, MY_MAC_ADDR, 6);// Source MAC: Our MAC
    eth->type = htons(ETH_P_ARP);// Ethernet type: ARP

    // ARP fields
    arp->htype = htons(1);          // Ethernet
    arp->ptype = htons(0x0800);     // IPv4
    arp->hlen = 6; // Hardware address length
    arp->plen = 4; // Protocol address length
    arp->oper = htons(ARP_REQUEST); // Operation: ARP Request
    memcpy(arp->sha, MY_MAC_ADDR, 6); // Sender MAC: Our MAC
    arp->spa = htonl(MY_IP_ADDR); // Sender IP
    memset(arp->tha, 0x00, 6);// Target MAC 
    arp->tpa = htonl(target_ip); // Target IP

    drv->send_packet(device,buf, sizeof(buf));  // send via NIC interface

    printf("[ARP] Request: Who has %d.%d.%d.%d? Tell %d.%d.%d.%d\n",
    (target_ip>>24)&0xFF, (target_ip>>16)&0xFF,
    (target_ip>>8)&0xFF, target_ip&0xFF,
    (MY_IP>>24)&0xFF, (MY_IP>>16)&0xFF,
    (MY_IP>>8)&0xFF, MY_IP&0xFF);
}

// Function to send an ARP reply
void arp_send_reply(nic_driver_t *drv,nic_device_t *device,uint8_t *target_mac, uint32_t target_ip) {
    uint8_t buf[42]; // Ethernet + ARP
    struct ethernet_frame *eth = (void*)buf;
    struct arp_packet *arp = (void*)(buf + sizeof(*eth));

    memcpy(eth->dst_mac, target_mac, 6);
    memcpy(eth->src_mac, MY_MAC_ADDR, 6);
    eth->type = htons(ETH_P_ARP);

    arp->htype = htons(1); // Ethernet
    arp->ptype = htons(0x0800); // IPv4
    arp->hlen = 6; // Hardware address length
    arp->plen = 4; // Protocol address length
    arp->oper = htons(ARP_REPLY); // Operation: ARP Reply
    memcpy(arp->sha, MY_MAC_ADDR, 6); // Sender MAC
    arp->spa = htonl(MY_IP_ADDR); // Sender IP
    memcpy(arp->tha, target_mac, 6); // Target MAC
    arp->tpa = htonl(target_ip); // Target IP

    drv->send_packet(device,buf, sizeof(buf));  // send via NIC interface

    printf("[ARP] Reply: %d.%d.%d.%d is at %02X:%02X:%02X:%02X:%02X:%02X\n",
    (target_ip>>24)&0xFF, (target_ip>>16)&0xFF,
    (target_ip>>8)&0xFF, target_ip&0xFF,
    target_mac[0],target_mac[1],target_mac[2],
    target_mac[3],target_mac[4],target_mac[5]);
}