//#define INLINE extern inline
#include "tree.h"
#include "tree_intern.h"

#include "blob.h"

const  Filter Default_Filter = {0,INT_MAX, 0,INT_MAX, 0, 0,255, NULL };

//+++++++++++++++++++++++++++++
// Blobtree functions
//+++++++++++++++++++++++++++++

void blobtree_create(Blobtree **pblob){
  if( *pblob != NULL ){
    blobtree_destroy(pblob);
  }
  Blobtree* blob = (Blobtree*) malloc(sizeof(Blobtree) );
  blob->tree = NULL;
  blob->tree_data = NULL;
  blob->filter = Default_Filter;
  Grid grid = {1,1};
  blob->grid = grid;

  *pblob = blob;
}

void blobtree_destroy(Blobtree **pblob){
  if( *pblob == NULL ) return;
  Blobtree *blob = *pblob;
  if( blob->tree != NULL ){
    tree_destroy(&blob->tree);
  }
  if( blob->tree_data != NULL) {
    free(blob->tree_data);
    blob->tree_data = NULL;
  }
  free(blob);
  *pblob = NULL;
}

void blobtree_set_filter(Blobtree *blob, const FILTER f, const uint32_t val){
  switch(f){
    case F_CLEAR: blob->filter = Default_Filter;
                  break;
    case F_TREE_DEPTH_MIN: blob->filter.tree_depth_min=val;
                           break;
    case F_TREE_DEPTH_MAX: blob->filter.tree_depth_max=val;
                           break;
    case F_AREA_MIN: blob->filter.min_area=val;
                     break;
    case F_AREA_MAX: blob->filter.max_area=val;
                     break;
    case F_ONLY_LEAFS: blob->filter.only_leafs=(val>0?1:0);
                       break;
    case F_AREA_DEPTH_MIN: { blob->filter.area_depth_min=val;
#if VERBOSE > 0
                             if(/*val<0||*/val>255) printf("(blobtree_set_filter) range error: val=%u leave range [0,255]", val);
#endif
                           }
                           break; 
    case F_AREA_DEPTH_MAX: { blob->filter.area_depth_max=val;
#if VERBOSE > 0
                             if(/*val<0||*/val>255) printf("(blobtree_set_filter) range error: val=%u leave range [0,255]", val);
#endif
                           }
                           break; 
  }
}

void blobtree_set_extra_filter(Blobtree *blob, FilterNodeHandler* extra_filter){
  blob->filter.extra_filter = extra_filter;
}

void blobtree_set_grid(Blobtree *blob, const uint32_t gridwidth, const uint32_t gridheight ){
  blob->grid.width = gridwidth;
  blob->grid.height =  gridheight;
}


void blobtree_next2(Blobtree *blob, Iterator* pit);

Node *blobtree_first( Blobtree *blob){
  blob->it.node = blob->tree->root;
  blob->it.depth = -1;
  blob->it_next.node = blob->tree->root;
  blob->it_next.depth = -1;
  if( blob->filter.only_leafs ){
    blobtree_next2(blob,&blob->it_next);
  }
  return blobtree_next(blob);
}

Node *blobtree_next( Blobtree *blob){

  if( blob->filter.only_leafs ){
    while( blob->it_next.node != NULL ){
      blob->it.node = blob->it_next.node;
      blob->it.depth = blob->it_next.depth;
      blobtree_next2(blob,&blob->it_next);
      /*check if it_next.node is successor of it.node.
       * This condition is wrong for leafs */
      if(!successor(blob->it.node, blob->it_next.node) ){
        return blob->it.node;
      }
    }
    return NULL;
  }else{
    blobtree_next2(blob,&blob->it);
    return blob->it.node;
  }
}

/* This method throws an NULL pointer exception
 * if it's has already return NULL and called again. Omit that. */
