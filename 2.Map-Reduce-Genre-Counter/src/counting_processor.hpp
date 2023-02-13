#ifndef COUNTING_PROCESSOR_HPP
#define COUNTING_PROCESSOR_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "log.hpp"

class CountingProcessor {
public:
    CountingProcessor(std::string file_path, std::vector<std::string> genres, Log log);

    void run();

private:
    const std::string file_path_;
    std::unordered_map<std::string, int> genres_count_;
    bool is_done_ = false;

    Log log_;

    void countGenres();
    void sendResultToGenresProcesses();
};

#endif
