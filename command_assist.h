#ifndef COMMAND_ASSIST_H_
#define COMMAND_ASSIST_H_

#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <stdbool.h> // bool




const std::string COMMAND_FOLLOW = "FOLLOW ";
const std::string COMMAND_UNFOLLOW = "UNFOLLOW ";
const std::string COMMAND_LIST = "LIST";
const std::string COMMAND_TIMELINE = "TIMELINE";


// convert to upper case but only for nCharacters specified
void toUpperCaseWithLimit(std::string str, const int nCharacters)
{
	int i;
    for (i = 0; i < nCharacters; i++)
        str[i] = toupper(str[i]);
}



// compare case insensetively, a given string with known reference in caprital letters for length of referenceSize are compared
bool doStringsMatchCaseInsensitive(std::string str1, const std::string referenceInCapital)
{
    int str1Size = str1.size();
    int referenceSize = referenceInCapital.size();

    if (str1Size >= referenceSize)
    {
        toUpperCaseWithLimit(str1, referenceSize); // Changing to capital
        if (str1.compare(0, referenceSize, referenceInCapital) == 0) // checking if command start with DELETE
        {
            return true;
        }
    }
    return false;
}



// extract chat room name from command message replied by client, make sure command does not contain the last enter and instead is null terminated
std::string extractUserName(const std::string command, const std::string commandType)
{
    int commandSize = command.size();
    // printf("command type size: %d\n", commandSize);
    
    int commandTypeSize = commandType.size();
    // printf("commandTypeSize: %d\n", commandTypeSize);

    int nameSize = commandSize - commandTypeSize;
    // printf("Name Size: %d\n", nameSize);

    if (nameSize > 0)
    { 
        return command.substr(commandTypeSize);
    }
    else
    {
        return "";
    }
}



#endif // COMMAND_ASSIST_H_