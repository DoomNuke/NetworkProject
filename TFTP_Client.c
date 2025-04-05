//
// Created by doomz on 3/18/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>

#include "UDP_File_Transfer.h"  // Ensure this header defines Packet, Operation, etc.

#define CLIENT_ADDRESS "127.0.0.1"
#define CLIENT_PORT 69
#define MAX_RETRIES 5

volatile sig_atomic_t running = true; // An Un-interruptable atomic type or in other words "An invisible object"

void handle_signal(int signal) {
    running = false; //Setting Running To False When Receiving A Signal
    printf("\nStopping server...\n");
    fflush(stdout);
}



void send_packet(int sockfd, struct sockaddr_in *server_addr, Packet *packet) {
    socklen_t addr_len = sizeof(*server_addr);
    if (sendto(sockfd, packet, sizeof(Packet), 0, (struct sockaddr *)server_addr, addr_len) < sizeof(Packet)) {
        perror("Failed to send packet");
        exit(EXIT_FAILURE);
    }
}

void receive_response(int sockfd, Packet *packet) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    ssize_t recv_len = recvfrom(sockfd, packet, sizeof(Packet), 0, NULL, &addr_len);
    if (recv_len < 0) {
        perror("Failed to receive response");
        exit(EXIT_FAILURE);
    }
}

void handle_read(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    Packet packet = {0};
    packet.oper = RRQ;  // RRQ operation
    strncpy(packet.filename, filename, FILE_NAME_LEN);

    // send the read request
    send_packet(sockfd, server_addr, &packet);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error Reading Local File\n");
        return;
    }

    size_t bytesRead;
    int retries = 0; // setting retries to 0 for each block

    while ((bytesRead = fread(packet.data, 1, BUFFER, file)) > 0) {
        packet.block_n = retries + 1; // increment block number
        send_packet(sockfd, server_addr, &packet);

        // waiting for ack
        receive_response(sockfd, &packet);

        retries++;

        // checking for the EOF
        if (bytesRead < BUFFER) {
            break; // reached EOF
        }
    }

    fclose(file);  // Close the file after reading
    printf("File '%s' sent to server.\n", filename);
}

void handle_write(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    FILE *file = fopen(filename, "wb");  // Open the file to write received data
    if (!file) {
        perror("Error: Couldn't create the file");
        return;
    }

    Packet packet = {0};
    packet.oper = WRQ;
    strncpy(packet.filename, filename, FILE_NAME_LEN);

    // Send WRQ to server
    send_packet(sockfd, server_addr, &packet);

    ssize_t recv_len;  // storing the data

    while ((recv_len = recvfrom(sockfd, &packet, sizeof(Packet), 0, NULL, NULL)) > 0) {
        // write the data into the file
        fwrite(packet.data, 1, recv_len - offsetof(Packet, data), file);

        printf("Received block %d with %zu bytes\n", packet.block_n, recv_len - offsetof(Packet, data));

        if (recv_len < sizeof(Packet)) {
            // EOF
            break;
        }
    }
    fclose(file);
    printf("File '%s' sent to server.\n", filename);
}

void handle_delete(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    Packet packet = {0};
    packet.oper = DEL;
    strncpy(packet.filename, filename, FILE_NAME_LEN);

    send_packet(sockfd, server_addr, &packet);
    Packet ack_packet = {0}; //Waiting for ack from server
    receive_response(sockfd, &packet);

    //checking if recived
    if (ack_packet.oper == ACK) {
        printf("Server Response: %s\n", ack_packet.data);  // Print the server's message (success or error)
    } else {
        printf("Error, No ACK received from the server\n");
    }
}

int main() {


    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask); //Clearing all signals
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL); // Handle Control+C


    struct sockaddr_in server_addr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CLIENT_PORT);
    inet_pton(AF_INET, CLIENT_ADDRESS, &server_addr.sin_addr);

    int choice;
    char filename[FILE_NAME_LEN];

    printf("UDP File Transfer Client\n");
    printf("1. Read a file from server\n");
    printf("2. Write a file to server\n");
    printf("3. Delete a file on server\n");
    printf("4. Exit\n");

    while (1) {
        printf("\nEnter choice: ");
        scanf("%d", &choice);
        if (choice == 4) break;

        printf("Enter filename: ");
        scanf("%s", filename);

        switch (choice) {
            case 1: handle_read(sockfd, &server_addr, filename); break;
            case 2: handle_write(sockfd, &server_addr, filename); break;
            case 3: handle_delete(sockfd, &server_addr, filename); break;
            default: printf("Invalid choice. Try again.\n"); break;
        }
    }

    close(sockfd);
    printf("Client exited.\n");
    return 0;
}