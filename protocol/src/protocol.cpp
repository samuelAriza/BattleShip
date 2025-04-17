#include "../include/protocol.hpp"
#include <sstream>
#include <charconv> // Necesario para std::from_chars
#include <iostream>
#include <stdexcept>

namespace BattleShipProtocol
{
    Message Protocol::parse_message(std::string_view raw_message) const
    {

        auto delim = raw_message.find('|');
        std::string_view type_str = raw_message.substr(0, delim);
        std::string_view type_data = raw_message.substr(delim + 1, raw_message.length() - 1);

        if (delim == std::string_view::npos)
        {
            throw ProtocolError("Invalid message format. Format expected: <message> ::= <message-type> \"|\" <message-data>");
        }

        /*
        if (type_data.find('|') != std::string_view::npos){
            throw ProtocolError("Invalid message format. Format expected: <message> ::= <message-type> \"|\" <message-data>");
        }
        */

        Message msg;
        msg.type = string_to_message_type(type_str);

        switch (msg.type)
        {
        case MessageType::PLAYER_ID:
            msg.data = parse_player_id_data(type_data);
            break;
        case MessageType::REGISTER:
            msg.data = parse_register_data(type_data);
            break;
        case MessageType::PLACE_SHIPS:
            msg.data = parse_place_ships_data(type_data);
            break;
        case MessageType::SHOOT:
            msg.data = parse_shoot_data(type_data);
            break;
        case MessageType::STATUS:
            msg.data = parse_status_data(type_data);
            break;
        case MessageType::SURRENDER:
            msg.data = std::monostate{};
            break;
        case MessageType::GAME_OVER:
            msg.data = parse_game_over_data(type_data);
            break;
        case MessageType::ERROR:
            msg.data = parse_error_data(type_data);
            break;
        default:
            break;
        }
        return msg;
    }

    MessageType Protocol::string_to_message_type(std::string_view type_str) const
    {
        if (type_str == "PLAYER_ID")
            return MessageType::PLAYER_ID;
        if (type_str == "REGISTER")
            return MessageType::REGISTER;
        if (type_str == "PLACE_SHIPS")
            return MessageType::PLACE_SHIPS;
        if (type_str == "SHOOT")
            return MessageType::SHOOT;
        if (type_str == "STATUS")
            return MessageType::STATUS;
        if (type_str == "SURRENDER")
            return MessageType::SURRENDER;
        if (type_str == "GAME_OVER")
            return MessageType::GAME_OVER;
        if (type_str == "ERROR")
            return MessageType::ERROR;
        throw ProtocolError("Invalid type:" + std::string(type_str));
    }

    PlayerIdData Protocol::parse_player_id_data(std::string_view data) const
    {
        PlayerIdData player_id_data;
        int player_id;
        auto [ptr, ec] = std::from_chars(data.data(), data.data() + data.size(), player_id);

        player_id_data.player_id = player_id;

        return player_id_data;
    }

    RegisterData Protocol::parse_register_data(std::string_view data) const
    {
        RegisterData registerData;

        // Eliminar el '\n'
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        auto delim = data.find(',');

        if (delim == std::string_view::npos)
        {
            throw ProtocolError("Invalid message format: missing ','");
        }

        std::string_view str_nickname = data.substr(0, delim);
        std::string_view str_email = data.substr(delim + 1);

        if (str_nickname.empty())
        {
            throw ProtocolError("Nickname field cannot be empty");
        }

        if (str_email.empty())
        {
            throw ProtocolError("Email field cannot be empty");
        }

        registerData.nickname = str_nickname;
        registerData.email = str_email;

        return registerData;
    }

    /*
    struct Ship{
    ShipType type;
    Coordinate coordinate;
    };

    struct PlaceShipsData {
        std:: vector<Ship> ships;
    };
    */

