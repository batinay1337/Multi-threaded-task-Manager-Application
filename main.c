#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>// for string
#include <time.h> // for random time
#include <stdarg.h> // for debug txt 
#include <unistd.h> // for sleep function

#define MAX_TASKS 100
#define MAX_RESOURCES 10

#define MAX_TEXTS 5
#define MAX_TEXT_LENGTH 220

char *file_name = "output.txt";
/* 
---TASK STATES---

READY = 0
RUNNING = 1
BLOCKED = 2

*/

/* HOW TO USE?? 

-----------------------------------
c: Create a new task. In prompt you have to enter the priority, resource allocation. Time value will be calculated by your resource choice. 
d: Destroy a task. The program will prompt you to enter the task ID of the task you want to destroy.
l: Display the current task list.
r: Run task. 
f: Clean the txt file. This is important to use, so be careful :)
q: Quit the program.

NOT:: Every error or debug process will be write in output.txt file. Check some errors. 
-----------------------------------

A- To create a new task, enter C and follow the prompts. For example:

Enter priority: 1
Enter resource allocation:
Enter resource 1: 1
Enter resource 2: 1
Enter resource 3: 1
Enter resource 4: 1
Enter resource 5: 1
Enter resource 6: 1
Enter resource 7: 1
Enter resource 8: 1
Enter resource 9: 1
Enter resource 10: 1

--------------------------
Try it! :)
--------------------------




*/

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


void destroy_task(int task_id) {
    pthread_mutex_lock(&task_mutex);

    if (tasks[task_id].state == -1) {
        printf("Task %d has already been destroyed \n", task_id);
        printFile(file_name, "Task %d has already been destroyed \n", task_id);
        pthread_mutex_unlock(&task_mutex);
        return;
    }

    // kill task thread
    pthread_cancel(tasks[task_id].thread);

    // kill resources
    pthread_mutex_lock(&resource_mutex);
    int i;
    for (i = 0; i < MAX_RESOURCES; i++) {
        resources.freeResources[i] += tasks[task_id].resources[i];
        resources.reserved[i][task_id] -= tasks[task_id].resources[i];
    }
    pthread_mutex_unlock(&resource_mutex);

    // Free resources array memory
    free(tasks[task_id].resources);

    // Update task state
    tasks[task_id].state = -2; // Deleted state

    printf("Task %d has been destroyed \n", task_id);
    printFile(file_name, "Task %d has been destroyed \n", task_id);

    if (task_id != taskNumber - 1) {
        
        tasks[task_id] = tasks[taskNumber - 1];
        tasks[task_id].id = task_id; 

        
        for (i = 0; i < MAX_RESOURCES; i++) {
            resources.reserved[i][task_id] = resources.reserved[i][taskNumber - 1];
            resources.reserved[i][taskNumber - 1] = 0;
        }
    }

    taskNumber--; 

    // Update task ids
    for (i = 0; i < taskNumber; i++) {
        if (tasks[i].id > task_id) {
            tasks[i].id--;
        }
    }

    pthread_mutex_unlock(&task_mutex);
}




//round robin scheduler_fn

void *scheduler_fn(void *arg) {
    sem_wait(&run_semaphore);

    int current_task = -1;

    while (1) {
        pthread_mutex_lock(&task_mutex);
        

        // Find the next ready task to run
        int next_task = -1;
        do {
            current_task = (current_task + 1) % taskNumber;
            if (tasks[current_task].state == 0) {
                next_task = current_task;
                break;
            }
        } while (current_task != next_task);

        if (next_task != -1) {
            
            tasks[next_task].state = 1; // Running state

            pthread_mutex_unlock(&task_mutex);

            // Run the selected task
            printf("Running task %d\n", tasks[next_task].id);
            printf("%s", tasks[next_task].text);
            printFile(file_name, "Running task %d\n", tasks[next_task].id);
            printFile(file_name, "%s", tasks[next_task].text);
            
            
            /*
			// for a scenario where resource usage will decrease over time-->
			if(tasks[next_task].resources[index] > 0 && tasks[next_task].resources[index + 1] != NULL) {
				pthread_mutex_lock(&resource_mutex);
            	tasks[next_task].resources[index] -= 1;
            	pthread_mutex_unlock(&resource_mutex);
            	pthread_mutex_lock(&resource_mutex);
            	resources.freeResources[index] += 1;
            	pthread_mutex_unlock(&resource_mutex);
            	
			}
			
			else {
				pthread_mutex_lock(&resource_mutex);
            	tasks[next_task].resources[index + 1] -= 1;
            	pthread_mutex_unlock(&resource_mutex);
            	pthread_mutex_lock(&resource_mutex);
            	resources.freeResources[index + 1] += 1;
            	pthread_mutex_unlock(&resource_mutex);
			}
			*/
			
			

            sleep(tasks[next_task].task_time);

            pthread_mutex_lock(&task_mutex);
            if (tasks[next_task].state == 1) {
                
                tasks[next_task].state = 0; // Ready state
                printFile(file_name, "Task %d is READY \n", tasks[next_task].id);
            }
            pthread_mutex_unlock(&task_mutex);
        } else {
            pthread_mutex_unlock(&task_mutex);
           
            sleep(1);
        }
    }
}



