/* msg.c : messaging system
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <i386.h>

pcb proctab[MAX_PROC];


// Add target to the queue safely
void addToQueue(pcb *p, pcb **queueHead) {
    pcb *nextItem;
    if (*queueHead == NULL) {
        // p becomes the queue head
        p->next = NULL;
        *queueHead = p;
        // Next is null because this a new queue.
        // queueHead->next = NULL;
    } else {
        // p becomes the tail of the queue
        nextItem = *queueHead;
        while (nextItem->next) {
            nextItem = nextItem->next;
        }
        nextItem->next = p;
    }
}

void printPCB(char *name, pcb *p) {
    if(!p){
        kprintf("Invalid PCB \n");
        return;
    }
    kprintf("   %s Pointer Val: %x \n", name, p);
    kprintf("   %s->esp: %x \n", name, p->esp);
    kprintf("   %s->next: %x \n", name, p->next);
    kprintf("   %s->state: %x \n", name, p->state);
    kprintf("   %s->pid: %x \n", name, p->pid);
    kprintf("   %s->ret: %x \n", name, p->ret);
    kprintf("   %s->prio: %x \n", name, p->prio);
    kprintf("   %s->args: %x \n", name, p->args);
    pcb *node = p->sender;
    int i = 0;
    while (node) {
        kprintf("   %s->sender %d: %x \n", name, i, node);
        node = node->next;
        i++;
    }
    node = p->receiver;
    i = 0;
    while (node) {
        kprintf("   %s->receiver %d: %x \n", name, i, node);
        node = node->next;
        i++;
    }
    kprintf("   %s->buf: %x \n", name, p->buf);
    kprintf("   %s->receiveAddr: %x \n", name, p->receiveAddr);
    kprintf("   %s->from_pid: %x \n", name, p->from_pid);
}

extern int send(pcb *p, unsigned int dest_pid, unsigned long num) {
    // If invalid param
    // TODO: Add other invalid cases
    if (num == 0) {
        return -100;
    }

    // If try to send to itself
    if (p->pid == dest_pid) {
        return -3;
    }

    // destPCB is rcving P's PCB
    pcb *destPCB = (pcb *) &proctab[dest_pid % MAX_PROC];

    // If process doesn't exist
    if (destPCB->state == STATE_STOPPED) {
        return -2;
    }

    // Check sending P rcvr queue for dest_pid
    pcb *isRcvrWaiting = p->receiver;

    while (isRcvrWaiting && isRcvrWaiting->pid != dest_pid) {
        isRcvrWaiting = isRcvrWaiting->next;
    }

    // Check if rcvr P has called rcvany
    if (*(destPCB->from_pid) == 0) {
        isRcvrWaiting = destPCB;
    }

    // If rcvr P is already waiting for send
    if (isRcvrWaiting) {
        if (*(destPCB->from_pid) != 0) {
            // Update rcvr queue for sending process if not receiveany
            p->receiver = p->receiver->next;
        } else {
            // Update from pid if reciveAny
            *(destPCB->from_pid) = p->pid;
        }
        // Chang rcv buffer
        *(destPCB->receiveAddr) = num;

        // Change rcvPCB rc to 0. Success.
        destPCB->ret = 0;
        // Add destPCB back to ready queue
        ready(destPCB);
        return 0;
    }

    // If target not in receive state
    p->state = STATE_SENDING;
    p->buf = num;
    // Add sending p to rcving prcoess senders
    addToQueue(p, (pcb **) &destPCB->sender);


    // Sending P not in return queue
    return PCB_BLOCKED;
}

extern int recv(pcb *p, unsigned int *from_pid, unsigned int *num) {
    // If invalid param
    // TODO: Add other invalid cases
    if (num == 0) {
        return -100;
    }

    // If receiveALL case
    if (*from_pid == 0) {
        if (isInvalidAddr(from_pid)) {
            return -5;
        }

        // Find the first sender in queue
        if (p->sender) {
            pcb* sendingPCB = p->sender;
            *num = sendingPCB->buf;
            // SEt retval to 0 because send completed succesfully
            sendingPCB->ret = 0;
            // Remove sending P from  rcving P senders
            p->sender = p->sender->next;

            // Unblock blocked sending PCB
            ready(sendingPCB);

            return 0;
        }
    } else {
        // If try to receive to itself
        if (p->pid == *from_pid) {
            return -3;
        }

        pcb *sendingPCB = &proctab[*from_pid % MAX_PROC];

        // If process doesn't exist
        // TODO: MAke a switch in case sending PCB was not inited yet, ret -100
        if (sendingPCB->state == STATE_STOPPED) {
            return -2;
        }

        // Check rcving P senders for from pid
        pcb *isSenderWaiting = p->sender;
        while (isSenderWaiting && isSenderWaiting->pid != *from_pid) {
            isSenderWaiting = isSenderWaiting->next;
        }

        // Sending P already sent to rcving P
        if (isSenderWaiting) {
            // Remove sending P from rcving p senders
            p->sender = p->sender->next;
            // Update num to sending buf
            *num = sendingPCB->buf;
            // Add blocked sender to ready queue
            ready(sendingPCB);
            printReadyQueue();
            return 0;
        } else {
            // Add rcving P to sending P's rcvr list
            addToQueue(p, (pcb**) &sendingPCB->receiver);
        }
    }

    // If sender not ready
    p->state = STATE_RECEIVING;
    p->receiveAddr = num;
    p->from_pid = from_pid;

    return PCB_BLOCKED;
}


// Remove target from queue safely
extern void removeFromQueue(pcb *target, pcb *prev) {
    if (prev) {
        if (target->next) {
            // If target is in middle of queue
            prev->next = target->next;
        } else {
            // If target is at end of queue
            prev->next = NULL;
        }
    }
}

extern int isInvalidAddr(unsigned int *pidAddr) {
    if (
        // In kernel
            (pidAddr > 0 && pidAddr < (unsigned int *) KERNEL_STACK) ||
            // In hole region
            (pidAddr > (unsigned int *) HOLESTART && pidAddr < (unsigned int *) HOLEEND)
            ) {
        return 1;
    } else {
        return 0;
    }
}
