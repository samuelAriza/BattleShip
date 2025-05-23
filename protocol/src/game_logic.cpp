// ~/universidad/telematica/test/protocol/src/game_logic.cpp
#include "../include/game_logic.hpp"
#include "../include/protocol.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace BattleShipProtocol
{
    // Lista de inicializacion de miembros. Es mas eficiente de inicializar dentro del cuerpo del constructor porque:
    //  Inicializalos atributos directamente (sin primero crear valores por defecto y luego sobreescribirlos), es obligatoria para incializar referencias, constantes y algunos objetos complejos
    GameLogic::GameLogic() : state_(), current_turn_(1), game_over_(false)
    {
        for (int i = 1; i <= MAX_PLAYERS; ++i)
        {
            int index = 0;
            for (char row = 'A'; row <= 'J'; ++row)
            {
                for (int col = 1; col <= 10; ++col)
                {
                    players_[i].board[index] = Cell{Coordinate{std::string(1, row), col}, CellState::WATER};
                    ++index;
                }
            }
        }
    }

    void GameLogic::register_player(int player_id, const RegisterData &data)
    {
        if (player_id != 1 && player_id != 2)
        {
            throw GameLogicError("Invalid player ID: " + std::to_string(player_id));
        }
        if (data.nickname.empty())
        {
            throw GameLogicError("Nickname cannot be emtpy");
        }
        if (!players_[player_id].nickname.empty())
        {
            throw GameLogicError("Player " + std::to_string(player_id) + " already registered");
        }
        players_[player_id].nickname = data.nickname;
    }

    bool GameLogic::are_both_registered() const
    {
        return !players_.at(1).nickname.empty() && !players_.at(2).nickname.empty();
    }

    bool GameLogic::are_both_ships_placed() const
    {
        return players_.at(1).ships.size() == 9 && players_.at(2).ships.size() == 9;
    }

    size_t GameLogic::ships_placed(int player_id) const
    {
        return players_.at(player_id).ships.size();
    }

    // Mensaje del tipo REGISTER|<register-data>
    //<register-data> ::= <nickname> "," <email>
    //<nickname> ::= <string>
    //<email> ::= <string> "@" <string> "." <string>

    void GameLogic::place_ships(int player_id, const PlaceShipsData &data)
    {
        if (player_id != 1 && player_id != 2)
        {
            throw GameLogicError("Invalid player ID: " + std::to_string(player_id));
        }
        if (players_[player_id].ships.size() == 9)
        { // 1+1+2+2+3 según enunciado
            throw GameLogicError("Ships already placed for Player " + std::to_string(player_id));
        }
        if (!are_both_registered())
        {
            throw GameLogicError("Both players must be registered before placing ships");
        }
        validate_and_place_ships(players_[player_id], data.ships);
    }

    void GameLogic::process_shot(int player_id, const ShootData &shot)
    {
        //if (player_id != current_turn_) {
        //    throw GameLogicError("Not Player " + std::to_string(player_id) + "'s turn");
        //}
        if (player_id == current_turn_)
        {
            if (game_over_)
            {
                throw GameLogicError("Game is already over");
            }
            int target_id = (player_id == 1) ? 2 : 1;
            bool valid_shot = update_board(player_id, target_id, shot.coordinate);
            if (valid_shot)
            {
                current_turn_ = target_id; // Cambiar turno solo si el disparo fue válido
                if (all_ships_sunk(target_id))
                {
                    game_over_ = true;
                    winner_ = players_[player_id].nickname;
                }
            }
            else
            {
                throw GameLogicError("Coordenada ya atacada: " + shot.coordinate.letter + std::to_string(shot.coordinate.number));
            }
        }
    }

    /*
    void GameLogic::surrender(int player_id) {
        if (player_id != 1 && player_id != 2) {
            throw GameLogicError("Invalid player ID: " + std::to_string(player_id));
        }
        if (game_over_) {
            throw GameLogicError("Game is already over");
        }
        players_[player_id].surrendered = true;
        game_over_ = true;
        winner_ = players_[player_id == 1 ? 2 : 1].nickname;
    }
    */

    StatusData GameLogic::get_status(int player_id) const
    {
        if (player_id != 1 && player_id != 2)
        {
            throw GameLogicError("Invalid player ID: " + std::to_string(player_id));
        }
        StatusData status;
        status.turn = (current_turn_ == player_id) ? Turn::YOUR_TURN : Turn::OPPONENT_TURN;
        //.begin devuelve un iterador apuntando al primer elemento del array board de player_
        // Realiza copia del array map usando constructor de rango
        status.boardOwn = std::vector<Cell>(players_.at(player_id).board.begin(), players_.at(player_id).board.end());
        status.boardOpponent = std::vector<Cell>(players_.at(player_id == 1 ? 2 : 1).board.begin(), players_.at(player_id == 1 ? 2 : 1).board.end());
        if (get_phase() == PhaseState::Phase::FINISHED)
        {
            status.gameState = BattleShipProtocol::GameState::ENDED;
        }
        else if (are_both_registered() && are_both_ships_placed())
        {
            status.gameState = BattleShipProtocol::GameState::ONGOING;
        }
        else
        {
            status.gameState = BattleShipProtocol::GameState::WAITING;
        }

        return status;
    }

    bool GameLogic::is_game_over() const noexcept
    {
        return game_over_;
    }

    GameOverData GameLogic::get_game_over_result() const
    {
        if (!game_over_)
        {
            throw GameLogicError("Game is not over yet");
        }
        return GameOverData{winner_.value_or("NONE")};
    }

    std::string GameLogic::get_player_nickname(int player_id) const
    {
        auto it = players_.find(player_id);
        if (it == players_.end())
        {
            throw GameLogicError("Player ID not found");
        }
        return it->second.nickname;
    }

    // coord_to_index(shot);
    int GameLogic::coord_to_index(const Coordinate &coord) const
    {
        // Resta de valores ASCII, row termina siendo una posicion basada en 0
        int row = coord.letter[0] - 'A';
        int col = coord.number - 1;
        if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        {
            throw GameLogicError(
                "Coordinate out of bounds: expected format <coord> ::= <letter><number>, "
                "where <letter> ::= \"A\" to \"J\" and <number> ::= \"1\" to \"10\". "
                "Received: \"" +
                coord.letter + std::to_string(coord.number) + "\".");
        }
        return row * BOARD_SIZE + col;
    }

    void GameLogic::validate_and_place_ships(Player &player, const std::vector<Ship> &ships)
    {
        if (ships.size() != 9)
        {
            throw GameLogicError("Incorrect number of ships: " + std::to_string(ships.size()));
        }

        std::map<ShipType, std::pair<int, int>> required_ships = {
            {ShipType::PORTAAVIONES, {1, 5}},
            {ShipType::BUQUE, {1, 4}},
            {ShipType::CRUCERO, {2, 3}},
            {ShipType::DESTRUCTOR, {2, 2}},
            {ShipType::SUBMARINO, {3, 1}}};
        std::map<ShipType, int> ship_counts;

        // Para cada barco, se cuenta el tipo y validar las coordenadas
        for (const auto &ship : ships)
        {
            ship_counts[ship.type]++;
            int expected_size = required_ships[ship.type].second;
            if (ship.coordinates.size() != static_cast<size_t>(expected_size))
            {
                throw GameLogicError("Invalid ship configuration: coordinate count mismatch.");
            }
        }

        // Validar cantidad de barcos
        for (const auto &[type, count_size] : required_ships)
        {
            if (ship_counts[type] != count_size.first)
            {
                throw GameLogicError("Ship count does not match the required configuration.");
            }
        }

        // Colocación sin superposición
        std::array<bool, BOARD_SIZE * BOARD_SIZE> occupied{};
        for (const auto &ship : ships)
        {
            for (const auto &coord : ship.coordinates)
            {
                int idx = coord_to_index(coord);
                if (occupied[idx])
                {
                    throw GameLogicError("Ship overlap at " + coord.letter + std::to_string(coord.number));
                }
                occupied[idx] = true;
                player.board[idx] = Cell{coord, CellState::SHIP};
            }
        }

        player.ships = ships;
    }

    bool GameLogic::update_board(int shooter_id, int target_id, const Coordinate &shot)
    {
        try
        {
            int idx = coord_to_index(shot);
            auto &target_board = players_[target_id].board;

            // Verificar si la coordenada ya fue atacada
            if (target_board[idx].cellState == CellState::HIT ||
                target_board[idx].cellState == CellState::SUNK ||
                target_board[idx].cellState == CellState::MISS)
            {
                return false; // Disparo inválido, no se cambia de turno
            }

            // Coordenada válida, aplicar el disparo
            if (target_board[idx].cellState == CellState::SHIP)
            {
                target_board[idx].cellState = CellState::HIT;

                // Verificar si se hundió un barco
                for (auto &ship : players_[target_id].ships)
                {
                    bool all_hit = true;
                    for (const auto &coord : ship.coordinates)
                    {
                        if (players_[target_id].board[coord_to_index(coord)].cellState != CellState::HIT)
                        {
                            all_hit = false;
                            break;
                        }
                    }
                    if (all_hit)
                    {
                        for (const auto &coord : ship.coordinates)
                        {
                            players_[target_id].board[coord_to_index(coord)].cellState = CellState::SUNK;
                        }
                        players_[target_id].ships_remaining--;
                    }
                }
            }
            else if (target_board[idx].cellState == CellState::WATER)
            {
                target_board[idx].cellState = CellState::MISS;
            }

            return true; // Disparo válido, puede cambiar de turno
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Error inesperado en update_board: " << e.what() << std::endl;
            return false;
        }
    }
    bool GameLogic::all_ships_sunk(int player_id) const
    {
        return players_.at(player_id).ships_remaining == 0;
    }

} // namespace BattleShipProtocol