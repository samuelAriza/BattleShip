# Battle Ship Protocol
### Table of Contents
- [1. Introduction](#1-introduction)
  - [1.1 Purpose of the Document](#11-purpose-of-the-document)
  - [1.2 Project Overview](#12-project-overview)
  - [1.3 Scope and Objectives](#13-scope-and-objectives)
  - [1.4 Audience](#14-audience)
- [2. System Overview](#2-system-overview)
  - [2.1 High-Level Description of the Battleship Game](#21-high-level-description-of-the-battleship-game)
  - [2.2 Key Features and Functionality](#22-key-features-and-functionality)
  - [2.3 Assumptions and Constraints](#23-assumptions-and-constraints)
- [3. Requirements Analysis](#3-requirements-analysis)
- [4. Architecture Design](#4-architecture-design)
  - [4.1 Client-Server Model Overview](#41-client-server-model-overview)
  - [4.2 Component Diagram](#42-component-diagram)
  - [4.3 Protocol specification](#43-protocol-specification)
    - [4.3.1 Notational Conventions and Generic Grammar (BNF)](#431-notational-conventions-and-generic-grammar-bnf)
    - [4.3.2  Protocol Message Examples](#432-protocol-message-examples)
- [5. Detailed Design](#5-detailed-design)
  - [5.1 Class Diagram](#51-class-diagram)
  - [5.2 Concurrency Model](#52-concurrency-model)
  - [5.3 State Machine Diagram](#53-state-machine-diagram)
- [6. Development Environment](#6-development-environment)
  - [6.1 Tools and Libraries Used](#61-tools-and-libraries-used)
  - [6.2 Language-Specific Notes](#62-language-specific-notes)
  - [6.3 Deployment Instructions](#63-deployment-instructions)
    - [6.3.1 Server Deployment](#631-server-deployment)
    - [6.3.2 Client Execution](#632-client-execution)
- [7. Testing and Validation](#7-testing-and-validation)
- [8. Conclusion](#8-conclusion)


## 1. Introduction
### 1.1 Purpose of the Document
This document serves as the comprehensive design and implementation guide for the Battleship network programming project, developed as part of the ST0255 Telemática course. It outlines the system's architecture, technical specifications, and development process, providing a clear roadmap for stakeholders, including developers, instructors, and evaluators. The document details the client-server architecture, custom application-layer protocol, concurrency model, and testing strategy, ensuring alignment with the project's academic and technical objectives.

### 1.2 Project Overview
The Battleship project involves designing and implementing a multiplayer online version of the classic two-player strategy game, Batalla Naval, using the Berkeley Sockets API. The system comprises a server and a client, both implemented in C++. The server manages game sessions for multiple pairs of players, enforces game rules, and synchronizes state, while clients provide user interfaces for players to interact with the game. A custom text-based protocol facilitates communication over TCP/IP, enabling reliable data exchange. 

### 1.3 Scope and Objectives
The scope of the project includes:
- Deloping a server that handles multiple concurrent game sessions, validates player actions, and logs events.
- Creating a client application for player interaction, including registration, ship placement, and gameplay.
- Designing and implementing a text-based application-layer protocol for client-server communication.
- Deploying the server on an AWS Academy instance.
- Documenting the system design, implementation, and testing using UML diagrams and a README.md file

The objectives, as outlined in the project documentation, are to:
- Implement TCP/IP socket communication in a client-server architecture.
- Design and implement a custom application-layer protocol.
- Manage concurrency and multiple client connections on the server.
- Synchronize game states across clients.
- Apply data validation and error control in a distributed environment.
- Document the system and protocol comprehensively.

### 1.4 Audience
This document is intended for:
- Development Team: 

|  Name  |  GitHub  | LinkedIn   |
| ------------ | ------------ | ------------ |
| Samuel Andrés Ariza Gómez  | [samuelAriza](https://github.com/samuelAriza "samuelAriza")|  [Samuel Ariza Gómez](https://www.linkedin.com/in/samuel-ariza-g%C3%B3mez-84a12b293/ "Samuel Ariza Gómez") |
|  Andrés Vélez Rendón |  [AndresVelezR](https://github.com/AndresVelezR "AndresVelezR") |  [Andrés Vélez Rendón](https://www.linkedin.com/in/andres-velez-rendon-b5a499285/ "Andrés Vélez Rendón") |

- Instructors and Evaluators: Faculty members assessing the project against academic and technical criteria.

- Future Developers: Individuals who may extend or maintain the system, requiring insight into its design and implementation.


## 2. System Overview
### 2.1 High-Level Description of the Battleship Game
The Battleship game is a networked multiplayer implementation of the traditional two-player strategy game, Battleship. Players place a fleet of ships on a 10x10 grid, hidden from their opponent, and alternate shooting at the coordinates to sink the opponent's ships. The game progresses through three general phases: registration (registration of players with nickname and email), preparation (placement of ships), gameplay (turn-based shooting) and conclusion (when a player's fleet is completely sunk). Built on a client-server architecture, the system uses TCP/IP sockets for communication, with a central server managing game state and clients providing user interfaces. A custom text-based protocol ensures reliable data exchange between components.

### 2.2 Key Features and Functionality
The system supports the following features:
- Multiplayer Capability: Enables multiple pairs of players to engage in simultaneous game sessions.
- Client-Server Architecture:
	- Clients: Allow players to register, place ships, fire shots, and view game states (own and opponent boards).
	- Server: Manages client connections, pairs players, maintains game state, validates actions, and synchronizes updates.
- Custom Protocol: A text-based protocol defines message formats (`<message> ::= <message-type> "|" <message-data>`) for client-server communication.
- Concurrency: Utilizes threads to handle multiple client connections and game sessions concurrently.
- Turn Management: Enforces a 30-second turn limit to ensure timely gameplay.
- State Synchronization: Maintains consistent game states across clients, reflecting ship positions, shot outcomes (hit, miss, sunk), and turn status.
- Error and Disconnection Handling: Detects client disconnections, notifies affected players, and releases resources.
- Logging: Logs server events (e.g., connections, actions, errors) in a file with the format date time clientIP query response.
- Cloud Deployment: Runs the server on an AWS Academy instance for internet accessibility.

### 2.3 Assumptions and Constraints
Assumptions and limitations:
- Assumptions
	- Players maintain stable network connections, and the system handles disconnections gracefully.
	- The clients and server operate on platforms compatible with the Berkeley Sockets API.
	- Each game session involves exactly two players, matched by the server.
	- The game follows standard Battleship rules, with a fixed fleet configuration.
	- Sufficient system resources are available to support concurrent sessions.

- Restrictions
	- Implementation language: The server must be in C/C++ using the Berkeley Sockets API; clients may use any language that supports sockets.
	- Protocol: Must be text-based and run over TCP/IP.
	- Concurrency: The server must use threads for concurrent client management.
	- Implementation: The server must be deployed on AWS Academy.
	 Turn timer: Limit of 30 seconds per player turn, imposed by the server.
	- Messages: Messages must follow the specified format (`<message> ::= <message-type> "|" <message-data>`)

## 3. Requirements Analysis
For a detailed overview of the functional and non-functional requirements of this project, please refer to the project's backlog. It contains the full specification of features, user stories, and technical considerations.

🔗 [Click here to view the project backlog](https://github.com/users/samuelAriza/projects/4)

## 4. Architecture Design
### 4.1 Client-Server Model Overview
### 4.2 Component Diagram 
### 4.3 Protocol specification
#### 4.3.1 Notational Conventions and Generic Grammar (BNF)
The following BNF (Backus-Naur Form) defines the formal grammar for the communication protocol used in the client-server interaction of the Battleship game. This protocol is designed to ensure strict syntactic and semantic consistency across all message exchanges, enabling reliable parsing, validation, and processing of game-related instructions and states.

The BNF grammar below specifies all valid message types, their corresponding payloads, and the permissible structure and constraints of each component, including nicknames, coordinates, board states, and error reporting formats.

    <message> ::= <message-type> "|" <message-data>
    <message-type> ::= "REGISTER" | "PLACE_SHIPS" | "SHOOT" | "STATUS" | "SURRENDER" | "GAME_OVER" | "ERROR" 
    <message-data> ::= <register-data> | <place-ships-data> | <shoot-data> | <status-data> | <surrender-data> | <game-over-data> | <error-data> 
    <register-data> ::= <nickname> "," <email> 
    <nickname> ::= <string> 
    <email> ::= <string> "@" <string> "." <string> 
    <place-ships-data> ::= <ship-list> 
    <ship-list> ::= <ship> | <ship> ";" <ship-list> 
    <ship> ::= <ship-type> ":" <coordinates> 
    <ship-type> ::= "PORTAAVIONES" | "BUQUE" | "CRUCERO" | "DESTRUCTOR" | "SUBMARINO" 
    <coordinates> ::= <coord>  | <coord> "," <coordinates> ","  
    <coord> ::= <letter><number> 
    <letter> ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" 
    <number> ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "10" 
    <shoot-data> ::= <coord>
    <status-data> ::= <turn> ";" <board-own> ";" <board-opponent> ";" <game-state> 
    <turn> ::= "YOUR_TURN" | "OPPONENT_TURN" 
    <board-own> ::= <cell-list> 
    <board-opponent> ::= <cell-list> 
    <cell-list> ::= <cell> | <cell> "," <cell-list> 
    <cell> ::= <coord> ":" <cell-state> 
    <cell-state> ::= "WATER" | "HIT" | "SUNK" | "SHIP" 
    <game-state> ::= "ONGOING" | "WAITING" | "ENDED" 
    <surrender-data> ::= "" 
    <game-over-data> ::= <winner> 
    <winner> ::= <nickname> | "NONE" 
    <error-data> ::= <error-code> "," <error-description> 
    <error-code> ::= <digit><digit><digit>
    <digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" 
    <error-description> ::= <string> 
    <string> ::= <char> | <char><string> 
    <char> ::= <letter> | <digit> | "_" | "-" | "."

#### 4.3.2 Protocol Message Examples
Below are example messages that conform strictly to the defined BNF grammar. These examples illustrate how each message type should be formatted and parsed during client-server communication.

- REGISTER:
Registers a new player in the system.
`REGISTER|john_doe,john.doe@example.com`
- PLACE_SHIPS:
Sends the player's ship configuration to the server.
`PLACE_SHIPS|PORTAAVIONES:A1,A5;BUQUE:B1,B3;CRUCERO:C1,C3;DESTRUCTOR:D1,D2;SUBMARINO:E1,E1`
- SHOOT:
Submits a shot to a specific coordinate on the opponent's board.
`SHOOT|F6`
- STATUS:
Retrieves the current game status including turn, board state (own board and opponent's board), and game state.
`STATUS|YOUR_TURN;A1:SHIP,A2:WATER,A3:HIT;B1:WATER,B2:HIT;ONGOING`
- SURRENDER:
Indicates that the player surrenders. This message carries no data.
`SURRENDER|`
- GAME_OVER:
Sent when the game ends, declaring the winner.
`GAME_OVER|john_doe`
Or if the game ends in a draw:
`GAME_OVER|NONE`
- ERROR:
Returns a structured error code and description.
`ERROR|404,Invalid coordinate provided`


## 5 Detailed Design
### 5.1 Class Diagram 
![UML Class Diagram](assets/class_diagram.png)
### 5.2 Concurrency Model
### 5.3 State Machine Diagram


## 6 Development Environment
### 6.1 Tools and Libraries Used
### 6.2 Language-Specific Notes
### 6.3 Deployment Instructions
#### 6.3.1 Server Deployment
#### 6.3.2 Client Execution

## 7 Testing and Validation
## 8 Conclusion


The BattleShipProtocol is an application-layer communication protocol specifically designed for real-time, turn-based multiplayer games. It enables interaction between clients and a central game server, facilitating the synchronization of state, player actions, and game events in a networked version of the classic Battleship game. The protocol is built over TCP/IP using the Berkeley Sockets API, ensuring reliable message delivery and concurrency support for multiple simultaneous game sessions.

BattleShipProtocol is a text-based protocol that supports a structured and extensible set of message types and rules for game control. It allows for clear separation between user input and game logic, empowering independent development of client and server components. Key features of the protocol include message typing, data validation, session management, and game state broadcasting to ensure fairness and consistency between players.

BattleShipProtocol is currently implemented in C++ for the server and compatible with client applications developed in any language supporting the Berkeley Sockets interface.



## 2. Notational Conventions and Generic Grammar
### 2.1 Backus-Naur Form (BNF)
The following BNF (Backus-Naur Form) defines the formal grammar for the communication protocol used in the client-server interaction of the Battleship game. This protocol is designed to ensure strict syntactic and semantic consistency across all message exchanges, enabling reliable parsing, validation, and processing of game-related instructions and states.

The BNF grammar below specifies all valid message types, their corresponding payloads, and the permissible structure and constraints of each component, including nicknames, coordinates, board states, and error reporting formats.

    <message> ::= <message-type> "|" <message-data>
    <message-type> ::= "REGISTER" | "PLACE_SHIPS" | "SHOOT" | "STATUS" | "SURRENDER" | "GAME_OVER" | "ERROR" 
    <message-data> ::= <register-data> | <place-ships-data> | <shoot-data> | <status-data> | <surrender-data> | <game-over-data> | <error-data> 
    <register-data> ::= <nickname> "," <email> 
    <nickname> ::= <string> 
    <email> ::= <string> "@" <string> "." <string> 
    <place-ships-data> ::= <ship-list> 
    <ship-list> ::= <ship> | <ship> ";" <ship-list> 
    <ship> ::= <ship-type> ":" <coordinates> 
    <ship-type> ::= "PORTAAVIONES" | "BUQUE" | "CRUCERO" | "DESTRUCTOR" | "SUBMARINO" 
    <coordinates> ::= <coord>  | <coord> "," <coordinates> ","  
    <coord> ::= <letter><number> 
    <letter> ::= "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" | "J" 
    <number> ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" | "10" 
    <shoot-data> ::= <coord>
    <status-data> ::= <turn> ";" <board-own> ";" <board-opponent> ";" <game-state> 
    <turn> ::= "YOUR_TURN" | "OPPONENT_TURN" 
    <board-own> ::= <cell-list> 
    <board-opponent> ::= <cell-list> 
    <cell-list> ::= <cell> | <cell> "," <cell-list> 
    <cell> ::= <coord> ":" <cell-state> 
    <cell-state> ::= "WATER" | "HIT" | "SUNK" | "SHIP" 
    <game-state> ::= "ONGOING" | "WAITING" | "ENDED" 
    <surrender-data> ::= "" 
    <game-over-data> ::= <winner> 
    <winner> ::= <nickname> | "NONE" 
    <error-data> ::= <error-code> "," <error-description> 
    <error-code> ::= <digit><digit><digit>
    <digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" 
    <error-description> ::= <string> 
    <string> ::= <char> | <char><string> 
    <char> ::= <letter> | <digit> | "_" | "-" | "."

### 2.2 Protocol Message Examples

## 3. System Diagrams
### 3.1 UML Class Diagram
