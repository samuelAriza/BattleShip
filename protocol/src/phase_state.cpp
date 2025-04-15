/// \file phase_state.cpp
/// \brief Implementation of the PhaseState class for managing game phase transitions.

#include "../include/phase_state.hpp"
#include <stdexcept>

namespace BattleShipProtocol
{

    /**
     * @brief Constructs a PhaseState object with the initial phase set to REGISTRATION.
     */
    PhaseState::PhaseState() : phase_(Phase::REGISTRATION) {}

    /**
     * @brief Transitions the current phase from REGISTRATION to PLACEMENT.
     * @throws std::runtime_error if the current phase is not REGISTRATION.
     */
    void PhaseState::transition_to_placement()
    {
        if (phase_ != Phase::REGISTRATION)
        {
            throw std::runtime_error("Invalid transition to PLACEMENT from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::PLACEMENT;
    }

    /**
     * @brief Transitions the current phase from PLACEMENT to PLAYING.
     * @throws std::runtime_error if the current phase is not PLACEMENT.
     */
    void PhaseState::transition_to_playing()
    {
        if (phase_ != Phase::PLACEMENT)
        {
            throw std::runtime_error("Invalid transition to PLAYING from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::PLAYING;
    }

    /**
     * @brief Transitions the current phase from PLAYING to FINISHED.
     * @throws std::runtime_error if the current phase is not PLAYING.
     */
    void PhaseState::transition_to_finished()
    {
        if (phase_ != Phase::PLAYING)
        {
            throw std::runtime_error("Invalid transition to FINISHED from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::FINISHED;
    }

} // namespace BattleShipProtocol
