/* disp.c : dispatcher
*/

#include <xeroskernel.h>
#include <xeroslib.h>
#include <i386.h>
#include <stdarg.h>

static pcb      *head = NULL;
static pcb      *tail = NULL;

static pcb *prio3QHead = NULL;
static pcb *prio3QTail = NULL;
static pcb *prio2QHead = NULL;
static pcb *prio2QTail = NULL;
static pcb *prio1QHead = NULL;
static pcb *prio1QTail = NULL;
static pcb *prio0QHead = NULL;
static pcb *prio0QTail = NULL;

void     dispatch( void ) {
    /********************************/

    pcb            *p;
    int            r;
    funcptr        fp;
    int            stack;
    va_list        ap;
    // TODO: 100 is random
    char           *putResult;
    PID_t          pid;
    int            priority;
    unsigned int   dest_pid;
    unsigned int   *from_pid;
    unsigned int   sendNum;
    unsigned int   *receiveNum;

    for( p = next(); p; ) {
        //      kprintf("Process %x selected stck %x\n", p, p->esp);

        r = contextswitch( p );
        switch( r ) {
            case(SYS_CREATE):
                ap = (va_list)p->args;
                fp = (funcptr)(va_arg( ap, int ) );
                stack = va_arg( ap, int );
                p->ret = create( fp, stack );
                break;
            case(SYS_YIELD):
                // Even if p was promoted before, when yield was called
                // it means it ran. Hence, put it back to its intended queue
                ready(p, p->prio);
                p = next();
                break;
            case(SYS_STOP):
                cleanup(p);
                p = next();
                break;
            case(SYS_GET_PID):
                p->ret = p->pid;
                break;
            case(SYS_PUTS):
                ap = (va_list) p->args;
                putResult = (va_arg(ap, char*));
                kprintf(putResult);
                break;
            case(SYS_KILL):
                ap = (va_list) p->args;
                pid = (PID_t) va_arg(ap, int);

                if (p->pid == pid) {
                    cleanup(p);
                    p = next();
                    break;
                }

                p->ret = kill(pid);
                break;
            case(SYS_PRIORITY):
                ap = (va_list) p->args;
                priority = va_arg(ap, int);
                p->ret = setPriority(p, priority);
                break;
            case(SYS_TIMER):
                // If p is already highest priority
                if (p->prio == 0) {
                    ready(p, p->prio);
                } else {
                    // Raise priority
                    ready(p, p->prio - 1);
                }
                p = next();
                end_of_intr();
                break;
            case(SYS_SEND):
                ap = (va_list) p->args;
                dest_pid = va_arg(ap, unsigned int);
                sendNum = va_arg(ap, unsigned long);
                int sendResult = send(p, dest_pid, sendNum);
                if (sendResult == PCB_BLOCKED) {
                    // Receiver is not ready so
                    // block sender and remove it from ready queue
                    p = next();
                } else {
                    p->ret = sendResult;
                }
                break;
            case(SYS_RECEIVE):
                ap = (va_list) p->args;
                from_pid = va_arg(ap, unsigned int*);
                receiveNum = va_arg(ap, unsigned int*);
                /* kprintf("   from_pid: %d \n", *from_pid); */
                /* kprintf("   receiveNum: %d \n", *receiveNum); */

                int receiveResult = recv(p, from_pid, receiveNum);
                /* kprintf("   receiveResult: %d \n", receiveResult); */
                /* kprintf("   <<<< returned from recv\n"); */
                if (receiveResult == PCB_BLOCKED) {
                    // Sender is not ready so
                    // block receiver and remove it from ready queue
                    p = next();
                } else {
                    p->ret = receiveResult;
                }
                break;
            default:
                kprintf( "Bad Sys request %d, pid = %u\n", r, p->pid );
        }
    }

    kprintf( "Out of processes: dying\n" );

    for( ;; );
}

