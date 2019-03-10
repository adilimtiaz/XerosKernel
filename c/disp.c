/* disp.c : dispatcher
*/

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>


static pcb      *head = NULL;
static pcb      *tail = NULL;

void     dispatch( void ) {
    /********************************/

    pcb         *p;
    int         r;
    funcptr     fp;
    int         stack;
    va_list     ap;
    // TODO: 100 is random
    char         putResult[100];
    PID_t       pid;
    int         priority;

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
                p->state = STATE_STOPPED;
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

                // If pid is current process
                // TODO: is this proper way to terminate?
                if (p->pid == pid) {
                    p->state = STATE_STOPPED;
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

    p->state = STATE_STOPPED;
    // TODO: kfree here?
    return 0;
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
