#include <gtest/gtest.h>
#include "../include/phase_state.hpp"
#include <iostream>

namespace BattleShipProtocol
{
    class PhaseStateTest : public ::testing::Test
    {
    protected:
        PhaseState phase_state;
    };

    TEST_F(PhaseStateTest, TransitionFromRegistrationToPlacement)
    {
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::REGISTRATION);
        phase_state.transition_to_placement();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLACEMENT);
    }

    TEST_F(PhaseStateTest, InvalidTransitionFromRegistrationToPlaying)
    {
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::REGISTRATION);
        EXPECT_THROW(phase_state.transition_to_playing(), std::runtime_error);
    }

    TEST_F(PhaseStateTest, TransitionFromPlacementToPlaying)
    {
        phase_state.transition_to_placement();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLACEMENT);
        phase_state.transition_to_playing();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLAYING);
    }

    TEST_F(PhaseStateTest, InvalidTransitionFromPlacementToFinished)
    {
        EXPECT_THROW(phase_state.transition_to_finished(), std::runtime_error);
    }

    TEST_F(PhaseStateTest, TransitionFromPlayingToFinished)
    {
        phase_state.transition_to_placement();
        phase_state.transition_to_playing();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLAYING);
        phase_state.transition_to_finished();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::FINISHED);
    }

    TEST_F(PhaseStateTest, FullTransitionCycle)
    {
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::REGISTRATION);
        phase_state.transition_to_placement();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLACEMENT);
        phase_state.transition_to_playing();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLAYING);
        phase_state.transition_to_finished();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::FINISHED);
    }

    TEST_F(PhaseStateTest, InvalidMultipleTransitionsToSamePhase)
    {
        phase_state.transition_to_placement();
        EXPECT_EQ(phase_state.get_phase(), PhaseState::Phase::PLACEMENT);
        EXPECT_THROW(phase_state.transition_to_placement(), std::runtime_error);
    }
} // namespace BattleShipProtocol

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