    PlaceShipsData Protocol::parse_place_ships_data(std::string_view data) const
    {

        if (data.empty())
        {
            throw ProtocolError("<message-data> for PLACE_SHIPS cannot be empty");
        }

        // Eliminar el '\n'
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        std::vector<Ship> ships;
        std::string data_str(data);
        std::istringstream ships_stream(data_str);
        std::string ship_segment;

        while (std::getline(ships_stream, ship_segment, ';'))
        {
            if (ship_segment.empty())
            {
                throw ProtocolError("Empty ship definition encountered");
            }

            auto colon_pos = ship_segment.find(':');
            if (colon_pos == std::string::npos)
            {
                throw ProtocolError("Missing ':' in ship definition: expected format <ship-type> ':' <coordinates>");
            }

            std::string_view type_str = std::string_view(ship_segment).substr(0, colon_pos);
            std::string_view coords_str = std::string_view(ship_segment).substr(colon_pos + 1);

            ShipType type = string_to_ship_type(type_str);

            if (coords_str.empty())
            {
                throw ProtocolError("No coordinates provided for ship type: " + std::string(type_str));
            }

            std::vector<Coordinate> coordinates;
            std::istringstream coord_stream{std::string(coords_str)};
            std::string coord_token;
            while (std::getline(coord_stream, coord_token, ','))
            {
                if (coord_token.empty())
                {
                    throw ProtocolError("Empty coordinate found in ship definition");
                }
                coordinates.push_back(string_to_coordinate(coord_token));
            }

            if (coordinates.empty())
            {
                throw ProtocolError("No valid coordinates parsed for ship type: " + std::string(type_str));
            }

            ships.push_back({type, coordinates});
        }

        if (ships.empty())
        {
            throw ProtocolError("No valid ships parsed from PLACE_SHIPS data");
        }
        return PlaceShipsData{ships};
    }

    ShipType Protocol::string_to_ship_type(std::string_view type) const
    {
        if (type == "PORTAAVIONES")
            return ShipType::PORTAAVIONES;
        if (type == "BUQUE")
            return ShipType::BUQUE;
        if (type == "CRUCERO")
            return ShipType::CRUCERO;
        if (type == "DESTRUCTOR")
            return ShipType::DESTRUCTOR;
        if (type == "SUBMARINO")
            return ShipType::SUBMARINO;
        throw ProtocolError("Invalid ship type:" + std::string(type));
    }

    std::vector<Coordinate> Protocol::string_to_coordinates(std::string_view coords_str) const
    {
        std::vector<Coordinate> coordinates;

        if (coords_str.empty())
        {
            throw ProtocolError("Coordinate list cannot be empty");
        }

        std::string coords_string(coords_str); // Para usar std::istringstream
        std::istringstream ss(coords_string);
        std::string coord_token;

        while (std::getline(ss, coord_token, ','))
        {
            if (coord_token.empty())
            {
                throw ProtocolError("Empty coordinate found in list");
            }
            coordinates.push_back(string_to_coordinate(coord_token));
        }

        if (coordinates.empty())
        {
            throw ProtocolError("No valid coordinates found in the list");
        }

        return coordinates;
    }

    std::string Protocol::coordinates_to_string(const std::vector<Coordinate> &coordinates) const
    {
        if (coordinates.empty())
        {
            throw ProtocolError("Cannot serialize an empty list of coordinates");
        }

        std::ostringstream oss;

        for (size_t i = 0; i < coordinates.size(); ++i)
        {
            const auto &coord = coordinates[i];
            if (coord.letter.empty() || coord.number <= 0)
            {
                throw ProtocolError("Invalid coordinate found while converting to string");
            }
            oss << coord.letter << coord.number;
            if (i != coordinates.size() - 1)
            {
                oss << ",";
            }
        }

        return oss.str();
    }

    Coordinate Protocol::string_to_coordinate(std::string_view coor) const
    {
        if (coor.length() > 4 || coor.length() < 2)
        {
            throw ProtocolError("Invalid coordinate format. Expected format: <Letter><Number>");
        }
        Coordinate coordinate;
        char letter = coor.front();
        int number;

        /*
        Lo siguiente utiliza estructuras de retorno con desestructuracion en C++ 17 para extraer multiples valores de la funcion std::from_chars.

        Realiza una conversion numerica eficiente, funcion de <charconv>, se utiliza para convertir datos numericos desde una secuencia de caracteres sin necesidad de usar std::stringstream o crear objetos std::string. Su firma es:

        std::from_chars_result from_chars(const char* first, const char* last, T& value, int base = 10);

        first: Puntero al inicio de la cadena de caracteres.
        last: Puntero al final de la cadena.
        value: Referencia a la variable donde se almacenará el resultado.
        base: Base numérica (por defecto, 10).

        struct from_chars_result {
        const char* ptr;  // Puntero al primer carácter no convertido
        std::errc ec;     // Código de error (std::errc::invalid_argument, std::errc::result_out_of_range, etc.)
        };
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
        auto [ptr, ec] usa estructuras de retorno con desestructuración (C++17) para extraer los valores del std::from_chars_result en dos variables separadas:

        ptr: Un puntero (const char*) que apunta al primer carácter no convertido en la cadena.

        ec: Un std::errc que indica si hubo errores durante la conversión.

        Si ec no es std::errc(), significa que std::from_chars no pudo convertir la cadena:

        std::errc::invalid_argument: La cadena no contenía un número válido.

        std::errc::result_out_of_range: El número es demasiado grande para int.

        */
        auto [ptr, ec] = std::from_chars(coor.substr(1, coor.length() - 1).data(), coor.substr(1, coor.length() - 1).data() + coor.substr(1, coor.length() - 1).size(), number);
        if (ec != std::errc())
        {
            throw ProtocolError("Invalid number: " + std::string(coor));
        }

        coordinate.letter = letter;
        coordinate.number = number;
        return coordinate;
    }

