#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <stdexcept>

namespace BattleShipProtocol
{

    /**
     * @brief Exception thrown when a protocol-related error occurs.
     */
    class ProtocolError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a ProtocolError with a message.
         * @param message Error description.
         */
        explicit ProtocolError(const std::string &message) : std::runtime_error(message) {}
    };

    /**
     * @brief Types of messages exchanged between client and server.
     */
    enum class MessageType
    {
        REGISTER,    ///< Register a player
        PLACE_SHIPS, ///< Send ship placements
        SHOOT,       ///< Shoot at a coordinate
        STATUS,      ///< Request or send game status
        SURRENDER,   ///< Surrender the match
        GAME_OVER,   ///< Game ended notification
        ERROR,       ///< Error message
        PLAYER_ID    ///< Player ID assignment
    };

    /**
     * @brief Types of ships used in the game.
     */
    enum class ShipType
    {
        PORTAAVIONES,
        BUQUE,
        CRUCERO,
        DESTRUCTOR,
        SUBMARINO
    };

    /**
     * @brief Indicates whose turn it is.
     */
    enum class Turn
    {
        YOUR_TURN,
        OPPONENT_TURN
    };

    /**
     * @brief State of a board cell.
     */
    enum class CellState
    {
        WATER,
        HIT,
        SUNK,
        SHIP,
        MISS
    };

    /**
     * @brief Current state of the game.
     */
    enum class GameState
    {
        ONGOING,
        WAITING,
        ENDED
    };

    /**
     * @brief Message content containing the player ID.
     */
    struct PlayerIdData
    {
        int player_id; ///< Assigned ID (1 or 2)
    };

    /**
     * @brief Data structure for player registration.
     */
    struct RegisterData
    {
        std::string nickname; ///< Player's nickname
        std::string email;    ///< Player's email
    };

    /**
     * @brief Board coordinate (e.g., A5).
     */
    struct Coordinate
    {
        std::string letter; ///< Row label (letter)
        int number;         ///< Column number
    };

    /**
     * @brief Represents a ship and its occupied coordinates.
     */
    struct Ship
    {
        ShipType type;                       ///< Type of ship
        std::vector<Coordinate> coordinates; ///< List of occupied coordinates
    };

    /**
     * @brief Data structure for placing ships.
     */
    struct PlaceShipsData
    {
        std::vector<Ship> ships; ///< List of ships to place
    };

    /**
     * @brief Data structure for a shooting action.
     */
    struct ShootData
    {
        Coordinate coordinate; ///< Target coordinate
    };

    /**
     * @brief Represents a cell on the board.
     */
    struct Cell
    {
        Coordinate coordinate; ///< Coordinate of the cell
        CellState cellState;   ///< State of the cell
    };

    /**
     * @brief Game status report structure.
     */
    struct StatusData
    {
        Turn turn;                       ///< Whose turn it is
        std::vector<Cell> boardOwn;      ///< Player's own board
        std::vector<Cell> boardOpponent; ///< Opponent's board
        GameState gameState;             ///< Overall game state
        int time_remaining;              ///< Remaining time (seconds)
    };

    /**
     * @brief Contains game result information.
     */
    struct GameOverData
    {
        std::string winner; ///< Nickname of the winner
    };

    /**
     * @brief Error description sent in response.
     */
    struct ErrorData
    {
        int code;                ///< Error code
        std::string description; ///< Human-readable error description
    };

    /**
     * @brief Variant of all possible data types that a message can contain.
     */
    using MessageData = std::variant<
        std::monostate,
        PlayerIdData,
        RegisterData,
        PlaceShipsData,
        ShootData,
        StatusData,
        GameOverData,
        ErrorData>;

    /**
     * @brief High-level structure of a message in the protocol.
     */
    struct Message
    {
        MessageType type; ///< Type of message
        MessageData data; ///< Associated data
    };

    /**
     * @brief Provides parsing and serialization methods for the Battleship protocol.
     */
    class Protocol
    {
    public:
        /**
         * @brief Parses a raw message string into a Message object.
         * @param raw_message Raw string from the client/server.
         * @return Parsed Message with type and data.
         */
        Message parse_message(std::string_view raw_message) const;

        /**
         * @brief Serializes a Message object into a string.
         * @param msg Message to serialize.
         * @return String representation ready to send.
         */
        std::string build_message(const Message &msg) const;

    private:
        // --- String to enum/struct conversions ---

        /**
         * @brief Converts string to MessageType.
         * @param type_str Message type string.
         * @return MessageType enum.
         */
        MessageType string_to_message_type(std::string_view type_str) const;

        /**
         * @brief Converts string to ShipType.
         * @param type Ship type string.
         * @return ShipType enum.
         */
        ShipType string_to_ship_type(std::string_view type) const;

        /**
         * @brief Converts string to Turn.
         * @param turn Turn string.
         * @return Turn enum.
         */
        Turn string_to_turn(std::string_view turn) const;

        /**
         * @brief Converts string to CellState.
         * @param state Cell state string.
         * @return CellState enum.
         */
        CellState string_to_cell_state(std::string_view state) const;

        /**
         * @brief Converts string to GameState.
         * @param state Game state string.
         * @return GameState enum.
         */
        GameState string_to_game_state(std::string_view state) const;

        /**
         * @brief Converts string to Cell.
         * @param cell_str String with cell data.
         * @return Cell object.
         */
        Cell string_to_cell(std::string_view cell_str) const;

        /**
         * @brief Converts string to Coordinate.
         * @param coor Coordinate string.
         * @return Coordinate object.
         */
        Coordinate string_to_coordinate(std::string_view coor) const;

        /**
         * @brief Converts string to list of Coordinates.
         * @param coor_str Comma-separated coordinates.
         * @return Vector of Coordinate objects.
         */
        std::vector<Coordinate> string_to_coordinates(std::string_view coor_str) const;

        // --- Enum to string conversions ---

        /**
         * @brief Converts MessageType to string.
         */
        std::string message_type_to_string(MessageType type) const;

        /**
         * @brief Converts ShipType to string.
         */
        std::string ship_type_to_string(ShipType type) const;

        /**
         * @brief Converts Turn to string.
         */
        std::string turn_to_string(Turn turn) const;

        /**
         * @brief Converts CellState to string.
         */
        std::string cell_state_to_string(CellState state) const;

        /**
         * @brief Converts GameState to string.
         */
        std::string game_state_to_string(GameState state) const;

        /**
         * @brief Converts a list of Coordinates to a single string.
         */
        std::string coordinates_to_string(const std::vector<Coordinate> &coordinates) const;

        // --- Specific data parsers ---

        /**
         * @brief Parses player ID data.
         */
        PlayerIdData parse_player_id_data(std::string_view data) const;

        /**
         * @brief Parses register data.
         */
        RegisterData parse_register_data(std::string_view data) const;

        /**
         * @brief Parses ship placement data.
         */
        PlaceShipsData parse_place_ships_data(std::string_view data) const;

        /**
         * @brief Parses shooting data.
         */
        ShootData parse_shoot_data(std::string_view data) const;

        /**
         * @brief Parses full game status data.
         */
        StatusData parse_status_data(std::string_view data) const;

        /**
         * @brief Parses game over data.
         */
        GameOverData parse_game_over_data(std::string_view data) const;

        /**
         * @brief Parses error data.
         */
        ErrorData parse_error_data(std::string_view data) const;

        /**
         * @brief Parses board state from a raw string.
         * @param board_str String with board cells.
         * @return Vector of Cell objects.
         */
        std::vector<Cell> parse_board_data(std::string_view board_str) const;
    };

} // namespace BattleShipProtocol

#endif
