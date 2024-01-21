#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "account.h"
#include "test.h"
#include <time.h>
#define how_many_proc 2

int main(int argc, char *argv[])
{
    int fd = parsing(argc, argv);
    int number_of_accs = find_number_of_accounts(fd);
    close(fd);
    char *reader = "./reader";
    char *writer = "writer";
    char *arg1 = "-f";
    char *arg2 = "-d";
    char *arg3 = "-v";
    char *arg4 = "-l";
    char *arg5 = "-s";
    char *arg6 = "my_smh";
    char sleep_time[32];
    char single_account[32];
    char balance[32];

    char range[32];
    for (int i = 0; i < how_many_proc; i++)
    {
        if (fork() == 0)
        {
            unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)getpid();
            srand(seed);
            int read_or_write = rand() % 2; // create random number from 0 to 1
            if (read_or_write == 0)         // reader
            {
                int multiple_or_single = rand() % 2; // value that indicates to read multiple accounts or a single one
                if (multiple_or_single == 0)
                {
                    int select_random_acc = rand() % number_of_accs; // select random record from 0 to accounts-1
                    int random_sleep_time = rand() % 10;             // random sleep time from 0 to 20
                    sprintf(sleep_time, "%d", random_sleep_time);
                    sprintf(single_account, "%d", select_random_acc);
                    char *args[] = {reader, arg1, argv[2], arg4, single_account, arg2, sleep_time, arg5, arg6, NULL};
                    execv(reader, args);
                    exit(0);
                }
                else
                {                                              // multiple accounts to read
                    int lower_bound = rand() % number_of_accs; // lower bound
                    int upper_bound = rand() % (number_of_accs - lower_bound) + lower_bound;
                    int random_sleep_time = rand() % 10; // random sleep time from 0 to 20
                    sprintf(sleep_time, "%d", random_sleep_time);
                    sprintf(range, "%d,%d", lower_bound, upper_bound);

                    char *args[] = {reader, arg1, argv[2], arg4, range, arg2, sleep_time, arg5, arg6, NULL};
                    execv(reader, args);

                    exit(0);
                }
            }
            else // writer
            {
                int account_id = rand() % number_of_accs;
                int random_sleep_time = rand() % 10;
                int random_account_balance = rand() % 2001 - 1000; // create balance from -1000 to 1000

                sprintf(sleep_time, "%d", random_sleep_time);
                sprintf(single_account, "%d", account_id);
                sprintf(balance, "%d", random_account_balance);
                char *args[] = {writer, arg1, argv[2], arg4, single_account, arg2, sleep_time, arg3, balance, arg5, arg6, NULL};
                execv(writer, args);
                exit(0);
            }
        }
    }

    for (int i = 0; i < 2; i++)
    {
        wait(NULL);
    }
    if (fork() == 0)
    {
        char *args[] = {"./delete_print", NULL};
        execv("./delete_print", args);
    }
    else
    {
    }
    wait(NULL);
}
int parsing(int argc, char *argv[])
{
    int flagf;
    char *filename;
    int fd;
    if (argc > 3 || argc < 3)
    {
        perror("sorry wrong you provided more or less arguments..exiting\n");
        exit(-1);
    }
    else
    {
        for (int i = 1; i < argc; i++) // check each user argument
        {

            if (strcmp(argv[i], "-f") == 0) // if flag is -f
            {
                flagf = 1;
                if (argv[i + 1] != NULL)
                {
                    filename = argv[i + 1];      // set filename
                    fd = open(filename, O_RDWR); // open file for reading
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
        }
        if (flagf == 0)
        {
            perror("Please provide flag -f and a filename after that flag..exiting\n");
            exit(-1);
        }
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