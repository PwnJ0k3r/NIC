#include <stdio.h>
#include <string.h>
#include "arp_table.h"

static arp_entry_t arp_table[ARP_TABLE_SIZE];

void arp_table_init(void) {
    memset(arp_table, 0, sizeof(arp_table)); // Vacía memoria
}

void arp_table_add(uint32_t ip, uint8_t *mac) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip = ip;
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].valid = 1;
            return;
        }
    }
    // Si la tabla está llena, reemplaza la primera entrada (simple)
    arp_table[0].ip = ip;
    memcpy(arp_table[0].mac, mac, 6);
    arp_table[0].valid = 1;
}

uint8_t *arp_table_lookup(uint32_t ip) {
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            return arp_table[i].mac;
        }
    }
    return NULL;
}

void arp_table_print(void) {
    printf("\nARP table:\n");
    printf("IP address        MAC address\n");
    printf("----------------  -----------------\n");
    for (int i = 0; i < ARP_TABLE_SIZE; i++) {
        if (arp_table[i].valid) {
            uint32_t ip = arp_table[i].ip;
            uint8_t *m = arp_table[i].mac;
            printf("%d.%d.%d.%d     %02X:%02X:%02X:%02X:%02X:%02X\n",
                (ip>>24)&0xFF, (ip>>16)&0xFF,
                (ip>>8)&0xFF, ip&0xFF,
                m[0],m[1],m[2],m[3],m[4],m[5]);
        }
    }
}