void blobtree_next2(Blobtree *blob, Iterator* pit){

  //go to next element
  Node *it = pit->node;
  int32_t it_depth = pit->depth;

  if( it->child != NULL){
    it = it->child;
    it_depth++;
  }else if( it->sibling != NULL ){
    it = it->sibling;
  }else{
    while( 1 ){
      it = it->parent;
      it_depth--;
      if(it->sibling != NULL){
        it = it->sibling;
        break;
      }
      if( it_depth < 0 ){
        pit->node = NULL;
        return;
      }
    }
  }

  const Node * const root = blob->tree->root;
  //check criteria/filters.
  do{
    if( it_depth < blob->filter.tree_depth_min 
#ifdef SAVE_DEPTH_MAP_VALUE
        || ((Blob*)it->data)->depth_level < blob->filter.area_depth_min
#endif
        || ((Blob*)it->data)->area > blob->filter.max_area ){
      if( it->child != NULL){
        it = it->child;
        it_depth++;
        continue;
      }
      if( it->sibling != NULL ){
        it = it->sibling;
        continue;
      }
      while( 1 ){
        it = it->parent;
        it_depth--;
        if(it->sibling != NULL){
          it = it->sibling;
          break;
        }
        if( it_depth < 0 ){
          pit->node = NULL;
          return;
        }
      }
      continue;
    }
    if( it_depth > blob->filter.tree_depth_max
#ifdef SAVE_DEPTH_MAP_VALUE
        || ((Blob*)it->data)->depth_level > blob->filter.area_depth_max
#endif
      ){
      while( 1 ){
        it = it->parent;
        it_depth--;
        if(it->sibling != NULL){
          it = it->sibling;
          break;
        }
        if( it_depth < 0 ){
          pit->node = NULL;
          return;
        }
      }
      continue;
    }
    if( ((Blob*)it->data)->area < blob->filter.min_area ){
      if( it->sibling != NULL ){
        it = it->sibling;
        continue;
      }
      while( 1 ){
        it = it->parent;
        it_depth--;
        if(it->sibling != NULL){
          it = it->sibling;
          break;
        }
        if( it_depth < 0 ){
          pit->node = NULL;
          return;
        }
      }
      continue;
    }

    //extra filter handling
    if( blob->filter.extra_filter != NULL ){
      uint32_t ef = (*blob->filter.extra_filter)(it);
      if( ef ){
        if( ef<2 && it->child != NULL){
          it = it->child;
          it_depth++;
          continue;
        }
        if( ef<3 && it->sibling != NULL ){
          it = it->sibling;
          continue;
        }
        while( 1 ){
          it = it->parent;
          it_depth--;
          if(it->sibling != NULL){
            it = it->sibling;
            break;
          }
          if( it_depth < 0 ){
            pit->node = NULL;
            return;
          }
        }
      }
    }

    // All filters ok. Return node
    pit->node = it;
    pit->depth = it_depth;
    return;

  }while( it != root );

  //should never reached
  pit->node = NULL;
  pit->depth = -1;
  return;
}

/* Returns node with given id
 * or NULL */
Node *blobtree_find_id(Blobtree *blob, uint32_t id, int32_t respect_filter)
{
    if (respect_filter){
        Node *cur = blobtree_first(blob);
        while( cur != NULL ){
            Blob *data = (Blob*)cur->data;
            //BlobtreeRect *rect = &data->roi;
            if (data->id == id) return cur;

            cur = blobtree_next(blob);
        }

        return NULL;
    }

  Node *root = blob->tree->root;
  Node *cur = root;
  do{
    Blob *data = (Blob*)cur->data;
    if (data->id == id) return cur;

    /* To to next node. update parent node if possible */
    if( cur->child != NULL ){
      cur = cur->child;
      continue;
    }else if( cur->sibling != NULL ){
      cur = cur->sibling;
      continue;
    }else{
      while( cur->parent != NULL ){
        cur = cur->parent;
        if( cur->sibling != NULL ){
          cur = cur->sibling;
          break;
        }
      }
    }
  }while( cur != root );

  return NULL;
}

