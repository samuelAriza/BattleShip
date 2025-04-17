cmake_minimum_required(VERSION 3.10)
project(Battleship CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir directorios de encabezados
include_directories(protocol/include)
include_directories(server/include)

# Añadir librería del protocolo
add_library(protocol STATIC
    protocol/src/protocol.cpp
    protocol/src/phase_state.cpp  # Asegúrate de agregar este archivo
)
target_include_directories(protocol PUBLIC protocol/include)

# Añadir librería de lógica del juego
add_library(game_logic STATIC
    protocol/src/game_logic.cpp
)
target_include_directories(game_logic PUBLIC protocol/include)
target_link_libraries(game_logic protocol)

# Añadir ejecutable del servidor
add_executable(battleship_server
    server/src/server.cpp
    server/src/main.cpp
)
target_include_directories(battleship_server PRIVATE server/include protocol/include)
target_link_libraries(battleship_server protocol game_logic)

# Añadir ejecutable del cliente
add_executable(bsclient
    client/src/client.cpp
    client/src/main.cpp
)
target_include_directories(bsclient PRIVATE client/include protocol/include)
target_link_libraries(bsclient protocol)

# Buscar GoogleTest para pruebas unitarias
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Ejecutable de pruebas de protocolo
add_executable(protocol_test
    protocol/test/protocol_test.cpp
)
target_link_libraries(protocol_test protocol ${GTEST_LIBRARIES} pthread)

# Ejecutable de pruebas de lógica del juego
add_executable(game_logic_test
    protocol/test/game_logic_test.cpp
)
target_link_libraries(game_logic_test game_logic protocol ${GTEST_LIBRARIES} pthread)

# Ejecutable de pruebas de estado del juego
add_executable(phase_state_test
    protocol/test/phase_state_test.cpp
)
target_link_libraries(phase_state_test protocol ${GTEST_LIBRARIES} pthread)

# Habilitar pruebas
enable_testing()

# Añadir las pruebas
add_test(NAME ProtocolTests COMMAND protocol_test)
add_test(NAME GameLogicTests COMMAND game_logic_test)
add_test(NAME PhaseStateTests COMMAND phase_state_test)  # Añadido para las pruebas de phase_state