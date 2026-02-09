#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "interface.h"

//Hardcoded test packet for sending
#include "config.h"
//ARP
#include "arp.h"
#include "arp_table.h"
//  IP
#include "ip.h"

struct ethernet_frame {
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short ethertype;
    unsigned char payload[1500]; //Hardcoded mtu for testing!!!
} __attribute__((packed));

void received_packet_eth(const void *data, unsigned int length) {
    printf("Received packet of length %u [", length);
    const unsigned char *bytes = (const unsigned char *)data;
    for (unsigned int i = 0; i < length && i < 10; i++) {
        printf("%02x ", bytes[i]);
    }
    printf("]\n");
}

void received_packet(const void *data, unsigned int length)
{
    uint8_t *frame = (uint8_t *)data;

    struct ethernet_frame *patata = (struct ethernet_frame *)data;

    // Imprimir MAC de destino
    printf("Dest MAC: ");
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
        patata->dest_mac[0], patata->dest_mac[1], patata->dest_mac[2], patata->dest_mac[3], patata->dest_mac[4], patata->dest_mac[5]);


    // Imprimir MAC de origen
    printf("Src MAC: ");
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
        patata->src_mac[0], patata->src_mac[1], patata->src_mac[2], patata->src_mac[3], patata->src_mac[4], patata->src_mac[5]);


    // Imprimir Ethertype
    printf("Ethertype: %04x\n", ntohs(patata->ethertype));
    
    if (length < 14)
    return;
    
    //uint16_t ethertype = (frame[12] << 8) | frame[13];
    //printf("Received Ethernet frame with Ethertype: 0x%04x\n", ethertype);

    switch (ntohs(patata->ethertype)) {
        case 0x9000: // Test Ethernet
            received_packet_eth(patata, length);
            break;
        case 0x0806: // ARP
            //arp_rx(frame, length);
            break;

        case 0x0800: // IPv4 (futuro)
            // ip_rx(frame, length);
            break;

        default:
            break;
    }
}



unsigned int test_ethernet(unsigned char * buffer, const unsigned char *src, const unsigned char *dst, unsigned short ethertype, const unsigned char *payload, unsigned int payload_length) {
    struct ethernet_frame *frame = (struct ethernet_frame *)buffer;
    for (int i = 0; i < 6; i++) {
        frame->dest_mac[i] = dst[i];
    }
    for (int i = 0; i < 6; i++) {
        frame->src_mac[i] = src[i];
    }
    frame->ethertype = htons(ethertype);
    memcpy(frame->payload, payload, payload_length);
    return sizeof(struct ethernet_frame) - 1500 + payload_length; //Hardcoded mtu for testing!!!
}


