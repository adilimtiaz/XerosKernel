/* user.c : User processes
*/

#include <xeroskernel.h>

// /* Your code goes here */
// void producer( void ) {
//     /****************************/
//
//     int         i;
//
//     for( i = 0; i < 5; i++ ) {
//         kprintf( "Produce %d\n", i );
//         // kprintf("<<< producer PID: %u\n", sysgetpid()); // TEST: 3.1
//         // syssetprio(i); // TEST 3.1
//         sysyield();
//     }
//
//
//     return;
// }
//
// void consumer( void ) {
//     /****************************/
//
//     int         i;
//
//     for( i = 0; i < 5; i++ ) {
//         kprintf( "Consume %d \n", i );
//         // syskill(3); // TEST: 3.1
//         sysyield();
//     }
//
//     return;
// }
//
// void     root( void ) {
//     /****************************/
//     PID_t proc_pid, con_pid;
//
//     kprintf("Root has been called\n");
//
//     sysyield();
//     sysyield();
//     proc_pid = syscreate( &producer, 4096 );
//     con_pid =  syscreate( &consumer, 4096 );
//
//     kprintf("Proc pid = %u Con pid = %u\n", proc_pid, con_pid);
//
//     // sysputs("  << Print from sysputs"); // TEST 3.1
//     // syssetprio(10); // TEST 3.1
//
//     for( ;; ) {
//         sysyield();
//     }
// }

// ==============================
// Processes for testing send / recv

void sender(void) {
    /****************************/
    PID_t rcv_pid = 3;
   // kprintf("Stopping send process");
   // sysstop();
    unsigned int sendpid = sysgetpid();
    unsigned int send = syssend(rcv_pid, sendpid);

    sysputs("Should not print");
    kprintf(" Syssend ret: %d", send);

    return;
}

void receiver(void) {
    /****************************/
    PID_t send_pid = 0;
    unsigned int recvInt = 5;

    // From pid is greater than maxAddr
    unsigned  int rcv = sysrecv(&send_pid, &recvInt);
    kprintf(" Sysrcv ret: %x \n", rcv);
    kprintf("Returned from sysrecv: %d\n", recvInt);
    int killResult = syskill((PID_t) recvInt);
    kprintf("Kill result: %d", killResult);

    return;
}

void receiver2(void){
    kprintf("<< in receiver 2\n");

    PID_t send_pid = 4;
    unsigned int recvInt = 5;

    // Num is in hole
    unsigned  int rcv = sysrecv(&send_pid, (unsigned int*) (640*1025));
    kprintf(" Sysrcv ret 2: %x \n", rcv);
    kprintf("Returned from sysrecv 2: %d\n", recvInt);

}


// NOTE: root() for testing send / recv
void     root( void ) {
    /****************************/
    PID_t send_pid, recv_pid;

    kprintf("Root has been called\n");

    send_pid = syscreate( &sender, 4096 );    // 2
    recv_pid =  syscreate( &receiver, 4096 ); // 3
    kprintf("Send pid = %u recv_pid pid = %u\n", send_pid, recv_pid);

    // sysputs("  << Print from sysputs"); // TEST 3.1
    // syssetprio(10); // TEST 3.1

    for( ;; ) {
        sysyield();
    }
}
