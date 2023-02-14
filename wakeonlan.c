#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "wakeonlan.h"

/*
    https://github.com/GramThanos/WakeOnLAN/blob/master/WakeOnLAN.c
*/

// Create Magic Packet
void createMagicPacket(unsigned char packet[], unsigned int macAddress[]){
	int i;
	// Mac Address Variable
	unsigned char mac[6];

	// 6 x 0xFF on start of packet
	for(i = 0; i < 6; i++){
		packet[i] = 0xFF;
		mac[i] = macAddress[i];
	}
	// Rest of the packet is MAC address of the pc
	for(i = 1; i <= 16; i++){
		memcpy(&packet[i * 6], &mac, 6 * sizeof(unsigned char));
	}
}

int wakeonlan(char *targetAddress) {
    // Default broadcast address
	char broadcastAddress[] = "255.255.255.255";
	// Packet buffer
	unsigned char packet[102];
	// Mac address
	unsigned int mac[6];
	// Set broadcast
	int broadcast = 1;

    // Aux
    int i = 0;

	// Socket address
	struct sockaddr_in udpClient, udpServer;

    i = sscanf(targetAddress, "%x:%x:%x:%x:%x:%x", &(mac[0]), &(mac[1]), &(mac[2]), &(mac[3]), &(mac[4]), &(mac[5]));
	if(i != 6){
		printf("Invalid mac address to wakeonlan\n");
		return -1;
	}

    createMagicPacket(packet, mac);

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        printf("An error was encountered creating the UDP socket: '%s'.\n", strerror(errno));
        return -1;
    }
    int setsock_result = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
    if (setsock_result == -1) {
        printf("Failed to set socket options: '%s'.\n", strerror(errno));
        return -1;
    }
    // Set parameters
    udpClient.sin_family = AF_INET;
    udpClient.sin_addr.s_addr = INADDR_ANY;
    udpClient.sin_port = 0;
    // Bind socket
    int bind_result = bind(udpSocket, (struct sockaddr*) &udpClient, sizeof(udpClient));
    if (bind_result == -1) {
        printf("Failed to bind socket: '%s'.\n", strerror(errno));
        return -1;
    }

    // Set server end point (the broadcast addres)
    udpServer.sin_family = AF_INET;
    udpServer.sin_addr.s_addr = inet_addr(broadcastAddress);
    udpServer.sin_port = htons(9);

    // Send the packet
    int result = sendto(udpSocket, &packet, sizeof(unsigned char) * 102, 0, (struct sockaddr*) &udpServer, sizeof(udpServer));
    if (result == -1) {
        printf("Failed to send magic packet to socket: '%s'.\n", strerror(errno));
        return -1;
    }

	return 0;

}