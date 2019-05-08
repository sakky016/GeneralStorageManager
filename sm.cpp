#include "sm.h"
#include<iostream> 
#include<stdlib.h> 

// Storage manager initial size
const unsigned int SM_SIZE = 10 * 1024 * 1024;  // bytes
const bool DEBUG = false;
const bool DO_DEFRAGMENTATION = true;

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
    m_countFrees = 0;
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
        if (DEBUG)
            cout << "  Allocating from recycled memory" << endl;

        ptr = GetMemoryFromMap(size);
        if (ptr)
        {
            m_countMemoryMapAllocs++;
        }

        if (DEBUG)
            DisplayMemoryMapDetails();
    }

    // Update metadata and memory map if a valid memory is allocated
    if (ptr)
    {
        // Fill up the meta data
        metaData.isFree = false;
        metaData.size = size;
        m_memoryMap[ptr] = metaData;

        if (DEBUG)
            printf("  Allocated 0x%lu\n", ptr);
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
        printf("Custom dealloc for %lu\n", (char*)ptr);

    if (ptr == nullptr)
    {
        return;
    }

    auto it = m_memoryMap.find((char*)ptr);
    if (it != m_memoryMap.end())
    {
        // Do not actually deallocate memory, Mark it as free
        it->second.isFree = true;
        m_countFrees++;
    }
    else
    {
        cout << "Invalid memory address provided!" << endl;
    }

    sm_metaData_t & metaData = it->second;
    if (DO_DEFRAGMENTATION)
    {
        HandleFragmentedMemory((char*)ptr, metaData);

        if (DEBUG)
        {
            DisplayMemoryMapDetails();
        }
    }
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
    size_t freeSpaceInMemoryMap = FindFreeSpaceInMemoryMap();
    printf("+-------------------------------------------------------+\n");
    printf("|              Storage Manager Statistics               |\n");
    printf("+-------------------------------------------------------+\n");
    printf("| Total chunk size                 : %-12ld bytes |\n", m_chunkTotalSize);
    printf("| Used chunk size                  : %-12ld bytes |\n", m_chunkUsedSize);
    printf("| Available chunk size             : %-12ld bytes |\n", m_chunkTotalSize - m_chunkUsedSize);
    printf("| Reusable recycled memory size    : %-12ld bytes |\n", freeSpaceInMemoryMap);
    printf("| Total Allocs                     : %-12llu       |\n", m_countChunkAllocs + m_countMemoryMapAllocs);
    printf("|  a) From memory chunk            : %-12llu       |\n", m_countChunkAllocs);
    printf("|  b) From recycled memory         : %-12llu       |\n", m_countMemoryMapAllocs);
    printf("| Total Frees                      : %-12llu       |\n", m_countFrees);
    printf("+-------------------------------------------------------+\n");
}

//----------------------------------------------------------------------------------------------
// @name                    : FindFreeSpaceInMemoryMap
//
// @description             : Finds out the total size of free memory available in the 
//                            memory map.
//
// @returns                 : Total Size of free memory in Memory map
//----------------------------------------------------------------------------------------------
size_t StorageManager::FindFreeSpaceInMemoryMap()
{
    size_t freeSpaceInMemoryMap = 0;
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t metaData = it->second;
        if (metaData.isFree)
        {
            freeSpaceInMemoryMap += metaData.size;
        }
    }

    return freeSpaceInMemoryMap;
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
    printf("\nFree space in memory map:\n");
    size_t freeSpaceInMemoryMap = 0;
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t metaData = it->second;
        printf("0x%lu : %lu bytes  <%s>\n", it->first, metaData.size, metaData.isFree ? "Free" : "Occupied");
    }
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
    for (auto it = m_memoryMap.begin(); it != m_memoryMap.end(); it++)
    {
        sm_metaData_t & metaData = it->second;

        if (DEBUG && metaData.isFree)
        {
            printf("Required: %u, inMap: %u\n", size, metaData.size);
        }

        if (metaData.isFree == true && metaData.size >= size)
        {
            ptr = it->first;
            size_t originalBlockSize = metaData.size;
            metaData.size = size;

            // What to do if there is some memory left after this allocation?
            if (originalBlockSize > size)
            {
                sm_metaData_t fragmentedMetaData;
                fragmentedMetaData.isFree = true;
                fragmentedMetaData.size = originalBlockSize - size;
                char *fragmentedPtr = ptr + size;

                if (DEBUG)
                    printf("Adding fragmented block 0x%lu to memory map\n", fragmentedPtr);

                if (DO_DEFRAGMENTATION)
                {
                    HandleFragmentedMemory(fragmentedPtr, fragmentedMetaData);
                }

                // Add the defragmented memory to the map
                m_memoryMap[fragmentedPtr] = fragmentedMetaData;
            }

            // Found space from recycled memory
            break;
        }
    }

    return ptr;
}

//----------------------------------------------------------------------------------------------
// @name                    : HandleFragmentedMemory
//
// @description             : Once a particular block has been marked as free, we check if
//                            a larger block can be formed by merging this with a neighbouring
//                            free block.
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void StorageManager::HandleFragmentedMemory(char *ptr, sm_metaData_t & metaData)
{
    if (ptr >= m_chunkPtr + m_chunkTotalSize)
    {
        printf("Defragment Info: Reached end of chunk. No merge possible\n");
        return;
    }

    char *nextBlock = ptr + metaData.size;
    if (nextBlock >= m_chunkPtr + m_chunkTotalSize)
    {
        printf("Defragment Info: Reached end of chunk. No merge possible \n");
        return;
    }

    // Check if this is free
    auto it = m_memoryMap.find(nextBlock);
    if (it != m_memoryMap.end())
    {
        sm_metaData_t nextMetaData = it->second;
        if (nextMetaData.isFree == true)
        {
            if (DEBUG)
                printf("Merging %lu --> %lu bytes\n", metaData.size, metaData.size + nextMetaData.size);
            
            // Update size of the merged block
            metaData.size += nextMetaData.size;

            // remove next block's entry from map since it will 
            // get merged to previous block
            m_memoryMap.erase(it);

            // Check if further blocks can be merged
            HandleFragmentedMemory(ptr, metaData);
        }
        else
        {
            // DO NOTHING
            // Next block not free. Merge not possible
        }
    }
}
