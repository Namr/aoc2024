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

typedef struct Node {
  uint64_t val;
  struct Node* next;
} Node;

void print_list(Node* start) {
  while(start != NULL) {
    printf("%ld ", start->val);
    start = start->next;
  }
  printf("\n");
}

void push_back(Node* node, uint64_t val) {
  while(node->next != NULL) { node = node->next; };
  
  Node* new = malloc(sizeof(Node));
  new->val = val;
  new->next = NULL;
  node->next = new;
}

void insert_after(Node* node, uint64_t val) {
  Node* new = malloc(sizeof(Node));
  new->val = val;
  if(node->next == NULL) {
    new->next = NULL;
    node->next = new;
  } else {
    new->next = node->next;
    node->next = new;
  }
}

uint64_t count_digits(uint64_t val) {
  if(val < 10) return 1;
  return 1 + count_digits(val / 10);
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
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  // sentinel start
  Node* start = malloc(sizeof(Node));
  start->next = NULL;
  start->val = (uint64_t) -1;
  
  // load initial stones into a linked list
  uint64_t stone_count = 0;
  while(file < file_start + input_size - 1) {
    uint64_t curr_num = 0;
    while(isdigit(*file)) {
      curr_num = (curr_num * 10) + *file++ - '0';
    }
    push_back(start, curr_num); // you could just keep track of the end and insert after, would be faster
    stone_count++;
    file++;
  }
  
  for(int i = 0; i < 75; i++) {
    Node* head = start->next;
    while(head != NULL) {
      uint64_t digits = count_digits(head->val);
      if(head->val == 0) {
        head->val = 1;
      } else if(digits % 2 == 0) {
        uint64_t divisor = pow(10, digits / 2);
        uint64_t back = head->val % divisor;
        // printf("%ld %ld %ld\n", head->val, digits, divisor);
        head->val = head->val / divisor;
        insert_after(head, back);
        stone_count++;
        head = head->next;
      } else {
        head->val *= 2024;
      }
      head = head->next;
    }
    printf("tick %d\n", i);
  }

  printf("stone count: %ld\n", stone_count);
  file = file_start; 
}