    ShootData Protocol::parse_shoot_data(std::string_view data) const
    {
        // Eliminar el '\n'
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        ShootData shoot_data;
        shoot_data.coordinate = string_to_coordinate(data);
        return shoot_data;
    }

    // Función auxiliar para parsear un tablero desde una string_view
    std::vector<Cell> Protocol::parse_board_data(std::string_view board_str) const
    {
        std::vector<Cell> board;
        if (board_str.empty())
        {
            return board; // Tablero vacío es válido, devuelve vector vacío
        }

        const char delim = ',';
        size_t start = 0, end = 0;
        while ((end = board_str.find(delim, start)) != std::string_view::npos)
        {
            std::string_view cell_str = board_str.substr(start, end - start);
            if (cell_str.empty())
            {
                throw ProtocolError("Empty cell specification in board");
            }
            board.emplace_back(string_to_cell(cell_str));
            start = end + 1;
        }
        // Parsear el último elemento (o único si no hay delimitadores)
        std::string_view last_cell_str = board_str.substr(start);
        if (!last_cell_str.empty())
        {
            board.emplace_back(string_to_cell(last_cell_str));
        }

        return board;
    }
    StatusData Protocol::parse_status_data(std::string_view data) const
    {
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        StatusData status_data{};
        const char delim = ';';
        size_t start = 0, end = 0;

        // 1. Turno
        end = data.find(delim, start);
        if (end == std::string_view::npos)
            throw ProtocolError("Missing turn delimiter in STATUS data");
        status_data.turn = string_to_turn(data.substr(start, end - start));
        start = end + 1;

        // 2. Tablero propio
        end = data.find(delim, start);
        if (end == std::string_view::npos)
            throw ProtocolError("Missing board_own delimiter in STATUS data");
        status_data.boardOwn = parse_board_data(data.substr(start, end - start));
        start = end + 1;

        // 3. Tablero del oponente
        end = data.find(delim, start);
        if (end == std::string_view::npos)
            throw ProtocolError("Missing board_opponent delimiter in STATUS data");
        status_data.boardOpponent = parse_board_data(data.substr(start, end - start));
        start = end + 1;

        // 4. Estado del juego
        end = data.find(delim, start);
        if (end == std::string_view::npos)
            throw ProtocolError("Missing game_state or time_remaining in STATUS data");
        status_data.gameState = string_to_game_state(data.substr(start, end - start));
        start = end + 1;

        // 5. Tiempo restante
        std::string_view time_str = data.substr(start);
        int seconds;
        auto [ptr, ec] = std::from_chars(time_str.data(), time_str.data() + time_str.size(), seconds);
        if (ec != std::errc())
        {
            throw ProtocolError("Invalid time_remaining in STATUS data: " + std::string(time_str));
        }
        status_data.time_remaining = seconds;

        return status_data;
    }

    Turn Protocol::string_to_turn(std::string_view turn) const
    {
        if (turn == "OPPONENT_TURN")
            return Turn::OPPONENT_TURN;
        if (turn == "YOUR_TURN")
            return Turn::YOUR_TURN;
        throw ProtocolError("Invalid turn:" + std::string(turn));
    }

    CellState Protocol::string_to_cell_state(std::string_view state) const
    {
        if (state == "WATER")
            return CellState::WATER;
        if (state == "HIT")
            return CellState::HIT;
        if (state == "SUNK")
            return CellState::SUNK;
        if (state == "SHIP")
            return CellState::SHIP;
        if (state == "MISS")
            return CellState::MISS;
        throw ProtocolError("Invalid cell state:" + std::string(state));
    }

