#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>


int cmp_uint32(const void* a, const void* b) {
   return (*(uint32_t*)a - *(uint32_t*)b);
}

typedef struct qsortargs {
  uint32_t* list;
  uint32_t list_size;   
} qsortArgs_t;

void* parallel_qsort(void* pthread_args) {
  qsortArgs_t* args = (qsortArgs_t*) pthread_args;
  qsort(args->list, args->list_size, sizeof(uint32_t), cmp_uint32);
  return NULL;
}

typedef struct sampleSortArgs {
  uint32_t* arr;
  uint32_t* sorted_arr;
  uint32_t size;   
  uint32_t num_samples;   
} sampleSortArgs_t;

/*
 * Basic idea is to split the array into buckets and then sort the buckets.
 * Each bucket will be sorted by a seperate thread.
 */
void* sample_sort_uint32(void* thread_args) {
  sampleSortArgs_t* args = (sampleSortArgs_t*) thread_args;
  
  // randomly select elements
  srand(1000);
  uint32_t num_selectors = args->num_samples-1;
  uint32_t* selectors = malloc(num_selectors * sizeof(uint32_t));
  uint32_t* bucket_start_indicies = malloc(args->num_samples+1 * sizeof(uint32_t));

  pthread_t* thread_handles = malloc(args->num_samples * sizeof(pthread_t));
  qsortArgs_t* qsort_args = malloc(args->num_samples * sizeof(pthread_t));

  for(int i = 0; i < num_selectors; i++) {
    selectors[i] = args->arr[rand() % args->size];
  }
  // sort the selected elements
  qsortArgs_t selector_sort_args = {selectors, num_selectors};
  parallel_qsort(&selector_sort_args);
   
  // push elements into the buckets based on bucket bounds
  // FIXME this has way more loops than needed
  uint32_t lower_bound = 0;
  uint32_t upper_bound = 0;
  uint32_t sorted_arr_idx = 0;
  for(int s = 0; s < num_selectors; s++) {
    lower_bound = upper_bound;
    upper_bound = selectors[s];
    bucket_start_indicies[s] = sorted_arr_idx;
    for(int i = 0; i < args->size; i++) {
      if(args->arr[i] >= lower_bound && args->arr[i] < upper_bound) {
        args->sorted_arr[sorted_arr_idx++] = args->arr[i];
      }
    }
  }
  lower_bound = upper_bound;
  bucket_start_indicies[num_selectors] = sorted_arr_idx;
  bucket_start_indicies[args->num_samples] = args->size;
  for(int i = 0; i < args->size; i++) {
    if(args->arr[i] >= lower_bound) {
      args->sorted_arr[sorted_arr_idx++] = args->arr[i];
    }
  }
   
  // sort buckets in parallel
  for(int s = 0; s < args->num_samples; s++) {
    uint32_t bucket_size = bucket_start_indicies[s+1] - bucket_start_indicies[s];
    qsort_args[s].list = args->sorted_arr + bucket_start_indicies[s];
    qsort_args[s].list_size = bucket_size;
    pthread_create(thread_handles + s, NULL, parallel_qsort, qsort_args + s);
  }

  // wait for sorting to end
  for(int s = 0; s < args->num_samples; s++) {
    pthread_join(thread_handles[s], NULL);
  }

  free(selectors);
  free(bucket_start_indicies);
  free(thread_handles);
  free(qsort_args);
  return NULL;
}

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("You need to enter the input filename as an arguement");
  }
  
  unsigned long cntfrq;
  unsigned long program_start;
  unsigned long program_end;
  unsigned long file_read_start;
  unsigned long file_read_end;
  unsigned long file_parse_start;
  unsigned long file_parse_end;
  unsigned long sort_start;
  unsigned long sort_end;
  unsigned long compute_p1_start;
  unsigned long compute_p1_end;
  unsigned long compute_p2_start;
  unsigned long compute_p2_end;
  asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_start));

  // get file size 
  struct stat st;
  stat(argv[1], &st);
  off_t input_size = st.st_size;
  
  // alloc lists
  uint32_t left_list_size = 0;
  uint32_t* left_list = malloc(input_size * sizeof(uint32_t));
  uint32_t right_list_size = 0;
  uint32_t* right_list = malloc(input_size * sizeof(uint32_t));
  
  // put file in memory
  asm volatile("mrs %0, cntpct_el0" : "=r" (file_read_start));
  int fd = open(argv[1], O_RDONLY);
  char* file_start = mmap(NULL, input_size, PROT_READ, MAP_SHARED, fd, 0);
  char* file = file_start;
  asm volatile("mrs %0, cntpct_el0" : "=r" (file_read_end));
  
  asm volatile("mrs %0, cntpct_el0" : "=r" (file_parse_start));
  // iter over chars, push to list
  uint32_t total = 0;
  while(file < file_start + input_size) {
    // capture first int
    total = 0;
    while(isdigit(*file)) {
      total = (total * 10) + *file++ - '0';
    }
    left_list[left_list_size++] = total;

    // move over white space
    while(isspace(*file)) {
      file++;
    }

    // capture second int
    total = 0;
    while(isdigit(*file)) {
      total = (total * 10) + *file++ - '0';
    }
    right_list[right_list_size++] = total;

    // move over newline
    file++;
  }
  asm volatile("mrs %0, cntpct_el0" : "=r" (file_parse_end));
  
  asm volatile("mrs %0, cntpct_el0" : "=r" (sort_start));
  // sort
  //qsort(left_list, left_list_size, sizeof(uint32_t), cmp_uint32);
  //qsort(right_list, right_list_size, sizeof(uint32_t), cmp_uint32);
  pthread_t left_thread;
  pthread_t right_thread;
  qsortArgs_t left_sort_args = {left_list, left_list_size};
  qsortArgs_t right_sort_args = {right_list, right_list_size};
  
  pthread_create(&left_thread, NULL, parallel_qsort, &left_sort_args);
  pthread_create(&right_thread, NULL, parallel_qsort, &right_sort_args);
  pthread_join(left_thread, NULL);
  pthread_join(right_thread, NULL);

  if(left_list_size != right_list_size) {
    printf("right and left lists were not the same size... should be impossible\n");
  }
  asm volatile("mrs %0, cntpct_el0" : "=r" (sort_end));
  
  asm volatile("mrs %0, cntpct_el0" : "=r" (compute_p1_start));
  // compute dist
  int distance = 0;
  for(int i = 0; i < left_list_size; i++) {
    distance += abs((int) left_list[i] - (int) right_list[i]);
  }
  asm volatile("mrs %0, cntpct_el0" : "=r" (compute_p1_end));

  asm volatile("mrs %0, cntpct_el0" : "=r" (compute_p2_start));
  // compute similarity score
  int similarity_score = 0;
  int r_index = 0;
  int right_count = 0;
  uint32_t last_left_num = -1;
  for(int l_index = 0; l_index < left_list_size; l_index++) {
    if(last_left_num != left_list[l_index]) {
      right_count = 0;
      while(r_index < right_list_size && right_list[r_index] <= left_list[l_index]) {
        if(right_list[r_index] == left_list[l_index]) {
          right_count++;
        }
        r_index++;
      }
    }
    similarity_score += left_list[l_index] * right_count;
    last_left_num = left_list[l_index];
  } 
  asm volatile("mrs %0, cntpct_el0" : "=r" (compute_p2_end));
  asm volatile("mrs %0, cntpct_el0" : "=r" (program_end));

  printf("total distance: %d\n", distance);
  printf("similarity score: %d\n", similarity_score); 

  unsigned long compute_time = ((program_end - program_start) * 1000000) / cntfrq;
  unsigned long file_read_time = ((file_read_end- file_read_start) * 1000000) / cntfrq;
  unsigned long file_parse_time = ((file_parse_end - file_parse_start) * 1000000) / cntfrq;
  unsigned long sort_time = ((sort_end - sort_start) * 1000000) / cntfrq;
  unsigned long p1_time = ((compute_p1_end - compute_p1_start) * 1000000) / cntfrq;
  unsigned long p2_time = ((compute_p2_end - compute_p2_start) * 1000000) / cntfrq;
  printf("total time: %lu usec\n", compute_time);
  printf("read time: %lu usec\n", file_read_time);
  printf("parse time: %lu usec\n", file_parse_time);
  printf("sort time: %lu usec\n", sort_time);
  printf("p1 time: %lu usec\n", p1_time);
  printf("p2 time: %lu usec\n", p2_time);
  free(left_list);
  free(right_list);
  munmap(file_start, input_size);
}
