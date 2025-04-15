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
     * @class ProtocolError
     * @brief Exception thrown for protocol-specific parsing or formatting errors.
     */
    class ProtocolError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a new ProtocolError with a custom message.
         * @param message Error message.
         */
        explicit ProtocolError(const std::string &message) : std::runtime_error(message) {}
    };

    /**
     * @enum MessageType
     * @brief Represents the type of message exchanged between client and server.
     */
    enum class MessageType
    {
        REGISTER,    ///< Registration request.
        PLACE_SHIPS, ///< Ship placement data.
        SHOOT,       ///< Shot fired by a player.
        STATUS,      ///< Response with game status.
        GAME_OVER,   ///< Final result of the game.
        ERROR,       ///< Error message.
        PLAYER_ID    ///< Response containing assigned player ID.
    };

    /**
     * @enum ShipType
     * @brief Enumerates all ship types available in the game.
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
     * @enum Turn
     * @brief Indicates whose turn it is in the game.
     */
    enum class Turn
    {
        YOUR_TURN,
        OPPONENT_TURN
    };

    /**
     * @enum CellState
     * @brief Represents the state of a cell in the game board.
     */
    enum class CellState
    {
        WATER, ///< No ship present.
        HIT,   ///< Ship hit but not sunk.
        SUNK,  ///< Ship sunk.
        SHIP,  ///< Ship placed but not hit.
        MISS   ///< Shot missed.
    };

    /**
     * @enum GameState
     * @brief Represents the overall game status.
     */
    enum class GameState
    {
        ONGOING,
        WAITING,
        ENDED
    };

    /**
     * @struct PlayerIdData
     * @brief Contains the player ID assigned by the server.
     */
    struct PlayerIdData
    {
        int player_id;
    };

    /**
     * @struct RegisterData
     * @brief Contains the nickname and email of a player.
     */
    struct RegisterData
    {
        std::string nickname;
        std::string email;
    };

    /**
     * @struct Coordinate
     * @brief Represents a coordinate on the board (e.g., "A5").
     */
    struct Coordinate
    {
        std::string letter; ///< Row indicator (A-J).
        int number;         ///< Column indicator (1-10).
    };

    /**
     * @struct Ship
     * @brief Represents a ship with a type and a list of occupied coordinates.
     */
    struct Ship
    {
        ShipType type;
        std::vector<Coordinate> coordinates;
    };

    /**
     * @struct PlaceShipsData
     * @brief Contains the list of ships a player places on the board.
     */
    struct PlaceShipsData
    {
        std::vector<Ship> ships;
    };

    /**
     * @struct ShootData
     * @brief Contains the coordinate of a fired shot.
     */
    struct ShootData
    {
        Coordinate coordinate;
    };

    /**
     * @struct Cell
     * @brief Represents a cell on the board with its current state.
     */
    struct Cell
    {
        Coordinate coordinate;
        CellState cellState;
    };

    /**
     * @struct StatusData
     * @brief Contains game status information from the server.
     */
    struct StatusData
    {
        Turn turn;                       ///< Indicates whose turn it is.
        std::vector<Cell> boardOwn;      ///< Player's own board.
        std::vector<Cell> boardOpponent; ///< Opponent's board (partial view).
        GameState gameState;             ///< Overall game status.
    };

    /**
     * @struct GameOverData
     * @brief Contains the final result of the game.
     */
    struct GameOverData
    {
        std::string winner; ///< Nickname of the winning player or "NONE".
    };

    /**
     * @struct ErrorData
     * @brief Contains error information.
     */
    struct ErrorData
    {
        int code;                ///< Error code.
        std::string description; ///< Description of the error.
    };

    /**
     * @brief Variant type for all possible data types in a message.
     */
    using MessageData = std::variant<
        PlayerIdData,
        RegisterData,
        PlaceShipsData,
        ShootData,
        StatusData,
        // SurrenderData
        GameOverData,
        ErrorData>;

    /**
     * @struct Message
     * @brief Represents a protocol message consisting of a type and associated data.
     */
    struct Message
    {
        MessageType type;
        MessageData data;
    };

    /**
     * @class Protocol
     * @brief Implements the encoding and decoding of protocol messages.
     */
    class Protocol
    {
    public:
        /**
         * @brief Default constructor.
         */
        Protocol() = default;

        /**
         * @brief Parses a raw string into a structured Message object.
         * @param raw_message The raw message string.
         * @return A structured Message object.
         * @throws ProtocolError if the message is malformed.
         */
        Message parse_message(std::string_view raw_message) const;

        /**
         * @brief Builds a string from a structured Message object.
         * @param msg The Message to encode.
         * @return A formatted string suitable for transmission.
         */
        std::string build_message(const Message &msg) const;

    private:
        // Conversion from strings to enums
        MessageType string_to_message_type(std::string_view type_str) const;
        ShipType string_to_ship_type(std::string_view type) const;
        Turn string_to_turn(std::string_view turn) const;
        CellState string_to_cell_state(std::string_view state) const;
        GameState string_to_game_state(std::string_view state) const;
        Cell string_to_cell(std::string_view cell_str) const;
        Coordinate string_to_coordinate(std::string_view coor) const;
        std::vector<Coordinate> string_to_coordinates(std::string_view coor_str) const;

        // Conversion from enums to strings
        std::string message_type_to_string(MessageType type) const;
        std::string ship_type_to_string(ShipType type) const;
        std::string turn_to_string(Turn turn) const;
        std::string cell_state_to_string(CellState state) const;
        std::string game_state_to_string(GameState state) const;
        std::string coordinates_to_string(const std::vector<Coordinate> &coordinates) const;

        // Parsing functions for each message type
        PlayerIdData parse_player_id_data(std::string_view data) const;
        RegisterData parse_register_data(std::string_view data) const;
        PlaceShipsData parse_place_ships_data(std::string_view data) const;
        ShootData parse_shoot_data(std::string_view data) const;
        StatusData parse_status_data(std::string_view data) const;
        GameOverData parse_game_over_data(std::string_view data) const;
        ErrorData parse_error_data(std::string_view data) const;
        std::vector<Cell> parse_board_data(std::string_view board_str) const;
    };

} // namespace BattleShipProtocol

#endif