    GameState Protocol::string_to_game_state(std::string_view state) const
    {
        if (state == "ONGOING")
            return GameState::ONGOING;
        if (state == "WAITING")
            return GameState::WAITING;
        if (state == "ENDED")
            return GameState::ENDED;
        throw ProtocolError("Invalid game state:" + std::string(state));
    }

    Cell Protocol::string_to_cell(std::string_view cell_str) const
    {
        Cell cell;

        auto delim = cell_str.find(':');
        std::string_view str_coordinate = cell_str.substr(0, delim);
        std::string_view str_cell_state = cell_str.substr(delim + 1, cell_str.length() - 1);

        cell.coordinate = string_to_coordinate(str_coordinate);
        cell.cellState = string_to_cell_state(str_cell_state);

        return cell;
    }

    GameOverData Protocol::parse_game_over_data(std::string_view data) const
    {
        // Eliminar el '\n'
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        if (data.empty())
        {
            throw ProtocolError("Game over data cannot be empty");
        }

        auto pipe_delim = data.find('|');
        if (pipe_delim != std::string_view::npos)
        {
            throw ProtocolError("Invalid message format. Expected: <message> ::= <message-type> '|' <message-data>");
        }
        return GameOverData{std::string(data)};
    }

    ErrorData Protocol::parse_error_data(std::string_view data) const
    {
        // Eliminar el '\n'
        if (!data.empty() && data.back() == '\n')
        {
            data.remove_suffix(1);
        }
        else
        {
            throw ProtocolError("Invalid message format: missing end delimiter");
        }

        ErrorData error_data;
        int error_code;
        auto pipe_delim = data.find('|');

        if (pipe_delim != std::string_view::npos)
        {
            throw ProtocolError("Invalid message format. Expected: <message> ::= <message-type> '|' <message-data>");
        }
        auto delim = data.find(',');

        if (delim == std::string_view::npos)
        {
            throw ProtocolError("Invalid message format: missing ','");
        }

        std::string_view code_str = data.substr(0, delim);
        auto [ptr, ec] = std::from_chars(code_str.data(), code_str.data() + code_str.size(), error_code);
        if (ec != std::errc())
        {
            throw ProtocolError("Invalid code number: " + std::string(code_str));
        }
        std::string_view description = data.substr(delim + 1, data.length() - 1);
        if (description.empty())
        {
            throw ProtocolError("Description is empty");
        }
        error_data.code = error_code;
        error_data.description = description;

        return error_data;
    }

    std::string Protocol::message_type_to_string(MessageType type) const
    {
        switch (type)
        {
        case MessageType::PLAYER_ID:
            return "PLAYER_ID";
        case MessageType::REGISTER:
            return "REGISTER";
        case MessageType::PLACE_SHIPS:
            return "PLACE_SHIPS";
        case MessageType::SHOOT:
            return "SHOOT";
        case MessageType::STATUS:
            return "STATUS";
        case MessageType::SURRENDER:
            return "SURRENDER";
        case MessageType::GAME_OVER:
            return "GAME_OVER";
        case MessageType::ERROR:
            return "ERROR";
        default:
            throw ProtocolError("Unknown MessageType value encountered");
        }
    }

    std::string Protocol::ship_type_to_string(ShipType type) const
    {
        switch (type)
        {
        case ShipType::PORTAAVIONES:
            return "PORTAAVIONES";
        case ShipType::BUQUE:
            return "BUQUE";
        case ShipType::CRUCERO:
            return "CRUCERO";
        case ShipType::DESTRUCTOR:
            return "DESTRUCTOR";
        case ShipType::SUBMARINO:
            return "SUBMARINO";
        default:
            throw ProtocolError("Unknown ShipType value encountered");
        }
    }

    std::string Protocol::turn_to_string(Turn turn) const
    {
        switch (turn)
        {
        case Turn::YOUR_TURN:
            return "YOUR_TURN";
        case Turn::OPPONENT_TURN:
            return "OPPONENT_TURN";
        default:
            throw ProtocolError("Unknown Turn value encountered");
        }
    }

    std::string Protocol::cell_state_to_string(CellState state) const
    {
        switch (state)
        {
        case CellState::WATER:
            return "WATER";
        case CellState::HIT:
            return "HIT";
        case CellState::SUNK:
            return "SUNK";
        case CellState::SHIP:
            return "SHIP";
        case CellState::MISS:
            return "MISS";
        default:
            throw ProtocolError("Unknown CellState value encountered");
        }
    }

