#ifndef UTILITY
#define UTILITY

/* include files go here */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h> // Defines sockaddr_in structure
#include <arpa/inet.h> // Defines htons and inet_addr

/* #define section, for now we will define the number of rows and columns */
#define ROWS 3
#define COLUMNS 3
#define BUFFERLEN 6

// definition of special codes
#define VERSION 3
#define MOVE_COMMAND 0
#define NEW_GAME_REQUEST 1

// definition of response codes
#define OK 0
#define INVALID_MOVE 1
#define OUT_OF_SYNC 2
#define INVALID_REQUEST 3
#define GAME_OVER 4
#define GAME_OVER_ACK 5
#define INCOMPATIBLE_VERSION_NUMBER 6
#define SERVER_BUSY 7
#define GAME_MISMATCH 8

// Definition of the buffer indexes
#define VERSION_NUMBER 0
#define COMMAND_CODE 1
#define RESPONSE_CODE 2
#define MOVE_NUMBER 3
#define TURN_NUMBER_INDEX 4
#define GAME_NUMBER_INDEX 5

// Timeout values
#define TIMETOWAIT 3
#define RETRIES 10

// Static Ports
#define SRC_PORT 5001

#define IS_ODD(val) ((val % 2) ? 1 : 0) 
#define IS_EVEN(val) ((val % 2) ? 0 : 1)

typedef unsigned char BYTE;

extern int FLAG; // Value of 0 indicates no error. Value of 1 means disconnect from client and reinitialize.
extern int GAME_NUMBER;
extern struct timeval TV;

int tictactoe(char board[ROWS][COLUMNS], BYTE* buffer, BYTE* prev_recv_buffer, BYTE* prev_sent_buffer, int sd, struct sockaddr_in fromAddress);
void sendInvalidMove(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void gameOverNotSent(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void sendInvalidTurn(int turn, int turn_recv, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress);
int makeMove(char board[ROWS][COLUMNS], int mark, int *i, int *winner, BYTE *buffer, int turn, int choice, int update);
void fillBuffer(BYTE *buf, int c0, int c1, int c2, int c3, int c4);
int getChoice();
void genericError(char *s);
void functionError(int ret, char *s);
void print_board(char board[ROWS][COLUMNS]);
void initSharedState(char board[ROWS][COLUMNS]);
int connectError(int rc, int sd);
int checkwin(char board[ROWS][COLUMNS]);
void recvFromClient(int sd, BYTE* buffer, BYTE* prev_recv_buffer, BYTE* prev_sent_buffer, struct sockaddr_in *fromAddress, struct sockaddr_in toAddress);
void sendToClient(int sd, BYTE* buffer, BYTE* prev_sent_buffer, struct sockaddr_in fromAddress);
void sendInvalidRequest(int turn, char board[ROWS][COLUMNS], BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void checkIncompatibleVersion(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void checkInvalidCommandCode(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void checkInvalidGameNumber(int turn, BYTE *buffer, int sd, struct sockaddr_in fromAddress);
void print_bytes(char *s, BYTE* buffer);
#endif
