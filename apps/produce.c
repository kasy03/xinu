#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>
void producer(int count) {
	    int32 i;
	        for( i=1 ; i<=count ; i++ ){
	         n = i;		     
		 printf("Value produced : %d \n", n);
	        }
}

void producer_bb(int count) {
	 
	  for(int i =1; i<=count;i++){
		  	wait(psems);
				wait(mutex);
					printf("name : %s, write : %d \n",proctab[getpid()].prname, i);
						arr_q[writeIndex] = i;
							writeIndex = (writeIndex + 1) % buff_size;
								signal(mutex);
									signal(csems);
									  }
	    
	    
}
