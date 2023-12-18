#ifndef CONTEXT_H
#define CONTEXT_H

#include <SFML/Graphics.h>
#include <SFML/Network.h>

typedef struct {
    sfRenderWindow *window;
    int socket;
    struct sockaddr_in *si_other;
    struct sockaddr_in *si_me;
    AudioData *audioData;
    DoubleBuffer *doubleBuffer;
    // Ajouter d'autres champs selon les besoins
} Context;

#endif /* CONTEXT_H */
