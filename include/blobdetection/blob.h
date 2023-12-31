#ifndef BLOBTREE_H
#define BLOBTREE_H

/* Blobtree struct definitions. See tree.h for struct 'Blob'.
 * The blobtree struct bundles settings
 * for the main algorithms and
 * contains the result.
 * ( Some internal variables are stored in the workspace struct. Interesting could be:
 *  • *ids : Every pixel of Image (or ROI) gets an id. Attention blobs can contain multiple ids.
 *  • *comp_same : Projection on ids maps all ids of one area on unique id.
 *  • *prop_parent : Store ids of parent areas. Attention, there is no guarantee that
 *        prop_parent(id) = comp_same( prop_parent( comp_same(id) ) )
 *    holds for every pixel.
 * )
 *
 * Workflow:
 * 1. Create workspace (one time, optional(?) )
 * 2. Create blobtree struct (blobtree_create() ) 
 * For every image:
 *   3. Call blob search algorithm ( *_find_blobs(...) )
 *   4. Setup blob filter  ( blobtree_set_filter(...) )
 *   5. Loop through blobs ( blobtree_first(), blobtree_next() )
 * Cleanup:
 * 6. Destroy blobtree
 * 7. Destroy workspace
 *
 * */

#include "tree.h"

typedef struct {
  uint32_t width;
  uint32_t height;
} Grid;

typedef enum {
  F_TREE_DEPTH_MIN=1,
  F_TREE_DEPTH_MAX=2,
  F_AREA_MIN=3,
  F_AREA_MAX=4,
  F_ONLY_LEAFS=5,
  F_AREA_DEPTH_MIN=6,
  F_AREA_DEPTH_MAX=7,
  F_CLEAR=~0
} FILTER;

/* Filter handler to mark nodes for filtering out.
 * Interpretation of return value:
 * 0 - The node will not filtered out.
 * 1 -  The node will filtered out. Algorithm continues with child, if existing.
 * 2 -  The node will filtered out and all children, too.
 *       All children will be skiped during tree cycle.
 * >2 -  The node will filtered out and all children and all siblings
 *       will be skiped.
 * */
typedef int32_t (FilterNodeHandler)(Node *node);

typedef struct {
  uint32_t tree_depth_min;
  uint32_t tree_depth_max;
  uint32_t min_area;
  uint32_t max_area;
  uint8_t only_leafs; /*0 or 1*/
  uint8_t area_depth_min;
  uint8_t area_depth_max;
  FilterNodeHandler* extra_filter;
} Filter;

typedef struct {
  Node *node;
  int32_t depth;
} Iterator;

typedef struct {
  Tree *tree; 
  Blob *tree_data;
  Filter filter;
  Grid grid; // width between compared pixels (Could leave small blobs undetected.)
  Iterator it; //node itarator for intern usage
  Iterator it_next; //node itarator for intern usage
} Blobtree;


/* Create blob struct. Use threshtree_destroy to free mem. */
void blobtree_create(Blobtree **blob);
void blobtree_destroy(Blobtree **blob );

/* Set one of the default filter values */
void blobtree_set_filter( Blobtree *blob, const FILTER f, const uint32_t val);
/* Add own node filter function */
void blobtree_set_extra_filter(Blobtree *blob, FilterNodeHandler* extra_filter);

/* Set difference between compared pixels. Could ignore small blobs. */
void blobtree_set_grid(Blobtree *blob, const uint32_t gridwidth, const uint32_t gridheight );

/* Returns first node which is matching
 * the filter criteria or NULL. */
Node *blobtree_first( Blobtree *blob);

/* Returns next element under the given filters. 
 * or NULL */
Node *blobtree_next( Blobtree *blob);

/* Returns node with given id
 * or NULL */
Node *blobtree_find_id(Blobtree *blob, uint32_t id, int32_t respect_filter);


/* Textual output of tree with blob-Data. Shift defines number of spaces at line beginning. */
void blobtree_print(
        Blobtree *blob,
        int32_t shift);

/* Print only nodes with area>=minA */
void blobtreetree_print_filtered(
        Blobtree *blob,
        int32_t shift,
        uint32_t minA);

/* For trees with data type Blob */
#ifdef BLOB_COUNT_PIXEL
uint32_t blobtree_sum_areas(
        Node * const root,
        const uint32_t * const comp_size);
#ifdef BLOB_DIMENSION
void blobtree_approx_areas(
        const Tree * const tree,
        Node * const startnode,
        const uint32_t * const comp_size,
        const uint32_t stepwidth,
        const uint32_t stepheight);
#endif
#endif

#ifdef BLOB_BARYCENTER
void blobtree_eval_barycenters(
    Node * const root,
    const uint32_t * const comp_size,
    BLOB_BARYCENTER_TYPE * const pixel_sum_X,
    BLOB_BARYCENTER_TYPE * const pixel_sum_Y);
#endif

#ifdef BLOB_DIMENSION
void blobtree_set_area_prop(
        Node *root);
#endif

#endif
