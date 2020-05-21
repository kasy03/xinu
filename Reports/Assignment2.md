# Assignment 2
## Produce and Consume

1. Does your program output any garbage? If yes, why?
   Yes, the program does output some garbage values as displayed in the image below. This happens because the creation of two processes _producer_ and _consumer_ is asynchronous and both the process refer to the value of the global variable n.Since, the processes are not synchronized the operation system tends to switch between the processes. So as the producer starts producing values and by the time the values are actually consumed a lot of the values have already been produced. Also, it is observed that the xsh $ prompt is getting printed on the screen, which happens because after creation of the child processes, the main process exits printing xsh $ on the screen. 

2. Are all the produced values getting consumed? 
  Not all the values that are produced are consumed. Expecially the initial values that are produced. Because the produce function has a printf statement as well which takes time. The below image displays the output for prodcon 20. 
   
![alt text](https://github.iu.edu/knikharg/xinu-S20/blob/master/Reports/output.PNG)


### Functions 
Below are the two methods, producer and consumer where the values producer runs a loop and the value is assigned to the global variable n. The consumer function runs a loop to print the value of the global variable n.

_produce.c_ 
```
#include <xinu.h>
#include <prodcons.h>

void producer(int count) {
	    int32 i;
	        for( i=1 ; i<=count ; i++ ){
	         n = i;		     
		 printf("Value produced : %d \n", n);
	        }
}
```

_consume.c_
```
#include <xinu.h>
#include <prodcons.h>

void consumer(int count) {
	  int32 i;
	    for( i=1 ; i<=count ; i++ )
		      printf("Value consumed: %d \n", n);
}
```



