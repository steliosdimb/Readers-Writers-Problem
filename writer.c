#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "account.h"
#include "writer.h"
#include "shm.h"
#include "wait.h"
#include <time.h>
int main(int argc, char *argv[])
{
    srand(time(NULL));
    char *filename;
    int time;
    int shm_fd;
    char *shmid;
    int value;
    int recid;
    int fd = parsing(argc, argv, &filename, &time, &shmid, &recid, &value);
    lseek(fd, 0, SEEK_SET);

    struct shm *shm_mem;
    shm_fd = shm_open(shmid, O_RDWR, S_IRWXU | S_IRWXG); // open shared memory fd

    if (shm_fd < 0)
    {
        perror("Error in shm_open ..exiting\n");
        exit(-1);
    }
    int shm_size = sizeof(struct shm);
    shm_mem = (struct shm *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_mem == NULL)
    {
        if (shm_unlink(shmid) < 0)
        {
            perror("Error in shm_unlink..exiting\n");
            exit(-1);
        }
        perror("Error in mmap..exiting\n");
        exit(-1);
    }
    shm_sem = sem_open("shared_memory", 0); // create semaphore for accessing the shared memory
    writers_sem = sem_open("writers", 0);   // created semaphore for writers
    readers_sem = sem_open("readers", 0);   // created semaphore for readers
    file_sem = sem_open("file", 0);         // create semaphore for file
    if (shm_sem < 0 || readers_sem < 0 || file_sem < 0 || writers_sem < 0)
    {
        perror("Error in sem_open..exiting\n");
        exit(-1);
    }
    int val;
    pid_t pid = fork();

    if (pid == 0)
    {
        size_t accountsize = sizeof(struct account);                 // size of one account in file
        struct account *acc = (struct account *)malloc(accountsize); // dynamically allocate memory for one account
        int counter = 0;
        int flag = 0;
        sem_wait(shm_sem); // writer will access shared memory

        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->acc_being_written[i] == recid || shm_mem->acc_being_read[i][0] == recid) // this record is being written (or is being read) so we must wait to finish
            {
                sem_wait(writers_sem);
                flag = 1;
                break;
            }
        }
        sem_post(shm_sem); // writer wont need to take or update any value from shared memory now

        if (flag == 1)
        {
            printf("Waiting for writer to end\n");
            sem_getvalue(writers_sem, &val);
            sem_wait(writers_sem);
        }
        sem_wait(shm_sem); // access shared memory

        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->acc_being_written[i] == -1) // find an empty spot to accounts being written array to put the number of account that is being written
            {
                shm_mem->acc_being_written[i] = recid;

                int random_sleep_time = rand() % time + 1; // find random time for the writer to write to continue writting to this exact account
                shm_mem->average_writer_time = shm_mem->average_writer_time + random_sleep_time;

                printf("Writter writes for %d seconds to %d account..\n", random_sleep_time, recid);
                sem_wait(file_sem);
                while (read(fd, acc, accountsize) == accountsize)
                {
                    if (counter == recid + 1) // get the specific account
                    {

                        acc->account_balance = acc->account_balance + value;
                        if (write(fd, acc, accountsize) == -1) // change the account balance depending on the value v
                        {
                            perror("Error in write..exiting\n");
                            exit(-1);
                        }
                        sem_post(file_sem);
                        break;
                    }
                    else
                    {
                        counter++;
                    }
                }
                sem_post(shm_sem);

                sleep(random_sleep_time);

                printf("Writter stopped writting to %d account\n", recid);
                break;
            }
        }

        lseek(fd, 0, SEEK_SET);
        printf("Acccount :\nAccount firstname:%s\nAccount lastname:%s\nAccount id:%d\nAccount Balance:%d\n", acc->firstname, acc->lastname, acc->id, acc->account_balance);
        exit(0);
    }
    else
    {
        sem_wait(shm_sem);

        for (int i = 0; i < array_size; i++)
        {

            if (shm_mem->writers[i] == -1)
            { // run till you find an empty location and place writer pid
                shm_mem->writers[i] = (int)pid;
                break;
            }
        }

        sem_post(shm_sem);

        sem_wait(shm_sem);
        shm_mem->n_of_writers = shm_mem->n_of_writers + 1;
        sem_post(shm_sem);

        printf("Array of Active Writer Processes (Pids): \n");

        sem_wait(shm_sem);

        for (int i = 0; i < array_size; i++)
        {

            if (shm_mem->writers[i] != -1)
            {
                printf("%d \n", shm_mem->writers[i]);
            }
        }

        sem_post(shm_sem);
    }

    wait(NULL);

    sem_wait(shm_sem);

    for (int i = 0; i < array_size; i++)
    {

        if (shm_mem->writers[i] != -1)
        {
            shm_mem->writers[i] = -1;
        }
    }
    shm_mem->n_of_recs_proc = shm_mem->n_of_recs_proc + 1;

    sem_post(shm_sem);

    sem_wait(shm_sem);
    for (int i = 0; i < array_size; i++)
    {
        if (shm_mem->acc_being_written[i] == recid)
        {
            shm_mem->acc_being_written[i] = -1;
            sem_getvalue(writers_sem, &val);
            if (val == 0)
            {
                sem_post(writers_sem); // writer ended for specific account so post the writer semaphore
                sem_post(readers_sem); // writer ended for specific account so post the reader semaphore
            }
            break;
        }
    }
    sem_post(shm_sem);

    sem_close(file_sem);
    sem_close(readers_sem);
    sem_close(writers_sem);
    sem_close(shm_sem);
    close(fd);
}

