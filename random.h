#ifndef _RANDOM_H_
#define _RANDOM_H_
#include<time.h>
#include <stdlib.h>
#include<string>
#include<vector>

using namespace std;

//---------------------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------------------
const string MALES_FILE = "male.txt";
const string FEMALES_FILE = "female.txt";

//---------------------------------------------------------------------------------------
// Random generator class
//---------------------------------------------------------------------------------------
class RandomGenerator
{
private:
    bool m_bIsSeedGenerated;
    bool m_bIsRandomNamesLoaded;
    vector<string> m_males;
    vector<string> m_females;

    /* Private functions */
    void generateSeed();

public:
    RandomGenerator();
    ~RandomGenerator();

    unsigned int generateRandomNumber(unsigned int range);
    char* generateRandomString(int len, bool useUppercase, bool useNumbers);
    bool InitRandomNames();
    string GetRandomMaleName();
    string GetRandomFemaleName();
    string GetRandomName();
};

#endif#pragma once
