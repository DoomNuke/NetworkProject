CC = gcc
CFLAGS = -Wall

COMMON_SRCS = UDP_Logger.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

SERVER_SRCS = TFTP_Server.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_SRCS = TFTP_Client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

all: tftp_server tftp_client

tftp_server: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) -o tftp_server $(SERVER_OBJS) $(COMMON_OBJS)

tftp_client: $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) -o tftp_client $(CLIENT_OBJS) $(COMMON_OBJS)

clean:
	rm -f *.o tftp_server tftp_client