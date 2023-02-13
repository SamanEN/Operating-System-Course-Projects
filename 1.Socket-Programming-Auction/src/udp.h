#ifndef BROADCAST_H
#define BROADCAST_H

void broadcast_msg(const char*, const char*);

int connect_udp(const int);

char* rcv_udp(int);

#endif
