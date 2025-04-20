//
// Created by doomz on 4/15/25.
//

#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include "UDP_Utils.h"


//The Operations Handler

void tftp_rrq(const char *ip, const char *filename,const TftpConfig *config);
void tftp_wrq(const char *ip, const char *filename,const TftpConfig *config);
void tftp_delete(const char *ip, const char *filename,const TftpConfig *config);



#endif //TFTP_CLIENT_H
