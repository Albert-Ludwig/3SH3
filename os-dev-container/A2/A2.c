// gcc -Wall -Wextra -pthread -g -o A2 A2.c
// ./ A2 <num_students>
#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

/* Added for named semaphores on macOS */
#include <fcntl.h>
#include <sys/stat.h>

#define NUM_CHAIR 3
#define MAX_HELP 3                  // in order to avoid the infinite loop of the student just set a random number of max participation of each student thread
#define PROGRAMMING_MIN_TIME 200000 // us
#define PROGRAMMING_MAX_TIME 900000 // us
#define HELP_MIN_TIME 200000        // us
#define HELP_MAX_TIME 700000        // us

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int waiting_student = 0; /* number of students waiting in hallway chairs */
static int queue_id[NUM_CHAIR]; /* circular queue of waiting student IDs */
static int in_idx = 0, out_idx = 0;

static int N = 0;                   /* number of students */
static int total_help_needed = 0;   /* total successful helps needed before TA exits */
static int total_help_provided = 0; /* total successful helps already provided */

/* Switched to named semaphores on macOS (sem_init/sem_destroy are deprecated) */
static sem_t *waiting_student_sem = NULL; /* counts waiting students; TA sleeps on this */
static sem_t **student_called_sem = NULL; /* per-student: TA calls student */
static sem_t **help_done_sem = NULL;      /* per-student: TA finished helping */

/* Added: store semaphore names for cleanup (unlink) */
static char waiting_sem_name[64];

int rand_range(int low, int high) // returns a random integer in the inclusive range [low, high]
{
    if (high <= low)
    {
        return low;
    }
    return low + rand() % (high - low + 1);
}
void rand_sleep(int low, int high)
{
    usleep((unsigned int)rand_range(low, high)); // sleep for random time in given range
}
void enqueue_student(int student_id)
{
    queue_id[in_idx] = student_id;
    in_idx = (in_idx + 1) % NUM_CHAIR; // move to next position in circular queue
}
int dequeue_student(void)
{
    int student_id = queue_id[out_idx];  // get student ID at front of queue
    out_idx = (out_idx + 1) % NUM_CHAIR; // move to next position
    return student_id;
}