void *deadlock_fn(void *arg) {
	while (1) {
	    sleep(10);

	    pthread_mutex_lock(&task_mutex);
	    pthread_mutex_lock(&resource_mutex);
	
	    int i, j;
	    int deadlock = 0;
	
	    for (i = 0; i < taskNumber; i++) {
	        if (tasks[i].state == 2) {
	            
	            int can_unblock = 1;
			
	            for (j = 0; j < MAX_RESOURCES; j++) {
	                if (tasks[i].resources[j] > resources.freeResources[j] ){
	                    can_unblock = 0;
	                    break;
	                }
	            }
	
	            if (can_unblock) {
	                
	                tasks[i].state = 0; // Ready state
	
	                for (j = 0; j < MAX_RESOURCES; j++) {
	                    resources.freeResources[j] += tasks[i].resources[j];
	                }
	            } else {
	                // Potential deadlock detected
	                deadlock = 1;
	                printFile(file_name, "Potential deadlock detected \n");
	                break;
	            }
	        }
	    }
	
	    if (deadlock) {
	    	
	        printf("Deadlock detected! Aborting task %d \n", tasks[i].id);
	        printFile(file_name,"Deadlock detected! Aborting task %d \n", tasks[i].id);
	
	        
	        int result = pthread_cancel(tasks[i].thread);
	        if (result != 0) {
	            printf("Failed to cancel task thread \n");
	            printFile(file_name, "Failed to cancel task thread \n");
	            pthread_mutex_unlock(&task_mutex);
	            pthread_mutex_unlock(&resource_mutex);
	            return NULL;
	        }
	        
	        
	
	        
	        result = pthread_join(tasks[i].thread, NULL);
	        if (result != 0) {
	            printf("Failed to join task thread \n");
	            printFile(file_name, "Failed to join task thread \n");
	            pthread_mutex_unlock(&task_mutex);
	            pthread_mutex_unlock(&resource_mutex);
	            return NULL;
	        }
	
	        
	        int k;
	        for (k = i + 1; k < taskNumber; k++) {
	            tasks[k - 1] = tasks[k];
	        }
	
	        taskNumber--;
	
	
	
	
	        
	        for (j = 0; j < MAX_RESOURCES; j++) {
	            resources.freeResources[j] += tasks[i].resources[j];
	            printFile(file_name, "Release resources %d held by the aborted task %d ",tasks[i].resources[j], tasks[i].id );
	        }
	    }
	
	
	
	
	    pthread_mutex_unlock(&task_mutex);
	    pthread_mutex_unlock(&resource_mutex);
	}
}



void display_task_list() {
    pthread_mutex_lock(&task_mutex);
    
    printf("Task List:\n");
    char * state;
    printf(" ______________________________________________________________________________________________________________________________________ \n");
    printf("|       Task ID       |   Priority   |        State       |   Run Time   |                          Resources                          |\n");
    printf(" ______________________________________________________________________________________________________________________________________ \n");
    
    printFile(file_name, " ______________________________________________________________________________________________________________________________________ \n");
    printFile(file_name, "|       Task ID       |   Priority   |        State       |   Run Time   |                          Resources                          |\n");
    printFile(file_name, " ______________________________________________________________________________________________________________________________________ \n");
    int i;
    for (i = 0; i < taskNumber; i++) {
    	if(tasks[i].state == 0){
    		state = "READY";
		}
		
		else if (tasks[i].state == 1){
			state = "RUNNING";
		}
		else {
			state = "BLOCKED";
		}
        printf("|          %-10d |      %-6d  |        %-8s    |      %-6.1f  |", tasks[i].id, tasks[i].priority, state, tasks[i].task_time);
        printFile(file_name, "|          %-10d |      %-6d  |        %-8s    |      %-6.1f  |", tasks[i].id, tasks[i].priority, state, tasks[i].task_time);
        int j;
        for (j = 0; j < MAX_RESOURCES; j++) {
            printf(" %-5d", tasks[i].resources[j]);
            printFile(file_name, " %-5d", tasks[i].resources[j]);
        }
        
        printf(" |\n");
        printFile(file_name, " |\n");
    }
    
    printf(" ______________________________________________________________________________________________________________________________________ \n");
    printFile(file_name, " ______________________________________________________________________________________________________________________________________ \n");
    
    pthread_mutex_unlock(&task_mutex);
}

