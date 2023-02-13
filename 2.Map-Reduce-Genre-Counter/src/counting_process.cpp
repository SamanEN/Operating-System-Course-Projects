#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "counting_processor.hpp"
#include "log.hpp"

int main(int argc, char const* argv[]) {
    std::string logger_name = (argc > 1) ? std::string(argv[1]) : "counting processor";
    Log log(logger_name, true, true);

    if (argc < 1) {
        log.error("Part argument missing");
        exit(EXIT_FAILURE);
    }

    std::string part_file_name(argv[1]);
    int genres_size;
    std::cin >> genres_size;
    std::vector<std::string> genres;
    genres.reserve(genres_size);
    while (genres_size--) {
        std::string temp;
        std::cin >> temp;
        genres.push_back(temp);
    }

    try {
        CountingProcessor processor(part_file_name, genres, log);
        processor.run();
    }
    catch (const std::exception& e) {
        log.error(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}
