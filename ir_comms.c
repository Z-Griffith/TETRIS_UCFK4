/** @file   ir_comms.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/


#include <stdlib.h>
#include "ir_uart.h"
#include "ir_comms.h"

static irHandler_t irHandler;

// Remove oldest message from queue   TODO: DEPRECATED
char irQueuePop(void)
{
    char message = irHandler.queue[0];
    // Shuffle down
    for (int i = 1; i < IR_QUEUE_MAX; i++) {
        irHandler.queue[i-1] = irHandler.queue[i];
    }
    // Fill last spot with NO_MESSAGE
    irHandler.queue[IR_QUEUE_MAX-1] = NO_MESSAGE;
    
    if (irHandler.nextQueueIndex > 0) {
        irHandler.nextQueueIndex--;
    }
    
    return message;
}

// Recieve an IR message
void irRecieve(void)
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

// Send a IR message
void irSend(char messageToSend) {
    while (!ir_uart_write_ready_p ()) {
        continue;
    }
    
    irHandler.lastMessageSent = messageToSend;
    ir_uart_putc(messageToSend);
    irHandler.isConfirmationRecieved = false;
}

// Send confirmation messages to other board
void irSendConfirmation(void) {
    for (int i = 0; i < 3; i++) { // Send 3 confirms
        ir_uart_putc(CONFIRM);
    }
}

// Is there a new message ready
bool irHasNewMessage(void) {
    return (irHandler.lastMessageRecieved != NO_MESSAGE 
                && irHandler.isConfirmationSent);
}

// Retrieve new message
char irGetMessage(void) {
    char newMessage = irHandler.lastMessageRecieved;
    bool hasNewMessage = (newMessage != NO_MESSAGE 
                && irHandler.isConfirmationSent);
    
    //if (hasNewMessage) {
        // Clear the last message to prevent doubleups
        //irHandler.lastMessageRecieved = NO_MESSAGE; // -------------- TODO: Fix this?
    //} 
    
    return newMessage;
}

// Checks whether a CONFIRM has been sent back
void irRecieveConfirmation(void)
{
    if (ir_uart_read_ready_p()) {
        // Recieve the (hopefully) confirmation message
        char newMessage = ir_uart_getc();
        if (newMessage == CONFIRM) {
            // The last message has been confirmed
            
            irHandler.isConfirmationRecieved = true;
            irHandler.watchdogTick = 0;
        }
    }
}

// Add new char message to queue for sending
void irQueuePush(char message)
{
    if (irHandler.nextQueueIndex < IR_QUEUE_MAX) {
        irHandler.queue[irHandler.nextQueueIndex] = message;
        irHandler.nextQueueIndex++;
    } // else ignore it...
}

// Defines the irTask machine, run through each frame
void irTask(void)
{
    irHandler.sendTick++;
    
    bool isSending = (irHandler.sendTick > irHandler.loopRate / irHandler.irRate);
    bool hasMessageToBeSent = irHandler.nextQueueIndex > 0;
    
    if (irHandler.isConfirmationRecieved == false) {
        // The last message send is yet to be confirmed
        irHandler.watchdogTick++;
        if (irHandler.watchdogTick > IR_WATCHDOG_MAX) {
            // Exeeded timer, resend last message
            irHandler.watchdogTick = 0;
            irHandler.nResendAttempts++;
            irSend(irHandler.lastMessageSent);
            
            if (irHandler.nResendAttempts > IR_RESEND_ATTEMPTS) {
                // TODO: Could wait a random amount of time and try again?
                // Exeeded max resend attempts, give up
                irHandler.nResendAttempts = 0;
                irHandler.isConfirmationRecieved = true;
            }
            
        } else {
            // Check for confirmation response
            irRecieveConfirmation();
        }
        
    } else if (irHandler.isConfirmationSent == false) {
        // We need to send a confirmation message immediately
        irHandler.isConfirmationSent = true;
        irSendConfirmation();
        
    } else if (isSending && hasMessageToBeSent) {
        // There's a message waiting, SEND IT
        irHandler.sendTick = 0; // Reset sending tick
        char messageToSend = irQueuePop();
        irSend(messageToSend);
            
    } else {
        // Nothing else to do, wait for a message
        irRecieve();
    }
}

// Initialises the irHandler to defaults
void irInit(int loopRate, int irRate)
{
    for (int i = 0; i < IR_QUEUE_MAX; i++) {
        irHandler.queue[i] = NO_MESSAGE;
    }
    irHandler.lastMessageSent = NO_MESSAGE;
    irHandler.lastMessageRecieved = NO_MESSAGE;
    irHandler.isConfirmationRecieved = false;
    irHandler.isConfirmationSent = false;
    irHandler.nResendAttempts = 0;
    irHandler.sendTick = 0;
    irHandler.watchdogTick = 0;
    irHandler.nextQueueIndex = 0;
    irHandler.loopRate = loopRate;
    irHandler.irRate = irRate;
}

// Checks whether confirmation was recieved for last message
bool irConfirmMessageRecieved(char sent_message) 
{
    return irHandler.isConfirmationRecieved && irHandler.lastMessageSent == sent_message;
}

// Returns last message sent over IR
char irGetLastMessageSent(void)
{
    return irHandler.lastMessageSent;
}

// Sends missile targetting coordinates to other board
void irSendMissile(char encodedCoords) {
    irQueuePush(encodedCoords);
}

// Sends a request to the other board requesting player 1
void irRequestPlayerOne(void) {
    irQueuePush(REQUEST_PLAYER_ONE);
}

// Sends the other board a notification saying last missile hit
void irSendHit(void) {
    irQueuePush(SEND_HIT);
}

// Sends the other board a notification saying last missile missed
void irSendMiss(void) {
    irQueuePush(SEND_MISS);
}



