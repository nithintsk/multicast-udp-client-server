/* include files go here */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // Defines sockaddr_in structure
#include <arpa/inet.h> // Defines htons and inet_addr

#include "utility.h"

int FLAG = 0;
int GAME_NUMBER = 0;
struct timeval TV;

int tictactoe(char board[ROWS][COLUMNS],
            BYTE* buffer,
            BYTE* prev_recv_buffer,
            BYTE* prev_sent_buffer,
            int sd,
            struct sockaddr_in fromAddress
            )
{
    /* this is the meat of the game, you'll look here for how to change it up */
    int turn = 0; // keep track of turn number
    int winner;
    int i = -1, choice;  // Keep track of candidate's move choice
    char mark; // either an 'x' or an 'o'
    int goAck = 0;
    struct sockaddr_in fromAddress1;
    memset(&fromAddress1, 0, sizeof(fromAddress1));

    /* First print the board, then ask player 'n' to make a move */
    do {
        //system("clear");
        print_board(board);                                                     // Call function to print the board on the screen
        mark = IS_EVEN(turn) ? 'X' : 'O';                                       // Depending on who the player is, either use x or o
        if (IS_ODD(turn)) {                                                    // Even number = Player is 1 -> Send to client
            do {
                choice = getChoice();                                           // Get the move from Player
            } while(makeMove(board, mark, &i, &winner, buffer, turn, choice, 1) != 0); // makeMove returns 0 if move is valid and breaks while loop
            if(i == 0) winner = 0; else winner = 1;                                // -----> why did I add this line? 
            turn++;                                                             // Increase turn number by 1
            sendToClient(sd, buffer, prev_sent_buffer, fromAddress);                  // Write to the socket to send buffer value to Player 1
        } else {                                                                // Odd number = Player is 2 -> Receive from Player 2
            /* Receive from Server */
            if (turn != 0) {
                printf("Waiting for response from server....\n");
                do {
                    recvFromClient(sd, buffer, prev_recv_buffer, prev_sent_buffer, &fromAddress1, fromAddress);
                } while(fromAddress1.sin_addr.s_addr != fromAddress.sin_addr.s_addr || fromAddress1.sin_port != fromAddress.sin_port);
    
                //buffer[VERSION_NUMBER] = (BYTE)'@';
                //buffer[COMMAND_CODE] = (BYTE)'!';
                //buffer[RESPONSE_CODE] = (BYTE)GAME_OVER_ACK;
                //buffer[MOVE_NUMBER] = (BYTE)'^';
                //buffer[TURN_NUMBER_INDEX] = (BYTE)'7';
                 
                //print_bytes("Prev received buffer", prev_recv_buffer);
                //print_bytes("Buffer", buffer);
                
                checkIncompatibleVersion(turn, buffer, sd, fromAddress);
                checkInvalidCommandCode(turn, buffer, sd, fromAddress);
                checkInvalidGameNumber(turn, buffer, sd, fromAddress);
            }

            BYTE turn_recv = buffer[TURN_NUMBER_INDEX];
            /* Task - * Check all response codes*/
            switch(buffer[RESPONSE_CODE]) {
                case (BYTE)OK:  
                            // No errors
                            // If turn number received is invalid
                            if(turn_recv != (BYTE)(turn)) {
                                sendInvalidTurn(turn, turn_recv, board, buffer, sd, fromAddress);
                            } else { // Turn number is valid
                                choice = (int)buffer[MOVE_NUMBER]; // Move is recorded
                                if(makeMove(board, mark, &i, &winner, buffer, turn, choice, 0) == 0) {          // makeMove returns 0 if move is valid
                                    // If the move is valid and Player 2 won, but response code was not 4
                                    if (i != -1) { // Indicates that the game is over.
                                        turn++;
                                        gameOverNotSent(turn, board, buffer, sd, fromAddress);                 // Sending invalid request and Exiting
                                    } else {
                                        turn++;
                                        break;
                                    }
                                } else { // Send invalid Move
                                    turn++;
                                    sendInvalidMove(turn, board, buffer, sd, fromAddress);
                                }  
                            }
                            break;
                case (BYTE)INVALID_MOVE:  
                            printf("Server responded with Move %d: Invalid Move\n", buffer[MOVE_NUMBER] & 0xff);
                            printf("Shutting Down connection with server...\n");
                            close(sd);
                            exit(1);
                case (BYTE)OUT_OF_SYNC:  
                            printf("Server responded with response code %d: Turn Number Invalid\n", buffer[RESPONSE_CODE] & 0xff);
                            printf("The turn number sent to player 2 was %d.\n", turn);
                            printf("The turn number received from server was %d.\n", buffer[TURN_NUMBER_INDEX] & 0xff);
                            printf("Shutting Down connection with server...\n");
                            close(sd);
                            exit(1);
                case (BYTE)INVALID_REQUEST:  // Invalid request
                            printf("Server responded with response code %d: Invalid request\n", buffer[RESPONSE_CODE] & 0xff);
                            printf("Shutting Down connection with server...\n");
                            close(sd);
                            exit(1);
                case (BYTE)GAME_OVER:  
                            // Game over
                            // Check turn number
                            // If turn number is valid, read move
                            // If turn number is invalid send back 
                            // update board
                            // check win status
                            // if there is a mismatch, send back invalid request
                            // if valid, send back game over ack
                            // No errors
                            if(turn_recv != (BYTE)(turn)) {
                                sendInvalidTurn(turn, turn_recv, board, buffer, sd, fromAddress);
                            } else {
                                choice = (int)buffer[MOVE_NUMBER]; // Move is recorded
                                if(makeMove(board, mark, &i, &winner, buffer, turn, choice, 0) == 0) { // makeMove returns 0 if move is valid
                                    if (i != -1) { // Indicates that the game is over.
                                        turn++;
                                        fillBuffer(buffer, VERSION, MOVE_COMMAND, GAME_OVER_ACK, choice, turn); // Send game over ack if there is a clear winner
                                        if(i == 0) winner = 0; else winner = 2;
                                        goAck = 1;
                                        printf("Sending game over ack to Server.\n");
                                        sendToClient(sd, buffer, prev_sent_buffer, fromAddress);
                                    } else {
                                        turn++;
                                        printf("Server sent Game Over. Server's move does not end the game.\n");
                                        printf("This is an invalid request to end the game.\n");
                                        fillBuffer(buffer, VERSION, MOVE_COMMAND, INVALID_REQUEST, choice, turn); // Send Invalid request if there is a disagreement over winner
                                        sendToClient(sd, buffer, prev_sent_buffer, fromAddress);
                                        printf("Shutting Down connection with server...\n");
                                        close(sd);
                                        exit(1);
                                    }
                                } else { // Send invalid Move
                                    turn++;
                                    sendInvalidMove(turn, board, buffer, sd, fromAddress);
                                }  
                            }
                            break;
                case (BYTE)GAME_OVER_ACK:  
                            printf("Server responded with response code %d: Game Over Acknowledged\n", buffer[RESPONSE_CODE] & 0xff);
                            goAck = 1;
                            if(i==-1) {
                                printf("Game state is not over yet. Closing connection.\n");
                                close(sd);
                                exit(1);
                            }
                            break;
                case (BYTE)INCOMPATIBLE_VERSION_NUMBER:  
                            printf("Server responded with response code %d: Incompatible version number.\n", buffer[RESPONSE_CODE] & 0xff);
                            printf("Version number sent by server was: %d.\n", buffer[VERSION_NUMBER] & 0xff);
                            printf("Shutting Down connection with server...\n\n");
                            close(sd);
                            exit(1);
                case (BYTE)SERVER_BUSY:  
                            printf("Server responded with response code %d: SERVER_BUSY.\n", buffer[RESPONSE_CODE] & 0xff);
                            printf("Shutting Down connection with server...\n\n");
                            close(sd);
                            exit(1);
                case (BYTE)GAME_MISMATCH:  
                            printf("Server responded with response code %d: GAME_MISMATCH.\n", buffer[RESPONSE_CODE] & 0xff);
                            printf("Shutting Down connection with server...\n\n");
                            close(sd);
                            exit(1);
                default:
                            printf("Invalid Response code Received: %d.\n", buffer[RESPONSE_CODE]);
                            turn++;
                            sendInvalidRequest(turn, board, buffer, sd, fromAddress);
            }
        }
    } while (i == -1 || goAck == 0); // -1 means no one won
    
    /* print out the board again */
    print_board(board);
    
    if (winner == 1) {// means player 1 won!! congratulate them
        printf("\033[0;32m");
        printf("==>\aYou win\n\n");
        printf("\033[0m");
    }
    else if (winner == 2) {// means player 2 won!! congratulate them
        printf("\033[0;31m");
        printf("==>\aYou lost\n\n");
        printf("\033[0m");
    }
    else
        printf("==>\aGame draw\n\n"); // ran out of squares, it is a draw

    return 0;
}

