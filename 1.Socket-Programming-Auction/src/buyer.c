#include "buyer.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "consts.h"
#include "tcp.h"
#include "udp.h"
#include "utils.h"

static struct BuyerInfo buyer_info;

static const char* EXIT_CMD = "exit";

static const char* HELP_CMD = "help";

static const char* OFFER_CMD = "put_offer";
static const int OFFER_ARGC = 3;

static const char* SHOW_LIST_CMD = "show_ads";
static const int SHOW_LIST_ARGC = 1;
static const int SHOW_LIST_MAX_ARGC = 2;

// ad related tools

static enum ad_stat_t str_to_ad_stat(const char* str_stat) {
    if (!strcmp(str_stat, WAITING_STAT_STR))
        return waiting;

    if (!strcmp(str_stat, IN_PROG_STAT_STR))
        return in_progress;

    return expired;
}

static struct BuyerAdInfo* str_to_ad_info(const char* ad) {
    char* found_token;
    struct BuyerAdInfo* result = (struct BuyerAdInfo*)malloc(sizeof(struct BuyerAdInfo));
    result->msg_body = (char*)calloc(strlen(ad) + 1, sizeof(char));
    memcpy(result->msg_body, ad, strlen(ad));
    const char* status_token = "status : ";
    if ((found_token = (char*)strstr(ad, status_token)) != NULL) {
        char* status_str = getline_str(found_token + strlen(status_token));
        result->status = str_to_ad_stat(status_str);
        free(status_str);
    }
    const char* port_token = "port : ";
    if ((found_token = (char*)strstr(ad, port_token)) != NULL) {
        char* port_str = getline_str(found_token + strlen(port_token));
        result->port = port_str;
    }
    const char* unique_id_token = "unique_id : ";
    if ((found_token = (char*)strstr(ad, unique_id_token)) != NULL) {
        char* unique_id_str = getline_str(found_token + strlen(unique_id_token));
        result->unique_id = unique_id_str;
    }
    return result;
}

static struct BuyerAdInfo* find_ad(const int id) {
    struct BuyerAdInfo* result = NULL;
    struct BuyerAdInfo* current_ad = buyer_info.ads_list.begin;
    for (; current_ad != NULL; current_ad = current_ad->next)
        if (current_ad->id == id) {
            result = current_ad;
            break;
        }

    return result;
}

static struct BuyerAdInfo* find_ad_by_unique_id(const char* unique_id) {
    struct BuyerAdInfo* result = NULL;
    struct BuyerAdInfo* current_ad = buyer_info.ads_list.begin;
    for (; current_ad != NULL; current_ad = current_ad->next)
        if (!strcmp(current_ad->unique_id, unique_id)) {
            result = current_ad;
            break;
        }

    return result;
}

static void push_ad_to_list(struct BuyerAdInfo* ad) {
    if (buyer_info.ads_list.begin == NULL) {
        buyer_info.ads_list.begin = ad;
        buyer_info.ads_list.end = ad;
        ad->prev = NULL;
        ad->id = 0;
    }
    else {
        buyer_info.ads_list.end->next = ad;
        ad->prev = buyer_info.ads_list.end;
        buyer_info.ads_list.end = ad;
        ad->id = ad->prev->id + 1;
    }
    ad->next = NULL;
}

static void rm_ad(struct BuyerAdInfo* ad) {
    if (buyer_info.ads_list.begin == ad)
        buyer_info.ads_list.begin = ad->next;
    if (buyer_info.ads_list.end == ad)
        buyer_info.ads_list.end = ad->prev;

    if (ad->prev != NULL)
        ad->prev->next = ad->next;

    if (ad->next != NULL)
        ad->next->prev = ad->prev;

    free(ad->msg_body);
    free(ad->port);
    free(ad->unique_id);
    free(ad);
}

static void replace_ad(struct BuyerAdInfo* new, struct BuyerAdInfo* old) {
    free(old->msg_body);
    free(old->port);
    free(old->unique_id);

    new->next = old->next;
    new->prev = old->prev;
    new->id = old->id;

    if (old->prev != NULL)
        old->prev->next = new;
    if (old->next != NULL)
        old->next->prev = new;

    if (buyer_info.ads_list.begin == old)
        buyer_info.ads_list.begin = new;
    if (buyer_info.ads_list.end == old)
        buyer_info.ads_list.end = new;

    free(old);
}

static void update_ads_list(struct BuyerAdInfo* new_ad) {
    struct BuyerAdInfo* found_ad = find_ad_by_unique_id(new_ad->unique_id);
    if (found_ad != NULL) {
        replace_ad(new_ad, found_ad);
        if (new_ad->status == expired)
            rm_ad(new_ad);
        return;
    }
    push_ad_to_list(new_ad);
}

