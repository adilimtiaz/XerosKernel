/* user.c : User processes
*/

#include <xeroskernel.h>

/* Your code goes here */
void producer( void ) {
    /****************************/

    int         i;

    for( i = 0; i < 5; i++ ) {
        kprintf( "Produce %d\n", i );
        // TEST: kprintf("<<< producer PID: %u\n", sysgetpid());
        sysyield();
    }


    sysstop();
}

void consumer( void ) {
    /****************************/

    int         i;

    for( i = 0; i < 5; i++ ) {
        kprintf( "Consume %d \n", i );
        sysyield();
    }

    sysstop();
}

void     root( void ) {
    /****************************/
    PID_t proc_pid, con_pid;

    kprintf("Root has been called\n");

    sysyield();
    sysyield();
    proc_pid = syscreate( &producer, 4096 );
    con_pid =  syscreate( &consumer, 4096 );

    kprintf("Proc pid = %u Con pid = %u\n", proc_pid, con_pid);

    for( ;; ) {
        sysyield();
    }
}