static void *TA_runner(void *param)
{
    (void)param;

    while (1)
    {
        /* If no students are waiting, TA sleeps here */
        sem_wait(waiting_student_sem);

        pthread_mutex_lock(&mutex);

        /* Added: allow TA to exit cleanly once enough helps are provided */
        if (total_help_provided >= total_help_needed)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Added: safety guard in case semaphore count and waiting_student desync */
        if (waiting_student <= 0)
        {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        /* Dequeue next waiting student */
        int student_id = dequeue_student();
        waiting_student--;

        pthread_mutex_unlock(&mutex);

        printf("TA is calling student %d.\n", student_id);
        sem_post(student_called_sem[student_id]); // wake up the student to be helped

        printf("TA is helping student %d.\n", student_id);
        rand_sleep(HELP_MIN_TIME, HELP_MAX_TIME); // simulate time taken to help
        printf("TA is done helping student %d.\n", student_id);

        /* Mark one successful help */
        pthread_mutex_lock(&mutex);
        total_help_provided++;
        int done = (total_help_provided >= total_help_needed);
        pthread_mutex_unlock(&mutex);

        sem_post(help_done_sem[student_id]);

        if (done)
        {
            /* Wake TA once so it can notice done and exit cleanly if it sleeps again */
            /* Added: wake TA in case it goes back to sleep before checking 'done' */
            sem_post(waiting_student_sem);
            break;
        }
    }

    pthread_exit(NULL);
}

static void *student_runner(void *param)
{
    int student_id = *(int *)param;

    int helps_received = 0;
    while (helps_received < MAX_HELP)
    {
        printf("Student %d is programming.\n", student_id);
        rand_sleep(PROGRAMMING_MIN_TIME, PROGRAMMING_MAX_TIME);

        pthread_mutex_lock(&mutex);

        if (waiting_student < NUM_CHAIR)
        {
            enqueue_student(student_id);
            waiting_student++;
            printf("Student %d is waiting in the hallway (%d/%d chairs occupied).\n",
                   student_id, waiting_student, NUM_CHAIR);

            pthread_mutex_unlock(&mutex);

            /* Notify TA */
            sem_post(waiting_student_sem);

            /* Wait to be called, then wait until help is finished */
            sem_wait(student_called_sem[student_id]);
            printf("Student %d is getting help.\n", student_id);

            sem_wait(help_done_sem[student_id]);
            helps_received++;
            printf("Student %d is done getting help (%d/%d).\n",
                   student_id, helps_received, MAX_HELP);
        }
        else
        {
            /* No chair available -> come back later (does NOT count as a help) */
            printf("No hallway chair available; student %d will come back later.\n", student_id);
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <num_students>\n", argv[0]);
        return 1;
    }

    N = atoi(argv[1]);
    if (N <= 0)
    {
        fprintf(stderr, "Number of students must be a positive integer.\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    for (int i = 0; i < NUM_CHAIR; i++)
        queue_id[i] = -1;

    total_help_needed = MAX_HELP * N;
    total_help_provided = 0;

    /* Added: create a unique named semaphore for this process */
    snprintf(waiting_sem_name, sizeof(waiting_sem_name), "/waiting_student_sem_%d", (int)getpid());
    sem_unlink(waiting_sem_name); /* Added: remove leftover semaphore name if it exists */
    waiting_student_sem = sem_open(waiting_sem_name, O_CREAT | O_EXCL, 0600, 0);
    if (waiting_student_sem == SEM_FAILED)
    {
        perror("sem_open(waiting_student_sem)");
        return 1;
    }

    /* Allocate arrays of semaphore pointers (named semaphores) */
    student_called_sem = malloc((size_t)N * sizeof(*student_called_sem));
    help_done_sem = malloc((size_t)N * sizeof(*help_done_sem));
    if (!student_called_sem || !help_done_sem)
    {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    char temp_called_name[64];
    char temp_done_name[64];

    for (int i = 0; i < N; i++)
    {
        /* Added: allocate and build unique names per student semaphore */
        snprintf(temp_called_name, 64, "/student_called_sem_%d_%d", (int)getpid(), i);
        snprintf(temp_done_name, 64, "/help_done_sem_%d_%d", (int)getpid(), i);

        sem_unlink(temp_called_name); /* Added: avoid leftover named semaphores */
        sem_unlink(temp_done_name);

        student_called_sem[i] = sem_open(temp_called_name, O_CREAT | O_EXCL, 0600, 0);
        if (student_called_sem[i] == SEM_FAILED)
        {
            perror("sem_open(student_called_sem)");
            return 1;
        }

        help_done_sem[i] = sem_open(temp_done_name, O_CREAT | O_EXCL, 0600, 0);
        if (help_done_sem[i] == SEM_FAILED)
        {
            perror("sem_open(help_done_sem)");
            return 1;
        }
    }

    pthread_t ta_tid;
    pthread_t *student_tid = malloc((size_t)N * sizeof(*student_tid));
    int *student_id = malloc((size_t)N * sizeof(*student_id));
    if (!student_tid || !student_id)
    {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    if (pthread_create(&ta_tid, NULL, TA_runner, NULL) != 0)
    {
        perror("pthread_create(TA)");
        return 1;
    }

    for (int i = 0; i < N; i++)
    {
        student_id[i] = i;
        if (pthread_create(&student_tid[i], NULL, student_runner, &student_id[i]) != 0)
        {
            perror("pthread_create(student)");
            return 1;
        }
    }

    for (int i = 0; i < N; i++)
        pthread_join(student_tid[i], NULL);
    pthread_join(ta_tid, NULL);

    /* Cleanup */
    for (int i = 0; i < N; i++)
    {
        /* Added: close + unlink named semaphores */
        if (student_called_sem[i] && student_called_sem[i] != SEM_FAILED)
            sem_close(student_called_sem[i]);
        if (help_done_sem[i] && help_done_sem[i] != SEM_FAILED)
            sem_close(help_done_sem[i]);

        snprintf(temp_called_name, 64, "/student_called_sem_%d_%d", (int)getpid(), i);
        snprintf(temp_done_name, 64, "/help_done_sem_%d_%d", (int)getpid(), i);
        sem_unlink(temp_called_name);
        sem_unlink(temp_done_name);
    }

    /* Added: close + unlink the global named semaphore */
    if (waiting_student_sem && waiting_student_sem != SEM_FAILED)
        sem_close(waiting_student_sem);
    sem_unlink(waiting_sem_name);

    free(student_called_sem);
    free(help_done_sem);
    free(student_tid);
    free(student_id);

    pthread_mutex_destroy(&mutex);

    printf("Simulation finished: TA provided %d/%d helps.\n", total_help_provided, total_help_needed);
    return 0;
}