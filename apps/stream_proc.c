#include<xinu.h>
#include<stream.h>
#include "tscdf.h"
#include<stdio.h>

struct stream **stream_obj;
int global_queue_depth, global_output_time, global_time_window;
uint pcport;

//producer
int32 stream_proc(int nargs, char* args[]) {
  //variable declarations
  ulong secs, msecs, time;
  secs = clktime;
  msecs = clkticks;
  int32 num_streams, work_queue_depth,time_window,output_time;
  int st,ts,v, i, head;
  char *a, *ch, c;
  char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n"; 
  // Parse arguments

  /* Parse arguments out of flags */
  /* if not even # args, print error and exit */
  if (!(nargs % 2)) {
    printf("%s", usage);
    return(-1);
  }
  else {
    int i = nargs - 1;
    while (i > 0) {
      ch = args[i-1];
      c = *(++ch);
      
      switch(c) {
      case 's':
        num_streams = atoi(args[i]);
        break;

      case 'w':
        work_queue_depth = atoi(args[i]);
        break;

      case 't':
        time_window = atoi(args[i]);
        break;
        
      case 'o':
        output_time = atoi(args[i]);
        break;

      default:
        printf("%s", usage);
        return(-1);
      }

      i -= 2;
    }
  }

  if((pcport = ptcreate(num_streams)) == SYSERR) {
      printf("ptcreate failed\n");
      return(-1);
  }

  global_queue_depth = work_queue_depth;
  global_time_window = time_window;
  global_output_time = output_time;

    
  // Create streams
    if ((stream_obj = (struct stream *)getmem(sizeof(struct stream *) * (num_streams))) == (struct stream *)SYSERR){
      printf("ERROR");
      return(SYSERR);
  }

  for (i = 0; i < num_streams; i++) {
    if ((stream_obj[i] = (struct stream *)getmem(sizeof(struct stream) + (sizeof(de) * work_queue_depth))) == (struct stream *)SYSERR){
            printf("problem allocating error");
            return;
    }
    stream_obj[i]->head = 0;
    stream_obj[i]->tail = 0;
    stream_obj[i]->spaces = semcreate(0);
    stream_obj[i]->mutex = semcreate(1);
    stream_obj[i]->items = semcreate(work_queue_depth);
    stream_obj[i]->queue = sizeof(struct stream) + (char *)stream_obj[i]; 
  }

  // Create consumer processes and initialize streams
  // Use `i` as the stream id.
  for (int i = 0; i < num_streams; i++) {
    	resume( create(stream_consumer, 1024, 20, "stream_consumer", 2, i, stream_obj[i]) );
  }

// Parse input header file data and populate work queue
  for (i = 0; i < n_input; i++){
        a = (char *)stream_input[i];
        st = atoi(a);
        while (*a++ != '\t');
        ts = atoi(a);
        while (*a++ != '\t');
        v = atoi(a);
        wait(stream_obj[st]->items);
        wait(stream_obj[st]->mutex);
        head = stream_obj[st]->head;
        stream_obj[st]->queue[head].time = ts;
        stream_obj[st]->queue[head].value = v;
        head = ++head % work_queue_depth;
        stream_obj[st]->head = head;

        signal(stream_obj[st]->mutex);
        signal(stream_obj[st]->spaces);
  }



   for (int i = 0; i < num_streams; i++) {
        uint32 pm;
        pm = ptrecv(pcport);
        printf("process %d exited\n", pm);
  }

   ptdelete(pcport, 0);

  //Time Calculation
  time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
  printf("time in ms: %u\n", time);

  return 0;
}

void stream_consumer(int32 id, struct stream *str){
    int tail;
    kprintf("stream_consumer id:%d ", id);
    kprintf("(pid:%d)\n", getpid());
    struct tscdf *tc;
    int count = 0;
    tc = tscdf_init(global_time_window);
    while (TRUE){
        wait(str->spaces);
        wait(str->mutex);
        tail = str->tail;
        if (str->queue[str->tail].time == 0 && str->queue[str->tail].value == 0){
            kprintf("stream_consumer exiting\n");
            break;
        }
        tscdf_update(tc, str->queue[tail].time, str->queue[tail].value);

        if (count++ == (global_output_time-1)){
            char output[10];
            int *qarray;
            qarray = tscdf_quartiles(tc);
            if (qarray == NULL){
                kprintf("tscdf_quartiles returned NULL\n");
                continue;
            }
            sprintf(output, "s%d: %d %d %d %d %d \n", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("%s", output);
            freemem((char *)qarray, (6 * sizeof(int32)));
            count = 0;
        }

        tail = ++tail % global_queue_depth;
        str->tail = tail;
        signal(str->mutex);
        signal(str->items);
    }
    tscdf_free(tc);
	  ptsend(pcport, getpid());
}
