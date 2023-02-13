#include "genre_processor.hpp"

#include <sstream>
#include <string>

#include "consts.hpp"
#include "named_pipe.hpp"

GenreProcessor::GenreProcessor(const std::string& genre_name, Log log)
    : genre_name_(genre_name),
      genre_counter_pipe_(NamedPipeClient(Consts::GENRE_COUNTER_PIPE_NAME)),
      server_pipe_(NamedPipeServer(genre_name)),
      log_(log) {
        log_.info("Created named pipe " + genre_name + ".");
}

void GenreProcessor::run() {
    bool is_finished = false;
    while (!is_finished) {
        std::string current_pipe_command = server_pipe_.receive();
        std::istringstream pipe_command_str(current_pipe_command);
        std::string cmd;
        while (pipe_command_str >> cmd) {
            if (cmd == Consts::PipeCommands::UPDATE_GENRE_COUNT) {
                std::string count;
                pipe_command_str >> count;
                updateCount(count);
            }
            else if (cmd == Consts::PipeCommands::FINISH_GENRE_PROCESS) {
                log_.info("Sending results.");
                sendResult();
                is_finished = true;
                break;
            }
        }
    }
}

GenreProcessor::~GenreProcessor() {

}

void GenreProcessor::updateCount(const std::string& new_count) {
    count_ += std::stoi(new_count);
}

void GenreProcessor::sendResult() {
    std::ostringstream result_stream;
    result_stream << genre_name_;
    result_stream << ' ' << count_ << '\n';
    genre_counter_pipe_.send(result_stream.str());
}
