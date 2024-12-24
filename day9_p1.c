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

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement");
  }

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  
  // compute how many blocks are in the file
  uint64_t num_blocks = 0;
  while(file < file_start + input_size - 1) { // minus one is to skip the terminating newline
    num_blocks += *file++ - '0';
  }
  file = file_start;
  
  // populate blocks
  int64_t* blocks = malloc(num_blocks * sizeof(int64_t));
  bool is_free = false;
  int64_t id = 0;
  uint64_t idx = 0;
  while(file < file_start + input_size - 1) { // minus one is to skip the terminating newline
    int value = *file++ - '0';
    int64_t tmp_id = is_free ? -1 : id;
    if(!is_free) {
      id++;
    }
    for(int i = 0; i < value; i++) {
      blocks[idx++] = tmp_id;
    }
    is_free = !is_free;
  }

  uint64_t free_idx = 0;
  uint64_t pull_idx = num_blocks - 1;
  // march to the next free block
  while(blocks[free_idx] != -1) {
    free_idx++;
  }
  while(pull_idx >= 0) {
    if(blocks[pull_idx] != -1) {
      blocks[free_idx] = blocks[pull_idx];
      blocks[pull_idx] = -1;
    }
    pull_idx--;
    
    while(blocks[free_idx] != -1) {
      free_idx++;
    }

    if(pull_idx <= free_idx) {
      break;
    }
  }
  
  uint64_t checksum = 0;
  for(uint64_t i = 0; i < num_blocks; i++) {
    if(blocks[i] != -1) {
      checksum += i * blocks[i]; 
    }
  } 
  printf("%ld\n", checksum);
}
