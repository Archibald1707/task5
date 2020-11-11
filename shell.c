#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

enum Error
{
    MEMORY_ALLOCATION = 1,
    ARGUMENT_COUNT,
    WORD_SIZE,
    PATH_ERROR,
    LOGIN_ERROR,
    HOSTNAME_ERROR,
    MEMORY_CHUNK = 10
};

typedef struct list
{
    char *word;
    struct list *next;
}ListOfWord;

void systemInfo(void)
{
    char path[PATH_MAX], user[LOGIN_NAME_MAX], hostname[HOST_NAME_MAX];
    if (!(getcwd(path , sizeof(path))))
        exit(PATH_ERROR);
    if ((getlogin_r(user, sizeof(user))))
        exit(LOGIN_ERROR);
    if ((gethostname(hostname, sizeof(hostname))))
        exit(HOSTNAME_ERROR);
    printf("[%s@%s %s]$ ", user, hostname, path);
}

char* getWord(FILE *filePointer)
{
    char *wordToList;
    char temporarySymbol;
    int quotesFlag = INT_MAX - 1, argument = 0, length = MEMORY_CHUNK;
    wordToList = malloc(length * sizeof(char));
    if (wordToList == NULL)
        exit(MEMORY_ALLOCATION);
    while ((temporarySymbol = getc(filePointer)) != EOF)
    {
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
        }
        if ((temporarySymbol == '<')||(temporarySymbol == ')')||(temporarySymbol == '(')||(temporarySymbol == '>')||(temporarySymbol == '|')||(temporarySymbol == '&'))
        {
            wordToList = realloc(wordToList, argument + MEMORY_CHUNK);
            if(argument != 0)
            {
                wordToList[argument] = '\n';
                ++argument;
            }
            wordToList[argument] = temporarySymbol;
            ++argument;
            quotesFlag = 0;
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
            if (wordToList == NULL)
                exit(WORD_SIZE);
        }
        wordToList[argument] = temporarySymbol;
        ++argument;
    }
    wordToList[argument] = '\0';
    return wordToList;
}

ListOfWord* addElement(ListOfWord *lastElement, char *inputWord)
{
    if (lastElement == NULL)
    {
        lastElement = malloc(sizeof(ListOfWord));
        if (lastElement == NULL)
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
    char *wordGet;
    wordGet = getWord(fileDescriptor);
    while (wordGet[0] != '\0')
    {
        head = addElement(head, wordGet);
        wordGet = getWord(fileDescriptor);
    }
    free(wordGet);
    return head;
}

void specialSymbolDivision(ListOfWord *headInput)
{
    char *newWord;
    int i = 0;
    ListOfWord *temporary, *memory, *head;
    head = headInput;
    while(head != NULL)
    {
        while((head->word)[i] != '\0')
        {
            if(((head->word)[i] == '<')||((head->word)[i] == '>')||((head->word)[i] == '(')||((head->word)[i] == ')')||((head->word)[i] == '&')||((head->word)[i] == '|'))
            {
                newWord = malloc(MEMORY_CHUNK*sizeof(char));
                newWord[0] = (head->word)[i];
                newWord[1] = '\0';
                (head->word)[i-1] = '\0';
                temporary = malloc(sizeof(ListOfWord));
                memory = head->next;
                head->next = temporary;
                temporary->word = newWord;
                temporary->next = memory;
            }
            ++i;
        }
        head = head->next;
    }
}

void specialSymbolMerge(ListOfWord *headInput)
{
    ListOfWord *head;
    head = headInput;
    ListOfWord *temporary;
    while((head->next) != NULL)
    {
        if((((head->word)[0] == '<')||((head->word)[0] == '>')||((head->word)[0] == '(')||((head->word)[0] == ')')||((head->word)[0] == '&')||((head->word)[0] == '|'))&&((head->word)[1] == '\0'))
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

void freeList(ListOfWord *head)
{
    if (head == NULL)
        return;
    freeList(head->next);
    free(head->word);
    free(head);
}

void listOutput(ListOfWord *head)
{
    if (head == NULL)
        return;
    printf("%s\n", head->word);
    listOutput(head->next);
}

int main(int argc, char **argv)
{
    FILE *inputFile;
    ListOfWord *listingWords = NULL;

    if (argc > 2)
        exit(ARGUMENT_COUNT);

    if (argc == 2)
        inputFile = fopen(argv[1], "r");
    else
        inputFile = stdin;
    
    systemInfo();
    
    listingWords = wordList(listingWords, inputFile);
    if(inputFile == stdin)
        printf("\n");
    specialSymbolDivision(listingWords);
    specialSymbolMerge(listingWords);
    listOutput(listingWords);
    freeList(listingWords);
    if (argc == 2)
        fclose(inputFile);
    return 0;
}
