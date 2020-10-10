/** @file   ir_comms.h
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/

#ifndef IR_COMMS_H
#define IR_COMMS_H

#include <stdbool.h>

// Checks for confirm N times before resend
#define IR_WATCHDOG_MAX 10 

// Maximum times to resend an unconfirmed message before giving up
#define IR_RESEND_ATTEMPTS 50 


// Enumerates important message codes
typedef enum ir_state {
    NO_MESSAGE = 100,
    CONFIRM = 101,
    REQUEST_PLAYER_ONE = 102,
    SEND_HIT = 104,
    SEND_MISS = 105,
    MESSAGE_SENT = 106,
    MESSAGE_READ = 107
} ir_state_t;

// Defines the IR handler struct
typedef struct ir_handler_t {
    char messageToSend;
    char lastMessageSent;
    char lastMessageRecieved;
    bool hasNewMessage;
    bool wasLastSentConfirmed;
    bool isConfirmationSent;
    int nResendAttempts;
    int sendTick;
    int watchdogTick;
    int loopRate;
    int irRate;
} irHandler_t;


// Recieve an IR message
void irRecieveMessage(void);

// -----------------------------
void irMarkMessageAsRead(void);

// Send a IR message
void irSendMessage(char);


// Send confirmation messages to other board
void irSendConfirmationMessage(void);


// Is there a new message ready
bool irHasNewMessage(void);


// Checks whether a CONFIRM has been sent back
void irCheckForConfirmationMessage(void);


// Defines the irTask machine, run through each frame
void irTask(void);


// Initialises the irHandler to defaults
void irInit(int, int);

// Checks whether confirmation was recieved for last message
bool irWasSentMessageReceived(char);


// Retrieve new message
char irGetLastMessageRecieved(void);


// Returns last message sent over IR
char irGetLastMessageSent(void);

// Sends missile targetting coordinates to other board
void irSendMissile(char);


// Sends a request to the other board requesting player 1
void irRequestPlayerOne(void);


// Sends the other board a notification saying last missile hit
void irSendHit(void);


// Sends the other board a notification saying last missile missed
void irSendMiss(void);

#endif
