#include "client.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
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

    const std::string server_ip = "127.0.0.1";
    const int server_port = 8080 ;
    while (true)
{
    try
    {
        BattleshipClient::Client client(server_ip, server_port, nickname, email, log_path);
        client.run();  // Ejecuta una partida completa
        std::cout << "ESTOY EN EL MAIN " << std::endl;
        std::string input;

        std::cout << "\n==========================================\n";
        std::cout << "         ¿Deseas jugar otra partida?\n";
        std::cout << "==========================================\n";
        std::cout << " > Ingresa cualquier tecla para continuar\n";
        std::cout << " > Escribe 'Q' o 'QUIT' para salir\n";
        std::cout << "------------------------------------------\n";
        std::cout << " > Tu elección: ";
        std::getline(std::cin, input);

        
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);
        
        if (input == "q" || input == "quit")
        {
            std::cout << "\nGracias por jugar. ¡Hasta la próxima!\n";
            break;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[ERROR] Fallo en el cliente: " << e.what() << "\n";
        break;
    }
}

return 0;
}