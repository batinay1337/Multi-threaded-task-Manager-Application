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




void create_task(int priority, int *resourcesnumber, double task_time) {
    pthread_mutex_lock(&task_mutex);

    // check if task number is max
    if (taskNumber >= MAX_TASKS) {
        printf("Maximum number of tasks reached! \n");
        printFile(file_name, "Maximum number of tasks reached! \n");
        pthread_mutex_unlock(&task_mutex);
        return;
    }

	
    int task_id = taskNumber++;
	
    tasks[task_id].id = task_id;
    tasks[task_id].priority = priority;
    tasks[task_id].state = 0; // Ready state
    tasks[task_id].resources = malloc(MAX_RESOURCES * sizeof(int));
    tasks[task_id].task_time = task_time;
    tasks[task_id].text = selectRandomText();

    // Copy resourcesnumber array to tasks[task_id].resources
    int i;
    for (i = 0; i < MAX_RESOURCES; i++) {
        tasks[task_id].resources[i] = resourcesnumber[i];
    }

    pthread_mutex_unlock(&task_mutex);

    // Acquire required resources
    pthread_mutex_lock(&resource_mutex);

    int can_be_reserved = 1; // flag for allocation
    for (i = 0; i < MAX_RESOURCES; i++) {
        if (resources.freeResources[i] < resourcesnumber[i]) {
            can_be_reserved = 0;
            printf("Not enough resources for this task ");
            printf("Exiting...");
            printFile(file_name, "Not enough resources for this task ");
            printFile(file_name, "Exiting...");
            sleep(3);
            break;
        }
    }

    if (can_be_reserved) {
        // Allocate resources
        for (i = 0; i < MAX_RESOURCES; i++) {
            resources.freeResources[i] -= resourcesnumber[i];
            resources.reserved[task_id][i] += resourcesnumber[i];
        }
        printf("Resources reserved for task %d \n", tasks[task_id].id);
        printFile(file_name, "Resources reserved for task %d \n", task_id);
        pthread_mutex_unlock(&resource_mutex);
        sleep(2);

        if (pthread_create(&tasks[task_id].thread, NULL, task_fn, (void *)&tasks[task_id]) != 0) {
            printf("Error creating thread for task %d.\n", task_id);
            printFile(file_name, "Error creating thread for task %d.\n", task_id);
            return;
        }
    } else {
        pthread_mutex_unlock(&resource_mutex);
        printf("Not enough resources to allocate. Task %d is blocked.\n", task_id);
        printFile(file_name, "Not enough resources to allocate. Task %d is blocked.\n", task_id);
        return;
    }

    sem_post(&task_creation_semaphore);
}
