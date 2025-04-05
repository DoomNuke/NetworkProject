//
// Created by doomz on 3/18/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "UDP_File_Transfer.h"

#define SERVER_PORT 69
#define SERVER_ADDRESS "127.0.0.1"
#define MAX_RETRIES 5


volatile sig_atomic_t running = true; // An Un-interruptable atomic type or in other words "An invisible object"


void handle_signal(int signal) {
    running = false; //Setting Running To False When Receiving A Signal
    printf("\nStopping server...\n");
    fflush(stdout);
}



void handle_request(int sockfd, struct sockaddr_in client_addr) {
    Packet packet;
    socklen_t client_addr_len = sizeof(client_addr);
    FILE *file = NULL;
    int retries = 0; //Number Of Retries
    uint16_t block_n = 1; //Starting Data Packets Number With 1


    //Receive Packets Of Data

    recvfrom(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, &client_addr_len);

    //Acknowledgment
    Packet ack_packet; //Acknowledgment Packet
    ack_packet.oper = ACK;
    ack_packet.ack = 0; //Starting the ACK with 0
    ack_packet.block_n = block_n;



    switch (packet.oper){
        case RRQ:
        { //Reading From a File
            printf("Reading Request For The File Named: %s\n", packet.filename);
            //To Read In Binary Mode
            file = fopen(packet.filename, "rb");
            if (file == NULL) {
                send_error(sockfd, &client_addr, 1, "File not found\n");
                break;
            }
                    size_t bytesRead;
                    while ((bytesRead = fread(packet.data, 1, BUFFER, file)) > 0)
                        {
                            retries = 0; // Resetting retries with each send
                            packet.block_n = block_n;
                            printf("Read %zu bytes from file\n", bytesRead);
                            while (retries < MAX_RETRIES) {
                                if (sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, client_addr_len) < sizeof(Packet)) {
                                    printf("Failed to send the complete packet... Retrying...\n");
                                    retries++;
                                    continue;
                                }

                               const ssize_t ack_size = recvfrom(sockfd, &ack_packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, &client_addr_len);

                                if (ack_size > 0 && ack_packet.oper == ACK && ack_packet.block_n == block_n ) {
                                    printf("Received ACK for block: %d\n", block_n);
                                    block_n++;
                                    break;
                                }
                                else {
                                    printf("No ACK Received, retrying... %d\n", retries);
                                    sleep(5);
                                    retries++;
                                }

                                if (retries == MAX_RETRIES) {
                                    printf("Max retries reached, exiting...\n");
                                    fclose(file);
                                    return;
                                }
                            }
                    fclose(file);
                  }
            break;
        }
        case WRQ: {
            //Creating/Appending a file
            printf("Writing Request For The File Named: %s\n", packet.filename);
            //Appending is the cause of creation of files and appending to the last line of the requested line
            file = fopen(packet.filename, "wb");
            if (file == NULL) {
                send_error(sockfd, &client_addr, 2, "Unable To Open File For Writing\n");
            }
            else {

                block_n = 1; //Starting Block Number
                while (1){
                    retries = 0;

                    const ssize_t data_size = recvfrom(sockfd, &ack_packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, &client_addr_len);

                    if (data_size < sizeof(Packet)) {
                        printf("Failed To Receive complete packet... retrying...\n");
                        retries++;
                        continue;
                    }

                   const size_t bytesWritten = fwrite(packet.data, 1, data_size, file);
                    if (bytesWritten < bytesWritten - sizeof(Packet)) {
                        printf("Failed writing to file... Retrying...\n");
                        retries++;
                        continue;
                    }

                    packet.oper = ACK; //Acknowledge operation enum
                    packet.block_n = block_n; //Block Number

                    while (retries < MAX_RETRIES) {
                        if (sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, client_addr_len) < sizeof(Packet)) {
                            printf("Failed to send ACK... Retrying...\n");
                            retries++;
                            continue;
                        }

                        if (data_size < BUFFER) {
                            printf("All data received, write complete\n");
                            fclose(file);
                            return;
                        }

                        block_n++;
                        break;
                    }

                    if (retries == MAX_RETRIES) {
                        printf("Max retries reached, exiting...\n");
                        fclose(file);
                        return;
                    }
                }
            }
            break;
        }


        case DEL:
        {
        //Deleting a file

        printf("Deleting Request For The File Named: %s\n", packet.filename);

        if (access(packet.filename, F_OK) != 0) //Checking if the file exist and has the right permissions
        {
            send_error(sockfd, &client_addr, 1, "File not found\n");
            break;
        }
            if (remove(packet.filename) == 0) {
                printf("File has %s has been deleted successfully\n", packet.filename);


                packet.oper = ACK;
                packet.block_n = block_n;

                if (sendto(sockfd, &packet, sizeof(Packet), 0, (struct sockaddr*)&client_addr, client_addr_len) < sizeof(Packet)) {
                    printf("Failed to send ACK after deleting the file...\n");
                }
            }
            else {
                    send_error(sockfd, &client_addr, 2, "Unable to delete file\n");
                }
             break;
        }

        default:
            printf("Unknown Operation\n");
            break;
    }
}

    int main() {

        struct sockaddr_in server_addr;

    // Set up signal handling

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask); //Clearing all signals
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL); // Handle Control+C

        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("Socket Creation Failed\n");
            exit(EXIT_FAILURE);
        }

        //Structure Of The Server

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
        server_addr.sin_port = htons(SERVER_PORT);

        if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Binding Failed\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

    printf("Server is running on port %d, Press Control+C To Stop\n", SERVER_PORT);

    while (running) {
        handle_request(sockfd, server_addr);
    }

    printf("Server Shutting Down...\n");
    close(sockfd);


    return 0;
}
