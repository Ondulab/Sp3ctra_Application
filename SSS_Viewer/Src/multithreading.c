#include "config.h"

#include <SFML/Graphics.h>
#include <SFML/Network.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include<arpa/inet.h>
#include<sys/socket.h>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>

#include "error.h"
#include "synth.h"
#include "display.h"
#include "udp.h"
#include "audio.h"
#include "multithreading.h"
#include "context.h"

void initDoubleBuffer(DoubleBuffer *db) {
    pthread_mutex_init(&db->mutex, NULL);
    pthread_cond_init(&db->cond, NULL);
    db->activeBuffer = db->bufferA;
    db->processingBuffer = db->bufferB;
    db->dataReady = false;
}

void swapBuffers(DoubleBuffer *db) {
    int32_t *temp = db->activeBuffer;
    db->activeBuffer = db->processingBuffer;
    db->processingBuffer = temp;
}

void *udpThread(void *arg) {
    Context *ctx = (Context *)arg;
    DoubleBuffer *db = ctx->doubleBuffer;
    int s = ctx->socket;  // Assurez-vous que le socket UDP est correctement initialisé
    struct sockaddr_in *si_other = ctx->si_other;
    socklen_t slen = sizeof(*si_other);

    ssize_t recv_len;
    uint32_t buf[UDP_PACKET_SIZE];
    static uint32_t curr_packet = 0, curr_packet_header = 0;

    while (true) {  // ou une autre condition pour la boucle
        for (curr_packet = 0; curr_packet < UDP_NB_PACKET_PER_LINE; curr_packet++) {
            recv_len = recvfrom(s, (uint8_t *)buf, UDP_PACKET_SIZE * sizeof(int32_t), 0, (struct sockaddr *)si_other, &slen);
            if (recv_len == -1) {
                // Gérer l'erreur de manière appropriée
                continue;
            }

            //pthread_mutex_lock(&db->mutex);
            curr_packet_header = buf[0];
            memcpy(&db->activeBuffer[curr_packet_header], &buf[1], recv_len - (UDP_HEADER_SIZE * sizeof(int32_t)));

            db->dataReady = true;
            pthread_cond_signal(&db->cond);
            //pthread_mutex_unlock(&db->mutex);
        }

        // Attendez avant de traiter le prochain paquet ou ajoutez une logique pour gérer la fin de la réception
        usleep(1);
    }
    return NULL;
}

void *imageProcessingThread(void *arg) {
    Context *context = (Context *)arg;
    DoubleBuffer *db = context->doubleBuffer;
    sfRenderWindow *window = context->window;

    // Initialisation de la texture et du sprite pour l'affichage
    sfTexture* background_texture = sfTexture_create(WINDOWS_WIDTH, WINDOWS_HEIGHT);
    sfTexture* foreground_texture = sfTexture_create(WINDOWS_WIDTH, WINDOWS_HEIGHT);
    sfSprite* background_sprite = sfSprite_create();
    sfSprite* foreground_sprite = sfSprite_create();
    sfSprite_setTexture(background_sprite, background_texture, sfTrue);
    sfSprite_setTexture(foreground_sprite, foreground_texture, sfTrue);

    while (sfRenderWindow_isOpen(window)) {  // Remplacez ceci par votre condition de continuation
        //pthread_mutex_lock(&db->mutex);
        while (!db->dataReady) {
            pthread_cond_wait(&db->cond, &db->mutex);
        }

        // Traiter les données dans db->processingBuffer
        printImage(window, db->processingBuffer, background_texture, foreground_texture);
        
        // Après le traitement, marquez que les données ont été traitées
        db->dataReady = false;

        // Échangez les buffers pour préparer le prochain cycle de traitement
        swapBuffers(db);

        //pthread_mutex_unlock(&db->mutex);

        // Autres logiques d'affichage si nécessaire
        // ...
        //usleep(10);
    }

    // Nettoyage des ressources graphiques
    sfTexture_destroy(background_texture);
    sfTexture_destroy(foreground_texture);
    sfSprite_destroy(background_sprite);
    sfSprite_destroy(foreground_sprite);

    return NULL;
}


void *audioProcessingThread(void *arg) {
    Context *context = (Context *)arg;
    DoubleBuffer *db = context->doubleBuffer;

    while (true) { // Continuez tant que le programme tourne
        //pthread_mutex_lock(&db->mutex);
        
        //while (!db->dataReady) {
        //    pthread_cond_wait(&db->cond, &db->mutex);
        //}

        // Appel de synth_AudioProcess avec les données du buffer de traitement
        synth_AudioProcess(db->processingBuffer, audio_samples);

        // Marquer que les données ont été traitées
        db->dataReady = false;

        // Échanger les buffers pour le prochain cycle
        swapBuffers(db);

        //pthread_mutex_unlock(&db->mutex);
    }

    // Ajoutez ici le nettoyage nécessaire si le thread doit se terminer
    return NULL;
}