void print_bytes(char *s, BYTE* buffer) {
    /*printf("\n%s:\n"  \
    "VERSION_NUMBER: %x\n"      \
    "COMMAND_CODE: %x\n"        \
    "RESPONSE_CODE: %x\n"       \
    "MOVE_NUMBER: %x\n"         \
    "TURN_NUMBER: %x\n"         \
    "GAME_NUMBER: %x\n",        \
    */
    printf("\033[0;32m");
    printf("%s: "  \
    "VERS: %x | "      \
    "COMM: %x | "        \
    "RESP: %x | "       \
    "MOVE: %x | "         \
    "TURN: %x | "         \
    "GAME: %x\n",        \
    s,                          \
    buffer[VERSION_NUMBER],     \
    buffer[COMMAND_CODE],       \
    buffer[RESPONSE_CODE],      \
    buffer[MOVE_NUMBER],        \
    buffer[TURN_NUMBER_INDEX],  \
    buffer[GAME_NUMBER_INDEX]);
    printf("\033[0m");
}

void sendInvalidMove(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    printf("Server responded with move %d which is deemed invalid\n", buffer[MOVE_NUMBER] & 0xff);
    fillBuffer(buffer, VERSION, MOVE_COMMAND, INVALID_MOVE, 0, turn);
    sendToClient(sd, buffer, buffer, fromAddress);            // Write to the socket to send buffer value to Player 1
    // Closing down a connection
    // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
    printf("Shutting Down connection with server...\n\n");
    close(sd);
    exit(1);
}