void _blobtree_print_filtered(
    Node *root,
    int32_t shift,
    uint32_t minA)
{
  int32_t i;
  int32_t shift2=0;
  Blob* data = (Blob*)root->data;
  //printf("• ");
  //printf("%u (%u) ", root->data.id, root->data.area);
  //printf("%2i (w%u,h%u,a%2i) ", root->data.id, root->width, root->height, root->data.area);
  //shift2+=9+4;
#ifdef SAVE_DEPTH_MAP_VALUE
#ifdef BLOB_DIMENSION
  printf("%2i (lvl:%3i, a:%4i) ", data->id, data->depth_level, data->area);
  //printf("%2i (wxh:%3i, a:%4i) ", data->id, data->roi.width*data->roi.height, data->area);
  shift2+=22;
#else
  printf("%2i (lvl:%3i) ", data->id, data->depth_level);
  shift2+=14;
#endif
#else
  printf("%2i (area:%4i) ", data->id, data->area);
  shift2+=9+6;
#endif

  if( data->area < minA){
    printf("\n");
    return;
  }
  if( root->child != NULL){
    printf("→");
    _blobtree_print_filtered(root->child, shift+shift2, minA);
  }else{
    printf("\n");
  }

  if( root->sibling != NULL){
  //  printf("\n");
    for(i=0; i<shift-1; i++) printf(" ");
    printf("↘");
    _blobtree_print_filtered(root->sibling, shift, minA);
  }
}

void blobtree_print(
    Blobtree *blob,
    int32_t shift)
{
  _blobtree_print_filtered(blob->tree->root, shift, 0);
}

void blobtree_print_filtered(
    Blobtree *blob,
    int32_t shift,
    uint32_t minA)
{
  _blobtree_print_filtered(blob->tree->root, shift, minA);
}



#ifdef BLOB_COUNT_PIXEL
uint32_t blobtree_sum_areas(
    Node * const root,
    const uint32_t * const comp_size)
{

#if 1
  Node *node = root;
  Blob* data = (Blob*)node->data;
  if( root->child == NULL){
    data->area = *(comp_size + data->id );
    return data->area;
  }

  do{
    data->area = *(comp_size + data->id );

    /* Go to next node. update parent node on uprising flank */
    if( node->child != NULL ){
      node = node->child;
      data = (Blob*)node->data;
      continue;
    }

    ((Blob*)node->parent->data)->area += data->area;

    if( node->sibling != NULL ){
      node = node->sibling;
      data = (Blob*)node->data;
      continue;
    }

    while( node != root ){
      node = node->parent;
      data = (Blob*)node->data;
      if(node != root ){
        ((Blob*)node->parent->data)->area += data->area;
      }
      if( node->sibling != NULL ){
        node = node->sibling;
        data = (Blob*)node->data;
        break;
      }
    }

  }while( node != root );
  return data->area;
#else
  /* Recursive formulation */
    Blob* data = (Blob*)root->data;
  int32_t *val=&data->area;
  *val = *(comp_size + data->id );
  if( root->child != NULL) *val += blobtree_sum_areas(root->child, comp_size);
  if( root->sibling != NULL) return *val+blobtree_sum_areas(root->sibling, comp_size);
  else return *val;
#endif
}

#ifdef BLOB_DIMENSION

