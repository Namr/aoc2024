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

typedef struct Point {
  uint8_t x;
  uint8_t y;
} Point;

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

  char* antinode_storage = calloc(input_size, sizeof(uint8_t)); 
  int num_antinodes = 0;
  
  // each possible letter gets its own array to hold 2D positions
  // and some extras to simplify indexing
  uint8_t range = 'z' - '0' + 1;
  uint16_t num_possible_instances = 30;
  Point* symbol_to_occurances = malloc(range * num_possible_instances * sizeof(Point));
  uint16_t* symbol_occurance_count = calloc(range, sizeof(uint16_t));
  
  // first pass, determine bounds
  bool compute_width = true;
  int width = 0;
  int height = 0;
  while(file < file_start + input_size) {
    if(*file == '\n') {
      height++;
      compute_width = false;
    } else if(compute_width){
      width++;
    }
    file++;
  }
  printf("width %d, height %d\n", width, height);
  file = file_start; 

  // second pass, store positions
  for(int x = 0; x < width; x++) {
    for(int y = 0; y < height; y++) {
      char f = file[(y * (width+1)) + x];
      if(isalpha(f) || isdigit(f)) {
        char idx = f - '0';
        Point p = {x, y};
        symbol_to_occurances[(idx * num_possible_instances) + symbol_occurance_count[idx]] = p;
        symbol_occurance_count[idx]++;
      }
    }
  }

  // third pass, compute & count antinodes
  for(int s = 0; s < range; s++) {
    uint16_t count = symbol_occurance_count[s];
    // for every pair of nodes, draw a line and plant an antinode
    for(int i = 0; i < count; i++) {
      for(int w = 0; w < count; w++) {
        if(i == w) { continue; }
        Point a = symbol_to_occurances[(s * num_possible_instances) + i];
        Point b = symbol_to_occurances[(s * num_possible_instances) + w];
        int x_dist = a.x - b.x;
        int y_dist = a.y - b.y;

        // walk in a line and add nodes
        int iter = 0;
        // remove the while true line (and its closing brace) to restore the part 1 answer
        while(true) {
          int node_x = a.x + x_dist * iter;
          int node_y = a.y + y_dist * iter;
          if(node_x < 0 || node_x >= width || node_y < 0 || node_y >= height) {
            break;
          }

          if(antinode_storage[(node_y * width) +  node_x] != '#') {
            antinode_storage[(node_y * width) +  node_x] = '#';
            num_antinodes++;
          }
          iter++;
        }
      }
    }
  }
  
  printf("antinodes: %d\n", num_antinodes);

}
