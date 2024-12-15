#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// operations is a bit array where 0 = + and 1 = *
uint64_t compute_line_total(int* line, int line_size, int operations) {
  uint64_t total = line[0];
  for(int i = 1; i < line_size; i++) {
    if(((operations >> (i-1)) & 1)) {
      total *= line[i];
    } else {
      total += line[i];
    }
  }
  return total;
}

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement");
  }
  
  /*
  unsigned long cntfrq;
  unsigned long program_start;
  unsigned long program_end;
  asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_start));
  */

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;

  int* line = malloc(15 * sizeof(int));
  int line_size = 0;
  int operations = 0; // this is a bitset representing the current permutation of +/* ops being tested

  uint64_t total = 0;
  uint64_t line_total = 0;
  int curr_num = 0;
  while(file < file_start + input_size) {
    // start line
    if(line_total == 0) {
      while(isdigit(*file)) {
        line_total = (line_total * 10) + *file++ - '0';
      }
      file += 2; // skip over colon & space
    } else if(*file == '\n'){
      // end of line
      
      // to test every permutation of binary bits of size n, increment an int up to 2^n
      int op_limit = pow(2, line_size - 1);
      for(int i = 0; i < op_limit; i++) {
        if(compute_line_total(line, line_size, operations++) == line_total) {
          total += line_total;
          break;
        }
      }

      // reset line vars
      line_total = 0; 
      line_size = 0;
      file++;
    } else {
      while(isspace(*file)) { file++; };

      // create list
      curr_num = 0;
      while(isdigit(*file)) {
        curr_num = (curr_num * 10) + *file++ - '0';
      }
      line[line_size++] = curr_num;
    }
  }
  
  /*
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("compute time: %lu usec \n", compute_time);
  */
  printf("total : %ld\n", total);

  munmap(file_start, input_size);
  free(line);
}
