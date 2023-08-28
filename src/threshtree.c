#ifndef THRESHTREE_C
#define THRESHTREE_C

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> //for memset

#include <assert.h>

#include "blobdetection/threshtree.h"

#include "tree_intern.h"
#include "threshtree_macros.h"
//#include "threshtree_macros_old.h"


int32_t threshtree_create_workspace(
    const uint32_t w, const uint32_t h,
    ThreshtreeWorkspace **pworkspace
    ){

  if( *pworkspace != NULL ){
    //destroy old struct.
    threshtree_destroy_workspace( pworkspace );
  }
  //Now, *pworkspace is NULL

  if( w*h == 0 ) return -1;

  ThreshtreeWorkspace *r = malloc( sizeof(ThreshtreeWorkspace) );

  r->w = w;
  r->h = h;
  const uint32_t max_comp = (w+h)*100;
  r->max_comp = max_comp;
  r->used_comp = 0;
  if(
      ( r->ids = (uint32_t*) malloc( w*h*sizeof(uint32_t) ) ) == NULL ||
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
      0 ){
    // alloc failed
    threshtree_destroy_workspace( &r );
    return -1;
  }

#ifdef BLOB_SUBGRID_CHECK
  r->triangle = NULL;
  r->triangle_len = 0;
#endif

  r->real_ids = NULL;
  r->real_ids_inv = NULL;

  r->blob_id_filtered = NULL;

  *pworkspace=r;
  return 0;
}

int32_t threshtree_realloc_workspace(
    const uint32_t max_comp,
    ThreshtreeWorkspace **pworkspace
    ){

  ThreshtreeWorkspace *r = *pworkspace;
  r->max_comp = max_comp;
  if(
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
    threshtree_destroy_workspace( pworkspace );
    return -1;
  }

  free(r->blob_id_filtered);//omit unnessecary reallocation and omit wrong/low size
  r->blob_id_filtered = NULL;//should be allocated later if needed.

  return 0;
}

void threshtree_destroy_workspace(
    ThreshtreeWorkspace **pworkspace
    ){
  if( *pworkspace == NULL ) return;

  ThreshtreeWorkspace *r = *pworkspace ;
  free(r->ids);
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

#ifdef BLOB_SUBGRID_CHECK
  free(r->triangle);
  r->triangle = NULL;
  r->triangle_len = 0;
#endif

  free(r->real_ids);
  free(r->real_ids_inv);

  free(r->blob_id_filtered);

  free(r);
  *pworkspace = NULL;
}

Tree* find_connection_components(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const uint8_t thresh,
    Blob** tree_data,
    ThreshtreeWorkspace *workspace )
{
  const BlobtreeRect roi = {0,0,w,h};
  return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
}


Tree* find_connection_components_roi(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    Blob** tree_data,
    ThreshtreeWorkspace *workspace )
{
  return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
}

/* now include the old algo without subchecks ( find_connection_components_coarse ) */
//#include "threshtree_old.c"


