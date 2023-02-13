#include "seller.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include "consts.h"
#include "tcp.h"
#include "udp.h"
#include "utils.h"

const char* sellers_broadcast_port = "8080";

static struct SellerInfo seller_info;

static const char* EXIT_CMD = "exit";

static const char* HELP_CMD = "help";

static const char* ACCEPT_OFFER_CMD = "accept_offer";
static const int ACCEPT_OFFER_ARGC = 2;

static const char* DECLINE_OFFER_CMD = "decline_offer";
static const int DECLINE_OFFER_ARGC = 2;

static const char* BROADCAST_ADV_CMD = "broadcast";
static const int BROADCAST_ADV_ARGC = 4;

static const char* SHOW_OFFERS_CMD = "show_offers";
static const int SHOW_OFFERS_ARGC = 1;

static void log_seller_info(const char* msg) {
    const char* file_path_form = "./%slog.txt";
    int file_path_len = strlen(seller_info.name) + strlen(file_path_form) - 2 + 1;
    char* file_path_str = (char*)calloc(file_path_len, sizeof(char));
    sprintf(file_path_str, file_path_form, seller_info.name);
    int file_fd = open(file_path_str, O_WRONLY | O_APPEND);
    if (file_fd == -1) {
        file_fd = open(file_path_str, O_WRONLY | O_APPEND | O_CREAT);
    }
    free(file_path_str);
    log_info(file_fd, msg, 1, 1);
    close(file_fd);
}

// ad tools

static struct SellerAdInfo* find_ad_by_unique_id(const char* unique_id) {
    struct SellerAdInfo* result = NULL;
    struct SellerAdInfo* current_ad = seller_info.ads_list.begin;
    for (; current_ad != NULL; current_ad = current_ad->next)
        if (!strcmp(current_ad->unique_id, unique_id)) {
            result = current_ad;
            break;
        }

    return result;
}

static const char* ad_status_to_str(const struct SellerAdInfo* ad) {
    switch (ad->status) {
    case waiting:
        return WAITING_STAT_STR;
    case in_progress:
        return IN_PROG_STAT_STR;
    case expired:
        return EXPIRED_STAT_STR;
    default:
        return NULL;
    }
}

static char* seller_ad_to_str(const struct SellerAdInfo* ad) {
    const char* msg_form =
        "%s\n"                // title
        "status : %s\n"       // status
        "seller : %s\n"       // seller name
        "description: \n%s\n" // description
        "port : %s\n"         // port
        "unique_id : %s\n";   // unique id

    int msg_len = strlen(msg_form) - 6; //- 6 for "%s"s in text which are gonna be replaced with some other text
    msg_len += strlen(ad->title);       // title len
    const char* status_str = ad_status_to_str(ad);
    msg_len += strlen(status_str);       // status len
    msg_len += strlen(seller_info.name); // name len
    char* indented_desc = indent_str(ad->desc, '\t', 1);
    msg_len += strlen(indented_desc); // desc len
    msg_len += strlen(ad->port);      // port len
    msg_len += strlen(ad->unique_id);
    msg_len += 1; // for '\0' char

    char* result = (char*)calloc(msg_len, sizeof(char));
    sprintf(result, msg_form, ad->title, status_str, seller_info.name, indented_desc, ad->port, ad->unique_id);
    free(indented_desc);
    return result;
}

static struct SellerAdInfo* make_ad(const char* title, const char* desc, const char* port) {
    struct SellerAdInfo* result = (struct SellerAdInfo*)malloc(sizeof(struct SellerAdInfo));

    result->title = (char*)calloc(strlen(title) + 1, sizeof(char));
    result->desc = (char*)calloc(strlen(desc) + 1, sizeof(char));
    result->port = (char*)calloc(strlen(port) + 1, sizeof(char));

    strcpy(result->title, title);
    strcpy(result->desc, desc);
    strcpy(result->port, port);

    result->id = (seller_info.ads_list.end == NULL) ? 0 : (seller_info.ads_list.end->id + 1);
    char id_buf[16] = {0};
    sprintf(id_buf, "%d", result->id);
    const char* unique_id_form = "{%s}_{%s}_{%s}";
    int unique_id_len = strlen(seller_info.name) + strlen(title) + strlen(id_buf) + strlen(unique_id_form) - 6 + 1;
    result->unique_id = (char*)calloc(unique_id_len, sizeof(char));
    sprintf(result->unique_id, unique_id_form, seller_info.name, title, id_buf);

    result->sock_fd = connect_tcp_server((int)strtol(port, (char**)NULL, 0));
    if (result->sock_fd == -1)
        log_err(STDERR_FILENO, "Ad's server was not connected.\n", 1, 1);

