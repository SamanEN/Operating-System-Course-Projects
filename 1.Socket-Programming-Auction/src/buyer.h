#ifndef BUYER_H
#define BUYER_H

#define ERR_BUF_SIZE 256
#define OFFER_TIME 10

#include <sys/types.h>

struct BuyerAdInfo {
    int id;
    char* msg_body;
    char* port;
    enum ad_stat_t { waiting,
                     in_progress,
                     expired } status;
    char* unique_id;

    struct BuyerAdInfo* next;
    struct BuyerAdInfo* prev;
};

struct BuyerOffer {
    char* offer;
    struct BuyerAdInfo* related_ad;
};

struct BuyerInfo {
    char* name;
    char* listen_port_;
    int broadcast_fd;

    struct BuyerAdsList {
        struct BuyerAdInfo* begin;
        struct BuyerAdInfo* end;
    } ads_list;

    struct ReadFDs {
        fd_set fds;
        int max_fd;
    } read_fds;
};

#endif
