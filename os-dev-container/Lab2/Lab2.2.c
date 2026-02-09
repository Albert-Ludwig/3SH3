# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>
# include <semaphore.h>

int amount = 0;
pthread_mutex_t mutex;
sem_t full;
sem_t empty;

#define NUM_DEPOSIT 7
#define NUM_WITHDRAW 3
#define NUM_THREAD 10
#define UNIT 100

void* deposit(void* param){
  (void)param;
  printf("Executing the deposit function \n");
  if (sem_wait(&empty) != 0){
    printf("Error in getting the semaphore empty \n");
    return NULL;
  }
  if (pthread_mutex_lock(&mutex) != 0){
    printf("Error in getting lock \n");
    return NULL;
  }
  amount += UNIT;
  printf("Amount after deposit = %d\n", amount);
  if (pthread_mutex_unlock(&mutex) != 0){
    printf("Error in unlock the mutex \n");
    return NULL;
  }
  if (sem_post(&full) != 0){
    printf("Error in releasing semaphore full \n");
    sem_post(&empty);
    return NULL;
  }
  return NULL;
}

void* withdraw(void* param){
  (void)param;
  printf("Executing the withdraw function \n");
  if (sem_wait(&full) != 0){
    printf("Error in getting the semaphore full \n");
    return NULL;
  }
  if (pthread_mutex_lock(&mutex) != 0){
    printf("Error in getting lock \n");
    return NULL;
  }
  amount -= UNIT;
  printf("Amount after withdraw = %d\n", amount);
  if (pthread_mutex_unlock(&mutex) != 0){
    printf("Error in unlock the mutex \n");
    return NULL;
  }
  if (sem_post(&empty) != 0){
    printf("Error in releasing semaphore empty \n");
    sem_post(&full);
    return NULL;
  }
  return NULL;
}

int main(int arg, char* argv[]){
  if (arg != 2){
    printf("The argument of this function should be 2 \n");
    return 1;
  }
  int input = atoi(argv[1]);
  if (input != 100){
    printf("The input must be 100 \n");
    return 1;
  }
  if (pthread_mutex_init(&mutex, NULL) != 0){
    printf("Error in initiating mutex lock \n");
    return 1;
  }
  if (sem_init(&full, 0, 0) != 0){
    printf("Error in initiating semaphore full \n");
    return 1;
  }
  if (sem_init(&empty, 0, 4) != 0){
    printf("Error in initiating semaphore empty \n");
    return 1;
  }
  pthread_t tids[NUM_THREAD];
  for (int i=0; i<NUM_DEPOSIT; i++){
    int state = pthread_create(&tids[i], NULL, deposit, argv[1]);
    if (state != 0){
      printf("Error in creating thread \n");
      return 1;
    }
  }
  for (int i=NUM_DEPOSIT; i<NUM_THREAD; i++){
    int state = pthread_create(&tids[i], NULL, withdraw, argv[1]);
    if (state != 0){
      printf("ERROR in creating thread \n");
      return 1;
    }
  }
  for (int i=0; i<NUM_THREAD; i++){
    int state = pthread_join(tids[i], NULL);
    if (state != 0){
      printf("Error in joining thread \n");
      return 1;
    }
  }
  printf("Balance is %d\n", amount);
  if (sem_destroy(&full) != 0){
    printf("Error in destroy semaphore full \n");
    return 1;
  }
  if (sem_destroy(&empty) != 0){
    printf("Error in destroy semaphire empty \n");
    return 1;
  }
  if (pthread_mutex_destroy(&mutex)!=0){
    printf("Error in destroy mutex \n");
    return 1;
  }
  return 0;
}
