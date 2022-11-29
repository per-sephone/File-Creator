#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <ctype.h>
#include <stdint.h>

int numFiles;
int numThreads;

struct file {
    int fd;
    int id; //file number ie xxsorted_12.bin -> id = 12
    int size; //size of files
    char* name;

};
struct node {
    struct file data;
    struct node* next;
};

struct node* shead = NULL;
int ssize = 0;
struct node* uhead = NULL;
int usize = 0;

int findFileSize(struct file* f)
{
    off_t ret = lseek(f->fd, 0, SEEK_END);
    if (ret == -1) {
        fprintf(stderr, "ERROR %d: lseek unable to access file desc %d\n", errno, f->fd);
        exit(errno);
    }
    lseek(f->fd, 0, SEEK_SET);
    return ret;
}

//swap taken from https://www.programiz.com/dsa/selection-sort
void swap(uint32_t* x, uint32_t* y)
{
    uint32_t temp = *x;
    *x = *y;
    *y = temp;
}

//selection sort taken from https://www.programiz.com/dsa/selection-sort
void selectionSort(uint32_t* arr, int* size)
{
    int min;

    for(int i = 0; i < (*size)-1; i++)
    {
        min = i;
        for(int j = i+1; j < *size; j++)
        {
            if(arr[j] < arr[min]) min = j;
        }
        swap(&arr[min], &arr[i]);
    }
}

