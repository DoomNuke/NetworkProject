cmake_minimum_required(VERSION 3.31)
project(Networking_Project C)

set(CMAKE_C_STANDARD 11)

add_executable(Networking_Project TFTP_Server.c
        TFTP_Client.c
        UDP_Utils.h
        Error_Handling.c
        TFTP_Server.h
        TFTP_Client.h
        Error_Handling.h
        Utils.c
        Utils.h
        UDP_Logger.c
        UDP_Logger.h)
