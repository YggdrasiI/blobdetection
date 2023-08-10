
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> //for memset

#include "threshtree.h"
#include "threshtree_old.h"

#include "threshtree_macros.h"
#include "threshtree_macros_old.h"

Tree* find_connection_components_coarse(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    const uint32_t stepheight,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace )
{
  if( stepwidth == 1 && stepheight == 1 ){
    return _find_connection_components_coarse(data,w,h,roi,thresh,
        1,1,
        tree_data, workspace);
  }else{
    return _find_connection_components_coarse(data,w,h,roi,thresh,
        stepwidth,stepheight
        ,tree_data, workspace);
  }
}


/* Let compiler optimize code for fixed stepwidth and stepheight by inlining*/
FORCEINLINE
Tree* _find_connection_components_coarse(
    const uint8_t *data,
    const uint32_t w, const uint32_t h,
    const BlobtreeRect roi,
    const uint8_t thresh,
    const uint32_t stepwidth,
    const uint32_t stepheight,
    Blob **tree_data,
    ThreshtreeWorkspace *workspace )
{
  /* Marks of 10 Cases:
   *  x - stepwidth, y - stepheigt, swr - (w-1)%x, shr - (h-1)%y
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
  uint32_t r=w-roi.x-roi.width; //right border
  uint32_t b=h-roi.y-roi.height; //bottom border
  if( r<0 || b<0 ){
    fprintf(stderr,"[blob.c] BlobtreeRect not matching.\n");
    *tree_data = NULL;
    return NULL;
  }

  uint32_t swr = (roi.width-1)%stepwidth; // remainder of width/stepwidth;
  uint32_t shr = (roi.height-1)%stepheight; // remainder of height/stepheight;
  uint32_t sh = stepheight*w;
  uint32_t sh1 = (stepheight-1)*w;
  uint32_t sh2 = shr*w;

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

  const uint8_t* const dS = data+w*roi.y+roi.x;
  const uint8_t* dR = dS+roi.width; //Pointer to right border. Update on every line
  const uint8_t* dR2 = dR-swr; //cut last indizies.

  const uint8_t* const dE = dR + (roi.height-1)*w;
  const uint8_t* const dE2 = dE - sh2;//remove last lines.

  //uint32_t i = w*roi.y+roi.x;
  const uint8_t* dPi = dS; // Pointer to data+i 
  uint32_t* iPi = ids+(dS-data); // Poiner to ids+i

  /**** A,A'-CASE *****/
  //top, left corner of BlobtreeRect get first id.
  NEW_COMPONENT_OLD(DUMMY_ID);
  BLOB_INC_COMP_SIZE( *iPi );
  BLOB_INC_BARY( *iPi );  
  iPi += stepwidth;
  dPi += stepwidth;

#ifdef PIXEL_POSITION
  s += stepwidth;
#endif

  /* Split all logic to two subcases:
   * *(dPi)<=thresh (marked as B,C,…),
   * *(dpi)>thresh (marked as B',C',…)
   * to avoid many 'x&&y || x|y==0' checks.
   * */

  //top border
  for(;dPi<dR2;iPi += stepwidth,dPi += stepwidth){
    if( *(dPi) > thresh ){
      /**** B-CASE *****/
      if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
        LEFT_CHECK(stepwidth)
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-stepwidth) )
      }
    }else{
      /**** B'-CASE *****/
      if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
        LEFT_CHECK(stepwidth)
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-stepwidth) )
      }
    }
    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif
  }

  //correct pointer shift of last for loop step.
  iPi -= stepwidth-swr;
  dPi -= stepwidth-swr;
#ifdef PIXEL_POSITION
  s -= stepwidth-swr;
#endif

  //continue with +swr stepwidth
  if(swr){
    if( *(dPi) > thresh ){
      /**** C-CASE *****/
      if( *(dPi-swr) > thresh ){//same component as left neighbour
        LEFT_CHECK(swr)
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-swr) )
      }
    }else{
      /**** C'-CASE *****/
      if( *(dPi-swr) <= thresh ){//same component as left neighbour
        LEFT_CHECK(swr)
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-swr) )
      }
    }
    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
  }

  //move pointer to 'next' row
  dPi += r+roi.x+sh1+1;
  iPi += r+roi.x+sh1+1;
  dR += sh; //rechter Randindex wird in nächste Zeile geschoben.
  dR2 += sh;
#ifdef PIXEL_POSITION
  s=roi.x;
  z += stepheight;