void gameOverNotSent(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    printf("Server responded with move %d and response code %d.\n", buffer[MOVE_NUMBER] & 0xff, buffer[RESPONSE_CODE] & 0xff);
    printf("Server should have instead sent Game Over response code: 4.\n");
    printf("Board status after Server's move is: \n");
    print_board(board);
    turn++;
    fillBuffer(buffer, VERSION, MOVE_COMMAND, INVALID_REQUEST, 0, turn);
    sendToClient(sd, buffer, buffer, fromAddress);            // Write to the socket to send buffer value to Player 1
    printf("Sent Invalid Request Response code \"4\" to Server.\n");
    // Closing down a connection
    // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
    printf("Shutting Down connection with server...\n\n");
    close(sd);
    exit(1);
}

void sendInvalidRequest(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    /*printf("Received bytes are:\n       \
                VERSION_NUMBER: %x\n        \
                COMMAND_CODE: %x\n          \
                RESPONSE_CODE: %x\n         \
                MOVE_NUMBER: %x\n           \
                TURN_NUMBER: %x\n           \
                GAME_NUMBER: %x\n",         \
                buffer[VERSION_NUMBER],     \
                buffer[COMMAND_CODE],       \
                buffer[RESPONSE_CODE],      \
                buffer[MOVE_NUMBER],        \
                buffer[TURN_NUMBER_INDEX],  \
                buffer[GAME_NUMBER_INDEX]);
    */
    turn++;
    printf("There is a mismatch in the received bytes and the expected bytes.\n");
    fillBuffer(buffer, VERSION, MOVE_COMMAND, INVALID_REQUEST, 0, turn);
    sendToClient(sd, buffer, buffer, fromAddress); // Write to the socket to send buffer value to Player 1
    printf("Sent Invalid Request Response code \"%d\" to Server\n", INVALID_REQUEST);
    // Closing down a connection
    // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
    printf("Shutting Down connection with server...\n\n");
    close(sd);
    exit(1);
}