#ifdef BLOB_SUBGRID_CHECK
#define stepheight stepwidth
Tree* find_connection_components_subcheck(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    Blob** tree_data,
    ThreshtreeWorkspace *workspace )
{
  /* With region of interrest (roi)
   * and fixed stepwidth.
   * A - stepwidth - B
   * |               |
   * |           stepwidth
   * |               |
   * C ------------- D
   * */
  /* Marks of ι Cases:
   *  x - stepwidth, y - stepwidth, swr - (w-1)%x, shr - (h-1)%y
   * <----------------------- w --------------------------------->
   * |        <-- roi.width ------------------------>
   * |        <- roi.width-stepwidth-swr -->
   * |
   * | | |    A ←x→ B ←x→ … B ←x→ C ←swr+stepwidth→ C
   * | | roi  ↑                   ↑                 ↑
   * h | hei  y                   y                 y
   * | r ght  ↓                   ↓                 ↓
   * | o -    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
   * | i ste  ↑                   ↑                 ↑
   * | . pwi  y                   y                 y
   * | h dth  ↓                   ↓                 ↓
   * | e -    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
   * | i shr  …                   …                 …
   * | g |    E ←x→ F ←x→ … F ←x→ G ←swr+stepwidth→ H
   * | h      ↑                   ↑                 ↑
   * | t      1                   1                 1
   * | |      ↓                   ↓                 ↓
   * h |      L ←x→ M ←x→ … M ←x→ N ←swr+stepwidth→ P
   * | |      ↑                   ↑                 ↑
   * | |      1                   1                 1
   * | |      ↓                   ↓                 ↓
   * h |      L ←x→ M ←x→ … M ←x→ N ←swr+stepwidth→ P
   * |
   * |
   * |
   *
   */
  if( stepwidth < 2){
    return find_connection_components_coarse(data,w,h,roi,thresh,1,1,tree_data, workspace);
  }

  //init
  uint32_t r=w-roi.x-roi.width; //right border
  uint32_t b=h-roi.y-roi.height; //bottom border
  if( r>(1<<16) || b>(1<<16) ){
    fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
    *tree_data = NULL;
    return NULL;
  }

  uint32_t swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
  uint32_t shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
  uint32_t sh = stepheight*w;
  uint32_t sh1 = (stepheight-1)*w;
  // uint32_t sh2 = shr*w;

#define DUMMY_ID -1 //id virtual parent of first element (id=0)
  uint32_t id=-1;//id for next component would be ++id
  uint32_t a1,a2; // for comparation of g(f(x))=a1,a2=g(f(y))
  uint32_t k; //loop variable

  /* Create pointer to workspace arrays */
  uint32_t max_comp = workspace->max_comp;

  uint32_t* ids = workspace->ids;

  uint32_t* comp_same = workspace->comp_same;
  uint32_t* prob_parent = workspace->prob_parent;
#ifdef BLOB_COUNT_PIXEL
  uint32_t* comp_size = workspace->comp_size;
#endif
#ifdef BLOB_DIMENSION
  uint32_t* top_index = workspace->top_index;
  uint32_t* left_index = workspace->left_index;
  uint32_t* right_index = workspace->right_index;
  uint32_t* bottom_index = workspace->bottom_index;
#endif
#ifdef BLOB_BARYCENTER
  BLOB_BARYCENTER_TYPE *pixel_sum_X = workspace->pixel_sum_X; 
  BLOB_BARYCENTER_TYPE *pixel_sum_Y = workspace->pixel_sum_Y; 
#endif
#ifdef PIXEL_POSITION
  uint32_t s=roi.x,z=roi.y; //s-spalte, z-zeile
#else
  const uint32_t s=0,z=0; //Should not be used.
#endif

  /* triangle array store information about the
   * evaluation of ids between the grid pixels.
   * This info is needed to avoid double evaluation
   * of ids for some pixels, which would be a critical problem.
   * Let sw = stepwidth, x%sw=0, y%sw=0.
   *
   * The image will divided into quads [x,x+sw) x (y-sw,y+sw]
   * Quads on right and bottom border will expand to image border.
   * Quads on the top border will notional shrinked to [x,x+sw) x (-1,0].
   *
   * 0 - Only the ids of the corners of a quad was evaluated
   * 1 - Ids for all Pixels over the anti-diagonal and the corners was evaluated.
   * 2 - All pixels of the quad was examined.
   *
   * */
  const uint32_t triwidth = (roi.width-1)/stepwidth + 1;
  const size_t triangle_len = (triwidth+1)* ( (roi.height-1)/stepheight + 1);
  if( triangle_len  > workspace->triangle_len ){
    free(workspace->triangle);
    workspace->triangle = (uint8_t*) malloc( triangle_len * sizeof(uint8_t) );
    if( workspace->triangle == NULL ){
      printf("(threshtree) Critical error: Mem allocation for triangle failed\n");
    }
    workspace->triangle_len = triangle_len;
  }

  uint8_t* const triangle = workspace->triangle;
  uint8_t* tri = triangle;
#if VERBOSE > 0
  printf("triwidth: %u\n", triwidth);
#endif

  const uint8_t* const dS = data+w*roi.y+roi.x;
  const uint8_t* dR = dS+roi.width; //Pointer to right border. Update on every line
  //uint8_t* dR2 = swr>0?dR-swr-stepwidth-stepwidth:dR; //cut last indizies.
  const uint8_t* dR2 = dR-swr-stepwidth; //cut last indizies.

  const uint8_t* const dE = dR + (roi.height-1)*w;
  const uint8_t* const dE2 = dE - shr*w;//remove last lines.

  //uint32_t i = w*roi.y+roi.x;
  const uint8_t* dPi = dS; // Pointer to data+i
  uint32_t* iPi = ids+(dS-data); // Poiner to ids+i

#if VERBOSE > 0
  //debug: prefill array
  printf("Note: Prefill ids array with 0. This will be removed for VERBOSE=0\n");
  memset(ids,0, w*h*sizeof(uint32_t));
#endif

  /**** A,A'-CASE *****/
  //top, left corner of BlobtreeRect get first id.
  NEW_COMPONENT(DUMMY_ID);
  BLOB_INC_COMP_SIZE( *iPi );
  BLOB_INC_BARY( *iPi );  
  iPi += stepwidth;
  dPi += stepwidth;

#ifdef PIXEL_POSITION
  s += stepwidth;
#endif
  ++tri;

  /* Split all logic to two subcases:
   * *(dPi)<=thresh (marked as B,C,…),
   * *(dPi)>thresh (marked as B',C',…)
   * to avoid many 'x&&y || x|y==0' checks.
   * */

  /* *tri beschreibt, was in der Zelle rechts davon/darüber passiert ist.*/

  //top border
  for( ;dPi<dR2; ){
    if( *(dPi) > thresh ){
      /**** B-CASE *****/
      if(*(dPi-stepwidth) > thresh ){//same component as left neighbour
        LEFT_CHECK(stepwidth);
        *(tri-1)=0;
      }else{//new component
        SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,0);
        *(tri-1)=2;
      }
    }else{
      /**** B'-CASE *****/
      if(*(dPi-stepwidth) <= thresh ){//same component as left neighbour
        LEFT_CHECK(stepwidth)
          *(tri-1)=0;
      }else{//new component
        SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,0);
        *(tri-1)=2;
      }
    }
    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
    iPi += stepwidth;
    dPi += stepwidth;
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif
    ++tri;
  }

  //now process last, bigger, cell. stepwidth+swr indizies left.
  SUBCHECK_ROW(dPi,iPi,stepwidth,w,sh,s,z,swr);
  *(tri-1)=2;
  *(tri) = 9;//Dummy, unused value.

  /* Pointer is swr behind currrent element.*/
  //BLOB_INC_COMP_SIZE; would increase wrong element, omit macro
