#include <stdio.h>
#include <string.h>

#include "output.h"

#define CHECK_CONSUMED_CHARS(CONSUMED, BUF, USED) \
            if( CONSUMED < 0 ){ \
                fprintf(stderr, "(%s) Hey sprintf failed in line %i\n", __FILE__, __LINE__); \
                BUF[USED] = '\0'; \
                return BUF; \
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

char *sprint_coloured_threshblob_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace,
        int background_id,
        const char *char_map
        )
{
    BlobtreeRect output_roi = {0, 0, tworkspace->w, tworkspace->h}; // Region to print

    unsigned int seed, id, s2, s3, s4;
    unsigned int *ids, *riv, *bif, *cm;
    int display_filtered_areas = 0;
    int reset_ids = 0;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];
    ids = tworkspace->ids;
    cm = tworkspace->comp_same;
    riv = tworkspace->real_ids_inv;
    if(char_map == NULL) char_map = "0123456789ABCDEF";
    size_t char_map_len = sizeof(char_map);

    if( display_filtered_areas ){
        threshtree_filter_blob_ids(frameblobs, tworkspace);
    }
    bif = tworkspace->blob_id_filtered; //maps  'unfiltered id' on 'parent filtered id'

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    char *buf = malloc(buf_len);

    const int bg = 0;  // Flag indicates if background or foreground color should be changed.
    int consumed_chars = 0; // set by sprintf calls

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){

        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y!= H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {

            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x!= W-1 ) continue;

            id = ids[ (y+output_roi.y)*tworkspace->w + x + output_roi.x ];
            seed = id;

            if( display_filtered_areas && bif ){
                s2 = *(bif + seed);
            }else{
                /* This transformation just adjust the set of if- and else-branch.
                 * to avoid color flickering after the flag changes. */
                s2 = *(riv + *(cm + seed)) + 1 ;
            }

#if 0
            /* To reduce color flickering for thresh changes
             * eval variable which depends on some geometric information.
             * Use seed(=id) to get the associated node of the tree structure. 
             * Adding 1 compensate the dummy element at position 0.
             */
            Blob *pixblob = (Blob*) (frameblobs->tree->root + s2 )->data;
            s3 = pixblob->area + pixblob->roi.x + pixblob->roi.y;
#else
            s3 = s2;
#endif

            unsigned char d = *(data+
                    y * tworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            col[0] =  (s3*5*5+100)%256; 
            col[1] =  (s3*7*7+10)%256; 
            col[2] =  (s3*29*29+1)%256; 

            expand_buf_if_required(&buf_len, &buf, used_buf_size, 50, 100);

            if( col[0] != prev_col[0] || col[1] != prev_col[1] || col[2] != prev_col[2]){
                // Prepend next char with new color information
                consumed_chars = sprintf(buf+used_buf_size,
                        "\x1b[%d;2;%d;%d;%dm%c", 
                        bg?48:38, 
                        col[0], col[1], col[2],
                        s2==background_id?' ':char_map[s2%char_map_len]
                        );
            }else{
                // Reuse previous color
                consumed_chars = sprintf(buf+used_buf_size,
                        "%c",
                        s2==background_id?' ':char_map[s2%char_map_len]
                        );
            }
            CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
            used_buf_size += consumed_chars;

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

void print_coloured_threshblob_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace,
        int background_id,
        const char *char_map
        )
{
	char *out = sprint_coloured_threshblob_ids(data, frameblobs, tworkspace, background_id, char_map);
	printf("%s\n", out);
  free(out);
}

char *sprint_coloured_threshblob_areas(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace)
{
    BlobtreeRect output_roi = {0, 0, tworkspace->w, tworkspace->h}; // Region to print

    unsigned int seed, id, s2, s3, s4;
    unsigned int *ids, *riv, *bif, *cm;
    int display_filtered_areas = 0;
    int reset_ids = 0;
    unsigned int col[3] = {0, 0, 0};
    unsigned int prev_col[3];
    ids = tworkspace->ids;
    cm = tworkspace->comp_same;
    riv = tworkspace->real_ids_inv;

    if( display_filtered_areas ){
        threshtree_filter_blob_ids(frameblobs, tworkspace);
    }
    bif = tworkspace->blob_id_filtered; //maps  'unfiltered id' on 'parent filtered id'

    //printf("Roi: (%i %i %i %i)\n", output_roi.x, output_roi.y, output_roi.width, output_roi.height);
    size_t buf_len = output_roi.width * output_roi.height * 4;
    size_t used_buf_size = 0;
    char *buf = malloc(buf_len);

    const int bg = 0;  // Flag indicates if background or foreground color should be changed.
    int consumed_chars = 0; // set by sprintf calls

    for( unsigned int y=0, H=output_roi.height; y<H; ++y){

        //restrict on grid pixels.
        if( y % frameblobs->grid.height != 0 && y!= H-1 ) continue;

        for( unsigned int x=0, W=output_roi.width; x<W; ++x) {

            //restrict on grid pixels.
            if( x % frameblobs->grid.width != 0 && x!= W-1 ) continue;

            id = ids[ (y+output_roi.y)*tworkspace->w + x + output_roi.x ];

            seed = id;

            if( display_filtered_areas && bif ){
                s2 = *(bif + seed);
            }else{
                /* This transformation just adjust the set of if- and else-branch.
                 * to avoid color flickering after the flag changes. */
                s2 = *(riv + *(cm + seed)) + 1 ;
            }

#if 0
            /* To reduce color flickering for thresh changes
             * eval variable which depends on some geometric information.
             * Use seed(=id) to get the associated node of the tree structure. 
             * Adding 1 compensate the dummy element at position 0.
             */
            Blob *pixblob = (Blob*) (frameblobs->tree->root + s2 )->data;
            s3 = pixblob->area + pixblob->roi.x + pixblob->roi.y;
#else
            s3 = s2;
#endif

            unsigned char d = *(data+
                    y * tworkspace->w + x);
            prev_col[0] = col[0]; prev_col[1] = col[1]; prev_col[2] = col[2];
            col[0] =  (s3*5*5+100)%256; 
            col[1] =  (s3*7*7+10)%256; 
            col[2] =  (s3*29*29+1)%256; 

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

        consumed_chars = sprintf(buf+used_buf_size, "\n");
        CHECK_CONSUMED_CHARS(consumed_chars, buf, used_buf_size);
        used_buf_size += consumed_chars;
    }

    // Reset color information
    expand_buf_if_required(&buf_len, &buf, used_buf_size, 3, 0);
    used_buf_size += sprintf(buf+used_buf_size, "\e[m");

    return buf;
}

void print_coloured_threshblob_areas(
        unsigned char* data,
        Blobtree *frameblobs, 
        ThreshtreeWorkspace *tworkspace)
{
	char *out = sprint_coloured_threshblob_areas(data, frameblobs, tworkspace);
	printf("%s\n", out);
  free(out);
}
