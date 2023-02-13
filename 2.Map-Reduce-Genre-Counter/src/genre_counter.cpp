#include "genre_counter.hpp"

#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "consts.hpp"
#include "counting_processor.hpp"
#include "log.hpp"
#include "named_pipe.hpp"
#include "utils.hpp"

GenreCounter::GenreCounter(Log log)
    : server_pipe_(Consts::GENRE_COUNTER_PIPE_NAME),
      log_(log) {}

const std::map<std::string, int>&
GenreCounter::countGenres() {
    makeGenreProcesses();
    makeCounterProcesses();
    waitParts();
    gatherResults();
    return genre_counts_;
}

const std::map<std::string, int>&
GenreCounter::countGenres(const std::string& dir_name) {
    setDirName(dir_name);
    return countGenres();
}

void GenreCounter::setDirName(const std::string& dir_name) {
    dir_name_ = dir_name;
}

void GenreCounter::save_csv(const std::string& file_name) {
    std::ofstream file(file_name);
    if (!file.is_open()) {
        throw std::runtime_error("Couldn't open file " + file_name + ".");
    }

    log_.info("Saving results to csv.");

    file << "Genre name, Count\n";
    for (const auto& genre_count : genre_counts_) {
        file << genre_count.first << ", " << genre_count.second << '\n';
    }
    file << '\n';

    log_.info("Results Saved to csv file.");
    file.close();
}

GenreCounter::~GenreCounter() {
    for(const std::string& genre : genres_)
        NamedPipe::removePipe(genre);
    NamedPipe::removePipe(Consts::GENRE_COUNTER_PIPE_NAME);
}

void GenreCounter::fetchGenres() {
    const std::string genres_file_path =
        dir_name_ + "/" + Consts::GENRE_FILE_NAME;

    std::ifstream genres_f_stream;
    genres_f_stream.open(genres_file_path);
    if (!genres_f_stream.is_open())
        throw std::runtime_error("Couldn't open genres file.");

    log_.info("Reading genres file.");
    while (!genres_f_stream.eof()) {
        std::string current_genre;
        std::getline(genres_f_stream, current_genre, ',');
        trim(current_genre);
        genres_.push_back(current_genre);
    }
    genres_f_stream.close();
}

std::vector<std::string> GenreCounter::fetchParts() {
    log_.info("Fetching parts.");
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(dir_name_.c_str())) == NULL)
        throw std::runtime_error("Given directory doesn't exist.");

    std::vector<std::string> result;
    while ((ent = readdir(dir)) != NULL) {
        result.push_back(std::string(ent->d_name));
    }

    // remove unwanted names
    result.erase(std::find(result.begin(), result.end(), "."));
    result.erase(std::find(result.begin(), result.end(), ".."));
    result.erase(std::find(result.begin(), result.end(), "genres.csv"));

    closedir(dir);
    return result;
}

void GenreCounter::waitParts() {
    log_.info("Waiting for part processes to finish.");
    while (!counting_processes_map_.empty()) {
        int return_status;
        int pid = wait(&return_status);
        if (return_status != EXIT_SUCCESS)
            throw std::runtime_error("An error has occurred in counting process.");
        log_.info(counting_processes_map_[pid] + " finished.");
        counting_processes_map_.erase(counting_processes_map_.find(pid));
    }
}

void GenreCounter::waitStart() {
    log_.info("Waiting for genres to make pipes.");
    std::unordered_set<std::string> pipes_to_make(genres_.begin(), genres_.end());
    while (!pipes_to_make.empty()) {
        std::string received_cmd = server_pipe_.receive();
        log_.warning(received_cmd);
        std::istringstream str(received_cmd);
        
        std::string genre;
        while (str >> genre) {
            pipes_to_make.erase(genre);
        }
    }
}

void GenreCounter::makeGenreProcesses() {
    fetchGenres();
    for (const std::string& genre : genres_) {
        int pid = fork();
        if (pid > 0)
            continue;
        else if (pid == 0) {
            execl(Consts::GENRE_PROCESS_PATH.c_str(), "genre_process", genre.c_str(), (char*)NULL);
        }
        else
            throw std::runtime_error("fork() failed in makeGenreProcesses.");
    }
}

void GenreCounter::makeCounterProcesses() {
    auto parts_vec = fetchParts();
    for (const std::string& part : parts_vec) {
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1)
            throw std::runtime_error("Couldn't open pipe between main and counter processes.");

        int pid = fork();
        if (pid > 0) {
            close(pipe_fds[0]);
            std::string genres_count_str = std::to_string(genres_.size()) + ' ';
            write(pipe_fds[1], genres_count_str.c_str(), genres_count_str.size());
            for (int i = 0; i < genres_.size(); ++i) {
                std::string to_write = genres_[i] + " ";
                write(pipe_fds[1], to_write.c_str(), to_write.size());
            }
            counting_processes_map_[pid] = part;
        }
        else if (pid == 0) {
            close(0);
            dup2(pipe_fds[0], 0);
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            std::string part_file_name = dir_name_ + "/" + part;
            execl(Consts::COUNTING_PROCESS_PATH.c_str(), "counting_process", part_file_name.c_str(), (char*)NULL);
        }
        else
            throw std::runtime_error("fork() failed in makeCounterProcesses.");
    }
}

void GenreCounter::gatherResults() {
    log_.info("Gathering results.");
    for (const auto& genre : genres_) {
        std::string pipe_name = genre;
        NamedPipeClient client_pipe(pipe_name);
        std::string pipe_cmd = Consts::PipeCommands::FINISH_GENRE_PROCESS + '\n';
        client_pipe.send(pipe_cmd);
    }

    std::istringstream genres_str(server_pipe_.receive());
    std::string genre, count;
    while (genres_str >> genre) {
        genres_str >> count;
        genre_counts_.insert({genre, std::stoi(count)});
    }
}
