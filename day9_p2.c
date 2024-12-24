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

typedef struct Partition {
  int64_t id;
  uint8_t size;
} Partition;

void print_partitions(Partition* partitions, size_t num_partitions) { 
  for(int i = 0; i < num_partitions; i++) {
    for(int w = 0; w < partitions[i].size; w++) {
      if(partitions[i].id == -1) {
        printf(".");
      } else {
        printf("%ld", partitions[i].id);
      }
    }
  }
  printf("\n");
}

void insert(Partition item, Partition* partitions, size_t* num_partitions, size_t idx) {
  // shift all elements after idx to the right
  for(size_t i = *num_partitions; i > idx; i--) {
    partitions[i] = partitions[i-1];
  }
  // set idx to be the 
  partitions[idx] = item;
  (*num_partitions)++;
}

void delete(Partition* partitions, size_t* num_partitions, size_t idx) {
  // shift all elements after idx to the left
  for(size_t i = idx; i < (*num_partitions)-1; i++) {
    partitions[i] = partitions[i+1];
  }
  (*num_partitions)--;
}

void merge(Partition* partitions, size_t* num_partitions) {
  for(int i = 0; i < (*num_partitions)-1; i++) {
    if(partitions[i].id == partitions[i+1].id) {
      partitions[i].size += partitions[i+1].size;
      delete(partitions, num_partitions, i+1);
      i--;
    }
  }
}

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
  
  Partition* partitions = malloc(input_size * sizeof(Partition));
  size_t num_partitions = 0;

  // compute how many blocks are in the file
  int64_t id = 0;
  bool is_free = false;
  while(file < file_start + input_size - 1) {
    uint8_t size = *file++ - '0';
    int64_t tmp_id = is_free ? -1 : id++;
    Partition p = {tmp_id, size};
    partitions[num_partitions++] = p;
    is_free = !is_free;
  }
  file = file_start;
 
  size_t next_to_move = num_partitions - 1;
  while(next_to_move > 0) {

    if(partitions[next_to_move].id != -1) {
      
      size_t next_free_idx = 0;
      while(next_free_idx < num_partitions && (partitions[next_to_move].size > partitions[next_free_idx].size || partitions[next_free_idx].id != -1)) {
        next_free_idx++;
      }

      if(next_to_move <= next_free_idx) {
        next_to_move--;
        continue;
      }

      if(partitions[next_to_move].size == partitions[next_free_idx].size) {
        // just swap
        partitions[next_free_idx].id = partitions[next_to_move].id;
        partitions[next_to_move].id = -1;
      } else if(partitions[next_to_move].size < partitions[next_free_idx].size) {
        uint8_t size_diff = partitions[next_free_idx].size - partitions[next_to_move].size;
        // swap
        partitions[next_free_idx] = partitions[next_to_move];
        partitions[next_to_move].id = -1;
        
        // add a new free space
        Partition split_free = {-1, size_diff};
        insert(split_free, partitions, &num_partitions, next_free_idx + 1);
      }
      merge(partitions, &num_partitions);
    }
    next_to_move--;
  }
  
  uint64_t checksum = 0;
  uint64_t idx = 0;
  for(size_t i = 0; i < num_partitions; i++) {
    for(size_t w = 0; w < partitions[i].size; w++) {
      if(partitions[i].id != -1) {
        checksum += partitions[i].id * idx;
      }
      idx++;
    }
  }
  printf("checksum: %ld\n", checksum);
}
