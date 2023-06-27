// SPDX-License-Identifier: BSD-3-Clause

#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static int max_tasks;

/* Checks if the malloc for an address failed or not */
void memory_check(void *address)
{
	if (address == NULL) {
		fprintf(stderr, "%s\n", "Error at allocating memory");
		exit(1);
	}
}

/* Checks if the pthread_create or pthread_join succeded */
void pthread_check(int return_value)
{
	if (return_value != 0) {
		fprintf(stderr, "%s\n", "Error at creating or joining the threads");
		exit(1);
	}
}

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
	os_task_t *new_task = malloc(sizeof(*new_task));

	memory_check((void *) new_task);

	new_task->argument = arg;
	new_task->task = f;

	return new_task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
	// We wait until we have less than max_tasks in the queue before we
	// add a new one
	pthread_mutex_lock(&tp->taskLock);
	while (1) {
		if (max_tasks > 0) {
			max_tasks--;
			break;
		}
	}
	pthread_mutex_unlock(&tp->taskLock);

	// If we have space in the queue, we create the new element for the queue
	os_task_queue_t *new_task_queue = malloc(sizeof(*new_task_queue));

	memory_check(new_task_queue);

	new_task_queue->task = t;
	new_task_queue->next = NULL;

	// If the queue is null, we add it as the first element in the queue
	pthread_mutex_lock(&tp->taskLock);
	if (tp->tasks == NULL) {
		tp->tasks = new_task_queue;
		pthread_mutex_unlock(&tp->taskLock);
		return;
	}

	// Otherwise, we iterate until the last element in the queue and add it at the end
	os_task_queue_t *head = tp->tasks;

	while (head->next != NULL)
		head = head->next;

	head->next = new_task_queue;
	pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
	// If the queue is null we have nothing to return from it so we return null
	pthread_mutex_lock(&tp->taskLock);
	if (tp->tasks == NULL) {
		pthread_mutex_unlock(&tp->taskLock);
		return NULL;
	}

	// Otherwise we take the first task in the queue and update the head of the queue
	os_task_queue_t *head = tp->tasks;
	os_task_t *current_task = head->task;

	tp->tasks = tp->tasks->next;
	max_tasks++;

	pthread_mutex_unlock(&tp->taskLock);
	free(head);
	return current_task;
}

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
	max_tasks = nTasks;

	os_threadpool_t *new_threadpool = malloc(sizeof(*new_threadpool));

	memory_check(new_threadpool);

	new_threadpool->tasks = NULL;
	new_threadpool->should_stop = 0;
	new_threadpool->num_threads = nThreads;
	pthread_mutex_init(&new_threadpool->taskLock, NULL);

	new_threadpool->threads = malloc(nThreads * sizeof(*(new_threadpool->threads)));
	memory_check(new_threadpool->threads);

	int return_value;

	for (int i = 0; i < nThreads; i++) {
		return_value = pthread_create(&new_threadpool->threads[i], NULL,
									  thread_loop_function, new_threadpool);

		pthread_check(return_value);
	}

	return new_threadpool;
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
	os_threadpool_t *threadpool = (os_threadpool_t *) args;
	os_task_t *current_task = NULL;

	// Each thread iterates in a true while until they find a task in the queue so they can take
	// it or when they get a signal that they should stop their execution
	while (1) {
		if (threadpool->should_stop == 1)
			return NULL;

		current_task = get_task(threadpool);

		if (current_task != NULL) {
			current_task->task(current_task->argument);
			free(current_task);
		}
	}

	return NULL;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
	// The main thread waits untill processingIsDone method signals that the process is over
	// so he can signal the other threads that they should stop by setting the should_stop field
	// and then wait for the other threads to finish their process so he can join them
	while (1) {
		if (processingIsDone(tp) == 1) {
			tp->should_stop = 1;
			break;
		}
	}

	int return_value;
	void *status;

	for (int i = 0; i < tp->num_threads; i++) {
		return_value = pthread_join(tp->threads[i], &status);
		pthread_check(return_value);
	}

	pthread_mutex_destroy(&tp->taskLock);
	free(tp->threads);
	free(tp);
}
