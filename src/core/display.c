//
//  Display.c
//  SSS_Viewer
//
//  Created by Zhonx on 16/12/2023.
//

#include "config.h"
#include <stdint.h> // Pour uint8_t, uint32_t et autres types entiers de taille fixe

#ifdef __LINUX__
// Sur Linux, vérifier si SFML est explicitement désactivé
#ifndef NO_SFML
// SFML/CSFML disponible sur Linux
// Utiliser les en-têtes CSFML pour les fichiers C
#include <CSFML/Graphics.h>
#include <CSFML/Network.h> // Si CSFML/Network est utilisé
#endif                     // NO_SFML
#else                      // Pas __LINUX__ (par exemple macOS)
// Sur les autres plateformes (comme macOS), on suppose que SFML/CSFML est
// disponible à moins que NO_SFML ne soit également défini pour ces plateformes.
#ifndef NO_SFML
// Utiliser les en-têtes CSFML pour les fichiers C
#include <CSFML/Graphics.h>
#include <CSFML/Network.h> // Si CSFML/Network est utilisé
#endif                     // NO_SFML
#endif                     // __LINUX__
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h> // For clock_gettime
#include <unistd.h>

#include "display.h"
#include "error.h"

// NOTE: All Visual Freeze Feature code previously here has been removed
// as the freeze logic is now handled in synth.c for synth data.

int display_Init(sfRenderWindow *window) {
#ifdef CLI_MODE
  if (window) {
    printf("SFML window detected in CLI mode, using it for display\n");
    printf("SFML CONFIGURED IN CLI+WINDOW MODE\n");
  } else {
    printf("Running in CLI mode, no SFML window required\n");
  }
  return 0;
#else
  if (!window) {
    die("sfRenderWindow_create failed");
  }

  printf("SFML FULL CONFIGURED\n");

  return 0;
#endif
}

// Nouvelle fonction printImageRGB acceptant 6 arguments
void printImageRGB(sfRenderWindow *window, uint8_t *buffer_R, uint8_t *buffer_G,
                   uint8_t *buffer_B, sfTexture *background_texture,
                   sfTexture *foreground_texture) {
#ifndef NO_SFML
#ifdef CLI_MODE
  // En mode CLI, vérifier si une fenêtre SFML est disponible
  if (!window || !background_texture || !foreground_texture) {
    // Si aucune fenêtre ou texture n'est disponible, on ne fait rien
    return;
  }
  // Sinon, on procède à l'affichage même en mode CLI
#else
  // Vérification de sécurité en mode normal
  if (!window || !background_texture || !foreground_texture) {
    fprintf(stderr, "Error: Missing window or textures for display\n");
    return;
  }
#endif

  // NOTE: Visual Freeze logic previously here has been removed.
  // printImageRGB now directly uses the provided buffer_R, buffer_G, buffer_B
  // for display. The decision of whether these buffers contain "live" or
  // "frozen/faded" data (derived from synth.c's processed_grayScale)
  // will be handled by the caller in main.c, which will prepare
  // appropriate R,G,B buffers to pass here.

  // Create an image of one line (width = CIS_MAX_PIXELS_NB, height = 1)
  sfImage *image = sfImage_create(CIS_MAX_PIXELS_NB, 1);
  if (image == NULL) {
    fprintf(stderr, "Error: Unable to create image\n");
    return;
  }

  // Set the color of each pixel by combining the three channels
  for (int x = 0; x < CIS_MAX_PIXELS_NB; x++) {
    sfColor color = sfColor_fromRGB(buffer_R[x], buffer_G[x], buffer_B[x]);
    sfImage_setPixel(image, x, 0, color);
  }

  // Create a texture from the image of the line
  sfTexture *line_texture = sfTexture_createFromImage(image, NULL);
  if (line_texture == NULL) {
    fprintf(stderr, "Error: Unable to create line texture\n");
    sfImage_destroy(image);
    return;
  }

  // Copy the background texture into the foreground texture with a 1-pixel
  // vertical shift
  sfTexture_updateFromTexture(foreground_texture, background_texture, 0, 1);

  // Update the foreground texture with the new image line at the top
  sfTexture_updateFromImage(foreground_texture, image, 0, 0);

  // Create a sprite to draw the foreground texture
  sfSprite *foreground_sprite = sfSprite_create();
  sfSprite_setTexture(foreground_sprite, foreground_texture, sfTrue);
  sfRenderWindow_drawSprite(window, foreground_sprite, NULL);

  // Display the window contents
  sfRenderWindow_display(window);

  // Copy the updated foreground texture back into the background texture for
  // the next iteration
  sfTexture_updateFromTexture(background_texture, foreground_texture, 0, 0);

  // Cleanup
  sfImage_destroy(image);
  // sfTexture_destroy(line_texture); // line_texture is created from image, no
  // need to destroy if image is destroyed? Actually, SFML docs say
  // sfTexture_createFromImage creates a new texture that must be destroyed.
  if (line_texture)
    sfTexture_destroy(line_texture);
  if (foreground_sprite)
    sfSprite_destroy(foreground_sprite);
#else
  // NO_SFML is defined, do nothing.
  // Add (void) casts to prevent unused parameter warnings if necessary.
  (void)window;
  (void)buffer_R;
  (void)buffer_G;
  (void)buffer_B;
  (void)background_texture;
  (void)foreground_texture;
#endif
}

