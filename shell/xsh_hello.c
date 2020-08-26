/* xsh_hello.c - xsh_hello */

#include<xinu.h>
#include<stdio.h>

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 * xsh_hello - Modified form of an echo statement, add the string along with commandline arguemnt with an appended statement
 * -----------------------------------------------------------------------------------------------------------------------------------------------------
 */

shellcmd xsh_hello(int nargs, char *args[]){
	
	int32 i; //iterating though the arguments
	printf("Hello ");
	if(nargs > 1){
		printf("%s",args[1]);

		for(i=2;i<nargs;i++) {
			printf(" %s", args[i]);
		}
	}
	printf(", Welcome to the world of Xinu!!!!\n");
	return 0;
}
