# include <stdio.h>
# include <stdlib.h>
# include <pthread.h>

int amount;
pthread_mutex_t mutex;

#define NUM_DEPOSIT 3
#define NUM_WITHDRAW 3
#define NUM_THREAD 6

void* deposit(void* param){
  int value = atoi(param);
  if (pthread_mutex_lock(&mutex) != 0){
    printf("Error in lock \n");
    return NULL;
  }
  amount += value;
  printf("Deposit amount = %d\n", amount);
  if (pthread_mutex_unlock(&mutex) != 0){
    printf("Error in unlock \n");
    return NULL;
  }
  return NULL;
}

void* withdraw(void* param){
  int value = atoi(param);
  if (pthread_mutex_lock(&mutex) != 0){
    printf("Error in lock \n");
    return NULL;
  }
  amount -= value;
  printf("Withdraw amount = %d\n", amount);
  if (pthread_mutex_unlock(&mutex) != 0){
    printf("Error in unlock \n");
    return NULL;
  }
  return NULL;
}

int main(int arg, char* argv[]){
  if (arg != 3){
    printf("The argument of this function should be 3 \n");
    return 1;
  }
  if (pthread_mutex_init(&mutex, NULL) != 0){
    printf("Error in initiating mutex \n");
    return 1;
  }
  pthread_t tids[6];
  for (int i=0; i<NUM_DEPOSIT; i++){
    int state = pthread_create(&tids[i], NULL, deposit, argv[1]);
    if (state != 0){
      printf("Error in create thread \n");
      return 1;
    }
  }
  for (int i=NUM_DEPOSIT; i<NUM_THREAD; i++){
    int state = pthread_create(&tids[i], NULL, withdraw, argv[2]);
    if (state != 0){
      printf("Error in create thread \n");
      return 1;
    }
  }
  for (int i=0; i<NUM_THREAD; i++){
    int state = pthread_join(tids[i], NULL);
    if (state != 0){
      printf("Error in join thread \n");
      return 1;
    }
  }
  printf("Balance is: %d\n", amount);
  if (pthread_mutex_destroy(&mutex)!=0){
    printf("Error in destroy mutex \n");
    return 1;
  }
  return 0;
}
