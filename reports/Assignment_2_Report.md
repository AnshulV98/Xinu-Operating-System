## Report Assignment 2 (ansvohra)

### Questions 

#### Does your program output any garbage? If yes, why?
Yes, it does output garbage, I believe the reason for this is:
* There is no controlling mechanism which may control the access to a shared resource( in this instance, the global variable 'n').
   This results in the consumer process trying to access the variable even before the producer is done with producing it.

#### Are all the produced values getting consumed? Check your program for a small count like 20.
No, both the processes are behaving in a chaotic manner as I mentioned due to lack of a controlling mechanism.
![Screenshot for prodcons with input: 20](https://github.iu.edu/ansvohra/xinu-S20/blob/Assignment_2/Os_20.PNG)


### Functions

#### include/prodcons.h

```C

/*Global variable for producer consumer*/
extern int n; /*this is just declaration*/

/*function Prototype*/
void consumer(int count);
void producer(int count);

```

#### shell/xsh_prodcons.c
```C
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
  resume( create(producer, 1024, 20, "producer", 1, count));
  resume( create(consumer, 1024, 20, "consumer", 1, count));
  return (0);
}
```


#### apps/produce.c

```C
#include <xinu.h>
#include <prodcons.h>

void producer(int count) {
    // Iterates from 0 to count, setting
    // the value of the global variable 'n'
    // each time.
    //print produced value e.g. produced : 8
    int i = 0;
    for(i; i<=count ; i++){
    	n = i;
    	printf("Produced: %d\n",n);
    }
}
```


#### apps/consume.c

```C
#include <xinu.h>
#include <prodcons.h>

void consumer(int count) {
  // reads the value of the global variable 'n'
  // 'count' times.
  // print consumed value e.g. consumed : 8
	int i=0;
	for(i;i<=count;i++){
		printf("Consumed: %d\n",n );
	}
}
```
