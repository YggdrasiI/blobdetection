#ifndef THRESHTREE_H
#define THRESHTREE_H

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
  uint32_t *real_ids;
  uint32_t *real_ids_inv;

#ifdef BLOB_BARYCENTER
  BLOB_BARYCENTER_TYPE *pixel_sum_X; //summation of all x coordinates for an id.
  BLOB_BARYCENTER_TYPE *pixel_sum_Y; //summation of all x coordinates for an id.
#endif
#ifdef BLOB_SUBGRID_CHECK
  uint8_t *triangle;
  size_t triangle_len;
#endif

  //extra data
  uint32_t *blob_id_filtered; //like comp_same, but respect blob tree filter.

} ThreshtreeWorkspace;


int32_t threshtree_create_workspace(
    const uint32_t w, const uint32_t h,
    ThreshtreeWorkspace **pworkspace
    );
int32_t threshtree_realloc_workspace(
    const uint32_t max_comp,
    ThreshtreeWorkspace **pworkspace
    );
void threshtree_destroy_workspace(
    ThreshtreeWorkspace **pworkspace
    );

/* See depthtree equivalent for explanation */
void threshtree_filter_blobs(
    Blobtree* blob,
    ThreshtreeWorkspace *pworkspace
    );

/* Find Blobs. 
 *
 * Assumption: All border elements x fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *   0---0 
 * h | A |  }data
 *   0---0 
 *     w
 *
 * Parameter:
 *   In: 
 *     -uint8_t *data: Image data
 *     -uint32_t w, uint32_t h: Dimension of image
 *     -uint8_t thresh: Distinct values in x>thresh and x<=thresh.
 *   Out:
 *     -Blob **tree_data: 
 *   Return: 
 *     -node*: Pointer to the array of nodes. 
 *      It's the pointer to the root node, too. If all assumtions fulfilled, node has
 *      exact one child which represent the full area.
 *
 */
Tree* find_connection_components(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const uint8_t thresh,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );

 
/* Find Blobs. With region of interrest (roi).
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 *   In: 
 *     -uint8_t *data: Image data
 *     -uint32_t w, uint32_t h: Dimension of image
 *     -BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 *     -uint8_t thresh: Distinct values in x>thresh and x<=thresh.
 *   Out:
 *     -Blob **tree_data: 
 *   Return: 
 *     -node*: Pointer to the array of nodes. 
 *      It's the pointer to the root node, too. If all assumtions fulfilled, node has
 *      exact one child which represent the full area.
 *
 */
Tree* find_connection_components_roi(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );


/* Find Blobs. With flexible stepwidth and
 *  region of interrest (roi).
 *  Only an subset of all pixels (grid structure) will checked.
 *  Stepwidth and stepheight control the mesh size.
 *  Attention: The founded bounding box values are not exact.
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 *   In: 
 *     -uint8_t *data: Image data
 *     -uint32_t w, uint32_t h: Dimension of image
 *     -BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 *     -char thresh: Distinct values in x>thresh and x<=thresh.
 *     -unsined int32_t stepwidth: 
 *   Out:
 *     -Blob **tree_data: 
 *   Return: 
 *     -node*: Pointer to the array of nodes. 
 *      It's the pointer to the root node, too. If all assumtions fulfilled, node has
 *      exact one child which represent the full area.
 *
 */
Tree* find_connection_components_coarse(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    const uint32_t stepheight,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );

#ifdef BLOB_SUBGRID_CHECK 
/* Find Blobs. With flexible stepwidth and
 *  region of interrest (roi).
 *  An subset of all pixels (grid structure) will checked.
 *  Stepwidth controls the mesh size. If an blob was detected
 *  the neighbourhood will regard to get the exact blob dimensions.
 *  Nevertheless, there is no quarantee to recognise fine structures.
 *
 * Assumption: All border elements x of roi fulfill 
 * 'x > thresh' or 'x <= thresh'
 *
 *  ____________________
 * |                    |
 * |            0---0   |
 * | roi.height | A |   |  } data
 * |            0---0   |
 * |         roi.width  |
 * |____________________|
 *
 * Parameter:
 *   In: 
 *     -uint8_t *data: Image data
 *     -uint32_t w, uint32_t h: Dimension of image
 *     -BlobtreeRect roi: Simple struct to describe roi, {x,y,width,height}.
 *     -char thresh: Distinct values in x>thresh and x<=thresh.
 *     -uint32_t stepwidth: 
 *   Out:
 *     -Blob **tree_data: 
 *   Return: 
 *     -node*: Pointer to the array of nodes. 
 *      It's the pointer to the root node, too. If all assumtions fulfilled, node has
 *      exact one child which represent the full area.
 *
 */

Tree* find_connection_components_subcheck(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace );
#endif

/* Main function to eval blobs */
void threshtree_find_blobs( Blobtree *blob, 
    const uint8_t *data, 
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    ThreshtreeWorkspace *workspace );

/* Postprocessing: Get blob id for coordinate. */
uint32_t threshtree_get_id(
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    );

/* Postprocessing: Get blob id for coordinate. Roi version */
uint32_t threshtree_get_id_roi(
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    );

/* Postprocessing: Get filtered blob id for coordinate.
 * This restricts the output on ids/blobs which fulfil the filter criteria,
 * set by blobtree_set_filter(...).
 * Call threshtree_filter_blobs(...) before you use this function.
 * */
uint32_t threshtree_get_filtered_id(
    const Blobtree *blobs,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    );

/* Postprocessing: Get filtered blob id for coordinate. Roi version */
uint32_t threshtree_get_filtered_id_roi(
    const Blobtree *blobs,
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    );
#ifdef __cplusplus
}
#endif



#endif
