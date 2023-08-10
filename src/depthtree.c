#ifndef DEPTHTREE_C
#define DEPTHTREE_C

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> //for memset

#include <assert.h>

#include "blob.h"
#include "depthtree.h"
#define INLINE static inline
#include "tree_intern.h" // _reallocarray_or_free()

#include "depthtree_macros.h"

int32_t depthtree_create_workspace(
    const uint32_t w, const uint32_t h,
    DepthtreeWorkspace **pworkspace
    ){

  if( *pworkspace != NULL ){
    //destroy old struct.
    depthtree_destroy_workspace( pworkspace );
  }
  //Now, *pworkspace is NULL

  if( w*h == 0 ) return -1;

  DepthtreeWorkspace *r = malloc( sizeof(DepthtreeWorkspace) );

  r->w = w;
  r->h = h;
  const uint32_t max_comp = (w+h)*100U;
  r->max_comp = max_comp;
  r->used_comp = 0;

  if(
      ( r->ids = (uint32_t*) malloc( w*h*sizeof(uint32_t) ) ) == NULL ||
#ifndef NO_DEPTH_MAP
      //            ( r->depths = (uint32_t*) malloc( w*h*sizeof(uint32_t) ) ) == NULL ||
      ( r->depths = (uint8_t*) calloc( w*h,sizeof(uint8_t) ) ) == NULL ||
#endif
      ( r->id_depth = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
      ( r->comp_same = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
      ( r->prob_parent = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
#ifdef BLOB_COUNT_PIXEL
      ( r->comp_size = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
#endif
#ifdef BLOB_DIMENSION
      ( r->top_index = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
      ( r->left_index = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
      ( r->right_index = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL ||
      ( r->bottom_index = (uint32_t*) malloc( max_comp*sizeof(uint32_t) ) ) == NULL || 
#endif
#ifdef BLOB_BARYCENTER
      ( r->pixel_sum_X = (BLOB_BARYCENTER_TYPE*) malloc( max_comp*sizeof(BLOB_BARYCENTER_TYPE) ) ) == NULL ||
      ( r->pixel_sum_Y = (BLOB_BARYCENTER_TYPE*) malloc( max_comp*sizeof(BLOB_BARYCENTER_TYPE) ) ) == NULL ||
#endif
      ( r->a_ids = (uint32_t*) malloc( 255*sizeof(uint32_t) ) ) == NULL || 
      ( r->b_ids = (uint32_t*) malloc( 255*sizeof(uint32_t) ) ) == NULL || 
      ( r->c_ids = (uint32_t*) malloc( 255*sizeof(uint32_t) ) ) == NULL || 
      ( r->d_ids = (uint32_t*) malloc( 255*sizeof(uint32_t) ) ) == NULL || 
      ( r->a_dep = (uint8_t*) malloc( 255*sizeof(uint8_t) ) ) == NULL || 
      ( r->b_dep = (uint8_t*) malloc( 255*sizeof(uint8_t) ) ) == NULL || 
      ( r->c_dep = (uint8_t*) malloc( 255*sizeof(uint8_t) ) ) == NULL || 
      ( r->d_dep = (uint8_t*) malloc( 255*sizeof(uint8_t) ) ) == NULL  
      ){
        // alloc failed
        depthtree_destroy_workspace( &r );
        return -1;
      }

  /* setup first entry of *_ids and *_dep to 0->255. (0 is dummy entry with
   * depth 255).
   * The dummy entry will guarantee the existance of a child element in every
   * case. */
  r->a_ids[0] = 0; r->a_dep[0] = 255;
  r->b_ids[0] = 0; r->b_dep[0] = 255;
  r->c_ids[0] = 0; r->c_dep[0] = 255;
  r->d_ids[0] = 0; r->d_dep[0] = 255;

  r->real_ids = NULL;
  r->real_ids_inv = NULL;

  r->blob_id_filtered = NULL;

  *pworkspace=r;
  return 0;
}

int32_t depthtree_realloc_workspace(
    const uint32_t max_comp,
    DepthtreeWorkspace **pworkspace
    ){

  DepthtreeWorkspace *r = *pworkspace;
  r->max_comp = max_comp;
  if( 
      ( r->id_depth = (uint32_t*) _reallocarray_or_free(r->id_depth, max_comp, sizeof(uint32_t) ) ) == NULL ||
      ( r->comp_same = (uint32_t*) _reallocarray_or_free(r->comp_same, max_comp, sizeof(uint32_t) ) ) == NULL ||
      ( r->prob_parent = (uint32_t*) _reallocarray_or_free(r->prob_parent, max_comp, sizeof(uint32_t) ) ) == NULL ||
#ifdef BLOB_COUNT_PIXEL
      ( r->comp_size = (uint32_t*) _reallocarray_or_free(r->comp_size, max_comp, sizeof(uint32_t) ) ) == NULL ||
#endif
#ifdef BLOB_DIMENSION
      ( r->top_index = (uint32_t*) _reallocarray_or_free(r->top_index, max_comp, sizeof(uint32_t) ) ) == NULL ||
      ( r->left_index = (uint32_t*) _reallocarray_or_free(r->left_index, max_comp, sizeof(uint32_t) ) ) == NULL ||
      ( r->right_index = (uint32_t*) _reallocarray_or_free(r->right_index, max_comp, sizeof(uint32_t) ) ) == NULL ||
      ( r->bottom_index = (uint32_t*) _reallocarray_or_free(r->bottom_index, max_comp, sizeof(uint32_t) ) ) == NULL ||
#endif
#ifdef BLOB_BARYCENTER
      ( r->pixel_sum_X = (BLOB_BARYCENTER_TYPE*) _reallocarray_or_free(r->pixel_sum_X, max_comp, sizeof(BLOB_BARYCENTER_TYPE) ) ) == NULL ||
      ( r->pixel_sum_Y = (BLOB_BARYCENTER_TYPE*) _reallocarray_or_free(r->pixel_sum_Y, max_comp, sizeof(BLOB_BARYCENTER_TYPE) ) ) == NULL ||
#endif
      0 ){
    // realloc failed
    VPRINTF("Critical error: Reallocation of workspace failed!\n");
    depthtree_destroy_workspace( pworkspace );
    return -1;
  }

  free(r->blob_id_filtered);//omit unnessecary reallocation and omit wrong/low size
  r->blob_id_filtered = NULL;//should be allocated later if needed.

  return 0;
}

void depthtree_destroy_workspace(
    DepthtreeWorkspace **pworkspace 
    ){
  if( *pworkspace == NULL ) return;

  DepthtreeWorkspace *r = *pworkspace ;
  free(r->ids);
#ifndef NO_DEPTH_MAP
  free(r->depths);
#endif
  free(r->id_depth);
  free(r->comp_same);
  free(r->prob_parent);
#ifdef BLOB_COUNT_PIXEL
  free(r->comp_size);
#endif
#ifdef BLOB_DIMENSION
  free(r->top_index);
  free(r->left_index);
  free(r->right_index);
  free(r->bottom_index);
#endif
#ifdef BLOB_BARYCENTER
  free(r->pixel_sum_X);
  free(r->pixel_sum_Y);
#endif
  free(r->a_ids);
  free(r->b_ids);
  free(r->c_ids);
  free(r->d_ids);
  free(r->a_dep);
  free(r->b_dep);
  free(r->c_dep);
  free(r->d_dep);

  free(r->real_ids);
  free(r->real_ids_inv);

  free(r->blob_id_filtered);

  free(r);
  *pworkspace = NULL;
}

FORCEINLINE
Tree* _find_depthtree(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t *depth_map,
    const uint32_t stepwidth,
    DepthtreeWorkspace *workspace,
    Blob** tree_data );

Tree* find_depthtree(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t *depth_map,
    const uint32_t stepwidth,
    DepthtreeWorkspace *workspace,
    Blob** tree_data )
{
  if( stepwidth == 1 )
      return _find_depthtree(data, w, h, roi, depth_map, 1, workspace, tree_data);
  if( stepwidth == 2 )
      return _find_depthtree(data, w, h, roi, depth_map, 2, workspace, tree_data);
  if( stepwidth == 3 )
      return _find_depthtree(data, w, h, roi, depth_map, 3, workspace, tree_data);
  if( stepwidth == 4 )
      return _find_depthtree(data, w, h, roi, depth_map, 4, workspace, tree_data);
  if( stepwidth == 5 )
      return _find_depthtree(data, w, h, roi, depth_map, 5, workspace, tree_data);

  return _find_depthtree(data, w, h, roi, depth_map, stepwidth, workspace, tree_data);
}

FORCEINLINE
Tree* _find_depthtree(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t *depth_map,
    const uint32_t stepwidth,
    DepthtreeWorkspace *workspace,
    Blob** tree_data )
{

  //#define stepwidth 7 //speed up due faster addition for fixed stepwidth?!
#define stepheight stepwidth
  /* Marks of 10 Cases:
   *  x - stepwidth, y - stepheight, swr - (w-1)%x, shr - (h-1)%y
   * <----------------------- w --------------------------------->
   * |        <-- roi.width -------------->
   * |        <-- roi.width-swr -->
   * | 
   * | | |    A ←x→ B ←x→ … B ←x→ B ←swr→ C
   * | | roi  ↑                   ↑       ↑
   * h | hei  y                   y       y
   * | r ght  ↓                   ↓       ↓
   * | o -    E ←x→ F ←x→ … F ←x→ G ←swr→ H
   * | i shr  ↑                   ↑       ↑
   * | . |    y                   y       y
   * | h |    ↓                   ↓       ↓
   * | e |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
   * | i |    …                   …       …
   * | g |    E ←x→ F ←x→ … F ←x→ G ←swr→ H
   * | h      ↑                   ↑       ↑
   * | t     shr                 shr     shr
   * | |      ↓                   ↓       ↓
   * h |      L ←x→ M ←x→ … M ←x→ N ←swr→ P
   * |  
   * |  
   * | 
   *
   */
  //init
  uint32_t const r=w-roi.x-roi.width; //right border
  uint32_t const b=h-roi.y-roi.height; //bottom border
  if( r<0 || b<0 ){
    fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
    *tree_data = NULL;
    return NULL;
  }

  uint32_t const swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
  uint32_t const shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
  uint32_t const sh = stepheight*w;
  uint32_t const sh1 = (stepheight-1)*w;
  uint32_t const sh2 = shr*w;

  uint32_t id=-1;//id for last component, attention, unsigned variable!!
  uint32_t k; //loop variable
  uint32_t max_comp = workspace->max_comp; 
  uint32_t idA, idB; 
  uint8_t depX; 

  uint32_t* ids = workspace->ids; 
  uint32_t* comp_same = workspace->comp_same; 
  uint32_t* prob_parent = workspace->prob_parent; 
  uint32_t* id_depth = workspace->id_depth; 
#ifdef BLOB_COUNT_PIXEL
  uint32_t* comp_size = workspace->comp_size; 
#endif
#ifdef BLOB_DIMENSION
  uint32_t* top_index = workspace->top_index;
  uint32_t* left_index = workspace->left_index;
  uint32_t* right_index = workspace->right_index;
  uint32_t* bottom_index = workspace->bottom_index;
#endif
#ifdef PIXEL_POSITION
  uint32_t s=roi.x,z=roi.y; //s-spalte, z-zeile
#else
  const uint32_t s=0,z=0; //Should not be used.
#endif
#ifdef BLOB_BARYCENTER
  BLOB_BARYCENTER_TYPE *pixel_sum_X = workspace->pixel_sum_X; 
  BLOB_BARYCENTER_TYPE *pixel_sum_Y = workspace->pixel_sum_Y; 
#endif
  uint32_t *a_ids, *b_ids/*, *c_ids, *d_ids*/;
  uint8_t *a_dep, *b_dep/*, *c_dep, *d_dep*/;  


#ifdef NO_DEPTH_MAP
  /* map depth on data array */
  uint8_t * const depths = data;
#else
  uint8_t * const depths = workspace->depths;
#endif

  uint8_t * const depS = depths+w*roi.y+roi.x;
  const uint8_t* const dS = data+(depS-depths);

  const uint8_t* depR = depS+roi.width; //Pointer to right border. Update on every line
  const uint8_t* depR2 = depR-swr; //cut last indizies.

  const uint8_t* const depE = depR + (roi.height-1)*w;
  const uint8_t* const depE2 = depE - shr*w;//remove last lines.

  uint32_t* iPi = ids+(dS-data); // Poiner to ids+i
  uint8_t *depPi = depS;



  /* Eval depth(roi) aka  *(depth_map+i) for i∈roi 
   * */
#ifndef NO_DEPTH_MAP
  const uint8_t* dPi = dS; // Pointer to data+i 

  for( ; depPi<depE2 ;  ){
    for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
      *depPi = *(depth_map + *dPi);
    } 

    //handle last index of current line
    dPi -= stepwidth-swr;
    depPi -= stepwidth-swr;
    if( swr ){
      *depPi = *(depth_map + *dPi);
    }

    //move pointer to 'next' row
    dPi += r+roi.x+sh1+1;
    depPi += r+roi.x+sh1+1;
    depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
    depR2 += sh;
  }

  //handle last line & last element
  if( shr ){
    dPi -= sh-sh2;
    depPi -= sh-sh2;
    depR -= sh-sh2;
    depR2 -= sh-sh2;

    for( ; depPi<depR2 ; dPi += stepwidth, depPi += stepwidth ){
      *depPi = *(depth_map + *dPi);
    } 

    //handle last index of current line
    dPi -= stepwidth-swr;
    depPi -= stepwidth-swr;
    if( swr ){
      *depPi = *(depth_map + *dPi);
    }
  }

  //reset pointer;
  depPi = depths+(dS-data);
  depR = depS+roi.width; 
  depR2 = depR-swr; //cut last indizies.

#if VERBOSE > 0
  printf("Depth matix(roi): \n");
  debug_print_matrix_char( depths, w, h, roi, 1, 1);
#endif
#else
#endif

  /* Dummy for foreground (id=0, depth=255). It's important to set id=0 for
   * this component. */
  NEW_COMPONENT(1, 255 );
  /* Dummy for background (id=1, depth=0), Attention, -1=MAX_UINT */
  NEW_COMPONENT(-1, 0 );

#ifdef BLOB_COUNT_PIXEL
  /* Set size of dummy foreground component to 0. */
  *(comp_size+0) = 0;
#endif
#ifdef BLOB_BARYCENTER
  /* Dummy foreground should not influence the barycenter. */
  *(pixel_sum_X+0) = 0;
  *(pixel_sum_Y+0) = 0;
#endif



  /**** A,A'-CASE *****/
  //top, left corner of BlobtreeRect get first id (=2).
  depX = *depPi;
  if( depX>0 ){
#ifdef BLOB_COUNT_PIXEL
    /* Set size of background dummy component to 0. */
    *(comp_size+1) = 0;
#endif
#ifdef BLOB_BARYCENTER
    /* Dummy background should not influence the barycenter. */
    *(pixel_sum_X+1) = 0;
    *(pixel_sum_Y+1) = 0;
#endif
    NEW_COMPONENT(1, depX );
  }else{
    //use dummy component id=1 for corner element. This avoid wrapping of
    //all blobs.
    *iPi = 1;
  }

  iPi += stepwidth;
  depPi += stepwidth;
#ifdef PIXEL_POSITION
  s += stepwidth;
#endif

  //top border
  /**** B-CASE *****/
  for( ; depPi<depR2; iPi += stepwidth, depPi += stepwidth ){
    idA = *(iPi-stepwidth);
    depX = *depPi;
    INSERT_ELEMENT1( idA, iPi, *depPi);

#ifdef PIXEL_POSITION
    s += stepwidth;
#endif
  }

  //correct pointer shift of last for loop step.
  iPi -= stepwidth-swr;
  depPi -= stepwidth-swr;
#ifdef PIXEL_POSITION
  s -= stepwidth-swr;
#endif

  //continue with +swr stepwidth
  if(swr){
    /**** C-CASE *****/
    idA = *(iPi-swr);
    depX = *depPi;
    INSERT_ELEMENT1( idA, iPi, *depPi);
  }

  //move pointer to 'next' row
  depPi += r+roi.x+sh1+1;
  iPi += r+roi.x+sh1+1;
  depR += sh; //rechter Randindex wird in nächste Zeile geschoben.
  depR2 += sh;
#ifdef PIXEL_POSITION
  s=roi.x;
  z += stepheight;
#endif

  //2nd,...,(h-shr)-row
  for( ; depPi<depE2 ; depPi += r+roi.x+sh1+1, iPi += r+roi.x+sh1+1, depR += sh, depR2 += sh ){

    //left border
    /**** E-CASE *****/
    idA = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth), *(depPi-sh), *(depPi-sh+stepwidth) );
    depX = *depPi;
    INSERT_ELEMENT1( idA, iPi, *depPi);

    iPi += stepwidth;
    depPi += stepwidth;
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif

    /*inner elements till last colum before depR2 reached.
     * => Lefthand tests with -stepwidth
     *    Righthand tests with +stepwidth
     */
    for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
      /**** F-CASE *****/

      idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
          *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
      idB = ARGMAX2( *(iPi-sh), *(iPi-sh+stepwidth),
          *(depPi-sh), *(depPi-sh+stepwidth) ); /* max(b,c) */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);
#ifdef PIXEL_POSITION
      s += stepwidth;
#endif
    }


    /* If depR2==depR, then the last column is reached. Thus it's not possibe
     * to check diagonal element. (only G case)
     * Otherwise use G and H cases. 
     */
    if( swr /*depR2!=depR*/ ){
      //structure: (depPi-stepwidth),(depPi),(depPi+swr)
      /**** G-CASE *****/
      idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
          *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
      idB = ARGMAX2( *(iPi-sh), *(iPi-sh+swr),
          *(depPi-sh), *(depPi-sh+swr) ); /* max(b,c) */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

      iPi+=swr;
      depPi+=swr;
#ifdef PIXEL_POSITION
      s+=swr;
#endif

      //right border, not check diag element
      /**** H-CASE *****/
      idA = ARGMAX2( *(iPi-sh-swr), *(iPi-swr),
          *(depPi-sh-swr), *(depPi-swr) ); /* max(a,d) */
      idB = *(iPi-sh); /* b */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

    }else{
      /**** G-CASE *****/
      idA = ARGMAX2( *(iPi-sh-stepwidth), *(iPi-stepwidth),
          *(depPi-sh-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
      idB = *(iPi-sh); /* b */ 
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

    }//end of else case of (depR2!=depR)

#ifdef PIXEL_POSITION
    s=roi.x;
    z += stepheight;
#endif

  } //row for loop

  iPi -= sh-sh2;//(stepheight-1)*w;
  depPi -= sh-sh2;//(stepheight-1)*w;
  depR -= sh-sh2;
  depR2 -= sh-sh2;
#ifdef PIXEL_POSITION
  z -= stepheight-shr;
#endif

  if( shr /*dE2<dE*/ ){

    //left border
    /**** L-CASE *****/
    idA = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth), *(depPi-sh2), *(depPi-sh2+stepwidth) );
    depX = *depPi;
    INSERT_ELEMENT1( idA, iPi, *depPi);

    iPi += stepwidth;
    depPi += stepwidth;
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif

    /*inner elements till last colum before depR2 reached.
     * => Lefthand tests with -stepwidth
     *    Righthand tests with +stepwidth
     */
    for( ; depPi<depR2-stepwidth; iPi += stepwidth, depPi += stepwidth ){
      /**** M-CASE *****/
      idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
          *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
      idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+stepwidth),
          *(depPi-sh2), *(depPi-sh2+stepwidth) ); /* max(b,c) */

      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

#ifdef PIXEL_POSITION
      s += stepwidth;
#endif
    }

    /* If depR2==depR, then the last column is reached. Thus it's not possibe
     * to check diagonal element. (only N case)
     * Otherwise use N and P cases. 
     */
    if( swr/*depR2!=depR*/ ){
      //structure: (depPi-stepwidth),(depPi),(depPi+swr)
      /**** N-CASE *****/
      idA = ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
          *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */
      idB = ARGMAX2( *(iPi-sh2), *(iPi-sh2+swr),
          *(depPi-sh2), *(depPi-sh2+swr) ); /* max(b,c) */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

      iPi+=swr;
      depPi+=swr;
#ifdef PIXEL_POSITION
      s+=swr;
#endif

      //right border, not check diag element
      /**** P-CASE *****/
      idA = ARGMAX2( *(iPi-sh2-swr), *(iPi-swr),
          *(depPi-sh2-swr), *(depPi-swr) ); /* max(a,d) */
      idB = *(iPi-sh2); /* b */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

    }else{
      /**** N-CASE *****/
      idA =ARGMAX2( *(iPi-sh2-stepwidth), *(iPi-stepwidth),
          *(depPi-sh2-stepwidth), *(depPi-stepwidth) ); /* max(a,d) */ 
      idB = *(iPi-sh2); /* b */
      depX = *depPi;
      INSERT_ELEMENT2( idA, idB, iPi, *depPi);

    }//end of else case of (depR2!=depR)

  } //end of if(depE2<depE)

  /* end of main algo */

#if VERBOSE > 0 
  //printf("Matrix of ids:\n");
  //print_matrix(ids,w,h);

  //printf("comp_same:\n");
  //print_matrix(comp_same, id+1, 1);
  debug_print_matrix( ids, w, h, roi, 1, 1);
  debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, 1);
