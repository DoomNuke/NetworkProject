//
// Created by doomz on 4/19/25.
//

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "UDP_Logger.h"

void log_event(const char *message) {
    FILE *log = fopen("server.log", "a");
    if (log) {
        time_t now = time(NULL); //Returns Current Time
        char *timestamp = ctime(&now);
        timestamp[strcspn(timestamp, "\n")] = 0; // remove newline
        fprintf(log, "[%s] %s\n", timestamp, message);
        fclose(log);
    }
}

void get_client_ip(struct sockaddr_in *client, char *ip_str, size_t max_len) {
    inet_ntop(AF_INET, &(client->sin_addr), ip_str, max_len);
}