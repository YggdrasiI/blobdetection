#ifndef DEPTHTREE_OUTPUT_H
#define DEPTHTREE_OUTPUT_H

#include "depthtree.h"

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

char *sprint_coloured_depthtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,  // could be NULL
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
    ;

int print_coloured_depthtree_ids(
        unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas,
        const int background_id,
        const char *char_map
        )
    ;

/* Colouring pixels by its connection component.
 * ░: Pixels < thresh value of previous depthtree evaluation
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
char *sprint_coloured_depthtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas)
    ;

int print_coloured_depthtree_areas(
        const unsigned char* data,
        Blobtree *frameblobs,
        const BlobtreeRect *pprint_roi,
        DepthtreeWorkspace *pworkspace,
        const int display_filtered_areas)
    ;


#endif

