#include "random.h"
#include<fstream>

//------------------------------------------------------------------------------------------------------------------
// @name                    : RandomGenerator
//
// @description             : Constructor
//
// @returns                 : Nothing
//------------------------------------------------------------------------------------------------------------------
RandomGenerator::RandomGenerator()
{
    m_bIsSeedGenerated = false;
    m_bIsRandomNamesLoaded = false;
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : RandomGenerator
//
// @description             : Destructor
//
// @returns                 : Nothing
//------------------------------------------------------------------------------------------------------------------
RandomGenerator::~RandomGenerator()
{
    m_males.clear();
    m_females.clear();
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : generateSeed
//
// @description             : Generates a seed which is used by the Random Number Generator.
//
// @returns                 : Nothing
//------------------------------------------------------------------------------------------------------------------
void RandomGenerator::generateSeed()
{
    srand((unsigned)time(0));
    m_bIsSeedGenerated = true;
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : generateRandomNumber
//
// @description             : Generate a random number in between [0, range)
//
// @param range             :  Maximum value of number that needs to be generated (not inclusive)
//
// @returns                 : Random unsigned integer.
//------------------------------------------------------------------------------------------------------------------
unsigned int RandomGenerator::generateRandomNumber(unsigned int range)
{
    //unsigned int actualRange = range + 1;

    if (m_bIsSeedGenerated)
    {
        return rand() % range;
    }
    else
    {
        generateSeed();
        return rand() % range;
    }
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : generateRandomString
//
// @description             : Generates a random string. String returned has to be freed by the caller.
//
// @param len:              :  Length of string to generate.
// @param useUppercase      :  Generate uppercase characters 
// @param useNumbers        :  Generate numbers
//
// @returns                 : Pointer to a string present in heap memory, user has responsibility
//                            to free the memory used by this string.
//------------------------------------------------------------------------------------------------------------------
char *RandomGenerator::generateRandomString(int len, bool useUppercase, bool useNumbers)
{
    bool firstShouldBeChar = true;
    const int UPPER_CASE_START = 65; //A
    const int UPPER_CASE_END = 90;  //Z
    const int LOWER_CASE_START = 97;  //a
    const int LOWER_CASE_END = 122;  //z
    const int NUMBER_START = 48;  //0
    const int NUMBER_END = 57;  //9
    const int CHARS = 26;
    const int NUMBERS = 10;

    int strLen = len + 1;
    char *output = (char *)malloc(strLen * sizeof(char));
    memset(output, 0, strLen);

    int i = 0;
    int val = 0;
    unsigned int valId = 0;

    /* Generate 1st as a lower case character */
    if (firstShouldBeChar && i == 0)
    {
        val = LOWER_CASE_START + generateRandomNumber(CHARS);
        output[i] = val;
        i++;
    }

    while (i < len)
    {
        valId = generateRandomNumber(90);

        if (useUppercase && valId < 30)
        {
            //Generate uppercase
            val = UPPER_CASE_START + generateRandomNumber(CHARS);
        }
        else if (useNumbers && valId < 60)
        {
            //Generate number
            val = NUMBER_START + generateRandomNumber(NUMBERS);
        }
        else
        {
            // Genereate lowercase
            val = LOWER_CASE_START + generateRandomNumber(CHARS);
        }

        output[i] = val;
        i++;
    }

    output[i] = '\0';

    return output;
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : InitRandomNames
//
// @description             : Loads the males and females file containing list of random names.
//
// @returns                 : True if both files loaded successfully, else false.
//------------------------------------------------------------------------------------------------------------------
bool RandomGenerator::InitRandomNames()
{
    // Load male names list
    fstream fileStream;
    fileStream.open(MALES_FILE, ios::in);
    if (!fileStream)
    {
        printf("File [ %s ] NOT found!\n", MALES_FILE.c_str());
        return false;
    }

    string line;
    while (getline(fileStream, line))
    {
        m_males.push_back(line);
    }

    printf("Loaded %ld male names\n", m_males.size());

    // Close male file
    fileStream.close();


    // Load female names list
    fileStream.open(FEMALES_FILE, ios::in);
    if (!fileStream)
    {
        printf("File [ %s ] NOT found!\n", FEMALES_FILE.c_str());
        return false;
    }

    while (getline(fileStream, line))
    {
        m_females.push_back(line);
    }

    printf("Loaded %ld female names\n", m_females.size());

    // Close female file
    fileStream.close();

    m_bIsRandomNamesLoaded = true;
    printf("Total names in record: %ld\n", m_males.size() + m_females.size());
    return true;
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : GetRandomMaleName
//
// @description             : Gets a random male name
//
// @returns                 : Random name
//------------------------------------------------------------------------------------------------------------------
string RandomGenerator::GetRandomMaleName()
{
    if (m_bIsRandomNamesLoaded && m_males.size())
    {
        int index = generateRandomNumber(m_males.size());
        return m_males[index];
    }
    else
    {
        printf("Names list not loaded! Please use InitRandomNames()\n");
        return "";
    }
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : GetRandomFemaleName
//
// @description             : Gets a random female name
//
// @returns                 : Random name
//------------------------------------------------------------------------------------------------------------------
string RandomGenerator::GetRandomFemaleName()
{
    if (m_bIsRandomNamesLoaded && m_females.size())
    {
        int index = generateRandomNumber(m_females.size());
        return m_females[index];
    }
    else
    {
        printf("Names list not loaded! Please use InitRandomNames()\n");
        return "";
    }
}

//------------------------------------------------------------------------------------------------------------------
// @name                    : GetRandomName
//
// @description             : Gets a random name
//
// @returns                 : Random name
//------------------------------------------------------------------------------------------------------------------
string RandomGenerator::GetRandomName()
{
    // Randomly choose a male/female
    int genderRandom = generateRandomNumber(10);

    if (genderRandom < 5)
    {
        return GetRandomMaleName();
    }
    else
    {
        return GetRandomFemaleName();
    }
}