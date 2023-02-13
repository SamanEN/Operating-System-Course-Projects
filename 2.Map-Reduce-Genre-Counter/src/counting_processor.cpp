#include "counting_processor.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "consts.hpp"
#include "named_pipe.hpp"
#include "utils.hpp"

CountingProcessor::CountingProcessor(std::string file_path, std::vector<std::string> genres, Log log)
    : file_path_(file_path),
      log_(log) {
    for (const std::string& genre : genres)
        genres_count_.insert({genre, 0});
}

void CountingProcessor::run() {
    countGenres();
    sendResultToGenresProcesses();
}

void CountingProcessor::countGenres() {
    std::ifstream part_file;
    part_file.open(file_path_);
    if (!part_file.is_open())
        throw std::runtime_error("Can't open part file.");

    while (!part_file.eof()) {
        std::string current_line;
        std::getline(part_file, current_line);

        std::vector<std::string> parsed_line =
            split_line(current_line, ',');

        parsed_line.erase(parsed_line.begin());
        for (const std::string& genre : parsed_line) {
            if (genres_count_.find(genre) != genres_count_.end()) {
                ++genres_count_[genre];
                continue;
            }

            throw std::runtime_error(genre + " is in " + file_path_ + " but is not listed in genres.");
        }
    }
}

void CountingProcessor::sendResultToGenresProcesses() {
    for (const auto& genre_count : genres_count_) {
        // make a client side to send msgs to genre pipe
        NamedPipeClient client_pipe(genre_count.first);
        std::string cmd_to_send =
            Consts::PipeCommands::UPDATE_GENRE_COUNT + " " + std::to_string(genre_count.second) + "\n";
        client_pipe.send(cmd_to_send);
    }
}