#endif

  /* Postprocessing.
   * Sum up all areas with connecteted ids.
   * Then create nodes and connect them. 
   * If BLOB_DIMENSION is set, detect
   * extremal limits in [left|right|bottom]_index(*(real_ids+X)).
   * */
  uint32_t nids = id+1; //number of ids
  uint32_t tmp_id,/*tmp_id2,*/ real_ids_size=0,l;
  //int32_t* real_ids = calloc( nids,sizeof(int32_t) ); //store join of ids.
  //int32_t* real_ids_inv = calloc( nids,sizeof(int32_t) ); //store for every id with position in real_id link to it's position.
  free(workspace->real_ids);
  workspace->real_ids = calloc( nids, sizeof(int32_t) ); //store join of ids.
  uint32_t* const real_ids = workspace->real_ids;

  free(workspace->real_ids_inv);
  workspace->real_ids_inv = calloc( nids, sizeof(uint32_t) ); //store for every id with position in real_id link to it's position.
  uint32_t* const real_ids_inv = workspace->real_ids_inv;

  for(k=1;k<nids;k++){ // k=1 skips the foreground dummy component id=0

    /* Sei F=comp_same. Wegen F(x)<=x folgt (F wird innerhalb dieser Schleife angepasst!)
     * F^2 = F^3 = ... = F^*
     * D.h. um die endgültige id zu finden muss comp_same maximal zweimal aufgerufen werden.
     * */
    tmp_id = *(comp_same+k); 

#if VERBOSE > 0
    printf("%u: (%u->%u ",k,k,tmp_id);
#endif
    if( tmp_id != k ){
      tmp_id = *(comp_same+tmp_id); 
      *(comp_same+k) = tmp_id; 
#if VERBOSE > 0
      printf("->%u ",tmp_id);
#endif
    }
#if VERBOSE > 0
    printf(")\n");
#endif

    if( tmp_id != k ){

#ifdef BLOB_COUNT_PIXEL
      //move area size to other id.
      *(comp_size+tmp_id) += *(comp_size+k); 
      *(comp_size+k) = 0;
#endif

#ifdef BLOB_DIMENSION
      //update dimension
      if( *( top_index+tmp_id ) > *( top_index+k ) )
        *( top_index+tmp_id ) = *( top_index+k );
      if( *( left_index+tmp_id ) > *( left_index+k ) )
        *( left_index+tmp_id ) = *( left_index+k );
      if( *( right_index+tmp_id ) < *( right_index+k ) )
        *( right_index+tmp_id ) = *( right_index+k );
      if( *( bottom_index+tmp_id ) < *( bottom_index+k ) )
        *( bottom_index+tmp_id ) = *( bottom_index+k );
#endif

#ifdef BLOB_BARYCENTER
      //shift values to other id
      *(pixel_sum_X+tmp_id) += *(pixel_sum_X+k); 
      *(pixel_sum_X+k) = 0;
      *(pixel_sum_Y+tmp_id) += *(pixel_sum_Y+k); 
      *(pixel_sum_Y+k) = 0;
#endif

    }else{
      //Its a component id of a new area
      *(real_ids+real_ids_size) = tmp_id;
      *(real_ids_inv+tmp_id) = real_ids_size;//inverse function
      real_ids_size++;
    }

  }

  /*
   * Generate tree structure
   */

  /* store for real_ids the index of the node in the tree array */
  uint32_t *tree_id_relation = malloc( (real_ids_size+1)*sizeof(uint32_t) );

  Tree *tree = tree_create(real_ids_size + 1, 0);
  *tree_data = malloc( tree->size * sizeof(Blob) ); // data
  Node * const root = tree->root;

  //init all node as leafs
  for(l=0;l<real_ids_size+1;l++) *(root+l)=Leaf;

  //set root node (The desired output are the children of this node.)
  Node *cur  = root;
  Blob *curdata  = *tree_data;

  /* Set root node which represents the whole image/ROI.
   * Keep in mind, that the number of children depends 
   * on the border pixels of the ROI.
   * Almost in every case it's only one child, but not always.
   * */
  curdata->id = -1; /* = MAX_UINT */
  memcpy( &curdata->roi, &roi, sizeof(BlobtreeRect) );
  curdata->area = roi.width * roi.height;