void checkIncompatibleVersion(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    if(buffer[VERSION_NUMBER] != VERSION) {
        printf("Server responded with version %d. The current version number is %d.\n", buffer[VERSION_NUMBER] & 0xff, VERSION);
        turn++;
        fillBuffer(buffer, VERSION, MOVE_COMMAND, INCOMPATIBLE_VERSION_NUMBER, 0, turn);
        sendToClient(sd, buffer, buffer, fromAddress);            // Write to the socket to send buffer value to Player 1
        printf("Sent incompatible version response code to server.\n");
        // Closing down a connection
        // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
        printf("Terminated connection with server...\n\n");
        close(sd);
        exit(1);
    }
}

void checkInvalidGameNumber(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    if(buffer[GAME_NUMBER_INDEX] != GAME_NUMBER) {
        printf("Server responded with game number %d. The expected game number is %d.\n", buffer[GAME_NUMBER_INDEX] & 0xff, GAME_NUMBER);
        turn++;
        fillBuffer(buffer, VERSION, MOVE_COMMAND, GAME_MISMATCH, 0, turn);
        sendToClient(sd, buffer, buffer, fromAddress);            // Write to the socket to send buffer value to Player 1
        printf("Sent game number mismatch response code to server.\n");
        // Closing down a connection
        // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
        printf("Terminated connection with server...\n\n");
        close(sd);
        exit(1);
    }
}


void checkInvalidCommandCode(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    if(buffer[COMMAND_CODE] != MOVE_COMMAND) {
        turn++;
        printf("Server responded with command %d. The command number required to make move is %d.\n", buffer[COMMAND_CODE] & 0xff, MOVE_COMMAND);
        fillBuffer(buffer, VERSION, MOVE_COMMAND, INVALID_REQUEST, 0, turn);
        sendToClient(sd, buffer, buffer, fromAddress);            // Write to the socket to send buffer value to Player 1
        printf("Sent Invalid Request Response code \"4\" to Server\n");
        // Closing down a connection
        // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
        printf("Terminated connection with server...\n\n");
        close(sd);
        exit(1);
    }
}

void sendInvalidTurn(int turn, int turn_recv, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress) {
    printf("Encountered Error: invalid turn number.\n");
    printf("The turn number sent to server was %d.\n", turn-1);
    printf("The turn number received from server was %d.\n", turn_recv & 0xff);
    fillBuffer(buffer, VERSION, MOVE_COMMAND, OUT_OF_SYNC, 0, turn);                      // Response code is 2: Out of Sync
    turn++;                                                 // Increase turn number by 1
    sendToClient(sd, buffer, buffer, fromAddress);
    printf("Sent Game out of sync Response code \"%d\" to Server\n", OUT_OF_SYNC);
    // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
    printf("Terminated connection with server...\n\n");
    close(sd);
    exit(1);
}

int makeMove(char board[ROWS][COLUMNS], int mark, int *i, int *winner, BYTE *buffer, int turn, int choice, int update) {
    int row, column;
    /******************************************************************/
    /** little math here. you know the squares are numbered 1-9, but  */
    /* the program is using 3 rows and 3 columns. We have to do some  */
    /* simple math to conver a 1-9 to the right row/column            */
    /******************************************************************/
    row = (int)((choice - 1) / ROWS);
    column = (choice - 1) % COLUMNS;

    if (board[row][column] == (choice + '0')) {
        board[row][column] = mark;
        /* after a move, check to see if someone won! (or if there is a draw */
        *i = checkwin(board);
        if(*i != -1) {                                                 // If true, Game is over
            update ? fillBuffer(buffer, VERSION, MOVE_COMMAND, GAME_OVER, choice, turn) : 0;                   // Response code 4: Game Over
        } else {
            update ? fillBuffer(buffer, VERSION, MOVE_COMMAND, OK, choice, turn) : 0;                   // Response code 0: no errors
        }
        //printf("Sending bytes: %d:%d:%d:%d\n", buffer[VERSION_NUMBER], buffer[RESPONSE_CODE], buffer[MOVE_NUMBER], buffer[TURN_NUMBER_INDEX]);
        return 0;                                      // Break out of infinite while loop

    } else {
        //printf("Turn value is %d.\n", turn);
        if(IS_EVEN(turn)) {
            return 1;
        }
        else {
            printf("Invalid move! Reenter a valid Move.\n");
            fflush(stdin);
            return 1;
        }
    }
}