    FD_SET(result->sock_fd, &seller_info.open_fds);
    seller_info.open_fds_max =
        (seller_info.open_fds_max > result->sock_fd) ?
            seller_info.open_fds_max :
            result->sock_fd;

    result->status = waiting;

    return result;
}

static void free_ad(struct SellerAdInfo to_free) {
    free(to_free.desc);
    free(to_free.port);
    free(to_free.title);
    free(to_free.unique_id);
}

// ad command

static void push_ad_to_list(struct SellerAdInfo* ad) {
    if (seller_info.ads_list.begin == NULL) {
        seller_info.ads_list.begin = ad;
        seller_info.ads_list.end = ad;
        ad->prev = NULL;
    }
    else {
        seller_info.ads_list.end->next = ad;
        ad->prev = seller_info.ads_list.end;
        seller_info.ads_list.end = ad;
    }
    ad->next = NULL;
}

static void broadcast_ad(struct SellerAdInfo* ad) {
    char* ad_str = seller_ad_to_str(ad);
    broadcast_msg(sellers_broadcast_port, ad_str);
    free(ad_str);
}

// this function will execute "broadcast" command
static void broadcast(const char* title, const char* desc, const char* port) {
    struct SellerAdInfo* new_ad = make_ad(title, desc, port);
    broadcast_ad(new_ad);
    push_ad_to_list(new_ad);
}

// offer tools

static struct SellerOffer* find_offer(const int id) {
    struct SellerOffer* result = NULL;
    struct SellerOffer* current_offer = seller_info.offers_list.begin;

    for (; current_offer != NULL; current_offer = current_offer->next)
        if (current_offer->id == id) {
            result = current_offer;
            break;
        }

    return result;
}

static void add_new_offer(struct SellerOffer* new_offer) {
    if (seller_info.offers_list.begin == NULL) {
        seller_info.offers_list.begin = new_offer;
        seller_info.offers_list.end = new_offer;
        new_offer->prev = NULL;
    }
    else {
        seller_info.offers_list.end->next = new_offer;
        new_offer->prev = seller_info.offers_list.end;
        seller_info.offers_list.end = new_offer;
    }
    new_offer->next = NULL;
}

static struct SellerOffer* str_to_offer(const char* offer_msg) {
    struct SellerOffer* new_offer = (struct SellerOffer*)malloc(sizeof(struct SellerOffer));
    new_offer->offer_msg = (char*)calloc(strlen(offer_msg) + 1, sizeof(char));
    memcpy(new_offer->offer_msg, offer_msg, strlen(offer_msg));

    char* found_token;

    const char* unique_id_token = "ad unique id : ";
    if ((found_token = (char*)strstr(offer_msg, unique_id_token)) != NULL) {
        char* unique_id = getline_str(found_token + strlen(unique_id_token));
        struct SellerAdInfo* found_ad = find_ad_by_unique_id(unique_id);
        if (found_ad == NULL) {
            log_err(STDERR_FILENO, "The ad that the below offer is refering to doesn't exist:\n", 1, 1);
            log_err(STDERR_FILENO, offer_msg, 0, 0);
            return NULL;
        }
        new_offer->related_ad = found_ad;
    }

    const char* offer_token = "offer : ";
    if ((found_token = (char*)strstr(offer_msg, offer_token)) != NULL)
        new_offer->offer = getline_str(found_token + strlen(offer_token));

    return new_offer;
}

static void rm_offer(struct SellerOffer* offer) {
    if (offer->prev != NULL)
        offer->prev->next = offer->next;
    if (offer->next != NULL)
        offer->next->prev = offer->prev;
    if (offer == seller_info.offers_list.begin)
        seller_info.offers_list.begin = offer->next;
    if (offer == seller_info.offers_list.end)
        seller_info.offers_list.end = offer->prev;

    struct SellerOffer* current_offer = offer->next;
    for (; current_offer != NULL; current_offer = current_offer->next)
        --current_offer->id;

    offer->related_ad->offer = NULL;

    free(offer->offer_msg);
    free(offer);
}

static void update_timed_out_offers(int sock_fd) {
    char* tcp_msg = receive_tcp(sock_fd);
    char* unique_id = get_field_val(tcp_msg, "ad unique id : ");
    struct SellerAdInfo* related_ad = find_ad_by_unique_id(unique_id);
    free(unique_id);

    if (strstr(tcp_msg, TIME_OUT_MSG) != NULL) {
        struct SellerOffer* related_offer = related_ad->offer;
        log_info(STDOUT_FILENO, "The below offer timed out:", 1, 1);
        log_info(STDOUT_FILENO, related_offer->offer_msg, 0, 0);
        rm_offer(related_offer);
        related_ad->status = waiting;
        broadcast_ad(related_ad);
    }
    FD_CLR(sock_fd, &seller_info.open_fds);
}

