/* create.c : create a process
 */

#include <xeroskernel.h>
#include <xeroslib.h>

pcb     proctab[MAX_PROC];

/* make sure interrupts are armed later on in the kernel development  */
#define STARTING_EFLAGS         0x00003200

// PIDs can't start at 0 nor can they be negative
static PID_t      nextpid = 1;



int create( funcptr fp, size_t stackSize ) {
/***********************************************/

    context_frame       *cf;
    pcb                 *p = NULL;
    int                 i;


    /* If the PID becomes 0 it  has wrapped.
     * This means that the next PID we handout could be
     * in use. To find such a free number we have to propose a
     * new PID and then scan to see if it is in the table. If it
     * is then we have to try again.
     */


    if (nextpid == 0)
      return CREATE_FAILURE;

    // If the stack is too small make it larger
    if( stackSize < PROC_STACK ) {
        stackSize = PROC_STACK;
    }

    for( i = 0; i < MAX_PROC; i++ ) {
        if( proctab[i].state == STATE_STOPPED ) {
            p = &proctab[i];
            break;
        }
    }

    //    Some stuff to help wih debugging
    //    char buf[100];
    //    sprintf(buf, "Slot %d empty\n", i);
    //    kprintf(buf);
    //    kprintf("Slot %d empty\n", i);

    if( !p ) {
        return CREATE_FAILURE;
    }


    cf = kmalloc( stackSize );
    if( !cf ) {
        return CREATE_FAILURE;
    }

    cf = (context_frame *)((unsigned char *)cf + stackSize - 4);
    *(unsigned long*) cf = (unsigned long) sysstop;
    cf--;

    cf->edi = 0;
    cf->esi = 0;
    cf->ebp = 0;
    cf->ebx = 0;
    cf->edx = 0;
    cf->ecx = 0;
    cf->eax = 0;
    cf->iret_cs = getCS();
    cf->iret_eip = (unsigned int)fp;
    cf->eflags = STARTING_EFLAGS;

    cf->esp = (int)(cf + 1);
    cf->ebp = cf->esp;
    p->esp = (unsigned long*)cf;
    p->state = STATE_READY;
    p->pid = nextpid++;
    // Default priority level
    p->prio = 3;

    ready( p );
    return p->pid;
}
