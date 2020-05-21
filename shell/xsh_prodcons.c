


#include <xinu.h>
#include <prodcons.h>
#include <ctype.h>
#include <stdlib.h>
/*------------------------------------------------------------------------
 *  * xsh_prodcons- shell command to produce and consume
 *   *------------------------------------------------------------------------
 *    */

int n;                 

shellcmd xsh_prodcons(int nargs, char *args[])
{
	 
	 
	  int count = 2000;            
	    
	    
	     if( strcmp(args[0],"prodcons")==0 && (nargs==1 || nargs ==2)){
		     if(nargs==2){
			if(atoi(args[1])>0){

			     count = atoi(args[1]);
			     resume( create(producer, 1024, 20, "producer", 1, count));
			     resume( create(consumer, 1024, 20, "consumer", 1, count));

			    }else{
			    	 fprintf(stderr, "%s: count cannot be less than or equal to 0\n", args[0]);
				 fprintf(stderr, "Try '%s --help' for more information\n",args[0]);

			    }

		     }else{

		      resume( create(producer, 1024, 20, "producer", 1, count));
	              resume( create(consumer, 1024, 20, "consumer", 1, count));
		   }
  	     }
	        if(nargs > 2){
	   		fprintf(stderr, "%s: too many arguments\n", args[0]);
			fprintf(stderr, "Try '%s --help' for more information\n",args[0]);
		}

	    return (0);
}
