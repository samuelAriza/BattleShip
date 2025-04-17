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
     * @brief Exception thrown when a game rule is violated or an invalid state is reached.
     */
    class GameLogicError : public std::runtime_error
    {
    public:
        /**
         * @brief Constructs a GameLogicError with the specified message.
         * @param msg Description of the error.
         */
        explicit GameLogicError(const std::string &msg) : std::runtime_error(msg) {}
    };

    /**
     * @brief Main game logic controller for Battleship with two players.
     *
     * Manages player state, game phases, turn-taking, shot processing,
     * ship placement, and game completion.
     */
    class GameLogic
    {
    public:
        /**
         * @brief Constructs the game logic with empty boards and initial state.
         */
        GameLogic();

        /**
         * @brief Returns the current game phase.
         * @return Current phase.
         */
        PhaseState::Phase get_phase() const noexcept { return state_.get_phase(); }

        /**
         * @brief Transitions the game to the ship placement phase.
         */
        void transition_to_placement() { state_.transition_to_placement(); }

        /**
         * @brief Transitions the game to the playing phase.
         */
        void transition_to_playing() { state_.transition_to_playing(); }

        /**
         * @brief Transitions the game to the finished phase.
         */
        void transition_to_finished() { state_.transition_to_finished(); }

        /**
         * @brief Registers a player with the given ID and registration data.
         * @param player_id ID of the player (1 or 2).
         * @param data Player's nickname and email.
         * @throws GameLogicError if the ID is invalid or already taken.
         */
        void register_player(int player_id, const RegisterData &data);

        /**
         * @brief Checks if both players are registered.
         * @return True if both players are registered.
         */
        bool are_both_registered() const;

        /**
         * @brief Checks if both players have placed all their ships.
         * @return True if both players have completed ship placement.
         */
        bool are_both_ships_placed() const;

        /**
         * @brief Returns the number of ships placed by a player.
         * @param player_id ID of the player.
         * @return Number of ships placed.
         */
        size_t ships_placed(int player_id) const;

        /**
         * @brief Places ships for the specified player.
         * @param player_id ID of the player.
         * @param data Ships and their coordinates.
         * @throws GameLogicError on invalid placement (overlap, out of bounds, etc.).
         */
        void place_ships(int player_id, const PlaceShipsData &data);

        /**
         * @brief Processes a shot from one player to the other.
         * @param player_id ID of the player who is shooting.
         * @param shot Target coordinate.
         * @throws GameLogicError if it's not the player's turn or input is invalid.
         */
        void process_shot(int player_id, const ShootData &shot);

        /**
         * @brief Returns the current game status from the perspective of the player.
         * @param player_id ID of the player requesting the status.
         * @return StatusData containing boards, turn info, and game state.
         * @throws GameLogicError if the player ID is invalid.
         */
        StatusData get_status(int player_id) const;

        /**
         * @brief Checks whether the game has finished.
         * @return True if the game is over.
         */
        bool is_game_over() const noexcept;

        /**
         * @brief Returns the game result including the winner's nickname.
         * @return GameOverData with the winner info.
         * @throws GameLogicError if the game is not yet finished.
         */
        GameOverData get_game_over_result() const;

        /**
         * @brief Returns the nickname of the specified player.
         * @param player_id ID of the player.
         * @return Player's nickname.
         */
        std::string get_player_nickname(int player_id) const;

    private:
        PhaseState state_; ///< Current phase controller.

        static constexpr int BOARD_SIZE = 10; ///< Board dimensions (10x10).
        static constexpr int MAX_PLAYERS = 2; ///< Maximum number of players.

        /**
         * @brief Represents a player's state in the game.
         */
        struct Player
        {
            std::string nickname;                            ///< Player's nickname.
            std::array<Cell, BOARD_SIZE * BOARD_SIZE> board; ///< Player's board.
            std::vector<Ship> ships;                         ///< Ships placed on the board.
            bool surrendered = false;                        ///< True if the player surrendered.
            int ships_remaining = 9;                         ///< Remaining ships (counted by units).
        };

        std::map<int, Player> players_;     ///< Map of player IDs to their state.
        int current_turn_;                  ///< ID of the player whose turn it is.
        bool game_over_;                    ///< True if the game has ended.
        std::optional<std::string> winner_; ///< Winner's nickname if known.

        /**
         * @brief Converts a board coordinate into a linear index (0 to 99).
         * @param coord Coordinate to convert.
         * @return Index in the board array.
         * @throws GameLogicError if coordinate is invalid.
         */
        int coord_to_index(const Coordinate &coord) const;

        /**
         * @brief Validates and places ships for a player.
         * @param player Player reference.
         * @param ships List of ships to place.
         * @throws GameLogicError if placement is invalid.
         */
        void validate_and_place_ships(Player &player, const std::vector<Ship> &ships);

        /**
         * @brief Updates the board after a shot and checks for ship sinking.
         * @param shooter_id ID of the shooting player.
         * @param target_id ID of the target player.
         * @param shot Shot coordinate.
         * @return True if a ship was hit or sunk.
         */
        bool update_board(int shooter_id, int target_id, const Coordinate &shot);

        /**
         * @brief Checks if all of a player's ships are sunk.
         * @param player_id ID of the player to check.
         * @return True if no ships remain.
         */
        bool all_ships_sunk(int player_id) const;
    };

} // namespace BattleShipProtocol

#endif