/*
 *
 * factory_manager.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>

#define BUFF_SIZE 2048
#define semaphoreName "/semaphore_1"

sem_t *sem; // Semaphore that is passed to all child processes
int length; // The length of the file in bytes
char * buffer; // Buffer to store the contents of the file

void invalidFileErr(){ // Print error message
	fprintf(stderr, "[ERROR][factory_manager] Invalid file\n");
}

int handleInput(const char * filePath){ // Factory Manager opens the file, acquire the information and closes the file.
	int fd; // The file descriptor for opening the input file
	if ( (fd = open(filePath, O_RDONLY)) < 0 ) { // Input read incorrectly
		return -1;
	}
	else{ // Read the contents of the file into a buffer
		length = lseek(fd, 0, SEEK_END); // Number of characters in file
		lseek(fd, 0, SEEK_SET); // Set file pointer to beginning
		if(length == 0){ // Did not read the length correctly
			return -1;
		}
		buffer = malloc (length); // Allocate the memory for the buffer
		if(!buffer){ // Memory allocation failed
			return -1;
		}
		if(read(fd, buffer, length) < 0){ // Read the data to the buffer
        	return -1;
    	}
		if (close(fd) < 0){ // We are done with the file, close the file descriptors
        	return -1;
		}
		return 0;
	}
}

int main (int argc, const char * argv[] ){
	pid_t pid; // Process ID
	int maxPM; // Maximum number of Process managers
	int status; //For getting exit status when waiting for child process

	int id;	// ID of the assigned belt
	int maxSize;	// Max size of the belt
	int countElems;	 // Number of products to be generated
	
	int value; // The value of the semaphore
	int PMsRunning = 0; // The number of active Process managers

	// Factory Manager reads the path of the file with the parameters.
	if(argc <= 1 || argv[1] == NULL || argv[1] == '\0'){
		// Error input file is not given as argument or input file is empty
		invalidFileErr();
		return -1;
	}
	
	int boolean = 1; // Variable to check for errors
	int i;

	// Factory Manager opens the file, acquire the information and closes the file.
	const char * filePath = argv[1];
	boolean = handleInput(filePath);
	if(boolean < 0){
		invalidFileErr();
		return -1;
	}
	

	// Parse the input file
	maxPM = buffer[0]; // Max Process managers equals the first number in file
	int neededPMs; // Number of Process managers needed
	int assertCountArgs = (length + 1) % 6; // If there are not 3 numbers for every PM, the input file is invalid
	if(assertCountArgs != 2){ // The input file is not in a valid format
		invalidFileErr();
		return -1;
	}
	for(i = 2; i < length + 1; i+=2){
		if(i % 6 == 0){ // For every 3 numbers, we need another process manager
			neededPMs++;
		}
	}
	if(neededPMs > maxPM){ // More process_manager processes are needed than the maximum specified
		invalidFileErr();
		return -1; // File is invalid
	}

	//Creation of the synchronization structures necessary for the correct operation of the factory (check semaphores).
	if( (sem = sem_open(semaphoreName, O_CREAT, 0644, 0) ) == (sem_t *) - 1){ // Open the semaphore with init value of 0	
		perror("Could not create the semaphore\n");
		exit(1); 
	}

	// Creation of all the process_manager processes.
	for(i = 2; i < length + 1; i += 6){ // Create as many children that execute as a PMs as needed
		// Get the arguments for the PM
		id = buffer[i];
		maxSize = buffer[i + 2]; 
		countElems = buffer[i + 4];

		pid = fork(); // Create a child process that will become a process manager
		if(pid == 0){ // If the process is a child
			break; // Variables are correctly set for the child process i
		}
	}
	if (pid == 0) { // Child process
		printf("[OK][factory_manager] Process_manager with id %d has been created.\n", id - '0');
		PMsRunning = (i / 6 + 1); // A Counter for running PMs
		if(PMsRunning == neededPMs){ // When all PMs have been created, signal to start producing
			sem_post(sem);
			sem_getvalue(sem, &value);
			printf("All PMs created, value of sem is: %d\n", value);
		}
		char argId[10], argMaxSize[10], argCountElems[10], argSemName[PATH_MAX]; // Convert to string for the parameter passing
		snprintf(argId, sizeof(argId), "%c", id);
		snprintf(argMaxSize, sizeof(argMaxSize), "%c", maxSize);
		snprintf(argCountElems, sizeof(argCountElems), "%c", countElems);
		snprintf(argSemName, sizeof(argSemName), "%s", semaphoreName);

		const char *my_argv[64] = {"process" , argId, argSemName, argMaxSize, argCountElems, NULL}; // The arguments (argv) for the PM
		//printf ("!Child(%d) is in critical section.!\n", (i / 6 + 1));
		execl(my_argv[0], my_argv[1], my_argv[2], my_argv[3], my_argv[4], NULL); // Execute the Process Manager
		printf("Failed to execute PM\n"); // If we get to this point the exec failed.
		exit(EXIT_FAILURE);
	}
	else if(pid == -1){
		printf("Error in fork()\n");
		return -1;
	}
	else{	// Parent process
		// Wait for children to finish
		while (wait(&status) != pid){
			if (status != 0){
				fprintf(stderr, "[ERROR][factory_manager] Process_manager with id %d has finished with errors %d.\n", id - '0', status);
			}
			else{
				printf("[OK][factory_manager] Process_manager with id %d has finished.\n", id - '0');
			}
		}
	} 
	
	// When all process_manager processes have finished, free the resources and end the program
	// Free buffers
	free(buffer);
	// Close semaphores
	sem_close(sem);
	sem_unlink(semaphoreName);

	printf("[OK][factory_manager] Finishing.\n"); //If Factory finishes correctly
	return 0;
}
