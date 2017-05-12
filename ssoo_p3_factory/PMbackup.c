/*
 *
 * process_manager.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <pthread.h>
#include "queue.h"
#include <semaphore.h>

#define NUM_THREADS 2

pthread_mutex_t mutex;	// mutex to access shared buffer
pthread_cond_t non_full; // can we add more elements?
pthread_cond_t non_empty; // can we remove elements?
int	elemsInQueue; // number of elements in the queue
int maxSize;	// Max size of the belt
int countElems;	 // Number of products to be generated
int id;	// ID of the assigned belt

void argsNotValid(){ // Print error message
	fprintf(stderr, "[ERROR][process_manager] Arguments not valid.\n");
}

void errorInPM(int id){ // Print error message
	fprintf(stderr,"[ERROR][process_manager] There was an error executing process_manager with id: %d.\n", id);
}

int checkArgs(int i, const char * semName, const char * number){
	if(i == 2){ // Expect string for name of semapore, not a number
		if(semName[0] == '\0'){ // The string is empty
			return 0;
		}
	}
	else{ // Expect a number
		return isNumber(number); // Check if the number is valid
	}
}

int isNumber(const char arg[]){ // Check if a given argument is a positive number
    int i = 0;
    // Check for negative numbers
    if (arg[1] == '-'){
		return 0;
	}
    for (i; arg[i] != 0; i++){ //Loop through each char in string and check if it is a number
        if (!isdigit(arg[i])){
            return 0;
		}
    }
	return 1;
}

// Thread functions
// Producer produces an element and inserts it in the belt until all specified elements have been produced
void * producer(void *threadid){
	int i = 0;
	int elemsProduced = 0;
	for(i = 0; i < countElems; i++){ // Produce as many elements as indicated
		// Create the element
		struct element * elem;
		elem->num_edition = i;
		elem->id_belt = id;
		if(i + 1 == countElems){ // If the current element is the last to be produced
			elem->last = 1;
		}
		else{
			elem->last = 0;
		}
		pthread_mutex_lock(&mutex); // Access to buffer, critical section
		while(queue_full()){ // When buffer is full
			pthread_cond_wait(&non_full, &mutex); // Wait until consumer signals that it is not full
			pthread_cond_signal(&non_empty); // Signal that the queue is not empty
		}
		queue_put(elem); // Put the element on the queue
		if(elem->last){ // If we have created as many elements as requested
			pthread_cond_wait(&non_full, &mutex); // Wait until not full?
		}
		pthread_cond_signal(&non_empty); // Signal that the buffer is not empty
		pthread_mutex_unlock(&mutex); // Exit from critical section
	}
   	long tid = pthread_self();
   	//printf("Hello World! It's me, thread #%ld!\n", tid);
	printf("Thread #%ld producer ends\n", tid);
   	return NULL;
}

// Consumer will obtain an element from the belt until there exist no more elements (last==1)
void * consumer(void *threadid){
	struct element * elem;
	int i;
	 do{
		pthread_mutex_lock(&mutex); // Access to buffer, critical section
		while(queue_empty()){ // When buffer is empty
			pthread_cond_wait(&non_empty, &mutex); // Wait until consumer signals that it is not empty
		}
		elem = queue_get(); // Get the element from the start of the queue
		if(elem == NULL){
			printf("Failed to get the element from the queue\n");
		}
		else if(elem->last == 1){
			printf("[OK][process_manager] Process_manager with id: %d has produced %d elements.\n", id, countElems);
		}
		pthread_cond_signal(&non_full); // Signal that the buffer is not full
		pthread_mutex_unlock(&mutex); // Exit from critical section
	} while(elem->last == 0);
	long tid = pthread_self();
   	//printf("Hello World! It's me, thread #%ld!\n", tid);
	printf("Thread #%ld consumer ends\n", tid);
   	return NULL;
}

int main (int argc, const char * argv[] ){
	int i;
	const char * semName; // Name of the semaphore
	int boolean; // Variable to check for errors
	int value; // Contains the value of a semaphore
	sem_t *sem;
	void *end;

	// PM Acquires the input parameters.
	// Convert arguments to the correct integer values
	id = *argv[0] - '0';
	semName = argv[1];
	maxSize = *argv[2] - '0';
	countElems = *argv[3] - '0';
	
	if(maxSize < 0 || id < 0 || countElems < 0){ // Any argument is negative
		argsNotValid();
		return -1;
	} 
	
	boolean = 1; // Check if arguments are numbers
	for(i = 0; i < argc - 1; i+=2){
		boolean = checkArgs(i, semName, argv[i]);
		if(!boolean){ // Arguments are not valid
			argsNotValid();
			return -1;
		}
	}
	
	
	if ( (sem = sem_open(semName, 0)) == SEM_FAILED){ // Open the semaphore created in FM
		printf("Process_manager failed to open semaphore: %s\n", semName);
	}

	// Wait until the factory_manager indicates that PM must start to produce.
	printf("[OK][process_manager] Process_manager with id: %d waiting to produce %d elements.\n", id, countElems);

	//Start producing
	sem_wait(sem); // Request access to begin working // Critical Section begins
	// Create and initialize the belt.
	boolean = queue_init(maxSize); // Initialize a queue for the producer-consumer relationships
	if(boolean != 0){ // Initializing queue returns 0 on success
		errorInPM(id);
		return -1;
	}
	printf("[OK][process_manager] Belt with id: %d has been created with a maximum of %d elements.\n", id, maxSize);
	sem_post(sem);
	
	// Create two lightweight processes producer and consumer
	pthread_t t_producer, t_consumer;
	
	// Create threads
	if(pthread_create(&t_producer, NULL, (void*)producer, pthread_self)){
		printf("Failed to create the producer thread\n");
		fflush(stdout);
	}
	if(pthread_create(&t_consumer, NULL, (void*)consumer, pthread_self)){
		printf("Failed to create the consumer thread\n");
		fflush(stdout);
	}
	
	// Block for thread termination
	// When they have finished producing and consuming they will finish.
	
	pthread_join(t_consumer, &end);
	printf("Join Consumer\n"); 
	pthread_join(t_producer, &end);
	printf("Join Producer\n"); 
	
	// Free reasources
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&non_full);
	pthread_cond_destroy(&non_empty);
	boolean = queue_destroy();
	if (boolean != 0){ // Destroying queue returns 0 on success
		errorInPM(id);
		return -1;
	}
	
	//need to remove this post bc it did not register earlier when it worked fine
	//sem_post(sem); // Signal that PM is done working // Critical Section ends
	
	return 0;
}
