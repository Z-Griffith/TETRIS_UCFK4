/** @file   ir_comms.h
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/

#ifndef IR_COMMS_H
#define IR_COMMS_H
#include <stdbool.h>

/** 
 * @brief Check for a CONFIRM message this many IR ticks before resending message.
 */
#define IR_WATCHDOG_MAX 5


/** 
 * @brief Maximum times to resend an unresponded message before giving up.
 */
#define IR_RESEND_ATTEMPTS 50 


/** 
 * @brief Defines the actual values of flag messages send over IR.
 */
typedef enum ir_state {
    NO_MESSAGE = 100,
    CONFIRM = 101,
    REQUEST_PLAYER_ONE = 102,
    SEND_HIT = 104,
    SEND_MISS = 105,
    MESSAGE_SENT = 106,
} ir_state_t;


/** 
 * @brief Declares the IR handler struct which carries all information
 * for the IR send and recieve logic.
 */
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


/** 
 * @brief Recieves an IR message (char) and stores it.
 */
void irRecieveMessage(void);


/** 
 * @brief Sends an IR message (char).
 * @param message: Message to be sent.
 */
void irSendMessage(char);


/** 
 * @brief Sends a CONFIRM message to confirm a message was recieved.
 */
void irSendConfirmationMessage(void);


/** 
 * @brief Recieves a (hopefully) CONFIRM IR message as a response to 
 * the last message sent.
 */
void irCheckForConfirmationMessage(void);


/** 
 * @brief IR task machine. Runs IR logic and must be called each frame.
 */
void irTask(void);


/** 
 * @brief Initialises the IR handler and it's parameters.
 * @param loopRate: loop speed of the main program.
 * @param irRate: Rate at which IR logic is polled.
 */
void irInit(int, int);


/** 
 * @brief Checks whether the last sent message was recieved properly.
 * @param message: The message we want to check.
 * @return bool: Was it recieved properly?
 */
bool irWasSentMessageReceived(char);


/** 
 * @brief IR utility function for getting the last message recieved.
 * @return char: Last message recieved.
 */
char irGetLastMessageRecieved(void);


/** 
 * @brief IR utility function. Mark last recieved message as read to prevent double-checking
 */
void irMarkMessageAsRead(void);


/** 
 * @brief IR utility function. Returns last message sent over IR.
 * @return char: last message sent.
 */
char irGetLastMessageSent(void);


/** 
 * @brief Sends encoded coordinates of missile impact to other board.
 * @param char of encoded coordinates.
 */
void irSendMissile(char);


/** 
 * @brief Sends a request to the other board to make the sending board Player 1
 */
void irRequestPlayerOne(void);


/** 
 * @brief Sends the other board a notification saying last missile hit
 */
void irSendHit(void);


/** 
 * @brief Sends the other board a notification saying last missile missed
 */
void irSendMiss(void);

#endif
