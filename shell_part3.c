#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

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
    SHELL_EXIT,
    MEMORY_CHUNK = 20
};

typedef enum MetaCharacter
{
    SPECIAL_WRITE,
    SPECIAL_READ,
    SPECIAL_APPEND,
    SPECIAL_PIPE,
    SPECIAL_OR_EXECUTION,
    SPECIAL_AND_EXECUTION,
    SPECIAL_OPEN_GROUP,
    SPECIAL_CLOSE_GROUP,
    SPECIAL_BACKGROUND,
    SPECIAL_NULL,
    SPECIAL_NOTHING
}MetaCharacter;

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
    if (!directory)
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
            if (!directory)
                exit(MEMORY_ALLOCATION);
        }
        if (path[i] == '/')
        {
            j = 0;
            position = MEMORY_CHUNK;
            free(directory);
            directory = malloc(position * sizeof(char));
            if (!directory)
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

char* getWord(int filePointer)
{
    char *wordToList;
    char temporarySymbol = 0;
    int quotesFlag = INT_MAX - 1, argument = 0, length = MEMORY_CHUNK, stopFlag = 0;
    int isSpecialSymbol, isBracket, isComparison, isTerm;
    wordToList = malloc(length * sizeof(char));
    if (!wordToList)
        exit(MEMORY_ALLOCATION);
    while ((temporarySymbol = getchar()) != EOF)
    {
        isBracket       = (temporarySymbol == ')') || (temporarySymbol == '(');
        isComparison    = (temporarySymbol == '<') || (temporarySymbol == '>');
        isTerm          = (temporarySymbol == '|') || (temporarySymbol == '&');
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
        if ((isSpecialSymbol) && (quotesFlag % 2 == 0))
        {
            wordToList = realloc(wordToList, argument + MEMORY_CHUNK);
            if (!wordToList)
                exit(MEMORY_ALLOCATION);
            if (stopFlag == 0)
            {
                if (argument != 0)
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

ListOfWord* wordList(ListOfWord *head, int fileDescriptor)
{
    int i = 0;
    char *wordGet = getWord(fileDescriptor);
    if (!(wordGet))
        exit(NULL_READ);
    if (wordGet[0] == '\n')
    {
        free(wordGet);
        return NULL;
    }
    while ((wordGet[i] != '\0') || (wordGet[i] != '\n'))
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

void specialSymbolDivision(ListOfWord *head)
{
    char *newWord = NULL;
    int isSpecialSymbol, isBracket, isComparison, isTerm, isLastSpace, i = 0;
    ListOfWord *temporary = NULL, *memory = NULL, *headInput = NULL;
    headInput = head;
    while (head != NULL)
    {
        i = 0;
        while ((head->word)[i] != '\0')
        {
            isBracket       = ((head->word)[i] == ')') || ((head->word)[i] == '(');
            isComparison    = ((head->word)[i] == '<') || ((head->word)[i] == '>');
            isTerm          = ((head->word)[i] == '|') || ((head->word)[i] == '&');
            isSpecialSymbol = isBracket || isComparison || isTerm;
            
            if (i > 0)
                isLastSpace = (head->word[i-1] == ' ');
            else
                isLastSpace = 0;
            if ((isSpecialSymbol) && (isLastSpace))
            {
                newWord = malloc(MEMORY_CHUNK * sizeof(char));
                if (!newWord)
                    exit(MEMORY_ALLOCATION);
                newWord[0] = (head->word)[i];
                newWord[1] = '\0';
                (head->word)[i-1] = '\0';
                temporary = malloc(sizeof(ListOfWord));
                if (!temporary)
                {
                    perror("");
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
    head = headInput;
}

void specialSymbolMerge(ListOfWord *headInput)
{
    ListOfWord *head = headInput;
    int isBracket       = ((head->word)[0] == ')') || ((head->word)[0] == '(');
    int isComparison    = ((head->word)[0] == '<') || ((head->word)[0] == '>');
    int isTerm          = ((head->word)[0] == '|') || ((head->word)[0] == '&');
    int isSpecialSymbol = isBracket || isComparison || isTerm;
    ListOfWord *temporary = NULL;
    while ((head->next) != NULL)
    {
        isBracket       = ((head->word)[0] == ')') || ((head->word)[0] == '(');
        isComparison    = ((head->word)[0] == '<') || ((head->word)[0] == '>');
        isTerm          = ((head->word)[0] == '|') || ((head->word)[0] == '&');
        isSpecialSymbol = isBracket || isComparison || isTerm;
        if ((isSpecialSymbol)&&((head->word)[1] == '\0'))
            if ((head->word)[0] == ((head->next)->word)[0])
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

MetaCharacter defineSpecialSymbol(char *specialSymbol)
{
    if (!specialSymbol)
        return SPECIAL_NULL;
    
    else if (!strcmp(specialSymbol, ">"))
        return SPECIAL_WRITE;
    else if (!strcmp(specialSymbol, "<"))
        return SPECIAL_READ;
    else if (!strcmp(specialSymbol, ">>"))
        return SPECIAL_APPEND;
    
    else if (!strcmp(specialSymbol, "|"))
        return SPECIAL_PIPE;
    else if (!strcmp(specialSymbol, "||"))
        return SPECIAL_OR_EXECUTION;
    else if (!strcmp(specialSymbol, "&&"))
        return SPECIAL_AND_EXECUTION;
    else if (!strcmp(specialSymbol, "("))
        return SPECIAL_OPEN_GROUP;
    else if (!strcmp(specialSymbol, ")"))
        return SPECIAL_CLOSE_GROUP;
    else if (!strcmp(specialSymbol, "&"))
        return SPECIAL_BACKGROUND;
    else
        return SPECIAL_NOTHING;
}

char **listToArray(ListOfWord *headOfList)
{
    int position = 0, element = MEMORY_CHUNK;
    ListOfWord *head = NULL;
    char **array = malloc(sizeof(char*) * MEMORY_CHUNK);
    head = headOfList;
    if (!array)
    {
        perror("");
        exit(MEMORY_ALLOCATION);
    }
    while (head != NULL)
    {
        if (!((position + 1) % MEMORY_CHUNK))
        {
            element += MEMORY_CHUNK;
            array = realloc(array, sizeof(char*) * element);
            if (!array)
            {
                perror("");
                exit(MEMORY_ALLOCATION);
            }
        }
        array[position++] = head->word;
        head = head->next;
    }
    array[position] = NULL;
    return array;
}

int checkPipe(char **globalArray)
{
    MetaCharacter meta;
    int i = 0, pipeFlag = 0, pigeonFlag = 0;
    while (globalArray[i] != NULL)
    {
        meta = defineSpecialSymbol(globalArray[i]);
        if (meta == SPECIAL_PIPE)
            pipeFlag = 1;
        else if (meta == SPECIAL_WRITE)
            pigeonFlag = 1;
        else if (meta == SPECIAL_WRITE)
            pigeonFlag = 1;
        else if (meta == SPECIAL_WRITE)
            pigeonFlag = 1;
        ++i;
    }
    if (pipeFlag && pigeonFlag)
    {
        perror("");
        return 4;
    }
    else
    {
        if (pipeFlag)
            return 1;
        if (pigeonFlag)
            return 2;
        if (!(pipeFlag + pigeonFlag))
            return 3;
    }
    return 0;
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

char** getCommand(char **globalArray, int commandNumber)
{
    int pipeCounter = 0, wordCounter = 0, commandArgument = 0;
    int position = 0, element = MEMORY_CHUNK;
    int specialCounter, isPipe = 0, isWrite = 0, isRead = 0, isAppend = 0;
    char **command = malloc(sizeof(char*) * MEMORY_CHUNK);
    while ((pipeCounter + 1) != commandNumber)
    {
        isPipe         = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_PIPE;
        isWrite        = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_WRITE;
        isRead         = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_READ;
        isAppend       = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_APPEND;
        specialCounter = isWrite || isRead || isAppend || isPipe;
        
        if (specialCounter)
            ++pipeCounter;
        ++wordCounter;
        if (defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_NULL)
            return NULL;
    }
    isPipe           = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_PIPE;
    isWrite          = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_WRITE;
    isRead           = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_READ;
    isAppend         = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_APPEND;
    specialCounter   = isWrite || isRead || isAppend || isPipe;
    int isCommandEnd = (specialCounter || (defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_NULL));
    while (!isCommandEnd)
    {
        if (!((position + 1) % MEMORY_CHUNK))
        {
            element += MEMORY_CHUNK;
            command = realloc(command, sizeof(char*) * element);
            if (!command)
            {
                perror("");
                exit(MEMORY_ALLOCATION);
            }
        }
        command[commandArgument] = globalArray[wordCounter];
        ++wordCounter;
        ++commandArgument;
        isPipe         = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_PIPE;
        isWrite        = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_WRITE;
        isRead         = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_READ;
        isAppend       = defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_APPEND;
        specialCounter = isWrite || isRead || isAppend || isPipe;
        isCommandEnd   = (specialCounter || (defineSpecialSymbol(globalArray[wordCounter]) == SPECIAL_NULL));
    }
    command[commandArgument] = NULL;
    return command;
}

int changeDirectory(char **arguments)
{
    int directoryChangeComplete;
    int isNull = (arguments[1]) == NULL;
    int isTilda, isZero;
    if (isNull)
    {
        isTilda = 0;
        isZero = 0;
    }
    else
    {
        isTilda = strcmp(arguments[1], "~") == 0;
        isZero = strcmp(arguments[1], "\0") == 0;
    }
    int isHome = isNull || isTilda || isZero;
    if (isHome)
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

int makePipe(char **globalArray)
{
    int commandNumber = 0, pid = -1, fileDescriptor[2];
    char **command, **isNull;
    while ((command = getCommand(globalArray, ++commandNumber)) != NULL)
    {
        pipe(fileDescriptor);
        switch ((pid = fork()))
        {
            case -1:
                return 1;
            case 0:
                if ((isNull = getCommand(globalArray, (commandNumber + 1))) != NULL)
                {
                    dup2(fileDescriptor[1], 1);
                    free(isNull);
                }
                close(fileDescriptor[0]);
                close(fileDescriptor[1]);
                execvp(command[0], command);
                free(command);
                exit(1);
        }
        dup2(fileDescriptor[0], 0);
        close(fileDescriptor[0]);
        close(fileDescriptor[1]);
    }
    while (wait(NULL) != -1);
    return 0;
}

void redirectIO(char **globalArray)
{
    int i = 0, fileDescriptor = -1;
    MetaCharacter meta;
    while (globalArray[i] != NULL)
    {
        meta = defineSpecialSymbol(globalArray[i]);
        
        if (meta == SPECIAL_WRITE)
        {
            fileDescriptor = creat(globalArray[i + 1], 0666);
            if (fileDescriptor < 0)
                perror("");
            else
                dup2(fileDescriptor, 1);
            i += 2;
            continue;
        }
        
        else if (meta == SPECIAL_READ)
        {
            fileDescriptor = open(globalArray[i + 1], O_RDONLY, 0666);
            if ( fileDescriptor < 0 )
                perror("");
            else
                dup2(fileDescriptor, 0);
            i += 2;
            continue;
        }
        
        else if (meta == SPECIAL_APPEND)
        {
            fileDescriptor = open(globalArray[i + 1], O_CREAT | O_APPEND | O_WRONLY, 0666);
            if ( fileDescriptor < 0 )
                perror("");
            else
                dup2(fileDescriptor, 1);
            i += 2;
            continue;
        }
        ++i;
    }
}

int executeCommand(char **arguments, int isPipeRedirect)
{
    int commandDirectory, commandExit, pid = -1;
    commandDirectory = strcmp(arguments[0], "cd");
    commandExit      = strcmp(arguments[0], "exit");
    
    if (commandExit == 0)
        return 1;
    else if (commandDirectory == 0)
    {
        if (changeDirectory(arguments) == 1)
            perror("Error in directory change...");
    }
    else
    {
        if ((pid = fork()) == 0)
        {
            if (isPipeRedirect == 3)
            {
                execvp(arguments[0], arguments);
                exit(COMMAND_EXECUTION);
            }
            else if (isPipeRedirect == 1)
            {
                makePipe(arguments);
                exit(COMMAND_EXECUTION);
            }
            else if (isPipeRedirect == 2)
            {
                char **command;
                redirectIO(arguments);
                command = getCommand(arguments, 1);
                execvp(command[0], command);
                free(command);
                exit(COMMAND_EXECUTION);
            }
        }
        else if (pid == -1)
            return 1;
        wait(NULL);
    }
    return 0;
}

int globalRun(ListOfWord *list, int file)
{
    int isPipeRedirect = 0;
    char **arrayOfWord = NULL;
    int exitFlag = 0;
    systemInfo();
    list = wordList(list, file);
    if (!list)
        return 0;
    specialSymbolDivision(list);
    specialSymbolMerge(list);
    arrayOfWord = listToArray(list);
    isPipeRedirect = checkPipe(arrayOfWord);
    exitFlag = executeCommand(arrayOfWord, isPipeRedirect);
    //listOutput(list);
    free(arrayOfWord);
    freeList(list); 
    return exitFlag;
}

int main(int argc, char **argv)
{
    int inputFile = 0;
    ListOfWord *listingWords = NULL;
    int exitCommand = 0;

    if (argc > 2)
    {
        printf("Invalid number of arguments...\n");
        exit(ARGUMENT_COUNT);
    }
    else if (argc == 2)
    {
        inputFile = open(argv[1], O_RDONLY);
        dup2(inputFile, 0);
    }
    else
        inputFile = 0;
    
    while (exitCommand == 0)
    {
        if ((exitCommand = globalRun(listingWords, inputFile)) == 1)
            break;
    }
    
    if (argc == 2)
        close(inputFile);
    
    return 0;
}
