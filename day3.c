#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement");
  }

  unsigned long cntfrq;
  unsigned long program_start;
  unsigned long program_end;
  asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_start));

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  int* line = malloc(10 * sizeof(int));
  int line_size = 0;
  
  int num1 = 0;
  int num2 = 0;
  int state = 0; 
  int total = 0;
  bool enable = true;
  while(file < file_start + input_size) {
    char c = *file++;

    if(enable) {
      if(state == 0 && c == 'm') {
        state++;
      } else if(state == 1 && c == 'u') {
        state++;
      } else if(state == 2 && c == 'l') {
        state++;
      } else if(state == 3 && c == '(') {
        state++;
      } else if(state == 4 && isdigit(c)) {
        num1 = (num1 * 10) + c - '0';
      } else if(state == 4 && c == ',') {
        state++;
      } else if(state == 5 && isdigit(c)) {
        num2 = (num2 * 10) + c - '0';
      } else if(state == 5 && c == ')') {
        total += num1 * num2;
        state = 0;
        num1 = 0;
        num2 = 0;
      } else {
        if(state == 0 && c == 'd') {
          state = 6;
        } else if(state == 6 && c == 'o') {
          state++;
        } else if(state == 7 && c == 'n') {
          state++;
        } else if(state == 8 && c == '\'') {
          state++;
        } else if(state == 9 && c == 't') {
          state++;
        } else if(state == 10 && c == '(') {
          state++;
        } else if(state == 11 && c == ')') {
          enable = false;
          state = 0;
          num1 = 0;
          num2 = 0;
        } else {
          state = 0;
          num1 = 0;
          num2 = 0;
        }
      }
    } else {
      if(state == 0 && c == 'd') {
        state++;
      } else if(state == 1 && c == 'o') {
        state++;
      } else if(state == 2 && c == '(') {
        state++;
      } else if(state == 3 && c == ')') {
        state = 0;
        enable = true;
      } else {
        state = 0;
      }
    }
  }

  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("total: %d\n", total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
  free(line);
}
