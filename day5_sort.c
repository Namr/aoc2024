#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

int16_t* lookup_table;
int16_t* lookup_table_limits;

int lookup_cmp(const void* a, const void* b) {
  int16_t aa = *(int16_t*)a;
  int16_t bb = *(int16_t*)b;
  
  for(int i = 0; i < lookup_table_limits[aa]; i++) {
    if(lookup_table[(aa * 100) + i] == bb) {
      return 1;
    }
  }

  for(int i = 0; i < lookup_table_limits[bb]; i++) {
    if(lookup_table[(bb * 100) + i] == aa) {
      return -1;
    }
  }
  
  return 0;
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

  // lookup table (x,y) s.t all x in y are all numbers that cannot preceed y in a valid list
  lookup_table = malloc(100 * 100 * sizeof(int16_t));
  lookup_table_limits = malloc(100 * sizeof(int16_t));
  int total = 0;
  
  int first_num = 0;
  int second_num = 0;
  while(true) {

    // capture number
    first_num = 0;
    while(isdigit(*file)) {
      first_num = (first_num * 10) + *file++ - '0';
    }

    // move over seperator, if its not there we need to process the lists
    if(*file++ != '|') {
      break;
    }

    // capture number
    second_num = 0;
    while(isdigit(*file)) {
      second_num = (second_num * 10) + *file++ - '0';
    }
    
    lookup_table[(second_num * 100) + lookup_table_limits[second_num]++] = first_num;

    // skip new line
    file++; 
  }

  // consume list
  int16_t* list = malloc(100 * sizeof(int16_t));
  int16_t* sorted_list = malloc(100 * sizeof(int16_t));
  int list_size = 0;
  while(file < file_start + input_size) {
    // capture num
    first_num = 0;
    while(isdigit(*file)) {
      first_num = (first_num * 10) + *file++ - '0';
    }

    // push into list
    list[list_size++] = first_num;

    // check if we are done with a list
    if(*file++ == '\n') {
      // for every item in the list
      memcpy(sorted_list, list, list_size * sizeof(int16_t));
      qsort(list, list_size, sizeof(int16_t), lookup_cmp);
      if(memcmp(sorted_list, list, list_size * sizeof(uint16_t))) {
        total += list[list_size / 2];
      }
      list_size = 0;
    }
  }

  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("total: %d\n", total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
  free(lookup_table);
  free(lookup_table_limits);
  free(list);
  free(sorted_list);
}
