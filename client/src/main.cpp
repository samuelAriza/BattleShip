#include "client.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Uso correcto: ./bsclient </ruta/log.log>\n";
        return 1;
    }

    std::string log_path = argv[1];

    // Crear carpetas si no existen
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
        std::cerr << "Error creando directorios para el log: " << e.what() << "\n";
        return 1;
    }

    std::string nickname;
    std::string email;

    std::cout << "=== Bienvenido a Batalla Naval ===\n";
    std::cout << "Ingresa tu nickname: ";
    std::getline(std::cin, nickname);

    std::cout << "Ingresa tu email: ";
    std::getline(std::cin, email);

    const std::string server_ip = "127.0.0.1";
    const int server_port = 8080;

    try
    {
        BattleshipClient::Client client(server_ip, server_port, nickname, email, log_path);
        client.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Cliente falló: " << e.what() << "\n";
        return 1;
    }

    return 0;
}