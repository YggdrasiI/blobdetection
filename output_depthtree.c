#include <stdio.h>
#include <string.h>

#include "output_depthtree.h"

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
            (COL)[0] =  ((ID)*5*511+100)%180; \
            (COL)[1] =  ((ID)*7*753+10)%180;  \
            (COL)[2] =  ((ID)*29*29+1)%180; 

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

char *sprint_coloured_depthtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
{
    // Region to print
    BlobtreeRect output_roi = {0, 0, pworkspace->w, pworkspace->h};
    if( pprint_roi != NULL ) output_roi = *pprint_roi;

    if(char_map == NULL) char_map = "0123456789ABCDEF";
    size_t char_map_len = strlen(char_map);

    if( display_filtered_areas ){
        // Setup filtered ids ( = pworkspace->blob_id_filtered)
        depthtree_filter_blobs(frameblobs, pworkspace);
    }
    const unsigned int *bif = pworkspace->blob_id_filtered;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    char *buf = malloc(buf_len);

    const int bg = 1;  // Flag indicates if background or foreground color should be changed.
    int consumed_chars = 0; // set by sprintf calls
    unsigned int id;

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){
        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y!= H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {
            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x!= W-1 ) continue;

            if( display_filtered_areas && bif ){
                id = depthtree_get_filtered_id_roi(frameblobs, output_roi, x, y, pworkspace);
            }else{
                /* The translation by 1 just adjust the results of both branches
                 * to avoid color flickering after the flag changes. */
                id = depthtree_get_id_roi(output_roi, x, y, pworkspace); //+ 1;
            }

            unsigned char d = *(data + y * pworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            ID_TO_RGB_B(id, col);

            expand_buf_if_required(&buf_len, &buf, used_buf_size, 50, 100);

            if(col[0] != prev_col[0] || col[1] != prev_col[1] || col[2] != prev_col[2]){
                if( id==background_id ){
                  // Do not define any color information for 'empty space'.
                
                  // Reset color information
                  expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
                  used_buf_size += sprintf(buf+used_buf_size, "\e[m");

                }else{
                  // Prepend next char with new color information
                  consumed_chars = sprintf(buf+used_buf_size,
                      "\x1b[%d;2;%d;%d;%dm", 
                      bg?48:38, 
                      col[0], col[1], col[2]
                      );
                  CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
                  used_buf_size += consumed_chars;

                  if( bg ){
                    // Foreground color should depends on background for better readability
                    if( col[0] + col[1] + col[2] > 400 ){
                      consumed_chars = sprintf(buf+used_buf_size, "\x1b[%d;2;%d;%d;%dm", 38, 0, 0, 0);
                    }else{
                      consumed_chars = sprintf(buf+used_buf_size, "\x1b[%d;2;%d;%d;%dm", 38, 240, 240, 240);
                    }
                    CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
                    used_buf_size += consumed_chars;
                  }
                }
            }else{
                // Reuse previous color
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
          used_buf_size += sprintf(buf+used_buf_size, "\e[m");
          col[0] = -1; // Trigger color generation at next line
        }

        consumed_chars = sprintf(buf+used_buf_size, "\n");
        CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
        used_buf_size += consumed_chars;
    }

    // Reset color information
    expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
    used_buf_size += sprintf(buf+used_buf_size, "\e[m");

    return buf;
}

int print_coloured_depthtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
{
  char *out = sprint_coloured_depthtree_ids(data, frameblobs, pprint_roi, pworkspace,
          display_filtered_areas, background_id, char_map);
  int ret = printf("%s\n", out);
  free(out);
  return ret;
}

char *sprint_coloured_depthtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
    // Region to print
    BlobtreeRect output_roi = {0, 0, pworkspace->w, pworkspace->h};
    if( pprint_roi != NULL ) output_roi = *pprint_roi;

    if( display_filtered_areas ){
        // Setup filtered ids ( = pworkspace->blob_id_filtered)
        depthtree_filter_blobs(frameblobs, pworkspace);
    }
    const unsigned int *bif = pworkspace->blob_id_filtered;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    char *buf = malloc(buf_len);

    const int bg = 0;  // Flag indicates if background or foreground color should be changed.
    int consumed_chars = 0; // set by sprintf calls
    unsigned int id;

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){
        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y!= H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {
            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x!= W-1 ) continue;

            if( display_filtered_areas && bif ){
                id = depthtree_get_filtered_id_roi(frameblobs, output_roi, x, y, pworkspace);
            }else{
                /* The translation by 1 just adjust the results of both branches
                 * to avoid color flickering after the flag changes. */
                id = depthtree_get_id_roi(output_roi, x, y, pworkspace); //+ 1;
            }

            unsigned char d = *(data + y * pworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            ID_TO_RGB_B(id, col);

            expand_buf_if_required(&buf_len, &buf, used_buf_size, 50, 100);

            if( col[0] != prev_col[0] || col[1] != prev_col[1] || col[2] != prev_col[2]){
                // Prepend next char with new color information
                consumed_chars = sprintf(buf+used_buf_size,
                        "\x1b[%d;2;%d;%d;%dm%s", 
                        bg?48:38, 
                        col[0], col[1], col[2],
                        d!=0?"█":"░"
                        );
            }else{
                // Reuse previous color
                consumed_chars = sprintf(buf+used_buf_size,
                        "%s", d!=0?"█":"░");
            }
            CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
            used_buf_size += consumed_chars;

        }

        if( bg ){
          // Reset color information
          expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
          used_buf_size += sprintf(buf+used_buf_size, "\e[m");
          col[0] = -1; // Trigger color generation at next line
        }

        consumed_chars = sprintf(buf+used_buf_size, "\n");
        CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
        used_buf_size += consumed_chars;
    }

    // Reset color information
    expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
    used_buf_size += sprintf(buf+used_buf_size, "\e[m");

    return buf;
}

int print_coloured_depthtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas)
{
  char *out = sprint_coloured_depthtree_areas(data, frameblobs, pprint_roi, pworkspace, display_filtered_areas);
  int ret = printf("%s\n", out);
  free(out);
  return ret;
}
