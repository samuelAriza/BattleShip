#include "client.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <../../include/dotenv.h>

std::string get_env(const std::string& key, const std::string& fallback) {
    const char* val = std::getenv(key.c_str());
    return val ? val : fallback;
}

int main(int argc, char *argv[])
{
    dotenv::init();
    if (argc != 2)
    {
        std::cerr << "Usage: ./bsclient </path/to/log.log>\n";
        return 1;
    }

    std::string log_path = argv[1];

    // Create directories if they do not exist
    try
    {
        std::filesystem::path log_dir = std::filesystem::path(log_path).parent_path();
        if (!log_dir.empty() && !std::filesystem::exists(log_dir))
        {
            std::filesystem::create_directories(log_dir);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating directories for log file: " << e.what() << "\n";
        return 1;
    }

    std::string nickname;
    std::string email;

    std::cout << "=== Welcome to Battleship ===\n";
    std::cout << "Enter your nickname: ";
    std::getline(std::cin, nickname);

    std::cout << "Enter your email: ";
    std::getline(std::cin, email);

    const std::string server_ip = get_env("SERVER_IP", "127.0.0.1");
    const int server_port = std::stoi(get_env("SERVER_PORT", "8080"));
    while (true)
    {
        try
        {
            BattleshipClient::Client client(server_ip, server_port, nickname, email, log_path);
            client.run(); // Runs a complete game

            std::string input;
            std::cout << "\nDo you want to play another game? (Y/N): ";
            std::getline(std::cin, input);

            if (input != "Y" && input != "y")
            {
                std::cout << "Thanks for playing. See you next time!\n";
                break;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Client failed: " << e.what() << "\n";
            break;
        }
    }
    return 0;
}