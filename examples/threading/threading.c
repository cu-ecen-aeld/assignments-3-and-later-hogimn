#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Cast the thread arguments to the correct type
    struct thread_data* thread_func_args = (struct thread_data*) thread_param;

    // Wait for the specified time before attempting to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Attempt to obtain the mutex
    int result = pthread_mutex_lock(thread_func_args->mutex);
    if (result != 0) {
        ERROR_LOG("Failed to obtain mutex: %s\n", strerror(result));
        return NULL;
    }

    // Wait for the specified time before releasing the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Release the mutex
    result = pthread_mutex_unlock(thread_func_args->mutex);
    if (result != 0) {
        ERROR_LOG("Failed to release mutex: %s\n", strerror(result));
        return NULL;
    }

    // Save the result
    thread_func_args->thread_complete_success = true;

    // Return the thread arguments
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

     // Allocate memory for the thread data
    struct thread_data* thread_data_ptr = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (thread_data_ptr == NULL) {
        ERROR_LOG("Failed to allocate memory for thread data\n");
        return false;
    }

    // Initialize the thread data
    thread_data_ptr->mutex = mutex;
    thread_data_ptr->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_ptr->wait_to_release_ms = wait_to_release_ms;
    thread_data_ptr->thread_complete_success = false;

    // Create the thread and pass the thread data as an argument
    int result = pthread_create(thread, NULL, threadfunc, (void*) thread_data_ptr);
    if (result != 0) {
        ERROR_LOG("Failed to create thread: %s\n", strerror(result));
        free(thread_data_ptr);
        return false;
    }

    // Return success
    return true;
}

