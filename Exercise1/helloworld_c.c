// gcc 4.7.2 +
// gcc -std=gnu99 -Wall -g -o helloworld_c helloworld_c.c -lpthread

#include <pthread.h>
#include <stdio.h>
/*
	main:
	    global shared int i = 0
	    spawn thread_1
	    spawn thread_2
	    join all threads
	    print i

	thread_1:
	    do 1_000_000 times:
		i++
	thread_2:
	    do 1_000_000 times:
		i--
*/
long int g_i = 0;
// Note the return type: void*
void* thread_1(){
    long i;
	for( i = 0; i < 1000000; i++)
		g_i++;
    return NULL;
}
void* thread_2(){
    long i;
	for( i = 0; i < 1000000; i++)
		g_i--;
    return NULL;
}



int main(){
    pthread_t td[2];
    pthread_create(&td[0], NULL, thread_1, NULL);
	pthread_create(&td[1], NULL, thread_2, NULL);
    // Arguments to a thread would be passed here ---------^
    
    pthread_join(td[0], NULL);
	pthread_join(td[1], NULL);
    printf("Hello from main! the g_i is %ld\n", g_i);
    return 0;
    
}
