#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>
 
void producer(int count) {
    // Iterates from 0 to count, setting
    // the value of the global variable 'n'
    // each time.
    //print produced value e.g. produced : 8
 
    for (int i = 0; i < count; i++){
        printf("Producer: Setting value to %d\n", i);
        n = i;
        printf("Producer: Value of n set to %d\n", n);
    }
}
 
void producer_bb(int count) {
  // Iterate from 0 to count and for each iteration add iteration value to the global array `arr_q`, 
  // print producer process name and written value as,
  // name : producer_1, write : 8
    for (int i = 0; i < count; i++){
        wait(sprod);
        wait(writelock);
        printf(
            "Name: Producer %s: Written: %d\n", proctab[getpid()].prname, i
        );
        arr_q[writeind] = i;
        writeind = (writeind+1)%5;
        signal(scons);
        signal(writelock);
    }
}