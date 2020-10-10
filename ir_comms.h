/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   10 Oct 2020
*/

#ifndef IR_COMMS
#define IR_COMMS

#define IR_SEND_QUEUE_MAX 10
#define IR_LOOP_RATE 500

typedef enum ir_state {
    NO_MESSAGE,
    REQUEST_PLAYER_ONE,
    WAITING_FOR_RESPONSE,
    WAIT_FOR_CONFIRM,
    READY_TO_SEND,
    CONFIRM,
    SENDING_STATE,
    SENDING_COORDINATES,
    SEND_HIT,
    SEND_MISS
} ir_state_t;

static char irLastMessageRecieved;
static char irSendQueue[IR_SEND_QUEUE_MAX];
static int irTick;

// Pop oldest message from queue
char irQueuePop(void);

// Add new message to queue
void irQueueAdd(ir_state_t, char);

// Pop and send the first item in the queue
void irSend(void);
#endif
