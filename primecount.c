#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

void parseargs(char *argv[], int argc, int *lval, int *uval, int *nval, int *threads_number);
void *calculate_primes(void *args);
int isprime(int n);
typedef struct
{
  int lval;
  int uval;
  char *flagarr;
  pthread_mutex_t *mutex;
  int *nextNumber;
  int thread_number;
} ThreadArgs;

void *calculate_primes(void *args)
{
  ThreadArgs *threadArgs = (ThreadArgs *)args;
  int lval = threadArgs->lval;
  int uval = threadArgs->uval;
  char *flagarr = threadArgs->flagarr;
  pthread_mutex_t *mutex = threadArgs->mutex;
  int *nextNumber = threadArgs->nextNumber;
  int theadIndex = threadArgs->thread_number;
  int initialNumber = lval + theadIndex;
  *nextNumber = initialNumber;
  while (1)
  {
    pthread_mutex_lock(mutex);
    int number = (*nextNumber)++;
    pthread_mutex_unlock(mutex);

    if (number > uval)
      break;

    if (flagarr[number - lval] == 0 && isprime(number))
    {
      pthread_mutex_lock(mutex);
      flagarr[number - lval] = 1;
      pthread_mutex_unlock(mutex);
    }
  }

  pthread_exit(NULL);
}
int main(int argc, char **argv)
{
  int lval = 1;
  int uval = 100;
  int nval = 100;
  char *flagarr = NULL;
  int number;
  int count = 0;
  int printed_count = 0;
  int threads_number = 4;

  // Parse arguments
  parseargs(argv, argc, &lval, &uval, &nval, &threads_number);
  if (uval < lval)
  {
    fprintf(stderr, "Upper bound should not be smaller than lower bound\n");
    exit(1);
  }
  if (lval < 2)
  {
    lval = 2;
    uval = (uval > 1) ? uval : 1;
  }
  // Allocate flags
  flagarr = (char *)malloc(sizeof(char) * (uval - lval + 1));
  if (flagarr == NULL)
    exit(1);
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  ThreadArgs threadArgs[threads_number];
  pthread_t threads[threads_number];

  // Initialize the nextNumber variable
  int nextNumber = lval;

  printf("creating %d threads\n", threads_number);
  for (int i = 0; i < threads_number; i++)
  {
    threadArgs[i].lval = lval;
    threadArgs[i].uval = uval;
    threadArgs[i].flagarr = flagarr;
    threadArgs[i].mutex = &mutex;
    threadArgs[i].nextNumber = &nextNumber;
    threadArgs[i].thread_number = i;
    pthread_create(&threads[i], NULL, calculate_primes, (void *)&threadArgs[i]);
  }

  for (int i = 0; i < threads_number; i++)
    pthread_join(threads[i], NULL);

  // Cleanup
  pthread_mutex_destroy(&mutex);
  for (number = lval; number <= uval; number++)
  {
    if (flagarr[number - lval] == 1)
      count++;
  }
  printf("Found %d primes%c\n", count, count ? ':' : '.');

  for (number = lval; number <= uval; number++)
  {
    if (flagarr[number - lval])
    {
      printed_count++;
      printf("%d%c", number, (printed_count < nval && printed_count < count) ? ',' : '\n');
      if (printed_count >= nval || printed_count >= count)
        break;
    }
  }

  free(flagarr);
  return 0;
}

void parseargs(char *argv[], int argc, int *lval, int *uval, int *nval, int *threads_number)
{
  int ch;

  opterr = 0;
  while ((ch = getopt(argc, argv, "l:u:n:t:")) != -1)
  {
    switch (ch)
    {
    case 'l': // Lower bound flag
      *lval = atoi(optarg);
      break;
    case 'u': // Upper bound flag
      *uval = atoi(optarg);
      break;
    case 'n': // Maximum number of primes to print
      *nval = atoi(optarg);
      break;
    case 't': // Maximum number of primes to print
      *threads_number = atoi(optarg);
      break;
    case '?':
      if (optopt == 'l' || optopt == 'u' || optopt == 'n' || optopt == 'p')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
      exit(1);
    default:
      exit(1);
    }
  }
}
int isprime(int n)
{
  static int *primes = NULL; // NOTE: static !
  static int size = 0;       // NOTE: static !
  static int maxprime;       // NOTE: static !
  int root;
  int i;

  // Init primes array (executed on first call)
  if (primes == NULL)
  {
    primes = (int *)malloc(2 * sizeof(int));
    if (primes == NULL)
      exit(1);
    size = 2;
    primes[0] = 2;
    primes[1] = 3;
    maxprime = 3;
  }

  root = (int)(sqrt(n));

  // Update primes array, if needed
  while (root > maxprime)
    for (i = maxprime + 2;; i += 2)
      if (isprime(i))
      {
        size++;
        primes = (int *)realloc(primes, size * sizeof(int));
        if (primes == NULL)
          exit(1);
        primes[size - 1] = i;
        maxprime = i;
        break;
      }

  // Check 'special' cases
  if (n <= 0)
    return -1;
  if (n == 1)
    return 0;

  // Check prime
  for (i = 0; ((i < size) && (root >= primes[i])); i++)
    if ((n % primes[i]) == 0)
      return 0;
  return 1;
}
