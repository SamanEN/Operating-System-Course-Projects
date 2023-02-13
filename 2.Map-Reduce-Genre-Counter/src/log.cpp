#include "log.hpp"

#include <chrono>
#include <ctime>
#include <ostream>

Log::Log(const std::string& logger, bool has_label, bool has_time_header)
    : logger_(logger),
      has_label_(has_label),
      has_time_header_(has_time_header) {}

void Log::info(const std::string& info) {
    if (has_time_header_)
        add_time_header(info_stream_);
    if (has_label_)
        info_stream_ << "[INFO : " + logger_ + "] ";

    info_stream_ << info << '\n';
}

void Log::error(const std::string& error) {
    if (has_time_header_)
        add_time_header(error_stream_);
    if (has_label_)
        error_stream_ << "[ERROR : " + logger_ + "] ";

    error_stream_ << error << '\n';
}

void Log::warning(const std::string& warning) {
    if (has_label_)
        add_time_header(warning_stream_);
    if (has_label_)
        warning_stream_ << "[WARNING : " + logger_ + "] ";

    warning_stream_ << warning << '\n';
}

void Log::add_time_header(std::ostream& stream) {
    auto cur_time = std::chrono::system_clock::now();
    auto cur_time_t = std::chrono::system_clock::to_time_t(cur_time);
    stream << std::ctime(&cur_time_t);
}
