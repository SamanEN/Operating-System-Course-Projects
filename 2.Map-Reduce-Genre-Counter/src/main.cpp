#include <iostream>
#include <stdexcept>

#include "genre_counter.hpp"
#include "log.hpp"

int main(int argc, char const* argv[]) {
    Log log("genre counter", true, true);

    if (argc < 1) {
        log.error("Directory name arg missing.");
        exit(EXIT_FAILURE);
    }

    std::string dir_name(argv[1]);
    try {
        GenreCounter genre_counter(log);
        genre_counter.setDirName(dir_name);
        auto result = genre_counter.countGenres();
        for (const auto& result_pair : result)
            std::cout << result_pair.first << " : " << result_pair.second << '\n';
        genre_counter.save_csv("result.csv");
    }
    catch (const std::exception& e) {
        log.error(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}
