#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>
void consumer(int count) {
	  int32 i;
	    for( i=1 ; i<=count ; i++ )
		      printf("Value consumed: %d \n", n);
}


void consumer_bb(int count) {
	 
	    for(int i =1; i<=count;i++){
		    		wait(csems);
						wait(mutex);
							int j = arr_q[readIndex];
						 printf("name : %s, read : %d \n",proctab[getpid()].prname,j);

							readIndex = (readIndex + 1) % buff_size;
							signal(mutex);
							signal(psems);
																  }
}
