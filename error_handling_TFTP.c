//
// Created by doomz on 3/30/25.
//

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "UDP_File_Transfer.h"

#define ADDRESS "127.0.0.1"
#define TFTP_ERROR_PORT 69

void send_error(int sockfd, struct sockaddr_in *client_addr, int error_code,const char *error_message)
{
    unsigned char error_packet[516];
    error_packet[0] = 0; //MSB of opcode
    error_packet[1] = 5; //opcode for ERROR
    error_packet[2] = (error_code >> 8) & 0xFF;
    error_packet[3] = error_code & 0xFF;

    strncpy((char *)&error_packet[4], error_message, 512);
    error_packet[4 + strlen(error_message)] = '\0'; //Null terminator for the error message

    if (sendto(sockfd, error_packet, sizeof(error_packet), 0, (struct sockaddr *)client_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Failed to send error message\n");
    }
}