int command_line_interface() {
	
    while (1) {
        char command;
        int priority;
        int allocate_resources[MAX_RESOURCES];
        int task_id;
        double task_time;
        float timevalue = 0.2;
        
        int error_flag = 0;
        printf("\n                   *** COMMAND LINE INTERFACE ***\n");
		printf(" ____________________________________________________________________________________________________________________\n");
    	printf("|       c: Create       |   d: Destroy   |       l: List      |    r: Run    |    f: Clear TXT File   |    q: quit   |\n");
    	printf(" ____________________________________________________________________________________________________________________ \n");
        scanf(" %c", &command);

        switch (command) {
            case 'c':
            	
                printf("Enter priority: ");
                if (scanf("%d", &priority) != 1 || priority < 0) {
			        printf("Invalid input. Please enter a valid positive integer.\n");
			        printFile(file_name, "Invalid input. Please enter a valid positive integer.\n");
			        error_flag = 1;
			        break; 
			            
        		}
                

                printf("Enter resource allocation one by one: \n");
                int i;
                for (i = 0; i < MAX_RESOURCES; i++) {
                	
                	printf("Enter resource %d: ", i+1);
                	if (scanf("%d", &allocate_resources[i]) != 1 || allocate_resources[i] < 0) {
			            printf("Invalid input. Please enter a valid positive integer.\n");
			            printFile(file_name, "Invalid input. Please enter a valid positive integer.\n");
			            error_flag = 1;
			            break; // Error condition break
			            
        			}
                    //scanf("%d", &allocate_resources[i]);
                    
                    if(error_flag == 0){
                    	
                		int temp = allocate_resources[i];
				    
				    	task_time = task_time + (timevalue * temp);
                    
					}
				    
                    
                }
                if(error_flag == 0){
                	create_task(priority, allocate_resources, task_time);
                	
				}
                
                
                break;

            case 'd':
            	
                printf("Enter task ID: ");
                if (scanf("%d", &task_id) != 1 || task_id < 0) {
			        printf("Invalid input. Please enter a valid positive integer! \n");
			        printFile(file_name, "Invalid input. Please enter a valid positive integer! \n");
			        error_flag = 1;
			        break; // Error condition, program terminates //or break
			            
        		}
                
				 if(error_flag == 0){
                	destroy_task(task_id);
				}
               
                break;
                
            case 'l':
                display_task_list();
                break;
                
            case 'r':
            	// check for created tasks
            	if(taskNumber > 0){
            		sem_post(&run_semaphore);
				}
                else{
                	printf("Not enough task created! \n");
                	printFile(file_name, "Not enough task created! \n");
                	
				}
                break;
                
            case 'f':
                clear_file("output.txt");
                break;

            case 'q':
                printf("Exiting program.\n");
            	// Cancel scheduler thread
	            int result = pthread_cancel(scheduler_thread);
	            if (result != 0) {
	                printf("Failed to cancel scheduler thread.\n");
	                printFile(file_name, "Failed to cancel scheduler thread.\n");
	                return 1;
	            }
	            
	            result = pthread_cancel(deadlock_thread);
	            if (result != 0) {
	                printf("Failed to cancel deadlock detection thread.\n");
	                printFile(file_name, "Failed to cancel deadlock detection thread.\n");
	                return 1;
	            }
	            
	            result = pthread_join(scheduler_thread, NULL);
	            if (result != 0) {
	                printf("Failed to join scheduler thread.\n");
	                printFile(file_name, "Failed to join scheduler thread.\n");
	                return 1;
	            }
	            
	            result = pthread_join(deadlock_thread, NULL);
	            if (result != 0) {
	                printf("Failed to join deadlock detection thread.\n");
	                printFile(file_name, "Failed to join deadlock detection thread.\n");
	                return 1;
	            }
	            return 0;

            default:
            	if(error_flag = 0){
	                printf("Invalid command.\n");
	                printFile(file_name, "Invalid command.\n");
	                break;
            	}
        }
		if(error_flag = 0){
			sem_post(&task_creation_semaphore); // Signal task creation
		}
        
        
    }
    
    return 0;
}