static void fetch_offer(int server_fd) {
    int client_fd = accept_tcp_client(server_fd);
    char* tcp_msg = receive_tcp(client_fd);
    FD_SET(client_fd, &seller_info.open_fds);
    seller_info.open_fds_max =
        (seller_info.open_fds_max > client_fd) ? seller_info.open_fds_max : client_fd;

    char* unique_id = get_field_val(tcp_msg, "ad unique id : ");
    struct SellerAdInfo* related_ad = find_ad_by_unique_id(unique_id);
    free(unique_id);

    if (related_ad->status != waiting) {
        log_err(
            STDERR_FILENO,
            "The ad that the below offer is referring to is not currently available for offering:\n",
            1,
            1);
        log_err(STDERR_FILENO, tcp_msg, 0, 0);
        free(tcp_msg);
        return;
    }

    struct SellerOffer* new_offer = str_to_offer(tcp_msg);
    new_offer->client_fd = client_fd;
    new_offer->related_ad->offer = new_offer;

    new_offer->id = (seller_info.offers_list.begin == NULL) ?
        0 : seller_info.offers_list.end->id + 1;
    add_new_offer(new_offer);

    related_ad->status = in_progress;

    broadcast_ad(related_ad);

    free(tcp_msg);
}

static void update_offers(fd_set* working_set) {
    struct SellerAdInfo* current_ad = seller_info.ads_list.begin;
    for (; current_ad != NULL; current_ad = current_ad->next) {
        if (current_ad->status != expired && FD_ISSET(current_ad->sock_fd, working_set)) {
            fetch_offer(current_ad->sock_fd);
        }
    }
    struct SellerOffer* current_offer = seller_info.offers_list.begin;
    for(; current_offer != NULL; current_offer = current_offer->next) {
        if (FD_ISSET(current_offer->client_fd, working_set)) {
            update_timed_out_offers(current_offer->client_fd);
        }
    }
}

static void log_offer_acceptance(const struct SellerOffer* offer) {
    const char* log_info_form =
        "Succesfuly sold %s.\n";
    int info_len = strlen(log_info_form) + strlen(offer->related_ad->title) - 2 + 1;
    char* info_str = (char*)calloc(info_len, sizeof(char));
    sprintf(info_str, log_info_form, offer->related_ad->title);
    printf("%s", info_str);
    log_seller_info(info_str);
    free(info_str);
}

static void log_offer_decline(const struct SellerOffer* offer) {
    const char* log_info_form =
        "declined offer for %s.\n";
    int info_len = strlen(log_info_form) + strlen(offer->related_ad->title) - 2 + 1;
    char* info_str = (char*)calloc(info_len, sizeof(char));
    sprintf(info_str, log_info_form, offer->related_ad->title);
    log_seller_info(info_str);
    free(info_str);
}

static char* log_time_out(const struct SellerOffer* offer) {
    const char* log_info_form =
        "Offer for %s timed out.\n";
    int info_len = strlen(log_info_form) + strlen(offer->related_ad->title) - 2 + 1;
    char* info_str = (char*)calloc(info_len, sizeof(char));
    sprintf(info_str, log_info_form, offer->related_ad->title);
    log_seller_info(info_str);
    free(info_str);
}

static char* offer_to_str(const struct SellerOffer* offer) {
    const char* accept_msg_form =
        "%s";

    int accept_msg_len = strlen(offer->related_ad->unique_id) - 2 + 1;
    char* result = (char*)calloc(accept_msg_len, sizeof(char));
    sprintf(result, accept_msg_form, offer->related_ad->unique_id);
    return result;
}

static void accept_offer(const int id) {
    struct SellerOffer* found_offer = find_offer(id);
    if (found_offer == NULL) {
        log_err(STDERR_FILENO, INVALID_OFFER_ID, 1, 1);
        return;
    }

    int sock_fd = found_offer->client_fd;

    char* accept_offer_msg = offer_to_str(found_offer);
    char* temp = accept_offer_msg;
    accept_offer_msg = add_data_header(accept_offer_msg, ACCEPT_OFFER_MSG);
    free(temp);

    if (send_tcp_msg(sock_fd, accept_offer_msg, 10) == -1) {
        log_time_out(found_offer);
        found_offer->related_ad->status = waiting;
        broadcast_ad(found_offer->related_ad);
    }
    else {
        log_offer_acceptance(found_offer);

        found_offer->related_ad->status = expired;
        close(found_offer->client_fd);
        close(found_offer->related_ad->sock_fd);
        FD_CLR(found_offer->client_fd, &seller_info.open_fds);
        FD_CLR(found_offer->related_ad->sock_fd, &seller_info.open_fds);
        broadcast_ad(found_offer->related_ad);
    }

    rm_offer(found_offer);
    free(accept_offer_msg);
}

