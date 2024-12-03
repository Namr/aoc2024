#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

bool is_line_safe(int* line, int size, int* violating_idx) {
  int increasing = -1; // -1 = unknown, 0 = false, 1 = true
  for(int i = 1; i < size; i++) {
    int dist = line[i-1]  - line[i];
    int dist_abs = abs(dist);
    if(increasing == -1) {
      increasing = dist < 0;
    } else if(dist < 0 != increasing){
      *violating_idx = i;
      return false;
    }
    
    if(dist_abs > 3 || dist_abs <= 0) {
      *violating_idx = i;
      return false;
    }
  }
  return true;
}

bool is_line_safe_with_exclusion(int* line, int size, int skip_idx) {
  int increasing = -1; // -1 = unknown, 0 = false, 1 = true
  int pi = 1;
  int bi = 0;
  if(skip_idx == 0) {
    bi++;
    pi++;
  }

  while(pi < size) {
    if(bi == skip_idx) {
      bi++;
    } else if(pi == skip_idx) {
      pi++;
      if(pi >= size) {
        break;
      }
    }

    int dist = line[bi]  - line[pi];
    int dist_abs = abs(dist);
    if(increasing == -1) {
      increasing = dist < 0;
    } else if(dist < 0 != increasing){
      return false;
    }
    
    if(dist_abs > 3 || dist_abs <= 0) {
      return false;
    }

    bi++;
    pi++;
  }
  return true;
}

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

  int total = 0;
  int curr_num = 0;
  bool safe = true;
  while(file < file_start + input_size) {
    // capture number
    curr_num = 0;
    while(isdigit(*file)) {
      curr_num = (curr_num * 10) + *file++ - '0';
    }
    line[line_size++] = curr_num;
    
    // move over white space & handle line end
    while(isspace(*file)) {
      if(*file++ == '\n') {
        int violating_idx = -1;
        if(is_line_safe(line, line_size, &violating_idx)) {
          total++;
        } else {
          int tmp = 0;
          for(int i = violating_idx - 2; i < violating_idx + 1; i++) {
            if(is_line_safe_with_exclusion(line, line_size, i)) {
              total++;
              break;
            }
          }
        }
        line_size = 0;
      }
    }
  }

  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("safe: %d\n", total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
  free(line);
}
