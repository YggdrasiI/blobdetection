#ifndef DEPTHTREE_H
#define DEPTHTREE_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __has_include("settings.h")
#include "settings.h"
#endif
#include "settings.default.h"

#include "tree.h"
#include "blob.h"

/* Workspace struct for array storage */
typedef struct {
  uint32_t w; // (Maximal) width of input data.
  uint32_t h; // (Maximal) height of input data.
  uint32_t max_comp; // = w+h;//maximal number of components. If the value is reached some arrays will reallocate.
  uint32_t used_comp; // number of used ids ; will be set after the main algorithm finishes ; <=max_comp
  uint32_t *ids;
  uint8_t *depths; // use monotone function to map image data into different depths
  uint32_t *id_depth; //store depth (group) of id anchor.
  uint32_t *comp_same; //map ids to unique ids g:{0,...,}->{0,....}
  uint32_t *prob_parent; //store ⊂-Relation.
#ifdef BLOB_COUNT_PIXEL
  uint32_t *comp_size;
#endif
#ifdef BLOB_DIMENSION
  uint32_t *top_index; //save row number of most top element of area.
  uint32_t *left_index; //save column number of most left element of area.
  uint32_t *right_index; //save column number of most right element.
  uint32_t *bottom_index; //save row number of most bottom element.
#endif
#ifdef BLOB_BARYCENTER
  BLOB_BARYCENTER_TYPE *pixel_sum_X; //summation of all x coordinates for an id.
  BLOB_BARYCENTER_TYPE *pixel_sum_Y; //summation of all x coordinates for an id.
#endif
  /* Geometric interpretation of the positions a,b,c,d
   * in relation to x(=current) position:
   * abc
   * dx
   * */
  uint32_t *a_ids; //save chain of ids with depth(a_ids[0])>depth(a_ids[1])>...
  uint8_t *a_dep; //save depth(a_ids[...]) to avoid lookup with id_depth(a_ids[k]).
  uint32_t *b_ids, *c_ids, *d_ids; //same for b,c,d 
  uint8_t *b_dep, *c_dep, *d_dep;  

  uint32_t *real_ids;
  uint32_t *real_ids_inv;

  //extra data
  uint32_t *blob_id_filtered; //like comp_same, but respect blob tree filter.

} DepthtreeWorkspace;


int32_t depthtree_create_workspace(
    const uint32_t w, const uint32_t h,
    DepthtreeWorkspace **pworkspace
    );
int32_t depthtree_realloc_workspace(
    const uint32_t max_comp,
    DepthtreeWorkspace **pworkspace
    );
void depthtree_destroy_workspace(
    DepthtreeWorkspace **pworkspace
    );

/* The tree contains all blobs 
 * and the id array does not respect
 * the setted filter values.
 * This function uses the filter to
 * generate an updated version of the
 * id mapping. 
 * After calling this method the workspace
 * contains the map blob_id_filtered
 * with the property 
 *     blob_id_filtered(id(pixel)) ∈ {Ids of matching Nodes}
 *
 * This is useful to draw the map of filtered blobs.
 * */
void depthtree_filter_blobs(
    Blobtree* blob,
    DepthtreeWorkspace *pworkspace
    );

Tree* find_depthtree(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t *depth_map,
    const uint32_t stepwidth,
    DepthtreeWorkspace *workspace,
    Blob** tree_data );

// Note: keyword static required for -std=gnu11 
static uint32_t inline getRealId( uint32_t * const comp_same, uint32_t const id ){
  uint32_t rid1, rid2;
  rid1 = *(comp_same + id);
  if( (rid2 = *(comp_same + rid1)) != rid1 ){
    VPRINTF("Map %i from %i ", id, rid1);
    rid1 = rid2;
    while( (rid2 = *(comp_same + rid1)) != rid1 ){
      rid1 = rid2;
    }
    *(comp_same + id) = rid2;
    /* Attention, only the mapping of id are changed to rid2.
     * Other elements of the while loop not. Thus,
     * the calling of comp_same for all Ids does in general not
     * transform comp_same to a projection.
     *
     */

    /*uint32_t rid0 = id;
    rid1 = *(comp_same + rid0);
    while( rid2 != rid1 ){
     *(comp_same + rid0) = rid2;
      rid0 = rid1;
      rid1 = *(comp_same + rid1);
    }*/
    /* This approach would be more correct, but comp_same
     * is still no projection because there is no guarantee
     * that getRealId will be called for all ids!
     * => Do not lay on the projection property here.
     * comp_same will be modified during postprocessing.
     */

    VPRINTF("to %i\n", rid2);
  }
  return rid1;
}

static uint32_t inline getRealParent( uint32_t * const prob_parent, uint32_t * const comp_same, uint32_t const id ){
      return getRealId( comp_same, *(prob_parent + id) );
}


//------------------------------------

void depthtree_find_blobs(
    Blobtree *blob,
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t *depth_map,
    DepthtreeWorkspace *workspace
    );


#ifdef EXTEND_BOUNDING_BOXES
void extend_bounding_boxes( Tree * const tree);
#endif

/* Postprocessing: Get blob id for coordinate. */
uint32_t depthtree_get_id(
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    );

/* Postprocessing: Get blob id for coordinate. Roi version */
uint32_t depthtree_get_id_roi(
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    );

/* Postprocessing: Get filtered blob id for coordinate.
 * This restricts the output on ids/blobs which fulfil the filter criteria,
 * set by blobtree_set_filter(...).
 * Call depthtree_filter_blobs(...) before you use this function.
 * */
uint32_t depthtree_get_filtered_id(
    const Blobtree *blobs,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    );

/* Postprocessing: Get filtered blob id for coordinate. Roi version */
uint32_t depthtree_get_filtered_id_roi(
    const Blobtree *blobs,
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    );

#ifdef __cplusplus
}
#endif


#endif
