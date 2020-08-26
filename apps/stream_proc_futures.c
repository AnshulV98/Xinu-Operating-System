#include<xinu.h>
#include<future.h>
#include<stream.h>
#include "tscdf.h"

future_t **farray;
int global_queue_depth, global_output_time, global_time_window;
uint pcport;


int stream_proc_futures(int nargs, char* args[]) {
  // Parse arguments
  ulong secs, msecs, time;
  secs = clktime;
  msecs = clkticks;
  char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";
  int i, num_streams, work_queue_depth, time_window, output_time, st, ts ,v, head;
  char *a, *ch, c;
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

  // Create array to hold `n_streams` number of futures
  if ((farray = (future_t **)getmem(sizeof(future_t *) * (num_streams))) == (future_t **)SYSERR)
    {
        printf("getmem failed\n");
        return (SYSERR);
    }

    for (i = 0; i < num_streams; i++)
    {
        if ((farray[i] = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth)) == (future_t *)SYSERR)
        {
            printf("problem allocating error");
            return ;
        }
    }

  // Create consumer processes and allocate futures
  // Use `i` as the stream id.
  // Future mode = FUTURE_QUEUE
  // Size of element = sizeof(struct data_element)
  // Number of elements = work_queue_depth
  for (i = 0; i < num_streams; i++) {
      resume(create((void *)stream_consumer_future, 4096, 20, "stream_consumer_future", 2, i, farray[i]));
  }

  // Parse input header file data and set future values
  for (i = 0; i < n_input; i++)
    {
        a = (char *)stream_input[i];
        st = atoi(a);
        while (*a++ != '\t')
            ;
        ts = atoi(a);
        while (*a++ != '\t')
            ;
        v = atoi(a);
        de *tempdata = (de *)getmem(sizeof(de));
        tempdata->time = ts;
        tempdata->value = v;
        future_set(farray[st], tempdata);
    }


  // Wait for all consumers to exit
  for (i = 0; i < num_streams; i++)
    {
        uint32 pm;
        pm = ptrecv(pcport);
        printf("process %d exited\n", pm);
    }

    ptdelete(pcport, 0);

    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    printf("time in ms: %u\n", time);
    // free futures array

  return 0;
}

void stream_consumer_future(int32 id, future_t *f)
{
    kprintf("stream_consumer_future id:%d ", id);
    kprintf("(pid:%d)\n", getpid());
    struct tscdf *tc;
    int count = 0;
    tc = tscdf_init(global_time_window);
    while (TRUE)
    {
        de *tempData;
        future_get(f, tempData);

        if (tempData->time == 0 && tempData->value == 0)
        {
            break;
        }

        tscdf_update(tc, tempData->time, tempData->value);

        if (count++ == (global_output_time - 1))
        {
            char output[10];
            int *qarray;
            qarray = tscdf_quartiles(tc);
            if (qarray == NULL)
            {
                kprintf("tscdf_quartiles returned NULL\n");
                continue;
            }
            sprintf(output, "s%d: %d %d %d %d %d \n", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("%s", output);
            freemem((char *)qarray, (6 * sizeof(int32)));
            count = 0;
        }
    }
    tscdf_free(tc);
    ptsend(pcport, getpid());
}
