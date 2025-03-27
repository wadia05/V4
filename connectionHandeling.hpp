#pragma once

#include "main.hpp"
#include "server.hpp"
// #include "./req/HTTPRequest.hpp" // Add this line to include the full definition of HTTPRequest
class HTTPRequest;
enum Method
{
    GET,
    POST,
    DELETE,
    NOTDETECTED
};

template <typename T>
std::string to_string(const T &value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

class Connection
{
public:
    int fd;
    int status_code;

    std::string read_buffer;
    std::string write_buffer;
    std::string path;
    std::ifstream *readFormFile;
    std::string query;
    std::string upload_path;
    Method method;

    time_t last_active;
    size_t content_length;
    size_t total_sent;

    bool keep_alive;
    bool headersSend;
    bool chunked;
    bool is_reading;
    bool is_writing;
    bool is_closing;
    // bool is_parsing;
    bool is_possessing;

    // const HTTPRequest &request; // This requires the full definition of HTTPRequest

    Connection(int fd);
    ~Connection();

    std::string GetHeaderResponse(int status_code);
    std::string GetContentType();
    std::string GetStatusMessage(int status_code);
    void GetBodyResponse();
 
};