//print elements to the txt file
void printFile(const char *filename, const char *format,...) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}

// clear txt file
void clear_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file : %s\n", filename);
        return;
    }
    
    printf("Text is now clear! ");

    fclose(file);
}



char* selectRandomText() {
	
 char* texts[MAX_TEXTS] = {
        "Umutsuz durumlar yoktur , umutsuz insanlar vardır. Ben hicbir zaman umudumu yitirmedim!\n",
        "Icimizde yanan bu hürriyet atesi, elbet bir gün vuku bulacaktır. Turk milletinin aziz evlatlari bu vatan size minettardir.\n",
        "Turk Milleti bagımsız yasamış ve bagimsizligi varolmalarinin yegane kosulu olarak kabul etmis cesur insanlarin torunlaridir. Bu millet hicbir zaman hur olmadan yasamamistir, yasayamaz ve yasamayacaktir.\n",
        "Ey yukselen yeni nesil! Istikbal sizsiniz. Cumhuriyeti biz kurduk, onu yukseltecek ve yasatacak sizsiniz.\n",
        "İşte, bu ahval ve şerait içinde dahi, vazifen; Türk istiklal ve cumhuriyetini kurtarmaktır! Muhtaç olduğun kudret, damarlarındaki asil kanda, mevcuttur!\n"
    };
    
    srand(time(NULL));  

    int randomIndex = rand() % MAX_TEXTS; 

    char* selectedText = texts[randomIndex];  

    return selectedText;
}



int main() {
	
	// Initialize mutexes
	pthread_mutex_init(&task_mutex, NULL);
	pthread_mutex_init(&resource_mutex, NULL);
	
	// Create scheduler thread
	int result = pthread_create(&scheduler_thread, NULL, scheduler_fn, NULL);
	if (result != 0) {
	    printf("Failed to create scheduler thread.\n");
	    printFile(file_name, "Failed to create scheduler thread.\n");
	    return 1;
	}
	
	// Create deadlock detection thread
	result = pthread_create(&deadlock_thread, NULL, deadlock_fn, NULL);
	if (result != 0) {
	    printf("Failed to create deadlock detection thread.\n");
	    printFile(file_name, "Failed to create deadlock detection thread.\n");
	    return 1;
	}
    

    sem_init(&task_creation_semaphore, 0, 0);
    sem_init(&resource_allocation_semaphore, 0, 0);
    sem_init(&resource_release_semaphore, 0, 0);
    sem_init(&run_semaphore, 0, 0);

    pthread_t scheduler_thread;
    pthread_t deadlock_thread;

    if (pthread_create(&scheduler_thread, NULL, scheduler_fn, NULL) != 0) {
        printf("Error creating scheduler thread.\n");
        printFile(file_name, "Error creating scheduler thread.\n");
        return 1;
    }

    if (pthread_create(&deadlock_thread, NULL, deadlock_fn, NULL) != 0) {
        printf("Error creating deadlock detection thread.\n");
        printFile(file_name, "Error creating deadlock detection thread.\n");
        return 1;
    }
    
    while(1){
    	// resource giving...
    printf("Firstly, give a resource (space-separated): \n");
    int i;
	for (i = 0; i < MAX_RESOURCES; i++) {
        //scanf("%d", &resources.freeResources[i]);
        resources.freeResources[i] = 10;
    }
        
        
    for (i = 0; i < MAX_RESOURCES; i++) {
        if(resources.freeResources[i] < 0){
            printf("Resource must be positive! \n");
            printFile(file_name, "Resource must be positive! \n");
            printf("Going main menu...\n");
            sleep(1);
		}
    } 
		       
    printf("Resource creation succesfully!\n");
        
    sleep(1);
    break;
    	
    	
	}
    
	
	
    command_line_interface();
	
	
    pthread_join(scheduler_thread, NULL);
    pthread_join(deadlock_thread, NULL);

    pthread_mutex_destroy(&task_mutex);
    pthread_mutex_destroy(&resource_mutex);

    sem_destroy(&task_creation_semaphore);
    sem_destroy(&resource_allocation_semaphore);
    sem_destroy(&resource_release_semaphore);

    return 0;
}