#ifdef SAVE_DEPTH_MAP_VALUE
  curdata->depth_level = 0; 
#endif
#ifdef BLOB_BARYCENTER
  /* The barycenter will not set here, but in blobtree_eval_barycenters(...) */
  //curdata->barycenter[0] = roi.x + roi.width/2;
  //curdata->barycenter[1] = roi.y + roi.height/2;
#endif
  cur->data = curdata; // link to the data array.

  BlobtreeRect *rect;

  for(l=0;l<real_ids_size;l++){
    cur++;
    curdata++;
    cur->data = curdata; // link to the data array.

    const uint32_t rid = *(real_ids+l);
    curdata->id = rid; //Set id of this blob.
#ifdef BLOB_DIMENSION
    rect = &curdata->roi;
    rect->y = *(top_index + rid);
    rect->height = *(bottom_index + rid) - rect->y + 1;
    rect->x = *(left_index + rid);
    rect->width = *(right_index + rid) - rect->x + 1;
#endif
#ifdef BLOB_BARYCENTER
    /* The barycenter will not set here, but in blobtree_eval_barycenters(...) */
    //curdata->barycenter[0] = *(pixel_sum_X + rid) / *(comp_same + rid);
    //curdata->barycenter[1] = *(pixel_sum_Y + rid) / *(comp_same + rid);
#endif
#ifdef SAVE_DEPTH_MAP_VALUE
    curdata->depth_level = *(id_depth + rid );
#endif

    tmp_id = *(prob_parent+*(real_ids+l)); //get id of parent (or child) area. 
    if( tmp_id == -1U /*=MAX_UINT*/ ){
      /* Use root as parent node. */
      tree_add_child(root, cur );
    }else{
      //find real id of parent id.
#if 1
      tmp_id = *(comp_same+tmp_id); 
#else
      //this was commented out because comp_same is here a projection.
      tmp_id2 = *(comp_same+tmp_id); 
      while( tmp_id != tmp_id2 ){
        tmp_id = tmp_id2; 
        tmp_id2 = *(comp_same+tmp_id); 
      }
#endif

      /*Now, tmp_id is in real_id array. And real_ids_inv is defined. */
      tree_add_child( root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id ),
          cur );
    }

    //tree_print(tree, NULL, 0);
  }


