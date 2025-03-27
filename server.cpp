#include "server.hpp"

Server::Server(const Config &config) : current_fd_index(0){
    
    this->inx = 0;
    this->configs = config.getConfigs();
    this->serverIp = this->configs[this->inx].getHost()[0];
    this->port = std::atoi(this->configs[this->inx].getPort()[0].c_str());
    this->max_upload_size = 0;
    this->serverName = this->configs[this->inx].getServerName()[0];
    this->root = "www";
    this->index_page = "www/index.html";
    this->error_page = "www/error_pages/404.html";
    


    // int epoll_create(int size);
    this->epoll_fd = epoll_create(1);
    if (this->epoll_fd < 0) {
        throw std::runtime_error("Failed to create epoll");
    }

    // Create server socket
    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR");
    }


    // Bind and listen
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(this->serverIp.c_str());
    addr.sin_port = htons(this->port);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    
    if (listen(server_fd, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen");
    }
    set_non_blocking(server_fd);
    // Add server socket to epoll
    add_to_epoll(server_fd, EPOLLIN);
}
Server::~Server() {
    // Cleanup connections
    for (std::map<int, Connection*>::iterator it = connections.begin(); it != connections.end(); ) {
        close_connection(it->second);  // This erases from map and deletes
        it = connections.begin();  // Reset iterator after erase
    }
    close(server_fd);
    close(epoll_fd);
}


