#include "kernel/types.h"
#include "user/user.h"

// Concurrent pipeline sieve for prime discovery (Eratosthenes algorithm)
static void
filter_primes_stage(int input_fd)
{
  int discovered_prime = 0;
  if (read(input_fd, &discovered_prime, sizeof(int)) <= 0) {
    close(input_fd);
    exit(0);
  }

  printf("prime %d\n", discovered_prime);

  int downstream_channel[2];
  if (pipe(downstream_channel) != 0) {
    fprintf(2, "primes: pipe allocation failed\n");
    close(input_fd);
    exit(1);
  }

  int next_worker = fork();
  if (next_worker < 0) {
    fprintf(2, "primes: worker fork failed\n");
    close(input_fd);
    exit(1);
  }

  if (next_worker == 0) {
    // Child manages downstream stage
    close(input_fd);
    close(downstream_channel[1]);
    filter_primes_stage(downstream_channel[0]);
  } else {
    // Parent filters numbers against discovered_prime
    close(downstream_channel[0]);
    int candidate_number = 0;
    while (read(input_fd, &candidate_number, sizeof(int)) == sizeof(int)) {
      if ((candidate_number % discovered_prime) != 0) {
        write(downstream_channel[1], &candidate_number, sizeof(int));
      }
    }
    close(input_fd);
    close(downstream_channel[1]);
    wait((int *)0);
    exit(0);
  }
}

int
main(int argc, char *argv[])
{
  int initial_pipe[2];
  if (pipe(initial_pipe) != 0) {
    fprintf(2, "primes: initial pipe creation failed\n");
    exit(1);
  }

  int first_worker = fork();
  if (first_worker < 0) {
    fprintf(2, "primes: initial fork failed\n");
    exit(1);
  }

  if (first_worker == 0) {
    close(initial_pipe[1]);
    filter_primes_stage(initial_pipe[0]);
  } else {
    close(initial_pipe[0]);
    const int ceiling_val = 35;
    for (int seed_val = 2; seed_val <= ceiling_val; ++seed_val) {
      write(initial_pipe[1], &seed_val, sizeof(int));
    }
    close(initial_pipe[1]);
    wait((int *)0);
    exit(0);
  }
}
