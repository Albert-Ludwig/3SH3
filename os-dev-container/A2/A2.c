#define _DEFAULT_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#define NUM_CHAIR 3
#define MAX_HELP 3                  // in order to avoid the infinite loop of the student just set a random number of max participation of each student thread
#define PROGRAMMING_MIN_TIME 200000 // us
#define PROGRAMMING_MAX_TIME 900000 // us
#define HELP_MIN_TIME 200000        // us
#define HELP_MAX_TIME 700000        // us

pthread_mutex_t mutex;
int waiting_student = 0;
int queue_id[NUM_CHAIR];
int in_idx = 0;
int out_idx = 0;

int N = 0;                // number of students
int total_help_times = 0; // total times of help provided by the TA

sem_t waiting_student_sem;
sem_t *student_called_sem = NULL;
sem_t *help_done_sem = NULL;

int rand_range(int low, int high)
{
    if (high <= low)
    {
        return low;
    }
    return low + rand() % (high - low + 1);
}
void rand_sleep(int low, int high)
{
    usleep((unsigned int)rand_range(low, high));
}
void enqueue_student(int student_id)
{
    queue_id[in_idx] = student_id;
    in_idx = (in_idx + 1) % NUM_CHAIR;
}
int dequeue_student(void)
{
    int student_id = queue_id[out_idx];
    out_idx = (out_idx + 1) % NUM_CHAIR;
    return student_id;
}

void *TA_runner(void *param)
{
    (void)param;
    for (int current_served = 0; current_served < MAX_HELP * N; current_served++)
    {
        sem_wait(&waiting_student_sem);     // if there is no students are waiting, then TA sleeps.
        pthread_mutex_lock(&mutex);         // if there is a student is waiting, lock the mutex.
        int student_id = dequeue_student(); // check the student id
        waiting_student--;                  // remove a student from chair
        pthread_mutex_unlock(&mutex);       // after editing the shared variable, unlock the mutex
        printf("TA is calling the student %d.\n", student_id);
        sem_post(&student_called_sem[student_id]); // release the semaphore of student_called so that the corresponding student can be waken up
        printf("TA is helping the student %d.\n", student_id);
        rand_sleep(HELP_MIN_TIME, HELP_MAX_TIME); // the occupation (help time) should be random
        printf("TA is done helping the student %d.\n", student_id);
        sem_post(&help_done_sem[student_id]); // release the semaphore of help_done so that the corresponding student can be waken up and leave the office
    }
    pthread_exit(0);
}

void *student_runner(void *param)
{
    int student_id = *(int *)param;
    for (int current_asked = 0; current_asked < MAX_HELP; current_asked++)
    {
        printf("Student %d is programming. \n", student_id);
        rand_sleep(PROGRAMMING_MIN_TIME, PROGRAMMING_MAX_TIME);
        pthread_mutex_lock(&mutex);      // enter the CS
        if (waiting_student < NUM_CHAIR) // if there is an empty chair
        {
            enqueue_student(student_id);
            waiting_student++; // sit down and wait
            printf("Student %d is waiting. \n", student_id);
            pthread_mutex_unlock(&mutex);              // finish editing the shared variable and leave the CS
            sem_post(&waiting_student_sem);            // wake up the TA if it is sleeping
            sem_wait(&student_called_sem[student_id]); // wait for the TA to call
            printf("Student %d is getting help. \n", student_id);
            sem_wait(&help_done_sem[student_id]); // wait for the TA to finish helping
            printf("Student %d is done getting help. \n", student_id);
        }
        else
        {
            printf("There is no more chair, student %d is back to programming. \n", student_id);
            pthread_mutex_unlock(&mutex); // leave the CS
        }
    }
    return NULL;
}

int main(int arg, char *argv[])
{
    if (arg < 2)
    {
        printf("Usage: %s <num_students>\n", argv[0]);
        return 1;
    }
    N = atoi(argv[1]);
    if (N <= 0)
    {
        printf("The number of students should be a positive integer number.\n");
        return 1;
    }
    srand((unsigned)time(NULL)); // seed the random number generator with the current time
    total_help_times = MAX_HELP * N;
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        printf("Failed to initialize mutex.\n");
        return 1;
    }
    student_called_sem = (sem_t *)malloc(N * sizeof(sem_t));
    help_done_sem = (sem_t *)malloc(N * sizeof(sem_t));
    if (!student_called_sem || !help_done_sem)
    {
        printf("malloc failed\n");
        return 1;
    }
    if (sem_init(&waiting_student_sem, 0, 0) != 0)
    {
        printf("Failed to initialize waiting_student_sem.\n");
        return 1;
    }
    for (int i = 0; i < N; i++)
    {
        sem_init(&student_called_sem[i], 0, 0);
        sem_init(&help_done_sem[i], 0, 0);
    }
    pthread_t TA_tid;
    pthread_t *student_tid = (pthread_t *)malloc(N * sizeof(pthread_t));
    int *student_id = (int *)malloc(N * sizeof(int));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&TA_tid, &attr, TA_runner, NULL);
    for (int i = 0; i < N; i++)
    {
        student_id[i] = i;
        pthread_create(&student_tid[i], &attr, student_runner, &student_id[i]);
    }
    for (int i = 0; i < N; i++)
    {
        pthread_join(student_tid[i], NULL);
    }
    pthread_join(TA_tid, NULL);
    return 0;
}