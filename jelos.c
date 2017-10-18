// JEL OS
// Fall 2016
// James E. Lumpp, Jr.
// 4/11/16

#include <stdlib.h>
#include <stdint.h>
#include "jelos.h"
#include <stdio.h>
#include <time.h>
#include "mylib.h"

static TaskControlBlock task_list[NUM_TASKS], *TASK_LIST_PTR;
static TaskControlBlock *CURRENT_TASK;
unsigned int *sem;
int i;
clock_t start, end;
double cpu_time_used;

static int NEXT_TID;
static unsigned char null_task_stack[60];  // is not used, null task uses original system stack
static void InitSystem(void);
static void NullTask(void);
void EdgeCounter_Init(void);
void SysTick_Init(void);
void OS_Sem_Init(unsigned int *sem, unsigned int count);
void OS_Sem_Signal(unsigned int *sem);
void OS_Sem_Wait(unsigned int *sem);
void OS_Suspend(void);

            /* Start the multi-tasking system */
int StartScheduler(void)
	{
	if (CURRENT_TASK == NULL)
		return -1;
	OS_Sem_Init(sem, 1);
	PortF_Init();
	SysTick_Init();
	EnableInterrupts();
	//EdgeCounter_Init();           // initialize GPIO Port F interrupt OR SysTick OR ...
	
  NullTask();                   // Will not return
	return 0;	 
	}

/*
	Create a new process and link it to the task list 
*/
int CreateTask(void (*func)(void), 
                    unsigned char *stack_start,
                    unsigned stack_size)
					//,unsigned ticks)
	{
//	long ints;
	TaskControlBlock *p, *next;

	if (TASK_LIST_PTR == 0)
		InitSystem();
	
//	ints=StartCritical();
	p = TASK_LIST_PTR;
	TASK_LIST_PTR = TASK_LIST_PTR->next;
	p->func = func;
	p->state = T_CREATED;
	p->tid = NEXT_TID++;

	       /* stack grows from high address to low address */
	p->stack_start = stack_start;
	p->stack_end = stack_start+stack_size-1;
	//p->stack_size = stack_size;
	
	p->sp = p->stack_end;
	p->clk_ticks = 0;

	           /* create a circular linked list */
	if (CURRENT_TASK == NULL)
		p->next = p, CURRENT_TASK = p;
	else
		next = CURRENT_TASK->next, CURRENT_TASK->next = p, p->next = next;
//  EndCritical(ints);
	return p->tid;
	}

/* Initialize the system.
 */
static void InitSystem(void)
	{
	int i;

	         /* initialize the free list  */
	for (i = 0; i < NUM_TASKS-1; i++)
		task_list[i].next = &task_list[i+1];
	TASK_LIST_PTR = &task_list[0];

	         /* null task has tid of 0 */
	CreateTask(NullTask, null_task_stack, sizeof(null_task_stack));
	}


/* Always runnable task. This has the tid of zero
 */
static void NullTask(void)
	{

	while (1) 
		;          //  putchar('n');
	 
	}


// Schedule will save the current SP and then call teh scheduler
//	SHOULD ONLY BE CALLED IN ISR
/* Schedule(): Run a different task. Set the current task as the next one in the (circular)
 * list, then set the global variables and call the appropriate asm routines
 * to do the job. 
 */
unsigned char * Schedule(unsigned char * the_sp)  
	{
		unsigned char * sp; // save the current sp and schedule
		
	 //CURRENT_TASK->clk_ticks = (ROM_SysTickValueGet();
	 CURRENT_TASK->sp = the_sp;
	 CURRENT_TASK->state = T_READY;
	 CURRENT_TASK = CURRENT_TASK->next;	 

	 if (CURRENT_TASK->state == T_READY){
		  CURRENT_TASK->state = T_RUNNING;
	    sp = CURRENT_TASK->sp;    
		 
			for(i = 0; i < NUM_TASKS-1; i ++){
					
						CURRENT_TASK->next->clk_ticks = (ROM_SysTickPeriodGet() - ROM_SysTickValueGet() - CURRENT_TASK->clk_ticks);
						CURRENT_TASK = CURRENT_TASK->next;
			}
	 } else {     /* task->state == T_CREATED so make it "ready" 
	                give it an interrupt frame and then launch it 
	    		        (with a blr sith 0xfffffff9 in LR in StartNewTask())  */
		  CURRENT_TASK->state = T_RUNNING;
			sp = StartNewTask(CURRENT_TASK->sp,(uint32_t) CURRENT_TASK->func); // Does not return!
		}
		return(sp);
	}

