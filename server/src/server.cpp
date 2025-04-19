#include "server.hpp"
#include <iostream>
#include <set>
#include <iterator>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace BattleshipServer
{
    GameSession::GameSession(int session_id) : session_id_(session_id), game_(std::make_unique<BattleShipProtocol::GameLogic>()) {}
    GameSession::~GameSession()
    {
        for (auto &[id, info] : players_)
        {
            if (info.first > 0)
            {
                close(info.first);
            }
        }
        if (session_thread_.joinable())
        {
            session_thread_.join();
        }
    }

    void GameSession::add_player(int player_id, int client_fd, const std::string &client_ip)
    {
        if (is_full())
        {
            throw ServerError("Session " + std::to_string(session_id_) + " is already full");
        }
        players_[player_id] = {client_fd, client_ip};

        try
        {
            BattleShipProtocol::Message player_id_msg{
                BattleShipProtocol::MessageType::PLAYER_ID,
                BattleShipProtocol::PlayerIdData{player_id}};
            send_message(client_fd, player_id_msg, protocol_);
            std::cout << "Enviado PLAYER_ID a " << client_ip << ": " << protocol_.build_message(player_id_msg) << std::endl;
            if (!log_fn_)
            {
                std::cerr << "Error: log_fn_ está vacío en add_player" << std::endl;
                throw ServerError("Logging function is not initialized");
            }
            log_fn_(client_ip, protocol_.build_message(player_id_msg), "Player " + std::to_string(player_id) + " assigned", "INFO");
        }
        catch (const std::exception &e)
        {
            if (log_fn_)
            {
                log_fn_(client_ip, "PLAYER_ID assignment failed", e.what(), "ERROR");
            }
            else
            {
                std::cerr << "Error: log_fn_ vacío, no se puede loguear: " << e.what() << std::endl;
            }
            throw;
        }
    }

    int GameSession::get_client_fd(int player_id) const
    {
        return players_.at(player_id).first;
    }

    std::string GameSession::get_player_ip(int player_id) const
    {
        return players_.at(player_id).second;
    }

    void GameSession::start(const BattleShipProtocol::Protocol &protocol,
                            std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> log_fn)
    {
        protocol_ = protocol;
        log_fn_ = log_fn;
        session_thread_ = std::thread(&GameSession::run_session, this, std::cref(protocol), log_fn_);
    }

    void GameSession::run_session(const BattleShipProtocol::Protocol &protocol,
                                  std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> log_fn)
    {
        auto send_fn = [&](int fd, const BattleShipProtocol::Message &msg)
        { send_message(fd, msg, protocol_); };

        auto handle_disconnect = [&](int player_id, const std::string &client_ip, const std::string &reason)
        {
            std::cout << "[DEBUG] Jugador " << player_id << " desconectado: " << reason << std::endl;
            log_fn(client_ip, "Client disconnected", reason, "ERROR");

            if (players_[player_id].first > 0)
            {
                close(players_[player_id].first);
                players_[player_id].first = -1;
            }

            int remaining_player = (player_id == 1) ? 2 : 1;
            int remaining_fd = players_[remaining_player].first;
            std::string remaining_ip = players_[remaining_player].second;

            if (remaining_fd > 0)
            {
                try
                {
                    send_fn(remaining_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Opponent disconnected"}});
                    close(remaining_fd);
                    players_[remaining_player].first = -1;
                }
                catch (const std::exception &e)
                {
                    log_fn(remaining_ip, "Failed to notify", e.what(), "ERROR");
                }
            }

            finished_ = true;
        };

        auto send_status = [&](int player_id, int current_turn)
        {
            int client_fd = players_.at(player_id).first;
            std::string client_ip = players_.at(player_id).second;
            if (client_fd <= 0)
                return;

            try
            {
                auto status = game_->get_status(player_id);
                int time_remaining = 0;
                if (game_->get_phase() == BattleShipProtocol::PhaseState::Phase::PLAYING)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - turn_start_time_).count();
                    time_remaining = std::max(0, TURN_TIMEOUT_SECONDS - static_cast<int>(elapsed));
                }

                BattleShipProtocol::Turn turn_view = (player_id == current_turn)
                                                         ? BattleShipProtocol::Turn::YOUR_TURN
                                                         : BattleShipProtocol::Turn::OPPONENT_TURN;

                BattleShipProtocol::Message status_msg{
                    BattleShipProtocol::MessageType::STATUS,
                    BattleShipProtocol::StatusData{
                        turn_view,
                        status.boardOwn,
                        status.boardOpponent,
                        status.gameState,
                        time_remaining}};

                send_message(client_fd, status_msg, protocol_);
                log_fn(client_ip, protocol_.build_message(status_msg), "Status sent", "INFO");
            }
            catch (const std::exception &e)
            {
                log_fn(client_ip, "Failed to send status", e.what(), "ERROR");
            }
        };

        try
        {
            // Fase de Registro
            std::cout << "[DEBUG] Iniciando fase REGISTRATION para sesión " << session_id_ << std::endl;
            std::set<int> registered_players;
            while (registered_players.size() < 2 && !finished_)
            {
                for (int i = 1; i <= 2; ++i)
                {
                    if (registered_players.count(i))
                        continue;
                    int client_fd = players_.at(i).first;
                    std::string client_ip = players_.at(i).second;
                    try
                    {
                        std::cout << "[DEBUG] Esperando REGISTER de jugador " << i << " (client_fd: " << client_fd << ")" << std::endl;
                        auto messages = receive_messages(client_fd);
                        for (const auto &msg : messages)
                        {
                            if (game_->get_phase() != BattleShipProtocol::PhaseState::Phase::REGISTRATION)
                            {
                                std::cerr << "[ERROR] Fase inválida para mensaje recibido en REGISTRATION" << std::endl;
                                send_fn(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Mensaje recibido en fase incorrecta"}});
                                continue;
                            }
                            if (msg.type != BattleShipProtocol::MessageType::REGISTER)
                            {
                                std::cerr << "[ERROR] Mensaje inesperado en REGISTRATION: " << protocol_.build_message(msg) << std::endl;
                                send_fn(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Esperado REGISTER"}});
                                continue;
                            }
                            game_->register_player(i, std::get<BattleShipProtocol::RegisterData>(msg.data));
                            std::cout << "[DEBUG] Jugador " << i << " registrado correctamente" << std::endl;
                            log_fn(client_ip, protocol_.build_message(msg), "Player " + std::to_string(i) + " registered", "INFO");
                            registered_players.insert(i);
                            break;
                        }
                    }
                    catch (const BattleShipProtocol::ProtocolError &e)
                    {
                        handle_disconnect(i, client_ip, e.what());
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[ERROR] Error inesperado en REGISTRATION para jugador " << i << ": " << e.what() << std::endl;
                        log_fn(client_ip, "Unexpected error in REGISTRATION", e.what(), "ERROR");
                        handle_disconnect(i, client_ip, "Unexpected error: " + std::string(e.what()));
                        return;
                    }
                }
            }

            // Fase de Colocación de Barcos
            std::cout << "[DEBUG] Transicionando a fase PLACEMENT para sesión " << session_id_ << std::endl;
            game_->transition_to_placement();
            std::set<int> placed_ships;
            while (placed_ships.size() < 2 && !finished_)
            {
                for (int i = 1; i <= 2; ++i)
                {
                    if (placed_ships.count(i))
                        continue;
                    int client_fd = players_.at(i).first;
                    std::string client_ip = players_.at(i).second;
                    try
                    {
                        std::cout << "[DEBUG] Esperando PLACE_SHIPS de jugador " << i << " (client_fd: " << client_fd << ")" << std::endl;
                        auto messages = receive_messages(client_fd);
                        for (const auto &msg : messages)
                        {
                            if (game_->get_phase() != BattleShipProtocol::PhaseState::Phase::PLACEMENT)
                            {
                                std::cerr << "[ERROR] Fase inválida para mensaje recibido en PLACEMENT" << std::endl;
                                send_fn(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Mensaje recibido en fase incorrecta"}});
                                continue;
                            }
                            if (msg.type != BattleShipProtocol::MessageType::PLACE_SHIPS)
                            {
                                std::cerr << "[ERROR] Mensaje inesperado en PLACEMENT: " << protocol_.build_message(msg) << std::endl;
                                send_fn(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Esperado PLACE_SHIPS"}});
                                continue;
                            }
                            game_->place_ships(i, std::get<BattleShipProtocol::PlaceShipsData>(msg.data));
                            std::cout << "[DEBUG] Jugador " << i << " colocó barcos correctamente" << std::endl;
                            log_fn(client_ip, protocol_.build_message(msg), "Ships placed", "INFO");
                            placed_ships.insert(i);
                            break;
                        }
                    }
                    catch (const BattleShipProtocol::ProtocolError &e)
                    {
                        handle_disconnect(i, client_ip, e.what());
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[ERROR] Error inesperado en PLACEMENT para jugador " << i << ": " << e.what() << std::endl;
                        log_fn(client_ip, "Unexpected error in PLACEMENT", e.what(), "ERROR");
                        handle_disconnect(i, client_ip, "Unexpected error: " + std::string(e.what()));
                        return;
                    }
                }
            }

            // Transición a PLAYING
            std::cout << "[DEBUG] Transicionando a fase PLAYING para sesión " << session_id_ << std::endl;
            game_->transition_to_playing();

            int current_player = 1;
            for (int i = 1; i <= 2; ++i)
            {
                send_status(i, current_player);
            }

            // Fase de Juego
            std::cout << "[DEBUG] Iniciando fase PLAYING, turno inicial: Jugador " << current_player << std::endl;

            while (!finished_)
            {
                int client_fd = players_.at(current_player).first;
                std::string client_ip = players_.at(current_player).second;
                turn_start_time_ = std::chrono::steady_clock::now();

                bool turn_finished = false;

                while (!turn_finished && !finished_)
                {
                    try
                    {
                        auto now = std::chrono::steady_clock::now();
                        int elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - turn_start_time_).count();

                        // ⏳ Verificamos timeout manualmente
                        if (elapsed >= TURN_TIMEOUT_SECONDS)
                        {
                            std::cout << "[TIMEOUT] Jugador " << current_player << " perdió el turno\n";
                            log_fn(client_ip, "Turn timeout", "Turno perdido", "INFO");

                            current_player = (current_player == 1) ? 2 : 1;
                            turn_start_time_ = std::chrono::steady_clock::now();

                            for (int i = 1; i <= 2; ++i)
                                send_status(i, current_player);

                            turn_finished = true;
                            break;
                        }

                        // 🔍 Verificamos si hay datos del jugador actual
                        fd_set read_fds;
                        FD_ZERO(&read_fds);
                        FD_SET(client_fd, &read_fds);

                        struct timeval timeout;
                        timeout.tv_sec = 1;
                        timeout.tv_usec = 0;

                        int result = select(client_fd + 1, &read_fds, nullptr, nullptr, &timeout);
                        if (result < 0 && errno != EINTR)
                        {
                            std::cerr << "[ERROR] select() falló: " << strerror(errno) << std::endl;
                            handle_disconnect(current_player, client_ip, "select() failed: " + std::string(strerror(errno)));
                            return;
                        }

                        if (result == 0)
                        {
                            continue;
                        }

                        if (FD_ISSET(client_fd, &read_fds))
                        {
                            auto messages = receive_messages(client_fd);
                            for (const auto &msg : messages)
                            {
                                if (msg.type == BattleShipProtocol::MessageType::SURRENDER)
                                {
                                    std::cout << "[DEBUG] Jugador " << current_player << " se rinde\n";
                                    game_->transition_to_finished();

                                    int winner = (current_player == 1) ? 2 : 1;
                                    send_message(players_.at(winner).first, {BattleShipProtocol::MessageType::GAME_OVER, BattleShipProtocol::GameOverData{"YOU_WIN"}}, protocol_);
                                    send_message(players_.at(current_player).first, {BattleShipProtocol::MessageType::GAME_OVER, BattleShipProtocol::GameOverData{"YOU_LOSE"}}, protocol_);
                                    finished_ = true;
                                    return;
                                }

                                if (msg.type != BattleShipProtocol::MessageType::SHOOT)
                                {
                                    send_message(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, "Esperado SHOOT"}}, protocol_);
                                    continue;
                                }

                                try
                                {
                                    auto shoot_data = std::get<BattleShipProtocol::ShootData>(msg.data);
                                    game_->process_shot(current_player, shoot_data);
                                    current_player = (current_player == 1) ? 2 : 1;
                                    turn_start_time_ = std::chrono::steady_clock::now();

                                    for (int i = 1; i <= 2; ++i)
                                    {
                                        send_status(i, current_player);
                                    }

                                    if (game_->is_game_over())
                                    {
                                        game_->transition_to_finished();
                                        int winner_id = current_player;
                                        int loser_id = (current_player == 1) ? 2 : 1;

                                        send_message(players_.at(winner_id).first, {BattleShipProtocol::MessageType::GAME_OVER, BattleShipProtocol::GameOverData{"YOU_WIN"}}, protocol_);
                                        send_message(players_.at(loser_id).first, {BattleShipProtocol::MessageType::GAME_OVER, BattleShipProtocol::GameOverData{"YOU_LOSE"}}, protocol_);
                                        finished_ = true;
                                        return;
                                    }

                                    turn_finished = true;
                                    break;
                                }
                                catch (const BattleShipProtocol::GameLogicError &e)
                                {
                                    send_message(client_fd, {BattleShipProtocol::MessageType::ERROR, BattleShipProtocol::ErrorData{400, e.what()}}, protocol_);
                                }
                            }
                        }
                    }
                    catch (const BattleShipProtocol::ProtocolError &e)
                    {
                        handle_disconnect(current_player, client_ip, e.what());
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[ERROR] Error inesperado en PLAYING para jugador " << current_player << ": " << e.what() << std::endl;
                        log_fn(client_ip, "Unexpected error in PLAYING", e.what(), "ERROR");
                        handle_disconnect(current_player, client_ip, "Unexpected error: " + std::string(e.what()));
                        return;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Error crítico en run_session: " << e.what() << std::endl;
            log_fn("0.0.0.0", "Critical error in run_session", e.what(), "ERROR");
            finished_ = true;
        }
    }

    Server::Server(const std::string &ip, int port, const std::string &log_path)
        : ip_(ip), port_(port), server_fd_(-1)
    {
        address_.sin_family = AF_INET;
        if (inet_pton(AF_INET, ip.c_str(), &address_.sin_addr) <= 0)
        {
            throw ServerError("Invalid IP address: " + ip);
        }

        address_.sin_port = htons(port);

        log_file_.open(log_path, std::ios::app);
        if (!log_file_.is_open())
        {
            throw ServerError("Failed to open log file: " + log_path);
        }
    }

    Server::~Server()
    {
        running_ = false;
        if (server_fd_ != -1)
            close(server_fd_);
        if (log_file_.is_open())
            log_file_.close();
    }

    void Server::run()
    {
        server_fd_ = create_socket();
        bind_socket();
        listen_connections();

        log("0.0.0.0", "Server started", ip_ + ":" + std::to_string(port_));
        std::thread acceptor_thread(&Server::accept_clients, this);
        std::thread cleanup_thread(&Server::cleanup_finished_sessions, this);

        acceptor_thread.join();
        cleanup_thread.join();
    }

    int Server::create_socket()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            throw ServerError("Failed to create socket: " + std::string(strerror(errno)));
        }
        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            close(fd);
            throw ServerError("Failed to set socket options: " + std::string(strerror(errno)));
        }
        return fd;
    }

    void Server::bind_socket()
    {
        if (bind(server_fd_, (struct sockaddr *)&address_, sizeof(address_)) < 0)
        {
            throw ServerError("Bind failed: " + std::string(strerror(errno)));
        }
    }

    void Server::listen_connections()
    {
        if (listen(server_fd_, 10) < 0)
        {
            throw ServerError("Listen failed: " + std::string(strerror(errno)));
        }
    }

    void Server::accept_clients()
    {
        while (running_)
        {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd < 0)
            {
                if (running_)
                    log("0.0.0.0", "Accept failed", strerror(errno), "ERROR");
                continue;
            }
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            log(client_ip, "Client connected", "Assigning to session");

            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_clients_.push(client_fd);
            if (pending_clients_.size() >= 2)
            {
                int fd1 = pending_clients_.front();
                pending_clients_.pop();
                int fd2 = pending_clients_.front();
                pending_clients_.pop();

                struct sockaddr_in addr1, addr2;
                socklen_t len1 = sizeof(addr1), len2 = sizeof(addr2);
                getpeername(fd1, (struct sockaddr *)&addr1, &len1);
                getpeername(fd2, (struct sockaddr *)&addr2, &len2);
                char ip1[INET_ADDRSTRLEN], ip2[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &addr1.sin_addr, ip1, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &addr2.sin_addr, ip2, INET_ADDRSTRLEN);

                std::lock_guard<std::mutex> session_lock(sessions_mutex_);
                auto session = std::make_unique<GameSession>(next_session_id_++);

                session->start(protocol_, [this](const auto &ip, const auto &q, const auto &r, const auto &l)
                               { log(ip, q, r, l); });
                session->add_player(1, fd1, ip1);
                session->add_player(2, fd2, ip2);

                sessions_[session->get_session_id()] = std::move(session);
            }
        }
    }

    void Server::cleanup_finished_sessions()
    {
        while (running_)
        {
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                for (auto it = sessions_.begin(); it != sessions_.end();)
                {
                    if (it->second->is_finished())
                    {
                        it = sessions_.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void GameSession::send_message(int client_fd, const BattleShipProtocol::Message &msg, const BattleShipProtocol::Protocol &protocol) const
    {
        std::string data = protocol_.build_message(msg);
        std::cout << "-------------------- SERVER ENVIO ------------------------" << std::endl;
        std::cout << data << std::endl;
        std::cout << "-------------------- SERVER ENVIO ------------------------" << std::endl;

        size_t num_bytes = data.size();
        size_t total_sent = 0;
        size_t buffer_size = 4096;
        char buffer[buffer_size];

        while (total_sent < num_bytes)
        {
            size_t remaining = num_bytes - total_sent;
            size_t to_send = remaining > buffer_size ? buffer_size : remaining;

            data.copy(buffer, to_send, total_sent);

            ssize_t sent = send(client_fd, buffer, to_send, 0);
            if (sent < 0)
            {
                throw ServerError("Send failed: " + std::string(strerror(errno)));
            }
            total_sent += sent;
        }
    }

    std::vector<BattleShipProtocol::Message> GameSession::receive_messages(int client_fd) const
    {
        char buffer[4096] = {0};
        std::string response;
        ssize_t received;

        while ((received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
        {
            response.append(buffer, received);
            std::vector<BattleShipProtocol::Message> messages;
            size_t pos = 0;
            while ((pos = response.find('\n')) != std::string::npos)
            {
                std::string msg_str = response.substr(0, pos + 1);
                if (!msg_str.empty())
                {
                    try
                    {
                        messages.push_back(protocol_.parse_message(msg_str));
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[ERROR] Failed to parse message: [" << msg_str << "] Error: " << e.what() << std::endl;
                        throw BattleShipProtocol::ProtocolError("Failed to parse message: " + std::string(e.what()));
                    }
                }
                response.erase(0, pos + 1);
            }
            if (!messages.empty())
            {
                std::cout << "[DEBUG] Received " << messages.size() << " messages from client_fd " << client_fd << std::endl;
                return messages;
            }
        }

        if (received < 0)
        {
            throw BattleShipProtocol::ProtocolError("Receive failed: " + std::string(strerror(errno)));
        }
        if (received == 0)
        {
            throw BattleShipProtocol::ProtocolError("Client disconnected");
        }

        return {};
    }

    void Server::log(const std::string &client_ip, const std::string &query, const std::string &response,
                     const std::string &level) const
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::time_t now = std::time(nullptr);
        std::tm *local_time = std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << " " << client_ip << " " << query << " " << response;
        std::string log_entry = oss.str();
        std::cout << "[" << level << "] " << log_entry << std::endl;
        if (log_file_.is_open())
        {
            log_file_ << "[" << level << "] " << log_entry << "\n";
            log_file_.flush();
        }
    }

} // namespace BattleshipServer