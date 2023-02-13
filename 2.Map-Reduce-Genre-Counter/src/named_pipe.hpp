#ifndef NAMED_PIPE_HPP
#define NAMED_PIPE_HPP

#include <string>

class NamedPipe {
public:
    NamedPipe(const std::string& pipe_name);

    static void removePipe(const std::string& pipe_path);

protected:
    std::string pipe_name_;
    int pipe_fd_;
};

class NamedPipeClient : NamedPipe {
public:
    NamedPipeClient(const std::string& pipe_name);

    void send(const std::string& msg);

    ~NamedPipeClient();
};

class NamedPipeServer : NamedPipe {
public:
    NamedPipeServer(const std::string& pipe_name);

    std::string receive();

    ~NamedPipeServer();
};

#endif
