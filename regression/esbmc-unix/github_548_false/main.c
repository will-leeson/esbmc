#include <assert.h>
#include<pthread.h>

int glob1 = 0;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *t_fun(void *arg) {
  pthread_mutex_lock(&mutex2);
  glob1 = 5;
  pthread_mutex_unlock(&mutex2);
  return NULL;
}

void *foo(void *arg)
{
  pthread_mutex_lock(&mutex2); // wait/block mutex2
  __ESBMC_assert(glob1 == 0, "glob1 can be 5"); // 
  pthread_mutex_unlock(&mutex2); // unlock mutex2
}

int main(void) {
  pthread_t id;
  pthread_create(&id, NULL, t_fun, NULL);
  pthread_join (id, NULL);  
  *(foo)(0);
  return 0;
}