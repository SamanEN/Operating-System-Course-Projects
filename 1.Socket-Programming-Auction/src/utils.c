#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "consts.h"

char* read_line(int fd, int limit) {
    char* result = NULL;
    int curr_size = 0;
    for (; limit != 0; --limit) {
        ++curr_size;
        result = (char*)realloc(result, curr_size);
        read(fd, &result[curr_size - 1], 1);
        if (result[curr_size - 1] == '\n') {
            result[curr_size - 1] = '\0';
            break;
        }
    }
    return result;
}

ParsedLine parse_line(char* input_line, const char* delims) {
    ParsedLine result = {0, NULL};
    int input_line_len = strlen(input_line);
    int last_tok_end = 0;
    for (int i = 0; i <= input_line_len; ++i) {
        if (strchr(delims, input_line[i]) != NULL) {
            result.argv = (char**)realloc(result.argv, result.argc + 1);
            result.argv[result.argc] = (char*)calloc(i - last_tok_end + 1, sizeof(char));
            memcpy(result.argv[result.argc], &input_line[last_tok_end], i - last_tok_end);
            result.argv[result.argc][i - last_tok_end] = '\0';
            ++result.argc;
            last_tok_end = i + 1;
        }
    }
    return result;
}

void put_time_header(char* dest) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    sprintf(
        dest,
        "%d-%d-%d _ %d:%d:%d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void log_info(int fd, const char* msg, int include_time, int include_tag) {
    char buf[LOG_BUF_SIZE] = {0};
    if (include_time)
        put_time_header(buf);
    strcat(buf, "\n");
    if (include_tag)
        strcat(buf, "[INFO] ");
    strcat(buf, msg);
    write(fd, buf, strlen(buf));
}

void log_err(int fd, const char* msg, int include_time, int include_tag) {
    char buf[LOG_BUF_SIZE] = {0};
    if (include_time)
        put_time_header(buf);
    strcat(buf, "\n");
    if (include_tag)
        strcat(buf, "[ERROR] ");
    strcat(buf, msg);
    write(fd, buf, strlen(buf));
}

char* getline_str(const char* str) {
    char* occ;
    if ((occ = (char*)strchr(str, '\n')) == NULL)
        occ = strchr(str, '\0');

    char* result = (char*)calloc((size_t)(occ - str), sizeof(char));
    for (int i = 0; i + str < occ; ++i)
        result[i] = str[i];
    return result;
}

char* add_data_header(char* data, const char* header) {
    char* result;
    int result_len = strlen(data) + strlen(header);
    result = (char*)calloc(result_len, sizeof(char));
    strcpy(result, header);
    strcat(result, data);
    return result;
}

char* indent_str(const char* str, char indent_char, int indent_level) {
    int line_count = 1;
    int str_len = strlen(str);

    for (int i = 0; i < str_len; ++i)
        if (str[i] == '\n')
            ++line_count;

    const char* current_line_end = str;
    char* result = (char*)calloc(str_len + (indent_level * line_count), sizeof(char));
    int current_len = 0;

    for (; line_count > 0; --line_count) {
        for (int i = 0; i < indent_level; ++i)
            result[current_len + i] = indent_char;

        char* current_line = getline_str(current_line_end);
        strcat(result, current_line);
        current_line_end += strlen(current_line);
        free(current_line);
        current_len += strlen(result);
    }
    return result;
}

char* get_field_val(const char* str, const char* field) {
    char* result = NULL;
    char* field_begin;
    if ((field_begin = strstr(str, field)) != NULL)
        result = getline_str(field_begin + strlen(field));
    return result;
}
