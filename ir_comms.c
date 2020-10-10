/** @file   ir_comms.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/


#include <stdlib.h>
#include "ir_uart.h"
#include "ir_comms.h"

static irHandler_t irHandler;


// Recieve an IR message
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

// Send a IR message
void irSendMessage(char message) {
    while (!ir_uart_write_ready_p ()) {
        continue;
    }
    
    irHandler.lastMessageSent = message;
    ir_uart_putc(message);
    irHandler.messageToSend = MESSAGE_SENT;
    irHandler.wasLastSentConfirmed = false;
}

// Send confirmation messages to other board
void irSendConfirmationMessage(void) {
    for (int i = 0; i < 3; i++) { // Send 3 confirms
        ir_uart_putc(CONFIRM);
    }
}


// Checks whether a CONFIRM has been sent back
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

// Defines the irTask machine, run through each frame
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

// Initialises the irHandler to defaults
void irInit(int loopRate, int irRate)
{
    irHandler.messageToSend = NO_MESSAGE;
    irHandler.lastMessageSent = NO_MESSAGE;
    irHandler.lastMessageRecieved = NO_MESSAGE;
    irHandler.wasLastSentConfirmed = false;
    irHandler.isConfirmationSent = false;
    irHandler.nResendAttempts = 0;
    irHandler.sendTick = 0;
    irHandler.watchdogTick = 0;
    irHandler.loopRate = loopRate;
    irHandler.irRate = irRate;
}

// Checks whether confirmation was recieved for last message
bool irWasSentMessageReceived(char sent_message) 
{
    return (irHandler.wasLastSentConfirmed && irHandler.lastMessageSent == sent_message); // TODO: Conflicting
}

// Retrieve new message
char irGetMessage(void) {
    char newMessage = irHandler.lastMessageRecieved;
    bool hasNewMessage = (newMessage != NO_MESSAGE  // TODO: Check
               && irHandler.isConfirmationSent);
    
    if (hasNewMessage) {
        return newMessage;
    } else {
        return NO_MESSAGE;
    }
}



// Returns last message sent over IR
char irGetLastMessageSent(void)
{
    return irHandler.lastMessageSent;
}

// Sends missile targetting coordinates to other board
void irSendMissile(char encodedCoords) 
{
    irHandler.messageToSend = encodedCoords;
}

// Sends a request to the other board requesting player 1
void irRequestPlayerOne(void) 
{
    irHandler.messageToSend = REQUEST_PLAYER_ONE;
}

// Sends the other board a notification saying last missile hit
void irSendHit(void) 
{
    irHandler.messageToSend = SEND_HIT;
}

// Sends the other board a notification saying last missile missed
void irSendMiss(void) 
{
    irHandler.messageToSend = SEND_MISS;
}