void checkFile(struct file* unsort, struct file* sort)
{
    //make unsorted map
    unsort->size = findFileSize(unsort);
    uint32_t* umap = mmap(NULL, unsort->size, PROT_READ, MAP_PRIVATE, unsort->fd, 0);

    if(umap == MAP_FAILED)
    {
        fprintf(stderr, "Mapping Failed %d %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    //close unsorted fd
    int c = close(unsort->fd);
    if(c == -1)
    {
        fprintf(stderr, "Error occured while closing files: %i, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }


    //make sorted map
    sort->size = findFileSize(sort);
    uint32_t* smap = mmap(NULL, sort->size, PROT_READ, MAP_PRIVATE, sort->fd, 0);

    if(smap == MAP_FAILED)
    {
        fprintf(stderr, "Mapping Failed %d %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    //close sorted fd
    int d = close(sort->fd);
    if(d == -1)
    {
        fprintf(stderr, "Error occured while closing files: %i, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //make an array using mapped unsorted contents
    int numInts = unsort->size/4; //4 bytes per int
    uint32_t sortedUnsorted[numInts];

    for(int i = 0; i < numInts; i++)
    {
        sortedUnsorted[i] = umap[i];
    }

    selectionSort(&sortedUnsorted[0], &numInts);

    //compare the sorted "unsorted" array to the sorted map
    for(int j = 0; j < numInts; j++)
    {
        if(sortedUnsorted[j] != smap[j])
        {
            fprintf(stderr, "(un)sorted<%i>.bin DO NOT match!\n", unsort->id);
            exit(EXIT_FAILURE);
        }
    }

    printf("(un)sorted<%i>.bin match!\n", unsort->id);

    //munmap both maps
    int g = munmap(umap, numInts);
    if(g == -1)
    {
        fprintf(stderr, "munmap failure\n");
        exit(EXIT_FAILURE);
    }
    int h = munmap(smap, numInts);
    if(h == -1)
    {
        fprintf(stderr, "munmap failure\n");
        exit(EXIT_FAILURE);
    }
}

void makeIntoArray(struct file* files, struct node* curr)
{
    int i = 0;
    while(curr)
    {
        files[i] = curr->data;
        curr = curr->next;
        i++;
    }
}

void* checkFiles(void* param)
{
    int* key = (int*) param;

    //array of files
    struct file ufiles[numFiles];
    struct file sfiles[numFiles];

    makeIntoArray(&ufiles[0], uhead);
    makeIntoArray(&sfiles[0], shead);

    for(int i = *key; i < numFiles; i += numThreads) 
    {
        printf("Thread %i working\n", *key);
        checkFile(&ufiles[i], &sfiles[i]);
    }
    return NULL;
}

int getId(char* fname) //extracts the number from file name
{
    const int maxDigit = 20;
    char* s = fname;
    char* temp = malloc(sizeof(char) * maxDigit);

    if((strncmp(fname, "unsorted_", 9) == 0) || (strncmp(fname, "sorted_", 7) == 0)) 
    {
        while(*s)
        {
            if(isdigit(*s))
            {
                strncat(temp, s, 1);
            }
            s++;
        }
    }
    else
    {
        fprintf(stderr, "Only unsorted_xx.bin or sorted_xx.bin files\n");
        exit(EXIT_FAILURE);
    }

    //turn string into int
    int num = atoi(temp);

    return num;
}

/*
//test for linked list
void print(struct node** head, int* size)
{
    struct node* current = *head;
    while(current)
    {
        current = current->next;
    }
}
*/

void insertionSort(struct file* t, struct node** head, int* size)
{
    struct node* newNode = malloc(sizeof(struct node));
    newNode->data = *t;

    if(!(*head) || (*head)->data.id >= newNode->data.id)
    {
        newNode->next = *head;
        *head = newNode;
        (*size)++;
        return;
    }
    struct node* current = *head;
    while(current->next && newNode->data.id > current->next->data.id)
    {
        current = current->next;
    }
    newNode->next = current->next;
    current->next = newNode;
    (*size)++;
    return;

}

int readDir(DIR** dir, char* str, int len, struct node** head, int* listSize, char** path)
{
    int fileCount = 0;
    struct dirent* f;
    while((f = readdir(*dir)) != NULL)
    {
        if((f->d_type == 0x8) && (strncmp(f->d_name, str, len) == 0)
                && (strncmp((f->d_name + (strlen(f->d_name) - 4)), ".bin", 4) == 0))
        {
            struct file temp;
            temp.id = getId(f->d_name);

            //tack file name onto the end of path
            //char* fileName;
            int b = asprintf(&temp.name, "%s%s", *path, f->d_name);
            if(b == -1)
            {
                fprintf(stderr, "asprintf failed\n");
                exit(EXIT_FAILURE);
            }


            //temp.fd = open(fileName, O_RDONLY, S_IRUSR);
            temp.fd = open(temp.name, O_RDONLY, S_IRUSR);

            if(temp.fd == -1)
            {       
                fprintf(stderr, "Error occured when opening files: %i, %s\n", errno, strerror(errno));
                exit(EXIT_FAILURE);
            }

            fileCount++;

            //insert data into a node
            insertionSort(&temp, &(*head), &(*listSize));
        }

    }
    if(fileCount == 0)
    {
        fprintf(stderr, "No files to read in this folder\n Hint: must be named (un)sorted_<i>.bin\n");
        exit(EXIT_FAILURE);
    }

    return fileCount;
}

DIR* validatePath(char* directory)
{
    DIR* dir = opendir(directory);
    if(dir == NULL)
    {
        fprintf(stderr, "Not a valid directory, %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return dir;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "USAGE: %s <directory path> <number of threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if(atoi(argv[2]) < 0)
    {
        fprintf(stderr, "number of threads must be < 0\n");
        exit(EXIT_FAILURE);
    }

    char* udirectory = argv[1];
    numThreads = atoi(argv[2]);

    if(numThreads == 0)
    {
        fprintf(stderr, "Number of threads must be > 0\n");
        exit(EXIT_FAILURE);
    }

    numFiles = 0;

    if(strcmp(&udirectory[strlen(udirectory)-1], "/") != 0)
    {
        strcat(udirectory, "/");
    }

    char* sdirectory;

    int b = asprintf(&sdirectory, "%ssorted/", udirectory);
    if(b == -1)
    {
        fprintf(stderr, "asprintf failed\n");
        exit(EXIT_FAILURE);
    }

    //check if directory is valid
    DIR* unsorted = validatePath(udirectory);
    DIR* sorted = validatePath(sdirectory);

    printf("Directories validated\n");

    int uCount = readDir(&unsorted, "unsorted_", 9, &uhead, &usize, &udirectory);
    int sCount = readDir(&sorted, "sorted_", 7, &shead, &ssize, &sdirectory);

    printf("Files opened\n");

    if(uCount != sCount)
    {
        fprintf(stderr, "Different number of files in each directory\n");
        exit(EXIT_FAILURE);
    }

    numFiles = uCount;

    int threadKey[numThreads];
    int rc;
    pthread_t threads[numThreads];

    printf("Starting threads\n");

    //create threads & threadkey
    for(int i = 0; i < numThreads; i++)
    {
        threadKey[i] = i;
        rc = pthread_create(&threads[i], NULL, checkFiles, &threadKey[i]);
        if(rc)
        {
            fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    //wait for threads to complete
    for(int i = 0; i < numThreads; i++)
    {
        rc = pthread_join(threads[i], NULL);
        if(rc != 0)
        {
            fprintf(stderr, "ERROR joining threads, %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    //close unsorted directory stream
    if(closedir(unsorted) == -1)
    {
        fprintf(stderr, "close error: %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    //close sorted directory stream
    if (closedir(sorted) == -1)
    {
        fprintf(stderr, "close error: %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Completed Successfully!\n All lists match\n");
    exit(EXIT_SUCCESS);
}
