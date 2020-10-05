/* Fill in your Name and GNumber in the following two comment fields
 * Name: Jaime Botero Martinez
 * GNumber:01014336
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "structs.h"
#include "constants.h"
#include "scheduler.h"

/*  Init function needs to create a Schedule struct that is fully allocated (malloc). All
	allocations within this struct will be dynamic, also using malloc. Take note that Schedule
	contains pointers to Queue structs, which themselves also need to be allocated.
	Each Queue struct contains a pointer to the head of a singly linked list and a count of
	the number of items in that queue, which must be initialized to 0. All head pointers
	must be initialized to NULL.
	On any errors, return NULL, otherwise return your new Schedule.*/
Schedule *scheduler_init() {
	//Allocates memory to the Schedule
	Schedule *cpuSchedule  = malloc(sizeof(Schedule));
	if(cpuSchedule == NULL){
		return NULL;
	}

	cpuSchedule->ready_queue = malloc(sizeof(Queue));
	cpuSchedule->stopped_queue = malloc(sizeof(Queue));
	cpuSchedule->defunct_queue = malloc(sizeof(Queue));

	Queue *ready_queue = cpuSchedule->ready_queue;
	Queue *stopped_queue = cpuSchedule->stopped_queue;
	Queue *defunct_queue = cpuSchedule->defunct_queue;

	//queue heads are set to null and counts are set to 0
	ready_queue->head = NULL;
	ready_queue->count = 0;

	stopped_queue->head = NULL;
	stopped_queue->count = 0;

	defunct_queue->head = NULL;
	defunct_queue->count = 0;

  	return cpuSchedule;//returns Schedule
}//initialize

void insert_at_front(Queue *queue, Process *process){
	process->next = queue->head;//points the process to the original head process of the queue
	queue->head = process;//sets the process as the new head of the queue
	queue->count++;
}//function to add processes to the queue at the front

//function to swap what queue a process is in
int swapQueues(Process *current, Schedule *schedule, int turnedOff, int turnedOn, int type){
	//printf("We're inside the swap queues funcition........\n\n");

	current->flags ^= 1 << turnedOff;//toggles the previous flag off
	current->flags ^= 1 << turnedOn;//toggles the new flag on
	switch(type){
		case 0://ready to stopped
			schedule->ready_queue->count--;
			current->next = schedule->stopped_queue->head;//points next of the process being swapped to the head of the target stopped queue
			schedule->stopped_queue->head = current;//sets the process to the head of the stopped queue
			schedule->stopped_queue->count++;//increments the count of the stopped queue

			return 0;//returns 0 to signal a successful operation
		case 1://stopped to ready
			schedule->stopped_queue->count--;
			current->next = schedule->ready_queue->head;//points next of the process being swapped to the head of the ready queue
			schedule->ready_queue->head = current;//sets the process to the head of the ready queue
			schedule->ready_queue->count++;//increments the count of the ready queue

			return 0;//returns 0 to signal a successful operation
	}

	return -1;
	
}

/* Add the process into the appropriate linked list.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int scheduler_add(Schedule *schedule, Process *process) {
	//printf("We're inside the add funcition........\n\n");

	if(schedule == NULL || process == NULL){//prevents SEGFAULT, check pointers
		return -1;
	}

	int retrievedFlag = 0;//starts at 0 by default, changes to 1 when flag is found
	int i = 27;//index for bitwise operation
	while(retrievedFlag == 0){//searches for the flag and its position
		retrievedFlag = (process->flags >> i++) & 1;
	}

	switch(i){//i = bit position of flag
		case 31://created
			process->flags &= ~(1 << 30); //resets the created flag to 0
			process->flags ^= 1 << 29; //toggles the ready flag to 1
			insert_at_front(schedule->ready_queue, process);//inserts to ready queue
			return 0;

		case 30://ready
			if(process->time_remaining == 0){//the process has no more time remaining
				process->flags &= ~(1 << 29); //resets the ready flag to 0
				process->flags ^= 1 << 27; //toggles the defunct flag to 1

				insert_at_front(schedule->defunct_queue, process);//inserts to defunct queue
				return 0;
			}

			//more time is left for the process
			insert_at_front(schedule->ready_queue, process);//inserts to ready queue
			return 0;

		case 28://defunct
			insert_at_front(schedule->defunct_queue, process);//inserts to defunct queue
				return 0;

		default://all other flags
			return -1;
	}
}//add

/* This needs to find the process with matching pid from the Ready Queue.
 Once found, remove the process from the Ready Queue.
 Set the process’ state to STOPPED
 Insert that process into the front of the Stopped Queue.*/