// Ancienne fonction utilisant un buffer 32-bit combiné
void printImage(sfRenderWindow *window, int32_t *image_buff,
                sfTexture *background_texture, sfTexture *foreground_texture) {
#ifndef NO_SFML
#ifdef CLI_MODE
  // En mode CLI, vérifier si une fenêtre SFML est disponible
  if (!window || !background_texture || !foreground_texture) {
    return;
  }
  // Sinon, on procède à l'affichage même en mode CLI
#else
  // Vérification de sécurité en mode normal
  if (!window || !background_texture || !foreground_texture) {
    fprintf(stderr, "Error: Missing window or textures for display\n");
    return;
  }
#endif

  // Create an image for the new line
  sfImage *image = sfImage_createFromColor(CIS_MAX_PIXELS_NB, 1, sfBlack);

  // Set the color for each pixel in the new line
  for (int x = 0; x < CIS_MAX_PIXELS_NB; x++) {
    sfColor color =
        sfColor_fromRGB(image_buff[x] & 0xFF, (image_buff[x] >> 8) & 0xFF,
                        (image_buff[x] >> 16) & 0xFF);
    sfImage_setPixel(image, x, 0, color);
  }

  // Create a texture from the new line image
  sfTexture *line_texture = sfTexture_createFromImage(image, NULL);

  // Copy background texture into foreground texture with a 1-pixel downward
  // shift
  sfTexture_updateFromTexture(foreground_texture, background_texture, 0, 1);

  // Draw the new line at the top of the foreground texture
  sfTexture_updateFromImage(foreground_texture, image, 0, 0);

  // Draw the foreground texture onto the window
  sfSprite *foreground_sprite = sfSprite_create();
  sfSprite_setTexture(foreground_sprite, foreground_texture, sfTrue);
  sfRenderWindow_drawSprite(window, foreground_sprite, NULL);

  // Display the window contents
  sfRenderWindow_display(window);

  // Copy the updated foreground texture back into the background texture
  sfTexture_updateFromTexture(background_texture, foreground_texture, 0, 0);

  // Cleanup
  sfImage_destroy(image);
  if (line_texture)
    sfTexture_destroy(line_texture);
  if (foreground_sprite)
    sfSprite_destroy(foreground_sprite);
#else
  // NO_SFML is defined, do nothing.
  (void)window;
  (void)image_buff;
  (void)background_texture;
  (void)foreground_texture;
#endif
}

void printRawData(sfRenderWindow *window, uint32_t *image_buff,
                  sfTexture *background_texture,
                  sfTexture *foreground_texture) {
#ifndef NO_SFML
#ifdef CLI_MODE
  // En mode CLI, vérifier si une fenêtre SFML est disponible
  if (!window || !background_texture || !foreground_texture) {
    return;
  }
  // Sinon, on procède à l'affichage même en mode CLI
#else
  // Vérification de sécurité en mode normal
  if (!window || !background_texture || !foreground_texture) {
    fprintf(stderr, "Error: Missing window or textures for display\n");
    return;
  }
#endif

  sfRenderWindow_clear(window, sfBlack);

  // Create a vertical line for each x coordinate
  for (int x = 0; x < CIS_MAX_PIXELS_NB; x++) {
    // Draw a black vertical line
    sfVertexArray *line = sfVertexArray_create();
    sfVertexArray_setPrimitiveType(line, sfLinesStrip);
    sfVertex vertex1 = {.position = {x, 0}, .color = sfBlack};
    sfVertex vertex2 = {.position = {x, WINDOWS_HEIGHT}, .color = sfBlack};
    sfVertexArray_append(line, vertex1);
    sfVertexArray_append(line, vertex2);
    sfRenderWindow_drawVertexArray(window, line, NULL);
    sfVertexArray_destroy(line);

    // Draw a green point
    sfVertexArray *point = sfVertexArray_create();
    sfVertexArray_setPrimitiveType(point, sfPoints);
    sfVertex vertex = {
        .position = {x, (float)(image_buff[x] * (WINDOWS_HEIGHT / 8192.0f))},
        .color = sfGreen};
    sfVertexArray_append(point, vertex);
    sfRenderWindow_drawVertexArray(window, point, NULL);
    sfVertexArray_destroy(point);
  }

  // Prepare sprites for drawing textures
  sfSprite *background_sprite = sfSprite_create();
  sfSprite *foreground_sprite = sfSprite_create();
  sfSprite_setTexture(background_sprite, background_texture, sfTrue);
  sfSprite_setTexture(foreground_sprite, foreground_texture, sfTrue);

  // Draw the background texture
  sfRenderWindow_drawSprite(window, background_sprite, NULL);

  // Draw the foreground texture
  sfRenderWindow_drawSprite(window, foreground_sprite, NULL);

  // Display the window contents
  sfRenderWindow_display(window);

  // Clean up
  sfSprite_destroy(background_sprite);
  sfSprite_destroy(foreground_sprite);
#else
  // NO_SFML is defined, do nothing.
  (void)window;
  (void)image_buff;
  (void)background_texture;
  (void)foreground_texture;
#endif
}
