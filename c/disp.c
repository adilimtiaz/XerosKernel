/* disp.c : dispatcher
*/

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>

static pcb      *head = NULL;
static pcb      *tail = NULL;

extern void printPCB(char *name, pcb *p) {
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

void     dispatch( void ) {
    /********************************/

    pcb            *p;
    int            r;
    funcptr        fp;
    int            stack;
    va_list        ap;
    // TODO: 100 is random
    char           putResult[100];
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
                ready( p );
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
                sprintf(putResult, "%s\n", (va_arg(ap, char*)));
                kprintf(putResult);
                break;
            case(SYS_KILL):
                ap = (va_list) p->args;
                pid = (PID_t) va_arg(ap, int);

                p->ret = kill(pid);
                if(p->ret != 0){
                    // Kill failed
                    break;
                }
                if (p->pid == pid) {
                    // If current process switch to next process
                    p = next();
                } else {
                    pcb* node = head;
                    pcb* prevNode = NULL;
                    while(node){
                        if(node->pid == pid){
                           if(prevNode) {
                              prevNode->next = node->next;
                              if(!prevNode->next){
                                  // Tail case
                                  tail = prevNode;
                              }
                           }
                           break;
                        }
                        prevNode = node;
                        node = node->next;
                    }
                    // printReadyQueue();
                }

                break;
            case(SYS_PRIORITY):
                ap = (va_list) p->args;
                priority = va_arg(ap, int);
                p->ret = setPriority(p, priority);
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

                int receiveResult = recv(p, from_pid, receiveNum);
                if (receiveResult == PCB_BLOCKED) {
                    // Sender is not ready so
                    // block receiver and remove it from ready queue
                    p = next();
                } else {
                    p->ret = receiveResult;
                }
                break;
            case(SYS_TIMER):
                ready( p );
                p = next();
                end_of_intr();
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

extern void     ready( pcb *p ) {
    /*******************************/

    p->next = NULL;
    p->state = STATE_READY;

    if( tail ) {
        tail->next = p;
    } else {
        head = p;
    }

    tail = p;
}

extern pcb      *next( void ) {
    /*****************************/

    pcb *p;

    p = head;

    if( p ) {
        head = p->next;
        if( !head ) {
            tail = NULL;
        }
    }

    return( p );
}

extern int kill(PID_t pid) {
    pcb *p = (pcb*) &proctab[pid % MAX_PROC];

    // If pid not found
    if (p->state == STATE_STOPPED) {
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
    pcb* tempNext = node;
    // Add all item in send / receive queue back to ready queue
    int i = 0;
    while (tempNext) {
        node = tempNext;
        // Need tempNext as ready() clears p->next
        node->ret = -1;
        tempNext = node->next;
        ready(node);
        i++;
    }
    // printReadyQueue();
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
