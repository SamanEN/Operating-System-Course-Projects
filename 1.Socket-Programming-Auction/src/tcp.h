#ifndef TCP_H
#define TCP_H

int connect_tcp_server(int);

int accept_tcp_client(int);

int connect_tcp_client(int);

int send_tcp_msg(int, const char*, int);

char* receive_tcp(int);

#endif