static void recieve_broadcast() {
    char* udp_msg = rcv_udp(buyer_info.broadcast_fd);
    struct BuyerAdInfo* new_ad = str_to_ad_info(udp_msg);
    free(udp_msg);
    update_ads_list(new_ad);
}

// offer command tools

static char* offer_to_str(struct BuyerOffer* offer) {
    const char* offer_str_form =
        "ad unique id : %s\n"
        "offer : %s\n";

    int offer_str_len =
        strlen(offer_str_form) +
        strlen(offer->related_ad->unique_id) +
        strlen(offer->offer) -
        4 + 1; // +1 for '\0' char

    char* offer_str = (char*)calloc(offer_str_len, sizeof(char));
    sprintf(offer_str, offer_str_form, offer->related_ad->unique_id, offer->offer);
    return offer_str;
}

static void log_offer_result(const char* result) {
    char* msg;
    if (result == NULL) {
        log_info(STDOUT_FILENO, "Offer timed out.\n", 1, 1);
        return;
    }
    if (strstr(result, ACCEPT_OFFER_MSG) != NULL) {
        const char* msg_form =
            "Successfuly bought %s.\n";

        int msg_len = strlen(msg_form) + strlen(result) - strlen(ACCEPT_OFFER_MSG) - 2 + 1;
        msg = (char*)calloc(msg_len, sizeof(char));
        sprintf(msg, msg_form, result + strlen(ACCEPT_OFFER_MSG));
    }

    else {
        const char* msg_form =
            "Offer for %s was declined.\n";
        int msg_len = strlen(msg_form) + strlen(result) - strlen(DECLINE_OFFER_MSG) - 2 + 1;
        msg = (char*)calloc(msg_len, sizeof(char));
        sprintf(msg, msg_form, result + strlen(DECLINE_OFFER_MSG));
    }
    log_info(STDOUT_FILENO, msg, 1, 1);
    free(msg);
}

void signal_handler(int signal) {
    return;
}

static void put_offer(const int id, const char* offer) {
    struct BuyerAdInfo* found_ad = find_ad(id);
    if (found_ad == NULL) {
        log_err(STDERR_FILENO, INVALID_AD_OFFER, 1, 1);
        return;
    }

    if (found_ad->status != waiting) {
        log_err(STDERR_FILENO, INVALID_AD_OFFER, 1, 1);
        return;
    }

    struct BuyerOffer* offer_struct = (struct BuyerOffer*)malloc(sizeof(struct BuyerOffer));
    offer_struct->offer = (char*)calloc(strlen(offer) + 1, sizeof(char));
    memcpy(offer_struct->offer, offer, strlen(offer));
    offer_struct->related_ad = found_ad;
    char* offer_str = offer_to_str(offer_struct);
    free(offer_struct->offer);
    free(offer_struct);

    int sock_fd = connect_tcp_client(strtol(found_ad->port, (char**)NULL, 0));
    if (sock_fd == -1)
        log_err(STDERR_FILENO, "Couldn't connect to seller's server.\n", 1, 1);

    send_tcp_msg(sock_fd, offer_str, 10);
    free(offer_str);

    signal(SIGALRM, signal_handler);
    siginterrupt(SIGALRM, 1);
    alarm(OFFER_TIME);
    char* result = receive_tcp(sock_fd);
    alarm(0);

    if (result == NULL) {
        const char* time_out_msg_form = "%s/ad unique id : %s";
        int time_out_msg_len = strlen(time_out_msg_form) + strlen(found_ad->unique_id) + strlen(TIME_OUT_MSG) - 4 + 1;
        char* time_out_msg = (char*)calloc(time_out_msg_len, sizeof(char));
        sprintf(time_out_msg, time_out_msg_form, TIME_OUT_MSG, found_ad->unique_id);
        send_tcp_msg(sock_fd, time_out_msg, 10);
        free(time_out_msg);
    }

    close(sock_fd);
    log_offer_result(result);
    free(result);
}

// show list command tools

static char* ad_to_str(const struct BuyerAdInfo* ad) {
    const char* ad_str_form =
        "\nad id : %s\n"
        "%s\n";

    char id_buf[16] = {0};
    sprintf(id_buf, "%d", ad->id);
    char* id_str = (char*)calloc(strlen(id_buf) + 1, sizeof(char));
    memcpy(id_str, id_buf, strlen(id_buf));

    int ad_str_len =
        strlen(ad_str_form) +
        strlen(id_str) +
        strlen(ad->msg_body) -
        4 + 1; //+1 for '\0' char

    char* ad_str = (char*)calloc(ad_str_len, sizeof(char));
    sprintf(ad_str, ad_str_form, id_str, ad->msg_body);

    free(id_str);

    return ad_str;
}

