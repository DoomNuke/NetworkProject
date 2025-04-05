cmake_minimum_required(VERSION 3.31)
project(Networking_Project C)

set(CMAKE_C_STANDARD 11)

add_executable(Networking_Project TFTP_Server.c
        TFTP_Client.c
        UDP_File_Transfer.h
        error_handling_TFTP.c)
