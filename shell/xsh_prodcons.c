#include <xinu.h>
#include <prodcons.h>

int n;                 //Definition for global variable 'n'
/*Now global variable n will be on Heap so it is accessible all the processes i.e. consume and produce*/

shellcmd xsh_prodcons(int nargs, char *args[])
{
	int count;
  //Argument verifications and validations
  count = 2000; //local varible to hold count
  //check args[1] if present assign value to count
  if(nargs == 2){
  	count = atoi(args[1]);
  }
  else if(nargs > 2){
  	printf("Invalid number of arguments\n");
  }
  //create the process producer and consumer and put them in ready queue.
  //Look at the definations of function create and resume in the system folder for reference.      
    resume( create(producer_bb, 1024, 20, "producer", 1, count));
    resume( create(consumer_bb, 1024, 20, "consumer", 1, count));
  return (0);
}