#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

struct node {
    uint32_t value;
    struct node* next;
};

struct node* head = NULL;

DIR* validatePath(char* dir)
{
    //check directory is valid
    printf("checking path %s ...", dir);

    DIR* dStream = opendir(dir);
    if(dStream == NULL)
    {
        fprintf(stderr, "opendir error: %d, %s\n", errno, strerror(errno));
    }
    else
    {
        printf("path is valid\n");
    }
    return dStream;
}

DIR* makeSortedDir(char* dir)
{
    char* sortedPath;
    int q = asprintf(&sortedPath, "%s/sorted/", dir);
    if(q == -1)
    {
        fprintf(stderr, "asprintf failed\n");
        exit(EXIT_FAILURE);
    }

    //check if directory exists
    DIR* sortedStream = validatePath(sortedPath);
    while(sortedStream == NULL)
    {
        printf("creating sorted directory\n");
        if (mkdir(sortedPath, 0700) == -1)
        {
            fprintf(stderr, "error: %d, %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        sortedStream = validatePath(sortedPath);

    }
    free(sortedPath);
    return sortedStream; 
}

//test for linked list
void print()
{
    struct node* current = head;
    while(current)
    {
        printf("%i\n", current->value);
        current = current->next;
    }
}

void removeAll()
{
    struct node* curr = head;
    while(head)
    {
        head = curr->next;
        free(curr);
        curr = head;    
    }
}

void insertionSort(uint32_t* buffer)
{
    struct node* newNode = malloc(sizeof(struct node));
    newNode->value = *buffer;

    if(!head || head->value >= newNode->value)
    {
        newNode->next = head;
        head = newNode;
        return;
    }
    struct node* currentNode = head;

    while(currentNode->next && newNode->value > currentNode->next->value)
    {
        currentNode = currentNode->next;
    }
    newNode->next = currentNode->next;
    currentNode->next = newNode;
    return;
}

void writeFile(char* path, char* oldName)
{
    char* newName = oldName;
    newName++; 
    newName++;
    char* newFileName;
    int size = asprintf(&newFileName, "%s/sorted/%s", path, newName);
    if(size == -1)
    {
        fprintf(stderr, "asprintf failed in writeFiles");
        exit(EXIT_FAILURE);
    }

    int fd = open(newFileName, O_CREAT | O_RDWR, 0700);
    if(fd == -1)
    {
        fprintf(stderr, "Error occured when opening files: %i, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    free(newFileName);

    struct node* curr = head;
    while(curr != NULL)
    {
        ssize_t ret = write(fd, &curr->value, sizeof(uint32_t));
        if(ret == -1)
        {
            fprintf(stderr, "Error occured while writing file: %i, %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
        printf("wrote %i\n", curr->value);
        curr = curr->next;
    }
    printf("done writing numbers\n");
    printf("printing linked list...\n");
    print();

    //remove all files from linked list after write
    removeAll();
}

void fileCreate(DIR* unsorted, DIR* sorted, char* dir)
{
    struct dirent* files;
    char* path = NULL;
    unsigned int fileCount =0;

    //loops through each file in directory
    while((files = readdir(unsorted)) != NULL)
    {
        //checks if file, check file name
        if((files->d_type == 0x8) && (strncmp(files->d_name, "unsorted_", 9) == 0) 
                && (strncmp((files->d_name + (strlen(files->d_name) - 4)), ".bin", 4) == 0))
        {
            fileCount++;
            int s = asprintf(&path, "%s/%s", dir, files->d_name);
            if(s == -1)
            {
                fprintf(stderr, "asprintf failed\n");
                exit(EXIT_FAILURE);
            }
            printf("Reading %s\n", files->d_name);
            int fd = open(path, O_RDONLY);
            if(fd == -1)
            {
                fprintf(stderr, "Error occured when opening files: %i, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            }

            uint32_t buffer;

            while((read(fd, &buffer, sizeof(uint32_t)) == sizeof(uint32_t)))
            {
                insertionSort(&buffer);
            }

            //write to new file
            writeFile(dir, files->d_name);
            int c = close(fd);
            if(c == -1)
            {
                fprintf(stderr, "Error occured while closing files: %i, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            }   
        }
    }
    if(fileCount == 0)
    {
        fprintf(stderr, "No files to read in this folder\n Hint: must be named unsorted_<i>.bin\n");
        exit(EXIT_FAILURE);
    }
}

int main (int argc, char* argv[])
{
    if(argc != 2)
    {
        fprintf(stderr, "USAGE: %s <directory path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* directory = argv[1];
    DIR* ustream = validatePath(directory);
    if(ustream == NULL)
    {
        exit(EXIT_FAILURE);
    }
    DIR* sstream = makeSortedDir(directory);

    fileCreate(ustream, sstream, directory);

    if(closedir(ustream) == -1)
    {
        fprintf(stderr, "close error: %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(closedir(sstream) == -1)
    {
        fprintf(stderr, "close error: %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Completed Successfully!\n");

    exit(EXIT_SUCCESS);
}
