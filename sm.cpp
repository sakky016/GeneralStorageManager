#include<assert.h>
#include "sm.h"
#include<iostream> 
#include<stdlib.h> 

#ifdef TEST
// Storage manager initial size
const unsigned int SM_SIZE = 1000;  // bytes
const bool DEBUG = true;
#else
const unsigned int SM_SIZE = 1024 * 1024 * 1024;  // bytes
const bool DEBUG = false;
#endif

const bool DO_DEFRAGMENTATION = true;
const bool USE_CACHE = true;

//----------------------------------------------------------------------------------------------
// Create global Storage Manager object which will be used by entire system
//----------------------------------------------------------------------------------------------
StorageManager sm(SM_SIZE);

// Not allowing new and delete override for now
//----------------------------------------------------------------------------------------------
// Overriding new and delete operators to use our Storage Manager
//----------------------------------------------------------------------------------------------
//void * operator new (size_t size)
//{
//    return sm.SM_alloc(size);
//}
//
//void * operator new[](size_t size)
//{
//    return  sm.SM_alloc(size);
//}
//
//void operator delete (void* ptr)
//{
//    sm.SM_dealloc(ptr);
//}
//
//void operator delete[](void* ptr)
//{
//    sm.SM_dealloc(ptr);
//}

//----------------------------------------------------------------------------------------------
// @name                    : StorageManager
//
// @description             : Constructor
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
StorageManager::StorageManager(int size)
{
    if (!InitStorageManager(size))
    {
        printf("\n *** FATAL ERROR: InitStorageManager: Cannot proceed!\n");
        return;
    }

    m_chunkTotalSize = size;
    m_chunkUsedSize = 0;
    m_countChunkAllocs = 0;
    m_countMemoryMapAllocs = 0;
    m_countCacheAllocs = 0;
    m_countFrees = 0;
    m_cacheBlockSize = 0;
    m_cacheBlock = nullptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : StorageManager
//
// @description             : Destructor
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
StorageManager::~StorageManager()
{
    free(m_chunkPtr);
    m_chunkPtr = nullptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : InitStorageManager
//
// @description             : TODO
//
// @param size              : Memory chunk allocated on init.
//
// @returns                 : true on success, false otherwise.
//----------------------------------------------------------------------------------------------
bool StorageManager::InitStorageManager(size_t size)
{
    m_chunkPtr = (char *)malloc(size);
    if (m_chunkPtr)
    {
        printf("Storage Manager initialized with %lu bytes\n", size);
        m_currentPtr = m_chunkPtr;
        memset(m_chunkPtr, 0, size);
        //cout << "Chunk content: " << m_chunkPtr << endl;
        return true;
    }

    printf("Storage Manager failed to allocate %lu bytes\n", size);
    return false;
}

//----------------------------------------------------------------------------------------------
// @name                    : SM_alloc
//
// @description             : This function is called from SM_ALLOC_ARRAY macro. 
//
// @param size              : Size of memory requested for heap allocation.
//
// @returns                 : Pointer to start of the allocated memory
//----------------------------------------------------------------------------------------------
void * StorageManager::SM_alloc(size_t size)
{
    char *ptr = nullptr;
    sm_metaData_t metaData;

    if (size == 0)
    {
        return ptr;
    }

    if (DEBUG)
        cout << "\nCustom alloc for " << size << " bytes" << endl;

    // Allocate from chunk. 
    if (m_chunkTotalSize - m_chunkUsedSize >= size)
    {
        if (DEBUG)
            cout << "  Allocating from chunk" << endl;

        ptr = m_currentPtr;
        if (ptr)
        {
            m_countChunkAllocs++;
            m_chunkUsedSize += size;
            m_currentPtr = m_currentPtr + size;
        }
    }
    else
    {
        // Allocate from recycled memory
        ptr = GetMemoryFromMap(size);
    }

    // Update metadata and memory map if a valid memory is allocated
    if (ptr)
    {
        // Fill up the meta data
        metaData.isFree = false;
        metaData.size = size;
        m_memoryMap[ptr] = metaData;

        if (DEBUG)
        {
            printf("  Allocated 0x%lu\n", ptr);
            DisplayMemoryMapDetails();
        }
    }

    return ptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : SM_dealloc
//
// @description             : This function is called from SM_DEALLOC macro. It marks the 
//                            memory pointed to by ptr as free. Actual de-allocation DOES NOT
//                            takes place. This memory block is then re-claimed for future
//                            allocations from this pool.
//
// @param ptr               : Pointer to memory that needs to be freed.
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void StorageManager::SM_dealloc(void *ptr)
{
    if (DEBUG)
        printf("\nCustom dealloc for 0x%lu\n", (char*)ptr);

    if (ptr == nullptr)
    {
        return;
    }

    auto it = m_memoryMap.find((char*)ptr);
    if (it != m_memoryMap.end())
    {
        sm_metaData_t & metaData = it->second;

        // Do not actually deallocate memory, Mark it as free
        metaData.isFree = true;

        int defragCount = 0;
        if (DO_DEFRAGMENTATION)
        {
            defragCount = HandleFragmentedMemory((char*)ptr, metaData, nullptr);
        }

        // Update the cache block if the size of this freed block 
        // is larger than the current cache block
        if (it->second.size > m_cacheBlockSize)
        {
            m_cacheBlockSize = metaData.size;
            m_cacheBlock = it->first;
        }

        if (DEBUG)
        {
            if (defragCount)
            {
                printf ("  Memory map Defragmentation done %d times\n", defragCount);
            }
            
            DisplayCacheMemoryDetails();
            DisplayMemoryMapDetails();
        }

        m_countFrees++;
    }
    else
    {
        cout << "*** DEALLOC ERROR: Invalid memory address provided!" << endl;
    }
}

//----------------------------------------------------------------------------------------------
// @name                    : FindNextFreeSpaceInMemoryMap
//
// @description             : Finds out the free block in Memory map next to the given block.
//
// @returns                 : If a free block is found, then pointer to free block, 
//                            nullptr otherwise.
//----------------------------------------------------------------------------------------------
char* StorageManager::FindNextFreeSpaceInMemoryMap(char *ptr)
{
    auto it = m_memoryMap.find(ptr);
    if (it != m_memoryMap.end())
    {
        while (it != m_memoryMap.end())
        {
            sm_metaData_t metaData = it->second;
            if (metaData.isFree && it->first != ptr)
            {
                return it->first;
            }

            it++;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : FindFreeSpaceInMemoryMap
//
// @description             : Finds out the first free block in Memory map.
//
// @returns                 : If a free block is found, then pointer to free block, 
//                            nullptr otherwise.
//----------------------------------------------------------------------------------------------
char* StorageManager::FindFreeSpaceInMemoryMap()
{
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t metaData = it->second;
        if (metaData.isFree)
        {
            return it->first;
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : FindFreeSpaceSizeInMemoryMap
//
// @description             : Finds out the first free block in Memory map.
//
// @returns                 : If a free block is found, then pointer to free block, 
//                            nullptr otherwise.
//----------------------------------------------------------------------------------------------
size_t StorageManager::FindFreeSpaceSizeInMemoryMap()
{
    size_t totalFreeSize = 0;

    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t metaData = it->second;
        if (metaData.isFree)
        {
            totalFreeSize += metaData.size;
        }
    }

    return totalFreeSize;
}

//----------------------------------------------------------------------------------------------
// @name                    : FetchMemoryIfAvailable
//
// @description             : Given the memory address, this function checks if it is marked 
//                            as free in the memory map and has adequate size to cater to
//                            the requested alloc size. It also merges the leftover space
//                            with the subsequent free blocks (if available). In this process
//                            it also tries to check additional free blocks and merges (defragments) 
//                            them to form a single large block.
//
// @returns                 : pointer to memory in memory map
//----------------------------------------------------------------------------------------------
inline char *StorageManager::FetchMemoryIfAvailable(const size_t size, char *ptrToCheck, sm_metaData_t & metaData)
{
    char *ptr = nullptr; 

    if (DEBUG && metaData.isFree)
    {
        printf("  Required: %u, inMap: %u\n", size, metaData.size);
    }

    if (metaData.isFree == true && metaData.size >= size)
    {
        ptr = ptrToCheck;
        size_t originalBlockSize = metaData.size;
        metaData.size = size;
        metaData.isFree = false;

        // What to do if there is some memory left after this allocation?
        if (originalBlockSize > size)
        {
            sm_metaData_t fragmentedMetaData;
            fragmentedMetaData.isFree = true;
            fragmentedMetaData.size = originalBlockSize - size;
            char *fragmentedPtr = ptr + size;

            int defragCount = 0;
            if (DO_DEFRAGMENTATION)
            {
                defragCount = HandleFragmentedMemory(fragmentedPtr, fragmentedMetaData, nullptr);
            }

            // Add the defragmented memory to the map
            m_memoryMap[fragmentedPtr] = fragmentedMetaData;
            
            if (DEBUG)
            {
                printf("Adding fragmented block 0x%lu to memory map\n", fragmentedPtr);

                if (defragCount)
                {
                    printf("  Defragmentation done %d times\n", defragCount);
                }

                DisplayMemoryMapDetails();
            }
        }
    }

    return ptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : GetMemoryFromMap
//
// @description             : Looks for a memory block of sufficient size in the memory map. This
//                            function needs to be optimized so as to make a quick search of 
//                            next free block from the reusable memory map.
//
// @returns                 : pointer to free memory in Memory map (recycled memory)
//----------------------------------------------------------------------------------------------
char* StorageManager::GetMemoryFromMap(size_t size)
{
    char *ptr = nullptr;

    if (USE_CACHE)
    {
        if (DEBUG)
        {
            DisplayCacheMemoryDetails();
        }

        // Check if memory can be allocated from the cached block
        if (m_cacheBlockSize >= size)
        {
            sm_metaData_t & metaData = m_memoryMap[m_cacheBlock];
            ptr = FetchMemoryIfAvailable(size, m_cacheBlock, metaData);

            if (ptr)
            {
                if (DEBUG)
                {
                    printf("  Adding block 0x%lu from cache\n", ptr);
                }

                m_countCacheAllocs++;

                // Update the cache with the 1st available free block
                // in memory map. This is sort of a compromise as we
                // could have obtained the largest free block, but that 
                // would involve traversing the memory map, which we are
                // avoiding. This is the reason cache was brough in the first
                // place - to avoid memory map traversal on every 
                // allocation request.
                m_cacheBlock = FindFreeSpaceInMemoryMap();
                if (m_cacheBlock)
                {
                    m_cacheBlockSize = m_memoryMap[m_cacheBlock].size;
                }
                else
                {
                    // No free space avaiable in Memory map. Set the 
                    // cache to NULL
                    m_cacheBlock = nullptr;
                    m_cacheBlockSize = 0;
                }

                return ptr;
            }
            else
            {
                if (DEBUG)
                    printf("  Not found in cache\n");
            }
        }
    } // Use of cache

    // Required M/m not found in cache, look in the entire Memory Map
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t & metaData = it->second;
        char *ptrToCheck = it->first;
        ptr = FetchMemoryIfAvailable(size, ptrToCheck, metaData);
        if (ptr)
        {
            if (DEBUG)
            {
                printf("  Adding block 0x%lu from Memory map\n", ptr);
            }

            m_countMemoryMapAllocs++;
            break;
        }
    }

    return ptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : DefragmentMemoryMap
//
// @description             : NOT USED. Looks for consecutive free blocks and merges them.
//
// @returns                 : Number of times defragmentation was done
//----------------------------------------------------------------------------------------------
int StorageManager::DefragmentMemoryMap()
{
    int count = 0;
    bool done = false;
    char *curBlock = FindNextFreeSpaceInMemoryMap(m_chunkPtr);
    char *nextBlock = nullptr;

    if (DEBUG)
        printf("  Defragmenting memory map...\n");

    do
    {
        sm_metaData_t & metaData = m_memoryMap[curBlock];
        count += HandleFragmentedMemory(curBlock, metaData, nextBlock);
        if (nextBlock == nullptr)
        {
            curBlock = FindNextFreeSpaceInMemoryMap(curBlock);
        }
        else
        {
            curBlock = nextBlock;
        }
    } while (curBlock != nullptr && curBlock < m_chunkPtr + m_chunkTotalSize);

    return count;
}

//----------------------------------------------------------------------------------------------
// @name                    : HandleFragmentedMemory
//
// @description             : Once a particular block has been marked as free, we check if
//                            a larger block can be formed by merging this with the next
//                            free block. 
//                            Limitation: If a free block lies before the current block, then
//                                        this function would not be able to merge it.
//                                        TODO: Some mechanism to detect previous free blocks.
//
// @param ptr               : Current block address
// @param metaData          : Reference of meta data for current block
// @param nextOccupiedBlock : [OUTPUT] Will store the address of the next occupied block
//                            address in memory map.
//
// @returns                 : Number of times defragmentation was done
//----------------------------------------------------------------------------------------------
int StorageManager::HandleFragmentedMemory(char *ptr, sm_metaData_t & metaData, char *nextOccupiedBlock)
{
    static int count = 0;
    nextOccupiedBlock = nullptr;

    if (ptr == nullptr)
    {
        return count;
    }

    if (ptr >= m_chunkPtr + m_chunkTotalSize)
    {
        if (DEBUG)
        {
            printf("  Defragment Info: Reached end of chunk. No merge possible\n");
        }

        return count;
    }

    char *nextBlock = ptr + metaData.size;
    if (nextBlock >= m_chunkPtr + m_chunkTotalSize)
    {
        if (DEBUG)
        {
            printf("  Defragment Info: Reached end of chunk. No merge possible\n");
        }

        return count;
    }

    // Check if this is free
    auto it = m_memoryMap.find(nextBlock);
    if (it != m_memoryMap.end())
    {
        sm_metaData_t nextMetaData = it->second;
        if (nextMetaData.isFree == true)
        {
            if (DEBUG)
                printf("  Merging %lu --> %lu bytes\n", metaData.size, metaData.size + nextMetaData.size);
            
            // Update size of the merged block
            metaData.size += nextMetaData.size;
            count++;

            // remove next block's entry from map since it will 
            // get merged to previous block
            m_memoryMap.erase(it);

            // Check if further blocks can be merged, call this function recursively
            count = HandleFragmentedMemory(ptr, metaData, nextOccupiedBlock);
        }
        else
        {
            // Next block not free. Merge not possible.
            // Store the address of the next non-free block.
            nextOccupiedBlock = it->first;
        }
    }
    else
    {
        //if (DEBUG)
            //printf("  Defragmentation: Next block 0x%lu not present in Memory Map!\n", nextBlock);
    }

    int countToReturn = count;
    count = 0;
    return countToReturn;
}

//----------------------------------------------------------------------------------------------
// @name                    : DisplayCacheMemoryDetails
//
// @description             : Cache memory stats
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void StorageManager::DisplayCacheMemoryDetails()
{
    printf("  Cache block     : 0x%lu %u bytes\n", m_cacheBlock, m_cacheBlockSize);
}

//----------------------------------------------------------------------------------------------
// @name                    : DisplayMemoryMapDetails
//
// @description             : Displays the address, size and free/occupied status of memory map
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void StorageManager::DisplayMemoryMapDetails()
{
    printf("\n");
    printf("+-----------------------------------------------+\n");
    printf("|               Memory map                      |\n");
    printf("+-----------------------------------------------+\n");
    size_t freeSpaceInMemoryMap = 0;
    unsigned int index = 1;
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t metaData = it->second;
        printf("| %3d) 0x%lu : %-4lu bytes   <%-8s>     |\n", index, it->first, metaData.size, metaData.isFree ? "  Free  " : "Occupied");
        index++;
    }
    printf("+-----------------------------------------------+\n");
}

//----------------------------------------------------------------------------------------------
// @name                    : DisplayMemoryStats
//
// @description             : Memory usage statistics
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void StorageManager::DisplayMemoryStats()
{
    size_t freeSpaceInMemoryMap = FindFreeSpaceSizeInMemoryMap();
    unsigned long long totalAllocs = m_countChunkAllocs + m_countMemoryMapAllocs + m_countCacheAllocs;

    printf("+----------------------------------------------------------+\n");
    printf("|               Storage Manager Statistics                 |\n");
    printf("+----------------------------------------------------------+\n");
    printf("| 1) Total chunk size                 : %-12ld bytes |\n", m_chunkTotalSize);
    printf("| 2) Used chunk size                  : %-12ld bytes |\n", m_chunkUsedSize);
    printf("| 3) Available chunk size             : %-12ld bytes |\n", m_chunkTotalSize - m_chunkUsedSize);
    printf("| 4) Reusable recycled memory size    : %-12ld bytes |\n", freeSpaceInMemoryMap);
    printf("| 5) Total Allocs                     : %-12llu       |\n", totalAllocs);
    printf("|     a) From memory chunk            : %-12llu       |\n", m_countChunkAllocs);
    printf("|     b) From recycled memory         : %-12llu       |\n", m_countMemoryMapAllocs);
    printf("|     c) From cache memory            : %-12llu       |\n", m_countCacheAllocs);
    printf("| 6) Total Frees                      : %-12llu       |\n", m_countFrees);
    printf("+----------------------------------------------------------+\n");
}