#include "semaphore.h"
#define array_size 100
struct shm
{
    int readers[array_size];           // pids of active readers
    int writers[array_size];           // pids of active writers
    int n_of_readers;                  // number of accounts red
    int n_of_writers;                  // number of accounts written
    int n_of_recs_proc;                // number of records processed
    int acc_being_read[array_size][2]; // array that indicates which accounts are being read,also the array is 2d,2nd column indicates how many readers reading currenet account,if it goes 0 then it is being removed from the array
    int acc_being_written[array_size];
    int average_reader_time;
    int average_writer_time;
};

sem_t *shm_sem;
sem_t *readers_sem;
sem_t *writers_sem;
sem_t *file_sem;