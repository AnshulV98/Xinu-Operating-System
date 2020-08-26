#include <xinu.h>
#include <string.h>
#include <stdio.h>
#include <prodcons_bb.h>
#include <future.h>
#include <stream.h>
#include<fs.h>
 
// definition of array, semaphores and indices 
 
int arr_q[5];

sid32 sprod;
sid32 scons;
sid32 readlock;
sid32 writelock;
 
int readind;
int writeind;

//prodcons_bb
void prodcons_bb(int nargs, char *args[]) {
  //create and initialize semaphores to necessary values
  sprod = semcreate(5);
  scons = semcreate(0);
  readlock = semcreate(1);
  writelock = semcreate(1);
 
  //initialize read and write indices for the queue
  readind = 0;
  writeind = 0;
 
  //create producer and consumer processes and put them in ready queue
  char proc_name[100];
  for (int i=0; i < atoi(args[1]); i++){
    sprintf(proc_name, "Producer %d", i);
    resume (
        create(producer_bb, 1024, 20, proc_name, 1, atoi(args[3]))
      );
  }
  for (int i=0; i < atoi(args[2]); i++){
    sprintf(proc_name, "Consumer %d", i);
    resume (
        create(consumer_bb, 1024, 20, proc_name, 1, atoi(args[4]))
      );
  }
}

//future prodcons
void future_prodcons(int nargs, char* args[]){
   future_t* f_exclusive, * f_shared;
    f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
    f_shared    = future_alloc(FUTURE_SHARED, sizeof(int), 1);

    // Test FUTURE_EXCLUSIVE
    resume( create(future_cons, 1024, 20, "fcons1", 1, f_exclusive) );
    resume( create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, (char*) &one) );

    // Test FUTURE_SHARED
    resume( create(future_cons, 1024, 20, "fcons2", 1, f_shared) );
    resume( create(future_cons, 1024, 20, "fcons3", 1, f_shared) );
    resume( create(future_cons, 1024, 20, "fcons4", 1, f_shared) );
    resume( create(future_cons, 1024, 20, "fcons5", 1, f_shared) );
    resume( create(future_prod, 1024, 20, "fprod2", 2, f_shared, (char*) &two) );
}

