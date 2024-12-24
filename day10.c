#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

void floodfill(char prev_char, int x, int y, int max_x, int max_y, char* map, int* score, int* rating, int* blacklist, int* blacklist_len) { 
  if(x >= max_x || x < 0 || y >= max_y || y < 0) {
    return;
  }
  char current_char = map[(y * (max_x + 1)) + x];

  if(current_char != (prev_char + 1)) {
    return;
  }

  if(current_char == '9') {
    bool fine = true;
    int idx = (y * max_x) + x;
    for(int i = 0; i < *blacklist_len; i++) {
      if(blacklist[i] == idx) {
        fine = false;
      }
    }

    if(fine) {
      *score += 1;
      blacklist[(*blacklist_len)++] = idx;
    }
    *rating += 1;
    return;
  }

  floodfill(current_char, x + 1, y, max_x, max_y, map, score, rating, blacklist, blacklist_len);
  floodfill(current_char, x, y + 1, max_x, max_y, map, score, rating, blacklist, blacklist_len);
  floodfill(current_char, x - 1, y, max_x, max_y, map, score, rating, blacklist, blacklist_len);
  floodfill(current_char, x, y - 1, max_x, max_y, map, score, rating, blacklist, blacklist_len);
}

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement\n");
    return 1;
  }

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // load file
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  char* file = file_start;
  int* potential_trailheads = malloc(1000 * sizeof(int));
  int num_potential_trailheads = 0;

  int* blacklist = malloc(1000 * sizeof(int));
  int  blacklist_len = 0;
  
  // find bounds & mark potential trailhead starts
  int max_x = 0;
  int max_y = 0;
  while(file < file_start + input_size) {
    if(*file == '\n') {
      max_y++;
    } else if (*file == '0') {
      potential_trailheads[num_potential_trailheads++] = max_x;
      max_x++;
    } else {
      max_x++;
    }
    file++;
  }
  max_x /= max_y;
  file = file_start;
  
  int total_score = 0;
  int total_rating = 0;
  for(int i = 0; i < num_potential_trailheads; i++) {
    // extract x,y from idx
    int y = potential_trailheads[i] / max_y;
    int x = potential_trailheads[i] % max_y;
     
    // recursive flood fill search
    blacklist_len = 0;
    floodfill('0'-1, x, y, max_x, max_y, file_start, &total_score, &total_rating, blacklist, &blacklist_len);
  }
  printf("total score: %d\n", total_score);
  printf("total rating: %d\n", total_rating);
  munmap(file_start, input_size);
}
