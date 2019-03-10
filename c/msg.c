/* msg.c : messaging system
   This file does not need to modified until assignment 2
 */

#include <xeroskernel.h>
#include <i386.h>

pcb     *target;
pcb     *prevSender;
pcb     *prevReceiver;

pcb     proctab[MAX_PROC];

void printPCB(pcb *p) {
  kprintf("   p->esp: %x \n", p->esp);
  kprintf("   p->next: %x \n", p->next);
  kprintf("   p->state: %x \n", p->state);
  kprintf("   p->pid: %x \n", p->pid);
  kprintf("   p->ret: %x \n", p->ret);
  kprintf("   p->prio: %x \n", p->prio);
  kprintf("   p->args: %x \n", p->args);
  kprintf("   p->sender: %x \n", p->sender);
  kprintf("   p->receiver: %x \n", p->receiver);
  kprintf("   p->buf: %x \n", p->buf);
  kprintf("   p->receiveAddr: %x \n", p->receiveAddr);
  kprintf("   p->from_pid: %x \n", p->from_pid);
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

    *target = proctab[dest_pid % MAX_PROC];

    // If process doesn't exist
    if (target->state == STATE_STOPPED) {
        return -2;
    }

    // Update receive buffer and put it in ready queue
    target = p->receiver;
    prevReceiver = NULL;
    if (target) {
        while (target->pid != dest_pid) {
            prevReceiver = target;
            target = target->next;
        }
    }

    // If target is in receiver queue
    if (target) {
        removeFromQueue(target, prevReceiver);
        if (*(target->from_pid) == 0) {
            // If target is receiveAll, set from_pid to sender's pid
            *(target->from_pid) = p->pid;
        }
        *(target->receiveAddr) = num;
        target->ret = 0;
        ready(target);
        return 0;
    }

    // If target not in receive state
    p->state = STATE_SENDING;
    p->buf = num;
    addToQueue(p, target->sender);

    // syssend will remove send p from ready queue
    return PCB_BLOCKED;
}

extern int recv(pcb *p, unsigned int *from_pid, unsigned int * num) {
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
            *num = p->sender->buf;
            if (p->sender->next) {
                // Update the pointer to rest of the queue
                p->sender = p->sender->next;
            }
            return 0;
        }
    } else {
        // If try to receive to itself
        if (p->pid == *from_pid) {
            return -3;
        }

        *target = proctab[*from_pid % MAX_PROC];

        // If process doesn't exist
        if (target->state == STATE_STOPPED) {
            return -2;
        }

        // Find matching from_pid from queue
        target = p->sender;
        prevSender = NULL;
        if (target) {
            while (target->pid != *from_pid) {
                prevSender = target;
                target = target->next;
            }
        }

        // If matching pid sender exists
        if (target) {
            removeFromQueue(target, prevSender);
            *num = target->buf;
            ready(target);
            return 0;
        }

    }

    // If sender not ready
    p->state = STATE_RECEIVING;
    p->receiveAddr = num;
    p->from_pid = from_pid;
    addToQueue(p, target->receiver);
    return PCB_BLOCKED;
}

// Add target to the queue safely
extern void addToQueue(pcb *p, pcb *queueHead) {
    pcb *nextItem;

    if (queueHead == NULL) {
        // p becomes the queue head
        *queueHead = *p;
    } else {
        // p becomes the tail of the queue
        nextItem = queueHead;
        while (nextItem->next) {
            nextItem = nextItem->next;
        }
        nextItem->next = p;
    }
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
            (pidAddr > 0 && pidAddr < (unsigned int*) KERNEL_STACK) ||
            // In hole region
            (pidAddr > (unsigned int*) HOLESTART && pidAddr < (unsigned int*)HOLEEND)
       ) {
        return 1;
    } else {
        return 0;
    }
}