/*
 * Let i be the id of current blob. We search
 * • A = Number of pixels of current blob.
 *
 * The Problem: We do not has information about
 * every pixel because only the pixels of the coarse
 * grid was processed.
 *
 * Solution: Use the counting on the coarse values
 * and the bounding boxes to estimate the correct
 * values. Use the fact, that bounding boxes of children
 * propagate the correct values on subsets.
 *
 * Definition of algorithm:
 *
 * C - (Coarse) Bounding box of current blob
 * F - (Fine) Bounding box(es) of child(ren) blob(s).
 *  ┌──────┐
 *  │┌─┐   │
 *  ││F│ C │
 *  │└─┘   │
 *  └──────┘
 *
 * • coarse pixel: Pixel which is part of the coarse grid
 *         => p.x%stepwidth = 0 = p.y%stepwidth
 *
 * • A_C - Area of coarse box a.k.a. number of pixels in this box.
 * • A_F - Sum of areas of fine boxes.
 * • N_C - Number of coarse pixels in C.
 *         ( approx. A_C/stepwidth^2. )
 * • N_F - Number of coarse pixels in all children bounding boxes.
 *
 * • S_C - Number of coarse pixels in the blob.
 *         ( This value would stored in data->area after the call
 *           of blobtree_sum_areas, but we eval it on the fly, here. )
 * • S_F - Number of coarse pixels of children blobs.
 *
 * Fine bounding boxes are complete in the blob, thus
 * => A = A_F + X
 *
 * Y := N_F - S_F is the number of coarse pixels in F which are not
 *      in one of the children blobs.
 *
 * Estimation of X:
 *
 *  X = (A_C - A_F) * (S_C - Y)/(N_C - N_F)
 *
 * Moreover, S_C-Y = 2S_F + comp_size(i) - N_F.
 *
 * The algorithm loops over every node and eval
 * 2S_F + comp_size(i) - N_F, which will stored in node->data->area.
 * After all children of a node was processed the approximation
 * starts, which will replace node->data->area.
 *
 * */
static inline uint32_t number_of_coarse_roi(
    BlobtreeRect* roi,
    uint32_t sw,
    uint32_t sh);

void blobtree_approx_areas(
    const Tree * const tree,
    Node * const startnode,
    const uint32_t * const comp_size,
    const uint32_t stepwidth,
    const uint32_t stepheight)
{

  Node *node = startnode;
  Node *root = tree->root;
  Blob* data = (Blob*)node->data;

  if( node->child == NULL ){
    //tree has only one node: startnode.
    const uint32_t N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
    const uint32_t A_C = (data->roi.width*data->roi.height);
    data->area = A_C * ((float)data->area/N_C);
    return;
  }

  /*store A_F= Sum of areas of children bounding boxes.
   * We use *(pA_F +(node-root) ) for access. Thus, we
   * need the root node of the tree as anchor (or doubles the array size)
   * to avoid access errors.
   * */
  uint32_t * const pA_F = (uint32_t*) calloc(tree->size, sizeof(uint32_t) );

  do{
    data->area = *(comp_size + data->id );

    /* Go to next node. update parent node on uprising flank */
    if( node->child != NULL ){
      node = node->child;
      data = (Blob*)node->data;
      continue;
    }

    //printf("Id: %u Roi: (%u, %u, %u, %u)\n", data->id,
    //    data->roi.x, data->roi.y, data->roi.width, data->roi.height);
    const uint32_t N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
    const uint32_t A_C = (data->roi.width*data->roi.height);
    //printf("N_C=%u, A_C=%u\n\n", N_C, A_C);


    /* Update parent node. N_C, A_C of this level is part of N_F, A_F from parent*/
    ((Blob*)node->parent->data)->area += (data->area <<1) - N_C;
    *(pA_F + (node->parent - root) ) += A_C;

    /*Eval approximation, use A = A_C * S_C/N_C (for leafs is A_F=N_F=S_F=0 ) */
    if( N_C ){
      data->area = A_C * ((float)data->area/N_C) + 0.5f;
    }else{
      data->area = 0; //area contains only subpixel
    }

    if( node->sibling != NULL ){
      node = node->sibling;
      data = (Blob*)node->data;
      continue;
    }

    while( node->parent != root ){
      /*
       * All children of parent processed.
       * We can start the approximation for the parent node.
       * */
      node = node->parent;
      data = (Blob*)node->data;

      const uint32_t N_C = number_of_coarse_roi(&data->roi, stepwidth, stepheight);
      const uint32_t A_C = (data->roi.width*data->roi.height);

      if( node!=startnode ){//required to avoid changes over startnode
        /* Update parent node. N_C, A_C of this level is part of N_F, A_F from parent*/
        ((Blob*)node->parent->data)->area += (data->area <<1) - N_C;
        *(pA_F + (node->parent - root) ) += A_C;
      }

      const uint32_t A_F = *(pA_F + (node - root) );
      /* A = A_F + (A_C - A_F) * (2*S_F + comp_size(i) - N_F) */
      if( N_C ){
        data->area = A_F + (A_C - A_F) * ((float)data->area/N_C) +0.5f;
      }else{
        data->area = 0; //area contains only subpixel
      }

      if( node->sibling != NULL ){
        node = node->sibling;
        data = (Blob*)node->data;
        break;
      }
    }

  }//while( node != startnode );
  while( node->parent != root );

  free( pA_F);
}

