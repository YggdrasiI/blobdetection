#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK_CONSUMED_CHARS(CONSUMED, BUF, USED) \
            if( CONSUMED < 0 ){ \
                fprintf(stderr, "(%s) Hey sprintf failed in line %i\n", __FILE__, __LINE__); \
                BUF[USED] = '\0'; \
                return BUF; \
            }

#define ID_TO_RGB_A(ID, COL) \
            (COL)[0] =  ((ID)*5*5+100)%256; \
            (COL)[1] =  ((ID)*7*7+10)%256;  \
            (COL)[2] =  ((ID)*29*29+1)%256;

// Darker
#define ID_TO_RGB_B(ID, COL) \
            (COL)[0] =  ((ID)*5*12382511+100)%180; \
            (COL)[1] =  ((ID)*7*32758233+10)%180;  \
            (COL)[2] =  ((ID)*29*4817372229+1)%180;

#define RESETCOLOR "\e[m" // "\033[0m" aka "\x1b[0m"

/* Values for set_background flag in sprintf_color(...) */
#define CHANGE_FOREGROUND 0
#define CHANGE_BACKGROUND 1
#define CHANGE_BACKGROUND_PLUS 2 /* Change background, but also adapt forground color. */

/* Check if terminal supports rgb color values. Printing out unsupported codes
 * could messing up the terminal content complety.
 *
 * If term_color_mode is < 0, set_term_color_mode will be called by the
 * *_coloured_* functions.
 */
#define COLOR_SW_MODE 0
#define COLOR_LIMITED_MODE 1
#define COLOR_RGB_MODE 2
#define _BUFSIZE 128


extern int32_t term_color_mode;
extern int32_t term_num_colors;


int32_t set_term_color_mode(void);

int32_t sprintf_color(char *buf,
        int32_t set_background,
        int32_t r, int32_t g, int32_t b,
        const char *pextra_str);

void expand_buf_if_required(
        size_t *pbuf_len,  // Length of *ppbuf
        char **ppbuf,
        size_t already_used,  // <= *pbuf_len
        size_t required,
        size_t extra_padding  // reallocation to size buf_len-buf_used + required  + extra_padding
        );
#endif
