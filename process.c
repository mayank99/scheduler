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
  int start_time;
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
  fprintf(p->fp, "%9i", p->start_time);
  fseek(p->fp, 50, SEEK_SET);
  fprintf(p->fp, "%9i", p->end_time);

  fclose(p->fp);
  free(p);
}

void read_process_metadata(process *p) {
  int priority, position, memory, time_left, start_time, end_time;

  fseek(p->fp, 0, SEEK_SET);
  fscanf(p->fp, "%i", &priority);

  fseek(p->fp, 10, SEEK_SET);
  fscanf(p->fp, "%i", &position);

  fseek(p->fp, 20, SEEK_SET);
  fscanf(p->fp, "%i", &memory);

  fseek(p->fp, 30, SEEK_SET);
  fscanf(p->fp, "%i", &time_left);

  fseek(p->fp, 40, SEEK_SET);
  fscanf(p->fp, "%i", &start_time);

  fseek(p->fp, 50, SEEK_SET);
  fscanf(p->fp, "%i", &end_time);

  p->priority = priority;
  p->position = position;
  p->memory = memory;
  p->time_left = time_left;
  p->start_time = start_time;
  p->end_time = end_time;
}

void generate_process(int n) {
  // open file number n
  process *p = open_process(n, "w+");

  // TODO: metadata for process
  fprintf(p->fp, "%9i\n", random_between(0, 3));              // priority
  fprintf(p->fp, "%9i\n", 0);                                 // position
  fprintf(p->fp, "%9i\n", random_between(10, 200));           // memory
  fprintf(p->fp, "%9i\n", random_between(400000, 40000000));  // execution time
  fprintf(p->fp, "%9i\n", 0);                                 // start time
  fprintf(p->fp, "%9i\n", 0);                                 // end time

  // actual instructions
  for (int i = 0; i < random_between(1000, 100000); i++) {
    fprintf(p->fp, "%3i\n", random_between(1, 20));
  }

  // fclose(p->fp);
  read_process_metadata(p);
  close_process(p);
}

// process p to be ran for quantum time
int execute_process(process *p, int quantum) {
  // read metadata
  // read_process_metadata(p);

  int count = 0;  // counting time
  int i = 0;      // numbers read from file (execution time to add)

  if (p->position != 0) fseek(p->fp, 60 + (p->position * 4), SEEK_SET);  // skip metadata

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
      p->unblock_time = p->start_time + delay;
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
  printf("p_number start_time end_time state\n");
  printf("-----------------------------------\n");
  srand(time(0));

  int timer = 0;  // cpu clock
  int new_process_at = 0;
  int process_count;

  QUEUE *q = newQUEUE();
  setQUEUEfree(q, freeINTEGER);
  setQUEUEdisplay(q, displayINTEGER);

  // while (1) {
  do {
    // issue new process every 20-10000 ms
    if (timer == 0 || timer > new_process_at && process_count <= 10) {
      generate_process(process_count++);
      enqueue(q, newINTEGER(process_count - 1));
      // if (process_count == 5) break;
      new_process_at = timer + random_between(20, 10000);
    }

    process *p = open_process(getINTEGER(dequeue(q)), "r+");
    read_process_metadata(p);

    if (p->state == READY || p->state == PAUSED) {
      p->start_time = timer;
      timer += execute_process(p, 10000);
      p->end_time = timer;
    }

    // if (p->state == FINISHED || p->state == KILLED)
    if (p->state == FINISHED)
      printf("%7i %10i %8i %6i\n", p->index, p->start_time, p->end_time, p->state);

    if (p->state == BLOCKED && timer >= p->unblock_time) p->state = PAUSED;
    if (p->state == PAUSED || p->state == BLOCKED) enqueue(q, newINTEGER(p->index));

    close_process(p);
  } while (sizeQUEUE(q) > 0);
}