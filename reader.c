#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "reader.h"
#include "account.h"
#include "shm.h"
#include "time.h"
int main(int argc, char *argv[])
{
    srand(time(NULL));

    int val;
    char *filename;
    int time;
    char *shmid;
    int recid1, recid2;
    int shm_fd;
    int fd = parsing(argc, argv, &filename, &time, &shmid, &recid1, &recid2);

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

    if (recid2 == 0) // ony one account ton be read
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            int flag1 = 0;
            size_t accountsize = sizeof(struct account);                 // size of one account in file
            struct account *acc = (struct account *)malloc(accountsize); // dynamically allocate memory for one account
            int counter = 0;
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->acc_being_written[i] == recid1) // this record is being written so we must wait to finish
                {
                    sem_wait(readers_sem);
                    flag1 = 1;
                    break;
                }
            }
            sem_post(shm_sem);
            if (flag1 == 1)
            { // it means that reader should wait
                printf("Reader is waiting for writer to finish");
                sem_wait(readers_sem);
            }

            val = sem_getvalue(readers_sem, &val);

            int flag = 0;

            for (int i = 0; i < array_size; i++)
            {
                sem_wait(shm_sem);
                if (shm_mem->acc_being_read[i][0] == recid1)
                {
                    flag = 1;
                    shm_mem->acc_being_read[i][1] = shm_mem->acc_being_read[i][1] + 1;
                    int random_sleep_time = rand() % time + 1; // find random time for the reader to read specific account
                    shm_mem->average_reader_time = random_sleep_time + shm_mem->average_reader_time;
                    printf("Reader reads for %d seconds to %d account..\n", random_sleep_time, recid1);

                    sem_wait(file_sem);
                    while (read(fd, acc, accountsize) == accountsize)
                    {
                        if (counter == recid1)
                        {
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
                    printf("Reader stopped reading %d account\n", recid1);
                    break;
                }
                sem_post(shm_sem);
            }

            if (flag == 0)
            {
                sem_wait(shm_sem);

                for (int i = 0; i < array_size; i++)
                {
                    if (shm_mem->acc_being_read[i][0] == -1)
                    {
                        shm_mem->acc_being_read[i][0] = recid1;
                        shm_mem->acc_being_read[i][1] = 1;
                        int random_sleep_time = rand() % time + 1; // find random time for the reader to read specific account
                        shm_mem->average_reader_time = random_sleep_time + shm_mem->average_reader_time;
                        printf("Reader reads for %d seconds to %d account..\n", random_sleep_time, recid1);
                        sem_wait(file_sem);
                        while (read(fd, acc, accountsize) == accountsize)
                        {
                            if (counter == recid1)
                            {
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
                        printf("Reader stopped reading %d account\n", recid1);
                        break;
                    }
                }
            }

            printf("Acccount :\nAccount firstname:%s\nAccount lastname:%s\nAccount id:%d\nAccount Balance:%d\n", acc->firstname, acc->lastname, acc->id, acc->account_balance);

            printf("Average balance of accounts read %d\n", acc->account_balance);

            exit(0);
        }
        else
        {
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->readers[i] == -1)
                { // run till you find an empty location and place reader pid
                    shm_mem->readers[i] = (int)pid;
                    break;
                }
            }
            sem_post(shm_sem);
            sem_wait(shm_sem);
            shm_mem->n_of_readers = shm_mem->n_of_readers + 1; // input indicates to read only one account so accounts read will be one more
            sem_post(shm_sem);
            printf("Array of Active Read Processes (Pids): \n");
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->readers[i] != -1)
                {
                    printf("%d \n", shm_mem->readers[i]);
                }
            }

            sem_post(shm_sem);
        }
        wait(NULL);

        sem_wait(shm_sem);
        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->readers[i] != -1)
            {
                shm_mem->readers[i] = -1;
            }
        }
        sem_post(shm_sem);
        // now we need to set this pid to -1 because it finished its execution
        sem_wait(shm_sem);

        shm_mem->n_of_recs_proc = shm_mem->n_of_recs_proc + 1;

        sem_post(shm_sem);

        sem_wait(shm_sem);
        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->acc_being_read[i][0] == recid1)
            {
                if (shm_mem->acc_being_read[i][1] == 1)
                {
                    shm_mem->acc_being_read[i][0] = -1;
                    shm_mem->acc_being_read[i][1] = -1;
                    sem_getvalue(writers_sem, &val);
                    if (val == 0)
                    {
                        sem_post(writers_sem);
                    }
                }
                else
                {
                    shm_mem->acc_being_read[i][1] = shm_mem->acc_being_read[i][1] - 1;
                }
                break;
            }
        }
        sem_post(shm_sem);
    }
    else // for multiple accounts to be read
    {

        pid_t pid = fork();

        if (pid == 0)
        {
            int flag1 = 0;
            size_t accountsize = sizeof(struct account);                 // size of one account in file
            struct account *acc = (struct account *)malloc(accountsize); // dynamically allocate memory for one account
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->acc_being_written[i] >= recid1 && shm_mem->acc_being_written[i] <= recid2) // this record is being written so we must wait to finish
                {
                    sem_wait(readers_sem);
                    flag1 = 1;
                    break;
                }
            }
            sem_post(shm_sem);

            if (flag1 == 1)
            { // it means that reader should wait
                printf("Reader is waiting for writer to finish");
                sem_wait(readers_sem);
            }

            val = sem_getvalue(readers_sem, &val);
            sem_wait(shm_sem);
            for (int j = recid1; j <= recid2; j++)
            {
                int flag = 0;
                for (int i = 0; i < array_size; i++)
                {
                    if (shm_mem->acc_being_read[i][0] == j) // if any of those accounts is already being read
                    {
                        flag = 1;
                        shm_mem->acc_being_read[i][1] = shm_mem->acc_being_read[i][1] + 1; // add to it that it is being read one more time
                        break;
                    }
                }
                if (flag == 0)
                { // acc doesnt exist in this array then add it
                    for (int i = 0; i < array_size; i++)
                    {
                        if (shm_mem->acc_being_read[i][0] == -1)
                        {
                            shm_mem->acc_being_read[i][0] = j;
                            shm_mem->acc_being_read[i][1] = 1;
                            break;
                        }
                    }
                }
            }
            sem_post(shm_sem);

            int counter = 0;

            int acc_balance;
            int random_sleep_time = rand() % time + 1; // find random time for the reader to read specific account
            printf("Reader reads for %d seconds to %d-%d accounts..\n", random_sleep_time, recid1, recid2);
            sem_wait(shm_sem);
            shm_mem->average_reader_time = random_sleep_time + shm_mem->average_reader_time;
            sem_post(shm_sem);

            sem_wait(file_sem);

            while (read(fd, acc, accountsize) == accountsize)
            {

                if (counter >= recid1 && counter <= recid2)
                {
                    acc_balance = acc_balance + acc->account_balance;
                    printf("Acccount :\nAccount firstname:%s\nAccount lastname:%s\nAccount id:%d\nAccount Balance:%d\n", acc->firstname, acc->lastname, acc->id, acc->account_balance);
                }

                counter++;
            }
            sem_post(file_sem);
            printf("Reader stopped reading %d-%d accounts\n", recid1, recid2);

            printf("Average balance of accounts read %d\n", acc_balance / (recid2 - recid1));
            exit(0);
        }
        else
        {
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->readers[i] == -1)
                { // run till you find an empty location and place reader pid
                    shm_mem->readers[i] = (int)pid;
                    break;
                }
            }
            sem_post(shm_sem);

            sem_wait(shm_sem);
            shm_mem->n_of_readers = shm_mem->n_of_readers + 1; // input indicates to read only one account so accounts read will be one more
            sem_post(shm_sem);
            printf("Array of Active Read Processes (Pids): \n");
            sem_wait(shm_sem);
            for (int i = 0; i < array_size; i++)
            {
                if (shm_mem->readers[i] != -1)
                {
                    printf("%d \n", shm_mem->readers[i]);
                }
            }

            sem_post(shm_sem);
        }
        wait(NULL);

        sem_wait(shm_sem);
        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->readers[i] != -1)
            {
                shm_mem->readers[i] = -1;
            }
        }
        sem_post(shm_sem);
        // now we need to set this pid to -1 because it finished its execution
        sem_wait(shm_sem);

        shm_mem->n_of_recs_proc = shm_mem->n_of_recs_proc + (recid2 - recid1);

        sem_post(shm_sem);

        sem_wait(shm_sem);
        for (int i = 0; i < array_size; i++)
        {
            if (shm_mem->acc_being_read[i][0] >= recid1 && shm_mem->acc_being_read[i][0] <= recid2)
            {
                if (shm_mem->acc_being_read[i][1] == 1)
                {
                    shm_mem->acc_being_read[i][0] = -1;
                    shm_mem->acc_being_read[i][1] = -1;
                    sem_getvalue(writers_sem, &val);
                    if (val == 0)
                    {
                        sem_post(writers_sem);
                    }
                }
                else
                {
                    shm_mem->acc_being_read[i][1] = shm_mem->acc_being_read[i][1] - 1;
                }
                break;
            }
        }
        sem_post(shm_sem);
    }

    sem_close(file_sem);
    sem_close(readers_sem);
    sem_close(writers_sem);
    sem_close(shm_sem);
    close(fd);
}
int parsing(int argc, char *argv[], char **filename, int *time, char **shmid, int *recid1, int *recid2)
{
    int num1, num2;
    int flagf = 0;
    int flags = 0;
    int flagd = 0;
    int flagl = 0;
    int fd;
    int n_of_accs; // number of accounts in file

    if (argc == 9)
    {
        for (int i = 1; i < argc; i++) // check each user argument
        {

            if (strcmp(argv[i], "-f") == 0) // if flag is -f
            {
                flagf = 1;
                if (argv[i + 1] != NULL)
                {
                    *filename = argv[i + 1];        // set filename in main
                    fd = open(*filename, O_RDONLY); // open file for reading
                    if (fd == -1)
                    {
                        perror("Error Opening File,exiting..\n");
                        exit(-1);
                    }
                    else
                    {
                        perror("File opened successfully!\n");
                    }
                    n_of_accs = find_number_of_accounts(fd);
                    lseek(fd, 0, SEEK_SET);
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
                    if (sscanf(argv[i + 1], "%d,%d", &num1, &num2) >= 1)
                    {
                        if (sscanf(argv[i + 1], "%*d,%d", &num2) == 1)
                        {
                            if (num1 < num2)
                            {
                                if (num1 < 0 || num2 > n_of_accs - 1)
                                { // range from 0 to total-1
                                    perror("Sorry the recid you provided is out of range!..exiting\n");
                                    exit(-1);
                                }
                                else
                                {
                                    *recid1 = num1;
                                    *recid2 = num2;
                                }
                            }
                            else
                            {
                                perror("Provide a correct range of accounts to read(x1<x2)(300,500)..exiting\n");
                                exit(-1);
                            }
                        }
                        else
                        {
                            if (num1 < 0 || num1 > n_of_accs - 1)
                            {
                                perror("Sorry the recid you provided is out of range!..exiting\n");
                                exit(-1);
                            }
                            else
                            {
                                *recid1 = num1;
                            }
                        }
                    }
                    else
                    {
                        perror("Sorry ypu can provide only one recid,or a range of two numbers..exiting\n");
                        exit(-1);
                    }
                }
                else
                {
                    perror("Provide a number after flag -l and not an int\n"); // user need to provide an int after flag -l
                    exit(-1);
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