#endif  

  //2nd,...,(h-shr)-row
  for( ; dPi<dE2 ; dPi += r+roi.x+sh1+1,iPi += r+roi.x+sh1+1, dR += sh, dR2 += sh ){

    //left border
    if( *(dPi) > thresh ){
      /**** E-CASE *****/
      if( *(dPi-sh) > thresh ){//same component as top neighbour
        TOP_CHECK(stepheight, sh)
#ifdef BLOB_DIAGONAL_CHECK
      }else if( *(dPi-sh+stepwidth) > thresh ){//same component as diagonal neighbour
        DIAG_CHECK(stepwidth, stepheight, sh)
#endif
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-sh) )
      }
    }else{
      /**** E'-CASE *****/
      if( *(dPi-sh) <= thresh){//same component as top neighbour
        TOP_CHECK(stepheight, sh)
#ifdef BLOB_DIAGONAL_CHECK
      }else if( *(dPi-sh+stepwidth) <= thresh ){//same component as diagonal neighbour
        DIAG_CHECK(stepwidth, stepheight, sh)
#endif
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-sh) )
      }
    }

    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
    iPi += stepwidth;
    dPi += stepwidth;
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif

    /*inner elements till last colum before dR2 reached.
     * => Lefthand tests with -stepwidth
     *     Righthand tests with +stepwidth
     */
    for( ; dPi<dR2-stepwidth; iPi += stepwidth, dPi += stepwidth ){
      if( *(dPi) > thresh ){
        /**** F-CASE *****/
        if( *(dPi-sh) > thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh+stepwidth) > thresh ){
              LEFT_DIAG_COMP(stepwidth,sh)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh+stepwidth) > thresh ){
              ANTI_DIAG_COMP(stepwidth,sh)
            }
        }else if( *(dPi-sh+stepwidth) > thresh ){//same component as diagonal neighbour
          DIAG_CHECK(stepwidth, stepheight, sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** F'-CASE *****/
        if( *(dPi-sh) <= thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh+stepwidth) <= thresh ){
              LEFT_DIAG_COMP(stepwidth,sh)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh+stepwidth) <= thresh ){
              ANTI_DIAG_COMP(stepwidth,sh)
            }
        }else if( *(dPi-sh+stepwidth) <= thresh ){//same component as diagonal neighbour
          DIAG_CHECK(stepwidth, stepheight, sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }
      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
      s += stepwidth;
#endif
    }


    /* If dR2==dR, then the last column is reached. Thus it's not possibe
     * to check diagonal element. (only G case)
     * Otherwise use G and H cases.
     */
    if( dR2==dR ){
      if( *(dPi) > thresh ){
        /**** G-CASE *****/
        if( *(dPi-sh) > thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
#endif
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** G'-CASE *****/
        if( *(dPi-sh) <= thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
#endif
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }
      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  

    }else{
      //structure: (dPi-stepwidth),(dPi),(dPi+swr)
      if( *(dPi) > thresh ){
        /**** G-CASE *****/
        if( *(dPi-sh) > thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh+swr) > thresh ){
              LEFT_DIAG_COMP(swr,sh)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh+swr) > thresh ){
              ANTI_DIAG_COMP(swr,sh)
            }
        }else if( *(dPi-sh+swr) > thresh ){//same component as diagonal neighbour
          DIAG_CHECK(swr,stepheight,sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** G'-CASE *****/
        if( *(dPi-sh) <= thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh+swr) <= thresh ){
              LEFT_DIAG_COMP(swr,sh)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, stepheight, sh)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh+swr) <= thresh ){
              ANTI_DIAG_COMP(swr,sh)
            }
        }else if( *(dPi-sh+swr) <= thresh ){//same component as diagonal neighbour
          DIAG_CHECK(swr,stepheight,sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }
      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
      s+=swr;
#endif
      iPi+=swr;
      dPi+=swr;

      //right border, not check diag element
      if( *(dPi) > thresh ){
        /**** H-CASE *****/
        if( *(dPi-sh) > thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-swr) > thresh ){
              TOP_LEFT_COMP(swr);
            }
        }else  if( *(dPi-swr) > thresh ){//same component as left neighbour
          LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-swr) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(swr, stepheight, sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** H'-CASE *****/
        if( *(dPi-sh) <= thresh ){//same component as top neighbour
          TOP_CHECK(stepheight, sh)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-swr) <= thresh ){
              TOP_LEFT_COMP(swr);
            }
        }else  if( *(dPi-swr) <= thresh ){//same component as left neighbour
          LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh-swr) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(swr, stepheight, sh)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }
      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
    }//end of else case of (dR2==dR)

