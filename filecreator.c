#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

const int MAXLEN = 20;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
unsigned int seed = 0;

struct files {
    char** paths; //file paths
    int* fd; //file descriptors
    unsigned int numFiles; //num files needed
    unsigned int numInts; //number of ints per file
    int numThreads; //num threads
    int threadKey; //key for creating the files function
};

void checkDir(char* directory)
{
    int d = open(directory, O_DIRECTORY | O_RDONLY);
    if(d < 0)
    {
        printf("Not a valid directory\n");
        exit(-1);
    }
    else
    {
        close(d);
    }
}

void createFileNames(char* paths[], char* location, unsigned int* numOfFiles)
{
    for(int i = 0; i < *numOfFiles; i++)
    {
        char fileName[MAXLEN];
        sprintf(fileName, "unsorted_%d.bin", i);
        paths[i] = malloc(strlen(location) + MAXLEN + 1); //malloc each element
        if(paths[i] == NULL)
        {
            perror("malloc() failed in createFileNames");
            exit(EXIT_FAILURE);
        }
        strcpy(paths[i], location);
        strcat(paths[i], fileName);
    }
}

void writeFiles(struct files* fi, unsigned int* s, int* fileDesc)
{
    
    uint32_t randNum = -1;
    for(int i = 0; i < fi->numInts; i++)
    {
        randNum = rand_r(s);
        int success = write(*fileDesc, &randNum, sizeof(uint32_t));
        if(success < 0)
        {
            printf("writeFiles Error: %d, %s\n", errno, strerror(errno));
            exit(-1);
        }
    }
}

//single thread creates a single file
void* createFiles(void* param)
{
    struct files* f = (struct files*) param;
    unsigned int* seedPtr[f->numFiles];

    pthread_mutex_lock(&lock);
    printf("Thread %i working\n", f->threadKey +1);
    for(int i = 0; i < f->numFiles; i++)
    {
        seedPtr[i] = &seed;
        if(i % f->numThreads == f->threadKey)
        {
            f->fd[i] = open(f->paths[i], O_CREAT | O_EXCL | O_RDWR, 0700);
            if (f->fd[i] < 0)
            {
                printf("createFiles Error: %d, %s\n", errno, strerror(errno));
                exit(-1);
            }
            writeFiles(f, seedPtr[i], &f->fd[i]);
        }
    }
    f->threadKey+=1;
    pthread_mutex_unlock(&lock);

    return NULL;
}



int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "USAGE: %s <location of directory>, <num files>, <num integers>, <num threads>\n", argv[0]);
        exit(-1);
    }

    if(atoi(argv[2]) < atoi(argv[4]))
    {
        printf("numThreads must be <= numFiles");
        exit(-1);
    }

    if((atoi(argv[2]) < 1) || (atoi(argv[3]) < 1) || (atoi(argv[4]) < 1))
    {
        printf("must be < 0");
        exit(-1);
    }

    struct files fileInfo;
    char* directoryLoc = argv[1];
    fileInfo.numFiles = atoi(argv[2]);
    fileInfo.numInts = atoi(argv[3]);
    fileInfo.numThreads = atoi(argv[4]);

    if(strcmp(&directoryLoc[strlen(directoryLoc)-1], "/") != 0)
    {
        strcat(directoryLoc, "/");

    }

    //allocate for array of char*
    fileInfo.paths = malloc(sizeof(char*) * fileInfo.numFiles);
    if(fileInfo.paths == NULL)
    {
        perror("malloc() failed");
        exit(EXIT_FAILURE);
    }

    //allocate int array
    fileInfo.fd = malloc(sizeof(int) * fileInfo.numFiles);
    if(fileInfo.fd == NULL)
    {
        perror("malloc() failed");
        exit(EXIT_FAILURE);
    }

    fileInfo.threadKey = 0;
    int rc;
    pthread_t threads[fileInfo.numThreads];


    checkDir(directoryLoc);

    printf("%i thread(s) will create %i file(s) with %i binary integer(s)\n", fileInfo.numThreads, fileInfo.numFiles, fileInfo.numInts);

    createFileNames(fileInfo.paths, directoryLoc, &fileInfo.numFiles);

    //create threads
    for (int i = 0; i < fileInfo.numThreads; i++)
    {
        rc = pthread_create(&threads[i], NULL, createFiles, (void*) &fileInfo);
        if(rc)
        {
            fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //wait for threads to complete
    for(int i = 0; i < fileInfo.numThreads; i++)
    {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR joining threads, %d\n", rc);
            exit(-1);
        }
    }

    for(int i = 0; i < fileInfo.numFiles; i++)
    {
        free(fileInfo.paths[i]);
        close(fileInfo.fd[i]);
    }
    free(fileInfo.paths);
    printf("Completed Successfully\n");

    return(0);
}