/* Returns the number of coarse pixels of a roi, see sketch for 
   stepwidth=3=stepheight, W=10=H and roi={4, 0, 4, 7}:

   x - - 0 1 0 0 - x -
   - - - 0 0 0 0 - - -
   - - - 0 0 0 0 - - -
   - - - 0 0 0 0 - - -
   x - - 0 1 0 0 - x -
   - - - 0 0 0 0 - - -
   - - - 0 0 0 0 - - -
   - - - - - - - - - -
   x - - - x - - - x -
   - - - - - - - - - -
*/
static inline uint32_t number_of_coarse_roi(
    BlobtreeRect* roi,
    uint32_t sw,
    uint32_t sh)
{
  /* Note:
   * Three steps for each dimension of [a1, b1]x[a2, b2], a_i < b_i (not <= !)
   * 1. Shift roi to [0, b-a]
   * 2. Decrease length if startpoint is not on coarse grid (a%sw!=0)
   *    => [0, b-a-m]
   * 3. Divide by stepwidth, stepheight [0, (b-a-m)/sw]
   * 4. Transform   (0) 0 0 1 1 1 2 2 2 … (Interval length remainder)
   *            to  (1) 1 1 2 2 2 3 3 3
   *            (This will be done by adding sw-1 before step 3.)
   *    
   * Do not cut of +sw in (... +sw)%sw because -1%sw = -1 != sw-1 !!
   * */
  return (  (roi->width + sw-1 - (sw-1-(roi->x +sw-1 )%sw)  ) /sw)
    *(  (roi->height + sh-1 - (sh-1-(roi->y +sh-1 )%sh)  ) /sh);
}

#endif //BLOB_DIMENSION
#endif //BLOB_COUNT_PIXEL

#ifdef BLOB_BARYCENTER

// Variant of round(A/B) without floats
#define ROUND_DIV(DIVIDEND, DIVISOR) ((DIVIDEND + ((DIVISOR)>>1)) / DIVISOR)

/* Helper function only callable for start node below top level
 * (thus, start_node->data->id > -1). */
