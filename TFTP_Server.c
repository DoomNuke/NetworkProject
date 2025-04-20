//
// Created by doomz on 3/18/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "TFTP_Server.h"
#include "UDP_Logger.h"

volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    (void) sig; //To Avoid Param
    running = 0;
    printf("\n[SERVER] shut down...\n");
}

//Checks if the root directory exists
void dir_exists(const char *dir) {
    if (access(dir, F_OK) != 0) {
        printf("Creating %s ...\n", dir);
        if (mkdir(dir, 0777) != 0) {
            perror("Creating Directory Failed");
            exit(1);
        }
    }
}

void send_ack(int sockfd, struct sockaddr_in *client, socklen_t client_len, uint16_t block) {
    unsigned char ack[4] = {0};

    ack[0] = 0;
    ack[1] = OPCODE_ACK;
    ack[2] = (uint8_t)(block >> 8);
    ack[3] = (uint8_t)(block & 0xFF);

    sendto(sockfd, ack, 4, 0, (struct sockaddr *)client, client_len);
}

void handle_rrq(int sockfd, struct sockaddr_in *client, socklen_t client_len, const char *filename, const TftpConfig *config) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", config->root_folder, filename);
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("File %s/%s doesn't exist...", config->root_folder, filename);
        return;
    }

    //LOGGER - Logging the RRQ requests
    char client_ip[INET_ADDRSTRLEN];
    get_client_ip(client, client_ip, sizeof(client_ip));
    char log_msg[300];
    snprintf(log_msg, sizeof(log_msg), "RRQ from %s for file: %s", client_ip, filename);
    log_event(log_msg);

    unsigned char buffer[config->buffer_size];
    uint16_t block = 1;
    size_t bytes;

    do {
        buffer[0] = 0;
        buffer[1] = OPCODE_DATA;
        buffer[2] = (unsigned char)block >> 8;
        buffer[3] = (unsigned char)block & 0xFF;

        bytes = fread(&buffer[4], 1, config->data_size, file);
        sendto(sockfd, buffer, bytes + 4, 0, (struct sockaddr *)client, client_len);

        recvfrom(sockfd, buffer, config->buffer_size, 0, (struct sockaddr *) client, &client_len);
        block++;
    } while (bytes == config->data_size);

    fclose(file);
}

void handle_wrq(int sockfd, struct sockaddr_in *client, socklen_t client_len,const char *filename, const TftpConfig *config) {
    char path[256], buffer[config->buffer_size];
    snprintf(path, sizeof(path), "%s/%s", config->root_folder, filename);
    FILE *file = fopen(path, "wb");
    if (!file) {
        printf("File %s/%s could not be created", config->root_folder,filename);
        return;
    }

    //LOGGER - Logging the WRQ Requests
    char client_ip[INET_ADDRSTRLEN];
    get_client_ip(client, client_ip, sizeof(client_ip));
    char log_msg[300];
    snprintf(log_msg, sizeof(log_msg), "WRQ from %s to upload file: %s", client_ip, filename);
    log_event(log_msg);

    send_ack(sockfd, client, client_len, 0);
    uint16_t expected = 1;

    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, config->buffer_size, 0, (struct sockaddr *)client, &client_len);
        if (buffer[1] == OPCODE_DATA) {
            uint16_t block = (buffer[2] << 8 | buffer[3] );
            if (block == expected) {
                fwrite(&buffer[4], 1, n - 4, file);
                send_ack(sockfd, client, client_len, block);
                expected++;
                if (n < config->buffer_size) break;
            }
        }
    }

    fclose(file);
 }

void handle_delete(int sockfd, struct sockaddr_in *client, socklen_t client_len, const char *filename,const TftpConfig *config) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", config->root_folder, filename);
    char client_ip[INET_ADDRSTRLEN];
    get_client_ip(client, client_ip, sizeof(client_ip));


    if (remove(path) == 0) {
        send_ack(sockfd, client, client_len, 0);
        printf("Removed the file successfully\n");
        //LOGGER - For Delete Operation
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "DELETE from %s for file: %s", client_ip, filename);
        log_event(log_msg);
    }
    else {
        printf("Failed to delete the file\n");
    }
}


void start_server() {
    const TftpConfig *config = &default_tftp_config;

    dir_exists(config->root_folder);
    //If the server hits control+c it exits
    signal(SIGINT, sigint_handler);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server = {0}, client;
    server.sin_family = AF_INET;
    server.sin_port = htons(config->port);
    server.sin_addr.s_addr = INADDR_ANY; //Accepts any incoming connections
    socklen_t addr_len = sizeof(server);
    if (bind(sockfd, (struct sockaddr *)&server, addr_len) < 0) {
        perror("Binding Failed");
        exit(EXIT_FAILURE);
    }

    printf("TFTP Server started on port %d\n", config->port);
    log_event("Server started successfully");

    char buffer[config->buffer_size];
    socklen_t client_len = sizeof(client);

    while (running) {
        ssize_t n = recvfrom (sockfd, buffer, config->buffer_size, MSG_DONTWAIT, (struct sockaddr *)&client, &client_len); //Running on a non blocking state
        if (n <= 0) {
            usleep(25000); //Waits for 2.5 ms to avoid cpu overload
            continue;
        }

        char *filename = &buffer[2];

        switch (buffer[1]) {
            case OPCODE_RRQ:
                handle_rrq(sockfd, &client, client_len, filename, config);
                break;
            case OPCODE_WRQ:
                handle_wrq(sockfd, &client, client_len, filename, config);
                break;
            case OPCODE_DELETE:
                handle_delete(sockfd, &client, client_len, filename, config);
                break;
            default:
                printf("Unknown opcode\n");
        }
    }
    close(sockfd);
    log_event("Server shut down");
    printf("[SERVER] socket closed, Exiting...\n");
}

int main() {
    printf("[SERVER] Initializing...\n");
    start_server();
    printf("[SERVER] Terminated.\n");
    return 0;
}