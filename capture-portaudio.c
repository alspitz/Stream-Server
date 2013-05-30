/***************************************************************
 * Writes data from audio in (the microphone) to a given buffer.
 * Uses the cross platform PortAudio library to get access to 
 * the audio device. Has been tested on Linux (ALSA) and found
 * to be slightly problematic, though usable. Should function on
 * most other platforms as well, though it is untested. For a more
 * native version for Linux, see capture.c
 *
 * -- Alex Spitzer 2013
 *
 ***************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <portaudio.h>

#include "server.h"


PaStream *stream;

void record_thread_interrupt_handler(int signum) {
    printf("Record thread quitting...\n");
    Pa_CloseStream(stream);
    Pa_Terminate();
    exit(signum);
}

void* record(void *param) 
{
    struct thread_data* td = (struct thread_data *) param;

    PaError err;
    if ((err = Pa_Initialize()) != paNoError) {
        fprintf(stderr, "Error initializing PortAudio: %s\n", Pa_GetErrorText(err));
        return (void *)1;
    }

    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    
    /* Open the stream */
    if ((err = Pa_OpenStream(&stream, &inputParameters, NULL, 48000, td->CHUNK_SIZE, paClipOff, NULL, NULL)) != paNoError) {
        fprintf(stderr, "Error opening stream: %s\n", Pa_GetErrorText(err));
        return (void *)1;
    }

    signal(SIGINT, record_thread_interrupt_handler); // Register the interrupt handler

    /* Start the stream */
    if ((err = Pa_StartStream(stream)) != paNoError) {
        fprintf(stderr, "Error starting stream: %s\n", Pa_GetErrorText(err));
        return (void *)1;
    }

    int *writePtr = td->writePtr;

    while (1) {
        if ((err = Pa_ReadStream(stream, (td->DATA + *writePtr), td->CHUNK_SIZE)) != paNoError) {
            fprintf(stderr, "Error reading from audio interface: %s\n", Pa_GetErrorText(err));
            return (void *)1;
        }
        pthread_mutex_lock(&td->mutex);
        *writePtr = (*writePtr + td->CHUNK_SIZE) % td->BUFFER_SIZE;
        pthread_cond_broadcast(&td->cond);
        pthread_mutex_unlock(&td->mutex);

    }
    return NULL;
}
