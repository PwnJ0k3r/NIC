#ifndef ARP_TABLE_H
#define ARP_TABLE_H

#include <stdint.h>

#define ARP_TABLE_SIZE 8

typedef struct {
    uint32_t ip;
    uint8_t  mac[6];
    int      valid;
} arp_entry_t;

void arp_table_init(void);
void arp_table_add(uint32_t ip, uint8_t *mac);
uint8_t *arp_table_lookup(uint32_t ip);
void arp_table_print(void);

#endif