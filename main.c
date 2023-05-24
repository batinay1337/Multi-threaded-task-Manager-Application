#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>




#define MAX_TASKS 100
#define MAX_RESOURCES 10








typedef struct {
    int id;
    int priority;
    int state;
    int *resources;
    double task_time;
    char* text;
    pthread_t thread;
} PCB_t;

typedef struct {
    int freeResources[MAX_RESOURCES];
    int reserved[MAX_TASKS][MAX_RESOURCES];
} RESOURCE_t;


RESOURCE_t resources;

PCB_t tasks[MAX_TASKS];

int taskNumber = 0;


int command_line_interface();
char* selectRandomText();
void *task_fn(void *arg);
void create_task(int priority, int *resourcesnumber, double task_time);
void destroy_task(int task_id);
void *scheduler_fn(void *arg);
void *deadlock_fn(void *arg);

pthread_t scheduler_thread, deadlock_thread;
pthread_mutex_t task_mutex; // Mutex for task-related operations
pthread_mutex_t resource_mutex; // Mutex for resource-related operations

sem_t task_creation_semaphore; // Semaphore for task creation synchronization
sem_t resource_allocation_semaphore; // Semaphore for resource allocation synchronization
sem_t resource_release_semaphore; // Semaphore for resource release synchronization
sem_t run_semaphore;  // Semaphore for running all tasks



void *task_fn(void *arg) {
    PCB_t *task = (PCB_t *)arg;
    

    // Acquire required resources
    pthread_mutex_lock(&resource_mutex);
	int i;
    for (i = 0; i < MAX_RESOURCES; i++) {
        if (resources.freeResources[i] < task->resources[i]) {
            // Not enough resources, task is blocked
            
            task->state = 2; // Blocked state
			int j;
            
            for (j = 0; j < i; j++) {
                resources.freeResources[j] += task->resources[j];
                resources.reserved[task->id][j] -= task->resources[j];
            }
            
            sleep(6);

            pthread_mutex_unlock(&resource_mutex);
            sem_wait(&resource_allocation_semaphore);
            pthread_mutex_lock(&resource_mutex);

            i = -1; 
        } else {
            // Allocate resources
            resources.freeResources[i] -= task->resources[i];
            resources.reserved[task->id][i] += task->resources[i];
        }
    }

    pthread_mutex_unlock(&resource_mutex);
    
	sem_wait(&run_semaphore);
    // execute task
    printf("Executing task %d \n", task->id);
	printFile(file_name, "Executing task %d \n", task->id);
    // Release acquired resources
    pthread_mutex_lock(&resource_mutex);
	
    for (i = 0; i < MAX_RESOURCES; i++) {
        resources.freeResources[i] += task->resources[i];
        resources.reserved[task->id][i] = 0;
    }

    pthread_mutex_unlock(&resource_mutex);

    sem_post(&resource_release_semaphore);

    pthread_exit(NULL);
}