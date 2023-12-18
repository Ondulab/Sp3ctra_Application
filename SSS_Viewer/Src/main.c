/*
 ============================================================================
 Name        : SSS_Viewer.c
 Author      : ZHONX
 Version     :
 Copyright   : Your copyright notice
 Description : SSS image visualization
 ============================================================================
 */
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

//#define CALIBRATION
//#define SSS_MOD_MODE

int main(void) {
    // Initialisation CSFML
    sfVideoMode mode = {WINDOWS_WIDTH, WINDOWS_HEIGHT, 32};
    sfRenderWindow *window = sfRenderWindow_create(mode, "CSFML Viewer", sfResize | sfClose, NULL);
    if (!window) {
        perror("Erreur de création de fenêtre CSFML");
        return EXIT_FAILURE; // Assurez-vous que la fenêtre est créée
    }

    // Initialisation UDP et Audio
    struct sockaddr_in si_other, si_me;
    
    AudioData audioData;
    initAudioData(&audioData, AUDIO_CHANNEL, AUDIO_BUFFER_SIZE);
    audio_Init(&audioData);
    synth_IfftInit();
    display_Init(window);

    int s = udp_Init(&si_other, &si_me);
    if (s < 0) {
        perror("Erreur d'initialisation UDP");
        // Libération des ressources allouées précédemment
        sfRenderWindow_destroy(window);
        return EXIT_FAILURE;
    }

    OSStatus status = startAudioUnit();  // Assurez-vous de gérer ce status

    // Création des threads et des structures de contexte
    DoubleBuffer db;
    initDoubleBuffer(&db);

    Context context = {
        .window = window,
        .socket = s,
        .si_other = &si_other,
        .si_me = &si_me,
        .audioData = &audioData,
        .doubleBuffer = &db,
        // ... Autres initialisations
    };

    pthread_t udpThreadId, imageThreadId, audioThreadId;

    if (pthread_create(&udpThreadId, NULL, udpThread, (void *)&context) != 0) {
        perror("Erreur de création du thread UDP");
        // Nettoyage des ressources
        sfRenderWindow_destroy(window);
        // Autres nettoyages nécessaires
        return EXIT_FAILURE;
    }
    if (pthread_create(&imageThreadId, NULL, imageProcessingThread, (void *)&context) != 0) {
        perror("Erreur de création du thread UDP");
        // Nettoyage des ressources
        sfRenderWindow_destroy(window);
        // Autres nettoyages nécessaires
        return EXIT_FAILURE;
    }
    if (pthread_create(&audioThreadId, NULL, audioProcessingThread, (void *)&context) != 0) {
        perror("Erreur de création du thread UDP");
        // Nettoyage des ressources
        sfRenderWindow_destroy(window);
        // Autres nettoyages nécessaires
        return EXIT_FAILURE;
    }

    // Boucle principale de rendu CSFML
    while (sfRenderWindow_isOpen(window)) {
        // Gestion des événements et rendu
        sfEvent event;
        while (sfRenderWindow_pollEvent(window, &event)) {
            if (event.type == sfEvtClosed) {
                sfRenderWindow_close(window);
            }
        }

        // Rendu avec CSFML (à adapter selon les besoins)
        // ...

        //sfRenderWindow_display(window);
    }

    // Nettoyage et fermeture
    pthread_join(udpThreadId, NULL);
    pthread_join(imageThreadId, NULL);
    pthread_join(audioThreadId, NULL);
    audio_Cleanup();
    cleanupAudioData(&audioData);
    sfRenderWindow_destroy(window);
    
    return 0;
}
