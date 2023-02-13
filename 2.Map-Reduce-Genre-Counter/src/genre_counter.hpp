#ifndef GENRE_COUNTER_HPP
#define GENRE_COUNTER_HPP

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "log.hpp"
#include "named_pipe.hpp"

class GenreCounter {
public:
    GenreCounter(Log log);

    const std::map<std::string, int>&
    countGenres();

    const std::map<std::string, int>&
    countGenres(const std::string& dir_name);

    void setDirName(const std::string& dir_name);

    void save_csv(const std::string& file_name);

    ~GenreCounter();

private:
    std::string dir_name_;
    std::map<std::string, int> genre_counts_;
    std::unordered_map<int, std::string> counting_processes_map_;
    std::vector<std::string> genres_;
    NamedPipeServer server_pipe_;

    Log log_;

    void fetchGenres();

    std::vector<std::string> fetchParts();
    void waitParts();
    void waitStart();

    void makeGenreProcesses();
    void makeCounterProcesses();

    void gatherResults();
};

#endif