#ifdef PIXEL_POSITION
    s=roi.x;
    z += stepheight;
#endif

  } //row for loop
  iPi -= sh-sh2;//(stepheight-1)*w;
  dPi -= sh-sh2;//(stepheight-1)*w;
  dR -= sh-sh2;
  dR2 -= sh-sh2;
#ifdef PIXEL_POSITION
  z -= stepheight-shr;
#endif

  if(dE2<dE){

    //left border
    if( *(dPi) > thresh ){
      /**** L-CASE *****/
      if( *(dPi-sh2) > thresh ){//same component as top neighbour
        TOP_CHECK(shr, sh2)
#ifdef BLOB_DIAGONAL_CHECK
      }else if( *(dPi-sh2+stepwidth) > thresh ){//same component as diagonal neighbour
        DIAG_CHECK(stepwidth, shr, sh2)
#endif
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-sh2) )
      }
    }else{
      /**** L'-CASE *****/
      if( *(dPi-sh2) <= thresh ){//same component as top neighbour
        TOP_CHECK(shr, sh2)
#ifdef BLOB_DIAGONAL_CHECK
      }else if( *(dPi-sh2+stepwidth) <= thresh ){//same component as diagonal neighbour
        DIAG_CHECK(stepwidth, shr, sh2)
#endif
      }else{//new component
        NEW_COMPONENT_OLD( *(iPi-sh2) )
      }
    }

    BLOB_INC_COMP_SIZE( *iPi );
    BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
    s += stepwidth;
#endif
    iPi += stepwidth;
    dPi += stepwidth;

    /*inner elements till last colum before dR2 reached.
     * => Lefthand tests with -stepwidth
     *     Righthand tests with +stepwidth
     */
    for( ; dPi<dR2-stepwidth; iPi += stepwidth, dPi += stepwidth ){
      if( *(dPi) > thresh ){
        /**** M-CASE *****/
        if( *(dPi-sh2) > thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh2+stepwidth) > thresh ){
              LEFT_DIAG_COMP(stepwidth,sh2)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh2+stepwidth) > thresh ){
              ANTI_DIAG_COMP(stepwidth,sh2)
            }
        }else if( *(dPi-sh2+stepwidth) > thresh ){//same component as diagonal neighbour
          DIAG_CHECK(stepwidth, shr, sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** M'-CASE *****/
        if( *(dPi-sh2) <= thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh2+stepwidth) <= thresh ){
              LEFT_DIAG_COMP(stepwidth,sh2)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh2+stepwidth) <= thresh ){
              ANTI_DIAG_COMP(stepwidth,sh2)
            }
        }else if( *(dPi-sh2+stepwidth) <= thresh ){//same component as diagonal neighbour
          DIAG_CHECK(stepwidth, shr, sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }

      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
      s += stepwidth;
#endif
    }

    /* If dR2==dR, then the last column is reached. Thus it's not possibe
     * to check diagonal element. (only N case)
     * Otherwise use N and P cases.
     */
    if( dR2==dR ){
      if( *(dPi) > thresh ){
        /**** N-CASE *****/
        if( *(dPi-sh2) > thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
#endif
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** N'-CASE *****/
        if( *(dPi-sh2) <= thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
#endif
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }
      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  

    }else{
      //structure: (dPi-stepwidth),(dPi),(dPi+swr)
      if( *(dPi) > thresh ){
        /**** N-CASE *****/
        if( *(dPi-sh2) > thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) > thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) > thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh2+swr) > thresh ){
              LEFT_DIAG_COMP(swr,sh2)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh2+swr) > thresh ){
              ANTI_DIAG_COMP(swr,sh2)
            }
        }else if( *(dPi-sh2+swr) > thresh ){//same component as diagonal neighbour
          DIAG_CHECK(swr,shr,sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** N'-CASE *****/
        if( *(dPi-sh2) <= thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-stepwidth) <= thresh ){
              TOP_LEFT_COMP(stepwidth);
            }
        }else  if( *(dPi-stepwidth) <= thresh ){//same component as left neighbour
          LEFT_CHECK(stepwidth)
#ifdef BLOB_DIAGONAL_CHECK
            /* check if diag neighbour id can associate with left neighbour id.*/
            if( *(dPi-sh2+swr) <= thresh ){
              LEFT_DIAG_COMP(swr,sh2)
            }
#endif
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-stepwidth) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(stepwidth, shr, sh2)
            /* check if diagonal neighbour id can associate with anti diagonal neigbour id.*/
            if( *(dPi-sh2+swr) <= thresh ){
              ANTI_DIAG_COMP(swr,sh2)
            }
        }else if( *(dPi-sh2+swr) <= thresh ){//same component as diagonal neighbour
          DIAG_CHECK(swr,shr,sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }

      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
#ifdef PIXEL_POSITION
      s+=swr;
#endif
      iPi+=swr;
      dPi+=swr;

      //right border, not check diag element
      if( *(dPi) > thresh ){
        /**** P-CASE *****/
        if( *(dPi-sh2) > thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-swr) > thresh ){
              TOP_LEFT_COMP(swr);
            }
        }else  if( *(dPi-swr) > thresh ){//same component as left neighbour
          LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-swr) > thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(swr, shr, sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }else{
        /**** P'-CASE *****/
        if( *(dPi-sh2) <= thresh ){//same component as top neighbour
          TOP_CHECK(shr, sh2)
            // check if left neighbour id can associate with top neigbour id.
            if( *(dPi-swr) <= thresh ){
              TOP_LEFT_COMP(swr);
            }
        }else  if( *(dPi-swr) <= thresh ){//same component as left neighbour
          LEFT_CHECK(swr)
#ifdef BLOB_DIAGONAL_CHECK
        }else  if( *(dPi-sh2-swr) <= thresh ){//same component as anti diagonal neighbour
          ANTI_DIAG_CHECK(swr, shr, sh2)
#endif        
        }else{//new component
          NEW_COMPONENT_OLD( *(iPi-stepwidth) )
        }
      }

      BLOB_INC_COMP_SIZE( *iPi );
      BLOB_INC_BARY( *iPi );  
    }//end of else case of (dR2==dR)

  } //end of if(dE2==dE)

  /* end of main algo */

