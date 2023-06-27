// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "os_list.h"

#define MAX_TASK 1000
#define MAX_THREAD 4

static int sum;
static unsigned int remaining_nodes;
static os_graph_t *graph;
static os_threadpool_t *threadpool;

// Vector of mutexes for:
	// mutexes[0] -> checking the neighbours of a node because maybe more nodes have the same neighbour
				  // and we don't want to add the same task multiple time if two nodes check the same
				  // neighbour in the same time
	// mutexes[1] -> the sum mutex. We change the `sum` global variable and we don't want it to be written
				  // by two threads at the same time because the values can be wrong
	// mutexes[2] -> the remaining_nodes mutex. We change the `remaining_nodes` global variable and we don't
				  // want it to be written by two threads at the same time because the values can be wrong
pthread_mutex_t mutexes[3];

void free_memory(FILE *input_file)
{
	free(graph->visited);
	for (int i = 0; i < graph->nCount; i++) {
		free(graph->nodes[i]->neighbours);
		free(graph->nodes[i]);
	}

	free(graph->nodes);
	free(graph);

	for (int i = 0; i < 3; i++)
		pthread_mutex_destroy(&mutexes[i]);

	fclose(input_file);
}

void task_function(void *arg)
{
	os_node_t *current_node = (os_node_t *) arg;

	int current_value = current_node->nodeInfo;

	// We add the current node value to the overall sum
	pthread_mutex_lock(&mutexes[1]);
	sum = sum + current_value;
	pthread_mutex_unlock(&mutexes[1]);

	// We traverse each of its neighbours and if there are any neighbours that have not been visited
	// we create a new task for them and add it in the queue.
	for (int i = 0; i < current_node->cNeighbours; i++) {
		pthread_mutex_lock(&mutexes[0]);
		if (graph->visited[current_node->neighbours[i]] == 0) {
			graph->visited[current_node->neighbours[i]] = 1;
			pthread_mutex_unlock(&mutexes[0]);

			os_task_t *new_task = task_create((void *) graph->nodes[current_node->neighbours[i]], task_function);

			add_task_in_queue(threadpool, new_task);
		} else {
			pthread_mutex_unlock(&mutexes[0]);
		}
	}

	// Each time we finish a task it means that we finished one more node because we create one task/node.
	// When there are no more nodes left it means that we finished the problem so we can stop the threadpool.
	pthread_mutex_lock(&mutexes[2]);
	remaining_nodes--;
	pthread_mutex_unlock(&mutexes[2]);
}

void traverse_graph(void)
{
	// The main thread traverses all the nodes in the graph and if he finds any node that has not
	// been visited he creates a task for it and adds it in the queue of tasks.

	// We have to do that because we cannot be sure that the graph is connected or not so we cannot do
	// a classic BFS. Otherwise we have had to determine the connected components for the graph and then
	// add one node for each component but that would have been even slower than doing a task for each
	// node that has not been visited yet.
	for (int i = 0; i < graph->nCount; i++) {
		pthread_mutex_lock(&mutexes[0]);

		if (graph->visited[i] == 0) {
			graph->visited[i] = 1;
			pthread_mutex_unlock(&mutexes[0]);

			os_task_t *new_task = task_create((void *) graph->nodes[i], task_function);

			add_task_in_queue(threadpool, new_task);
		} else {
			pthread_mutex_unlock(&mutexes[0]);
		}
	}
}

int processingIsDone(os_threadpool_t *tp)
{
	// If there are no more nodes to be taken into consideration from the graph we just
	// return 1 in order for the main thread to know that he can join the other threads
	if (remaining_nodes == 0)
		return 1;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("%s\n", "Usage: ./main input_file");
		exit(1);
	}

	FILE *input_file = fopen(argv[1], "r");

	if (input_file == NULL) {
		printf("[Error] Can't open file\n");
		return -1;
	}

	graph = create_graph_from_file(input_file);
	if (graph == NULL) {
		printf("[Error] Can't read the graph from file\n");
		return -1;
	}

	remaining_nodes = graph->nCount;
	for (int i = 0; i < 3; i++)
		pthread_mutex_init(&mutexes[i], NULL);

	threadpool = threadpool_create(MAX_TASK, MAX_THREAD);

	traverse_graph();
	threadpool_stop(threadpool, processingIsDone);

	printf("%d", sum);
	free_memory(input_file);
	return 0;
}
