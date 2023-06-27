Tema 3 SO - Parallel Graph - 27.04.2023
Bloțiu Mihnea-Andrei - 333CA

The main ideea of the homework was to implement a generic thread pool in C using pthreads and to
test it by traversing a graph and copute the sum of the values of the nodes.

In order to do that, the project is divided into two main files:
	* os_threadpool.c - containing the implementation of the threadpool
	* parallel.c - containing the usage of the threadpool in order to traverse a graph

- The implementation of the threadpool (os_threadpoo.c):

	* threadpool_create:
		* In order to create the threadpool we allocate memory for the threadpool and the threads,
		we set the variables to their initial values and create a global variable that is going to
		remember the maximum number of tasks that we can have in the threadpool at a certain moment
		of time. We do that because, we will need the variable's value when we would like to add a
		new task to the threadpool.
		* After initializing all the values, we create the threads with the thread_loop_function and
		the threadpool as a parameter.

	* thread_loop_function:
		* Is the function that each of the threads use after they were created.
		* Consists of a true loop until they find a task in the threadpool that they can execute or
		until they are signaled by the should_stop field in the threadpool that all the tasks were done
		and there are no more task to come in the threadpool and they should stop their execution.

	* threadpool_stop:
		* Is the function that is it is the opposite of the threadpool_create;
		* The function is called by the main thread (the same that calls threadpool_create) and waits in
		a true loop until the processingIsDone method returns true. When it returns true it means that 
		there will be no more tasks to come to the threadpool so the function sets the should_stop variable
		from the threadpool to true in order to tell the secondary threads that they can stop and then the
		main thread can start to join all the other threads and free the memory usage of the threadpool

	* task_create:
		* In order to create a task for the threadpool we just allocate memory for the new task, we set
		its fields and we return the new task

	* add_task_to_queue:
		* In order to add a task to the threadpool we wait in a true loop until there is space in the
		threadpool to add a new task
		* When it is space, we just allocate memory for the new element in the task queue, we set the
		task and then add it to the end of the list, synchronizing with mutexes these operations because
		we want to make sure that there are not two threads that add in the queue in the same time because
		then there will be a race condition.

	* get_task:
		* In order to get a task from the queue we just always take the first element from the task queue
		(if it exists) an return it making sure that the operation will be synchronized because we don't
		want to have two threads that are going to execute the same task at the same time.

- The implementation of the parallel traverse (parallel.c):
	
	* main:
		* In order to parallel traverse a graph, after we have it in memory we initialize the threadpool
		with a number of threads and a maximum number of tasks and we also initialize three more mutexes
		because we need to synchronize some operations explained below.
		* After the main thread initializes the threadpool, it starts traversing the graph an then calls
		the threadpool_stop function with processingIsDone function, waiting for all the other threads to
		finish their tasks.

	* processingIsDone:
		* In order to know when there will be no more tasks for the threadpool, taking into consideration
		that we have to traverse a graph, we will have a global variable that memorizes the number of nodes
		that have not been traversed yet.
		* When this variable comes to 0 value it means that there will be no more tasks to come so we should
		return 1 (for the main thread to start joining all the other threads)

	* traverse_graph:
		* After the main thread starts the threadpool, he is the first to start adding tasks to the threadpool.
		He is doing that by starting to traverse the graph and each time he finds a node that has not been
		traversed yet, he creates a task for it and adds it to the threadpool with the task_function that
		is going to be executed by the thread that takes the task from the threadpool.

	* task_function:
		* Each of the secondary threads call the task_function for each of the tasks in the threadpool. When
		they take a task, they firstly add the value of the node to the general sum;
		* Then, they traverse each of the neighbours for the current node and each time they find a neighbour
		that has not been traversed yet, they make a task for it and add it in the queue;
		* At the end of the function they mark that one more node has been traversed (so the decrement the
		global remaining_nodes variable) in order to finish the threadpool when there are no more tasks left;

- The operations that have to be syncrhonized:
	* We have to synchronize the adding and removing from the threadpool task queue (this is done in
	the os_threadpool.c module because it has to be independent from the usage of the threadpool) in
	order to be sure that there won't be any race condition when adding a new task or in order to be
	sure that there won't be two threads that will take the same task from the queue when the want
	to execute one;
	* When traversing the graph we have to syncrhonize the access to the visited vector because if not
	maybe two or more threads will see that a node has not been visited at the same time and they will
	add the same node multiple times in the task queue;
	* We also have to synchronize the access to the global variables of the final sum and the remaining
	nodes to be traversed because if we don't do it, we might end up with multiple threads trying to modify
	it at the same time.