#ifdef BLOB_BARYCENTER
  blobtree_eval_barycenters(root, comp_size, pixel_sum_X, pixel_sum_Y);
#define SUM_AREAS_IS_REDUNDANT
#endif

  //sum up node areas
#ifdef BLOB_COUNT_PIXEL
#if VERBOSE > 1 
  uint32_t ci;
  printf("comp_size Array:\n");
  for( ci=0 ; ci<nids; ci++){
    printf("cs[%u]=%u\n",ci, *(comp_size + *(real_ids+ci) ) );
  }
#endif
#ifndef SUM_AREAS_IS_REDUNDANT
  blobtree_sum_areas(root->child, comp_size);
#endif
#endif

#ifdef BLOB_COUNT_PIXEL_XX
  /* If no pixel has depth=0, the background dummy component (with id=1)
   * wrapping all blobs. In this case we can remove this
   * extra level from the tree.
   *
   * Without counting the pixels of the components, we can not simply 
   * remove level 1, even if it has just one level 2 node. 
   * (Counter example: a fullsize cross)
   * */
  assert(root->child);
  //if( *(comp_size+1)==0 && root->child->child ){
  while(((Blob *)root->data)->area == ((Blob *)root->child->data)->area)
  {
    assert(root->child->sibling == NULL);
#ifdef TREE_REDUNDANT_INFOS
    assert(root->width == 1);
#endif
#if VERBOSE > -1
    printf("Remove node %d, because child covers same area.\n",
        ((Blob*)root->data)->id );
#endif
    // Adding nodes of level 2 as children of level 0
    tree_add_siblings(root, root->child->child);
    // Now, old level 1 should be leaf after tree_add_siblings-call
    assert(root->child->child == NULL);

    // Transfer data pointer to root (=> id changes from -1 to 1)
    root->data = root->child->data;
    // Remove leftmost child. (old level 1)
    tree_release_child(root->child, 0);
  }