void _eval_barycenters(
    Node * const start_node,
    const uint32_t * const comp_size,
    BLOB_BARYCENTER_TYPE * const pixel_sum_X,
    BLOB_BARYCENTER_TYPE * const pixel_sum_Y
    )
{
    Node * const root = start_node->parent;
    Node *node = start_node;
    Blob *data = (Blob*)node->data;
    Blob *parentdata;

    /* Loop through all nodes in depth-first traversal.
     *
     *  Pre-order operations:
     *     • (1) Set data->area.
     *   In-order operations:
     *     —
     * Post-order operations:
     *     • (2) Eval barycenter (Can only be processed after all child nodes)
     *     If parent exists:
     *     • (3) Update data->area of parent node
     *     • (4) Update pixel_sum_X, pixel_sum_Y for parent's data->id
     *
     * */

    /* The main root (id == -1)
     * needs to be skipped if we access arrays like pixel_sum_X
     * or updating data->area.
     */

    do{
        data->area = *(comp_size + data->id ); // (1)

        /* Go to next node. Other operations made on uprising flank */
        if( node->child != NULL ){
            node = node->child;
            data = (Blob*)node->data;
            continue;
        }

        // Here, the node has to be a leaf
        // (2)
        data->barycenter[0] = ROUND_DIV(*(pixel_sum_X + data->id), data->area);
        data->barycenter[1] = ROUND_DIV(*(pixel_sum_Y + data->id), data->area);

        // (3) and (4)
        parentdata = (Blob*)node->parent->data;
        if(node->parent != root ){
            parentdata->area += data->area;
            *(pixel_sum_X + parentdata->id ) += *(pixel_sum_X + data->id );
            *(pixel_sum_Y + parentdata->id ) += *(pixel_sum_Y + data->id );
        }

        if( node->sibling != NULL ){
            node = node->sibling;
            data = (Blob*)node->data;
            continue;
        }

        // Here, node has no siblings anymore
        // Thus, all children of parent node was handled and we can go upwards.
        node = node->parent;
        data = (Blob*)node->data;

        while( node != root /* Siblings of start_node will be processed 
                               in another call of _eval_barycenters() */){
            // (2)
            data->barycenter[0] = ROUND_DIV(*(pixel_sum_X + data->id), data->area);
            data->barycenter[1] = ROUND_DIV(*(pixel_sum_Y + data->id), data->area);

            // (3) and (4)
            parentdata = (Blob*)node->parent->data;
            if(node->parent != root ){
                parentdata->area += data->area;
                *(pixel_sum_X + parentdata->id ) += *(pixel_sum_X + data->id );
                *(pixel_sum_Y + parentdata->id ) += *(pixel_sum_Y + data->id );
            }

            if( node->sibling != NULL ){
                node = node->sibling;
                data = (Blob*)node->data;
                break;
            }

            node = node->parent;
            data = (Blob*)node->data;
        }

    }while( node != root );
}

/* 
 * This functions loops through the tree. If all children of
 * of a node P was handled, the values pixel_sum_*[P.id] will
 * be accumulate by the values of the children.
 * Finally, the barycenter will be evaluated.
 *
 * Notes:
 * - root node data will not be altered if it's id==-1.
 * - This function SUPERSEEDS blobtree_sum_areas and changes the area value, too.
 *   => Call other functions, which set the area value (i.e. approx areas)
 *   after this function.
 * - This function changes the values of the arguments pixel_sum_*.
 * - This function requires the values of comp_size because the 
 *   value of [node]->data->area could be unusable (for stepwidth>1).
 *   Thats the reason for setting the area value during the loop, too.
 * */
void blobtree_eval_barycenters(
    Node * const root,
    const uint32_t * const comp_size,
    BLOB_BARYCENTER_TYPE * const pixel_sum_X,
    BLOB_BARYCENTER_TYPE * const pixel_sum_Y
    )
{
    Node *node = root;
    Blob *data = (Blob*)node->data;
    if (data->id != -1){
        return _eval_barycenters(root, comp_size, pixel_sum_X, pixel_sum_Y);
    }

    node = node->child;
    while( node != NULL ) {
        _eval_barycenters(node, comp_size, pixel_sum_X, pixel_sum_Y);
        node = node->sibling;
    }
    // Finally, special treatment of node with id -1.
    // It's the whole plane/roi. Just take the midpoint.
    data->barycenter[0] = data->roi.x + data->roi.width/2;
    data->barycenter[1] = data->roi.y + data->roi.height/2;
}

#endif

#ifdef BLOB_DIMENSION
/* Assume type(data) = Blob* */
void blobtree_set_area_prop(
    Node * const root)
{
#if 1
  Node *node = root;
  Blob *data;
  do{
    data = (Blob*)node->data;
    data->area = data->roi.width * data->roi.height;
    if( node->child != NULL){
      node = node->child;
    }else if( node->sibling != NULL ){
      node = node->sibling;
    }else{
      while( node != root ){
        node = node->parent;
        if( node->sibling != NULL ){
          node = node->sibling;
          break;
        }
      }
    }
  }while( node != root );

#else
  /* Recursive formulation */
  Blob* data = (Blob*)root->data;
  data->area = data->roi.width * data->roi.height;
  if( root->child != NULL) set_area_prop(root->child);
  if( root->sibling != NULL) set_area_prop(root->sibling);
#endif
}

#endif

