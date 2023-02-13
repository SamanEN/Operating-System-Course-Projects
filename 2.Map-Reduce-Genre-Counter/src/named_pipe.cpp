#include "named_pipe.hpp"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

NamedPipe::NamedPipe(const std::string& pipe_name)
    : pipe_name_(pipe_name) {}

void NamedPipe::removePipe(const std::string& pipe_path) {
    remove(pipe_path.c_str());
}

NamedPipeClient::NamedPipeClient(const std::string& pipe_name)
    : NamedPipe(pipe_name) {
    do {
        pipe_fd_ = open(pipe_name.c_str(), O_WRONLY);
    } while (pipe_fd_ == -1);
}

void NamedPipeClient::send(const std::string& msg) {
    int sent_bytes = write(pipe_fd_, msg.c_str(), msg.size());
    if (sent_bytes != msg.size())
        throw std::runtime_error("Couldn't send complete msg over pipe.");
}

NamedPipeServer::NamedPipeServer(const std::string& pipe_name)
    : NamedPipe(pipe_name) {
    if ((pipe_fd_ = open(pipe_name.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
        if (mkfifo(pipe_name.c_str(), 0777) != 0) {
            throw std::runtime_error("Couldn't make FIFO file.");
        }
        else
            pipe_fd_ = open(pipe_name_.c_str(), O_RDONLY | O_NONBLOCK);
    }
    if (pipe_fd_ == -1)
        throw std::runtime_error("Couldn't open server side of named pipe.(" + pipe_name + ")");
}

NamedPipeClient::~NamedPipeClient() {
    close(pipe_fd_);
}

std::string NamedPipeServer::receive() {
    char buffer[256] = {0};
    int read_bytes = 0;

    std::string result;

    do {
        read_bytes = read(pipe_fd_, buffer, 256);

        if (read_bytes == -1 && errno != EAGAIN) {
            throw std::runtime_error("Error on pipe receive.");
        }

        result.append(std::string(buffer));
        memset(buffer, '\0', 256);
    } while (read_bytes != 0);

    return result;
}

NamedPipeServer::~NamedPipeServer() {
    close(pipe_fd_);
}
