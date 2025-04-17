#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <map>
#include <functional>
#include "../../protocol/include/protocol.hpp"
#include "../../protocol/include/game_logic.hpp"

namespace BattleshipServer
{

    /**
     * @class ServerError
     * @brief Exception class for server-related errors.
     */
    class ServerError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a ServerError with a message.
         * @param msg Error message.
         */
        explicit ServerError(const std::string &msg) : std::runtime_error(msg) {}
    };

    /**
     * @class GameSession
     * @brief Manages a single Battleship game session between two players.
     */
    class GameSession
    {
    public:
        /**
         * @brief Constructs a new GameSession with a unique session ID.
         * @param session_id Unique identifier for the session.
         */
        explicit GameSession(int session_id);

        /**
         * @brief Destructor. Ensures the session thread is joined.
         */
        ~GameSession();

        /**
         * @brief Adds a player to the game session.
         * @param player_id ID of the player (1 or 2).
         * @param client_fd File descriptor of the client socket.
         * @param client_ip IP address of the client.
         */
        void add_player(int player_id, int client_fd, const std::string &client_ip);

        /**
         * @brief Starts the game session thread.
         * @param protocol Reference to the game protocol.
         * @param log_fn Logging function for game events.
         */
        void start(const BattleShipProtocol::Protocol &protocol,
                   std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> log_fn);

        /**
         * @brief Gets the IP address of a given player.
         * @param player_id ID of the player.
         * @return IP address as string.
         */
        std::string get_player_ip(int player_id) const;

        /**
         * @brief Returns the unique session ID.
         * @return Session ID.
         */
        int get_session_id() const noexcept { return session_id_; }

        /**
         * @brief Checks if the session is full (2 players connected).
         * @return True if full, false otherwise.
         */
        size_t is_full() const noexcept { return players_.size() == 2; }

        /**
         * @brief Checks if the session is finished.
         * @return True if finished, false otherwise.
         */
        bool is_finished() const noexcept { return finished_; }

        /**
         * @brief Gets the client socket file descriptor for a player.
         * @param player_id ID of the player.
         * @return Socket file descriptor.
         */
        int get_client_fd(int player_id) const;

    private:
        int session_id_;                                                                                                 ///< Unique ID for the session.
        std::map<int, std::pair<int, std::string>> players_;                                                             ///< Map of player ID to (socket, IP).
        std::unique_ptr<BattleShipProtocol::GameLogic> game_;                                                            ///< Game logic handler.
        std::thread session_thread_;                                                                                     ///< Thread running the game session.
        std::atomic<bool> finished_{false};                                                                              ///< Flag to indicate if the session is over.
        BattleShipProtocol::Protocol protocol_;                                                                          ///< Communication protocol.
        std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> log_fn_; ///< Logging function.
        std::chrono::time_point<std::chrono::steady_clock> turn_start_time_;                                             ///< Start time of current turn.
        static constexpr int TURN_TIMEOUT_SECONDS = 30;                                                                  ///< Timeout for player turn in seconds.

        /**
         * @brief Main loop for handling the game session.
         *
         * This function runs the core game logic between the two players. It handles turns,
         * communication, timeouts, and game flow using the provided protocol and logs each interaction.
         *
         * @param protocol Reference to the communication protocol used for parsing and sending messages.
         * @param log_fn Callback function to log events with signature (client_ip, query, response, level).
         */
        void run_session(const BattleShipProtocol::Protocol &protocol,
                         std::function<void(const std::string &, const std::string &, const std::string &, const std::string &)> log_fn);

        /**
         * @brief Sends a protocol message to a specific client.
         * @param client_fd File descriptor of the client's socket.
         * @param msg Message object to be sent.
         * @param protocol Reference to the protocol handler used for serialization.
         */
        void send_message(int client_fd, const BattleShipProtocol::Message &msg,
                          const BattleShipProtocol::Protocol &protocol) const;

        /**
         * @brief Receives all pending messages from a client.
         * @param client_fd File descriptor of the client socket.
         * @return Vector of parsed protocol messages.
         */
        std::vector<BattleShipProtocol::Message> receive_messages(int client_fd) const;
    };

    /**
     * @class Server
     * @brief Handles socket setup, client management, and session coordination for the Battleship game.
     */
    class Server
    {
    public:
        /**
         * @brief Constructs a server instance with IP, port, and log file path.
         * @param ip IP address to bind.
         * @param port Port to bind.
         * @param log_path File path to write logs.
         */
        Server(const std::string &ip, int port, const std::string &log_path);

        /**
         * @brief Destructor. Cleans up resources and closes sockets.
         */
        ~Server();

        /**
         * @brief Starts the server's main loop.
         */
        void run();

    private:
        int server_fd_;                                        ///< Server socket file descriptor.
        struct sockaddr_in address_;                           ///< Socket address structure.
        std::string ip_;                                       ///< Server IP address.
        int port_;                                             ///< Server port number.
        mutable std::ofstream log_file_;                       ///< Output stream for logging.
        mutable std::mutex log_mutex_;                         ///< Mutex for thread-safe logging.
        BattleShipProtocol::Protocol protocol_;                ///< Protocol handler instance.
        std::map<int, std::unique_ptr<GameSession>> sessions_; ///< Active game sessions.
        std::mutex sessions_mutex_;                            ///< Mutex for session map.
        std::queue<int> pending_clients_;                      ///< Queue of clients waiting for a session.
        std::mutex pending_mutex_;                             ///< Mutex for pending client queue.
        std::atomic<bool> running_{true};                      ///< Server running flag.
        int next_session_id_{1};                               ///< Counter for assigning session IDs.

        /**
         * @brief Creates the server socket.
         * @return Socket file descriptor.
         */
        int create_socket();

        /**
         * @brief Binds the server socket to the specified address and port.
         */
        void bind_socket();

        /**
         * @brief Starts listening for incoming connections.
         */
        void listen_connections();

        /**
         * @brief Accepts client connections and enqueues them for session pairing.
         */
        void accept_clients();

        /**
         * @brief Cleans up finished game sessions.
         */
        void cleanup_finished_sessions();

        /**
         * @brief Logs a message to the log file with a given log level.
         * @param client_ip Client IP address.
         * @param query Original request or action.
         * @param response Server response or outcome.
         * @param level Log severity level (default is "INFO").
         */
        void log(const std::string &client_ip, const std::string &query,
                 const std::string &response, const std::string &level = "INFO") const;
    };

} // namespace BattleshipServer

#endif