int scheduler_stop(Schedule *schedule, int pid) {
	if(schedule == NULL){//checks schedule pointer to prevent SEGFAULT
  		return -1;
	}

	Process *previous = NULL;//holds the previous process
	Process *current = schedule->ready_queue->head;//holds the target process

	if(current == NULL){//Checks the Process pointer to prevent SEGFAULT
		return -1;
	}

	if(schedule->ready_queue->count == 1){
		schedule->ready_queue->head = NULL;
		return swapQueues(current, schedule, 29, 28, 0);
	}

	for(int i = 0; i < schedule->ready_queue->count; i++){//goes through the queue
		if(current->pid == pid){
			if(i == 0){//checks if target process is the current head of a queue
				schedule->ready_queue->head = current->next;//sets the head of the queue to the process following the current process
				return swapQueues(current, schedule, 29, 28, 0);
			}

			previous->next = current->next;//links the previous process to the process following the current one
			return swapQueues(current, schedule, 29, 28, 0);
		}

		previous = current;
		current = current->next;
	}

	return -1;
}//stop

/* Move the process with matching pid from Stopped to Ready.
 * Change its State to Ready.
 * Follow the specification for this function.
 * Returns a 0 on success or a -1 on any error.
 */
int scheduler_continue(Schedule *schedule, int pid) {
	if(schedule == NULL){//checks schedule pointer to prevent SEGFAULT
  		return -1;
	}

	Process *previous = schedule->stopped_queue->head;//holds the previous process
	Process *current = schedule->stopped_queue->head;//holds the target process

	if(previous == NULL || current == NULL){//Checks the Process pointer to prevent SEGFAULT
		return -1;
	}

	for(int i = 0; i < schedule->stopped_queue->count; i++){//goes through the queue
		if(current->pid == pid){
			if(i == 0){//checks if target process is the current head of a queue
				schedule->stopped_queue->head = current->next;//sets the head of the queue to the process following the current process
				return swapQueues(previous, schedule, 28, 29, 1);
			}

			previous->next = current->next;//links the previous process to the process following the current one
			return swapQueues(previous, schedule, 28, 29, 1);
		}

		previous = current;
		current = current->next;
	}

	return -1;
}//continue

/* Remove the process with matching pid from Defunct.
 * Follow the specification for this function.
 * Returns its exit code (from flags) on success or a -1 on any error.
 */
int scheduler_reap(Schedule *schedule, int pid) {
	if(schedule == NULL){//checks schedule pointer to prevent SEGFAULT
  		return -1;
	}

	Process *previous = schedule->defunct_queue->head;//holds the previous process
	Process *current = schedule->defunct_queue->head;//holds the target process

	if(previous == NULL || current == NULL){//Checks the Process pointer to prevent SEGFAULT
		free(current);
		return -1;
	}

	for(int i = 0; i < schedule->defunct_queue->count; i++){
		if(current->pid == pid){
			if(i == 0){
				schedule->defunct_queue->head = current->next;//assigns new head for the queue
			}

			schedule->defunct_queue->count--;//subtracts the count of the defunct queue

			int focusExitCode = 67108863;//equivalent to 00000011111111111111111111111111 in Binary, used to clear all flags and return only the exit code
			int exitCode;//holds the exit code for return

			//clears the defunct flag and sets the process flag to terminated
			current->flags ^= 1 << 27;//toggles the defunct flag to 0
			current->flags ^= 1 << 26;//toggles the terminated flag to 1

			exitCode = (current->flags &= focusExitCode);

			free(current);//deallocates memory
			return exitCode;//returns the exit code
		}

		previous = current;
		current = current->next;
	}

	return -1;
}//remove

/* Create a new Process with the given information.
 * - Malloc and copy the command string, don't just assign it!
 * Set the STATE_CREATED flag.
 * If is_sudo, also set the PF_SUPERPRIV flag.
 * Follow the specification for this function.
 * Returns the Process on success or a NULL on any error.
 */
