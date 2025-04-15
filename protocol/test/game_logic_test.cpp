#include <gtest/gtest.h>
#include "../include/game_logic.hpp"
#include <iostream>

namespace BattleShipProtocol
{

    class GameLogicTest : public ::testing::Test
    {
    protected:
        GameLogic game_logic;

        void prepare_game_ready_for_shots()
        {
            RegisterData p1{"PlayerOne", "player1@mail.com"};
            RegisterData p2{"PlayerTwo", "player2@mail.com"};
            game_logic.register_player(1, p1);
            game_logic.register_player(2, p2);

            std::vector<Ship> ships = {
                {ShipType::PORTAAVIONES, {{"A", 1}, {"A", 2}, {"A", 3}, {"A", 4}, {"A", 5}}},
                {ShipType::BUQUE, {{"B", 1}, {"B", 2}, {"B", 3}, {"B", 4}}},
                {ShipType::CRUCERO, {{"C", 1}, {"C", 2}, {"C", 3}}},
                {ShipType::CRUCERO, {{"D", 1}, {"D", 2}, {"D", 3}}},
                {ShipType::DESTRUCTOR, {{"E", 1}, {"E", 2}}},
                {ShipType::DESTRUCTOR, {{"F", 1}, {"F", 2}}},
                {ShipType::SUBMARINO, {{"G", 1}}},
                {ShipType::SUBMARINO, {{"H", 1}}},
                {ShipType::SUBMARINO, {{"I", 1}}}};

            game_logic.place_ships(1, {ships});
            game_logic.place_ships(2, {ships});
        }
    };

