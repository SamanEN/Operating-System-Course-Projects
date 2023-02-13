#include <stdexcept>
#include <string>

#include "genre_processor.hpp"
#include "log.hpp"

int main(int argc, char const* argv[]) {
    std::string logger_name = (argc > 1) ? std::string(argv[1]) : "genre processor";
    Log log(logger_name, true, true);

    if (argc < 1) {
        log.error("Genre name arg missing.");
        exit(EXIT_FAILURE);
    }

    std::string genre_name(argv[1]);
    try {
        GenreProcessor processor(genre_name, log);
        processor.run();
    }
    catch (const std::exception& e) {
        log.error(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}
