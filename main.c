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