#ifdef BLOB_COUNT_PIXEL
  *(comp_size+*(iPi-swr)) += 1;
#endif
#ifdef BLOB_BARYCENTER
  *(pixel_sum_X+*(iPi-swr)) += s;
  *(pixel_sum_Y+*(iPi-swr)) += z;
#endif

  /* Move pointer to 'next' row.*/
  dPi += r+roi.x+sh1+1;
  iPi += r+roi.x+sh1+1;

  dR += sh; // Move right border to next row.
  dR2 += sh;
#ifdef PIXEL_POSITION
  s=roi.x;
  z += stepheight;
#endif
  ++tri;

  //2nd,...,(h-shr)-row
  for( ; dPi<dE2 ; ){

    //left border
    /* 8 Cases for >thresh-Test of 3 positions
     *    a — b
     *    |
     *    x
     *
     *    The positions in '—' are already evaluated if a!=b (<=> *(tri-triwidth)==2 )
     * */
    {
      const uint8_t casenbr = ( *(dPi) > thresh )? \
                                    ( (( *(dPi-sh+stepwidth) <= thresh ) << 1)
                                      | (( *(dPi-sh) <= thresh ) << 0)): \
                                    ( (( *(dPi-sh+stepwidth) > thresh ) << 1)
                                      | (( *(dPi-sh) > thresh ) << 0));

      switch( casenbr ){
        case 0:{ /* no differences */
                 TOP_CHECK(stepheight, sh);
                 *(tri) = 0;
                 break;
               }
        case 1:
        case 2:{
                 SUBCHECK_PART2b(dPi,iPi,stepwidth,w,sh,s,z);
                 *(tri) = 1;
                 break;
               }
        case 3:{ /* a and b at same side of thresh, evaluate pixels between them*/
                 if( *(tri-triwidth)<3 ){ /* For second row value 2 is possible => do not use !=1.*/
                   SUBCHECK_PART1c(dPi,iPi,stepwidth,w,sh,s,z);
                 }
                 SUBCHECK_PART2b(dPi,iPi,stepwidth,w,sh,s,z);
                 *(tri) = 1;
                 break;
               }
      }
    }

    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
    iPi += stepwidth;
    dPi += stepwidth;
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif
    ++tri;

    /*inner elements till last colum before dR2 reached.
     * => Lefthand tests with -stepwidth
     *    Righthand tests with +stepwidth
     */
    for( ; dPi<dR2; ){

      /* Bit order:
       * 0 2 3
       * 1
       */
      /*const*/ uint8_t casenbr = ( *(dPi) > thresh )? \
                                        ( (( *(dPi-sh+stepwidth) <= thresh ) << 3)
                                          | (( *(dPi-sh) <= thresh ) << 2)
                                          | (( *(dPi-stepwidth) <= thresh ) << 1)
                                          | (( *(dPi-sh-stepwidth) <= thresh ) << 0)): \
                                        ( (( *(dPi-sh+stepwidth) > thresh ) << 3)
                                          | (( *(dPi-sh) > thresh ) << 2)
                                          | (( *(dPi-stepwidth) > thresh ) << 1)
                                          | (( *(dPi-sh-stepwidth) > thresh ) << 0));

#if VERBOSE > 1
      debug_print_matrix( ids, w, h, roi, 1, 1);
      printf("F, casenbr: %u, *tri: %u %u %u\n",casenbr, *(tri-1), *(tri-triwidth), *(tri-triwidth+1));
      free(debug_getline());
#endif
      if( dPi+stepwidth>=dR2 ){
        //set bit do avoid PART1c/PART1d calls for column of last loop step.
        //Could be unrolled...
        casenbr = casenbr | 0x08;
      }
      if( dPi+sh>=dE2 ){
        //set bit do force PART4 calls for this row.
        casenbr = casenbr | 0x02;
      }

      /* Legend:
       * X Current position of pointer
       * o Coarse grid positions
       * • Fine positions. Ids can already set or not ( information is encoded by casenbr )
       * ♣ Known ids, already set.
       * ♦ New id, set in this step. (Not set for -,| if already set)
       * */
      /* *(tri-2)=0: Positions
       *   -2   -1  0  <== tri-shift
       *  o   o•••o•••o       o   o♦♦♦o♦♦♦o       o    o•••o•••o
       *      •                   ♦♦♦♦♦♦♦♦             •       
       *      •           ==>     ♦♦♦♦♦♦♦     or       •       
       *      •                   ♦♦♦♦♦♦               •       
       *  o   o   X           o   o♦♦♦X           o    o   X   
       */
      /* *(tri-2)=1:
       *  o♣♣♣o•••o•••o       o♣♣♣o♦♦♦o♦♦♦o       o♣♣♣o•••o•••o
       *  ♣♣♣♣•               ♣♣♣♣♦♦♦♦♦♦♦♦        ♣♣♣♣•       
       *  ♣♣♣ •           ==> ♣♣♣ ♦♦♦♦♦♦♦   or    ♣♣♣ •        
       *  ♣♣  •               ♣♣  ♦♦♦♦♦♦          ♣♣  •        
       *  o   o   X           o   o♦♦♦X           o   o   X   
       */
      /* *(tri-2)=3:
       *  o♣♣♣o♣♣♣o•••o       o♣♣♣o♦♦♦o♦♦♦o       o♣♣♣o♣♣♣o•••o
       *  ♣♣♣♣♣♣♣♣            ♣♣♣♣♣♣♣♣♦♦♦♦        ♣♣♣♣♣♣♣♣   
       *  ♣♣♣♣♣♣♣         ==> ♣♣♣♣♣♣♣♦♦♦♦     or  ♣♣♣♣♣♣♣    
       *  ♣♣♣♣♣♣              ♣♣♣♣♣♣♦♦♦♦          ♣♣♣♣♣♣     
       *  o♣♣♣o   X           o♣♣♣o♦♦♦X           o   o   X 
       */
      /* ==> Wenn man die |-Stellen im Fall *(tri-2) nicht von den Dreieckswerten
       * links davor abhängig macht, sind Fall 0 und Fall 1. identisch zu behandeln.
       * In dem Fall kann man auch *(tri-1) für die Entscheidung heran ziehen.
       * Trickreich ist noch die Einbeziehung der tri-Werte der vorigen Reihe, was
       * nicht dargestellt ist.
       * */

#if VERBOSE > 1
      if( *(tri-1) > 1 ){
        printf("(threshtree) Logic error: tri>1 should not be possible here.\n");
      }
#endif

      if( *(tri-1) == 1 ){
        switch( casenbr) {
          case 0:
          case 8: {
                    TOP_CHECK(stepheight, sh);
                    TOP_LEFT_COMP(stepwidth);
                    *(tri) = 0; //tri filled, quad not.
                    break;
                  }
          case 1:{
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 2:{
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 3:{
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 4:{
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 5:{
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 6:{
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 7:{
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 9:{
                   SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 10:{
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 11:{
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 12:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 13:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 14:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 15:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3b(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
        }
      }else{
        switch( casenbr ){
          case 0:
          case 8: {
                    TOP_CHECK(stepheight, sh);
                    TOP_LEFT_COMP(stepwidth);
                    *(tri) = 0;
                    break;
                  }
          case 1:{
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 2:{
                   SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 3:{
                   SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 4:{
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 5:{
                   SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 6:{
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 7:{
                   SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 9:{
                   SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                   *(tri-1) = 3;
                   *(tri) = 1;
                   /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                   break;
                 }
          case 10:{
                    SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 11:{
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 12:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 13:{
                    SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    /*SUBCHECK_PART4b*/SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 14:{
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
          case 15:{
                    SUBCHECK_PART1ab(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART1cd(dPi,iPi,stepwidth,w,sh,s,z);
                    SUBCHECK_PART3a(dPi,iPi,stepwidth,w,sh,s,z);
                    *(tri-1) = 3;
                    *(tri) = 1;
                    SUBCHECK_PART4a(dPi,iPi,stepwidth,w,sh,s,z);
                    break;
                  }
        }

      }


      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
      dPi += stepwidth;
      iPi += stepwidth;
#ifdef PIXEL_POSITION
      s += stepwidth;
#endif
      ++tri;
    }

#if VERBOSE > 2
    debug_print_matrix( ids, w, h, roi, 1, 1);
    printf("G *tri: %u %u %u\n", *(tri-1), *(tri-triwidth), *(tri-triwidth+1));
    free(debug_getline());
#endif

    /* now process last, bigger, cell.
     * There exists only two cases:
     * triangle=0 or triangle=1. Top border was always evaluated
     * and bottom border should evaluated, too.
     */

    if( *(tri-1) == 1 ){
      SUBCHECK_PART6b(dPi,iPi,stepwidth,w,sh,s,z,swr);
    }else{
      SUBCHECK_PART6a(dPi,iPi,stepwidth,w,sh,s,z,swr);
    }
    *(tri-1) = 3;
    *(tri) = 3; //redundant

    /* Pointer is still on currrent element.*/
    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
    /* Move pointer to 'next' row.*/
    dPi += r+roi.x+sh1+swr+1;
    iPi += r+roi.x+sh1+swr+1;
    dR += sh; // Move border indizes to next row.
    dR2 += sh;
#ifdef PIXEL_POSITION
    s=roi.x;
    z += stepheight;
#endif
    ++tri;

#if VERBOSE > 2
    debug_print_matrix( ids, w, h, roi, 1, 1);
    printf("Z:%u, S:%u, I:%u %u\n",z,s,dPi-dS, iPi-ids-(dS-data) );
    free(debug_getline());
#endif

  } //row loop

  //correct pointer of last for loop step
  iPi -= sh1;
  dPi -= sh1;
  dR -= sh1;
  dR2 -= sh1;
#ifdef PIXEL_POSITION
  z -= stepheight-1;
#endif

#if VERBOSE > 1
  debug_print_matrix( ids, w, h, roi, 1, 1);
  printf("Z:%u, S:%u, I:%u %u\n",z,s,dPi-dS, iPi-ids-(dS-data) );
  free(debug_getline());
#endif

  if( dE2 != dE ){
    //Process elementwise till end of ROI reached.
    /* Note: This pixels are not influence the values of
     *  comp_size because it doesn't made sense to mix up
     *  the counting for coarse pixels and subgrid pixels.
     *  All pixels below dE2 are only elements of the fine grid.
     * */
    for( ; dPi<dE ; ){
      SUBCHECK_TOPDIAG(dPi,iPi,stepwidth,w,sh,s,z);
      ++dPi; ++iPi;
#ifdef PIXEL_POSITION
      ++s;
#endif
      for( ; dPi<dR-1 ; ){
        SUBCHECK_ALLDIR(dPi,iPi,stepwidth,w,sh,s,z);
        ++dPi; ++iPi;
#ifdef PIXEL_POSITION
        ++s;
#endif
      }
      //right border
      SUBCHECK_TOPLEFT(dPi,iPi,stepwidth,w,sh,s,z);
      //move pointer to 'next' row
      dPi += r+roi.x+1;
      iPi += r+roi.x+1;
#ifdef PIXEL_POSITION
      s = roi.x;
      ++z;
#endif
      dR += w;
      //dR2 += w; //not ness.

#if VERBOSE > 1
      debug_print_matrix( ids, w, h, roi, 1, 1);
      printf("Z:%u, S:%u, I:%u %u\n",z,s,dPi-dS, iPi-ids-(dS-data) );
      free(debug_getline());
#endif
    }
  }//end of if(dE2<dE)



  /* end of main algo */

#if VERBOSE > 0
  //printf("Matrix of ids:\n");
  //print_matrix(ids,w,h);

  //printf("comp_same:\n");
  //print_matrix(comp_same, id+1, 1);
  debug_print_matrix( ids, w, h, roi, 1, 1);
  debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, 0);
  if( stepwidth*stepheight >1 ){
    debug_print_matrix( ids, w, h, roi, stepwidth, stepheight);
    //printf("\n\n");
    debug_print_matrix2( ids, comp_same, w, h, roi, stepwidth, stepheight, 0);
  }
#endif

  /* Postprocessing.
   * Sum up all areas with connecteted ids.
   * Then create nodes and connect them.
   * If BLOB_DIMENSION is set, detect
   * extremal limits in [left|right|bottom]_index(*(real_ids+X)).
   * */
  uint32_t nids = id+1; //number of ids
  uint32_t tmp_id,/*tmp_id2,*/ real_ids_size=0,l;
  free(workspace->real_ids);
  workspace->real_ids = calloc( nids, sizeof(uint32_t) ); //store join of ids.
  uint32_t* const real_ids = workspace->real_ids;

  free(workspace->real_ids_inv);
  workspace->real_ids_inv = calloc( nids, sizeof(uint32_t) ); //store for every id with position in real_id link to it's position.
  uint32_t* const real_ids_inv = workspace->real_ids_inv;

#if 1
  for(k=0;k<nids;k++){

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

#ifdef BLOB_COUNT_PIXEL
      /* Elements without nodes on the coarse grid has area-value 0 (if the 
       * area value was evaluated). This elements are problematic because
       * this provoke the division by 0 due the barycenter evaluation.
       * Thus, it's probably the best decision
       * to ignore these nodes. */
      if( *(comp_size+tmp_id ) ){
        *(real_ids+real_ids_size) = tmp_id;
        *(real_ids_inv+tmp_id) = real_ids_size;//inverse function
        real_ids_size++;
      }
#else
      //Its a component id of a new area
      *(real_ids+real_ids_size) = tmp_id;
      *(real_ids_inv+tmp_id) = real_ids_size;//inverse function
      real_ids_size++;
#endif

    }

  }
#else
  /* Old approach: Attention, old version does not create
   * the projection property of comp_same (cs). Here, only cs^2=cs^3.
   */
  uint32_t found;
  for(k=0;k<nids;k++){
    tmp_id = k;
    tmp_id2 = *(comp_same+tmp_id);
#if VERBOSE > 0
    printf("%u: (%u->%u) ",k,tmp_id,tmp_id2);
#endif
    while( tmp_id2 != tmp_id ){
      tmp_id = tmp_id2;
      tmp_id2 = *(comp_same+tmp_id);
#if VERBOSE > 0
      printf("(%u->%u) ",tmp_id,tmp_id2);
#endif
    }
#if VERBOSE > 0
    printf("\n");
#endif

#ifdef BLOB_COUNT_PIXEL
    //move area size to other id.
    *(comp_size+tmp_id2) += *(comp_size+k);
    *(comp_size+k) = 0;
#endif

#ifdef BLOB_DIMENSION
    //update dimension
    if( *( top_index+tmp_id2 ) > *( top_index+k ) )
      *( top_index+tmp_id2 ) = *( top_index+k );
    if( *( left_index+tmp_id2 ) > *( left_index+k ) )
      *( left_index+tmp_id2 ) = *( left_index+k );
    if( *( right_index+tmp_id2 ) < *( right_index+k ) )
      *( right_index+tmp_id2 ) = *( right_index+k );
    if( *( bottom_index+tmp_id2 ) < *( bottom_index+k ) )
      *( bottom_index+tmp_id2 ) = *( bottom_index+k );
#endif

#ifdef BLOB_BARYCENTER
    //shift values to other id
    *(pixel_sum_X+tmp_id) += *(pixel_sum_X+k); 
    *(pixel_sum_X+k) = 0;
    *(pixel_sum_Y+tmp_id) += *(pixel_sum_Y+k); 
    *(pixel_sum_Y+k) = 0;
#endif

    //check if area id already identified as real id
    found = 0;
    for(l=0;l<real_ids_size;l++){
      if( *(real_ids+l) == tmp_id ){
        found = 1;
        break;
      }
    }
    if( !found ){
      *(real_ids+real_ids_size) = tmp_id;
      *(real_ids_inv+tmp_id) = real_ids_size;//inverse function
      real_ids_size++;
    }
  }
#endif


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

  Node *cur  = root;
  Blob *curdata  = blobs;

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
    curdata->depth_level = 0; /* ??? without anchor not trivial.*/
#endif

    tmp_id = *(prob_parent+rid); //get id of parent (or child) area.
    if( tmp_id == DUMMY_ID ){
      /* Use root as parent node. */
      //cur->parent = root;
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

  }


#ifdef BLOB_BARYCENTER
  blobtree_eval_barycenters(root, comp_size, pixel_sum_X, pixel_sum_Y);
#define SUM_AREAS_IS_REDUNDANT
#endif

  /* Evaluate exact areas of blobs for stepwidth==1
   * and try to approximate for stepwith>1. The
   * approximation requires a bounding box.
   * */
#ifdef BLOB_DIMENSION
#ifdef BLOB_COUNT_PIXEL
  if(stepwidth == 1){
#ifndef SUM_AREAS_IS_REDUNDANT
    sum_areas(root->child, comp_size);
#endif
  }else{
    approx_areas(tree, root->child, comp_size, stepwidth, stepheight);
    //replace estimation with exact value for full image area
    Blob* img = (Blob*)root->child->data;
    img->area = img->roi.width * img->roi.height;
  }
#else
  set_area_prop(root->child);
#endif
#else
#ifdef BLOB_COUNT_PIXEL
  if(stepwidth == 1){
#ifndef SUM_AREAS_IS_REDUNDANT
    sum_areas(root->child, comp_size);
#endif
  }else{
#ifndef SUM_AREAS_IS_REDUNDANT
    sum_areas(root->child, comp_size);
#endif
    //Be aware, this values scales by stepwidth.
    fprintf(stderr,"(threshtree) Warning: Eval areas for stepwidth>1.\n");
  }
#endif
#endif

#ifdef BLOB_SORT_TREE
  sort_tree(root);
#endif

  //current id indicates maximal used id in ids-array
  workspace->used_comp=id;

  //clean up
  free(tree_id_relation);
  //free(triangle);
  //free(anchors);
  return tree;
}

#undef stepheight
#endif // BLOB_SUBGRID_CHECK


void threshtree_find_blobs( Blobtree *blob,
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    ThreshtreeWorkspace *workspace )
{
  // Avoid hard crash for null data.
  if( data == NULL ){
    VPRINTF("Runtime error: Input data is NULL! threshtree_filter_blobs aborts.\n");
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
#ifdef BLOB_SUBGRID_CHECK
  blob->tree = find_connection_components_subcheck(
      data, w, h, roi, thresh,
      blob->grid.width,
      &blob->tree_data,
      workspace );
#else
  blob->tree = find_connection_components_coarse(
      data, w, h, roi, thresh,
      blob->grid.width, blob->grid.height,
      &blob->tree_data,
      workspace );
#endif
}


void threshtree_filter_blobs(
    Blobtree* blob,
    ThreshtreeWorkspace *pworkspace
    ){

  uint32_t numNodes = blob->tree->size;
  VPRINTF("Num nodes: %u\n", numNodes);

  if(pworkspace->blob_id_filtered==NULL){
    //Attention, correct size of blob_id_filtered is assumed if != NULL.
    //See workspace reallocation
    pworkspace->blob_id_filtered= malloc( pworkspace->max_comp*sizeof(uint32_t) );
  }
  uint32_t * const nodeToFilteredNode = calloc( numNodes,sizeof(uint32_t) );
  uint32_t * const blob_id_filtered = pworkspace->blob_id_filtered;
  const uint32_t * const comp_same = pworkspace->comp_same;
  const uint32_t * const real_ids_inv = pworkspace->real_ids_inv;

  if( nodeToFilteredNode != NULL && blob_id_filtered != NULL ){
    nodeToFilteredNode[0]=0;
    //nodeToFilteredNode[1]=1;

    /* 1. Map is identity on filtered nodes.
     * After this loop all other nodes will be still mapped to 0.
     * The ±1-shifts are caused by the dummy node on first position.
     * */
    const Node * const root = blob->tree->root;
    const Node *cur = blobtree_first(blob);
    while( cur != NULL ){
      //const uint32_t id = ((Blob*)cur->data)->id;
      //*(nodeToFilteredNode + node_id) = id;

      //const uint32_t node_id = *(pworkspace->real_ids_inv+id) + 1;
      const uint32_t node_id = cur-root;
      //note: Both definitions of node_id are equivalent.

      *(nodeToFilteredNode + node_id) = node_id;
      cur = blobtree_next(blob);
    }

    // 2. Take all nodes which are mapped to 0 and
    // search parent node with nonzero mapping.
    // Start for index=i=2 because first node is root.
    uint32_t pn, ri; //parent real id, read id of parent node
    for( ri=1; ri<numNodes; ri++){
      if( nodeToFilteredNode[ri] == 0 ){
        //find parent node of 'ri' which was not filtered out
        Node *pi = (blob->tree->root +ri)->parent;
        while( pi != NULL ){
          pn = nodeToFilteredNode[pi-root];
          if( pn != 0 ){
            nodeToFilteredNode[ri] = pn;
            break;
          }
          pi = pi->parent;
        }//if no matching element was founded, i mapping to root id (=0).
      }
    }

    /*3. Expand nodeToFilteredNode map information on all ids
     * 3a)  Use projection comp_same to map id
     *      on preimage of real_ids_inv. (=> id2)
     * 3b) Get node for id2. The dummy node produce +1 shift.
     * 3c) Finally, use nodeToFilteredNode map.
     */
    uint32_t id=pworkspace->used_comp;//dec till 0
    while( id ){
      *(blob_id_filtered+id) = *(nodeToFilteredNode + *(real_ids_inv + *(comp_same+id)) + 1 );
      //*(blob_id_filtered+id) = *(nodeToFilteredNode + *(real_ids_inv + *(comp_same+*(comp_same+id))) + 1 );
#if VERBOSE > 0
      printf("bif[%u] = %u, riv[%u]=%u\n",id, *(blob_id_filtered+id), id, *(real_ids_inv+id) );
#endif
      id--;
    }
    *(blob_id_filtered+id) = *(nodeToFilteredNode + *(real_ids_inv + *(comp_same+id)) + 1 );

#if VERBOSE > 0
    printf("nodeToFilteredNode[realid] = realid\n");
    for( ri=0; ri<numNodes; ri++){
      uint32_t id = ((Blob*)((blob->tree->root +ri)->data))->id;
      printf("id=%u, nodeToFilteredNode[%u] = %u\n",id, ri, nodeToFilteredNode[ri]);
    }
#endif

    free(nodeToFilteredNode);

  }else{
    printf("(threshtree_filter_blobs) Critical error: Mem allocation failed\n");
  }


}


uint32_t threshtree_get_id(
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    )
{
  uint32_t id;
  uint32_t *ids, *riv, *cm;

  ids = pworkspace->ids;      // id map for complete image
  cm = pworkspace->comp_same; // Map connected ids on one representor
  riv = pworkspace->real_ids_inv; // Map representor on output id (= blob id)

  id = ids[ y*pworkspace->w + x ];
  return *(riv + *(cm + id));
  //return *(cm + id);
}

uint32_t threshtree_get_id_roi(
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    )
{
  if( x < 0 || y < 0 || x >= roi.width || y >= roi.height ){
    fprintf(stderr, "(threshtree_get_id_roi) Requested pixel (%i,%i) not in roi (%i, %i, %i, %i).",
        x, y, roi.x, roi.y, roi.width, roi.height);
    return -1;
  }
  return threshtree_get_id(x + roi.x, y + roi.y, pworkspace);
}

uint32_t threshtree_get_filtered_id(
    const Blobtree *blobs,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    )
{
  uint32_t id;
  uint32_t *ids, *bif; //, *riv, *cm;

  ids = pworkspace->ids;      // id map for complete image
  bif = pworkspace->blob_id_filtered; //maps  'unfiltered id' on 'parent filtered id'
  //cm = pworkspace->comp_same; // Map connected ids on one representor
  //riv = pworkspace->real_ids_inv; // Map representor on output id
  if( bif == NULL ){
    fprintf(stderr, "(threshtree_get_filtered_id). 'blob_id_filtered' is NULL."
        "Call threshtree_filter_blobs(...) to initialize it.");
    return -1;
  }

  id = ids[ y*pworkspace->w + x ];
  return *(bif + id);
}

/* Postprocessing: Get filtered blob id for coordinate. Roi version */
uint32_t threshtree_get_filtered_id_roi(
    const Blobtree *blobs,
    const BlobtreeRect roi,
    const int32_t x, const int32_t y,
    ThreshtreeWorkspace *pworkspace
    )
{
  if( x < 0 || y < 0 || x >= roi.width || y >= roi.height ){
    fprintf(stderr, "(threshtree_get_filtered_id_roi) Requested pixel (%i,%i) not in roi (%i, %i, %i, %i).",
        x, y, roi.x, roi.y, roi.width, roi.height);
    return -1;
  }
  return threshtree_get_filtered_id( blobs, x + roi.x, y + roi.y, pworkspace);
}
#endif
