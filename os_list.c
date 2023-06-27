// SPDX-License-Identifier: BSD-3-Clause

#include "os_list.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

os_queue_t *queue_create(void)
{
	os_queue_t *newQueue;

	newQueue = malloc(sizeof(os_queue_t));
	if (newQueue == NULL) {
		perror("malloc");
		return NULL;
	}
	pthread_mutex_init(&newQueue->lock, NULL);
	newQueue->first =  NULL;
	newQueue->last = NULL;

	return newQueue;
}

void queue_add(os_queue_t *queue, void *info)
{
	os_list_node_t *newNode;

	newNode = malloc(sizeof(os_list_node_t));
	if (newNode == NULL) {
		perror("malloc");
		return;
	}

	newNode->next = NULL;
	newNode->info = info;

	if (queue->last == NULL && queue->first == NULL) {
		queue->first = newNode;
		queue->last = newNode;
	} else {
		queue->last->next = newNode;
		queue->last = newNode;
	}
}

os_list_node_t *queue_get(os_queue_t *queue)
{
	os_list_node_t *target;

	if (queue == NULL)
		return NULL;
	if (queue->first == NULL && queue->last == NULL)
		return NULL;

	target = queue->first;
	queue->first = queue->first->next;

	return target;
}
