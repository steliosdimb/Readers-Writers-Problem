#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "shm.h"
int main(void)
{

    char *shmid = "my_smh";
    struct shm *shm_mem;
    int shm_fd;

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
    printf("\n\n\n");
    printf("Final statistics:\n");
    printf("Readers worked on file:%d\n", shm_mem->n_of_readers);
    printf("Average time that readers worked:%d\n", shm_mem->average_reader_time / shm_mem->n_of_readers);
    printf("Writers worked on file:%d\n", shm_mem->n_of_writers);
    printf("Average time that writers worked:%d\n", shm_mem->average_writer_time / shm_mem->n_of_writers);
    printf("Total accounts that were written or read:%d\n\n\n", shm_mem->n_of_recs_proc);
    if (munmap(shm_mem, shm_size) < 0)
    {
        perror("Error in munmap..exiting\n");
        exit(-1);
    }
    close(shm_fd);
    if (shm_unlink(shmid) < 0)
    {
        perror("Error in shm unlink..exiting\n");
        exit(-1);
    }

    sem_unlink("file");
    sem_unlink("shared_memory");
    sem_unlink("readers");
    sem_unlink("writers");
}
