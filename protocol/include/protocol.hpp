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
         * @brief Constructs a ProtocolError with the specified message.
         * @param message Description of the error.
         */
        explicit ProtocolError(const std::string &message) : std::runtime_error(message) {}
    };

    /**
     * @brief Enumeration of the types of messages exchanged between client and server.
     */
    enum class MessageType
    {
        REGISTER,    ///< Player registration
        PLACE_SHIPS, ///< Ship placement data
        SHOOT,       ///< Player shot
        STATUS,      ///< Game status request or response
        SURRENDER,   ///< Player surrender
        GAME_OVER,   ///< Game ended notification
        ERROR,       ///< Error message
        PLAYER_ID    ///< Assigned player ID
    };

    /**
     * @brief Enumeration of ship types.
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
     * @brief State of a cell on the board.
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
     * @brief Overall game state.
     */
    enum class GameState
    {
        ONGOING,
        WAITING,
        ENDED
    };

    /**
     * @brief Data structure to assign a player ID.
     */
    struct PlayerIdData
    {
        int player_id; ///< Assigned player ID (1 or 2)
    };

    /**
     * @brief Data used for player registration.
     */
    struct RegisterData
    {
        std::string nickname; ///< Player nickname
        std::string email;    ///< Player email
    };

    /**
     * @brief Coordinate on the board.
     */
    struct Coordinate
    {
        std::string letter; ///< Row (e.g. "A")
        int number;         ///< Column (e.g. 5)
    };

    /**
     * @brief Describes a ship and its position on the board.
     */
    struct Ship
    {
        ShipType type;                       ///< Type of ship
        std::vector<Coordinate> coordinates; ///< Coordinates occupied by the ship
    };

    /**
     * @brief Data used for sending ship placements.
     */
    struct PlaceShipsData
    {
        std::vector<Ship> ships; ///< Ships placed on the board
    };

    /**
     * @brief Data structure to send a shot.
     */
    struct ShootData
    {
        Coordinate coordinate; ///< Target coordinate
    };

    /**
     * @brief Information about a single cell on the board.
     */
    struct Cell
    {
        Coordinate coordinate; ///< Cell coordinate
        CellState cellState;   ///< Current state of the cell
    };

    /**
     * @brief Represents the game state for one player.
     */
    struct StatusData
    {
        Turn turn;                       ///< Indicates whose turn it is
        std::vector<Cell> boardOwn;      ///< Player's own board
        std::vector<Cell> boardOpponent; ///< Known state of opponent's board
        GameState gameState;             ///< Current game status
        int time_remaining;              ///< Remaining time for turn
    };

    /**
     * @brief Contains the result when the game ends.
     */
    struct GameOverData
    {
        std::string winner; ///< Winner's nickname
    };

    /**
     * @brief Information about an error occurred in the game or protocol.
     */
    struct ErrorData
    {
        int code;                ///< Error code
        std::string description; ///< Human-readable error message
    };

    /**
     * @brief Variant that holds any of the possible message data types.
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
     * @brief Represents a protocol message with type and associated data.
     */
    struct Message
    {
        MessageType type; ///< Type of message
        MessageData data; ///< Associated data payload
    };

    /**
     * @brief Provides functions to parse and serialize protocol messages.
     */
    class Protocol
    {
    public:
        /**
         * @brief Default constructor.
         */
        Protocol() = default;

        /**
         * @brief Parses a raw string message and returns a structured Message object.
         * @param raw_message The raw message string received from the network.
         * @return A structured Message with type and data.
         */
        Message parse_message(std::string_view raw_message) const;

        /**
         * @brief Serializes a structured Message into a string to be sent over the network.
         * @param msg The structured message to serialize.
         * @return A string representing the serialized message.
         */
        std::string build_message(const Message &msg) const;

    private:
        // --- String to enum/object conversions ---

        /**
         * @brief Converts string to MessageType enum.
         */
        MessageType string_to_message_type(std::string_view type_str) const;

        /**
         * @brief Converts string to ShipType enum.
         */
        ShipType string_to_ship_type(std::string_view type) const;

        /**
         * @brief Converts string to Turn enum.
         */
        Turn string_to_turn(std::string_view turn) const;

        /**
         * @brief Converts string to CellState enum.
         */
        CellState string_to_cell_state(std::string_view state) const;

        /**
         * @brief Converts string to GameState enum.
         */
        GameState string_to_game_state(std::string_view state) const;

        /**
         * @brief Parses a string into a Cell object.
         */
        Cell string_to_cell(std::string_view cell_str) const;

        /**
         * @brief Parses a string into a Coordinate object.
         */
        Coordinate string_to_coordinate(std::string_view coor) const;

        /**
         * @brief Parses a list of coordinates from a string.
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
         * @brief Converts a list of coordinates into a string.
         */
        std::string coordinates_to_string(const std::vector<Coordinate> &coordinates) const;

        // --- Parsers for specific message data types ---

        /**
         * @brief Parses PlayerIdData from a string.
         */
        PlayerIdData parse_player_id_data(std::string_view data) const;

        /**
         * @brief Parses RegisterData from a string.
         */
        RegisterData parse_register_data(std::string_view data) const;

        /**
         * @brief Parses PlaceShipsData from a string.
         */
        PlaceShipsData parse_place_ships_data(std::string_view data) const;

        /**
         * @brief Parses ShootData from a string.
         */
        ShootData parse_shoot_data(std::string_view data) const;

        /**
         * @brief Parses StatusData from a string.
         */
        StatusData parse_status_data(std::string_view data) const;

        /**
         * @brief Parses GameOverData from a string.
         */
        GameOverData parse_game_over_data(std::string_view data) const;

        /**
         * @brief Parses ErrorData from a string.
         */
        ErrorData parse_error_data(std::string_view data) const;

        // --- Helpers ---

        /**
         * @brief Parses a full board from a string into a list of Cell objects.
         * @param board_str Raw string representing board state.
         * @return Vector of Cell objects.
         */
        std::vector<Cell> parse_board_data(std::string_view board_str) const;
    };

} // namespace BattleShipProtocol

#endif