#ifndef CONSTS_HPP
#define CONSTS_HPP

#include <string>

namespace Consts {

const std::string GENRE_FILE_NAME = "genres.csv";

const std::string GENRE_COUNTER_PIPE_NAME = "./genre_counter_pipe";

const std::string GENRE_PROCESS_PATH = "./genre_process";

const std::string COUNTING_PROCESS_PATH = "./counting_process";

namespace PipeCommands {

    const std::string UPDATE_GENRE_COUNT = "update_count";

    const std::string FINISH_GENRE_PROCESS = "finish";

} // namespace PipeCommands
} // namespace Consts

#endif
