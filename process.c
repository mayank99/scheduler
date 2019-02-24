#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

process *open_process(int n) {
  process *p = malloc(sizeof(process));
  assert(p != 0);

  char s[16];
  sprintf(s, "p%i.txt", n);
  FILE *fp = fopen(s, "r+");
  // if (!fp) damn("file never opened :/");
  assert(fp != 0);

  p->fp = fp;
  p->priority = 0;
  p->position = 0;
  p->memory = 0;
  p->time_left = 0;
  p->state = READY;
  return p;
}

void generate_process(int n) {
  // open file number n
  process *p = open_process(n);

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

  fclose(p->fp);
}

void close_process(process *p) {
  fseek(p->fp, 10, SEEK_SET);
  fprintf(p->fp, "%9i", p->position);
  fseek(p->fp, 20, SEEK_SET);
  fprintf(p->fp, "%9i", p->memory);
  fseek(p->fp, 30, SEEK_SET);
  fprintf(p->fp, "%9i", p->time_left);
  fseek(p->fp, 40, SEEK_SET);
  fprintf(p->fp, "%9i", p->start_time);

  fclose(p->fp);
  free(p);
}

void read_process_metadata(process *p) {
  int priority, position, time_left;

  fseek(p->fp, 0, SEEK_SET);
  fscanf(p->fp, "%i", &priority);

  fseek(p->fp, 10, SEEK_SET);
  fscanf(p->fp, "%i", &position);

  fseek(p->fp, 30, SEEK_SET);
  fscanf(p->fp, "%i", &time_left);

  p->priority = priority;
  p->position = position;
  p->time_left = time_left;
}

// process no. n to be ran for quantum time
int execute_process(process *p, int quantum) {
  // read metadata
  read_process_metadata(p);

  int count = 0;  // counting time
  int i = 0;      // numbers read from file (execution time to add)

  if (p->position == 0) fseek(p->fp, 60 + (p->position * 4), SEEK_SET);  // skip metadata

  while (1) {
    if (fscanf(p->fp, "%i", &i) == EOF) {
      p->state = FINISHED;
      break;
    }
    count += i;
    p->position++;
    p->time_left -= i;

    if (rand() % 50 == 0) {  // 2% chance for block
      // 90% chance for 4-100 ms, 10% for 100-100000 ms
      int x = (rand() % 9) > 0 ? (rand() % 9) + 2 : (rand() % 316) + 10;
      int delay = x * x * 1000;  // exponential delay in us
      p->unblock_time = p->start_time + delay;
      p->state = BLOCKED;
      break;
    }

    if (count > quantum) {
      p->state = PAUSED;
      break;
    }

    if (p->time_left <= 0) {
      p->state = KILLED;
      break;
    }
  }

  return count;
  // close_process(fp, position, time_left);
}

int main() {
  // for (int i = 0; i < 1; i++) {
  //   generate_process(i);
  // }
  generate_process(0);
  process *p = open_process(0);
  read_process_metadata(p);
  execute_process(p, 10000);
  close_process(p);
}