void fillBuffer(BYTE *buf, int c0, int c1, int c2, int c3, int c4) {
    
    buf[VERSION_NUMBER] = (BYTE)c0;
    buf[COMMAND_CODE] = (BYTE)c1;
    buf[RESPONSE_CODE] = (BYTE)c2;
    buf[MOVE_NUMBER] = (BYTE)c3;
    buf[TURN_NUMBER_INDEX] = (BYTE)c4;
    buf[GAME_NUMBER_INDEX] = GAME_NUMBER;
    
    /* Debugging info */
    //buf[VERSION_NUMBER] = (BYTE)2;
    //buf[COMMAND_CODE] = (BYTE)'@';
    //buf[RESPONSE_CODE] = (BYTE)'F';
    //buf[MOVE_NUMBER] = (BYTE)'%';
    //buf[TURN_NUMBER_INDEX] = (BYTE)0;
    //buf[GAME_NUMBER_INDEX] = (BYTE)5;
}

void recvFromClient(int sd, BYTE* buffer, BYTE* prev_recv_buffer, BYTE* prev_sent_buffer, struct sockaddr_in *fromAddress, struct sockaddr_in toAddress) {
    int rc;
    int retries = RETRIES;
    socklen_t from_len;
    from_len = sizeof(*fromAddress);
    do {
        // Set socket options and check for success
        if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &TV, sizeof(TV))) {
            functionError(-1, "setsockopt");
        }
        
        // Receive on the socket
        memset(buffer, 0, BUFFERLEN);
        rc = recvfrom(sd, buffer, BUFFERLEN, MSG_WAITALL, (struct sockaddr *) fromAddress, &from_len);
        // If nothing is received after timeout, retry
        if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            printf("\033[1;31m");
            printf("Receive timed out. resending...\n");
            printf("\033[0m");
            retries--;
            sendToClient(sd, prev_sent_buffer, prev_sent_buffer, toAddress);
            continue;
        }
        // Else check for failure
        else connectError(rc, sd);
        
        print_bytes("\t--->Received", buffer);
        
        // If received message == previously received message, resend last sent message
        if (!memcmp(buffer, prev_recv_buffer, BUFFERLEN)) { // They we have received a duplicate message
            printf("\033[1;31m");
            printf("Duplicate message. Resending...\n");
            printf("\033[0m");
            sendToClient(sd, prev_sent_buffer, prev_sent_buffer, toAddress);
            retries = RETRIES;
            continue;
        } 
        else break;
    } while (retries);

    // If retries == 0, close and exit
    if (!retries) {
        printf("\033[1;31m");
        printf("Retried receive %d times but to no avail. Closing and exiting...\n", RETRIES);
        printf("\033[0m");
        close(sd);
        exit(1);
    }

    memcpy(prev_recv_buffer, buffer, BUFFERLEN); 
}

void sendToClient(int sd, BYTE* buffer, BYTE* prev_sent_buffer, struct sockaddr_in fromAddress) {
    int rc;
    if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &TV, sizeof(TV))) {
        functionError(-1, "setsockopt");
    }

    int resp = buffer[RESPONSE_CODE];

    // Can have a non-blocking read loop here to clear the socket of duplicates
    rc = sendto(sd, buffer, BUFFERLEN, MSG_CONFIRM, (const struct sockaddr *) &fromAddress, sizeof(fromAddress));
    //print_bytes("Prev sent", prev_sent_buffer);
    print_bytes("\t--->Sent", buffer);    
    connectError(rc, sd);
    memcpy(prev_sent_buffer, buffer, BUFFERLEN); 
    
    if (resp == OK || resp == GAME_OVER || resp == SERVER_BUSY) {
        return;
    } else {
        struct timeval TV1 = {20, 0};
        if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &TV1, sizeof(TV1))) {
            functionError(-1, "setsockopt");
        }
        struct sockaddr_in recv_address;
        socklen_t recv_len;
        recv_len = sizeof(recv_address);
        BYTE* buffer1 = (BYTE*)malloc(sizeof(BYTE)*BUFFERLEN);
        do {
            printf("\033[1;34m");
            printf("FIN_WAIT state entered. Polling on receive buffer with a timeout of 20 seconds\n");
            printf("\033[0m");
            rc = recvfrom(sd, buffer1, BUFFERLEN, MSG_WAITALL, (struct sockaddr *) &recv_address, &recv_len);
            // If nothing is received after timeout, retry
            if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                printf("\033[1;34m");
                printf("Timeout expired. FIN_WAIT completed. Exiting...\n");
                printf("\033[0m");
                return;
            }
            // Else check for failure
            else connectError(rc, sd);
            
            printf("\033[1;31m");
            printf("Duplicate message. Resending...\n");
            printf("\033[0m");
            rc = sendto(sd, buffer, BUFFERLEN, MSG_CONFIRM, (const struct sockaddr *) &fromAddress, sizeof(fromAddress));
            print_bytes("\t--->Sent", buffer);    
            connectError(rc, sd);
        }while(1);
    }
}

