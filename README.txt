In this project we have implemented separate virtual memories for two cases:
PART 1 ==> virtual memory which is equal to physical memory in terms of their size
PART 2 ==> virtual memory which is 4 times bigger than physical memory

In part 1 there was no need for page swiping since page table entry size was equal to physical memory
entry size. When a page fault occurred we mapped the new page to a free memory location, then updated the page table

In order to run part1 
gcc part1.c 
./a.out BACKING_STORE.bin addresses.txt 

In part 2 due to the physical memory frame amount being less than virtual memory page amount, we had to implement page swapping by using FIFO and LRU algorithms. For FIFO we kept track of the number of page fault and if this number became bigger than physical memory frame amount, we mapped the new page to the page_fault % total_frame. For LRU we kept a external lru_count array which represents the pages's most recent usage order. Then if our physical memory become full, we find the frame which is mapped to the page with lowest order number and mapped the new page to it.

In order to run part2
gcc part2.c
./a.out BACKING_STORE.bin addresses.txt -p <0 or 1> 
0 for FIFO
1 for LRU