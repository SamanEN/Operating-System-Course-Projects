#ifndef GENRE_PROCESSOR_HPP
#define GENRE_PROCESSOR_HPP

#include <string>
#include <vector>

#include "named_pipe.hpp"
#include "log.hpp"

class GenreProcessor {
public:
    GenreProcessor(const std::string& genre_name, Log log);

    void run();

    ~GenreProcessor();

private:
    std::string genre_name_;
    int count_ = 0;

    NamedPipeClient genre_counter_pipe_;
    NamedPipeServer server_pipe_;

    Log log_;

    void updateCount(const std::string& new_count);
    void sendResult();
};

#endif
