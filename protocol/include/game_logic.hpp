/// \file game_logic.hpp
/// \brief Declaration of the GameLogic class for game logic management.
#ifndef GAME_LOGIC_HPP
#define GAME_LOGIC_HPP

#include "protocol.hpp"
#include "phase_state.hpp"
#include <array>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <iostream>

namespace BattleShipProtocol
{
    /**
     * @class GameLogicError
     * @brief Exception thrown when a game rule is violated or an invalid state occurs.
     */
    class GameLogicError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a new GameLogicError with the specified error message.
         * @param msg The error message.
         */
        explicit GameLogicError(const std::string &msg) : std::runtime_error(msg) {}
    };

    /**
     * @class GameLogic
     * @brief Manages the game logic and rules for a two-player Battleship game.
     *
     * This class encapsulates the complete game state, handles game phases, processes
     * player actions, and maintains synchronized game data.
     */
    class GameLogic
    {
    public:
        /**
         * @brief Constructs a new GameLogic instance with an initial empty state.
         */
        GameLogic();

        /**
         * @brief Returns the current game phase.
         * @return The current Phase.
         */
        PhaseState::Phase get_phase() const noexcept;

        /**
         * @brief Transitions the game state to the ship placement phase.
         */
        void transition_to_placement();

        /**
         * @brief Transitions the game state to the playing phase.
         */
        void transition_to_playing();

        /**
         * @brief Transitions the game state to the finished phase.
         */
        void transition_to_finished();

        /**
         * @brief Registers a player with their identification data.
         * @param player_id The player's unique ID.
         * @param data Registration data including nickname and email.
         * @throws GameLogicError if the ID is invalid or already in use.
         */
        void register_player(int player_id, const RegisterData &data);

        /**
         * @brief Checks if both players have completed registration.
         * @return true if both are registered; false otherwise.
         */
        bool are_both_registered() const;

        /**
         * @brief Checks if both players have placed all their ships.
         * @return true if all ships are placed; false otherwise.
         */
        bool are_both_ships_placed() const;

        /**
         * @brief Returns the number of ships placed by a given player.
         * @param player_id The player's ID.
         * @return The number of ships placed.
         */
        size_t ships_placed(int player_id) const;

        /**
         * @brief Places ships on a player's board.
         * @param player_id The player's ID.
         * @param data A list of ships with types and coordinates.
         * @throws GameLogicError if placement is invalid or violates game rules.
         */
        void place_ships(int player_id, const PlaceShipsData &data);

        /**
         * @brief Processes a shot fired by a player.
         * @param player_id The player's ID.
         * @param shot The shot coordinate (e.g., "B5").
         * @throws GameLogicError if it's not the player's turn, the coordinate is invalid, or the game is over.
         */
        void process_shot(int player_id, const ShootData &shot);

        /**
         * @brief Returns the game status from the perspective of the specified player.
         * @param player_id The player's ID.
         * @return A StatusData object containing the game state.
         * @throws GameLogicError if the player ID is invalid.
         */
        StatusData get_status(int player_id) const;

        /**
         * @brief Checks if the game is over.
         * @return true if a player has won or surrendered; false otherwise.
         */
        bool is_game_over() const noexcept;

        /**
         * @brief Returns the final game result.
         * @return A GameOverData object with the winner's nickname or "NONE".
         * @throws GameLogicError if the game has not yet ended.
         */
        GameOverData get_game_over_result() const;

        /**
         * @brief Retrieves a player's nickname.
         * @param player_id The player's ID.
         * @return The nickname of the player.
         */
        std::string get_player_nickname(int player_id) const;

    private:
        PhaseState state_; ///< Current phase state of the game.

        static constexpr int BOARD_SIZE = 10; ///< Board size (10x10 grid).
        static constexpr int MAX_PLAYERS = 2; ///< Maximum number of players (fixed to 2).

        /**
         * @struct Player
         * @brief Holds the state and data for an individual player.
         */
        struct Player
        {
            std::string nickname;                            ///< Player's nickname.
            std::array<Cell, BOARD_SIZE * BOARD_SIZE> board; ///< Player's board.
            std::vector<Ship> ships;                         ///< Ships placed by the player.
            bool surrendered = false;                        ///< Whether the player surrendered.
            int ships_remaining = 9;                         ///< Number of remaining (not sunk) ships.
        };

        std::map<int, Player> players_;     ///< Map of players by ID.
        int current_turn_;                  ///< ID of the player whose turn it is.
        bool game_over_;                    ///< Whether the game has ended.
        std::optional<std::string> winner_; ///< Nickname of the winner, if any.

        /**
         * @brief Converts a coordinate string (e.g., "A1") to a board index.
         * @param coord The coordinate in letter-number format.
         * @return Index in the board array.
         * @throws GameLogicError if the coordinate is invalid.
         */
        int coord_to_index(const Coordinate &coord) const;

        /**
         * @brief Validates and places ships on the player's board.
         * @param player The player object.
         * @param ships The list of ships to place.
         * @throws GameLogicError if any ship placement is invalid.
         */
        void validate_and_place_ships(Player &player, const std::vector<Ship> &ships);

        /**
         * @brief Updates the board based on a shot and checks for ship sinkings.
         * @param shooter_id ID of the shooting player.
         * @param target_id ID of the targeted player.
         * @param shot The coordinate of the shot.
         * @return true if a ship was hit or sunk; false otherwise.
         */
        bool update_board(int shooter_id, int target_id, const Coordinate &shot);

        /**
         * @brief Checks whether all ships of a player have been sunk.
         * @param player_id The player's ID.
         * @return true if no ships remain; false otherwise.
         */
        bool all_ships_sunk(int player_id) const;
    };

} // namespace BattleShipProtocol

#endif
