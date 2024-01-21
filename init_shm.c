#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "account.h"
#include "shm.h"
int main(int argc, char *argv[])
{
    sem_unlink("writers");
    sem_unlink("readers");
    sem_unlink("file");
    sem_unlink("shared_memory");
    struct shm *shm_seg;

    int shm_size = sizeof(struct shm);

    const char *shmid = "my_smh";

    int shm_fd;
    shm_fd = shm_open(shmid, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        perror("Can't open shared memory segment..exiting\n");
        exit(-1);
    }
    int r = ftruncate(shm_fd, shm_size);
    if (r < 0)
    {
        if (shm_unlink(shmid) < 0)
        {
            perror("Can't unlink shared memory..exiting\n");
            exit(-1);
        }
        perror("Error executing ftruncate..exiting\n");
        exit(-1);
    }
    shm_seg = (struct shm *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_seg == NULL)
    {
        if (shm_unlink(shmid) < 0)
        {
            perror("Can't unlink shared memory..exiting\n");
            exit(-1);
        }
        perror("Error in mmap..exiting\n");
        exit(-1);
    }
    printf("Succesfully created shared memory segment with id %s\n", shmid);
    for (int i = 0; i < array_size; i++) // initialize every array with -1
    {
        shm_seg->readers[i] = -1;
        shm_seg->writers[i] = -1;
        shm_seg->acc_being_read[i][0] = -1;
        shm_seg->acc_being_read[i][1] = -1;
        shm_seg->acc_being_written[i] = -1;
    }
    shm_seg->n_of_readers = 0;
    shm_seg->n_of_recs_proc = 0;
    shm_seg->n_of_writers = 0;
    shm_sem = sem_open("shared_memory", O_CREAT, S_IRUSR | S_IWUSR, 1); // create semaphore for accessing the shared memory
    readers_sem = sem_open("readers", O_CREAT, S_IRUSR | S_IWUSR, 1);   // created semaphore for readers
    writers_sem = sem_open("writers", O_CREAT, S_IRUSR | S_IWUSR, 1);
    file_sem = sem_open("file", O_CREAT, S_IRUSR | S_IWUSR, 1);

    if (shm_sem < 0 || readers_sem < 0 || file_sem < 0 || writers_sem < 0)
    {
        perror("Error in sem_open..exiting\n");
        exit(-1);
    }
    printf("Succesfully created each semaphore \n");
    if (sem_close(shm_sem) < 0)
    {
        perror("Error in sem_close..exiting\n");
        exit(-1);
    }
    if (sem_close(readers_sem) < 0)
    {
        perror("Error in sem_close..exiting\n");
        exit(-1);
    }
    if (sem_close(writers_sem) < 0)
    {
        perror("Error in sem_close..exiting\n");
        exit(-1);
    }
    if (sem_close(file_sem) < 0)
    {
        perror("Error in sem_close..exiting\n");
        exit(-1);
    }
}