    std::string Protocol::game_state_to_string(GameState state) const
    {
        switch (state)
        {
        case GameState::ONGOING:
            return "ONGOING";
        case GameState::WAITING:
            return "WAITING";
        case GameState::ENDED:
            return "ENDED";
        default:
            throw ProtocolError("Unknown GameState value encountered");
        }
    }

    /*
    std::ostringstream oss;

    std::ostringstream es un stream de salida para construir cadenas eficientemente.

    Actúa como un buffer en memoria, evitando concatenaciones costosas con std::string.

    case MessageType::PLACE_SHIPS: {
    oss << "PLACE_SHIPS|";

    Detecta del tipo del mensaje, por lo que debe formatearse con la estructura segun BNF

    Se escribe el prefijo "PLACE_SHIPS|" en el ostringstream, que será la cabecera del mensaje.

    std::get<PlaceShipsData>(msg.data):
    Extrae el valor almacenado en msg.data suponiendo que es de tipo PlaceShipsData.

    const auto&:
    const: No queremos modificar data.
    auto&: Deducimos el tipo (PlaceShipsData) y evitamos copiar el valor.

    msg.data es realmente el std::variant que tenemos para MessageData

    std::get<T>(variant) obtiene el valor almacenado en un std::variant, si y solo si el tipo actual coincide con T.
    Si msg.data contiene un PlaceShipsData, std::get<PlaceShipsData>(msg.data) lo devuelve.
    Si msg.data contiene otro tipo, se genera una excepción std::bad_variant_access
    */

    std::string Protocol::build_message(const Message &msg) const
    {
        std::ostringstream oss;
        switch (msg.type)
        {
        case MessageType::PLAYER_ID:
        {
            oss << "PLAYER_ID|";
            const auto &data = std::get<PlayerIdData>(msg.data);
            oss << data.player_id;
            break;
        }
        case MessageType::REGISTER:
        {
            oss << "REGISTER|";
            const auto &data = std::get<RegisterData>(msg.data);
            oss << data.nickname << "," << data.email;
            break;
        }
        case MessageType::PLACE_SHIPS:
        {
            oss << "PLACE_SHIPS|";
            const auto &data = std::get<PlaceShipsData>(msg.data);
            for (size_t i = 0; i < data.ships.size(); ++i)
            {
                oss << ship_type_to_string(data.ships[i].type) << ":" << coordinates_to_string(data.ships[i].coordinates);
                if (i < data.ships.size() - 1)
                {
                    oss << ";";
                }
            }
            break;
        }
        case MessageType::SHOOT:
        {
            oss << "SHOOT|";
            const auto &data = std::get<ShootData>(msg.data);
            oss << data.coordinate.letter << data.coordinate.number;
            break;
        }
        case MessageType::STATUS:
        {
            oss << "STATUS|";
            const auto &data = std::get<StatusData>(msg.data);
            oss << turn_to_string(data.turn) << ";";
            for (size_t i = 0; i < data.boardOwn.size(); i++)
            {
                oss << data.boardOwn[i].coordinate.letter << data.boardOwn[i].coordinate.number << ":" << cell_state_to_string(data.boardOwn[i].cellState);
                if (i < data.boardOwn.size() - 1)
                    oss << ",";
            }
            oss << ";";
            for (size_t i = 0; i < data.boardOpponent.size(); i++)
            {
                oss << data.boardOpponent[i].coordinate.letter << data.boardOpponent[i].coordinate.number << ":" << cell_state_to_string(data.boardOpponent[i].cellState);
                if (i < data.boardOpponent.size() - 1)
                    oss << ",";
            }
            oss << ";";
            oss << game_state_to_string(data.gameState);
            oss << ";";
            oss << data.time_remaining;
            break;
        }
        case MessageType::SURRENDER:
        {
            oss << "SURRENDER|";
            break;
        }
        case MessageType::GAME_OVER:
        {
            oss << "GAME_OVER|";
            const auto &data = std::get<GameOverData>(msg.data);
            oss << data.winner;
            break;
        }
        case MessageType::ERROR:
        {
            oss << "ERROR|";
            const auto &data = std::get<ErrorData>(msg.data);
            oss << data.code << ',' << data.description;
            break;
        }
        }
        oss << "\n";
        return oss.str();
    }
}