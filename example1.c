#include <stdlib.h>
#include <stdio.h>

#include "threshtree.h"
#include "output.h"

static const unsigned int W=28;
static const unsigned int H=28;

#include "example.h"


int main(int argc, char **argv) {
  // Stepwidth
  unsigned int gridW=1, gridH=1;
  // Region of interest
  BlobtreeRect roi= {0, 0, W, H};
  //BlobtreeRect roi= {18, 0, 9, 17};

  if( argc > 2){
      gridW = atoi(argv[1]);
      gridH = atoi(argv[2]);
  }

  // Generate test image
  unsigned char* sw;
  sw = calloc( W*H, sizeof(unsigned char) );
  if( sw == NULL ) return -1;
  gen_image_data2(sw, W, H, 4);

  printf("Input image data:\n");
  print_matrix_char_with_roi( (char*) sw, W, H, roi, 1, 1);

  //Init workspace
  ThreshtreeWorkspace *workspace = NULL;
  threshtree_create_workspace( W, H, &workspace );

  // Init blobtree struct
  Blobtree *blobs = NULL;
  blobtree_create(&blobs);

  // Set distance between compared pixels.
  // Look at blobtree.h for more information.
  blobtree_set_grid(blobs, gridW, gridH);

  // Now search the blobs.
  threshtree_find_blobs(blobs, sw, W, H, roi, 128, workspace);

  printf("Coloured map of connection components:\n");
  print_coloured_threshblob_areas(sw, blobs, &roi, workspace, 0);

  printf("Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  char *ids_out = sprint_coloured_threshblob_ids(sw, blobs, &roi, workspace,
      0, 0, "0123456789");
  printf("%s\n", ids_out);
  free(ids_out);

#if 1
  // Print out result tree.
  printf("===========\n");
  printf("Treesize: %u, Tree null? %s\n", blobs->tree->size, blobs->tree->root==NULL?"Yes.":"No.");
  print_tree(blobs->tree->root, 0);
#endif

  // Filter results
  printf("===========\n");
  printf("Restrict on blobs on three depth 2....\n");
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 2);
  //blobtree_set_filter(blobs, F_AREA_MIN, 0);
  //blobtree_set_filter(blobs, F_AREA_MAX, 30);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 2);

#if 1 
  // Loop over filtered elements.
  printf("List all blobs matching the filtering criterias:\n");
  Node *cur = blobtree_first(blobs);
  while( cur != NULL ){
    // bounding box
    Blob *data = (Blob*)cur->data;
    BlobtreeRect *rect = &data->roi;
    printf("Blob with id %u: x=%u y=%u w=%u h=%u area=%u\n", data->id, 
        rect->x, rect->y, 
        rect->width, rect->height, 
        data->area
        );

    cur = blobtree_next(blobs);
  }
#endif

  //printf("Print filtered coloured map of connection components:\n");
  //print_coloured_threshblob_areas(sw, blobs, &roi, workspace, 1);

  printf("Filtered Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  print_coloured_threshblob_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");


  printf("===========\n");
  printf("Restrict on blobs on three depth 1...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 1);
  printf("Filtered Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  print_coloured_threshblob_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");

  printf("===========\n");
  printf("Restrict on blobs on three depth 1 and 2...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 2);
  printf("Filtered Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  print_coloured_threshblob_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");

  // Clean up.
  blobtree_destroy(&blobs);
  threshtree_destroy_workspace( &workspace );

  // Free image data
  free(sw);

  return 0;
}
