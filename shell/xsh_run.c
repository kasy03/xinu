#include <xinu.h>
#include <prodcons_bb.h>
#include <stdlib.h>
#include <future.h>
#include <future_prodcons.h>
#include <stream.h>
#include <fs.h>
/*------------------------------------------------------------------------
 *  * xsh_run- shell command to run functions
 *   *------------------------------------------------------------------------
 *    */
void prodcons_bb(int nargs, char *args[]); 
void futures_test(int nargs, char * args[]);            
void stream_proc_(int nargs,char *args[]);

void futureq_test1(int nargs, char *args[]) {
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

void futureq_test2(int nargs, char *args[]) {
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

void futureq_test3(int nargs, char *args[]) {
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
void stream_proc_fut(int nargs, char *args[]){
	stream_proc_futures(nargs,args);
}
int one;
int two;
shellcmd xsh_run(int nargs, char *args[])
{
	 if ((nargs == 1) || (strncmp(args[1], "list", 4) == 0)) {
		printf("prodcons_bb\n");
		printf("futures_test\n");
		return OK;
	}
	/** check for command **/
	if((nargs == 5) && strncmp(args[1], "prodcons_bb", 11) == 0) {
		resume(create((void *)prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
	}else if(strncmp(args[1],"futures_test",12) == 0){
		resume(create((void *)futures_test, 4096, 20, "futures_test", 2,nargs, args));
	}else if(strncmp(args[1],"tscdf_fq",8)==0){
		resume(create((void *)stream_proc_fut, 4096, 20, "stream_proc_fut", 2, nargs, args));
	}else if(strncmp(args[1], "tscdf", 5) == 0){
		resume(create((void *)stream_proc_, 4096, 20, "stream_proc_", 2, nargs, args));
	}else if(strncmp(args[1], "fstest", 6) == 0){
		args++;
    	nargs--;
		resume(create((void *)fstest, 4096, 20, "fstest", 2, nargs, args));
	}	
}


			  
int readIndex, writeIndex, arr_q[buff_size];
sid32 psems, csems, mutex;

void stream_proc_(int nargs,char *args[]){
		stream_proc(nargs,args);
}

void prodcons_bb(int nargs, char *args[]) {
	 
	  psems = semcreate(buff_size);
	  csems = semcreate(0);
	  mutex = semcreate(1);
	  readIndex = 0;
		  writeIndex = 0;
		  char buffer[200];
	
	  if(nargs == 5 && (atoi(args[2])*atoi(args[4]) == atoi(args[3])*atoi(args[5])) ){
		for(int i =1; i<=atoi(args[2]);i++){
			sprintf(buffer, "produce_%d",i);
			resume(create((void *)producer_bb, 4096, 20, buffer, 1,atoi(args[4])));
		}
		for(int j=1; j<=atoi(args[3]);j++){
			sprintf(buffer, "consume_%d",j);
			resume(create((void *)consumer_bb, 4096, 20, buffer, 1,atoi(args[5])));
		  }
	}else{
		fprintf(stderr, "%s: Producer and consumer do not match\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n",args[0]);
	}
}


void futures_test(int nargs, char * args[]){
	/** for producer consumer  **/
	if(strncmp(args[2],"-pc",3) == 0){
		future_t* f_exclusive, * f_shared;
		f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
		f_shared    = future_alloc(FUTURE_SHARED, sizeof(int), 1);
		one = 1;
		resume( create(future_cons, 1024, 20, "fcons1", 1, f_exclusive) );
		resume( create(future_prod, 1024, 20, "fprod1", 2, f_exclusive, (char*) &one) );

		two =2;
		resume( create(future_cons, 1024, 20, "fcons2", 1, f_shared) );
		resume( create(future_cons, 1024, 20, "fcons3", 1, f_shared) );						
		resume( create(future_cons, 1024, 20, "fcons4", 1, f_shared) );
		resume( create(future_cons, 1024, 20, "fcons5", 1, f_shared) );
		resume( create(future_prod, 1024, 20, "fprod2", 2, f_shared, (char*) &two) );
	}else if(strncmp(args[2],"-fq1",4) ==0){
		futureq_test1(nargs, args);
	}else if(strncmp(args[2],"-fq2",4) ==0){
		futureq_test2(nargs, args);
	}else if(strncmp(args[2],"-fq3",4) ==0){
		futureq_test3(nargs, args);
	}else if(strncmp(args[2],"-f",2) == 0){
		/** for fibonnaci  **/
			int fib = -1, i;
			fib = atoi(args[3]);
			if (fib > -1) {
				int final_fib;
				int future_flags = FUTURE_SHARED; 
				if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1))) == (future_t **) SYSERR) {
					printf("getmem failed\n");
					return(SYSERR);
				}
				for (i=0; i <= fib; i++) {
					if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
						printf("future_alloc failed\n");
						return(SYSERR);
					}
				}

			 for (i=0; i <= fib; i++) {
				ffib(i);
			 }


			future_get(fibfut[fib], (char*) &final_fib);
			for (i=0; i <= fib; i++) {
				future_free(fibfut[i]);
			}

			freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
			printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
			return(OK);	
			}
	}
}
