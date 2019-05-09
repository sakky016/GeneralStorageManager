#ifndef SM_H
#define SM_H
#include<unordered_map>
#include<map>

using namespace std;

// Uncomment for testing purpose with debug enabled and small 
// number of test cases
#define TEST

//----------------------------------------------------------------------------------------------
// Instead of malloc, these macros should be used to allocate memory. For C++ style allocation
// new and delete have been overriden so they will automatically use our Storage manager.
//----------------------------------------------------------------------------------------------
#define SM_ALLOC_ARRAY(type, size)      (type *)sm.SM_alloc(size * sizeof(type))
#define SM_ALLOC(type)                  (type *)sm.SM_alloc(sizeof(type))
#define SM_DEALLOC(ptr)                 sm.SM_dealloc(ptr)

//----------------------------------------------------------------------------------------------
// Structs
//----------------------------------------------------------------------------------------------
typedef struct 
{
    size_t size;
    bool isFree;
}sm_metaData_t;

//----------------------------------------------------------------------------------------------
// StorageManager class
//----------------------------------------------------------------------------------------------
class StorageManager
{
private:
    char *m_chunkPtr;
    char *m_currentPtr;
    size_t m_chunkTotalSize;
    size_t m_chunkUsedSize;
    unsigned long long m_countChunkAllocs;
    unsigned long long m_countMemoryMapAllocs;
    unsigned long long m_countCacheAllocs;
    unsigned long long m_countFrees;
    map<char *, sm_metaData_t> m_memoryMap;

    // Cache memory
    char* m_cacheBlock;
    size_t m_cacheBlockSize;

public:
    StorageManager(int size);
    ~StorageManager();
    bool InitStorageManager(size_t size);
    void *SM_alloc(size_t size);
    void SM_dealloc(void *ptr);
    char* FindNextFreeSpaceInMemoryMap(char *ptr);
    char* FindFreeSpaceInMemoryMap();
    size_t FindFreeSpaceSizeInMemoryMap();
    char* GetMemoryFromMap(size_t size);
    int DefragmentMemoryMap();
    int HandleFragmentedMemory(char *ptr, sm_metaData_t & metaData, char *nextOccupiedBlock);
    char* FetchMemoryIfAvailable(const size_t size, char *ptrToCheck, sm_metaData_t & metaData);
    void DisplayMemoryStats();
    void DisplayMemoryMapDetails();
    void DisplayCacheMemoryDetails();
};


//----------------------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------------------
// Not allowing new and delete override for now
//void * operator new (size_t size);
//void * operator new[](size_t size);
//void operator delete (void* ptr);
//void operator delete[] (void* ptr);


#endif
