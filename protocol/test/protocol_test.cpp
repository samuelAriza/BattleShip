#include <gtest/gtest.h>
#include "../include/protocol.hpp"
#include <iostream>

namespace BattleShipProtocol
{

    class ProtocolTest : public ::testing::Test
    {
    protected:
        Protocol protocol;
    };

    TEST_F(ProtocolTest, ParseMessage_PlayerIdData)
    {
        Message msg = protocol.parse_message("PLAYER_ID|1\n");
        EXPECT_EQ(msg.type, MessageType::PLAYER_ID);
        const auto &player_id_data = std::get<PlayerIdData>(msg.data);
        EXPECT_EQ(player_id_data.player_id, 1);
    }

    TEST_F(ProtocolTest, ParseMessage_RegisterData_ParsesCorrectly)
    {
        Message msg = protocol.parse_message("REGISTER|samargo,samargo@email.com\n");
        EXPECT_EQ(msg.type, MessageType::REGISTER);
        const auto &register_data = std::get<RegisterData>(msg.data);
        EXPECT_EQ(register_data.nickname, "samargo");
        EXPECT_EQ(register_data.email, "samargo@email.com");
    }

    TEST_F(ProtocolTest, ParseMessage_Register_MissingCommaThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTER|PlayerOneplayer@example.com\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Register_EmptyNicknameThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTER|,player@example.com\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Register_EmptyEmailThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTER|PlayerOne,\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Register_EmptyDataThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTER|\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Register_MissingBothFieldsThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTER|,\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Register_MissingDelimiterThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("REGISTERPlayerOne,player@example.com\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_ParsesCorrectly)
    {
        Message msg = protocol.parse_message("PLACE_SHIPS|PORTAAVIONES:A1;BUQUE:A2;CRUCERO:A3;DESTRUCTOR:A4;SUBMARINO:A5\n");
        EXPECT_EQ(msg.type, MessageType::PLACE_SHIPS);

        const auto &place_ships_data = std::get<PlaceShipsData>(msg.data);

        std::vector<Ship> expected_ships = {
            {ShipType::PORTAAVIONES, {{"A", 1}}},
            {ShipType::BUQUE, {{"A", 2}}},
            {ShipType::CRUCERO, {{"A", 3}}},
            {ShipType::DESTRUCTOR, {{"A", 4}}},
            {ShipType::SUBMARINO, {{"A", 5}}}};

        ASSERT_EQ(place_ships_data.ships.size(), expected_ships.size());

        for (size_t i = 0; i < expected_ships.size(); ++i)
        {
            EXPECT_EQ(place_ships_data.ships[i].type, expected_ships[i].type) << "Failed at ship " << i;
            ASSERT_EQ(place_ships_data.ships[i].coordinates.size(), expected_ships[i].coordinates.size());
            for (size_t j = 0; j < expected_ships[i].coordinates.size(); ++j)
            {
                EXPECT_EQ(place_ships_data.ships[i].coordinates[j].letter, expected_ships[i].coordinates[j].letter) << "Ship " << i << ", coord " << j;
                EXPECT_EQ(place_ships_data.ships[i].coordinates[j].number, expected_ships[i].coordinates[j].number) << "Ship " << i << ", coord " << j;
            }
        }
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_InvalidFormat_NoColon)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|PORTAAVIONESA1;BUQUE:B2\n"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_InvalidFormat_NoComma)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|PORTAAVIONES:A1 BUQUE:B2\n"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_EmptyData)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|\n"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_InvalidShipType)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|AVION:A1;BUQUE:B2\n"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_InvalidCoordinate)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|PORTAAVIONES:1A\n"), ProtocolError);
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|PORTAAVIONES:A\n"), ProtocolError);
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS|PORTAAVIONES:\n"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_DuplicateShipTypesAllowed)
    {
        Message msg = protocol.parse_message("PLACE_SHIPS|BUQUE:A1,A2\n");
        const auto &data = std::get<PlaceShipsData>(msg.data);
        ASSERT_EQ(data.ships.size(), 1);
        ASSERT_EQ(data.ships[0].type, ShipType::BUQUE);
        ASSERT_EQ(data.ships[0].coordinates.size(), 2);
        EXPECT_EQ(data.ships[0].coordinates[0].letter, "A");
        EXPECT_EQ(data.ships[0].coordinates[0].number, 1);
        EXPECT_EQ(data.ships[0].coordinates[1].letter, "A");
        EXPECT_EQ(data.ships[0].coordinates[1].number, 2);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_ValidCoordinates)
    {
        struct TestCase
        {
            std::string input;
            std::string expected_letter;
            int expected_number;
        };

        std::vector<TestCase> test_cases = {
            {"SHOOT|A1\n", "A", 1},
            {"SHOOT|B5\n", "B", 5},
            {"SHOOT|J10\n", "J", 10},
            {"SHOOT|H7\n", "H", 7}};

        for (const auto &test : test_cases)
        {
            BattleShipProtocol::Message msg = protocol.parse_message(test.input);
            EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::SHOOT);

            const auto &shoot_data = std::get<BattleShipProtocol::ShootData>(msg.data);
            EXPECT_EQ(shoot_data.coordinate.letter, test.expected_letter) << "Failed for input: " << test.input;
            EXPECT_EQ(shoot_data.coordinate.number, test.expected_number) << "Failed for input: " << test.input;
        }
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_EmptyMessageThrows)
    {
        EXPECT_THROW(protocol.parse_message(""), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_MissingDataThrows)
    {
        EXPECT_THROW(protocol.parse_message("PLACE_SHIPS"), ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_PlaceShips_SingleShip)
    {
        Message msg = protocol.parse_message("PLACE_SHIPS|BUQUE:A1,A2\n");
        const auto &data = std::get<PlaceShipsData>(msg.data);
        ASSERT_EQ(data.ships.size(), 1);
        EXPECT_EQ(data.ships[0].type, ShipType::BUQUE);
        ASSERT_EQ(data.ships[0].coordinates.size(), 2);
        EXPECT_EQ(data.ships[0].coordinates[0].letter, "A");
        EXPECT_EQ(data.ships[0].coordinates[0].number, 1);
        EXPECT_EQ(data.ships[0].coordinates[1].letter, "A");
        EXPECT_EQ(data.ships[0].coordinates[1].number, 2);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_EmptyCoordinate)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT|\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_MissingDelimiter)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_InvalidFormat_NumberFirst)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT|1A\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_IncompleteCoordinate_OnlyLetter)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT|C\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_IncompleteCoordinate_OnlyNumber)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT|7\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Shoot_InvalidCoordinateSymbols)
    {
        EXPECT_THROW(protocol.parse_message("SHOOT|$@\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Status_LongMessage_ParsesCorrectly)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("STATUS|OPPONENT_TURN;A1:SHIP,A2:SHIP,A3:SHIP,A4:SHIP,A5:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,B1:SHIP,B2:SHIP,B3:SHIP,B4:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,C1:SHIP,C2:SHIP,C3:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,D1:SHIP,D2:SHIP,D3:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,E1:SHIP,E2:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,F1:SHIP,F2:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,G1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,H1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,I1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER;A1:SHIP,A2:SHIP,A3:SHIP,A4:SHIP,A5:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,B1:SHIP,B2:SHIP,B3:SHIP,B4:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,C1:SHIP,C2:SHIP,C3:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,D1:SHIP,D2:SHIP,D3:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,E1:SHIP,E2:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,F1:SHIP,F2:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,G1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,H1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,I1:SHIP,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER,X1:WATER;ONGOING\n");

        EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::STATUS);

        const auto &status_data = std::get<BattleShipProtocol::StatusData>(msg.data);
        EXPECT_EQ(status_data.turn, BattleShipProtocol::Turn::OPPONENT_TURN);
        EXPECT_EQ(status_data.gameState, BattleShipProtocol::GameState::ONGOING);

        ASSERT_EQ(status_data.boardOwn.size(), 100);
        ASSERT_EQ(status_data.boardOpponent.size(), 100);

        EXPECT_EQ(status_data.boardOwn[0].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOwn[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOwn[0].cellState, BattleShipProtocol::CellState::SHIP);

        EXPECT_EQ(status_data.boardOwn[5].coordinate.letter, "X");
        EXPECT_EQ(status_data.boardOwn[5].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOwn[5].cellState, BattleShipProtocol::CellState::WATER);

        EXPECT_EQ(status_data.boardOpponent[0].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOpponent[0].cellState, BattleShipProtocol::CellState::SHIP);

        EXPECT_EQ(status_data.boardOpponent[10].coordinate.letter, "B");
        EXPECT_EQ(status_data.boardOpponent[10].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOpponent[10].cellState, BattleShipProtocol::CellState::SHIP);
    }

    TEST_F(ProtocolTest, ParseMessage_Status_ParsesCorrectly)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("STATUS|YOUR_TURN;A1:SHIP,A2:WATER;B1:HIT,B2:SUNK;ONGOING\n");

        EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::STATUS);

        const auto &status_data = std::get<BattleShipProtocol::StatusData>(msg.data);
        EXPECT_EQ(status_data.turn, BattleShipProtocol::Turn::YOUR_TURN);
        EXPECT_EQ(status_data.gameState, BattleShipProtocol::GameState::ONGOING);

        ASSERT_EQ(status_data.boardOwn.size(), 2);
        EXPECT_EQ(status_data.boardOwn[0].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOwn[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOwn[0].cellState, BattleShipProtocol::CellState::SHIP);

        EXPECT_EQ(status_data.boardOwn[1].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOwn[1].coordinate.number, 2);
        EXPECT_EQ(status_data.boardOwn[1].cellState, BattleShipProtocol::CellState::WATER);

        ASSERT_EQ(status_data.boardOpponent.size(), 2);
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.letter, "B");
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOpponent[0].cellState, BattleShipProtocol::CellState::HIT);
    }

    TEST_F(ProtocolTest, ParseMessage_Status_OpponentTurn)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("STATUS|OPPONENT_TURN;A1:WATER;B1:SHIP;WAITING\n");

        EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::STATUS);

        const auto &status_data = std::get<BattleShipProtocol::StatusData>(msg.data);
        EXPECT_EQ(status_data.turn, BattleShipProtocol::Turn::OPPONENT_TURN);
        EXPECT_EQ(status_data.gameState, BattleShipProtocol::GameState::WAITING);

        ASSERT_EQ(status_data.boardOwn.size(), 1);
        EXPECT_EQ(status_data.boardOwn[0].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOwn[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOwn[0].cellState, BattleShipProtocol::CellState::WATER);

        ASSERT_EQ(status_data.boardOpponent.size(), 1);
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.letter, "B");
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOpponent[0].cellState, BattleShipProtocol::CellState::SHIP);
    }

    TEST_F(ProtocolTest, ParseMessage_StatusWithEmptyBoards_ParsesCorrectly)
    {
        Message msg = protocol.parse_message("STATUS|YOUR_TURN;;;ENDED\n");
        EXPECT_EQ(msg.type, MessageType::STATUS);
        const auto &status_data = std::get<StatusData>(msg.data);
        EXPECT_EQ(status_data.turn, Turn::YOUR_TURN);
        EXPECT_TRUE(status_data.boardOwn.empty());
        EXPECT_TRUE(status_data.boardOpponent.empty());
        EXPECT_EQ(status_data.gameState, GameState::ENDED);
    }

    TEST_F(ProtocolTest, ParseMessage_Status_MultipleCells)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("STATUS|OPPONENT_TURN;A1:SHIP,A2:SUNK,B3:HIT;C1:WATER,D4:SHIP;ONGOING\n");

        EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::STATUS);

        const auto &status_data = std::get<BattleShipProtocol::StatusData>(msg.data);
        EXPECT_EQ(status_data.turn, BattleShipProtocol::Turn::OPPONENT_TURN);
        EXPECT_EQ(status_data.gameState, BattleShipProtocol::GameState::ONGOING);

        ASSERT_EQ(status_data.boardOwn.size(), 3);
        EXPECT_EQ(status_data.boardOwn[0].coordinate.letter, "A");
        EXPECT_EQ(status_data.boardOwn[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOwn[0].cellState, BattleShipProtocol::CellState::SHIP);

        ASSERT_EQ(status_data.boardOpponent.size(), 2);
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.letter, "C");
        EXPECT_EQ(status_data.boardOpponent[0].coordinate.number, 1);
        EXPECT_EQ(status_data.boardOpponent[0].cellState, BattleShipProtocol::CellState::WATER);
    }

    TEST_F(ProtocolTest, ParseMessage_Status_InvalidFormat)
    {
        EXPECT_THROW(protocol.parse_message("STATUS|INVALID;A1:SHIP;B2:WATER\n"), BattleShipProtocol::ProtocolError);
        EXPECT_THROW(protocol.parse_message("STATUS|YOUR_TURN;A1:INVALIDSTATE;B2:WATER;ONGOING\n"), BattleShipProtocol::ProtocolError);
        EXPECT_THROW(protocol.parse_message("STATUS|YOUR_TURN|A1:SHIP,B2:WATER,ONGOING\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ParsesCorrectlyWithStandardName)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("GAME_OVER|Player1\n");

        EXPECT_EQ(msg.type, MessageType::GAME_OVER);

        const auto &data = std::get<GameOverData>(msg.data);
        EXPECT_EQ(data.winner, "Player1");
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ParsesCorrectlyWithNameContainingSpaces)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("GAME_OVER|John Doe\n");

        EXPECT_EQ(msg.type, MessageType::GAME_OVER);

        const auto &data = std::get<GameOverData>(msg.data);
        EXPECT_EQ(data.winner, "John Doe");
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ParsesWinnerWithSpecialChars)
    {
        auto msg = protocol.parse_message("GAME_OVER|Jugador#1_é\n");
        EXPECT_EQ(msg.type, MessageType::GAME_OVER);

        const auto &data = std::get<BattleShipProtocol::GameOverData>(msg.data);
        EXPECT_EQ(data.winner, "Jugador#1_é");
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ParsesCorrectlyWithEmptyWinner)
    {
        EXPECT_THROW(protocol.parse_message("GAME_OVER|\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ThrowsOnMissingPipeSeparator)
    {
        EXPECT_THROW(protocol.parse_message("GAME_OVER\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_GameOver_ThrowsOnExtraData)
    {
        EXPECT_THROW(protocol.parse_message("GAME_OVER|Player1|Extra\n"), BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Error_ParsesCorrectly)
    {
        BattleShipProtocol::Message msg = protocol.parse_message("ERROR|404,Not found\n");

        EXPECT_EQ(msg.type, BattleShipProtocol::MessageType::ERROR);
        const auto &error_data = std::get<BattleShipProtocol::ErrorData>(msg.data);
        EXPECT_EQ(error_data.code, 404);
        EXPECT_EQ(error_data.description, "Not found");
    }

    TEST_F(ProtocolTest, ParseMessage_Error_NonNumericCodeThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("ERROR|abc,Description\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Error_MissingCommaThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("ERROR|404Description\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Error_EmptyDescriptionThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("ERROR|404,\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Error_EmptyCodeThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("ERROR|,Description\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, ParseMessage_Error_EmptyDataThrows)
    {
        EXPECT_THROW(
            protocol.parse_message("ERROR|\n"),
            BattleShipProtocol::ProtocolError);
    }

    TEST_F(ProtocolTest, BuildMessage_Register_ReturnsCorrectFormat)
    {
        Message msg;
        msg.type = MessageType::REGISTER;
        msg.data = RegisterData{"PlayerOne", "player@example.com"};

        std::string expected = "REGISTER|PlayerOne,player@example.com\n";
        EXPECT_EQ(protocol.build_message(msg), expected);
    }

    TEST_F(ProtocolTest, BuildMessage_PlaceShips)
    {
        Ship s1{ShipType::PORTAAVIONES, {{"A", 1}, {"A", 2}, {"A", 3}}};
        Ship s2{ShipType::SUBMARINO, {{"B", 4}}};
        PlaceShipsData data{{s1, s2}};
        Message msg{MessageType::PLACE_SHIPS, data};

        std::string expected = "PLACE_SHIPS|PORTAAVIONES:A1,A2,A3;SUBMARINO:B4\n";
        EXPECT_EQ(protocol.build_message(msg), expected);
    }

    TEST_F(ProtocolTest, BuildMessage_Shoot)
    {
        ShootData shoot{{"C", 5}};
        Message msg{MessageType::SHOOT, shoot};

        EXPECT_EQ(protocol.build_message(msg), "SHOOT|C5\n");
    }

    TEST_F(ProtocolTest, BuildMessage_Status)
    {
        StatusData data;
        data.turn = Turn::YOUR_TURN;
        data.boardOwn = {
            {{"A", 1}, CellState::SHIP},
            {{"A", 2}, CellState::HIT}};
        data.boardOpponent = {
            {{"B", 1}, CellState::WATER},
            {{"B", 2}, CellState::SUNK}};
        data.gameState = GameState::ONGOING;

        Message msg{MessageType::STATUS, data};

        std::string expected = "STATUS|YOUR_TURN;A1:SHIP,A2:HIT;B1:WATER,B2:SUNK;ONGOING\n";
        EXPECT_EQ(protocol.build_message(msg), expected);
    }

    TEST_F(ProtocolTest, BuildMessage_GameOver)
    {
        GameOverData over{"Carlos"};
        Message msg{MessageType::GAME_OVER, over};

        EXPECT_EQ(protocol.build_message(msg), "GAME_OVER|Carlos\n");
    }

    TEST_F(ProtocolTest, BuildMessage_Error)
    {
        ErrorData err{500, "Internal server error"};
        Message msg{MessageType::ERROR, err};

        EXPECT_EQ(protocol.build_message(msg), "ERROR|500,Internal server error\n");
    }

}
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
