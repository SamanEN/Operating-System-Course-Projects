#include "consts.h"

const char* INVALID_COMMAND_FORMAT =
    "Invalid command format:\n"
    "   \"%s\" requires %d number of arg(s), You entered %d arg(s).\n";

const char* UNKNOWN_COMMAND =
    "Unknown command.\n";


const char* GETADDR_FAILED =
    "getaddrinfo failed.\n";

const char* SOCKET_ERR =
    "socket failed.\n";

const char* BIND_ERR =
    "bind failed.\n";

const char* UDP_BIND_FAILED =
    "udp client failed to bind socket.\n";

const char* UDP_RECV_FAILED =
    "recieving udp message failed.\n";

const char* BROADCAST_FAILED =
    "sending broadcast failed.\n";

const char* INVALID_AD_OFFER =
    "selected ad id is either in porgress, expired or doesn't exist.\n";


const char* INVALID_OFFER_ID =
    "offer not found.\n";


const char* ACCEPT_OFFER_MSG =
    "/accept/";

const char* DECLINE_OFFER_MSG =
    "/decline/";


const char* WAITING_STAT_STR =
    "waiting";

const char* IN_PROG_STAT_STR =
    "in progress";

const char* EXPIRED_STAT_STR =
    "expired";

const char* TIME_OUT_MSG =
    "time out";
