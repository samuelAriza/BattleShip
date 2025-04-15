/// \file game_state.hpp
/// \brief Declaration of the PhaseState class for managing the game phases of the BattleShipProtocol.

#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

namespace BattleShipProtocol
{

    /**
     * @class PhaseState
     * @brief Represents and manages the current phase of the Battleship game.
     *
     * This class handles transitions between the different game phases:
     * player registration, ship placement, active gameplay, and game over.
     */
    class PhaseState
    {
    public:
        /**
         * @enum Phase
         * @brief Defines the possible phases of the game.
         */
        enum class Phase
        {
            REGISTRATION, ///< Player registration phase.
            PLACEMENT,    ///< Ship placement phase.
            PLAYING,      ///< Active gameplay phase.
            FINISHED      ///< Game over phase.
        };

        /**
         * @brief Constructs a new PhaseState object and initializes the game in the registration phase.
         */
        PhaseState();

        /**
         * @brief Retrieves the current game phase.
         * @return The current Phase as an enum value.
         */
        Phase get_phase() const noexcept;

        /**
         * @brief Sets the game to a specific phase.
         * @param new_phase The phase to transition to.
         */
        void set_phase(Phase new_phase);

        /**
         * @brief Transitions the game state to the ship placement phase.
         */
        void transition_to_placement();

        /**
         * @brief Transitions the game state to the active playing phase.
         */
        void transition_to_playing();

        /**
         * @brief Transitions the game state to the finished phase.
         */
        void transition_to_finished();

    private:
        Phase phase_; ///< The current phase of the game.
    };

} // namespace BattleShipProtocol

#endif // GAME_STATE_HPP
