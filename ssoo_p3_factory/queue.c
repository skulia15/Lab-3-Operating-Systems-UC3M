#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "queue.h"

struct element * theQueue;
int itemsInQueue;
int maxInQueue;
int i;

// To create a queue
int queue_init(int size){
	// function that creates the queue and reserves	memory for the size specified as parameter.
	theQueue = malloc(size * sizeof(struct element)); // Allocate memory for the queue. 
	maxInQueue = size; // Size of the queue must never exceed this value
	if(theQueue == NULL){ // Failed to allocate memory for the queue
	 	// Cant print the usual error message because I dont have the ID of the queue
		fprintf(stderr, "[ERROR][queue] There was an error while using queue with id: <id>.\n");
		return -1;
	}
	itemsInQueue = 0;
	return 0;
}

// To Enqueue an element
int queue_put(struct element* x) {
	if(queue_full()){ // Cannot put element if the queue is full
		fprintf(stderr, "[ERROR][queue] There was an error while using queue with id: %d.\n", x->id_belt);
		return -1;
	}
	itemsInQueue++;
	theQueue[itemsInQueue] = *x; //put the element at the back of the queue
	printf("[OK][queue] Introduced element with id: %d in belt %d.\n", x->num_edition, x->id_belt);
	return 0;
}

// To Dequeue an element.
struct element* queue_get(void) {
	int idbelt; // Contains the ID of the belt/queue
	if(!queue_empty()){
		struct element* elem = &theQueue[0];
		// Move all elements to the prior position
		for(i = 0; i < itemsInQueue; i++){
			theQueue[i] = theQueue[i + 1];
		}
		itemsInQueue--;
		idbelt = elem->id_belt;
		printf("[OK][queue] Obtained element with id: %d in belt %d\n", elem->num_edition, elem->id_belt);
		return elem;
	}
	// Cannot retrieve en element from an empty queue
	fprintf(stderr,"[ERROR][queue] There was an error while using queue with id: %d.\n", idbelt);
	return NULL;
}

// To check queue state
int queue_empty(void){
	return (itemsInQueue == 0);
}

int queue_full(void){
	return (itemsInQueue == maxInQueue);
}

// To destroy the queue and free the resources
int queue_destroy(void){
	free(theQueue); // Free memory allocated for the queue
	return 0;
}
