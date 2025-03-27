#pragma once

#include "main.hpp"

class Connection;  // Forward declaration

class Server {
    private :
        int inx;
        int port;
        int server_fd;
        int max_upload_size;
        int epoll_fd;
        int is_ready;
        size_t current_fd_index;

        std::string serverName;
        std::string serverIp;
        std::string root; // server root directory
        std::string index_page; // html file  should be already uploded
        std::string error_page;
        
        std::map<int, Connection*> connections;
        std::map<int, std::string> server_configs;
        std::vector<int> ready_fds;
        std::vector<Config> configs;
        


    public :

        Server(const Config &config); // change parameter base on the parser
        ~Server();
        void run();

        void set_non_blocking(int fd);
        void add_to_epoll(int fd, uint32_t events);
        void mod_epoll(int fd, uint32_t events);
        void remove_from_epoll(int fd);
        void handle_connection(int fd);
        void handle_request(Connection* conn);
        void parseReaquest(Connection* conn);
        void possess_request(Connection* conn , HTTPRequest &request);
        void read_request(Connection* conn);
        void send_response(Connection* conn);
        void close_connection(Connection* conn);
        
        //methods fonctions
        int GET_hander(Connection *conn, HTTPRequest &request);
        int POST_hander(Connection *conn);
        int DELETE_hander(Connection *conn, HTTPRequest &request);

};