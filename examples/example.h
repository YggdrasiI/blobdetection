#ifndef EXAMPLE_H
#define EXAMPLE_H

#include <stdint.h>
#include <time.h>


#if __has_include("settings.h")
#include "settings.h"
#else
#include "settings.default.h"
#endif

#ifdef VISUAL_STUDIO
int32_t random(){
    return rand();
}

#endif

#include "tree.h"
#include "blob.h"

static const char * c( uint8_t i){
  switch(i){
    case 0: return " ";
    case 1: return "□";
    case 2: return "■";
    case 3: return "⬛";
    case 4: return "█";
    case 5: return "░";
    case 6: return "✘";
    default: return " ";
  }
}

void print_matrix_with_roi( uint32_t* data, uint32_t w, uint32_t h, BlobtreeRect roi){
  uint32_t i,j,d;
  for(i=roi.y;i<roi.height;i++){
    for(j=roi.x;j<roi.width;j++){
      d = *(data+i*w+j);
      printf("%2u ",d);
    }
    printf("\n");
  }
  printf("\n");
}

void print_matrix( uint32_t* data, uint32_t w, uint32_t h){
  BlobtreeRect roi = {0,0,w,h};
  print_matrix_with_roi(data,w,h,roi);
}

void print_matrix_char_with_roi( uint8_t* data, uint32_t w, uint32_t h,
        BlobtreeRect roi, uint32_t gridw, uint32_t gridh,
        const char (*print_strings)[5], /* Array of length 'print_strings_len + 1' */
        size_t print_strings_len /*                                           ^^^^ */
        ){
  uint32_t i,j, wr, hr, w2, h2;
  uint32_t d;

#define DEFAULT_PRINT_STRINGS_LEN 2
  const char default_print_strings[DEFAULT_PRINT_STRINGS_LEN+1][5] = {"░", "█", "?"};
  if( print_strings == NULL || print_strings_len == 0 ){
      print_strings = default_print_strings;
      print_strings_len = DEFAULT_PRINT_STRINGS_LEN;
  }

  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y;i<roi.y+h2;i+=gridh){
    for(j=roi.x;j<roi.x+w2;j+=gridw){
      d = *(data+i*w+j);

      //printf("%u ",d);
      //printf("%s", d==0?"■⬛":"□");
      //printf("%s", d==0?"✘":" ");
      //printf("%s", c(d));
      //printf("%s", d!=0?"█":"░");
      if( d >= print_strings_len) {
          d = print_strings_len-1; // Non-representable data displayed with highest value. Default int: '?'
      }
      printf("%s", print_strings[d]);
    }
    j-=gridw-wr;

    if( wr>0 ){
      d = *(data+i*w+j);
      printf("%s", c(d));
    }
    printf("\n");
  }

  i-=gridh-hr;
  if( hr>0 ){
    for(j=roi.x;j<roi.x+w2;j+=gridw){
      d = *(data+i*w+j);
      printf("%s", c(d));
    }
    j-=gridw-wr;

    if( wr>0 ){
      d = *(data+i*w+j);
      printf("%s", c(d));
    }
    printf("\n");
  }
}

void print_matrix_char( uint8_t* data, uint32_t w, uint32_t h){
  BlobtreeRect roi = {0,0,w,h};
  print_matrix_char_with_roi(data,w,h,roi,1,1, NULL, 0);
}


#if OLD_STUFF 
void test(Node* root, uint32_t tree_size){
  /* Allocate space for 'name' of tree */
  uint32_t* tree_name = (uint32_t*) malloc( tree_size*sizeof(uint32_t) );

  tree_generate_id(root->child, tree_name, tree_size);

  uint32_t l;
  for( l=0;l<tree_size;l++)
    printf("%u ", *(tree_name+l) );
  printf("\n");

  free(tree_name);
}
#endif


void gen_image_data(uint8_t* sw, uint32_t w, uint32_t h){

  srand( time(NULL) );
  uint32_t wh=w*h,i;
  for( i=0;i<w+1;i++){
    *(sw+i) = 0;
    *(sw+wh-1-i) = 0;
  }

  for( i=w+1;i<wh-w-1;i++)
    if( i%w==0 || i%w==w-1)
      *(sw+i) = 0;
    else{
      *(sw+i) = random()%2;
    }
}

void gen_image_data2(uint8_t* sw, uint32_t w, uint32_t h, uint32_t depth){
  //srand( time(NULL) );
  uint32_t wh=w*h,i,j,k,l,d,col;
  uint32_t tx,ty;
  //B == Block
  uint32_t Bbreite = 2*depth+1;
  uint32_t Bw = w/Bbreite;
  uint32_t Bh = h/Bbreite;

  //durchlaufe alle Blöcke
  for( i=0;i<Bh;i++ ){
    for( j=0;j<Bw;j++ ){
      col = 1;
      for( d=0;d<depth+1;d++ ){
        col = 200*(random()%2); // (200 is over thresh in example1)
        //col = 1-col;
        //von außen nach innen einen Block mit col füllen
        for(k=d;k<Bbreite-d;k++){
          for(l=d;l<Bbreite-d;l++){
            tx = i*Bbreite + k;
            ty = j*Bbreite + l;
            *(sw+ tx*h +ty) = col;
          }
        }
      }
    }
  }

  //Rand auf Null
  for( i=0;i<w+1;i++){
    *(sw+i) = 0;
    *(sw+wh-1-i) = 0;
  }
  for( i=w+1;i<wh-w-1;i++) if( i%w==0 || i%w==w-1) *(sw+i) = 0;
}

void gen_image_data3(uint8_t* sw, uint32_t w, uint32_t h, uint32_t depth){
  //srand( time(NULL) );
  uint32_t wh=w*h,i,j,k,l,d,col;
  uint32_t tx,ty;
  //B == Block
  uint32_t Bbreite = 2*depth+1;
  uint32_t Bw = w/Bbreite;
  uint32_t Bh = h/Bbreite;

  //durchlaufe alle Blöcke
  for( i=0;i<Bh;i++ ){
    for( j=0;j<Bw;j++ ){
      col = 1;
      for( d=0;d<depth+1;d++ ){
        col = (random()%5);
        //col = 1-col;
        //von außen nach innen einen Block mit col füllen
        for(k=d;k<Bbreite-d;k++){
          for(l=d;l<Bbreite-d;l++){
            tx = i*Bbreite + k;
            ty = j*Bbreite + l;
            *(sw+ tx*h +ty) = col;
          }
        }
      }
    }
  }

  //Rand auf Null
  for( i=0;i<w+1;i++){
    *(sw+i) = 0;
    *(sw+wh-1-i) = 0;
  }
  for( i=w+1;i<wh-w-1;i++) if( i%w==0 || i%w==w-1) *(sw+i) = 0;
}


#endif
