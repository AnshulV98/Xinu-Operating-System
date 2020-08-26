#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>
 
void consumer(int count) {
  // reads the value of the global variable 'n'
  // 'count' times.
  // print consumed value e.g. consumed : 8
    for (int i = 0; i < count; i++){
        printf("Consumer: Value of 'n' is %d\n", n);
    }
}
 
void consumer_bb(int count) {
  // Iterate from 0 to count and for each iteration read the next available value from the global array `arr_q`
  // print consumer process name and read value as,
  // name : consumer_1, read : 8
    for (int i = 0; i < count; i++){
        wait(scons);
        wait(readlock);
        printf(
            "Name: Consumer %s: Read: %d\n",
            proctab[getpid()].prname, arr_q[readind]
        );
        readind = (readind+1)%5;
        signal(sprod);
        signal(readlock);
    }
}