//future_queue_tests
void futureq_test1 (int nargs, char *args[]) {
    int three = 3, four = 4, five = 5, six = 6;
    future_t *f_queue;
    f_queue = future_alloc(FUTURE_QUEUE, sizeof(int), 3);

    resume(create(future_cons, 1024, 20, "fcons6", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons7", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons8", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons9", 1, f_queue));
    resume(create(future_prod, 1024, 20, "fprod3", 2, f_queue, (char *)&three));
    resume(create(future_prod, 1024, 20, "fprod4", 2, f_queue, (char *)&four));
    resume(create(future_prod, 1024, 20, "fprod5", 2, f_queue, (char *)&five));
    resume(create(future_prod, 1024, 20, "fprod6", 2, f_queue, (char *)&six));
    sleep(1);
}

void futureq_test2 (int nargs, char *args[]) {
    int seven = 7, eight = 8, nine=9, ten = 10, eleven = 11;
    future_t *f_queue;
    f_queue = future_alloc(FUTURE_QUEUE, sizeof(int), 3);

    resume(create(future_prod, 1024, 20, "fprod10", 2, f_queue, (char *)&seven));
    resume(create(future_prod, 1024, 20, "fprod11", 2, f_queue, (char *)&eight));
    resume(create(future_prod, 1024, 20, "fprod12", 2, f_queue, (char *)&nine));
    resume(create(future_prod, 1024, 20, "fprod13", 2, f_queue, (char *)&ten));
    resume(create(future_prod, 1024, 20, "fprod13", 2, f_queue, (char *)&eleven));

    resume(create(future_cons, 1024, 20, "fcons14", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons15", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons16", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons17", 1, f_queue));
    resume(create(future_cons, 1024, 20, "fcons18", 1, f_queue));
    sleep(1);
}

void futureq_test3 (int nargs, char *args[]) {
    int three = 3, four = 4, five = 5, six = 6;
    future_t *f_queue;
    f_queue = future_alloc(FUTURE_QUEUE, sizeof(int), 3);

    resume( create(future_cons, 1024, 20, "fcons6", 1, f_queue) );
    resume( create(future_prod, 1024, 20, "fprod3", 2, f_queue, (char*) &three) );
    resume( create(future_prod, 1024, 20, "fprod4", 2, f_queue, (char*) &four) );
    resume( create(future_prod, 1024, 20, "fprod5", 2, f_queue, (char*) &five) );
    resume( create(future_prod, 1024, 20, "fprod6", 2, f_queue, (char*) &six) );
    resume( create(future_cons, 1024, 20, "fcons7", 1, f_queue) );
    resume( create(future_cons, 1024, 20, "fcons8", 1, f_queue) );
    resume( create(future_cons, 1024, 20, "fcons9", 1, f_queue) );
    sleep(1);
}

//future_fibonacci
void future_fibonacci(int nargs, char* args[]){
  int fib = -1, i;

    fib = atoi(args[2]);

    if (fib > -1) {
      int final_fib;
      int future_flags = FUTURE_SHARED; // TODO - add appropriate future mode here

      // create the array of future pointers
      if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1)))
          == (future_t **) SYSERR) {
        printf("getmem failed\n");
        return(SYSERR);
      }

      // get futures for the future array
      for (i=0; i <= fib; i++) {
        if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
          printf("future_alloc failed\n");
          return(SYSERR);
        }
      }

      // spawn fib threads and get final value
      // TODO - you need to add your code here

      for (i = 0; i <= fib; i++){
            resume(create(ffib, 1024, 20, "ffib", 1, i));
        }

        int status = (int)future_get(fibfut[fib], (char *)&final_fib);
        if (status < 1){
            printf("future_get failed\n");
            return -1;
        }

        for (i = 0; i <= fib; i++){
            future_free(fibfut[i]);
        }

      freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
      printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
      return(OK);
    }
}
 
 
shellcmd xsh_run(int nargs, char *args[]) {
    //For listing out commands
   if ((nargs == 1) || (strncmp(args[1], "list", 5) == 0)) {
      printf("prodcons_bb\nfutures_test -f -pc [-fq1|-fq2|fq3]\ntscdf -s -w -t -o\ntscdf_fq -s -w -t -o\nfstest\n");
      return OK;
    }
 
    args++;
    nargs--;
    //For prodcons_bb
    if(strncmp(args[0], "prodcons_bb", 11) == 0) {
      resume (
        create(
          (void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args
        )
      );
    }

    else if(strncmp(args[0], "tscdf_fq", 8) == 0) {
      resume  (
        create (
            /*(int *)*/stream_proc_futures, 4096, 20, "stream_futures_producer", 2, nargs, args
          )
        );
    }

    //for FileSystem test
    else if(strncmp(args[0], "fstest", 6) == 0) {
      resume  (
        create (
            /*(int *)*/fstest, 4096, 20, "file_system_test", 2, nargs, args
          )
        );
    }

    //For futures_test
    else if(strncmp(args[0], "futures_test", 12) == 0) {
      if(strncmp(args[1], "-pc", 3) == 0){
        resume (
        create(
          (void *)future_prodcons, 4096, 20, "future_prodcons", 2, nargs, args
        )
      );   
      }
      else if(strncmp(args[1], "-fq1", 4) == 0){
        resume (
        create(
          (void *)futureq_test1, 4096, 20, "futureq_test1", 2, nargs, args
        )
      );
      }
      else if(strncmp(args[1], "-fq2", 4) == 0){
        resume (
        create(
          (void *)futureq_test2, 4096, 20, "futureq_test2", 2, nargs, args
        )
      );
      }
      else if(strncmp(args[1], "-fq3", 4) == 0){
        resume (
        create(
          (void *)futureq_test3, 4096, 20, "futureq_test3", 2, nargs, args
        )
      );
      }
      else if(strncmp(args[1], "-f", 2) == 0){
        resume (
        create(
          (void *)future_fibonacci, 4096, 20, "future_fibonacci", 2, nargs, args
        )
      );
      }
    }

    //For streams
    else if(strncmp(args[0], "tscdf", 5) == 0) {
      resume  (
        create (
            /*(int *)*/stream_proc, 4096, 20, "stream_producer", 2, nargs, args
          )
        );
    }

    //If all else fails
    else {
      printf("Unidentified command\n");
    }

  return 0;
}
