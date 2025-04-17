#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

namespace BattleShipProtocol
{

    /**
     * @brief Manages the current phase of the Battleship game.
     *
     * Provides accessors and transitions between the following phases:
     * - REGISTRATION: players are registering.
     * - PLACEMENT: players are placing their ships.
     * - PLAYING: players are taking turns shooting.
     * - FINISHED: game is over.
     */
    class PhaseState
    {
    public:
        /**
         * @brief Enumeration of the different game phases.
         */
        enum class Phase
        {
            REGISTRATION, ///< Waiting for players to register.
            PLACEMENT,    ///< Players are placing ships.
            PLAYING,      ///< Game is in progress.
            FINISHED      ///< Game has ended.
        };

        /**
         * @brief Constructs the phase manager with initial phase set to REGISTRATION.
         */
        PhaseState();

        /**
         * @brief Gets the current game phase.
         * @return Current Phase.
         */
        Phase get_phase() const noexcept { return phase_; }

        /**
         * @brief Sets the game phase to a new state.
         * @param new_phase The new phase to assign.
         */
        void set_phase(Phase new_phase) { phase_ = new_phase; }

        /**
         * @brief Transitions the game to the PLACEMENT phase.
         */
        void transition_to_placement();

        /**
         * @brief Transitions the game to the PLAYING phase.
         */
        void transition_to_playing();

        /**
         * @brief Transitions the game to the FINISHED phase.
         */
        void transition_to_finished();

    private:
        Phase phase_; ///< Current game phase.
    };

} // namespace BattleShipProtocol

#endif