#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum Error
{
    MEMORY_ALLOCATION = 1,
    ARGUMENT_COUNT,
    WORD_SIZE,
    PATH_ERROR,
    LOGIN_ERROR,
    HOSTNAME_ERROR,
    COMMAND_EXECUTION,
    DIRECTORY_CHANGE,
    NULL_READ,
    MEMORY_CHUNK = 20
};

typedef struct list
{
    char *word;
    struct list *next;
}ListOfWord;

void systemInfo(void)
{
    int position = MEMORY_CHUNK, i = 0, j = 0;
    char path[PATH_MAX], user[LOGIN_NAME_MAX], hostname[HOST_NAME_MAX];
    char *directory;
    directory = malloc(position * sizeof(char));
    if(!directory)
        exit(MEMORY_ALLOCATION);
    if (!(getcwd(path, sizeof(path))))
        exit(PATH_ERROR);
    if ((getlogin_r(user, sizeof(user))))
        exit(LOGIN_ERROR);
    if ((gethostname(hostname, sizeof(hostname))))
        exit(HOSTNAME_ERROR);
    while ((path[i]) != '\0')
    {
        if (j > MEMORY_CHUNK)
        {
            position += MEMORY_CHUNK;
            directory = realloc(directory, position * sizeof(char));
            if(!directory)
                exit(MEMORY_ALLOCATION);
        }
        if (path[i] == '/')
        {
            j = 0;
            position = MEMORY_CHUNK;
            free(directory);
            directory = malloc(position * sizeof(char));
            if(!directory)
                exit(MEMORY_ALLOCATION);
            ++i;
            continue;
        }
        directory[j] = path[i];
        ++i;
        ++j;
    }
    directory[j] = '\0';
    printf("\x1b[32m" "[%s@%s %s]$ " "\x1b[0m", user, hostname, directory);
    free(directory);
}

char* getWord(FILE *filePointer)
{
    char *wordToList;
    char temporarySymbol = 0;
    int quotesFlag = INT_MAX - 1, argument = 0, length = MEMORY_CHUNK, stopFlag = 0;
    int isSpecialSymbol, isBracket, isComparison, isTerm;
    wordToList = malloc(length * sizeof(char));
    if (!wordToList)
        exit(MEMORY_ALLOCATION);
    while ((temporarySymbol = getc(filePointer)) != EOF)
    {
        isBracket = (temporarySymbol == ')')||(temporarySymbol == '(');
        isComparison = (temporarySymbol == '<')||(temporarySymbol == '>');
        isTerm = (temporarySymbol == '|')||(temporarySymbol == '&');
        isSpecialSymbol = isBracket || isComparison || isTerm;
        switch (temporarySymbol)
        {
            case '"':
                quotesFlag -= 1;
                continue;
            case ' ':
                if (argument == 0)
                    continue;
                if ((quotesFlag % 2) == 0)
                    quotesFlag = 0;
                break;
            case '\n':
                wordToList[argument] = '\n';
                ++argument;
                quotesFlag = 0;
                stopFlag = 1;
        }
        if ((isSpecialSymbol)&&(quotesFlag % 2 == 0))
        {
            wordToList = realloc(wordToList, argument + MEMORY_CHUNK);
            if(!wordToList)
                exit(MEMORY_ALLOCATION);
            if(stopFlag == 0)
            {
                if(argument != 0)
                {
                    wordToList[argument] = ' ';
                    ++argument;
                }
                wordToList[argument] = temporarySymbol;
                ++argument;
                quotesFlag = 0;
            }
            else
            {
                wordToList[argument - 1] = ' ';
                wordToList[argument] = temporarySymbol;
                ++argument;
                wordToList[argument] = '\n';
                ++argument;
                quotesFlag = 0;
            }
        }
        if (!quotesFlag)
        {
            wordToList[argument] = '\0';
            return wordToList;
        }
        if ((length - 2) == argument)
        {
            length += MEMORY_CHUNK;
            wordToList = realloc(wordToList, length);
            if (!wordToList)
                exit(WORD_SIZE);
        }
        wordToList[argument] = temporarySymbol;
        ++argument;
    }
    free(wordToList);
    return NULL;
}

ListOfWord* addElement(ListOfWord *lastElement, char *inputWord)
{
    if (!lastElement)
    {
        lastElement = malloc(sizeof(ListOfWord));
        if (!lastElement)
            exit(MEMORY_ALLOCATION);
        lastElement->word = inputWord;
        lastElement->next = NULL;
    }
    else
    {
        lastElement->next = addElement(lastElement->next, inputWord);
    }
    return lastElement;
}

ListOfWord* wordList(ListOfWord *head, FILE *fileDescriptor)
{
    int i = 0;
    char *wordGet = getWord(fileDescriptor);
    if (!(wordGet))
        exit(NULL_READ);
    if(wordGet[0] == '\n')
    {
        free(wordGet);
        return NULL;
    }
    while ((wordGet[i] != '\0')||(wordGet[i] != '\n'))
    {
        if (wordGet[i] == '\0')
        {
            head = addElement(head, wordGet);
            wordGet = getWord(fileDescriptor);
            i = 0;
            continue;
        }
        if (wordGet[i] == '\n')
        {
            wordGet[i] = '\0';
            head = addElement(head, wordGet);
            return head;
        }
        ++i;
    }
    free(wordGet);
    return head;
}

