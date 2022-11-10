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


unsigned int seed = 0;
unsigned int numFiles; //num files needed
unsigned int numInts; //number of ints per file
int numThreads; //num threads
char* location; //directory

void checkDir(char* directory)
{
    int d = open(directory, O_DIRECTORY | O_RDONLY);
    if(d < 0)
    {
        perror("Not a valid directory\n");
        exit(-1);
    }
    else
    {
        close(d);
    }
}

void writeFile(int fd)
{
    uint32_t randNum = -1;
    unsigned int* seedPtr[numInts];

    //printf("write executed\n");

    for(int i = 0; i < numInts; i++)
    {
        seedPtr[i] = &seed;
        randNum = rand_r(seedPtr[i]);
        int success = write(fd, &randNum, sizeof(uint32_t));
        if(success < 0)
        {
            printf("writeFiles Error: %d, %s\n", errno, strerror(errno));
            exit(-1);
        }
    }
}

int openFile(int fileNum)
{
    char* fileName;
    int s = asprintf(&fileName, "%sunsorted_%d.bin", location, fileNum);
    if(s == -1)
    {
        perror("asprintf failed in createFileNames");
        exit(-1);
    }
    //printf("fileName: %s, open executed\n", fileName); 
        
    int fd = open(fileName, O_CREAT | O_EXCL | O_RDWR, 0700);
    if (fd < 0)
    {
        printf("createFiles Error: %d, %s\n", errno, strerror(errno));
        exit(-1);
    
    }
    free(fileName);
    return fd;
}


//single thread creates a single file
void* createFiles(void* param)
{
    int* key = (int*) param;

    for(int i = *key; i < numFiles; i+=numThreads) //each thread makes f/t files
    {
        //printf("thread %i executing\n", *key);
        int fileDescriptor = openFile(i);
        writeFile(fileDescriptor);
        close(fileDescriptor);
    }
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

    location = argv[1];
    numFiles = atoi(argv[2]);
    numInts = atoi(argv[3]);
    numThreads = atoi(argv[4]);


    //check directory
    checkDir(location);

    //add / if there is none
    if(strcmp(&location[strlen(location)-1], "/") != 0)
    {
        strcat(location, "/");
    }

    int threadKey[numThreads];

    //malloc for char array in each path, add thread key, create file names, create seed ptr
    for(int i = 0; i < numThreads; i++)
    {
        threadKey[i] = i;
    }

    int rc;
    pthread_t threads[numThreads];

    printf("%i thread(s) will create %i file(s) with %i binary integer(s)\n", numThreads, numFiles, numInts);

    //create threads
    for (int i = 0; i < numThreads; i++)
    {
        rc = pthread_create(&threads[i], NULL, createFiles, &threadKey[i]);
        if(rc)
        {
            fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //wait for threads to complete
    for(int i = 0; i < numThreads; i++)
    {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR joining threads, %d\n", rc);
            exit(-1);
        }
    }

    printf("Completed Successfully\n");

    return(0);
}
