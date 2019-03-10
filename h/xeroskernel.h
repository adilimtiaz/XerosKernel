/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout Xinu */

typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes
                              * addressable in this architecture.
                              */
#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */

#define CREATE_FAILURE -1  /* Process creation failed     */



/* Universal return constants */

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);


/* Some constants involved with process creation and managment */

   /* Maximum number of processes */
#define MAX_PROC        64
   /* Kernel trap number          */
#define KERNEL_INT      80
   /* Minimum size of a stack when a process is created */
#define PROC_STACK      (4096 * 4)


/* Constants to track states that a process is in */
#define STATE_STOPPED   0
#define STATE_READY     1
#define STATE_RECEIVING 2
#define STATE_SENDING   3

// For syssend
#define PCB_BLOCKED 20

/* System call identifiers */
#define SYS_STOP        10
#define SYS_YIELD       11
#define SYS_CREATE      22
#define SYS_TIMER       33
#define SYS_GET_PID     34
#define SYS_PUTS        35
#define SYS_KILL        36
#define SYS_PRIORITY    37
#define SYS_SEND        38
#define SYS_RECEIVE     39


/* Structure to track the information associated with a single process */

typedef unsigned int  PID_t;
typedef struct struct_pcb pcb;
struct struct_pcb {
  unsigned long  *esp;    /* Pointer to top of saved stack           */
  pcb            *next;   /* Next process in the list, if applicable */
  // pcb            *prev;   /* MAYBE??? */
  int             state;  /* State the process is in, see above      */
  PID_t           pid;    /* The process's ID                        */
  int             ret;    /* Return value of system call             */
                          /* if process interrupted because of system*/
                          /* call                                    */
  int             prio;   // Priority
  long            args;
  pcb             *sender;        // Queue of msg sender
  pcb             *receiver;      // Queue of msg sender
  unsigned long   buf;            // Buffer pointer for send / recv
  unsigned int    *receiveAddr;   // num used in recv()
  unsigned int    *from_pid;      // form_pid used in recv()
};


/* The actual space is set aside in create.c */
extern pcb     proctab[MAX_PROC];

#pragma pack(1)

/* What the set of pushed registers looks like on the stack */
typedef struct context_frame {
  unsigned long        edi;
  unsigned long        esi;
  unsigned long        ebp;
  unsigned long        esp;
  unsigned long        ebx;
  unsigned long        edx;
  unsigned long        ecx;
  unsigned long        eax;
  unsigned long        iret_eip;
  unsigned long        iret_cs;
  unsigned long        eflags;
  unsigned long        stackSlots[];
} context_frame;


/* Memory mangement system functions, it is OK for user level   */
/* processes to call these.                                     */

int      kfree(void *ptr);
void     kmeminit( void );
void     *kmalloc( size_t );


/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);


/* Internal functions for the kernel, applications must never  */
/* call these.                                                 */
void     dispatch( void );
void     dispatchinit( void );
void     ready( pcb *p );
void     cleanup(pcb *p);
void     terminateQueue(pcb *p, pcb *head);
pcb      *next( void );
int      kill(PID_t pid);
int      setPriority(pcb* p, int priority);
int      send(pcb *p, unsigned int dest_pid, unsigned long num);
int      recv(pcb *p, unsigned int *from_pid, unsigned int * num);
void     contextinit( void );
int      contextswitch( pcb *p );
int      create( funcptr fp, size_t stack );
void     set_evec(unsigned int xnum, unsigned long handler);
void     printCF (void * stack);  /* print the call frame */
int      syscall(int call, ...);  /* Used in the system call stub */
void     addToQueue(pcb *p, pcb *queue);
void     removeFromQueue(pcb *target, pcb *prevSender);
int      isInvalidAddr(unsigned int *pidAddr);



/* Function prototypes for system calls as called by the application */
unsigned int          syscreate( funcptr fp, size_t stack );
void                  sysyield( void );
void                  sysstop( void );
PID_t                 sysgetpid(void);
void                  sysputs(char *str);
int                   syskill(PID_t pid);
int                   syssetprio(int priority);
int                   syssend(unsigned int dest_pid, unsigned long num);
int                   sysrecv(unsigned int *from_pid, unsigned int * num);

/* The initial process that the system creates and schedules */
void     root( void );




void           set_evec(unsigned int xnum, unsigned long handler);


/* Anything you add must be between the #define and this comment */
#endif

