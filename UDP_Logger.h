//
// Created by doomz on 4/19/25.
//

#ifndef UDP_LOGGER_H
#define UDP_LOGGER_H

void log_event(const char *message);
void get_client_ip(struct sockaddr_in *client, char *ip_str, size_t max_len);

#endif //UDP_LOGGER_H