#endif

#ifdef BLOB_DIMENSION 
#ifdef EXTEND_BOUNDING_BOXES
  extend_bounding_boxes( tree );
#endif
#endif

#ifdef BLOB_SORT_TREE
  //sort_tree(root->child);
  sort_tree(root);
#endif

  //current id indicates maximal used id in ids-array
  workspace->used_comp=id;

  //clean up
  free(tree_id_relation);

  //free(real_ids);
  //free(real_ids_inv);

  return tree;
}


void depthtree_find_blobs(Blobtree *blob, const uint8_t *data, const uint32_t w, const uint32_t h, const BlobtreeRect roi, const uint8_t *depth_map, DepthtreeWorkspace *workspace ){
  // Avoid hard crash for null data.
  if( data == NULL ){
    VPRINTF("Runtime error: Input data is NULL! depthtree_find_blobs aborts.\n");
  }
  //clear old tree
  if( blob->tree != NULL){
    tree_destroy(&blob->tree);
    blob->tree = NULL;
  }
  if( blob->tree_data != NULL){
    free(blob->tree_data);
    blob->tree_data = NULL;
  }
  //get new blob tree structure.
  /*
     if( blob->grid.height == 11 ){
     blob->tree = find_depthtree11(data, w, h, roi, depth_map, 
     workspace, &blob->tree_data);
     }else{
     blob->tree = find_depthtree(data, w, h, roi, depth_map, 
     blob->grid.width,
     workspace, &blob->tree_data);
     }*/

  if( blob->grid.width == 1 ){
    blob->tree = find_depthtree(data, w, h, roi, depth_map, 1, workspace, &blob->tree_data);
  }else if( blob->grid.width == 2 ){
    blob->tree = find_depthtree(data, w, h, roi, depth_map, 2, workspace, &blob->tree_data);
  }else if( blob->grid.width == 3 ){
    blob->tree = find_depthtree(data, w, h, roi, depth_map, 3, workspace, &blob->tree_data);
  }else if( blob->grid.width == 4 ){
    blob->tree = find_depthtree(data, w, h, roi, depth_map, 4, workspace, &blob->tree_data);
  }else if( blob->grid.width == 5 ){
      blob->tree = find_depthtree(data, w, h, roi, depth_map, 5, workspace, &blob->tree_data);
  }else if( blob->grid.width == 6 ){
      blob->tree = find_depthtree(data, w, h, roi, depth_map, 6, workspace, &blob->tree_data);
    /*
       }else if( blob->grid.width == 7 ){
       blob->tree = find_depthtree(data, w, h, roi, depth_map, 7, workspace, &blob->tree_data);
       }else if( blob->grid.width == 8 ){
       blob->tree = find_depthtree(data, w, h, roi, depth_map, 8, workspace, &blob->tree_data);
       }else if( blob->grid.width == 9 ){
       blob->tree = find_depthtree(data, w, h, roi, depth_map, 9, workspace, &blob->tree_data);
       }else if( blob->grid.width == 10 ){
       blob->tree = find_depthtree(data, w, h, roi, depth_map, 10, workspace, &blob->tree_data);
       }else if( blob->grid.width == 11 ){
       blob->tree = find_depthtree(data, w, h, roi, depth_map, 11, workspace, &blob->tree_data);
       */
  }else{
    blob->tree = find_depthtree(data, w, h, roi, depth_map, blob->grid.width, workspace, &blob->tree_data);
  }







}