// Funciones de test separadas
void test_ethernet_packet(nic_driver_t *drv, nic_device_t *nic) {

    
    printf("\n=== ETHERNET TEST ===\n");
    // Registrar callbacks
    if (drv->ioctl(nic, NIC_IOCTL_ADD_RX_CALLBACK, (void *)&received_packet_eth) != STATUS_OK) {
        printf("Failed to add RX callback\n");
        drv->shutdown(nic);
        return;
    }
    
    unsigned char buffer[2048];
    unsigned int packet_length = test_ethernet(
        buffer,
        nic->mac_address,
        (unsigned char[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Broadcast
        0x9000,// Ethertype for testing
        (unsigned char *)"Hello, this is a test payload",//Payload content
        29// Payload length
    );
    
    if (drv->send_packet(nic, buffer, packet_length) != STATUS_OK) {
        printf("Failed to send Ethernet packet\n");
        drv->shutdown(nic);
        return;
    } 
    
}

void test_arp_request(nic_driver_t *drv, nic_device_t *nic) {
    printf("\n=== ARP REQUEST TEST ===\n");

    // Registrar callbacks
    if (drv->ioctl(nic, NIC_IOCTL_ADD_RX_CALLBACK, (void *)&received_packet) != STATUS_OK) {
        printf("Failed to add RX callback\n");
        drv->shutdown(nic);
        return;
    }

    // Inicializar tabla ARP
    arp_table_init();
    
    printf("Sending ARP Request to %d.%d.%d.%d...\n",
        (CLIENT_IP>>24)&0xFF, (CLIENT_IP>>16)&0xFF,
        (CLIENT_IP>>8)&0xFF, CLIENT_IP&0xFF);
    
    arp_send_request(drv, nic, CLIENT_IP);
    printf("ARP Request sent!\n");
    
    uint8_t client_mac[6] = CLIENT_MAC;
    
    printf("Sending ARP Reply to %d.%d.%d.%d...\n",
        (CLIENT_IP>>24)&0xFF, (CLIENT_IP>>16)&0xFF,
        (CLIENT_IP>>8)&0xFF, CLIENT_IP&0xFF);
    
    arp_send_reply(drv, nic, client_mac, CLIENT_IP);
    printf("ARP Reply sent!\n");
}

void test_arp_passive(nic_driver_t *drv, nic_device_t *nic) {
    printf("\n=== ARP PASSIVE MODE ===\n");

     // Registrar callbacks
    if (drv->ioctl(nic, NIC_IOCTL_ADD_RX_CALLBACK, (void *)&received_packet) != STATUS_OK) {
        printf("Failed to add RX callback\n");
        drv->shutdown(nic);
        return;
    }

    /*printf("Waiting for ARP requests for %d.%d.%d.%d ...\n",
        (MY_IP >> 24) & 0xFF,
        (MY_IP >> 16) & 0xFF,
        (MY_IP >> 8) & 0xFF,
        MY_IP & 0xFF);*/

    printf("Waiting for ARP requests for: %02x:%02x:%02x:%02x:%02x:%02x\n",
            nic->mac_address[0], nic->mac_address[1], nic->mac_address[2],
            nic->mac_address[3], nic->mac_address[4], nic->mac_address[5]);
    
    //printf("Press Enter to stop listening...\n");
    getchar();
}

/*
void test_ip_packet(nic_driver_t *drv, nic_device_t *nic) {
    printf("\n=== IP PACKET TEST ===\n");
    
    uint8_t client_mac[6] = CLIENT_MAC;
    arp_table_add(CLIENT_IP, client_mac);
    
    uint8_t msg[] = "Hello over IPv4";
    
    printf("Sending IP packet to %d.%d.%d.%d...\n",
        (CLIENT_IP>>24)&0xFF, (CLIENT_IP>>16)&0xFF,
        (CLIENT_IP>>8)&0xFF, CLIENT_IP&0xFF);
    
    ip_send_packet(drv, nic, CLIENT_IP, 1, msg, sizeof(msg) - 1);
    printf("IP packet sent!\n");
}
*/

void show_arp_table() {
    printf("\n=== ARP TABLE ===\n");
    arp_table_print();
}

void show_menu() {
    printf("\n========================================\n");
    printf("      NETWORK STACK TEST MENU\n");
    printf("========================================\n");
    printf("1. Ethernet Frame Test\n");
    printf("2. ARP Request Test\n");
    printf("3. ARP Passive Mode\n");
    printf("4. IP Packet Test\n");
    printf("5. Show ARP Table\n");
    printf("0. Exit\n");
    printf("========================================\n");
    printf("Select an option: ");
}

int main(int argc, char* argv[]) {
    nic_device_t nic;
    nic_driver_t *drv = nic_get_driver();
    int option;
    int running = 1;


    // Menú principal
    while (running) {
        // Inicializar NIC
        if (drv->init(&nic) != STATUS_OK) {
            printf("Failed to initialize NIC\n");
            return -1;
        }

        printf("NIC initialized successfully!\n");
        printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            nic.mac_address[0], nic.mac_address[1], nic.mac_address[2],
            nic.mac_address[3], nic.mac_address[4], nic.mac_address[5]);

        // Registrar callbacks
        /*if (drv->ioctl(&nic, NIC_IOCTL_ADD_RX_CALLBACK, (void *)&received_packet) != STATUS_OK) {
            printf("Failed to add RX callback\n");
            drv->shutdown(&nic);
            return -1;
        }*/
        
        show_menu();

        
        if (scanf("%d", &option) != 1) {
            // Limpiar buffer en caso de entrada inválida
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        // Limpiar buffer
        while (getchar() != '\n');

        switch (option) {
            case 1:
                test_ethernet_packet(drv, &nic);
                break;
            
            case 2:
                test_arp_request(drv, &nic);
                break;
            
            case 3:
                test_arp_passive(drv, &nic);
                break;
            
            case 4:
                //test_ip_packet(drv, &nic);
                break;
            
            case 5:
                show_arp_table(drv, &nic);
                break;
            
            case 0:
                printf("\nExiting...\n");
                running = 0;
                break;
            
            default:
                printf("Invalid option. Please try again.\n");
                break;
        }
        
        if (running && option != 0) {
            //printf("\nPress Enter to continue...");
            printf("Press Enter to stop ...\n");
            getchar();

            // Apagar NIC
            if (drv->shutdown(&nic) != STATUS_OK) {
                printf("Failed to shutdown NIC\n");
                return -1;
            }
        }
    }

    // Mostrar tabla ARP final
    printf("\nFinal ARP Table:\n");
    arp_table_print();

    

    printf("NIC shut down successfully.\n");
    return 0;
}