void specialSymbolDivision(ListOfWord *headInput)
{
    char *newWord = NULL;
    int isSpecialSymbol, isBracket, isComparison, isTerm, isLastSpace, i = 0;
    ListOfWord *temporary, *memory, *head;
    head = headInput;
    while(head != NULL)
    {
        while((head->word)[i] != '\0')
        {
            isBracket = ((head->word)[i] == ')')||((head->word)[i] == '(');
            isComparison = ((head->word)[i] == '<')||((head->word)[i] == '>');
            isTerm = ((head->word)[i] == '|')||((head->word)[i] == '&');
            isSpecialSymbol = isBracket || isComparison || isTerm;
            if(i > 0)
                isLastSpace = (head->word[i-1] == ' ');
            else
                isLastSpace = 1;
            if((isSpecialSymbol)&&(isLastSpace))
            {
                newWord = malloc(MEMORY_CHUNK*sizeof(char));
                if(!newWord)
                    exit(MEMORY_ALLOCATION);
                newWord[0] = (head->word)[i];
                newWord[1] = '\0';
                (head->word)[i-1] = '\0';
                temporary = malloc(sizeof(ListOfWord));
                if(!temporary)
                    exit(MEMORY_ALLOCATION);
                if(!temporary)
                {
                    perror("Error: ");
                    exit(MEMORY_ALLOCATION);
                }
                memory = head->next;
                head->next = temporary;
                temporary->word = newWord;
                temporary->next = memory;
                i = 0;
                break;
            }
            ++i;
        }
        head = head->next;
    }
}

void specialSymbolMerge(ListOfWord *headInput)
{
    ListOfWord *head;
    int isSpecialSymbol, isBracket, isComparison, isTerm;
    head = headInput;
    ListOfWord *temporary = NULL;
    while((head->next) != NULL)
    {
        isBracket = ((head->word)[0] == ')')||((head->word)[0] == '(');
        isComparison = ((head->word)[0] == '<')||((head->word)[0] == '>');
        isTerm = ((head->word)[0] == '|')||((head->word)[0] == '&');
        isSpecialSymbol = isBracket || isComparison || isTerm;
        if((isSpecialSymbol)&&((head->word)[1] == '\0'))
            if((head->word)[0] == ((head->next)->word)[0])
            {
                (head->word)[1] = (head->word)[0];
                (head->word)[2] = '\0';
                temporary = (head->next)->next;
                free((head->next)->word);
                free(head->next);
                head->next = temporary;
            }
        head = head->next;
    }
}

char **listToArray(ListOfWord *headOfList)
{
    int position = 0;
    ListOfWord *head = NULL;
    char **array = malloc(sizeof(char*) * MEMORY_CHUNK);
    if(!array)
        exit(MEMORY_ALLOCATION);
    head = headOfList;
    if (!array)
    {
        perror("Error: ");
        exit(MEMORY_ALLOCATION);
    }
    while (head != NULL)
    {
        if (!((position + 1) % MEMORY_CHUNK))
        {
            array = realloc(array, sizeof(char*) * MEMORY_CHUNK);
            if (!array)
            {
                perror("Error: ");
                exit(MEMORY_ALLOCATION);
            }
        }
        array[position++] = head->word;
        head = head->next;
    }
    array[position] = NULL;
    return array;
}

void freeList(ListOfWord *head)
{
    if (!head)
        return;
    freeList(head->next);
    free(head->word);
    free(head);
}

void listOutput(ListOfWord *head)
{
    if (!head)
        return;
    printf("%s\n", head->word);
    listOutput(head->next);
}

int changeDirectory(char **arguments)
{
    int directoryChangeComplete;
    if((!(arguments[1]))||(strcmp(arguments[1], "~") == 0)||(strcmp(arguments[1], "\0") == 0))
    {
        directoryChangeComplete = chdir(getenv("HOME"));
        if (directoryChangeComplete == 0)
            return 0;
    }
    if (!(arguments[2]))
    {
        directoryChangeComplete = chdir(arguments[1]);
        if (directoryChangeComplete == 0)
            return 0;
    }
    return 1;
}

int executeCommand(char **arguments)
{
    int command;
    command = strcmp(arguments[0], "cd");
    if (command == 0)
    {
        if(changeDirectory(arguments) == 1)
            perror("Error in directory change...");
    }
    else
    {
        if(fork() == 0)
        {
            execvp(arguments[0], arguments);
            exit(COMMAND_EXECUTION);
        }
        wait(NULL);
    }
    return 0;
}

void globalRun(ListOfWord *list, FILE *file)
{
    char **arrayOfWord = NULL;
    systemInfo();
    list = wordList(list, file);
    if(!list)
        return;
    specialSymbolDivision(list);
    specialSymbolMerge(list);
    arrayOfWord = listToArray(list);
    executeCommand(arrayOfWord);
    //listOutput(list);
    free(arrayOfWord);
    freeList(list);
}

int main(int argc, char **argv)
{
    FILE *inputFile = NULL;
    ListOfWord *listingWords = NULL;

    if (argc > 2)
    {
        printf("Invalid number of arguments...\n");
        exit(ARGUMENT_COUNT);
    }

    if (argc == 2)
        inputFile = fopen(argv[1], "r");
    else
        inputFile = stdin;
    
    while(!listingWords)
        globalRun(listingWords, inputFile);
    
    if (argc == 2)
        fclose(inputFile);
    return 0;
}
