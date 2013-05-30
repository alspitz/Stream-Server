/***************************************************************
 * Writes data from audio in (the microphone) to a given buffer.
 * Uses ALSA to get access to the audio device. Mostly standard
 * ALSA API usage. Here's a good resource:
 * http://www.linuxjournal.com/article/6735
 *
 * Sample rate and format must be edited manually.
 *
 * For a more portable and less stable version, see
 * capture-portaudio.c
 *
 * -- Alex Spitzer 2013
 *
 ***************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <alsa/asoundlib.h>

#include "server.h"

void* record(void *param) 
{
    struct thread_data* td = (struct thread_data *) param;
    snd_pcm_t *pcm_handle;  /* handle for the PCM device */

    /* Capture stream */
    snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;

    /* This struct contains info about   */
    /* the hardware and specifies the    */
    /* configuration used for the stream */
    snd_pcm_hw_params_t *hw_params;

    /* Name of the PCM device        */
    /* First number is the soundcard */
    /* Second number is the device   */
    char *pcm_name = strdup("plughw:0,0");

    snd_pcm_hw_params_alloca(&hw_params);

    /* Open PCM */
    if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
        fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
        return (void *)1;
    }

    /* Init hw_params with full configuration space */
    if (snd_pcm_hw_params_any(pcm_handle, hw_params) < 0) {
        fprintf(stderr, "Can not configure this PCM device.\n");
        return (void *)1;
    }

    unsigned int rate = 48000; /* Sample rate */
    unsigned int exact_rate;

    if (snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        fprintf(stderr, "Error setting access.\n");
        return (void *)1;
    }

    if (snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0) {
        fprintf(stderr, "Error setting format.\n");
        return (void *)1;
    }

    exact_rate = rate;
    if (snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &exact_rate, 0) < 0) {
        fprintf(stderr, "Error setting rate.\n");
        return (void *)1;
    }
    if (rate != exact_rate) {
        fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n ==> Using %d Hw isntead.\n", rate, exact_rate);
    }

    if (snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1) < 0) {
        fprintf(stderr, "Error setting channels.\n");
        return (void *)1;
    }
    
    if (snd_pcm_hw_params(pcm_handle, hw_params) < 0) {
        fprintf(stderr, "Error settings HW params.\n");
        return (void *)1;
    }

    if (snd_pcm_prepare(pcm_handle) < 0) {
        fprintf(stderr, "Error preparing audio interface for use.\n");
        return (void *)1;
    }
 
    int *writePtr = td->writePtr;

    while (1) {
        if (snd_pcm_readi(pcm_handle, (td->DATA + *writePtr), td->CHUNK_SIZE) != td->CHUNK_SIZE) {
            fprintf(stderr, "Error reading from audio interface.\n");
            return (void *)1;
        }
        pthread_mutex_lock(&td->mutex);
        *writePtr = (*writePtr + td->CHUNK_SIZE) % td->BUFFER_SIZE;
        pthread_cond_broadcast(&td->cond);
        pthread_mutex_unlock(&td->mutex);

        
    }
    return NULL;
}
