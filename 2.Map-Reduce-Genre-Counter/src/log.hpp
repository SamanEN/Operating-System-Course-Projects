#ifndef LOG_HPP
#define LOG_HPP

#include <ostream>
#include <iostream>

class Log {
public:
    Log(const std::string& logger, bool has_label, bool has_time_header);

    void info(const std::string& info);
    void error(const std::string& error);
    void warning(const std::string& warning);

private:
    std::ostream& info_stream_ = std::cout;
    std::ostream& error_stream_ = std::cerr;
    std::ostream& warning_stream_ = std::cout;
    
    std::string logger_;

    bool has_label_ = false;
    bool has_time_header_ = false;

    void add_time_header(std::ostream& stream);
};

#endif
