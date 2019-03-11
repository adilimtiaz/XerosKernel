/* user.c : User processes
*/

#include <xeroskernel.h>
#include <xeroslib.h>

// ========================
// Initial produce / consumer
//
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
// 3.5.1 Tests

// void prioThreeA( void ) {
//     /****************************/
//
//     int         i;
//     char putsStr[10];
//
//     for( i = 0; i < 2; i++ ) {
//         sprintf(putsStr, "A %d\n", i);
//         sysputs(putsStr);
//     }
//
//     return;
// }
//
// void prioThreeB( void ) {
//     /****************************/
//
//     int         i;
//     char putsStr[10];
//
//     for( i = 0; i < 2; i++ ) {
//         sprintf(putsStr, "B %d\n", i);
//         sysputs(putsStr);
//     }
//
//     return;
// }
//
// void prioThreeC( void ) {
//     /****************************/
//
//     int         i;
//     char putsStr[10];
//
//     for( i = 0; i < 2; i++ ) {
//         sprintf(putsStr, "C %d\n", i);
//         sysputs(putsStr);
//     }
//
//     return;
// }
//
// void root( void ) {
//     /****************************/
//     char putsStr[50];
//     PID_t A_pid, B_pid, C_pid;
//     sysputs("Root has been called\n");
//
//     // Test yield to cover empty queue case
//     sysyield();
//
//     // NOTE: See create.c for other cases
//     // CASE 1: A -> B -> C
//     A_pid = syscreate( &prioThreeA, 4096 );
//     B_pid = syscreate( &prioThreeB, 4096 );
//     C_pid = syscreate( &prioThreeC, 4096 );
//
//     sprintf(putsStr, "A pid = %u B pid = %u C pid = %u\n",
//                         A_pid, B_pid, C_pid);
//     sysputs(putsStr);
//
//     for( ;; ) {
//         // Works regardless of this yield
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
    kprintf(" Syssend ret: %d ", send);

    return;
}

void receiver(void) {
    /****************************/
    PID_t send_pid = 0;
    unsigned int recvInt = 5;

    // From pid is greater than maxAddr
    unsigned  int rcv = sysrecv(&send_pid, &recvInt);
    kprintf(" Sysrcv ret: %x \n ", rcv);
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
    kprintf(" Sysrcv ret 2: %x \n ", rcv);
    kprintf("Returned from sysrecv 2: %d\n", recvInt);
}

// NOTE: root() for testing send / recv
void     root( void  ) {
    /****************************/
    PID_t send_pid, recv_pid;

    kprintf("Root has been called\n");

    send_pid = syscreate( &sender, 4096  );    // 2
    recv_pid =  syscreate( &receiver, 4096  ); // 3
    kprintf("Send pid = %u recv_pid pid = %u\n", send_pid, recv_pid);

    // sysputs("  << Print from sysputs "); // TEST 3.1
    // syssetprio(10); // TEST 3.1

    for( ;;  ) {
        sysyield();
    }
}
