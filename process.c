#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lib/integer.h"
#include "lib/queue.h"

typedef enum { READY, PAUSED, BLOCKED, FINISHED, KILLED } STATE;

typedef struct {
  FILE *fp;
  int index;
  int priority;
  int position;
  int memory;
  int time_left;
  int process_start_time;
  int last_start_time;
  int end_time;
  int unblock_time;
  STATE state;
} process;

int random_between(int from, int to) { return (rand() % (to - from + 1)) + from; }

process *open_process(int n, char *mode) {
  process *p = malloc(sizeof(process));
  assert(p != 0);

  char s[16];
  sprintf(s, "processes/p%i.txt", n);
  FILE *fp = fopen(s, mode);
  // if (!fp) damn("file never opened :/");
  assert(fp != 0);

  p->index = n;
  p->fp = fp;
  p->priority = 0;
  p->position = 0;
  p->memory = 0;
  p->time_left = 0;
  p->state = READY;
  p->process_start_time = -1;
  p->last_start_time = -1;
  p->end_time = -1;
  p->unblock_time = 0;
  return p;
}

void close_process(process *p) {
  fseek(p->fp, 0, SEEK_SET);
  fprintf(p->fp, "%9i", p->priority);
  fseek(p->fp, 10, SEEK_SET);
  fprintf(p->fp, "%9i", p->position);
  fseek(p->fp, 20, SEEK_SET);
  fprintf(p->fp, "%9i", p->memory);
  fseek(p->fp, 30, SEEK_SET);
  fprintf(p->fp, "%9i", p->time_left);
  fseek(p->fp, 40, SEEK_SET);
  fprintf(p->fp, "%9i", p->process_start_time);
  fseek(p->fp, 50, SEEK_SET);
  fprintf(p->fp, "%9i", p->last_start_time);
  fseek(p->fp, 60, SEEK_SET);
  fprintf(p->fp, "%9i", p->end_time);
  fseek(p->fp, 70, SEEK_SET);
  fprintf(p->fp, "%9i", p->unblock_time);

  fclose(p->fp);
  free(p);
}

void read_process_metadata(process *p) {
  fseek(p->fp, 0, SEEK_SET);
  fscanf(p->fp, "%i", &(p->priority));

  fseek(p->fp, 10, SEEK_SET);
  fscanf(p->fp, "%i", &(p->position));

  fseek(p->fp, 20, SEEK_SET);
  fscanf(p->fp, "%i", &(p->memory));

  fseek(p->fp, 30, SEEK_SET);
  fscanf(p->fp, "%i", &(p->time_left));

  fseek(p->fp, 40, SEEK_SET);
  fscanf(p->fp, "%i", &(p->process_start_time));

  fseek(p->fp, 50, SEEK_SET);
  fscanf(p->fp, "%i", &(p->last_start_time));

  fseek(p->fp, 60, SEEK_SET);
  fscanf(p->fp, "%i", &(p->end_time));

  fseek(p->fp, 70, SEEK_SET);
  fscanf(p->fp, "%i", &(p->unblock_time));
}

// generates a process (given the process number), returns priority
int generate_process(int n) {
  // open file number n
  process *p = open_process(n, "w+");

  // TODO: metadata for process
  int priority = random_between(0, 3);
  fprintf(p->fp, "%9i\n", priority);                          // priority
  fprintf(p->fp, "%9i\n", 0);                                 // position
  fprintf(p->fp, "%9i\n", random_between(10, 200));           // memory
  fprintf(p->fp, "%9i\n", random_between(400000, 40000000));  // execution time
  fprintf(p->fp, "%9i\n", -1);                                // initial start time
  fprintf(p->fp, "%9i\n", -1);                                // last execution start time
  fprintf(p->fp, "%9i\n", -1);                                // end time
  fprintf(p->fp, "%9i\n", 0);                                 // time at which to unblock (if)

  // actual instructions
  for (int i = 0, max = random_between(1000, 100000); i < max; i++) {
    fprintf(p->fp, "%3i\n", random_between(1, 20));
  }

  // fclose(p->fp);
  read_process_metadata(p);
  close_process(p);

  return priority;
}

// process p to be ran for quantum time
int execute_process(process *p, int quantum) {
  int count = 0;  // counting time
  int i = 0;      // numbers read from file (execution time to add)

  if (p->position != 0) fseek(p->fp, 80 + (p->position * 4), SEEK_SET);  // skip metadata

  while (1) {
    if (fscanf(p->fp, "%i", &i) == EOF) {
      p->state = FINISHED;
      break;
    }

    count += i;
    p->position++;
    p->time_left -= i;

    if (rand() % 100 >= 98) {  // 2% chance for block
      // 90% chance for 4-100 ms, 10% for 100-100000 ms
      int x = (rand() % 9) > 0 ? (rand() % 9) + 2 : (rand() % 316) + 10;
      int delay = x * x * 1000;  // exponential delay in us
      p->unblock_time = p->last_start_time + delay;
      p->state = BLOCKED;
      break;
    }

    if (p->time_left <= 0) {
      p->state = KILLED;
      break;
    }

    if (count > quantum) {
      p->state = PAUSED;
      break;
    }
  }

  return count;
  // close_process(fp, position, time_left);
}

int main() {
  printf("p_number start_time current_time end_time priority state\n");
  printf("--------------------------------------------------------\n");
  srand(time(0));

  int timer = 0;  // cpu clock
  int new_process_at = 0;
  int process_count = 0;

  QUEUE *ready_queue = newQUEUE();
  QUEUE *blocked_queue = newQUEUE();
  // setQUEUEfree(ready_queue, freeINTEGER);
  // setQUEUEdisplay(ready_queue, displayINTEGER);

  do {
    // issue new process every 20-10000 ms
    if (timer == 0 || timer > new_process_at && process_count <= 10) {
      int priority = generate_process(process_count++);
      enqueue(ready_queue, newINTEGER(process_count - 1));
      new_process_at = timer + random_between(20, 10000);
    }

    process *p = open_process(getINTEGER(dequeue(ready_queue)), "r+");
    read_process_metadata(p);

    if (p->state == READY || p->state == PAUSED
      ) {
      if (p->process_start_time == -1) p->process_start_time = timer;
      p->last_start_time = timer;
      timer += execute_process(p, 10000);
      p->end_time = timer;
    }

    if (p->state == FINISHED || p->state == KILLED)
      printf("%7i %10i %11i %8i %8i %6i\n", p->index, p->process_start_time, p->last_start_time,
             p->end_time, p->priority, p->state);

    if (p->state == PAUSED) enqueue(ready_queue, newINTEGER(p->index));
    if (p->state == BLOCKED) enqueue(blocked_queue, newINTEGER(p->index));

    close_process(p);
    // timer++;

  } while (sizeQUEUE(ready_queue) > 0 || sizeQUEUE(blocked_queue) > 0);
}