#include <stdio.h>
#include <string.h>

#include "output_threshtree.h"

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
#define SW_MODE 0
#define LIMITED_MODE 1
#define RGB_MODE 2
#define _BUFSIZE 128
static int term_color_mode = -1;
static int term_num_colors = 2;

/* Set term_num_colors and term_color_mode or
 * return -1.
 */
int set_term_color_mode(void){
  char *cmd = "tput colors";

  char *buf = malloc(_BUFSIZE);
  FILE *fp;

  // Set fallback values
  term_color_mode = SW_MODE;
  term_num_colors = 2;

  if ((fp = popen(cmd, "r")) == NULL) {
    fprintf(stderr, "Error opening pipe!\n");
    free(buf);
    return -1;
  }

  if (fgets(buf, _BUFSIZE, fp) != NULL) {

    // Remove disturbing newline
    char *pnewline =  strchr(buf, '\n');
    if( pnewline ){ *pnewline = '\0'; }

    char *endptr;
    int val = (int) strtol(buf, &endptr, 0);
    if( *endptr == '\0' ){ /* string was valid */
      term_num_colors = val;
      if (val >= 256){ /* Assume that it also supports rgb mode
                          ('\033[38;2;%d;%d;%dm')  */
        term_color_mode = RGB_MODE;
      }else if (val >= 8){
        term_color_mode = LIMITED_MODE;
      }else{
        term_color_mode = SW_MODE;
      }
    }
    //fprintf(stderr, "OUTPUT: %s Colors: %d, Mode: %d\n", buf, term_num_colors, term_color_mode);
  }

  if(pclose(fp))  {
    fprintf(stderr, "Command not found or exited with error status\n");
    free(buf);
    return -1;
  }

  free(buf);
  return 0;
}

int sprintf_color(char *buf, int set_background, int r, int g, int b, const char *pextra_str){
    int consumed_chars = 0;
    if( term_color_mode == RGB_MODE ){
        if( set_background > 1 ){
            // Also adapt foreground color on background for better readability
            if( r + g + b > 400 ){
                consumed_chars += sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm", 38, 0, 0, 0);
            }else{
                consumed_chars += sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm", 38, 240, 240, 240);
            }
        }
        consumed_chars +=  sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm%s",
                      set_background?48:38, r, g, b,
                      pextra_str?pextra_str:"");

    }else if( term_color_mode == LIMITED_MODE ){
        int col = (r^g^b)%8; // one of the 8 colors
        int bright = (r+g+b > 400); // bright flag
        if( set_background > 1 ){
            if( bright==0 ){
                consumed_chars +=  sprintf(buf, "\x1b[37m"); // white
            }else{
                consumed_chars +=  sprintf(buf, "\x1b[30;1m"); // black
            }
        }
        consumed_chars +=  sprintf(buf+consumed_chars, "\x1b[%d%sm%s",
                col + (set_background?40:30),
                bright?";1":"",
                pextra_str?pextra_str:"");

    }else if(pextra_str){
        consumed_chars +=  sprintf(buf, "%s",
                pextra_str?pextra_str:"");

    }


    return consumed_chars;
}

/*
 * Explands buffer if less than 'required' characters left.
 */
void expand_buf_if_required(
        size_t *pbuf_len,  // Length of *ppbuf
        char **ppbuf,
        size_t already_used,  // <= *pbuf_len
        size_t required,
        size_t extra_padding  // reallocation to size buf_len-buf_used + required  + extra_padding
        ){
    if( *pbuf_len > already_used + required ) return;
    if( *pbuf_len < already_used ){
        fprintf(stderr, "(%s) Hey buf_used=%lu > %lu=buf_len\n", __FILE__, already_used, *pbuf_len);
        return;
    }
    size_t new_len = already_used + required + extra_padding;
    //fprintf(stderr, "Increase from %lu to %lu \n", *pbuf_len, new_len);
    *ppbuf = realloc(*ppbuf, new_len * sizeof(typeof(*pbuf_len)));
    *pbuf_len = new_len;
}