static void show_ads(int n) {
    struct BuyerAdInfo* current_ad = buyer_info.ads_list.begin;
    if (current_ad == NULL) {
        const char* no_ad_msg = "\nNo ad to show.\n\n";
        write(STDOUT_FILENO, no_ad_msg, strlen(no_ad_msg));
        return;
    }
    for (int i = 0; (i < n || n == -1) && current_ad != NULL; ++i) {
        char* ad_str = ad_to_str(current_ad);
        write(STDOUT_FILENO, ad_str, strlen(ad_str));
        free(ad_str);
        current_ad = current_ad->next;
    }
}

static void print_options() {
    const char* BUYER_OPTS_MENU =
        "Options:\n"
        "   put_offer [id] [offer]\n"
        "   show_ads [n]{optional}\n";

    write(STDOUT_FILENO, BUYER_OPTS_MENU, strlen(BUYER_OPTS_MENU));
}

static int execute_command(const ParsedLine command) {
    if (command.argc == 0)
        return 0;

    char err_buf[ERR_BUF_SIZE] = {0};

    if (!strcmp(command.argv[0], EXIT_CMD))
        return -1;
    if (!strcmp(command.argv[0], HELP_CMD)) {
        print_options();
    }
    else if (!strcmp(command.argv[0], OFFER_CMD)) {
        if (command.argc != OFFER_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, OFFER_CMD, OFFER_ARGC, command.argc - 1);
        }
        else {
            int id = strtol(command.argv[1], (char**)NULL, 0);
            put_offer(id, command.argv[2]);
        }
    }
    else if (!strcmp(command.argv[0], SHOW_LIST_CMD)) {
        if (command.argc < SHOW_LIST_ARGC || command.argc > SHOW_LIST_MAX_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, OFFER_CMD, OFFER_ARGC, command.argc - 1);
        }
        else {
            int n = -1;
            if (command.argc > SHOW_LIST_ARGC)
                n = strtol(command.argv[1], (char**)NULL, 0);
            show_ads(n);
        }
    }
    else {
        strcpy(err_buf, UNKNOWN_COMMAND);
    }

    if (strlen(err_buf) != 0)
        log_err(STDERR_FILENO, err_buf, 1, 1);

    return 0;
}

static int cmd_interface() {
    char* input_line = read_line(STDIN_FILENO, -1);
    ParsedLine parsed_line = parse_line(input_line, " \n\t");
    if (execute_command(parsed_line) != 0) {
        free(input_line);
        return 0;
    }
    free(input_line);
    return 1;
}

static void get_info() {
    const char* name_msg = "Enter your name:\n";
    write(STDOUT_FILENO, name_msg, strlen(name_msg));
    buyer_info.name = read_line(STDIN_FILENO, -1);
    if (buyer_info.listen_port_ == NULL) {
        const char* listen_port_msg = "Enter listen port:\n";
        write(STDOUT_FILENO, listen_port_msg, strlen(listen_port_msg));
        buyer_info.listen_port_ = read_line(STDIN_FILENO, -1);
    }
}

static void init_vars() {
    buyer_info.broadcast_fd = connect_udp(strtol(buyer_info.listen_port_, (char**)NULL, 0));
    FD_ZERO(&buyer_info.read_fds.fds);
    FD_SET(STDIN_FILENO, &buyer_info.read_fds.fds);
    FD_SET(buyer_info.broadcast_fd, &buyer_info.read_fds.fds);
    buyer_info.read_fds.max_fd =
        (STDIN_FILENO > buyer_info.broadcast_fd) ? STDIN_FILENO : buyer_info.broadcast_fd;

    buyer_info.ads_list.begin = NULL;
    buyer_info.ads_list.end = NULL;
}

int main(int argc, char const* argv[]) {
    get_info();
    init_vars();
    for (;;) {
        fd_set working_set = buyer_info.read_fds.fds;
        int ready_fd_count = select(buyer_info.read_fds.max_fd + 1, &working_set, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &working_set)) {
            --ready_fd_count;
            if (!cmd_interface())
                break;
        }
        if (ready_fd_count > 0) {
            recieve_broadcast();
        }
    }
    free(buyer_info.name);
    free(buyer_info.listen_port_);
}