//	
//AMW
//
void PortF_Init(void){
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); 
	ROM_SysCtlDelay(1); 
	ROM_GPIOPinTypeGPIOOutput(PORTF_BASE_ADDR, PINS_1_2_3); 
}

void SysTick_Init(void){
		ROM_SysTickEnable();   
		ROM_SysTickPeriodSet(16700000);   
		ROM_SysTickIntEnable(); 
}

//void SysTick_Handler(void){   
//	ROM_GPIOPinWrite(PORTF_BASE_ADDR, PIN_2 ,  
//	~(ROM_GPIOPinRead(PORTF_BASE_ADDR, PIN_2)) );  // toggle PF2 (Blue LED) 
//}
	
//
// AMW
//
void ps(void){ 
	int i, stack_size = 0;
	float percent_cpu, percent_stack = 0.0;
	unsigned int total_num_ticks;
	
	//DisableInterrupts();
	
	for(i = 0; i < NUM_TASKS; i++){
		total_num_ticks += CURRENT_TASK->clk_ticks;		
		CURRENT_TASK = CURRENT_TASK->next;
	}
	
	
	/* Print contents of task list */
	
	/*
			PROTECTED SHARED RESOURCE (UART)
	*/	
	printf("\nUSER\tTID\tTICKS\t\t%%CPU\tSTK_SZ\t%%STK\tSTATE\t\tADDR\n");
	for( i = 0; i < NUM_TASKS; i++ ){
			if(CURRENT_TASK->tid != 0){
				stack_size = (CURRENT_TASK->stack_end-CURRENT_TASK->stack_start+1);
				percent_cpu =	 (((float) (CURRENT_TASK->clk_ticks)) / total_num_ticks)*100.0;
				percent_stack = (((float)(CURRENT_TASK->stack_end-CURRENT_TASK->sp))/stack_size)*100.0;
				if( percent_stack < 0.0 ){
					percent_stack = 0.0;
				}
				printf("ROOT\t");
				printf("%02d\t", CURRENT_TASK->tid);
				printf("%08u\t", CURRENT_TASK->clk_ticks);
				printf("%02.1f\t", percent_cpu);
				printf("%d\t", (stack_size));
				printf("%02.1f\t", percent_stack);
				
				if( CURRENT_TASK->state == T_READY ){
					printf("RDY\t\t0x%p\n", CURRENT_TASK);
				}
				else if( CURRENT_TASK->state == T_RUNNING ){
					printf("RUN\t\t0x%p\n", CURRENT_TASK);
				}
			}
			CURRENT_TASK = CURRENT_TASK->next;
		}
			
	/*
			PROTECTED SHARED RESOURCE (UART)
	*/
	//EnableInterrupts();
}


void OS_Sem_Init(unsigned int *sem, unsigned int count){
	DisableInterrupts();
	/*
			CRITICAL SECTION OF CODE
	*/
	*sem = count;
	/*
			CRITICAL SECTION OF CODE
	*/
	EnableInterrupts();
}

void OS_Sem_Signal(unsigned int *sem){
	DisableInterrupts();
	/*
			CRITICAL SECTION OF CODE
	*/
	*sem = *sem + 1;
	/*
			CRITICAL SECTION OF CODE
	*/
	EnableInterrupts();
}

void OS_Sem_Wait(unsigned int *sem){
	DisableInterrupts();
	while( *sem == 0 ){
		EnableInterrupts(); // allow scheduler a chance to be called
		//OS_Suspend();
		DisableInterrupts(); // turn off interrupts when we go to exit the loop
			// It's a block party!!!
			// (Process is blocked)
	}
	/*
			CRITICAL SECTION OF CODE
	*/
	*sem = *sem - 1;	
	/*
			CRITICAL SECTION OF CODE
	*/	
	EnableInterrupts();
}

/*
__asm void OS_Suspend(void){
	 	//LDR R0, #0xE000E010
		//LDR R1, [R0]
		ORR STCSR, #0x02 // load address of SysTick Control Register
		//STR [R0], R1
		BX LR				 
}
*/