static void decline_offer(const int id) {
    struct SellerOffer* found_offer = find_offer(id);
    if (found_offer == NULL) {
        log_err(STDERR_FILENO, INVALID_AD_OFFER, 1, 1);
        return;
    }

    int sock_fd = found_offer->client_fd;

    char* decline_offer_msg = offer_to_str(found_offer);
    char* temp = decline_offer_msg;
    decline_offer_msg = add_data_header(decline_offer_msg, DECLINE_OFFER_MSG);
    free(temp);

    if (send_tcp_msg(sock_fd, decline_offer_msg, 10) == -1) {
        log_time_out(found_offer);
    }
    else {
        log_offer_decline(found_offer);
    }
    found_offer->related_ad->status = waiting;
    rm_offer(found_offer);

    free(decline_offer_msg);
}

static void show_offers() {
    struct SellerOffer* current_offer = seller_info.offers_list.begin;
    if (current_offer == NULL) {
        const char* no_offer_msg =
            "\nNo offers to show.\n\n";

        write(STDOUT_FILENO, no_offer_msg, strlen(no_offer_msg));
    }

    for (; current_offer != NULL; current_offer = current_offer->next) {
        char id_buf[16] = {0};
        sprintf(id_buf, "%d", current_offer->id);

        const char* offer_msg_form =
            "\n- offer id : %s\n"
            "ad title : %s\n"
            "ad unique id : %s\n"
            "offer : %s\n";

        int offer_msg_len = strlen(offer_msg_form) - 8 + 1;
        offer_msg_len +=
            strlen(id_buf) +
            strlen(current_offer->related_ad->title) +
            strlen(current_offer->related_ad->unique_id) +
            strlen(current_offer->offer) +
            1;

        char* offer_msg = (char*)calloc(offer_msg_len, sizeof(char));
        sprintf(
            offer_msg,
            offer_msg_form,
            id_buf,
            current_offer->related_ad->title,
            current_offer->related_ad->unique_id,
            current_offer->offer);
        write(STDOUT_FILENO, offer_msg, offer_msg_len);
        free(offer_msg);
    }
}

static void print_options() {
    static const char* BUYER_OPTS_MENU =
        "Options:\n"
        "   accept_offer [id]\n"
        "   decline_offer [id]\n"
        "   broadcast [title] [description] [port number]\n"
        "   show_offers\n";

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
    else if (!strcmp(command.argv[0], ACCEPT_OFFER_CMD)) {
        if (command.argc != ACCEPT_OFFER_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, ACCEPT_OFFER_CMD, ACCEPT_OFFER_ARGC, command.argc - 1);
        }
        else {
            int id = strtol(command.argv[1], (char**)NULL, 0);
            accept_offer(id);
        }
    }
    else if (!strcmp(command.argv[0], DECLINE_OFFER_CMD)) {
        if (command.argc != DECLINE_OFFER_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, DECLINE_OFFER_CMD, DECLINE_OFFER_ARGC, command.argc - 1);
        }
        else {
            int id = strtol(command.argv[1], (char**)NULL, 0);
            decline_offer(id);
        }
    }
    else if (!strcmp(command.argv[0], BROADCAST_ADV_CMD)) {
        if (command.argc != BROADCAST_ADV_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, BROADCAST_ADV_CMD, BROADCAST_ADV_ARGC, command.argc - 1);
        }
        else {
            broadcast(command.argv[1], command.argv[2], command.argv[3]);
        }
    }
    else if (!strcmp(command.argv[0], SHOW_OFFERS_CMD)) {
        if (command.argc != SHOW_OFFERS_ARGC) {
            sprintf(err_buf, INVALID_COMMAND_FORMAT, SHOW_OFFERS_CMD, SHOW_OFFERS_ARGC, command.argc - 1);
        }
        else {
            show_offers();
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
    ParsedLine parsed_line = parse_line(input_line, " \n\t\0");
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
    seller_info.name = read_line(STDIN_FILENO, -1);
}

static void init_vars() {
    seller_info.ads_list.begin = NULL;
    seller_info.ads_list.end = NULL;

    seller_info.offers_list.begin = NULL;
    seller_info.offers_list.end = NULL;

    seller_info.open_fds_max = 0;
    FD_ZERO(&seller_info.open_fds);
    FD_SET(STDIN_FILENO, &seller_info.open_fds);
}

int main(int argc, char const* argv[]) {
    init_vars();
    get_info();

    for (;;) {
        fd_set working_set = seller_info.open_fds;
        int ready_fds_count = select(seller_info.open_fds_max + 1, &working_set, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &working_set)) {
            if (!cmd_interface()) break;
            --ready_fds_count;
        }
        if (ready_fds_count > 0) {
            update_offers(&working_set);
        }
    }
    free(seller_info.name);
    return 0;
}
