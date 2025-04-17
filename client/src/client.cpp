#include "client.hpp"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <random>
#include <array>

namespace BattleshipClient
{

    Client::Client(const std::string &server_ip, int server_port, const std::string &nickname, const std::string &email, const std::string &log_path)
        : server_ip_(server_ip), server_port_(server_port), nickname_(nickname), email_(email), client_fd_(-1), running_(false)
    {
        server_addr_.sin_family = AF_INET;
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr_.sin_addr) <= 0)
        {
            throw ClientError("Invalid server IP: " + server_ip);
        }
        server_addr_.sin_port = htons(server_port);

        log_file_.open(log_path, std::ios::app);
        if (!log_file_.is_open())
        {
            throw ClientError("Failed to open log file: " + log_path);
        }
    }

    Client::~Client()
    {
        running_ = false;
        if (client_fd_ != -1)
            close(client_fd_);
        if (send_thread_.joinable())
            send_thread_.join();
        if (recv_thread_.joinable())
            recv_thread_.join();
        if (log_file_.is_open())
            log_file_.close();
    }

    void Client::run()
    {
        connect_to_server();
        running_ = true;

        send_thread_ = std::thread(&Client::send_loop, this);
        recv_thread_ = std::thread(&Client::receive_loop, this);

        send_thread_.join();
        recv_thread_.join();
    }

    void Client::connect_to_server()
    {
        client_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd_ == -1)
        {
            throw ClientError("Failed to create socket: " + std::string(strerror(errno)));
        }
        if (connect(client_fd_, (struct sockaddr *)&server_addr_, sizeof(server_addr_)) < 0)
        {
            close(client_fd_);
            throw ClientError("Failed to connect to server: " + std::string(strerror(errno)));
        }
        log("Connected to server", server_ip_ + ":" + std::to_string(server_port_));
    }

    void Client::send_loop()
    {
        // Esperar el mensaje PLAYER_ID
        {
            std::unique_lock<std::mutex> lock(player_id_mutex_);
            player_id_cv_.wait(lock, [this]
                               { return player_id_ != -1 || !running_; });
            if (!running_)
                return;
        }

        // Fase de Registro
        try
        {
            std::cout << "[DEBUG] Enviando REGISTER para nickname: " << nickname_ << std::endl;
            BattleShipProtocol::Message register_msg{
                BattleShipProtocol::MessageType::REGISTER,
                BattleShipProtocol::RegisterData{nickname_, email_}};
            send_message(register_msg);
            log(protocol_.build_message(register_msg), "Sent registration", "INFO");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] REGISTER failed: " << e.what() << std::endl;
            log("REGISTER failed", e.what(), "ERROR");
            running_ = false;
            return;
        }

        // Fase de Colocación
        {
            std::unique_lock<std::mutex> lock(state_mutex_);
            while (running_ && last_status_.gameState == BattleShipProtocol::GameState::WAITING)
            {
                lock.unlock();
                std::cout << "[DEBUG] Esperando para entrar en fase PLACEMENT..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                lock.lock();
            }
            if (!running_)
                return;
        }
        std::cout << "Fase de colocación de barcos.\n";
        std::cout << "¿Deseas colocar los barcos manualmente o de forma aleatoria?\n";
        std::cout << "Ingresa 'M' para manual o 'R' para aleatorio: ";
        std::string choice;
        std::getline(std::cin, choice);

        BattleShipProtocol::PlaceShipsData ships_data;
        if (choice == "M" || choice == "m")
        {
            std::cout << "[DEBUG] Seleccionada colocación manual de barcos.\n";
            ships_data = generate_manual_ships();
        }
        else
        {
            std::cout << "[DEBUG] Seleccionada colocación aleatoria de barcos.\n";
            ships_data = generate_initial_ships();
        }

        try
        {
            std::cout << "[DEBUG] Enviando PLACE_SHIPS\n";
            BattleShipProtocol::Message place_ships_msg{
                BattleShipProtocol::MessageType::PLACE_SHIPS,
                ships_data};
            send_message(place_ships_msg);
            log(protocol_.build_message(place_ships_msg), "Sent ship placement", "INFO");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] PLACE_SHIPS failed: " << e.what() << std::endl;
            log("PLACE_SHIPS failed", e.what(), "ERROR");
            running_ = false;
            return;
        }

        // Fase de Juego
        while (running_)
        {
            std::unique_lock<std::mutex> lock(state_mutex_);
            if (last_status_.gameState == BattleShipProtocol::GameState::ENDED)
            {
                std::cout << "[DEBUG] Game ended, exiting send_loop" << std::endl;
                running_ = false;
                break;
            }
            if (last_status_.turn == BattleShipProtocol::Turn::YOUR_TURN && last_status_.time_remaining == 30)
            {
                std::cout << "[INFO] Tu turno ha comenzado. Tienes 30 segundos.\n";
            }
            if (last_status_.turn != BattleShipProtocol::Turn::YOUR_TURN && last_status_.time_remaining == 30)
            {
                std::cout << "[INFO] Turno del oponente. Esperando...\n";
            }

            if (last_status_.turn != BattleShipProtocol::Turn::YOUR_TURN ||
                last_status_.gameState != BattleShipProtocol::GameState::ONGOING)
            {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            lock.unlock();

            std::string input;
            std::cout << "\nEnter your choice: ";
            std::getline(std::cin, input);
            if (!running_)
                break;

            try
            {
                if (input.substr(0, 6) == "SHOOT ")
                {
                    std::string coord = input.substr(6);
                    if (coord.length() < 2)
                    {
                        std::cout << "[ERROR] Invalid coordinate: too short (e.g., use 'SHOOT A1')" << std::endl;
                        continue;
                    }
                    std::string letter = coord.substr(0, 1);
                    int number = std::stoi(coord.substr(1));
                    if (letter < "A" || letter > "J" || number < 1 || number > 10)
                    {
                        std::cout << "[ERROR] Invalid coordinate: use A-J and 1-10 (e.g., 'SHOOT A1')" << std::endl;
                        continue;
                    }
                    auto coord_pair = std::make_pair(letter, number);
                    if (shot_history_.find(coord_pair) != shot_history_.end())
                    {
                        std::cout << "[ERROR] You already shot at " << letter << number << ". Select another coordinate.\n";
                        continue;
                    }
                    shot_history_.insert(coord_pair);

                    std::cout << "[DEBUG] Sending SHOOT to coordinate: " << letter << number << std::endl;
                    BattleShipProtocol::Message shoot_msg{
                        BattleShipProtocol::MessageType::SHOOT,
                        BattleShipProtocol::ShootData{BattleShipProtocol::Coordinate{letter, number}}};
                    send_message(shoot_msg);
                    log(protocol_.build_message(shoot_msg), "Sent shot", "INFO");
                }
                else if (input == "SURRENDER" || input == "4")
                {
                    std::cout << "[DEBUG] Sending SURRENDER\n";
                    BattleShipProtocol::Message surrender_msg{BattleShipProtocol::MessageType::SURRENDER};
                    send_message(surrender_msg);
                    log(protocol_.build_message(surrender_msg), "Sent surrender", "INFO");
                    running_ = false;
                    break;
                }
                else if (input == "2")
                {
                    display_shot_history();
                }
                else if (input == "3" || input == "QUIT" || input == "quit")
                {
                    std::cout << "Quitting game...\n";
                    running_ = false;
                    break;
                }
                else
                {
                    std::cout << "[ERROR] Invalid input. Use:\n";
                    std::cout << "  1. 'SHOOT <letter><number>' to shoot\n";
                    std::cout << "  2 to view shot history\n";
                    std::cout << "  3 or 'QUIT' to exit\n";
                    std::cout << "  4 or 'SURRENDER' to surrender\n";
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[ERROR] Invalid input format: " << e.what() << std::endl;
                log(input, "Invalid input format: " + std::string(e.what()), "ERROR");
            }
        }
    }

    void Client::receive_loop()
    {
        while (running_)
        {
            try
            {
                auto msg = receive_message();
                std::string response = protocol_.build_message(msg);
                log("Received", response);

                switch (msg.type)
                {
                case BattleShipProtocol::MessageType::PLAYER_ID:
                {
                    std::lock_guard<std::mutex> lock(player_id_mutex_);
                    player_id_ = std::get<BattleShipProtocol::PlayerIdData>(msg.data).player_id;
                    log("Received PLAYER_ID", "Assigned ID: " + std::to_string(player_id_), "INFO");
                    player_id_cv_.notify_all();
                    break;
                }
                case BattleShipProtocol::MessageType::STATUS:
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    last_status_ = std::get<BattleShipProtocol::StatusData>(msg.data);
                    log("Processing STATUS", "Turn: " + std::string(last_status_.turn == BattleShipProtocol::Turn::YOUR_TURN ? "YOUR_TURN" : "OPPONENT_TURN"));
                    try
                    {
                        display_game_state();
                    }
                    catch (const std::exception &e)
                    {
                        log("display_game_state failed", e.what(), "ERROR");
                    }
                    break;
                }
                case BattleShipProtocol::MessageType::GAME_OVER:
                {
                    std::string result = std::get<BattleShipProtocol::GameOverData>(msg.data).winner;
                    if (result == "YOU_WIN")
                    {
                        std::cout << "\n¡Felicidades! Has ganado la partida.\n";
                    }
                    else if (result == "YOU_LOSE")
                    {
                        std::cout << "\nHas perdido la partida. Mejor suerte la próxima vez.\n";
                    }
                    else
                    {
                        std::cout << "\nEl juego ha terminado. Resultado: " << result << "\n";
                    }
                    log("Received GAME_OVER", result, "INFO");
                    running_ = false;
                    break;
                }
                case BattleShipProtocol::MessageType::ERROR:
                {
                    std::string error_msg = std::get<BattleShipProtocol::ErrorData>(msg.data).description;
                    log("Received ERROR", error_msg, "ERROR");

                    if (error_msg.find("Not Player") != std::string::npos &&
                        error_msg.find("turn") != std::string::npos)
                    {
                        std::cerr << "Error: " << error_msg << std::endl;
                        std::cerr << "Por favor espera tu turno antes de disparar." << std::endl;
                    }
                    else if (error_msg.find("Invalid coordinate") != std::string::npos)
                    {
                        std::cerr << "Error: " << error_msg << std::endl;
                        std::cerr << "Por favor ingresa coordenadas válidas (A-J, 1-10)." << std::endl;
                    }
                    else if (error_msg.find("Client disconnected") != std::string::npos)
                    {
                        std::cerr << "Error: El servidor reporta que el cliente está desconectado." << std::endl;
                        running_ = false;
                    }
                    else
                    {
                        std::cerr << "Error del servidor: " << error_msg << std::endl;
                        if (error_msg.find("Game is already over") != std::string::npos ||
                            error_msg.find("Invalid player ID") != std::string::npos ||
                            error_msg.find("Server disconnected") != std::string::npos ||
                            error_msg.find("Time limit exceeded") != std::string::npos)
                        {
                            running_ = false;
                            std::cerr << "Error fatal. Saliendo del juego." << std::endl;
                        }
                        else
                        {
                            std::cerr << "Intentando continuar..." << std::endl;
                        }
                    }
                    break;
                }
                default:
                    log("Unexpected message", response, "ERROR");
                }
            }
            catch (const std::exception &e)
            {
                log("Receive failed", e.what(), "ERROR");
                running_ = false;
                break;
            }
        }
        log("receive_loop terminated", "running_: " + std::string(running_ ? "true" : "false"), "DEBUG");
    }

    void Client::send_message(const BattleShipProtocol::Message &msg) const
    {
        std::string data = protocol_.build_message(msg);
        ssize_t sent = send(client_fd_, data.c_str(), data.size(), 0);
        if (sent < 0)
        {
            throw ClientError("Send failed: " + std::string(strerror(errno)));
        }
        if (sent != static_cast<ssize_t>(data.size()))
        {
            throw ClientError("Incomplete send: sent " + std::to_string(sent) + " of " + std::to_string(data.size()) + " bytes");
        }
    }

    BattleShipProtocol::Message Client::receive_message()
    {
        char buffer[4096] = {0};
        std::string response;
        ssize_t received;

        while ((received = recv(client_fd_, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            response.append(buffer, received);
            if (response.find('\n') != std::string::npos)
            {
                break;
            }
        }

        if (received < 0)
        {
            throw ClientError("Receive failed: " + std::string(strerror(errno)));
        }
        if (received == 0)
        {
            throw ClientError("Server disconnected");
        }

        std::cout << "---------- REALMENTE RECIBIDO ------------ " << std::endl;
        std::cout << response << std::endl;
        std::cout << "---------- REALMENTE RECIBIDO------------ " << std::endl;

        log("Raw received", response, "DEBUG");

        size_t newline_pos = response.find('\n');
        if (newline_pos != std::string::npos)
        {
            response = response.substr(0, newline_pos + 1);
        }

        try
        {
            return protocol_.parse_message(response);
        }
        catch (const std::exception &e)
        {
            log("Failed to parse message", "Message: [" + response + "] Error: " + e.what(), "ERROR");
            throw;
        }
    }

    void Client::log(const std::string &query, const std::string &response, const std::string &level) const
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::time_t now = std::time(nullptr);
        std::tm *local_time = std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << " " << nickname_ << " " << query << " " << response;
        std::string log_entry = oss.str();
        std::cout << "[" << level << "] " << log_entry << std::endl;
        if (log_file_.is_open())
        {
            log_file_ << "[" << level << "] " << log_entry << "\n";
            log_file_.flush();
        }
    }

    BattleShipProtocol::PlaceShipsData Client::generate_initial_ships() const
    {
        std::vector<BattleShipProtocol::Ship> ships;
        std::vector<std::pair<BattleShipProtocol::ShipType, int>> ship_configs = {
            {BattleShipProtocol::ShipType::PORTAAVIONES, 5},
            {BattleShipProtocol::ShipType::BUQUE, 4},
            {BattleShipProtocol::ShipType::CRUCERO, 3},
            {BattleShipProtocol::ShipType::CRUCERO, 3},
            {BattleShipProtocol::ShipType::DESTRUCTOR, 2},
            {BattleShipProtocol::ShipType::DESTRUCTOR, 2},
            {BattleShipProtocol::ShipType::SUBMARINO, 1},
            {BattleShipProtocol::ShipType::SUBMARINO, 1},
            {BattleShipProtocol::ShipType::SUBMARINO, 1}};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::array<bool, 100> occupied{};

        auto is_valid_position = [&](const std::vector<BattleShipProtocol::Coordinate> &coords)
        {
            for (const auto &coord : coords)
            {
                int row = coord.letter[0] - 'A';
                int col = coord.number - 1;
                if (row < 0 || row >= 10 || col < 0 || col >= 10)
                    return false;
                if (occupied[row * 10 + col])
                    return false;
            }
            return true;
        };

        auto mark_occupied = [&](const std::vector<BattleShipProtocol::Coordinate> &coords)
        {
            for (const auto &coord : coords)
            {
                int row = coord.letter[0] - 'A';
                int col = coord.number - 1;
                occupied[row * 10 + col] = true;
            }
        };

        for (const auto &[type, size] : ship_configs)
        {
            bool placed = false;
            while (!placed)
            {
                bool horizontal = std::uniform_int_distribution<>(0, 1)(gen);
                int max_row = horizontal ? 10 : 11 - size;
                int max_col = horizontal ? 11 - size : 10;
                int row = std::uniform_int_distribution<>(0, max_row - 1)(gen);
                int col = std::uniform_int_distribution<>(0, max_col - 1)(gen);
                std::vector<BattleShipProtocol::Coordinate> coords;

                for (int i = 0; i < size; ++i)
                {
                    std::string letter(1, 'A' + (horizontal ? row : row + i));
                    int number = horizontal ? col + i + 1 : col + 1;
                    coords.push_back({letter, number});
                }

                if (is_valid_position(coords))
                {
                    ships.push_back({type, coords});
                    mark_occupied(coords);
                    placed = true;
                }
            }
        }

        return BattleShipProtocol::PlaceShipsData{ships};
    }

    BattleShipProtocol::PlaceShipsData Client::generate_manual_ships() const
    {
        std::vector<BattleShipProtocol::Ship> ships;
        std::array<bool, 100> occupied{};
        std::vector<std::pair<BattleShipProtocol::ShipType, int>> ship_configs = {
            {BattleShipProtocol::ShipType::PORTAAVIONES, 5},
            {BattleShipProtocol::ShipType::BUQUE, 4},
            {BattleShipProtocol::ShipType::CRUCERO, 3},
            {BattleShipProtocol::ShipType::CRUCERO, 3},
            {BattleShipProtocol::ShipType::DESTRUCTOR, 2},
            {BattleShipProtocol::ShipType::DESTRUCTOR, 2},
            {BattleShipProtocol::ShipType::SUBMARINO, 1},
            {BattleShipProtocol::ShipType::SUBMARINO, 1},
            {BattleShipProtocol::ShipType::SUBMARINO, 1}};

        auto ship_type_to_string = [](BattleShipProtocol::ShipType type) -> std::string
        {
            switch (type)
            {
            case BattleShipProtocol::ShipType::PORTAAVIONES:
                return "Portaaviones";
            case BattleShipProtocol::ShipType::BUQUE:
                return "Buque";
            case BattleShipProtocol::ShipType::CRUCERO:
                return "Crucero";
            case BattleShipProtocol::ShipType::DESTRUCTOR:
                return "Destructor";
            case BattleShipProtocol::ShipType::SUBMARINO:
                return "Submarino";
            default:
                return "Desconocido";
            }
        };

        auto is_valid_coord = [](const std::string &letter, int number)
        {
            return letter >= "A" && letter <= "J" && number >= 1 && number <= 10;
        };

        auto mark_occupied = [&](const std::vector<BattleShipProtocol::Coordinate> &coords)
        {
            for (const auto &coord : coords)
            {
                int row = coord.letter[0] - 'A';
                int col = coord.number - 1;
                occupied[row * 10 + col] = true;
            }
        };

        auto is_position_free = [&](const std::vector<BattleShipProtocol::Coordinate> &coords)
        {
            for (const auto &coord : coords)
            {
                int row = coord.letter[0] - 'A';
                int col = coord.number - 1;
                if (row < 0 || row >= 10 || col < 0 || col >= 10)
                    return false;
                if (occupied[row * 10 + col])
                    return false;
            }
            return true;
        };

        std::cout << "Colocación manual de barcos. Ingresa coordenadas en formato <letra><número> (ejemplo: A1) y orientación (H para horizontal, V para vertical, no aplica para submarinos).\n";

        for (const auto &[type, size] : ship_configs)
        {
            bool placed = false;
            while (!placed)
            {
                std::cout << "\nColocando " << ship_type_to_string(type) << " (tamaño: " << size << "):\n";
                std::string input;
                std::cout << "Ingresa coordenada inicial (ejemplo: A1): ";
                std::getline(std::cin, input);

                if (input.empty())
                {
                    std::cout << "[ERROR] Entrada vacía. Intenta de nuevo.\n";
                    continue;
                }

                std::string letter;
                int number;
                try
                {
                    letter = input.substr(0, 1);
                    number = std::stoi(input.substr(1));
                }
                catch (const std::exception &e)
                {
                    std::cout << "[ERROR] Formato inválido. Usa <letra><número> (ejemplo: A1).\n";
                    continue;
                }

                if (!is_valid_coord(letter, number))
                {
                    std::cout << "[ERROR] Coordenada inválida. Usa A-J y 1-10.\n";
                    continue;
                }

                std::vector<BattleShipProtocol::Coordinate> coords;
                coords.push_back({letter, number});

                if (size > 1)
                {
                    std::string orientation;
                    std::cout << "Ingresa orientación (H para horizontal, V para vertical): ";
                    std::getline(std::cin, orientation);
                    if (orientation != "H" && orientation != "V")
                    {
                        std::cout << "[ERROR] Orientación inválida. Usa H o V.\n";
                        continue;
                    }

                    bool horizontal = (orientation == "H");
                    for (int i = 1; i < size; ++i)
                    {
                        std::string next_letter = horizontal ? letter : std::string(1, letter[0] + i);
                        int next_number = horizontal ? number + i : number;
                        if (!is_valid_coord(next_letter, next_number))
                        {
                            std::cout << "[ERROR] Las coordenadas exceden el tablero (A-J, 1-10).\n";
                            break;
                        }
                        coords.push_back({next_letter, next_number});
                    }
                    if (coords.size() != static_cast<size_t>(size))
                    {
                        continue;
                    }
                }

                if (!is_position_free(coords))
                {
                    std::cout << "[ERROR] Las coordenadas están ocupadas o inválidas. Intenta de nuevo.\n";
                    continue;
                }

                ships.push_back({type, coords});
                mark_occupied(coords);
                placed = true;
                std::cout << "Barco colocado correctamente.\n";
            }
        }

        std::cout << "Todos los barcos han sido colocados.\n";
        return BattleShipProtocol::PlaceShipsData{ships};
    }

    std::string Client::cellStateToString(BattleShipProtocol::CellState state) const
    {
        switch (state)
        {
        case BattleShipProtocol::CellState::WATER:
            return "Water";
        case BattleShipProtocol::CellState::HIT:
            return "Hit";
        case BattleShipProtocol::CellState::SUNK:
            return "Sunk";
        case BattleShipProtocol::CellState::SHIP:
            return "Ship";
        case BattleShipProtocol::CellState::MISS:
            return "Miss";
        default:
            return "Unknown";
        }
    }

    void Client::display_game_state() const
    {
        log("Starting display_game_state", "Player: " + nickname_, "DEBUG");

        std::cout << "\n===============\n";
        std::cout << "=== Estado del Juego para " << nickname_ << " (Jugador " << player_id_ << ") ===\n";

        std::cout << "Estado del Juego: ";
        switch (last_status_.gameState)
        {
        case BattleShipProtocol::GameState::ONGOING:
            std::cout << "En curso - ";
            std::cout << (last_status_.turn == BattleShipProtocol::Turn::YOUR_TURN ? "Tu Turno" : "Turno del Oponente");
            break;
        case BattleShipProtocol::GameState::WAITING:
            std::cout << "Esperando a que ambos jugadores estén listos";
            break;
        case BattleShipProtocol::GameState::ENDED:
            std::cout << "Juego terminado";
            break;
        default:
            std::cout << "Estado desconocido";
            break;
        }
        std::cout << "\n";

        // Mostrar temporizador
        if (last_status_.gameState == BattleShipProtocol::GameState::ONGOING)
        {
            std::cout << "Tiempo restante: ";
            if (last_status_.time_remaining > 0)
            {
                int minutes = last_status_.time_remaining / 60;
                int seconds = last_status_.time_remaining % 60;
                std::cout << std::setfill('0') << std::setw(2) << minutes << ":"
                          << std::setfill('0') << std::setw(2) << seconds;
            }
            else
            {
                std::cout << "00:00";
            }
            std::cout << " (" << (last_status_.turn == BattleShipProtocol::Turn::YOUR_TURN ? "Tu turno" : "Turno del oponente") << ")\n";
        }

        std::cout << "\nTu Tablero (10x10):\n";
        std::cout << "   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |\n";
        std::cout << "---+---+---+---+---+---+---+---+---+---+---+\n";
        for (char row = 'A'; row <= 'J'; ++row)
        {
            std::cout << row << " |";
            for (int col = 1; col <= 10; ++col)
            {
                bool found = false;
                for (const auto &cell : last_status_.boardOwn)
                {
                    if (cell.coordinate.letter == std::string(1, row) && cell.coordinate.number == col)
                    {
                        switch (cell.cellState)
                        {
                        case BattleShipProtocol::CellState::SHIP:
                            std::cout << " S |";
                            break;
                        case BattleShipProtocol::CellState::HIT:
                            std::cout << " H |";
                            break;
                        case BattleShipProtocol::CellState::SUNK:
                            std::cout << " X |";
                            break;
                        case BattleShipProtocol::CellState::WATER:
                            std::cout << " ~ |";
                            break;
                        case BattleShipProtocol::CellState::MISS:
                            std::cout << " M |";
                            break;
                        default:
                            std::cout << " ? |";
                            break;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found)
                    std::cout << " ~ |";
            }
            std::cout << "\n---+---+---+---++++---+---+---+---+---+\n";
        }

        std::cout << "\nTablero del Oponente (10x10):\n";
        std::cout << "   | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |\n";
        std::cout << "---+---+---+---+---+---+---+---+---+---+---+\n";
        for (char row = 'A'; row <= 'J'; ++row)
        {
            std::cout << row << " |";
            for (int col = 1; col <= 10; ++col)
            {
                bool found = false;
                for (const auto &cell : last_status_.boardOpponent)
                {
                    if (cell.coordinate.letter == std::string(1, row) && cell.coordinate.number == col)
                    {
                        switch (cell.cellState)
                        {
                        case BattleShipProtocol::CellState::HIT:
                            std::cout << " H |";
                            break;
                        case BattleShipProtocol::CellState::SUNK:
                            std::cout << " X |";
                            break;
                        case BattleShipProtocol::CellState::MISS:
                            std::cout << " M |";
                            break;
                        case BattleShipProtocol::CellState::WATER:
                        case BattleShipProtocol::CellState::SHIP:
                        default:
                            std::cout << " ~ |";
                            break;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found)
                    std::cout << " ~ |";
            }
            std::cout << "\n---+---+---+---+---+---+---+---+---+---+---+\n";
        }
        std::cout << "\nLeyenda: S=Barco, H=Golpeado, X=Hundido, M=Fallo, ~=Agua\n";

        std::cout << "\nOpciones:\n";
        if (last_status_.gameState == BattleShipProtocol::GameState::ONGOING &&
            last_status_.turn == BattleShipProtocol::Turn::YOUR_TURN)
        {
            std::cout << "  1. Disparar (Ingresa 'SHOOT <letra><número>', ej. 'SHOOT A1')\n";
            std::cout << "  4. Rendirse (Ingresa 'SURRENDER' o '4')\n";
        }
        std::cout << "  2. Ver historial de disparos\n";
        std::cout << "  3. Salir\n";
        std::cout << "Ingresa tu elección: ";

        log("Finished rendering boards and menu", "Player: " + nickname_, "DEBUG");
    }

    void Client::display_shot_history() const
    {
        std::cout << "\n=== Historial de Disparos ===\n";
        if (shot_history_.empty())
        {
            std::cout << "Aún no se han realizado disparos.\n";
        }
        else
        {
            std::cout << "Coordenada | Resultado\n";
            std::cout << "-----------+----------\n";
            for (const auto &shot : shot_history_)
            {
                std::string letter = shot.first;
                int number = shot.second;
                std::string result = "Desconocido";

                for (const auto &cell : last_status_.boardOpponent)
                {
                    if (cell.coordinate.letter == letter && cell.coordinate.number == number)
                    {
                        switch (cell.cellState)
                        {
                        case BattleShipProtocol::CellState::HIT:
                            result = "Golpeado";
                            break;
                        case BattleShipProtocol::CellState::SUNK:
                            result = "Hundido";
                            break;
                        case BattleShipProtocol::CellState::MISS:
                            result = "Fallo";
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                }
                std::cout << letter << number << "      | " << result << "\n";
            }
        }
        std::cout << "====================\n";
    }

} // namespace BattleshipClient