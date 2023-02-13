#ifndef UTILS_H
#define UTILS_H

#include "sys/types.h"
#include "sys/socket.h"

#define LOG_BUF_SIZE 1024

typedef struct ParsedLine {
    int argc;
    char** argv;
} ParsedLine;

char* read_line(int, int);

ParsedLine parse_line(char*, const char*);

void put_time_header(char*);

void log_info(int, const char*, int, int);

void log_err(int, const char*, int, int);

char* getline_str(const char*);

char* add_data_header(char*, const char*);

char* indent_str(const char*, char, int);

char* get_field_val(const char*, const char*);

#endif
