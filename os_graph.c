// SPDX-License-Identifier: BSD-3-Clause

#include "os_graph.h"
#include <stdio.h>
#include <stdlib.h>

/*             [ ==== NODE FUNCTIONS === ]               */
os_node_t *os_create_node(unsigned int _nodeID, int _nodeInfo)
{
	os_node_t *newNode;

	newNode = calloc(1, sizeof(os_node_t));

	newNode->nodeID = _nodeID;
	newNode->nodeInfo = _nodeInfo;

	return newNode;
}

/*             [ ==== GRAPH FUNCTIONS === ]              */
os_graph_t *create_graph_from_data(unsigned int nc, unsigned int ec,
		int *values, os_edge_t *edges)
{
	int i, isrc, idst;
	os_graph_t *graph;

	graph = calloc(1, sizeof(os_graph_t));

	graph->nCount = nc;
	graph->eCount = ec;

	graph->nodes = calloc(nc, sizeof(os_node_t *));

	for (i = 0; i < nc; ++i) {
		graph->nodes[i] = os_create_node(i, values[i]);
		graph->nodes[i]->neighbours = calloc(nc, sizeof(unsigned int));
		graph->nodes[i]->cNeighbours = 0;
	}

	for (i = 0; i < ec; ++i) {
		isrc = edges[i].src; idst = edges[i].dst;

		graph->nodes[isrc]->neighbours[graph->nodes[isrc]->cNeighbours++] = idst;

		graph->nodes[idst]->neighbours[graph->nodes[idst]->cNeighbours++] = isrc;
	}

	graph->visited = calloc(graph->nCount, sizeof(unsigned int));

	return graph;
}

os_graph_t *create_graph_from_file(FILE *file)
{
	unsigned int nCount, eCount;
	int i;
	int *values;
	os_edge_t *edges;
	os_graph_t *graph;

	if (fscanf(file, "%d %d", &nCount, &eCount) == 0) {
		fprintf(stderr, "[ERROR] Can't read from file\n");
		return NULL;
	}

	values = malloc(nCount * sizeof(int));
	for (i = 0; i < nCount; ++i) {
		if (fscanf(file, "%d", &values[i]) == 0) {
			fprintf(stderr, "[ERROR] Can't read from file\n");
			return NULL;
		}
	}

	edges = malloc(eCount * sizeof(os_edge_t));
	for (i = 0; i < eCount; ++i) {
		if (fscanf(file, "%d %d", &edges[i].src, &edges[i].dst) == 0) {
			fprintf(stderr, "[ERROR] Can't read from file\n");
			return NULL;
		}
	}

	graph = create_graph_from_data(nCount, eCount, values, edges);

	free(edges);
	free(values);

	return graph;
}

void printGraph(os_graph_t *graph)
{
	int i, j;

	for (i = 0; i < graph->nCount; ++i) {
		printf("[%d]: ", i);
		for (j = 0; j < graph->nodes[i]->cNeighbours; ++j)
			printf("%d ", graph->nodes[i]->neighbours[j]);
		printf("\n");
	}
}