extern void dispatchinit( void ) {
    /********************************/

    //bzero( proctab, sizeof( pcb ) * MAX_PROC );
    memset(proctab, 0, sizeof( pcb ) * MAX_PROC);
}

extern void setPrioQueue(pcb ***targetHead, pcb ***targetTail, int prio) {
  switch (prio) {
    case 3:
      *targetHead  = &prio3QHead;
      *targetTail  = &prio3QTail;
      break;
    case 2:
      *targetHead  = &prio2QHead;
      *targetTail  = &prio2QTail;
      break;
    case 1:
      *targetHead  = &prio1QHead;
      *targetTail  = &prio1QTail;
      break;
    default:
      *targetHead  = &prio0QHead;
      *targetTail  = &prio0QTail;
      break;
  }
}

// Find non-empty Q from the higheset to lowest priority
extern void findPrioQueue(pcb ***targetHead, pcb ***targetTail) {
    if (prio0QHead) {
        /* kprintf(" < prio0QHead Found\n"); */
        *targetHead  = &prio0QHead;
        *targetTail  = &prio0QTail;
    } else if (prio1QHead) {
        /* kprintf(" < prio1QHead Found\n"); */
        *targetHead  = &prio1QHead;
        *targetTail  = &prio1QTail;
    } else if (prio2QHead) {
        /* kprintf(" < prio2QHead Found\n"); */
        *targetHead  = &prio2QHead;
        *targetTail  = &prio2QTail;
    } else {
        /* kprintf(" < prio3QHead Found\n"); */
        *targetHead  = &prio3QHead;
        *targetTail  = &prio3QTail;
    }
}

extern void     ready( pcb *p, int prio ) {
    /*******************************/

    p->next = NULL;
    p->state = STATE_READY;

    pcb **targetHead;
    pcb **targetTail;
    setPrioQueue(&targetHead, &targetTail, prio);

    if(*targetTail) {
        (*targetTail)->next = p;
    } else {
        *targetHead = p;
    }

    *targetTail = p;
}

extern pcb      *next( void ) {
    /*****************************/

    pcb *p;
    pcb **targetHead;
    pcb **targetTail;
    findPrioQueue(&targetHead, &targetTail);

    p = *targetHead;

    if( p ) {
        *targetHead = p->next;
        if( !*targetHead ) {
            *targetTail = NULL;
        }
    }

    return( p );
}

extern int kill(PID_t pid) {
    pcb *p = NULL;

    for (int i = 0; i < MAX_PROC; i++) {
        if (proctab[i].pid == pid) {
            p = &proctab[i];
        }
    }

    // If pid not found
    if (p == NULL) {
        return -1;
    }

    cleanup(p);
    return 0;
}

extern void cleanup(pcb *p) {
    if (p->sender) {
        terminateQueue(p, p->sender);
    }

    if (p->receiver) {
        terminateQueue(p, p->receiver);
    }

    p->state = STATE_STOPPED;
    kfree(p->esp);
}

extern void terminateQueue(pcb *p, pcb *queueHead) {
    pcb* node = queueHead;
    // Add all item in send / receive queue back to ready queue
    while (node) {
        // Need tempNext as ready() clears p->next
        node->ret = -1;
        ready(node, node->prio);
        node = node->next;
    }
    printReadyQueue();
}

extern int setPriority(pcb* p, int priority) {
    int currPrio;

    // Invalid priority level
    if (priority < -1 || priority > 3) {
        // kprintf("  INVALID PRIO LEVEL %u\n", priority); // TEST: 3.1
        return -1;
    }

    currPrio = p->prio;

    if (priority == -1) {
        return currPrio;
    }

    // kprintf("  Old prio: %u, New priority: %u\n", currPrio, priority); // TEST: 3.1
    p->prio = priority;
    return currPrio;
}

extern void printReadyQueue(void){
    pcb* node = head;
    int i = 0;
    while(node){
        kprintf("Ready %d: %x \n", i , node->pid);
        node = node->next;
        i++;
    }
}
