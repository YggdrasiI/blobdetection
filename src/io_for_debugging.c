#include "tree.h"
#include "tree_intern.h"

// Debug/Helper-Functions

uint8_t * debug_getline(void) 
{
    uint8_t * line = (uint8_t*) malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int32_t c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin); // returns unsigned char casted to int
        if(c == EOF)
            break;

        if(--len == 0) {
            size_t dist = (line - linep);
            uint8_t * linen = (uint8_t*) realloc(linep, lenmax *= 2);
            len = lenmax;

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + dist;
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}


void debug_print_matrix(
    uint32_t* data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      d = *(data+i*w+j);
      //printf("%u ", d);
      //printf("%s", d==0?"■⬛":"□");
      //printf("%s", d==0?"✘":" ");
      if(d>0){
        printf("%3u", d);
      }else{
        printf("   ");
      }
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        d = *(data+i*w+j);
        if(d>0){
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        d = *(data+i*w+j);
        if(d>0){
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          d = *(data+i*w+j);
          if(d>0){
            printf("%3u", d);
          }else{
            printf("   ");
          }
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}


void debug_print_matrix2(
    uint32_t* ids,
    uint32_t* data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh,
    int8_t b_twice)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      if( *(ids+i*w+j) > 0 ){
        d = *(data+*(ids+i*w+j));
        if(b_twice) d=*(data+d);
        //printf("%s%u", d<10&&d>=0?" ":"", d);
        printf("%3u", d);
      }else{
        //printf("  ");
        printf("   ");
      }
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        if( *(ids+i*w+j) > 0 ){
          d = *(data+*(ids+i*w+j));
          if(b_twice) d=*(data+d);
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        if( *(ids+i*w+j) > 0 ){
          d = *(data+*(ids+i*w+j));
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          if( *(ids+i*w+j) > 0 ){
            d = *(data+*(ids+i*w+j));
            printf("%s%u", d<10&&d>=0?" ":"", d);
          }else{
            printf("  ");
          }
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}


void debug_print_matrix_char(
    uint8_t * data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      d = *(data+i*w+j);
      //printf("%u ", d);
      //printf("%s", d==0?"■⬛":"□");
      //printf("%s", d==0?"✘":" ");
      if(d>0)
        //printf("%s%u", d<10&&d>=0?" ":"", d);
        printf("%2u", d);
      else
        printf("  ");
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        d = *(data+i*w+j);
        if(d>0)
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%2u", d);
        else
          printf("  ");
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        d = *(data+i*w+j);
        if(d>0)
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%2u", d);
        else
          printf("  ");
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          d = *(data+i*w+j);
          if(d>0)
            //printf("%s%u", d<10&&d>=0?" ":"", d);
            printf("%2u", d);
          else
            printf("  ");
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}
