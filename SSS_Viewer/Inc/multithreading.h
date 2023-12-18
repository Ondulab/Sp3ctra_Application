#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 3000  // Exemple de taille de buffer

typedef struct {
    int32_t bufferA[BUFFER_SIZE];
    int32_t bufferB[BUFFER_SIZE];
    int32_t *activeBuffer;
    int32_t *processingBuffer;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool dataReady;
} DoubleBuffer;

void initDoubleBuffer(DoubleBuffer *db);
void swapBuffers(DoubleBuffer *db);
void *udpThread(void *arg);
void *imageProcessingThread(void *arg);
void *audioProcessingThread(void *arg);

#endif
