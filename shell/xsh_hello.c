#include <xinu.h>
#include <stdio.h>
#include <string.h>

/*------------------------------------------------------------------------
 *  * xsh_hello- shell command to welcome 
 *   *------------------------------------------------------------------------
 *    */


shellcmd xsh_hello(int nargs, char *args[]){

	    if(nargs == 2 && strcmp(args[0],"hello") == 0){
		            printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
			        }
	        if (nargs > 2) {
			                fprintf(stderr, "%s: too many arguments\n", args[0]);
					                fprintf(stderr, "Try '%s --help' for more information\n",
									                                args[0]);

							        }
			if (nargs < 2) {
				                fprintf(stderr, "%s: too few arguments\n", args[0]);
						                fprintf(stderr, "Try '%s --help' for more information\n",
										                                args[0]);

								        }
			     return 0;
}