int getChoice() {
    int ch;
    do{ 
        printf("Enter your move to send (1-9): ");
        scanf("%d", &ch);                       // using scanf to get the choice
        if (ch < 1 || ch > 9) {
            printf("\033[1;31m");
            printf("Error!!!! The value entered should be between 1 and 9. Renter...\n");
            printf("\033[0m");
        }
        getchar();
    } while(ch < 1 || ch > 9);
    return ch;
}

void initSharedState(char board[ROWS][COLUMNS])
{
    /* this just initializing the shared state aka the board */
    int i, j, count = 1;
    printf("Initializing the board and setting up the game.\n");
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
        {
            board[i][j] = count + '0';
            count++;
        }
}

void print_board(char board[ROWS][COLUMNS])
{
    /*****************************************************************/
    /* brute force print out the board and all the squares/values    */
    /*****************************************************************/

    printf("\n\n\n\tCurrent TicTacToe Game\n\n");

    printf("Player 1 (X)  -  Player 2 (O)\n\n\n");

    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);

    printf("     |     |     \n\n");
}

int checkwin(char board[ROWS][COLUMNS])
{
    /************************************************************************/
    /* brute force check to see if someone won, or if there is a draw       */
    /* return a 0 if the game is 'over' and return -1 if game should go on  */
    /************************************************************************/
    if (board[0][0] == board[0][1] && board[0][1] == board[0][2]) // row matches
        return 1;

    else if (board[1][0] == board[1][1] && board[1][1] == board[1][2]) // row matches
        return 1;

    else if (board[2][0] == board[2][1] && board[2][1] == board[2][2]) // row matches
        return 1;

    else if (board[0][0] == board[1][0] && board[1][0] == board[2][0]) // column
        return 1;

    else if (board[0][1] == board[1][1] && board[1][1] == board[2][1]) // column
        return 1;

    else if (board[0][2] == board[1][2] && board[1][2] == board[2][2]) // column
        return 1;

    else if (board[0][0] == board[1][1] && board[1][1] == board[2][2]) // diagonal
        return 1;

    else if (board[2][0] == board[1][1] && board[1][1] == board[0][2]) // diagonal
        return 1;

    else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
             board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' &&
             board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')

        return 0; // Return of 0 means game over
    else
        return -1; // return of -1 means keep playing
}

void genericError(char *s) {
    perror(s);
    printf("Usage is: ./tictactoeOriginal <port number> <server ip>\n");
    printf("Can use only ports between 1024 and 65535.");
    printf("Exiting...\n");
    exit(1);
}

void functionError(int ret, char *s) {
    if(ret < 0) {
        fprintf(stderr, "Error calling %s() command: %s\n", s, strerror(errno));
        printf("Usage is: ./tictactoeOriginal <port number> <server ip>\n");
        printf("Exiting...\n");
        exit(1);
    }
}

int connectError(int rc, int sd) {
    if (rc < 0) {
        perror("Connection Error Encountered");
        printf("Terminating Connection with server...\n");
        // Close the connection and exit.
        close(sd);
        exit(1);
    }
    else if (rc < BUFFERLEN) {
        printf("Received/Sent less than %d Bytes.\n", BUFFERLEN);
        printf("Terminating Connection with server...\n");
        close(sd);
        exit(1);
    }
    return 0;
}
