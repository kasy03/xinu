#include <xinu.h>
#include "tscdf.h"
#include <stream.h>
#include <future.h>

uint pcport;
uint num_streams, work_queue_depth, time_window, output_time;
void stream_consumer_future(int32 id, future_t *f);


int stream_proc_futures(int nargs, char* args[]) {
    ulong secs= clktime, msecs = clkticks, time;
	int i;
	char *ch,c;
    char usage[] = "\nUsage: -s num_streams -w work_queue_depth -t time_window -o output_time";

    if ((nargs % 2)!=0) {
			kprintf("%s", usage);
			return(-1);
   }else {
		i = nargs - 1;
		while (i > 2) {
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
				kprintf("%s", usage);
				return(-1);
			}

			i -= 2;
		}
	}	
    if((pcport = ptcreate(num_streams)) == SYSERR) {
        printf("\nptcreate failed");
        return(-1);
    }
    future_t **farray = (struct future_t **)getmem(sizeof(struct future_t) * num_streams);
    
    for (i = 0; i < num_streams; i++) {
        future_t *f_queue;
        f_queue = future_alloc(FUTURE_QUEUE, sizeof(struct data_element), work_queue_depth);

        farray[i] = f_queue;

        resume(create((void *)stream_consumer_future, 4096, 20, 'str', 2, i, farray[i]));
    }
	
	int st, ts, v;
    char *a;

    for (i = 0; i < n_input; i++) {

        a = (char *)stream_input[i];
        st = atoi(a); 
        while (*a++ != '\t');
        ts = atoi(a);
        while (*a++ != '\t');
        v = atoi(a);

        struct data_element *tmp = (struct data_element *)getmem(sizeof(struct data_element));
        tmp->value = v;
		tmp->time = ts;
        

        future_set(farray[st], tmp);
    }


    for(i=0; i < num_streams; i++) {
        uint32 pm;
        pm = ptrecv(pcport);
        printf("\nprocess %d exited", pm);
    }
   

    for (i = 0; i < num_streams; i++) {
        future_free(farray[i]);
    }

    ptdelete(pcport, 0);

    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    printf("\ntime in ms: %u", time);

    return 0;
}

void stream_consumer_future(int32 id, future_t *f) {
    int i;
    int cnt = 0;
    char *output = "";
    struct tscdf *tc = tscdf_init(time_window);
	int32 *qarray;
	
    kprintf("\nstream_consumer_future id:%d (pid:%d)", id, getpid());

    while(1) {
        cnt++;
        struct data_element *consumed = (struct data_element *)getmem(sizeof(struct data_element));
        future_get(f, consumed);

        if (consumed->time == 0 && consumed->value == 0) {
            tscdf_free(tc);
            ptsend(pcport, getpid());
            break;
        }

        tscdf_update(tc, consumed->time, consumed->value);
        if (cnt == output_time) {
            cnt = 0;
            qarray = tscdf_quartiles(tc);

            if(qarray == NULL) {
                kprintf("\ntscdf_quartiles returned NULL");
                continue;
            }

            sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("\n%s", output);
            freemem((char *)qarray, (6*sizeof(int32)));
        }
    }
}

