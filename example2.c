#include <stdlib.h>
#include <stdio.h>

#include "depthtree.h"
#include "depthtree_output.h"

static const unsigned int W=28;
static const unsigned int H=28;

#include "example.h"


int main(int argc, char **argv) {
  // Stepwidth
  unsigned int w=1,h=1;
  // Region of interest
  BlobtreeRect roi= {0,0,W,H};
  //BlobtreeRect roi= {18,0,9,17};

  if( argc > 2){
    w = atoi(argv[1]);
    h = atoi(argv[2]);
  }

  // Generate test image
  unsigned char* sw;
  sw = calloc( W*H,sizeof(unsigned char) );
  if( sw == NULL ) return -1;
  gen_image_data3(sw,W,H,4);

  //*(sw) = 3; // Test break of unique border.

  printf("Input image data:\n");
  print_matrix_char_with_roi( (char*) sw,W,H,roi, 1, 1);

  //Init workspace
  DepthtreeWorkspace *workspace = NULL;
  depthtree_create_workspace( W, H, &workspace );
  if( workspace == NULL ){
    printf("Unable to create workspace.\n");
    free(sw);
    return -1;
  }

  // Init blobtree struct
  Blobtree *blobs = NULL;
  blobtree_create(&blobs);

  // Set distance between compared pixels.
  // Look at blobtree.h for more information.
  blobtree_set_grid(blobs, w,h);


  //Init depth_map
  unsigned char depth_map[256];
  unsigned int i; for( i=0; i<256; i++) depth_map[i] = i;

  // Now search the blobs.
  depthtree_find_blobs(blobs, sw, W, H, roi, depth_map, workspace);

  // Print out result tree.
  printf("===========\n");
  printf("Treesize: %i, Tree null? %s\n", blobs->tree->size, blobs->tree->root==NULL?"Yes.":"No.");
  print_tree(blobs->tree->root, 0);


  printf("Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  print_coloured_depthtree_ids(sw, blobs, &roi, workspace,
      0, 0, "0123456789");

  // Filter results and loop over elements.
  printf("===========\n");
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_AREA_MIN, 5);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 1);

  Node *cur = blobtree_first(blobs);
  while( cur != NULL ){
    // bounding box
    Blob *data = (Blob*)cur->data;
    BlobtreeRect *rect = &data->roi;
    printf("Blob with id %i: x=%i y=%i w=%i h=%i area=%i\n",data->id,
        rect->x, rect->y,
        rect->width, rect->height,
        data->area
        );

    cur = blobtree_next(blobs);
  }

  // Clean up.
  depthtree_destroy_workspace( &workspace );

  blobtree_destroy(&blobs);

  // Free image data
  free(sw);

  return 0;
}