#if VERBOSE > 0
  //printf("Matrix of ids:\n");
  //print_matrix(ids,w,h);

  //printf("comp_same:\n");
  //print_matrix(comp_same, id+1, 1);
  debug_print_matrix( ids, w, h, roi, 1, 1);
  debug_print_matrix2( ids, comp_same, w, h, roi, 1, 1, 0);
#endif

  /* Postprocessing.
   * Sum up all areas with connecteted ids.
   * Then create nodes and connect them.
   * If BLOB_DIMENSION is set, detect
   * extremal limits in [left|right|bottom]_index(*(real_ids+X)).
   * */
  uint32_t nids = id+1; //number of ids
  uint32_t tmp_id,real_ids_size=0,l;

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
    printf("%i: (%i->%i ",k,k,tmp_id);
#endif
    if( tmp_id != k ){
      tmp_id = *(comp_same+tmp_id);
      *(comp_same+k) = tmp_id;
#if VERBOSE > 0
      printf("->%i ",tmp_id);
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
#else
  /* Old approach: Attention, old version does not create
   * the projection property of comp_same (cs). Here, only cs^2=cs^3.
   */
  uint32_t tmp_id2;
  uint32_t found;
  for(k=0;k<nids;k++){
    tmp_id = k;
    tmp_id2 = *(comp_same+tmp_id);
#if VERBOSE > 0
    printf("%i: (%i->%i) ",k,tmp_id,tmp_id2);
#endif
    while( tmp_id2 != tmp_id ){

#ifdef BLOB_COUNT_PIXEL
      //move area size to other id.
      *(comp_size+tmp_id2) += *(comp_size+tmp_id);
      *(comp_size+tmp_id) = 0;
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

      tmp_id = tmp_id2;
      tmp_id2 = *(comp_same+tmp_id);

#if VERBOSE > 0
      printf("(%i->%i) ",tmp_id,tmp_id2);
#endif
    }
#if VERBOSE > 0
    printf("\n");
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
  *tree_data = malloc( tree->size * sizeof(Blob) );
  Node * const root = tree->root;

  //init all node as leafs
  for(l=0;l<real_ids_size+1;l++) *(root+l)=Leaf;

  //set root node (the desired output are the child(ren) of this node.)
  Node *cur  = root;
  Blob *curdata  = *tree_data;

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
    curdata->id = rid;  //Set id of this blob.
    //not useful?!
    //uint32_t anchor = *(anchors+*(real_ids+l)); //get anchor of this blob
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
      //cur->parent = root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id );
      tree_add_child( root + 1/*root pos shift*/ + *(real_ids_inv+tmp_id ),
          cur );
    }

  }

  /*
   *
   */
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
    blobtree_approx_areas(tree, root->child, comp_size, stepwidth, stepheight);
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
    sum_areas(root->child, comp_size);
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

  //set output parameter
  return tree;
}
