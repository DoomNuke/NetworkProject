//
// Created by doomz on 4/15/25.
//

#ifndef TFTP_SERVER_H
#define TFTP_SERVER_H


#include "UDP_Utils.h"

void start_tftp_server();
void ensure_directory_exists(const char *dir);

#endif //TFTP_SERVER_H
