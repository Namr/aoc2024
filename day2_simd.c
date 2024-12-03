#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <arm_neon.h>

int8_t clear3[8] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int8_t clear4[8] = {0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00};
int8_t clear5[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
int8_t clear6[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
int8_t clear7[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
int8_t clear8[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

bool is_line_safe(int8_t* line, int8_t size) {
  // compute all distances at once
  // compute all absolute values at once
  // compare all values to 0 at the same time
  // compare all values to 3 at the same time 
  int8x8_t simd_line = vld1_s8(line);
  int8x8_t simd_offset_line = vld1_s8(line + 1);
  int8x8_t distances = vsub_s8(simd_line, simd_offset_line);
  int8x8_t absolute_distances = vabs_s8(distances);
  int8x8_t one = vcreate_s8(0x0101010101010101);
  int8x8_t zero = vcreate_s8(0x0000000000000000);
  int8x8_t three = vcreate_s8(0x0303030303030303);
  uint8x8_t less_than_zero = vclt_s8(distances, zero);
  uint8x8_t less_than_or_equal_zero = vcle_s8(absolute_distances, zero);
  uint8x8_t greater_than_three = vcgt_s8(absolute_distances, three);
  
  // delete any results in the "extra" parts of the vectors
  int8x8_t clear;
  int8x8_t set;
  switch(size) {
    case 3:
      clear = vld1_s8((int8_t*) &clear3);
      break;
    case 4:
      clear = vld1_s8((int8_t*) &clear4);
      break;
    case 5:
      clear = vld1_s8((int8_t*) &clear5);
      break;
    case 6:
      clear = vld1_s8((int8_t*) &clear6);
      break;
    case 7:
      clear = vld1_s8((int8_t*) &clear7);
      break;
    case 8:
      clear = vld1_s8((int8_t*) &clear8);
      break;
  }
  less_than_zero = vand_u8(less_than_zero, clear);
  less_than_or_equal_zero = vand_u8(less_than_or_equal_zero, clear);
  greater_than_three = vand_u8(greater_than_three, clear);
  
  // clamp from 255 -> 1 after the comparisons
  less_than_zero = vmin_u8(less_than_zero, one);
  less_than_or_equal_zero = vmin_u8(less_than_or_equal_zero, one);
  greater_than_three = vmin_u8(greater_than_three, one);

  // sum of less than zero must be zero or size
  // sum of less than or equal to zero must be 0
  // sum of greater_than_three must be 0
  uint8_t lt0_sum = vaddv_u8(less_than_zero);
  uint8_t le0_sum = vaddv_u8(less_than_or_equal_zero);
  uint8_t gt3_sum = vaddv_u8(greater_than_three);
  
  return (lt0_sum == 0 || lt0_sum == (size - 1)) && le0_sum == 0 && gt3_sum == 0;
}

void copy_line_with_exclusion(int8_t* line, int8_t* line_copy, int8_t size, int8_t skip_idx) {
  int i = 0;
  int ii = 0;
  while(i != size) {
    if(i != skip_idx) {
      line_copy[ii++] = line[i];
    }
    i++;
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
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  int8_t* line = calloc(9, sizeof(int8_t));
  int8_t* line_cpy = calloc(9, sizeof(int8_t));
  int8_t line_size = 0;

  int total = 0;
  int8_t curr_num = 0;
  bool safe = true;
  while(file < file_start + input_size) {
    // capture number
    curr_num = 0;
    while(isdigit(*file)) {
      curr_num = (curr_num * 10) + *file++ - '0';
    }
    line[line_size++] = curr_num;
    
    // move over white space & handle line end
    while(isspace(*file)) {
      if(*file++ == '\n') {
        int8_t violating_idx = -1;
        if(is_line_safe(line, line_size)) {
          total++;
        } else {
          for(int8_t i = 0; i < line_size; i++) {
            copy_line_with_exclusion(line, line_cpy, line_size, i);
            if(is_line_safe(line_cpy, line_size - 1)) {
              total++;
              break;
            }
          }
        }
        line_size = 0;
      }
    }
  }

  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));
  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  printf("safe: %d\n", total);
  printf("compute time: %lu usec \n", compute_time);

  munmap(file_start, input_size);
  free(line);
}
