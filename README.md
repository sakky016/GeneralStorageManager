# GeneralStorageManager
Implements a custom variable sized storage manager. General working is described below:  

1) Allocates a large chunk of heap memory on initialization. This reduces the overhead of multiple malloc system call.  
2) Initially, all the allocations are done through this chunk. The allocated memory information is stored in a memory map.
3) Once the chunk memory gets used up, then the memory freed earlier, are re-used.
4) For re-using the memory, the memory map is searched for availability of free block, if available it is re-used.
5) For faster allocation of re-usable memory, caching is implemented. It tries to keep the largest freed block information in cache.  
6) Additionally, defragmentation of memory is also carried out. Since, by re-use of memory blocks, there could arise situations in which several consecutive free blocks are present. We try to merge these consecutive free blocks into one so that it can be re-used in a better way.
