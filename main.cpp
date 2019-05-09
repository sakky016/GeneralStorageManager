#include <chrono>
#include"random.h"
#include"sm.h"
#include<iostream>
#include<stdio.h>

//----------------------------------------------------------------------------------------------
// Configurations
//----------------------------------------------------------------------------------------------
#ifdef TEST
// Number of rounds of simulation
const unsigned int REPEATS = 100;
#else
const unsigned int REPEATS = 250000;
#endif

// Max length of allocated string (bytes)
const int MAX_LEN = 100;

// Enter a value between (0 - 100). 0 - means no deallocations will be done. 100 means 
// 1 deallocation will be attempted in every cycle. Higher the number of deallocations
// per cycle, greater is the performance of our custom Storage manager. This is 
// because cache memory will be available more often.
const int DO_DEALLOCS_PERCENT = 95;

// Enable both to simulate comparatively
const bool USE_STORAGE_MANAGER = true;
const bool USE_NATIVE_MALLOC = true;

//----------------------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------------------
RandomGenerator rng;
extern StorageManager sm;
unsigned long long g_countAllocs = 0;
unsigned long long g_countAllocsFailed = 0;
unsigned long long g_countFrees = 0;

//----------------------------------------------------------------------------------------------
// @name                    : DisplayStats
//
// @description             : DisplayStats of allocation/deallocation
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void DisplayStats() 
{
    printf("\n");
    printf("+-------------------------------------------------------+\n");
    printf("|                 Simulation Statistics                 |\n");
    printf("+-------------------------------------------------------+\n");
    printf("| Successful Allocs      : %-12llu                 |\n", g_countAllocs);
    printf("| Failed Allocs          : %-12llu                 |\n", g_countAllocsFailed);
    printf("| Frees                  : %-12llu                 |\n", g_countFrees);
    printf("+-------------------------------------------------------+\n");
}

//----------------------------------------------------------------------------------------------
// @name                    : ResetCounts
//
// @description             : Reset global counters used for the simulation. This is called
//                            at the start of each simulation.
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void ResetCounts()
{
    g_countAllocs = 0;
    g_countAllocsFailed = 0;
    g_countFrees = 0;
}

//----------------------------------------------------------------------------------------------
// @name                    : getCurrentTimestampInMilliseconds
//
// @description             : Get time stamp in milliseconds.
//
// @returns                 : timestamp in milliseconds
//----------------------------------------------------------------------------------------------
long long getCurrentTimestampInMilliseconds()
{
    long long ts_ms = chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::
                      now().time_since_epoch()).count();
    return ts_ms;
}

//----------------------------------------------------------------------------------------------
// @name                    : Cleanup
//
// @description             : Free the allocated memory. This is called at the end of simulation.
//
// @returns                 : Nothing
//----------------------------------------------------------------------------------------------
void Cleanup(vector<char *> & allocatedMemory, bool useStorageManager)
{
    for (size_t i = 0; i < allocatedMemory.size(); i++)
    {
        if (useStorageManager)
        {
            SM_DEALLOC(allocatedMemory[i]);
        }
        else
        {
            if (allocatedMemory[i])
            {
                free(allocatedMemory[i]);
                allocatedMemory[i] = nullptr;
            }
        }
    }

    allocatedMemory.clear();
}

//----------------------------------------------------------------------------------------------
// @name                    : DoSimulation
//
// @description             : Simulate the test. 
//
// @returns                 : Time taken for test to run (in milliseconds)
//----------------------------------------------------------------------------------------------
long long DoSimulation(const vector<unsigned int> & rngList, bool useStorageManager)
{
    ResetCounts();

    printf("\nRunning simulation with %s\n", useStorageManager ? "Storage Manager" : "Native malloc");

    char *ptr = nullptr;
    unsigned int len = 0;
    vector<char *> allocatedMemory;
    long long timeStart = 0;
    long long timeEnd = 0;

    timeStart = getCurrentTimestampInMilliseconds();

    for (size_t i = 0; i < rngList.size(); i++)
    {
        len = rngList[i];
        if (useStorageManager)
        {
            ptr = SM_ALLOC_ARRAY(char, len + 1);
        }
        else
        {
            ptr = (char *)malloc(sizeof(char) * (len + 1));
        }

        if (ptr)
        {
            memset(ptr, 0, len + 1);
            g_countAllocs++;

            // Create a sample string of len
            string tmp_string;
            for (size_t i = 0; i < len; i++)
            {
                tmp_string.push_back('A');
            }

            strncpy(ptr, tmp_string.c_str(), len);
            allocatedMemory.push_back(ptr);

            if (DO_DEALLOCS_PERCENT)
            {
                // Randomly delete an allocated memory
                if (rng.generateRandomNumber(100) < DO_DEALLOCS_PERCENT)
                {
                    int vectorIndex = rng.generateRandomNumber(allocatedMemory.size());
                    char *ptrToDeallocate = allocatedMemory[vectorIndex];
                    if (ptrToDeallocate)
                    {
                        if (useStorageManager)
                        {
                            SM_DEALLOC(ptrToDeallocate);
                        }
                        else
                        {
                            free(ptrToDeallocate);
                            ptrToDeallocate = nullptr;
                        }

                        allocatedMemory.erase(allocatedMemory.begin() + vectorIndex);
                        g_countFrees++;
                    }
                }
            } // Dealloc
        }
        else
        {
            //cout << "*** Allocation failed!" << endl;
            g_countAllocsFailed++;
        }
    }// Simulation ends here

    timeEnd = getCurrentTimestampInMilliseconds();

    // Cleanup the memory used by simulation
    Cleanup(allocatedMemory, useStorageManager);

    // Display statistics
    if (useStorageManager)
    {
        //sm.DisplayMemoryMapDetails();
        sm.DisplayMemoryStats();
    }

    DisplayStats();

    return (timeEnd - timeStart);
}

//----------------------------------------------------------------------------------------------
//            M A I N
//----------------------------------------------------------------------------------------------
int main()
{
    // Generate random len of string 1st
    vector<unsigned int> rngList;
    for (size_t i = 0; i < REPEATS; i++)
    {
        // We are ensuring that we do not do allocation for 0 bytes
        rngList.push_back(1 + rng.generateRandomNumber(MAX_LEN));
    }

    long long timeRequired1 = 0;
    long long timeRequired2 = 0;
    bool useStorageManager = false;

    // Simulate using native malloc and free
    if (USE_NATIVE_MALLOC)
    {
        
        timeRequired1 = DoSimulation(rngList, useStorageManager);
        cout << endl << "** Time required (using native malloc)   : " << timeRequired1 << " ms" << endl << endl;
    }

    // Simulate using StorageManager alloc/dealloc
    if (USE_STORAGE_MANAGER)
    {
        useStorageManager = true;
        timeRequired2 = DoSimulation(rngList, useStorageManager);
        cout << endl << "** Time required (using storage manager) : " << timeRequired2 << " ms" << endl << endl;
    }

    float result = ((float)(timeRequired1 - timeRequired2) / timeRequired1) * 100;
    printf("\n*** Time comparison of Storage manager: %f %%\n", result);

    getchar();
    return 0;
}