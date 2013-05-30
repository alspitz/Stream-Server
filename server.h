
#ifndef THREAD_DATA
#define THREAD_DATA

struct thread_data {
    short *DATA;
    int *writePtr;
    int CHUNK_SIZE;
    int BUFFER_SIZE;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

#endif
