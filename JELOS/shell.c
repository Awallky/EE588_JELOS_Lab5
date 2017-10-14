// shell.c  
//  Spring 2016
// James E. lumpp Jr.  
// 4/9/2016
//

#include <stdio.h>
#include <stdint.h>
#include "mylib.h"

void prmsg(char *);
int strcmp(const char *s1, const char *s2);
extern unsigned int *sem;

// -----------------------------------------------------------------
// SHELL BUILT_IN FUNCTIONS
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// FUNCTION  time:                                                  
//    Print the current time.                   
// ----------------------------------------------------------------- 

void time(void)
{  
unsigned char h,m,s;
 // Replace this with your print function or Unix time.
  printf("\nThe time is: "); 
  printf("%d:%d:%d\n",(int)h,(int)m,(int)s); // these variables will change in the background
}

// -----------------------------------------------------------------
// FUNCTION  settime:                                                  
//    Prompt the user for the current time and enter into the globals                  
// ----------------------------------------------------------------- 

void settime(char *instr)
{  
  int valid;

  do{
      printf("\n Set time to %s\n",instr);  // prompt user
 //     gets(str);  /repromt user?
		  valid=1;
  }while (valid==0);

}

void temp(void)
{  
int a;

 // Do analog to digital conversion and print the result

  printf("\nvalue is %d\n",a);  // request a single conversion
}
// -----------------------------------------------------------------
// Shell functions
// -----------------------------------------------------------------
// -----------------------------------------------------------------
// FUNCTION  parse:                                                  
//    This function replaces all white space with zeros until it     
// reaches a character that indicates the beginning of an     
// argument.  It saves the address to argv[].                   
// ----------------------------------------------------------------- 

void  parse(char *line, char **argv)
{
     while (*line != '\0') {       /* if not the end of line ....... */ 
          while (*line == ' ' || *line == '\t' || *line == '\n' || *line == '\r')
               *line++ = '\0';     /* replace white spaces with 0    */
          *argv++ = line;          /* save the argument position     */
          while (*line != '\0' && *line != ' ' && 
                 *line != '\t' && *line != '\n' && *line != '\r') 
               line++;             /* skip the argument until ...    */
     }
     *argv = 0;                 /* mark the end of argument list  */
}

// -----------------------------------------------------------------                                             
//    This function will start a new process.  For now it just prints
//    The requested call                                        
// ----------------------------------------------------------------- 
     
void  execute(char **argv)
{ unsigned char i;

  printf("fork-exec: ");

  for(i=0;i<9;i++){
    if (argv[i] == 0) 
	   break;
    prmsg(argv[i]);
	  putchar(' ');
  }
  printf("\n");

}

// -----------------------------------------------------------------
// implementation of a shell
// -----------------------------------------------------------------

void  shell(void)
{
     char  line[40] = {0};          /* the input line init all chars to zero  */
     char  *argv[10] = {0};              /* the command line argument      */
//	 unsigned char i;
     ROM_GPIOPinWrite(GPIO_PORTF_BASE, 
											GPIO_PIN_3 |GPIO_PIN_2 | GPIO_PIN_1,
											0); // All LEDs off
		 
     while (1) {                   /* repeat until done ....         */
					OS_Sem_Wait(sem);
          printf("jelos# ");     /*   display a prompt             */
					OS_Sem_Signal(sem);
		      gets(line);          // get a line from the user
          parse(line, argv);       /*   parse the line               */
          if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0 ) {
		                   /* is it an "exit"?     */
                printf("Exiting...\n");
				return;        
	      } else if (strcmp(argv[0], "time") == 0){
						OS_Sem_Wait(sem);
						time();
						OS_Sem_Signal(sem);
					}
					else if (strcmp(argv[0], "ps") == 0){
						OS_Sem_Wait(sem);
						ps();
						OS_Sem_Signal(sem);
					}
	        else if (strcmp(argv[0], "settime") == 0){
						OS_Sem_Wait(sem);
						settime(argv[1]);
						OS_Sem_Signal(sem);
					}
          else if (strcmp(argv[0], "temp") == 0){
						OS_Sem_Wait(sem);
						temp();
						OS_Sem_Signal(sem);
					}
					else if (strcmp(argv[0], "i") == 0){
						OS_Sem_Wait(sem);
						puts("an i\n");
						OS_Sem_Signal(sem);
					}
					else if (*argv[0] != 0 && argv[0] != 0){
						OS_Sem_Wait(sem);
						execute(argv);    /* if not empy line execute command as new process*/
						OS_Sem_Signal(sem);
					}
					else{
						OS_Sem_Wait(sem);
						putchar('\n');
						OS_Sem_Signal(sem);
					}
     }//while(1)
		 
}

// -----------------------------------------------------------------
// main function now just launches one shell
// -----------------------------------------------------------------


         
