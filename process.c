#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"

typedef enum {
  READY,
  PAUSED,
  BLOCKED,
  FINISHED,
  KILLED
} STATE;

typedef struct {
  FILE *fp;
  int index;
  int priority;
  int position;
  int memory;
  int time_left;
  STATE state;
} process;

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
  p->state = 0; // 1 = blocked, 2 = finished
  return p;
}

void generate_process(int n) {

  // open file number n
  process *p = open_process(n);

  // TODO: metadata for process
  fprintf(p->fp, "%9i\n", random_between(0, 3));
  fprintf(p->fp, "%9i\n", 0);
  fprintf(p->fp, "%9i\n", random_between(10, 200));
  fprintf(p->fp, "%9i\n", random_between(400, 40000));

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

  int count = 0; // counting time
  int i = 0; // lines read from file

  fseek(p->fp, 40 + (p->position * 4), SEEK_SET);
  while(1){ 
    if (fscanf(p->fp, "%i", &i) == EOF) {
      p->state = FINISHED;
      break;
    }
    count += i;
    p->position++;
    p->time_left -= i;

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