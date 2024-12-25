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

#define STEP_MAX 75
#define H_BITS 18
#define TABLE_MAX 262144

int hash_func(uint64_t a, uint64_t b) {
  uint64_t ha = a * 11400714819323198549ul;
  uint64_t hb = b * 11400714819323198549ul;
  uint64_t fh = ha ^ (hb + 0x9e3779b9 + (ha << 6) + (ha >> 2));
  return fh >> (64 - H_BITS);
}

typedef struct Node {
  uint64_t val;
  uint64_t step;
  uint64_t count;
  Node* next;
} Node;

Node* get(Node** map, uint64_t val, uint64_t step) {
  Node* direct = map[hash_func(val, step)]; 
  while(direct != NULL) {
    if(direct->val == val && direct->step == step) {
      return direct;
    }
    direct = direct->next;
  }
  return NULL;
}

Node* set(Node** map, uint64_t val, uint64_t step, uint64_t count) {
  Node* last = NULL;
  Node* direct = map[hash_func(val, step)];
  while(direct != NULL) {
    if(direct->val == val && direct->step == step) {
      direct->count = count;
      return direct;
    }
    last = direct;
    direct = direct->next;
  }

  Node* fresh = (Node*) malloc(sizeof(Node));
  fresh->val = val;
  fresh->step = step;
  fresh->count = count;
  fresh->next = NULL;
  if(last == NULL) {
    map[hash_func(val, step)] = fresh;
  } else {
    last->next = fresh;
  }

  return fresh;
}

uint64_t count_digits(uint64_t val) {
  if(val < 10) return 1;
  return 1 + count_digits(val / 10);
}

uint64_t blink(Node** memo, uint64_t val, uint64_t step) {
  if(step >= STEP_MAX) {
    return 1;
  }
  
  Node* find = get(memo, val, step);
  if(find == NULL) {
    uint64_t digits = count_digits(val);
    if(val == 0) {
      set(memo, val, step, blink(memo, 1, step + 1));
    } else if(digits % 2 == 0) {
      uint64_t divisor = pow(10, digits / 2);
      uint64_t front = val / divisor;
      uint64_t back = val % divisor;
      set(memo, val, step, blink(memo, front, step + 1) + blink(memo, back, step + 1));
    } else {
      set(memo, val, step, blink(memo, val * 2024, step + 1));
    }
  }
  return get(memo, val, step)->count;
}

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement");
    return 1;
  }
  
  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = (char*) mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;

  uint64_t* stones = (uint64_t*) malloc(30 * sizeof(uint64_t));
  // ruh roh
  Node** map = (Node**) malloc(TABLE_MAX * sizeof(Node*));
  for (int i = 0; i < TABLE_MAX; i++) {
    map[i] = NULL;
  }

  // load initial stones into a linked list
  size_t num_stones = 0;
  while(file < file_start + input_size - 1) {
    uint64_t curr_num = 0;
    while(isdigit(*file)) {
      curr_num = (curr_num * 10) + *file++ - '0';
    }
    stones[num_stones++] = curr_num;
    file++;
  }
  
  uint64_t stone_count = 0;
  for(int i = 0; i < num_stones; i++) {
    stone_count += blink(map, stones[i], 0);
  }
  printf("stone count: %ld\n", stone_count);
}