void depthtree_filter_blobs(
    Blobtree* blob,
    DepthtreeWorkspace *pworkspace
    ){

  uint32_t numNodes = blob->tree->size;
  VPRINTF("Num nodes: %u\n", numNodes);

  if(pworkspace->blob_id_filtered==NULL){
    //Attention, correct size of blob_id_filtered is assumed if != NULL.
    //See workspace reallocation
    pworkspace->blob_id_filtered= (uint32_t*) malloc( pworkspace->max_comp*sizeof(uint32_t) );
  }
  uint32_t * const bif = (uint32_t*) calloc( numNodes,sizeof(uint32_t) );
  uint32_t * const blob_id_filtered = pworkspace->blob_id_filtered;
  const uint32_t * const comp_same = pworkspace->comp_same;
  const uint32_t * const real_ids_inv = pworkspace->real_ids_inv;

  if( bif != NULL && blob_id_filtered != NULL ){
    bif[0]=0;
    bif[1]=1;

    /* 1. Map is identity on filtered nodes.
     * After this loop all other nodes will be still mapped to 0.
     * The ±1-shifts are caused by the dummy node on first position.
     * */
    const Node * const root = blob->tree->root;
    const Node *cur = blobtree_first(blob);
    while( cur != NULL ){
      //const uint32_t id = ((Blob*)cur->data)->id;
      //*(bif + node_id) = id;

      //const uint32_t node_id = *(pworkspace->real_ids_inv+id) + 1;
      const uint32_t node_id = cur-root;
      //note: Both definitions of node_id are equivalent.

      *(bif + node_id) = node_id;
      cur = blobtree_next(blob);
    }

    // 2. Take all nodes which are mapped to 0 and 
    // search parent node with nonzero mapping.
    // Start for index=i=2 because first node is dummy and second is root.
    uint32_t pn, ri; //parent real id, read id of parent node
    for( ri=2; ri<numNodes; ri++){
      if( bif[ri] == 0 ){
        //find parent node of 'ri' which was not filtered out
        Node *pi = (blob->tree->root +ri)->parent;
        while( pi != NULL ){
          pn = bif[pi-root];
          if( pn != 0 ){ 
            bif[ri] = pn;
            break;
          }
          pi = pi->parent;
        }//if no matching element is found, i is mapped to root id (=0).
      }
    }

    /*3. Expand bif map information on all ids
     * 3a) Use projection (yes, its project now) comp_same to map id
     *     on preimage of real_ids_inv. (=> id2)
     * 3b) Get node for id2. The dummy node produce +1 shift.
     * 3c) Finally, use bif map.
     */
    uint32_t id=pworkspace->used_comp;//dec till 0
    while( id > 1 /* id=1 and id=0 are background and dummy*/ ){
      *(blob_id_filtered+id) = *(bif + *(real_ids_inv + *(comp_same+id)) + 1 ); // Mit +1 -> Blob id, ohne +1 interne id
      id--;
    }
    *(blob_id_filtered+id) = *(bif + *(real_ids_inv + *(comp_same+id)) + 1);

#if VERBOSE > 0
    printf("bif[realid] = realid\n");
    for( ri=0; ri<numNodes; ri++){
      uint32_t id = ((Blob*)((blob->tree->root +ri)->data))->id;
      printf("id=%u, bif[%u] = %u\n",id, ri, bif[ri]);
    }
#endif

    free(bif);

  }else{
    printf("(depthtree_filter_blobs) Critical error: Mem allocation failed\n");
  }


}



