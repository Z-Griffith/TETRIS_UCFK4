/**
    @file   ir_comms.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/
#include <stdlib.h>
#include "ir_uart.h"
#include "ir_comms.h"

/** 
 * @brief Defines the irHandler struct.
 */
static irHandler_t irHandler;


/** 
 * @brief Recieves an IR message (char) and stores it.
 */
void irRecieveMessage(void)
{
    if (ir_uart_read_ready_p ()) {
        // Recieve the new message
        char message = ir_uart_getc();
        if (!(message == CONFIRM)) {
            irHandler.lastMessageRecieved = message;
            irHandler.isConfirmationSent = false;
        } // else ignore the confirm and wait for resend
    }
}

/** 
 * @brief Sends an IR message (char).
 * @param message: Message to be sent.
 */
void irSendMessage(char message) {
    while (!ir_uart_write_ready_p ()) {
        continue;
    }
    
    irHandler.lastMessageSent = message;
    ir_uart_putc(message);
    irHandler.messageToSend = MESSAGE_SENT;
    irHandler.wasLastSentConfirmed = false;
}

/** 
 * @brief Sends a CONFIRM message to confirm a message was recieved.
 */
void irSendConfirmationMessage(void) {
    for (int i = 0; i < 3; i++) { // Send 3 confirms
        ir_uart_putc(CONFIRM);
    }
    
    irHandler.hasNewMessage = true;
}

/** 
 * @brief Recieves a (hopefully) CONFIRM IR message as a response to 
 * the last message sent.
 */
void irCheckForConfirmationMessage(void)
{
    if (ir_uart_read_ready_p()) {
        // Recieve the (hopefully) confirmation message
        char newMessage = ir_uart_getc();
        if (newMessage == CONFIRM) {
            // The last message has been confirmed
            
            irHandler.wasLastSentConfirmed = true;
            irHandler.watchdogTick = 0;
        }
    }
}

/** 
 * @brief IR task machine. Runs IR logic and must be called each frame.
 */
void irTask(void)
{
    irHandler.sendTick++;
    
    bool isSending = (irHandler.sendTick > irHandler.loopRate / irHandler.irRate);
    bool hasMessageToBeSent = irHandler.messageToSend != MESSAGE_SENT; // TODO: Make parameter .messageSent?
    
    if (irHandler.wasLastSentConfirmed == false) {
        // The last message send is yet to be confirmed
        irHandler.watchdogTick++;
        if (irHandler.watchdogTick > IR_WATCHDOG_MAX) {
            // Exeeded timer, resend last message
            irHandler.watchdogTick = 0;
            irHandler.nResendAttempts++;
            irSendMessage(irHandler.lastMessageSent);
            
            if (irHandler.nResendAttempts > IR_RESEND_ATTEMPTS) {
                // TODO: Could wait a random amount of time and try again?
                // Exeeded max resend attempts, give up
                irHandler.nResendAttempts = 0;
                irHandler.wasLastSentConfirmed = true;
            }
            
        } else {
            // Check for confirmation response
            irCheckForConfirmationMessage();
        }
        
    } else if (irHandler.isConfirmationSent == false) {
        // We need to send a confirmation message immediately
        irSendConfirmationMessage();
        irHandler.isConfirmationSent = true;
        
    } else if (isSending && hasMessageToBeSent) {
        // There's a message waiting, SEND IT
        irHandler.sendTick = 0; // Reset sending tick
        irSendMessage(irHandler.messageToSend);
            
    } else {
        // Nothing else to do, wait for a message
        irRecieveMessage();
    }
}

/** 
 * @brief Initialises the IR handler and it's parameters.
 * @param loopRate: loop speed of the main program.
 * @param irRate: Rate at which IR logic is polled.
 */
void irInit(int loopRate, int irRate)
{
    irHandler.messageToSend = NO_MESSAGE;
    irHandler.lastMessageSent = NO_MESSAGE;
    irHandler.lastMessageRecieved = NO_MESSAGE;
    irHandler.hasNewMessage = false;
    irHandler.wasLastSentConfirmed = false;
    irHandler.isConfirmationSent = false;
    irHandler.nResendAttempts = 0;
    irHandler.sendTick = 0;
    irHandler.watchdogTick = 0;
    irHandler.loopRate = loopRate;
    irHandler.irRate = irRate;
}

/** 
 * @brief Checks whether the last sent message was recieved properly.
 * @param message: The message we want to check.
 * @return bool: Was it recieved properly?
 */
bool irWasSentMessageReceived(char message) 
{
    return (irHandler.wasLastSentConfirmed && irHandler.lastMessageSent == message); // TODO: Conflicting
}

/** 
 * @brief IR utility function for getting the last message recieved.
 * @return char: Last message recieved.
 */
char irGetLastMessageRecieved(void) {
    char newMessage = irHandler.lastMessageRecieved;
    
    if (irHandler.hasNewMessage) {
        return newMessage;
    } else {
        return NO_MESSAGE;
    }
}

/** 
 * @brief IR utility function. Mark last recieved message as read to prevent double-checking
 */
void irMarkMessageAsRead(void) {
    irHandler.hasNewMessage = false;
}


/** 
 * @brief IR utility function. Returns last message sent over IR.
 * @return char: last message sent.
 */
char irGetLastMessageSent(void)
{
    return irHandler.lastMessageSent;
}

/** 
 * @brief Sends encoded coordinates of missile impact to other board.
 * @param char of encoded coordinates.
 */
void irSendMissile(char encodedCoords) 
{
    irHandler.messageToSend = encodedCoords;
}


/** 
 * @brief Sends a request to the other board to make the sending board Player 1
 */
void irRequestPlayerOne(void) 
{
    irHandler.messageToSend = REQUEST_PLAYER_ONE;
}


/** 
 * @brief Sends the other board a notification saying last missile hit
 */
void irSendHit(void) 
{
    irHandler.messageToSend = SEND_HIT;
}

/** 
 * @brief Sends the other board a notification saying last missile missed
 */
void irSendMiss(void) 
{
    irHandler.messageToSend = SEND_MISS;
}



