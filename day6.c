#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

int dir_from_vel(int vel_x, int vel_y) {
  if(vel_x == 0 && vel_y == -1) {
    return 0;
  } else if(vel_x == 1 && vel_y == 0) {
    return 1;
  } else if(vel_x == 0 && vel_y == 1) {
    return 2;
  } else {
    return 3;
  }
}

int check_if_loops(char* map, char* history_buf, int guard_x, int guard_y, int max_x, int max_y) {
  int vel_x = 0;
  int vel_y = -1;
  int total = 0;
  while(true) {
    int next_x = guard_x + vel_x; 
    int next_y = guard_y + vel_y;
    if(next_x >= 0 && next_x < max_x && next_y >= 0 && next_y < max_y) {
      // is it an obstacle?
      if(map[(next_y * (max_x+1)) + next_x] == '#') {
        // see how many times we've hit this with this direction
        int dir = dir_from_vel(vel_x, vel_y);
        if(++history_buf[(dir * max_x * max_y) + (next_y * max_x) + next_x] > 1) {
          return 1;
        }

        // rotate 90, I swear inverse tan is slower than this
        if(vel_x == 0 && vel_y == -1) {
          vel_x = 1;
          vel_y = 0;
        } else if(vel_x == 1 && vel_y == 0) {
          vel_y = 1;
          vel_x = 0;
        } else if(vel_x == 0 && vel_y == 1) {
          vel_x = -1;
          vel_y = 0;
        } else {
          vel_x = 0;
          vel_y = -1;
        }
      } else {
        guard_x = next_x;
        guard_y = next_y;
      } 
    } else {
      // out of bounds, were done
      return 0;
      break;
    }
  }
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
  char* file_start = mmap(NULL, input_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  char* file_copy = (char*) malloc(input_size * sizeof(char));
  memcpy(file_copy, file_start, input_size * sizeof(char));
  char* file = file_start;
  
  // find bounds
  int max_x = 0;
  int max_y = 0;
  int guard_index = 0;
  while(file < file_start + input_size) {
    if(*file == '\n') {
      max_y++;
    } else if(*file == '^'){
      guard_index = max_x;
      max_x++;
    } else {
      max_x++;
    }
    file++;
  }
  max_x /= max_y;
  file = file_start;
  
  int guard_y = guard_index / max_y;
  int guard_x = guard_index % max_y;
  int vel_x = 0;
  int vel_y = -1;
  file_copy[(guard_y * (max_x+1)) + guard_x] = 'S';
  while(true) {
    int next_x = guard_x + vel_x; 
    int next_y = guard_y + vel_y;
    if(next_x >= 0 && next_x < max_x && next_y >= 0 && next_y < max_y) {
      // is it an obstacle?
      if(file[(next_y * (max_x+1)) + next_x] == '#') {
        // rotate 90, I swear inverse tan is slower than this
        if(vel_x == 0 && vel_y == -1) {
          vel_x = 1;
          vel_y = 0;
        } else if(vel_x == 1 && vel_y == 0) {
          vel_y = 1;
          vel_x = 0;
        } else if(vel_x == 0 && vel_y == 1) {
          vel_x = -1;
          vel_y = 0;
        } else {
          vel_x = 0;
          vel_y = -1;
        }
        file_copy[(guard_y * (max_x+1)) + guard_x] = '+';
      } else {
        guard_x = next_x;
        guard_y = next_y;
        if(file_copy[(guard_y * (max_x+1)) + guard_x] != 'S') {
          file_copy[(guard_y * (max_x+1)) + guard_x] = 'X';
        }
      } 
    } else {
      // out of bounds, were done
      break;
    }
  }

  int steps = 0;
  int num_blocks = 0;
  char* filec = file_copy;
  char* history_buffer = (char*) calloc(4 * max_x * max_y, sizeof(char));
  file = file_start;
  guard_y = guard_index / max_y;
  guard_x = guard_index % max_y;
  while(filec < file_copy + input_size) {
    if(*filec == 'X' || *filec == '+') {
      char old = *file;
      *file = '#';
      num_blocks += check_if_loops(file_start, history_buffer, guard_x, guard_y, max_x, max_y);
      memset(history_buffer, 0, 4 * max_x * max_y * sizeof(char));
      *file = old;
      steps++;
    } else if(*filec == 'S') {
      steps++;
    }
    file++;
    filec++;
  }
  
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("steps taken: %d\n", steps);
  printf("blocks: %d\n", num_blocks);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
}
