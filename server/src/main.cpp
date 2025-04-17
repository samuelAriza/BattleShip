// ~/universidad/telematica/test/server/src/main.cpp
#include "server.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

/**
 * @brief Punto de entrada principal para el servidor de Batalla Naval.
 *
 * Inicializa y ejecuta el servidor con los parámetros de IP, puerto y ruta de log
 * proporcionados por la línea de comandos.
 *
 * @param argc Número de argumentos de la línea de comandos.
 * @param argv Array de argumentos: [0] nombre del programa, [1] IP, [2] puerto, [3] ruta de log.
 * @return 0 si la ejecución es exitosa, 1 si hay un error.
 */
int main(int argc, char* argv[]) {
    // Verificar el número correcto de argumentos
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port> </path/log.log>\n";
        std::cerr << "Example: " << argv[0] << " 0.0.0.0 8080 ./logs/server.log\n";
        return 1;
    }

    // Convertir argumentos a tipos apropiados
    std::string ip = argv[1];
    int port;
    try {
        port = std::stoi(argv[2]);
        if (port < 1 || port > 65535) {
            throw std::out_of_range("Port out of valid range (1-65535)");
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid port: " << argv[2] << " (" << e.what() << ")\n";
        return 1;
    }
    std::string log_path = argv[3];

    // Iniciar el servidor
    try {
        BattleshipServer::Server server(ip, port, log_path);
        server.run();
    } catch (const BattleshipServer::ServerError& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}