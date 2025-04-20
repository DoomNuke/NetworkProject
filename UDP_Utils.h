//
// Created by doomz on 3/18/25.
//

#ifndef UDP_FILE_TRANSFER_H
#define UDP_FILE_TRANSFER_H

#include <stdint.h>

#define PORT 6969
#define BUFFER_SIZE 516
#define DATA_SIZE 512
#define TFTP_ROOT_FOLDER "./tftp_root"

typedef enum {
    OPCODE_RRQ    = 1, //Read Operation
    OPCODE_WRQ    = 2, //Write Operation
    OPCODE_DATA   = 3, //
    OPCODE_ACK    = 4, //Acknowledge
    OPCODE_ERROR  = 5, //Error
    OPCODE_DELETE = 9  // Custom
} TftpOpcode;

typedef struct {
    uint16_t port;
    int buffer_size;
    int data_size;
    const char *root_folder;
} TftpConfig;

static const TftpConfig default_tftp_config = {
    .port = PORT,
    .buffer_size = BUFFER_SIZE,
    .data_size = DATA_SIZE,
    .root_folder = TFTP_ROOT_FOLDER
};

#endif //UDP_FILE_TRANSFER_H
