#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

int line_width;
int num_lines;
char* file_start;

// -1 = error, 0 = continue, 1 = done
int xmas_forward_sm(int* state, char c) {
  if(*state == 0 && c == 'X') {
    (*state)++;
  } else if(*state == 1 && c == 'M') {
    (*state)++;
  } else if(*state == 2 && c == 'A') {
    (*state)++;
  } else if(*state == 3 && c == 'S') {
    (*state) = 0;
    return 1;
  } else if(c == 'X') {
    (*state) = 1;
  } else {
    (*state) = 0;
  }
  return 0;
}

// -1 = error, 0 = continue, 1 = done
int xmas_backward_sm(int* state, char c) {
  if(*state == 0 && c == 'S') {
    (*state)++;
  } else if(*state == 1 && c == 'A') {
    (*state)++;
  } else if(*state == 2 && c == 'M') {
    (*state)++;
  } else if(*state == 3 && c == 'X') {
    (*state) = 0;
    return 1;
  } else if(c == 'S') {
    (*state) = 1;
  } else {
    (*state) = 0;
  }
  return 0;
}

int mas_forward_sm(int* state, char c) {
  if(*state == 0 && c == 'M') {
    (*state)++;
  } else if(*state == 1 && c == 'A') {
    (*state)++;
  } else if(*state == 2 && c == 'S') {
    (*state) = 0;
    return 1;
  } else if(c == 'M') {
    (*state) = 1;
  } else {
    (*state) = 0;
  }
  return 0;
}

// -1 = error, 0 = continue, 1 = done
int mas_backward_sm(int* state, char c) {
  if(*state == 0 && c == 'S') {
    (*state)++;
  } else if(*state == 1 && c == 'A') {
    (*state)++;
  } else if(*state == 2 && c == 'M') {
    (*state) = 0;
    return 1;
  } else if(c == 'S') {
    (*state) = 1;
  } else {
    (*state) = 0;
  }
  return 0;
}

int xmas_diag_down_right(int x, int y) {
  int forward_state = 0;
  int backward_state = 0;
  int total = 0;
  for(int i = 0; i < 4; i++) {
    if(x >= 0 && x < line_width && y >= 0 && y < num_lines) {
      char c = file_start[(y * (line_width+1)) + x];
      total += xmas_forward_sm(&forward_state, c);
      total += xmas_backward_sm(&backward_state, c);
    }
    x++;
    y++;
  }
  return total;
}

int xmas_diag_down_left(int x, int y) {
  int forward_state = 0;
  int backward_state = 0;
  int total = 0;
  for(int i = 0; i < 4; i++) {
    if(x >= 0 && x < line_width && y >= 0 && y < num_lines) {
      char c = file_start[(y * (line_width+1)) + x];
      total += xmas_forward_sm(&forward_state, c);
      total += xmas_backward_sm(&backward_state, c);
    }
    x--;
    y++;
  }
  return total;
}

// assumes its started on an A
int mas_finder(int x, int y) {
  int forward_state = 0;
  int backward_state = 0;
  int x_dr = x - 1;
  int y_dr = y - 1;
  int total_dr = 0;
  for(int i = 0; i < 3; i++) {
    if(x_dr >= 0 && x_dr < line_width && y_dr >= 0 && y_dr < num_lines) {
      char c = file_start[(y_dr * (line_width+1)) + x_dr];
      total_dr += mas_forward_sm(&forward_state, c);
      total_dr += mas_backward_sm(&backward_state, c);
    }
    x_dr++;
    y_dr++;
  }

  forward_state = 0;
  backward_state = 0;
  int x_dl = x + 1;
  int y_dl = y - 1;
  int total_dl = 0;
  for(int i = 0; i < 3; i++) {
    if(x_dl >= 0 && x_dl < line_width && y_dl >= 0 && y_dl < num_lines) {
      char c = file_start[(y_dl * (line_width+1)) + x_dl];
      total_dl += mas_forward_sm(&forward_state, c);
      total_dl += mas_backward_sm(&backward_state, c);
    }
    x_dl--;
    y_dl++;
  }

  return 1 ? total_dl > 0 && total_dr > 0 : 0;
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
  file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  line_width = 0;
  num_lines = 0;
  while(*file++ != '\n') {line_width++;}
  file = file_start;
  while(file != file_start + input_size) {
    if(*file++ == '\n') {
      num_lines++;
    }
  }
  
  int total = 0;
  int p2_total = 0;
  // forward pass
  file = file_start;
  int forward_state = 0;
  int backward_state = 0;
  int status = 0;
  for(int y = 0; y < num_lines; y++) {
    forward_state = 0;
    backward_state = 0;
    for(int x = 0; x < line_width; x++) {
      char c = file[(y * (line_width+1)) + x];
      total += xmas_forward_sm(&forward_state, c);
      total += xmas_backward_sm(&backward_state, c);

      if(c == 'X' || c == 'S') {
        total += xmas_diag_down_right(x, y);
        total += xmas_diag_down_left(x, y);
      }

      if(c == 'A') { // just an optimization
        p2_total += mas_finder(x, y);
      }
    }
  }

  int down_state = 0;
  int up_state = 0;
  for(int x = 0; x < line_width; x++) {
    forward_state = 0;
    down_state = 0;
    for(int y = 0; y < num_lines; y++) {
      char c = file[(y * (line_width+1)) + x];
      total += xmas_forward_sm(&forward_state, c);
      total += xmas_backward_sm(&backward_state, c);
    }
  }

  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("p1 total: %d\n", total);
  printf("p2 total: %d\n", p2_total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
}