    TEST_F(GameLogicTest, RegisterPlayer_Player1_Success)
    {
        RegisterData data{"PlayerOne", "player1@example.com"};
        EXPECT_NO_THROW(game_logic.register_player(1, data));
        EXPECT_EQ(game_logic.get_status(1).turn, Turn::YOUR_TURN);
    }
    TEST_F(GameLogicTest, RegisterPlayer_Player2_Success)
    {
        RegisterData data{"PlayerTwo", "player2@example.com"};

        EXPECT_NO_THROW(game_logic.register_player(2, data));
        EXPECT_EQ(game_logic.get_player_nickname(2), "PlayerTwo");
    }
    TEST_F(GameLogicTest, RegisterPlayer_InvalidPlayerId_Throws)
    {
        RegisterData data{"InvalidPlayer", "invalid@example.com"};
        EXPECT_THROW(game_logic.register_player(3, data), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, RegisterPlayer_DuplicateRegistration_Throws)
    {
        RegisterData data{"PlayerOne", "player1@example.com"};
        game_logic.register_player(1, data);
        EXPECT_THROW(game_logic.register_player(1, data), BattleShipProtocol::GameLogicError);
    }
    TEST_F(GameLogicTest, RegisterPlayer_EmptyNickname_AllowsByDefault)
    {
        RegisterData data{"", "no_name@example.com"};
        EXPECT_THROW(game_logic.register_player(1, data), BattleShipProtocol::GameLogicError);
    }
    TEST_F(GameLogicTest, PlaceShips_ValidForPlayer1_Succeeds)
    {
        game_logic.register_player(1, {"PlayerOne", "player1@example.com"});
        game_logic.register_player(2, {"PlayerTwo", "player2@example.com"});

        std::vector<Ship> ships = {
            {ShipType::PORTAAVIONES, {{"A", 1}, {"A", 2}, {"A", 3}, {"A", 4}, {"A", 5}}},
            {ShipType::BUQUE, {{"B", 1}, {"B", 2}, {"B", 3}, {"B", 4}}},
            {ShipType::CRUCERO, {{"C", 1}, {"C", 2}, {"C", 3}}},
            {ShipType::CRUCERO, {{"D", 1}, {"D", 2}, {"D", 3}}},
            {ShipType::DESTRUCTOR, {{"E", 1}, {"E", 2}}},
            {ShipType::DESTRUCTOR, {{"F", 1}, {"F", 2}}},
            {ShipType::SUBMARINO, {{"G", 1}}},
            {ShipType::SUBMARINO, {{"H", 1}}},
            {ShipType::SUBMARINO, {{"I", 1}}}};

        PlaceShipsData data{ships};
        EXPECT_NO_THROW(game_logic.place_ships(1, data));
    }

    TEST_F(GameLogicTest, PlaceShips_InvalidPlayerId_Throws)
    {
        std::vector<Ship> empty;
        EXPECT_THROW(game_logic.place_ships(3, {empty}), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, PlaceShips_DuplicatePlacement_Throws)
    {
        game_logic.register_player(1, {"PlayerOne", "player1@example.com"});
        game_logic.register_player(2, {"PlayerTwo", "player2@example.com"});

        std::vector<Ship> ships = {
            {ShipType::PORTAAVIONES, {{"A", 1}, {"A", 2}, {"A", 3}, {"A", 4}, {"A", 5}}},
            {ShipType::BUQUE, {{"B", 1}, {"B", 2}, {"B", 3}, {"B", 4}}},
            {ShipType::CRUCERO, {{"C", 1}, {"C", 2}, {"C", 3}}},
            {ShipType::CRUCERO, {{"D", 1}, {"D", 2}, {"D", 3}}},
            {ShipType::DESTRUCTOR, {{"E", 1}, {"E", 2}}},
            {ShipType::DESTRUCTOR, {{"F", 1}, {"F", 2}}},
            {ShipType::SUBMARINO, {{"G", 1}}},
            {ShipType::SUBMARINO, {{"H", 1}}},
            {ShipType::SUBMARINO, {{"I", 1}}}};

        game_logic.place_ships(1, {ships});
        EXPECT_THROW(game_logic.place_ships(1, {ships}), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, PlaceShips_PlayersNotRegistered_Throws)
    {
        GameLogic logic;
        std::vector<Ship> empty;
        EXPECT_THROW(logic.place_ships(1, {empty}), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, PlaceShips_TooFewShips_Throws)
    {
        game_logic.register_player(1, {"PlayerOne", "player1@example.com"});
        game_logic.register_player(2, {"PlayerTwo", "player2@example.com"});

        std::vector<Ship> ships = {
            {ShipType::PORTAAVIONES, {{"A", 1}, {"A", 2}, {"A", 3}, {"A", 4}, {"A", 5}}}};

        EXPECT_THROW(game_logic.place_ships(1, {ships}), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, ProcessShot_ValidHit_Player1_Succeeds)
    {
        prepare_game_ready_for_shots();
        ShootData shot{{"A", 1}};

        EXPECT_NO_THROW(game_logic.process_shot(1, shot));
    }

    TEST_F(GameLogicTest, ProcessShot_WrongTurn_Throws)
    {
        prepare_game_ready_for_shots();
        ShootData shot{{"A", 1}};

        EXPECT_THROW(game_logic.process_shot(2, shot), BattleShipProtocol::GameLogicError);
    }

    TEST_F(GameLogicTest, ProcessShot_GameOver_Throws)
    {
        prepare_game_ready_for_shots();

        std::vector<Coordinate> target_coords;
        const auto &board_opponent = game_logic.get_status(1).boardOpponent;

        for (const auto &cell : board_opponent)
        {
            if (cell.cellState == CellState::SHIP)
            {
                target_coords.push_back(cell.coordinate);
            }
        }

        for (size_t i = 0; i < target_coords.size(); ++i)
        {
            EXPECT_NO_THROW(game_logic.process_shot(1, {target_coords[i]}));

            if (i == target_coords.size() - 1)
                break;

            EXPECT_NO_THROW(game_logic.process_shot(2, {{"J", 10}}));
        }

        EXPECT_TRUE(game_logic.is_game_over());
        EXPECT_EQ(game_logic.get_game_over_result().winner, "PlayerOne");

        EXPECT_THROW(game_logic.process_shot(1, {{"A", 10}}), GameLogicError);
    }

    TEST_F(GameLogicTest, ProcessShot_ChangesTurn)
    {
        prepare_game_ready_for_shots();
        ShootData shot{{"A", 1}};

        game_logic.process_shot(1, shot);
        StatusData status_p1 = game_logic.get_status(1);
        StatusData status_p2 = game_logic.get_status(2);

        EXPECT_EQ(status_p1.turn, Turn::OPPONENT_TURN);
        EXPECT_EQ(status_p2.turn, Turn::YOUR_TURN);
    }

    TEST_F(GameLogicTest, ProcessShot_InvalidCoordinate_Throws)
    {
        prepare_game_ready_for_shots();
        ShootData shot{{"Z", 99}};
        EXPECT_THROW(game_logic.process_shot(1, shot), GameLogicError);
    }
} // namespace BattleShipProtocol

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