void Server::run() {
    struct epoll_event events[MAX_EVENTS];
    
    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        
        for (int i = 0; i < num_events; i++) {
            int current_fd = events[i].data.fd;
            
            if (current_fd == server_fd) {
                handle_connection(current_fd);
            } else {
                std::map<int, Connection*>::iterator it = connections.find(current_fd);
                if (it != connections.end()) {
                    handle_request(it->second);
                }
            }
        }

        // Handle keep-alive timeouts safely
        time_t current_time = time(0);
        // std::cout << "Current time: " << current_time << std::endl;
        std::vector<int> expired_fds;
        for (std::map<int, Connection*>::const_iterator it = connections.begin(); it != connections.end(); ++it) {
            std::cout << "Checking connection: " << it->first << " with last active: " << it->second->last_active << std::endl;
            if (current_time - it->second->last_active > KEEP_ALIVE_TIMEOUT) {
                expired_fds.push_back(it->first);
            }
        }
        
        for (size_t i = 0; i < expired_fds.size(); ++i) {
            std::map<int, Connection*>::iterator it = connections.find(expired_fds[i]);
            if (it != connections.end()) {
                // std::cout << "from timeout : " << it->first << std::endl;
                close_connection(it->second);
            }
        }
    }
}
void Server::set_non_blocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::mod_epoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void Server::add_to_epoll(int fd, uint32_t events){
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void Server::remove_from_epoll(int fd){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void Server::handle_connection(int fd){
    // handle the connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        throw std::runtime_error("Failed to accept connection");
    }
    set_non_blocking(client_fd);
    add_to_epoll(client_fd, EPOLLIN);
    this->ready_fds.push_back(client_fd);
    connections[client_fd] = new Connection(client_fd);
    connections[client_fd]->is_reading = true;
    std::cout << "New connection accepted" << std::endl;
}

void Server::handle_request(Connection* conn){
    // HTTPRequest request;
    if(conn->is_reading)
        read_request(conn);
    else if(conn->is_possessing)
    {
        parseReaquest(conn);
        conn->last_active = time(0);
        // possess_request(conn);
        std::cout << "Parsing the request" << std::endl;
    }
    else if(conn->is_writing)
    {
        std::cout << MAGENTA << "Sending the response" << RESET  << std::endl;
        conn->last_active = time(0);
        send_response(conn);
        // if (conn->is_reading == true)
        //     mod_epoll(conn->fd, EPOLLIN);
            // restCient(conn);
    }
    // std::cout << "Last active: " << conn->last_active << std::endl;
}

void Server::read_request(Connection* conn){
    int read_bytes = 0;
    char buffer[BUFFER_SIZE];
    if (conn->fd == 0)
    {
        std::cout << "no socket available" << std::endl;
        return;
    }
    read_bytes = recv (conn->fd , buffer, sizeof(buffer), MSG_NOSIGNAL | MSG_DONTWAIT);
    if (read_bytes < 0) {
        // if (errno == EAGAIN || errno == EWOULDBLOCK) {
        //     // Would block, try again later
        //     mod_epoll(conn->fd, EPOLLIN);
        //     return;
        // }
        // throw std::runtime_error("Failed to read request");
        close_connection(conn);
        return;
    }
    conn->read_buffer.append(buffer, read_bytes);
    conn->last_active = time(0);
    if(read_bytes < BUFFER_SIZE )
    {
        // std::cout << CYAN << conn->read_buffer << RESET<< std::endl;
        conn->is_reading = false;
        conn->is_possessing = true;
        mod_epoll(conn->fd, EPOLLOUT);
        
    }
    // std::cout << conn->read_buffer << std::endl;
    // read the request
}
void setReqType(Connection *conn, HTTPRequest request)
{
    if (conn->method == NOTDETECTED)
    {
        if(request.getMethod() == "GET" && conn->status_code == 200)
            conn->method= GET;
        else if(request.getMethod() == "POST" && conn->status_code == 200)
            conn->method = POST;
        else if(request.getMethod() == "DELETE" && conn->status_code == 200)
            conn->method = DELETE;
    }
}
void Server::parseReaquest(Connection *conn)
{

    HTTPRequest request;
    if (!request.parse_request(conn->read_buffer, this->configs[this->inx]))
    {
        print_message("Error parsing request", RED);
        conn->status_code = request.getStatusCode();
        // conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
        // if (!conn->readFormFile->is_open())
        // {
        //     std::cerr << "Failed to open error file" << std::endl;
        //     return;
        // }
        // return ;
    }
    else
    {
        CGI cgi;
        int i = 0;
        bool is_upload = cgi.upload(request, this->configs[this->inx]);
        if (!is_upload && cgi.getStatus() != 200)
        {
            conn->status_code = cgi.getStatus();
            return;
        }
        else if (is_upload)
            i = 1;
        if (cgi.is_cgi(request.getPath(), this->configs[this->inx], request.getInLocation()) && i == 0)
        {
            if (!cgi.exec_cgi(request, conn->response))
            conn->status_code  = cgi.getStatus();
        }
        // else
        // {
        //     std::string file_content = read_file(std::string(request.getPath()));
        //     if (file_content.empty())
        //     {
        //         print_message("Error reading file", RED);
        //         conn->status_code  = 404;
        //     }
        //     else
        //     conn->response = file_content;
        // }
    }
    conn->read_buffer.clear();
    setReqType(conn, request);
    possess_request(conn , request);
    // request.print_all();
    conn->last_active = time(0);
    conn->is_possessing = false;
    conn->is_writing = true;
}
void Server::possess_request(Connection* conn, HTTPRequest &request)
{
    // HTTPRequest request = HTTPRequest(conn->read_buffer);
    if (conn->method == GET)
    {
        std::cout << "GET request" << std::endl;
        this->GET_hander(conn, request);
    }
    else if(conn->method == POST)
    {
        std::cout << "POST request" << std::endl;
        this->POST_hander(conn);
    }
    else if (conn->method == DELETE)
    {
        std::cout << "DELETE request" << std::endl;
        this->DELETE_hander(conn, request);
    }
    else
    {
        std::cout << conn->status_code << std::endl;
        print_message("Unknown request", RED);
    }
    conn->is_possessing = false;
    conn->is_writing = true;
}
void resetClient(Connection* conn)
{
    // Safely close the file if it's open
    if (conn->readFormFile && conn->readFormFile->is_open()) {
        conn->readFormFile->close();
    }

    // Reset string buffers
    conn->read_buffer.clear();
    conn->write_buffer.clear();
    conn->path.clear();
    conn->query.clear();
    conn->upload_path.clear();

    // Reset state variables
    conn->method = NOTDETECTED;
    conn->last_active = time(0);  // Set to current time instead of 0
    conn->content_length = 0;
    conn->total_sent = 0;
    conn->status_code = 0;

    // Reset connection flags
    conn->keep_alive = true;
    conn->headersSend = false;
    conn->chunked = false;
    conn->is_reading = true;
    conn->is_writing = false;
    conn->is_closing = false;
    conn->is_possessing = false;
    conn->is_cgi = false;
    // Reset file stream
    if (conn->readFormFile) {
        delete conn->readFormFile;
    }
    conn->readFormFile = new std::ifstream();

    std::cout << GREEN "Client connection reset" RESET << std::endl;
}

void Server::send_response(Connection* conn) {
    // First time sending: prepare header
    
    if (!conn->response.empty())
    {
        conn->is_cgi = true;
        print_message("is cgiiiiiiiiiiiiiiiiiiii", GREEN);

    }
    if (!conn->headersSend) {
        conn->write_buffer = conn->GetHeaderResponse(conn->status_code);
        // std::cout << conn->write_buffer << std::endl;
        conn->headersSend = true;
        conn->total_sent = 0;
    }

    // If we haven't opened the file yet, open it
    if (!conn->readFormFile->is_open() && !conn->is_cgi) {
        std::cerr << "No file to send" << std::endl;
        close_connection(conn);
        return;
    }

    // Prepare to send body in chunks
    char buffer[BUFFER_SIZE];
    std::streamsize bytes_read = 0;
    if (!conn->is_cgi)
    {
        conn->readFormFile->read(buffer, BUFFER_SIZE);
        bytes_read = conn->readFormFile->gcount();
        conn->write_buffer.append(buffer, bytes_read);
    }

    if (bytes_read > 0 || conn->is_cgi) {
        if (conn->is_cgi)
        {
            conn->write_buffer.append(conn->response.c_str(), conn->response.size()); 
        }
        // std::cout << "Sending: " << conn->write_buffer << std::endl;
        ssize_t sent = send(conn->fd, conn->write_buffer.c_str(), conn->write_buffer.size(), MSG_NOSIGNAL);
        conn->write_buffer.clear();
        conn->total_sent += sent;
        std::cout << "Total sent: " << conn->total_sent  << " / " << conn->content_length << std::endl;
        if (sent < 0) {
           
            close_connection(conn);
            return;
        }

        if(conn->is_cgi)
        {
            std::cout << GREEN << "File completely sent" << RESET << std::endl;
            conn->readFormFile->close();
            conn->is_writing = false;
            resetClient(conn);
            mod_epoll(conn->fd, EPOLLIN);
        }
        // If not all bytes were sent, handle partial send
        if (sent < bytes_read && !conn->is_cgi) {
            // You might want to keep track of partially sent data
            conn->write_buffer.append(buffer + sent, bytes_read - sent);
            mod_epoll(conn->fd, EPOLLOUT);
            return;
        }
    }

    // Check if we've reached the end of the file
    if (conn->readFormFile->eof()) {
        std::cout << GREEN << "File completely sent" << RESET << std::endl;
        conn->readFormFile->close();
        conn->is_writing = false;
        resetClient(conn);
        mod_epoll(conn->fd, EPOLLIN);
    }
}
void Server::close_connection(Connection* conn) {
    if (!conn) return;
    resetClient(conn);
    int fd = conn->fd;
    std::cout << "Closing connection: " << fd << std::endl;
// Remove from monitoring first
    remove_from_epoll(fd);

    // this->ready_fds.erase(std::remove(this->ready_fds.begin(), this->ready_fds.end(), fd), this->ready_fds.end());
    // Close socket
    close(fd);

    // Remove from connections map BEFORE deletion
    connections.erase(fd);

    // Delete connection object
    delete conn;
        // Remove from monitoring first
}




// << =================== Methods for Server =================== >> //


int Server::GET_hander(Connection *conn, HTTPRequest &request)
{
   if (request.getPath() == "/favicon.ico")
   {
     conn->path = this->root + request.getPath();
   }
    else
        conn->path = request.getPath();

    std::cout << RED << "Path: " << conn->path << RESET <<std::endl;
    conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
    if (!conn->readFormFile->is_open())
    {
        std::cerr << "Failed to open file" << std::endl;
        conn->path = this->error_page;
        conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
        if (!conn->readFormFile->is_open())
        {
            std::cerr << "Failed to open error file" << std::endl;
            return 0;
        }
        return 1;
    }
    conn->readFormFile->seekg(0, std::ios::end);
    conn->content_length = conn->readFormFile->tellg();
    conn->readFormFile->seekg(0, std::ios::beg);
    conn->status_code = 200;
    return 0;
}


int Server::POST_hander(Connection *conn)
{
    if (!conn->response.empty())
    {
        conn->status_code = 200;
        return 0;
    }
    conn->path = "www/suss/postsuss.html";
    std::cout << RED << "Path: " << conn->path << RESET <<std::endl;
    conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
    if (!conn->readFormFile->is_open())
    {
        std::cerr << "Failed to open file" << std::endl;
        conn->path = this->error_page;
        conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
        if (!conn->readFormFile->is_open())
        {
            std::cerr << "Failed to open error file" << std::endl;
            return 0;
        }
        return 1;
    }
    conn->readFormFile->seekg(0, std::ios::end);
    conn->content_length = conn->readFormFile->tellg();
    conn->readFormFile->seekg(0, std::ios::beg);
    conn->status_code = 201;
    return 0;
}
void deleteFile(std::string path)
{
    // Check if file exists before attempting to delete
    if (access(path.c_str(), F_OK) != 0) {
        std::cerr << "File does not exist: " << path << std::endl;
        return;
    }
    
    // Check if we have write permission to delete the file
    if (access(path.c_str(), W_OK) != 0) {
        std::cerr << "No permission to delete file: " << path << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return;
    }
    
    // Attempt to delete the file
    if (remove(path.c_str()) != 0) {
        std::cerr << "Error deleting file: " << path << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
    } else {
        std::cout << "File successfully deleted: " << path << std::endl;
    }
}

int Server::DELETE_hander(Connection *conn, HTTPRequest &request)
{
    std::cout << request.getPath() << std::endl;
    deleteFile(request.getPath());
    conn->path = "www/suss/deletesuss.html";
    std::cout << RED << "Path: " << conn->path << RESET <<std::endl;
    conn->readFormFile->open(conn->path.c_str(), std::ios::in | std::ios::binary);
    if (!conn->readFormFile->is_open())
    {
        std::cerr << "Failed to open file" << std::endl;
    }
    conn->readFormFile->seekg(0, std::ios::end);
    conn->content_length = conn->readFormFile->tellg();
    conn->readFormFile->seekg(0, std::ios::beg);
    conn->status_code = 204;
    return 0;
}