#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

void move_num_back(int16_t* list, int list_size, int index_to_move, int index_to_move_to) {
  for(int i = index_to_move; i > index_to_move_to; i--) {
    int tmp = list[i];
    list[i] = list[i-1];
    list[i-1] = tmp;
  }
}

bool is_list_ok(int16_t* list, int list_size, int16_t* lookup_table, int16_t* lookup_table_limits) {
  for(int i = 0; i < list_size; i++) {
    int current_item = list[i];
    // look at all items ahead
    for(int c = i+1; c < list_size; c++) {

      // ensure that the item to check is not in the list of banned items for the current item
      int item_to_check = list[c];
      for(int x = 0; x < lookup_table_limits[current_item]; x++) {
        if(lookup_table[(current_item * 100) + x] == item_to_check) {
          move_num_back(list, list_size, c, i);
          is_list_ok(list, list_size, lookup_table, lookup_table_limits);
          return false;
        }
      }
    }
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

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;

  // lookup table (x,y) s.t all x in y are all numbers that cannot preceed y in a valid list
  int16_t* lookup_table = malloc(100 * 100 * sizeof(int16_t));
  int16_t* lookup_table_limits = malloc(100 * sizeof(int16_t));
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
      if(!is_list_ok(list, list_size, lookup_table, lookup_table_limits)) {
        total += list[list_size / 2];
      }

      list_size = 0;
    }
  }

  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("total: %d\n", total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
  free(lookup_table);
  free(lookup_table_limits);
  free(list);
}