Process *scheduler_generate(char *command, int pid, int base_priority, int time_remaining, int is_sudo) {
	if(command == NULL){//checks if command is null, prevents SEGFAULT
		return NULL;
	}

	//Creates a new process and initializes its values to those passed into the function
	Process *newProcess = malloc(sizeof(Process));
	newProcess->command = malloc(sizeof(command));
	strcpy(newProcess->command, command);//copies the command string into the new pointer
	newProcess->pid = pid;
	newProcess->base_priority = base_priority;
	newProcess->cur_priority = base_priority;//current priority is set to passed based priority
	newProcess->time_remaining = time_remaining;
	
	//checks if the process is sudo, toggles sudo bit in flags accordingly
	if(is_sudo == 1){
		newProcess->flags ^= 1 << 31; //toggles the SUDO flag to 1
	}

	for(int i = 30; i > 24; i-- ){//sets CREATED state to 1, and all other state bits to 0. Sets first bit of the exit code flags to 0
		switch(i){
			case 30:
				newProcess->flags ^= 1 << i;
				break;

			default:
				newProcess->flags ^= 0 << i;
				break;
		}

	}

	return newProcess;

}//generate

/* Select the next process to run from Ready Queue.
 * Follow the specification for this function.
 * Returns the process selected or NULL if none available or on any errors.
 */
Process *scheduler_select(Schedule *schedule) {
	if(schedule == NULL){
		return NULL;
	}//Prevents SEGFAULT from a null schedule

	Queue *readyQueue = schedule->ready_queue;

	if(readyQueue == NULL || scheduler_count(readyQueue) == 0){
		return NULL;
	}//prevents SEGFAULT from a null queue;

	Process *beforeBest = NULL;
	Process *previousInQueue = readyQueue->head;
	Process *currentBest = readyQueue->head;
	Process *currentInQueue = currentBest->next;

	if(readyQueue->count == 1){
		readyQueue->count--;
		schedule->ready_queue->head = NULL;
		currentBest->cur_priority = currentBest->base_priority;

		return currentBest;
	}

	for(int i = 0; i < readyQueue->count - 1; i++){
		if(currentBest->cur_priority <= currentInQueue->cur_priority){//compares the priority value for the best process and the ones following it, traverses the queue
			previousInQueue = currentInQueue;//stores the current process in order to help maintain the queue should the next process be removed

			if(currentInQueue->cur_priority != 0){//prevents value from going any lower than 0
				currentInQueue->cur_priority--;//increases the priority of the incorrect process by moving it closer to 0
			}
			currentInQueue = currentInQueue->next;

			if(currentInQueue == NULL){//we've reached the end of the queue, break out of the loop
				break;
			}
		}
		else{//If a higher priority process is found, it is set to the current best process
			beforeBest = previousInQueue;
			previousInQueue = currentInQueue;
			currentBest->cur_priority--;//priority of the previous best process is increased (brought closer to 0) for next time
			currentBest = currentInQueue;
			currentInQueue = currentInQueue->next;
		}
	}

	if(beforeBest == NULL){//only triggers when the best in the queue is the head process of a 1 count queueu
		readyQueue->head = currentBest->next;//assigns the head of the queue to the process following the previous head
		currentBest->next = NULL;
		readyQueue->count--;
		currentBest->cur_priority = currentBest->base_priority;

		return currentBest;
	}

	beforeBest->next = currentBest->next;//maintains queue order
	currentBest->next = NULL;//removes highest priority process from queue
	currentBest->cur_priority = currentBest->base_priority;
	readyQueue->count--;
  	return currentBest;
}//select

/* Returns the number of items in a given Linked List (Queue) (Queue)
 * Follow the specification for this function.
 * Returns the count of the Linked List, or -1 on any errors.
 */
int scheduler_count(Queue *ll) {
	if(ll != NULL){//checks to make sure the Queue isn't null, no errors -> return count
		return ll->count;
	}
  	return -1;
}//count

/* Completely frees all allocated memory in the scheduler
 * Follow the specification for this function.
 */
void scheduler_free(Schedule *scheduler) {
	//all queue memory allocations inside the schedule and queues are freed before the schedule memory allocation is freed
	Process *current = scheduler->ready_queue->head;
	while(scheduler->ready_queue->count != 0){
		free(current->command);
		free(current);
		current = current->next;
		scheduler->ready_queue->count--;
	}
	free(scheduler->ready_queue);

	current = scheduler->stopped_queue->head;
	while(scheduler->stopped_queue->count != 0){
		free(current->command);
		free(current);
		current = current->next;
		scheduler->stopped_queue->count--;
	}
	free(scheduler->stopped_queue);

	current = scheduler->defunct_queue->head;
	while(scheduler->defunct_queue->count != 0){
		free(current->command);
		free(current);
		current = current->next;
		scheduler->defunct_queue->count--;
	}

	free(scheduler->defunct_queue);
	free(scheduler);
  	return;
}//free
