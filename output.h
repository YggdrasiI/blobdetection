#ifndef OUTPUT_H
#define OUTPUT_H

#include "threshtree.h"

/* Print out char_map[id%len(char_map)] of connection components.
 * The background_id value will printed as ' '.
 * char_map = NULL leads to usage of default map "0123456789ABCDEF".
 *
 * Example output for background_id = 1, char_map = "0123456789":
 *                            
 *        22       2 3333333  
 *  44444 22       2 3555553  
 *  44444 22  666  2 3577753  
 *  44444 22  666  2 3577753  
 *  44444 22  666  2 3577753  
 *  44444 22       2 3555553  
 *        22       2 3333333  
 * 22222222222222222          
 */

char *sprint_coloured_threshblob_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace,
        int background_id,
        const char *char_map
        )
    ;

void print_coloured_threshblob_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace,
        int background_id,
        const char *char_map
        )
;

/* Colouring pixels by its connection component.
 * ░: Pixels < thresh value of previous threshblob evaluation
 * █: Pixels >= thresh value
 *
 * Example output (without colouring information):
 * ░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 * ░░░░░░░░██░░░░░░░█░███████░░
 * ░░█████░██░░░░░░░█░█░░░░░█░░
 * ░░█████░██░░███░░█░█░███░█░░
 * ░░█████░██░░███░░█░█░███░█░░
 * ░░█████░██░░███░░█░█░███░█░░
 * ░░█████░██░░░░░░░█░█░░░░░█░░
 * ░░░░░░░░██░░░░░░░█░███████░░
 * ░█████████████████░░░░░░░░░░
 */
char *sprint_coloured_threshblob_areas(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace)
    ;

void print_coloured_threshblob_areas(
        unsigned char* data,
        Blobtree *frameblobs,
        ThreshtreeWorkspace *tworkspace)
    ;

#endif

