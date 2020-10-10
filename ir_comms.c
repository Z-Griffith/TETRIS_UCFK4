/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   10 Oct 2020
*/

#include <stdlib.h>
#include "ir_comms.h"
#include "ir_uart.h"


static char irLastMessageRecieved = NO_MESSAGE;
static char irSendQueue[IR_SEND_QUEUE_MAX];
static int irTick = 0;

// Pop oldest message from queue
char irQueuePop(void)
{
    ir_state_t front = irSendQueue[0];
    for (int i = 1; i < IR_SEND_QUEUE_MAX; i++) {
        irSendQueue[i-1] = irSendQueue[i];
    }
    irSendQueue[IR_SEND_QUEUE_MAX-1] = NO_MESSAGE;
    return front;
}

// Pop and send the first item in the queue
void irSend(void)
{
    char sendingChar = irQueuePop();
    ir_uart_putc(sendingChar);
}

// Add new message to queue
void irQueueAdd(ir_state_t messageConfig, char data)
{
    for (int i = 0; i < IR_SEND_QUEUE_MAX-1; i++) {
        if (irSendQueue[i] == NO_MESSAGE) {
            irSendQueue[i] = messageConfig;
            irSendQueue[i+1] = data;
        }
    }
}

