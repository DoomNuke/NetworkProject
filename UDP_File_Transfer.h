//
// Created by doomz on 3/18/25.
//

#ifndef UDP_FILE_TRANSFER_H
#define UDP_FILE_TRANSFER_H



#define FILE_NAME_LEN 60
#define BUFFER 1024

typedef enum operate {
    RRQ = 10, //Read Request
    WRQ = 20, //Write Request
    DEL = 30, //Delete Request
    ACK = 5 //Acknowledgement
} Operation;


 typedef struct Packet{
    enum operate oper;
    uint16_t ack;
    uint16_t block_n;
    char filename[FILE_NAME_LEN];
    char data[BUFFER];
}Packet;

void handle_request(int sockfd, struct sockaddr_in client_addr);
void handle_signal(int signal);
void send_error(int sockfd, struct sockaddr_in *client_addr, int error_code,const char *error_message);

#endif //UDP_FILE_TRANSFER_H
