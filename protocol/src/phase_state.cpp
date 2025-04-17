#include "../include/phase_state.hpp"
#include <stdexcept>

namespace BattleShipProtocol
{

    PhaseState::PhaseState() : phase_(Phase::REGISTRATION) {}

    void PhaseState::transition_to_placement()
    {
        if (phase_ != Phase::REGISTRATION)
        {
            /*
            enum class Phase es un enumerado fuertemente tipado. A diferencia de los enum tradicionales, se necesita hacer un cast explícito para convertirlo a su valor subyacente (por defecto int).

            static_cast<int>(phase_) convierte el valor del enum class Phase a su representación entera (por ejemplo, Phase::REGISTRATION sería 0, PLACEMENT sería 1, etc.).

            std::to_string(...) luego convierte ese int a una cadena.
            */
            throw std::runtime_error("Invalid transition to PLACEMENT from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::PLACEMENT;
    }

    void PhaseState::transition_to_playing()
    {
        if (phase_ != Phase::PLACEMENT)
        {
            throw std::runtime_error("Invalid transition to PLAYING from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::PLAYING;
    }

    void PhaseState::transition_to_finished()
    {
        if (phase_ != Phase::PLAYING)
        {
            throw std::runtime_error("Invalid transition to FINISHED from " + std::to_string(static_cast<int>(phase_)));
        }
        phase_ = Phase::FINISHED;
    }

} // namespace BattleShipProtocol