#ifdef EXTEND_BOUNDING_BOXES
void extend_bounding_boxes( Tree * const tree){

  Node* root = tree->root;
  Node* cur = tree->root;
  Node* child;

  while( cur != NULL ){
    if( cur->child != NULL ) cur = cur->child;
    else if( cur->sibling != NULL ) cur = cur->sibling;
    else{
      while(cur != root && cur->sibling == NULL){
        cur = cur->parent;

        //here, all children are handled. Compare properties of cur with child properties
        //the (x,y,width,height)-Format complicates the comparation.
        BlobtreeRect *dcur = &((Blob*)cur->data)->roi;
        child = cur->child;
        while( child != NULL ){
          BlobtreeRect *dchild = &((Blob*)child->data)->roi;
          uint32_t w = dchild->width;
          uint32_t h = dchild->height;
          int32_t a = (dchild->x - dcur->x);
          int32_t b = (dchild->y - dcur->y);
          if( a<0 ){ dcur->x += a; dcur->width -= a; }else{ w += a; }
          if( dcur->width<w ){ dcur->width=w; }
          if( b<0 ){ dcur->y += b; dcur->height -= b; }else{ h += b; }
          if( dcur->height<h ){ dcur->height=h; }

          VPRINTF("Compare parent node %u with %u \n", ((Blob*)cur->data)->id, ((Blob*)child->data)->id   );

          child = child->sibling;
        }

      }
      cur = cur->sibling;
    }
  }

}
#endif


