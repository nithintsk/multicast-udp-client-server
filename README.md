# Description
This project consists of the implementation of a robust TicTacToe game client that communicates over UDP. The client is capable of functioning over a bad network. The client will be able to detect duplicate packets as well as malformed packets and will also be able to reissue dropped packets. In the case of a dropped connection with a server, the client will send out a multicast request to look for any available servers and resume the game state with the first available server.

# Design
Packet Format
| 1Byte          | 1Byte        | 1Byte         | 1Byte         | 1Byte       | 1Byte       |
|----------------|--------------|---------------|---------------|-------------|-------------|
| uint32         | uint32       | uint32        | uint32        | uint32      | uint32      |
| version number | command code | response code | move position | turn number | game number |

Board Representation
| | | |
|-|-|-|
| 1 | 2 | 3 |
| 4 | 5 | 6 |
| 7 | 8 | 9 |

Command Codes
+ 0: Move Command (from client/server) .Send with first move (from server)
+ 1: New Game Request (from client to server)

Response Codes (Version 3)
+ 0: OK
+ 1: Invalid Move - The requested move cannot be performed given the current board configuration
+ 2: Game out of sync - The turn number received is invalid (out of sync)
+ 3: Invalid Request - Any other invalid request
+ 4: Game over - Sent along with game ending (win/tie) move
+ 5: Game over Acknowledged - Sent as acknowledgment of Game Over
+ 6: Incompatible Version Number - The version number received is incompatible with the local version
+ 7: Server is busy - The server cannot accept a new game right now
+ 8: Game Number Mismatch - The game number received is not valid


# Usage
- Navigate to the directory where tictactoeClient.c, utility.c, utility.h and the makefile exist
- Run the make command and then the executable as shown below
- Client is statically set to bind to port 5001
```
term2$ make
term1$ ./tictactoeServer <port> <botlevel>
term2$ ./tictactoeClient <port> <server-ip>
```

# ScreenShots
1. Progression of game on the client

![Client Screenshot](client_screenshot.jpg?raw=true "Client Screenshot")

1. Progression of game on the Server

![Server Screenshot](server_screenshot.jpg?raw=true "Server Screenshot")
