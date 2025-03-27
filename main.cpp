#include "main.hpp"


void print_message(std::string message, std::string color)
{
	std::cout << color << message << RESET << std::endl;
}

int main(int ac, char** av) {

    if (ac != 2)
        return (print_message("Usage: ./webserv <config_file>", RED), 1);
    MimeTypes mimeTypes("mimeTypes.csv");
    std::ifstream file(av[1]);
    Config config;
    config.parseConfig(file);
    // std::vector<Config> configs = config.getConfigs();
    try {
        Server server(config);
        // exit(1);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