uint32_t depthtree_get_id(
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    )
{
  uint32_t id;
  uint32_t *ids, *riv, *cm;

  ids = pworkspace->ids;      // id map for complete image
  cm = pworkspace->comp_same; // Map connected ids on one representor
  riv = pworkspace->real_ids_inv; // Map representor on output id (= blob id?!)

  id = ids[ y*pworkspace->w + x ];
  return *(riv + *(cm + id)) + 1; /* -1 shift for dummy node??! */
}

uint32_t depthtree_get_id_roi(
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    )
{
  if( x < 0 || y < 0 || x >= roi.width || y >= roi.height ){
    fprintf(stderr, "(depthtree_get_id_roi) Requested pixel (%i,%i) not in roi (%i, %i, %i, %i).",
        x, y, roi.x, roi.y, roi.width, roi.height);
    return -1;
  }
  return depthtree_get_id(x + roi.x, y + roi.y, pworkspace);
}

uint32_t depthtree_get_filtered_id(
    const Blobtree *blobs,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    )
{
  uint32_t id;
  uint32_t *ids, *bif; //, *riv, *cm;

  ids = pworkspace->ids;      // id map for complete image
  //cm = pworkspace->comp_same; // Map connected ids on one representor
  //riv = pworkspace->real_ids_inv; // Map representor on output id
  bif = pworkspace->blob_id_filtered; //maps  'unfiltered id' on 'parent filtered id'
  if( bif == NULL ){
    fprintf(stderr, "(depthtree_get_filtered_id). 'blob_id_filtered' is NULL."
        "Call depthtree_filter_blobs(...) to initialize it.");
    return -1;
  }

  id = ids[ y*pworkspace->w + x ];
  return *(bif + id) - 1;
}

/* Postprocessing: Get filtered blob id for coordinate. Roi version */
uint32_t depthtree_get_filtered_id_roi(
    const Blobtree *blobs,
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    DepthtreeWorkspace *pworkspace
    )
{
  if( x < 0 || y < 0 || x >= roi.width || y >= roi.height ){
    fprintf(stderr, "(depthtree_get_filtered_id_roi) Requested pixel (%i,%i) not in roi (%i, %i, %i, %i).",
        x, y, roi.x, roi.y, roi.width, roi.height);
    return -1;
  }
  return depthtree_get_filtered_id( blobs, x + roi.x, y + roi.y, pworkspace);
}




#endif
