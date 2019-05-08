#include <chrono>
#include"random.h"
#include"sm.h"
#include<iostream>
#include<stdio.h>

//----------------------------------------------------------------------------------------------
// Configurations
//----------------------------------------------------------------------------------------------
const unsigned int REPEATS = 210000;   // Number of rounds of simulation
const int MAX_LEN = 100;              // Max length of allocation (bytes)
const bool DO_DEALLOCS = true;
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
// DisplayStats of allocation/deallocation
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
// Reset counts
//----------------------------------------------------------------------------------------------
void ResetCounts()
{
    g_countAllocs = 0;
    g_countAllocsFailed = 0;
    g_countFrees = 0;
}

//----------------------------------------------------------------------------------------------
// Get time stamp in milliseconds.
//----------------------------------------------------------------------------------------------
long long getCurrentTimestampInMilliseconds()
{
    long long ts_ms = chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::
                      now().time_since_epoch()).count();
    return ts_ms;
}

//----------------------------------------------------------------------------------------------
// Simulation
//----------------------------------------------------------------------------------------------
void DoSimulation(const vector<unsigned int> & rngList, bool useStorageManager)
{
    ResetCounts();

    printf("\nRunning simulation with %s\n", useStorageManager ? "Storage Manager" : "Native malloc");

    char *ptr = nullptr;
    unsigned int len = 0;

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
            for (int i = 0; i < len; i++)
            {
                tmp_string.push_back('A');
            }

            strncpy(ptr, tmp_string.c_str(), len);

            if (DO_DEALLOCS)
            {
                // Do deallocations randomly
                if (rng.generateRandomNumber(10) < 5)
                {
                    if (useStorageManager)
                    {
                        SM_DEALLOC(ptr);
                    }
                    else
                    {
                        free(ptr);
                        ptr = nullptr;
                    }
                    g_countFrees++;
                }
            }
        }
        else
        {
            cout << "*** Allocation failed!" << endl;
            g_countAllocsFailed++;
        }
    }

    if (useStorageManager)
    {
        sm.DisplayMemoryStats();
    }

    DisplayStats();
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

    long long timeStart = 0;
    long long timeEnd = 0;
    bool useStorageManager = false;

    // Simulate using native malloc and free
    if (USE_NATIVE_MALLOC)
    {
        timeStart = getCurrentTimestampInMilliseconds();
        DoSimulation(rngList, useStorageManager);
        timeEnd = getCurrentTimestampInMilliseconds();
        cout << endl << "** Time required (using native malloc)   : " << timeEnd - timeStart << " ms" << endl << endl;
    }

    // Simulate using StorageManager alloc/dealloc
    if (USE_STORAGE_MANAGER)
    {
        useStorageManager = true;
        timeStart = getCurrentTimestampInMilliseconds();
        DoSimulation(rngList, useStorageManager);
        timeEnd = getCurrentTimestampInMilliseconds();
        cout << endl << "** Time required (using storage manager) : " << timeEnd - timeStart << " ms" << endl << endl;
    }

    getchar();
    return 0;
}