int parsing(int argc, char *argv[], char **filename, int *time, char **shmid, int *recid, int *value)
{
    int flagf = 0;
    int flags = 0;
    int flagd = 0;
    int flagl = 0;
    int fd;
    if (argc == 11)
    {
        for (int i = 1; i < argc; i++) // check each user argument
        {

            if (strcmp(argv[i], "-f") == 0) // if flag is -f
            {
                flagf = 1;
                if (argv[i + 1] != NULL)
                {
                    *filename = argv[i + 1];      // set filename in main
                    fd = open(*filename, O_RDWR); // open file for reading
                    if (fd == -1)
                    {
                        perror("Error Opening File,exiting..\n");
                        exit(-1);
                    }
                    else
                    {
                        perror("File opened successfully!\n");
                    }
                }
                else
                {
                    perror("Sorry Wrong file name!Try Again!..exiting\n"); // user need to provide a filename after flag -f
                    exit(-1);
                }
            }
            else if (strcmp(argv[i], "-l") == 0) // if flag is -l
            {
                flagl = 1;
                if (argv[i + 1] != NULL)
                {
                    char *temp;
                    long int n = strtol(argv[i + 1], &temp, 10);

                    if (*temp != '\0')
                    {
                        perror("Sorry time must be an int..exiting\n");
                    }
                    else
                    {
                        *recid = (int)n;
                        int number_of_accs = find_number_of_accounts(fd);
                        if (*recid < 0 || *recid > (number_of_accs - 1))
                        {
                            perror("Sorry the recid you provided is out of range!..exiting\n");
                            exit(-1);
                        }
                    }
                }
                else
                {
                    perror("Provide a number after flag -l and not an int\n"); // user need to provide an int after flag -l
                    exit(-1);
                }
            }
            else if (strcmp(argv[i], "-v") == 0)
            {
                if (argv[i + 1] != NULL)
                {
                    char *temp;
                    long int n = strtol(argv[i + 1], &temp, 10);

                    if (*temp != '\0')
                    {
                        perror("Sorry time must be an int..exiting\n");
                    }
                    else
                    {
                        *value = (int)n;
                    }
                }
                else
                {
                    perror("Sorry you must enter a value after -v flag..exiting\n");
                }
            }
            else if (strcmp(argv[i], "-d") == 0) // if flag is -d
            {
                flagd = 1;
                if (argv[i + 1] != NULL)
                {
                    char *temp;
                    long int ti = strtol(argv[i + 1], &temp, 10);

                    if (*temp != '\0')
                    {
                        perror("Sorry time must be an int..exiting\n");
                    }
                    else
                    {
                        *time = ti;
                    }
                }
            }
            else if (strcmp(argv[i], "-s") == 0)
            { // if flag is -s
                flags = 1;
                if (argv[i + 1] != NULL)
                {
                    *shmid = argv[i + 1];
                }
                else
                {
                    perror("Please provide a string after -s flag..exiting\n"); // user needs to provide a string after flag -s
                    exit(-1);
                }
            }
        }
        if (flagd == 0 || flagf == 0 || flagl == 0 || flags == 0)
        {
            perror("Something is missing..exiting!\n");
        }
    }
    else
    {
        perror("Sorry,you provided more or less arguments..exiting\n");
        exit(-1);
    }
    return fd;
}
int find_number_of_accounts(int fd)
{
    size_t accountsize = sizeof(struct account);                 // size of one account in file
    struct account *acc = (struct account *)malloc(accountsize); // dynamically allocate memory for one account
    if (acc == NULL)
    {
        perror("malloc failed\n");
        exit(-1);
    }
    int numberofaccs = 0;
    while ((size_t)read(fd, acc, accountsize) == accountsize) // read each account from file
    {
        numberofaccs++; // find the total number of accounts in file
    }
    free(acc);
    return numberofaccs;
}
