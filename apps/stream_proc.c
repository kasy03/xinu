#include <stream.h>
#include "tscdf.h"
uint pcport;
int num_streams, work_queue_depth, time_window,output_time;
void stream_consumer(int32 id, struct stream *str);
int32 stream_proc(int nargs, char *args[]) {
	  ulong secs, msecs, time;
	    secs = clktime;
	      msecs = clkticks;
		 
		 char usage[] = "Usage: -s num_streams -w work_queue_depth -t time_window -o output_time\n";
		 char *ch,c;
		  int i;
		  //printf("%d",nargs);     
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
		      printf("ptcreate failed\n");
		       return(-1);
		}
					  	
		struct stream **str_array = (struct stream **)getmem(sizeof(struct stream*) * num_streams);
		for (i = 0; i < num_streams; i++) {
		    struct stream *curr_stream = (struct stream *)getmem(sizeof(struct stream));
		    curr_stream->queue = (struct data_element **) getmem(sizeof(struct data_element*) * work_queue_depth);
		     for(int j = 0; j < work_queue_depth; j++) {
				 curr_stream->queue[j] = (struct data_element *) getmem(sizeof(struct data_element));
		       }
			curr_stream->head = 0;
			curr_stream->tail = 0;
			curr_stream->spaces = semcreate(work_queue_depth);
			curr_stream->items = semcreate(0);
			curr_stream->mutex = semcreate(1);
			str_array[i] = curr_stream;
			resume(create(stream_consumer, 1024, 20, 'str', 2, i, str_array[i]));

		  }


	      int st, ts, v;
	      char *a;
	      for(int i=0;i<n_input;i++){
			a = (char *)stream_input[i];
			st = atoi(a);
			while (*a++ != '\t');
			ts = atoi(a);
			while (*a++ != '\t');
			v = atoi(a);
				//resume(create(stream_consumer, 1024, 20, 'str', 2, st, str_array[st]));
			wait(str_array[st]->spaces);
			wait(str_array[st]->mutex);

			struct data_element *tmp = str_array[st]->queue[str_array[st]->tail % work_queue_depth];
			tmp->value = v;
			tmp->time = ts;

			str_array[st]->tail += 1;
			signal(str_array[st]->mutex);
			signal(str_array[st]->items);

		}
		  for(i=0; i < num_streams; i++) {
			 uint32 pm;
			 pm = ptrecv(pcport);
			 kprintf("process %d exited\n", pm);
		}

	ptdelete(pcport, 0);
	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
	kprintf("time in ms: %u\n", time);
	return(0);
}



void stream_consumer(int32 id, struct stream *str) {
	  int i;
	  int count =0;
	  char *output = ""; 
	  int32 *qarray;
	  //kprintf("stream_consumer id :%d (pid:%d)\n",id,getpid());
	  kprintf("stream_consumer id:%d (pid:%d)\n",id,getpid());
	   struct tscdf *tc = tscdf_init(time_window);
	     while(1) {
		     	 //count++;
			 wait(str->items);
			 wait(str->mutex);
		     	 
			 struct data_element *value = str->queue[str->head % work_queue_depth];
			 str->head +=1;
			 if (value->time == 0 && value->value==0) {
				kprintf("stream_consumer exiting\n");
				ptsend(pcport, getpid());
				return;
			}
				
			tscdf_update(tc,value->time, value->value);
		     	count+=1;
			//kprintf("count: %d\n",count);
			if(count==output_time){
				//kprintf("before\n");
				qarray = tscdf_quartiles(tc);
				//kprintf("after\n");
				if(qarray == NULL) {
				  kprintf("tscdf_quartiles returned NULL\n");
				  continue;
				}
				sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
				kprintf("%s\n", output);
				freemem((char *)qarray, (6*sizeof(int32)));
				count=0;
			}
		     	
		     	 
			signal(str->mutex);
			signal(str->spaces);

		 }
}
