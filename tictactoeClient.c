/**********************************************************/
/* This program is a 'pass and play' version of tictactoe */
/* Two users, player 1 and player 2, pass the game back   */
/* and forth, on a single computer                        */
/**********************************************************/

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

int main(int argc, char *argv[])
{
    /* Check number of arguments */
    if(argc != 3) {
        errno = EINVAL;
        genericError("Error Encountered");
    }

    /* Validate the port number */
    int port_number = (int)strtod(argv[1], NULL);
    if(port_number < 1024 || port_number > 65535) {
        errno = EINVAL;
        genericError("Port Number is Invalid. Enter a port between 1024 and 65535.");
    }

    /* Get the server address*/
    char serverIP[30], fromIP[30];
    strcpy(serverIP, argv[2]);

    /* Clear the screen and begin */
    //system("clear");

    /* Initialize and setup the board */
    char board[ROWS][COLUMNS];

    /* Declare/Initialize the connection variables */
    int sd;
    int rc;
    struct sockaddr_in server_address, src_address, from_address;
    BYTE *buffer = (BYTE *)malloc(BUFFERLEN * sizeof(BYTE)); // define bytes for buffer
    BYTE *prev_recv_buffer = (BYTE *)malloc(BUFFERLEN * sizeof(BYTE)); // define bytes for the previous buffer
    BYTE *prev_sent_buffer = (BYTE *)malloc(BUFFERLEN * sizeof(BYTE)); // define bytes for the previous buffer

    /* Set up the connection */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    functionError(sd, "socket");
    memset((char*) &src_address, 0, sizeof(src_address));
    src_address.sin_family = AF_INET;
    src_address.sin_addr.s_addr = htonl(INADDR_ANY);
    src_address.sin_port = htons(SRC_PORT);
    rc = bind(sd, (struct sockaddr *) &src_address, sizeof(src_address));
    functionError(rc, "bind");
    memset((char*) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    if(inet_aton(serverIP, &server_address.sin_addr) == 0) {
        rc = -1;
        errno=EINVAL;
        functionError(rc, "inet_aton");
    }
    
    /* Set timeout values */
    TV.tv_sec = TIMETOWAIT;
    TV.tv_usec = 0;

    /* Initialize the game board */
    initSharedState(board);
    printf("Completed initialization of game board.\n\n");
    printf("Sending request to server %s on port %d...\n\n", inet_ntoa(server_address.sin_addr), port_number);

    /* Fill buffer with new game request */
    fillBuffer(buffer, VERSION, NEW_GAME_REQUEST, OK, 0, 0);
    
    sendToClient(sd, buffer, prev_sent_buffer, server_address);

    printf("Waiting for response from server %s on port %d...\n\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
    
    recvFromClient(sd, buffer, prev_recv_buffer, prev_sent_buffer, &from_address, server_address);
    
    //printf("Received bytes are: %x:%x:%x:%x:%x%x\n", buffer[VERSION_NUMBER], buffer[COMMAND_CODE], buffer[RESPONSE_CODE], buffer[MOVE_NUMBER], buffer[TURN_NUMBER_INDEX], buffer[GAME_NUMBER_INDEX]);

    // Verify the version and the command code to start a new game
    if (buffer[COMMAND_CODE] == MOVE_COMMAND && 
            buffer[VERSION_NUMBER] == VERSION &&
            buffer[RESPONSE_CODE] != SERVER_BUSY &&
            from_address.sin_addr.s_addr == server_address.sin_addr.s_addr &&
            from_address.sin_port == server_address.sin_port) {
        GAME_NUMBER = buffer[GAME_NUMBER_INDEX];
        tictactoe(board, buffer, prev_recv_buffer, prev_sent_buffer, sd, server_address);
    }
    // Check if server is busy 
    else if (buffer[RESPONSE_CODE] == SERVER_BUSY) {
        printf("Server responded with response code %d. Server is busy.\n", buffer[RESPONSE_CODE]);
        printf("Client is terminating. Please try again...\n");
        close(sd);
        exit(1);
    }
    // Default behavior
    else {
        GAME_NUMBER = buffer[GAME_NUMBER_INDEX];
        printf("Version: %x and Command Code: %x was sent by the server.\n", buffer[VERSION_NUMBER], buffer[COMMAND_CODE]);
        printf("This client expects VERSION: %d and Command Code: %d to start a new game.\n", VERSION, MOVE_COMMAND);
        printf("Server address received was %s and port was %d.\n", 
        inet_ntop(AF_INET, &from_address.sin_addr,fromIP,sizeof(fromIP)),
                htons(from_address.sin_port));
        sendInvalidRequest(buffer[TURN_NUMBER_INDEX],board,buffer, sd, server_address);
    }
    return 0;
}








