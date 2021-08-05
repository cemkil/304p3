/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
#define PAGE_MASK 1023
#define FRAMES 256

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 1023

#define MAIN_MEMORY_SIZE FRAMES *PAGE_SIZE
#define VIRTUAL_MEMORY_SIZE PAGES *PAGE_SIZE
// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry
{
  int logical;
  int physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];
int lru_count[PAGES];
signed char main_memory[MAIN_MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(int logical_page)
{
  for (size_t i = 0; i < TLB_SIZE; i++)
  {
    struct tlbentry *curr = &tlb[i];
    if (curr->logical == logical_page)
    {
      return curr->physical;
    }
  }
  return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(int logical, int physical)
{
  int nextindex = tlbindex % TLB_SIZE;
  struct tlbentry *next = &tlb[nextindex];
  next->logical = logical;
  next->physical = physical;
  tlbindex++;
}

//find the page number whic is mapped to the input frame.
int find_index_in_page(int frame)
{
  for (size_t i = 0; i < PAGES; i++)
  {
    if (pagetable[i] == frame)
    {
      return i;
    }
  }
  return -1;
}
//find the least recently used page which is mapped to a frame.
int page_with_min_lru(int total_plug){
  int lru_page = -1;
  int least_score = total_plug +1;
  for (size_t i = 0; i < PAGES; i++)
  {
    if ((lru_count[i] < least_score) && pagetable[i] != -1 )
    {
      lru_page = i;
      least_score = lru_count[i];
    }
    
  }
 return lru_page; 
}

int main(int argc, const char *argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, "Usage ./virtmem backingstore input -p <0 or 1> \n");
    exit(1);
  }
  int page_replacement_algo = -1;
  if (strcmp(argv[3], "-p") == 0)
  {

    if (strcmp(argv[4], "0") == 0)
    {
      page_replacement_algo = 0;
    }
    else if (strcmp(argv[4], "1") == 0)
    {
      page_replacement_algo = 1;
    }
  }

  const char *backing_filename = argv[1];
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, VIRTUAL_MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");

  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++)
  {
    pagetable[i] = -1;
    lru_count[i] = 0;
  }

  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];

  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;

  // Number of the next unallocated physical page in main memory
  int free_page = 0;

  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL)
  {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* 
    / Calculate the page offset and logical page number from logical_address */
    //int offset = logical_address % PAGE_SIZE;
    //int logical_page = logical_address / PAGE_SIZE;
    ///////
    int offset = logical_address & OFFSET_MASK;
    int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;

    int physical_page = search_tlb(logical_page);
    // TLB hit
    if (physical_page != -1)
    {
      tlb_hits++;
      // TLB miss
    }
    else
    {
      physical_page = pagetable[logical_page];

      // Page fault
      if (physical_page == -1)
      {
        /* TODO */
        page_faults++;

        //every frames are filled. 
        if (free_page >= FRAMES)
        {
         //fifo algorithm is used to swap pages 
          if (page_replacement_algo == 0)
          { 
            int frame_to_replace = free_page % FRAMES; 
            memcpy(main_memory + frame_to_replace * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            int pagetoreplace = find_index_in_page(frame_to_replace);
            if (pagetoreplace != -1)
            {
              //update page table, make replaced page invalid
              pagetable[pagetoreplace] = -1;
            }
            //map the new logical page to the selected frame
            pagetable[logical_page] = frame_to_replace;
            physical_page = frame_to_replace;
            free_page++;
          }
          //lru algorithm is used to swap pages
          else{
            int pagetoreplace = page_with_min_lru(total_addresses); 
            int frametoreplace = pagetable[pagetoreplace];
            memcpy(main_memory + frametoreplace * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
            //update page table, make replaced page invali
            pagetable[pagetoreplace] = -1;
             //map the new logical page to the selected frame
            pagetable[logical_page] = frametoreplace;
            lru_count[logical_page] = total_addresses;
            physical_page = frametoreplace;
            free_page++; 
          }
        }
        //there are still free frames
        else
        {
          memcpy(main_memory + free_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
          pagetable[logical_page] = free_page;
          physical_page = pagetable[logical_page];
          free_page++;
          //keep lru score of the logical page 
          if (page_replacement_algo == 1)
          {
            lru_count[logical_page] = total_addresses;
          }
          
        }
      }else{
        //update the lru score of the logical page
        if (page_replacement_algo == 1)
        {
          lru_count[logical_page] = total_addresses;
        }
        
      }

      add_to_tlb(logical_page, physical_page);
    }

    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];

    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }

  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

  return 0;
}
