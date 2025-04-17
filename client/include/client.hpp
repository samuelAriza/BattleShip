#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <set>
#include <utility>
#include "../../protocol/include/protocol.hpp"

namespace BattleshipClient
{

    /**
     * @brief Exception thrown when a client-related error occurs.
     */
    class ClientError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a ClientError with a message.
         * @param msg Error message.
         */
        explicit ClientError(const std::string &msg) : std::runtime_error(msg) {}
    };

    /**
     * @brief Client class for connecting and interacting with the Battleship server.
     */
    class Client
    {
    public:
        /**
         * @brief Constructs a new client instance.
         * @param server_ip IP address of the server.
         * @param server_port Port number of the server.
         * @param nickname Player nickname.
         * @param email Player email address.
         * @param log_path Path to the log file.
         */
        Client(const std::string &server_ip, int server_port, const std::string &nickname, const std::string &email, const std::string &log_path);

        /**
         * @brief Destructor. Cleans up threads and resources.
         */
        ~Client();

        /**
         * @brief Starts the client's main loop.
         */
        void run();

    private:
        /**
         * @brief Manually generates ship placements.
         * @return PlaceShipsData with manually assigned ships.
         */
        BattleShipProtocol::PlaceShipsData generate_manual_ships() const;

        std::set<std::pair<std::string, int>> shot_history_; ///< Record of shots made.

        int client_fd_;                              ///< Client socket file descriptor.
        struct sockaddr_in server_addr_;             ///< Server address structure.
        std::string server_ip_;                      ///< Server IP address.
        int server_port_;                            ///< Server port number.
        mutable std::ofstream log_file_;             ///< Log file output stream.
        mutable std::mutex log_mutex_;               ///< Mutex for thread-safe logging.
        BattleShipProtocol::Protocol protocol_;      ///< Protocol parser and serializer.
        std::thread send_thread_;                    ///< Thread handling user input and sending messages.
        std::thread recv_thread_;                    ///< Thread receiving messages from the server.
        std::atomic<bool> running_{false};           ///< Flag indicating if the client is active.
        mutable std::mutex state_mutex_;             ///< Mutex for protecting shared state.
        std::string nickname_;                       ///< Player nickname.
        std::string email_;                          ///< Player email.
        BattleShipProtocol::StatusData last_status_; ///< Last received game status.
        int player_id_{-1};                          ///< Assigned player ID from the server.
        std::mutex player_id_mutex_;                 ///< Mutex for accessing player_id.
        std::condition_variable player_id_cv_;       ///< Condition variable to wait for player ID.

        /**
         * @brief Establishes a TCP connection to the server.
         */
        void connect_to_server();

        /**
         * @brief Sends a message to the server.
         * @param msg Message to send.
         */
        void send_message(const BattleShipProtocol::Message &msg) const;

        /**
         * @brief Receives a message from the server.
         * @return Received Message.
         */
        BattleShipProtocol::Message receive_message();

        /**
         * @brief Loop for sending messages (e.g. registering, placing ships, shooting).
         */
        void send_loop();

        /**
         * @brief Loop for continuously receiving and handling messages.
         */
        void receive_loop();

        /**
         * @brief Logs a message to the log file.
         * @param query Sent message or action.
         * @param response Server's response.
         * @param level Log severity level (default is "INFO").
         */
        void log(const std::string &query, const std::string &response, const std::string &level = "INFO") const;

        /**
         * @brief Generates initial ship placements for the start of the game.
         * @return PlaceShipsData with ships and positions.
         */
        BattleShipProtocol::PlaceShipsData generate_initial_ships() const;

        /**
         * @brief Displays the current game state (own and enemy boards).
         */
        void display_game_state() const;

        /**
         * @brief Displays a summary of all the shots made.
         */
        void display_shot_history() const;

        /**
         * @brief Converts a CellState to its string representation.
         * @param state Cell state to convert.
         * @return String representing the state.
         */
        std::string cellStateToString(BattleShipProtocol::CellState state) const;
    };

} // namespace BattleshipClient

#endif