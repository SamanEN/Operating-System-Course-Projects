#ifndef SELLER_H
#define SELLER_H

#include "sys/select.h"

#define ERR_BUF_SIZE 256

extern const char* sellers_broadcast_port;

struct SellerAdInfo {
    int id;
    char* title;
    char* desc;
    enum ad_stat_t { waiting,
                     in_progress,
                     expired } status;
    char* port;
    char* unique_id;
    int sock_fd;

    struct SellerOffer* offer;

    struct SellerAdInfo* next;
    struct SellerAdInfo* prev;
};

struct SellerOffer {
    int id;
    struct SellerAdInfo* related_ad;
    char* offer;
    char* offer_msg;
    int client_fd;

    struct SellerOffer* next;
    struct SellerOffer* prev;
};

struct SellerInfo {
    char* name;

    struct SellerAdslist {
        struct SellerAdInfo* begin;
        struct SellerAdInfo* end;
    } ads_list;

    struct OffersList {
        struct SellerOffer* begin;
        struct SellerOffer* end;
    } offers_list;
    
    fd_set open_fds;
    int open_fds_max;
};


#endif
