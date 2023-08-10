#ifndef THRESHTREE_OUTPUT_H
#define THRESHTREE_OUTPUT_H

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

char *sprint_threshtree_ids(
        uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,  // could be NULL
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas,
        const int32_t background_id,
        const char *char_map
        )
    ;

int32_t print_threshtree_ids(
        uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas,
        const int32_t background_id,
        const char *char_map
        )
    ;

char *sprint_coloured_threshtree_ids(
        uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,  // could be NULL
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas,
        const int32_t background_id,
        const char *char_map
        )
    ;

int32_t print_coloured_threshtree_ids(
        uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas,
        const int32_t background_id,
        const char *char_map
        )
    ;

/* Colouring pixels by its connection component.
 * ░: Pixels < thresh value of previous threshtree evaluation
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
char *sprint_threshtree_areas(
        const uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas)
    ;

int32_t print_threshtree_areas(
        const uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas)
    ;

char *sprint_coloured_threshtree_areas(
        const uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas)
    ;

int32_t print_coloured_threshtree_areas(
        const uint8_t* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        ThreshtreeWorkspace *pworkspace,
        const int32_t display_filtered_areas)
    ;

#endif

