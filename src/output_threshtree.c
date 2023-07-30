#include <stdio.h>
#include <string.h>

#include "output.h"
#include "output_threshtree.h"

char *sprint_threshtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
    int _term_color_mode = term_color_mode;
    term_color_mode = COLOR_SW_MODE;
    char *ret = sprint_coloured_threshtree_areas(
            data, frameblobs, pprint_roi, pworkspace,
            display_filtered_areas);

    term_color_mode = _term_color_mode;
    return ret;
}

int print_threshtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
    int _term_color_mode = term_color_mode;
    term_color_mode = COLOR_SW_MODE;
    int ret = print_coloured_threshtree_areas(
            data, frameblobs, pprint_roi, pworkspace,
            display_filtered_areas);

    term_color_mode = _term_color_mode;
    return ret;
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
                // Translation of dummy node.
                if( id > 0 ) { --id; }
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
                    //CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
                    used_buf_size += consumed_chars;
                }
            }else{
                // Re-use previous color
            }

            consumed_chars = sprintf(buf+used_buf_size,
                "%c",
                id==background_id?' ':char_map[id%char_map_len]
                );

            //CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
            used_buf_size += consumed_chars;

        }

        if( bg ){
          // Reset color information
          expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
          used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);
          col[0] = -1; // Trigger color generation at next line
        }

        used_buf_size += sprintf(buf+used_buf_size, "\n");
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
                id = threshtree_get_filtered_id_roi(frameblobs, output_roi, x, y, pworkspace) - 2;
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
          used_buf_size += sprintf(buf+used_buf_size, RESETCOLOR);
          col[0] = -1; // Trigger color generation at next line
        }

        used_buf_size += sprintf(buf+used_buf_size, "\n");
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