char *sprint_coloured_threshtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
{

    if( term_color_mode < 0 && set_term_color_mode()){
        fprintf(stderr, "Can't detect color capabilities of terminal.");
    }

    // Region to print
    BlobtreeRect output_roi = {0, 0, pworkspace->w, pworkspace->h};
    if( pprint_roi != NULL ) output_roi = *pprint_roi;

    if(char_map == NULL) char_map = "0123456789ABCDEF";
    size_t char_map_len = strlen(char_map);

    if( display_filtered_areas ){
        // Setup filtered ids ( = pworkspace->blob_id_filtered)
        threshtree_filter_blobs(frameblobs, pworkspace);
    }
    const unsigned int *bif = pworkspace->blob_id_filtered;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    char *buf = malloc(buf_len);

    const int bg = CHANGE_BACKGROUND_PLUS;
    int consumed_chars = 0; // set by sprintf calls
    unsigned int id;

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){
        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y != H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {
            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x != W-1 ) continue;

            if( display_filtered_areas && bif ){
                id = threshtree_get_filtered_id_roi(frameblobs, output_roi, x, y, pworkspace);
            }else{
                /* The translation by 1 just adjust the results of both branches
                 * to avoid color flickering after the flag changes. */
                id = threshtree_get_id_roi(output_roi, x, y, pworkspace); //+ 1;
            }

            unsigned char d = *(data + y * pworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            ID_TO_RGB_B(id, col);

            expand_buf_if_required(&buf_len, &buf, used_buf_size, 50, 100);

            if(col[0] != prev_col[0] || col[1] != prev_col[1] || col[2] != prev_col[2]){
                if( id==background_id ){
                    // Do not define any color information for 'empty space'.

                    // Reset color information
                    used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);

                }else{
                    // Prepend next char with new color information
                    consumed_chars = sprintf_color(buf+used_buf_size,
                            bg, col[0], col[1], col[2],
                            NULL);
                    CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
                    used_buf_size += consumed_chars;
                }
            }else{
                // Re-use previous color
            }

            consumed_chars = sprintf(buf+used_buf_size,
                "%c",
                id==background_id?' ':char_map[id%char_map_len]
                );

            CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
            used_buf_size += consumed_chars;

        }

        if( bg ){
          // Reset color information
          expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
          used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);
          col[0] = -1; // Trigger color generation at next line
        }

        consumed_chars = sprintf(buf+used_buf_size, "\n");
        CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
        used_buf_size += consumed_chars;
    }

    // Reset color information
    expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
    used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);

    return buf;
}

int print_coloured_threshtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
{
  char *out = sprint_coloured_threshtree_ids(data, frameblobs, pprint_roi, pworkspace,
          display_filtered_areas, background_id, char_map);
  int ret = printf("%s\n", out);
  free(out);
  return ret;
}

char *sprint_coloured_threshtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
    if( term_color_mode < 0 && set_term_color_mode()){
        fprintf(stderr, "Can't detect color capabilities of terminal.");
    }

    // Region to print
    BlobtreeRect output_roi = {0, 0, pworkspace->w, pworkspace->h};
    if( pprint_roi != NULL ) output_roi = *pprint_roi;

    if( display_filtered_areas ){
        // Setup filtered ids ( = pworkspace->blob_id_filtered)
        threshtree_filter_blobs(frameblobs, pworkspace);
    }
    const unsigned int *bif = pworkspace->blob_id_filtered;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    //char *buf = malloc(buf_len);
    char *buf = calloc(1, buf_len);

    const int bg = CHANGE_FOREGROUND;
    int consumed_chars = 0; // set by sprintf calls
    unsigned int id;

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){
        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y != H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {
            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x != W-1 ) continue;

            if( display_filtered_areas && bif ){
                id = threshtree_get_filtered_id_roi(frameblobs, output_roi, x, y, pworkspace);
            }else{
                /* The translation by 1 just adjust the results of both branches
                 * to avoid color flickering after the flag changes. */
                id = threshtree_get_id_roi(output_roi, x, y, pworkspace); //+ 1;
            }

            unsigned char d = *(data + y * pworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            ID_TO_RGB_B(id, col);

            expand_buf_if_required(&buf_len, &buf, used_buf_size, 50, 100);

            if( col[0] != prev_col[0] || col[1] != prev_col[1] || col[2] != prev_col[2]){
                // Prepend next char with new color information
                consumed_chars = sprintf_color(buf+used_buf_size,
                        bg, col[0], col[1], col[2],
                        d!=0?"█":"░");
            }else{
                // Re-use previous color
                consumed_chars = sprintf(buf+used_buf_size,
                        "%s", d!=0?"█":"░");
            }
            CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
            used_buf_size += consumed_chars;

        }

        if( bg ){
          // Reset color information
          expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
          consumed_chars = sprintf(buf+used_buf_size, RESETCOLOR);
          CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
          used_buf_size += consumed_chars;
          col[0] = -1; // Trigger color generation at next line
        }

        consumed_chars = sprintf(buf+used_buf_size, "\n");
        CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
        used_buf_size += consumed_chars;
    }

    // Reset color information
    expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
    used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);

    return buf;
}

int print_coloured_threshtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
  char *out = sprint_coloured_threshtree_areas(data, frameblobs, pprint_roi, pworkspace, display_filtered_areas);
  int ret = printf("%s\n", out);
  free(out);
  return ret;
}
