#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "depthtree.h"
#include "output_depthtree.h"

static const uint32_t W=28;
static const uint32_t H=28;

#include "../example.h"


int32_t main(int32_t argc, char **argv) {
  // Stepwidth
  uint32_t w=1,h=1;
  // Region of interest
  BlobtreeRect roi= {0,0,W,H};
  //BlobtreeRect roi= {18,0,9,17};

  if( argc > 2){
    w = atoi(argv[1]);
    h = atoi(argv[2]);
  }

  // Generate test image
  uint8_t* sw;
  sw = calloc( W*H,sizeof(uint8_t) );
  if( sw == NULL ) return -1;

  // Image with 5 different levels 0,...,4
  gen_image_data3(sw,W,H,4);

  //*(sw) = 3; // Test break of unique border.

  // Visualisation of input data.
  const char level_strings[7+1][5] = {"░", "▒", "▓", "█", "■", "?", "▞", "▚"};
  printf("Input image data:\n");
  print_matrix_char_with_roi( sw,W,H,roi, 1, 1, level_strings, 5);

  printf("\t\t\t\tLegend: 0=%s, 1=%s, 2=%s, 3=%s, 4=%s\n",
          level_strings[0], level_strings[1], 
          level_strings[2], level_strings[3], 
          level_strings[4]); 

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
  uint8_t depth_map[256];
  uint32_t i; for( i=0; i<256; i++) depth_map[i] = i;

  // Now search the blobs.
  depthtree_find_blobs(blobs, sw, W, H, roi, depth_map, workspace);

  // Print out result tree.
  printf("===========\n");
  //printf("Allocated node size: %i, Tree null? %s\n", blobs->tree->size, blobs->tree->root==NULL?"Yes.":"No.");
  tree_print(blobs->tree, NULL, 0);

  blobtree_print(blobs, 0);


  printf("Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  print_coloured_depthtree_ids(sw, blobs, &roi, workspace,
      0, 0, "0123456789");

  // Filter results and loop over elements.
  printf("===========\n");
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_AREA_MIN, 5);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 1);

  // Loop over filtered elements.
  Node *cur = blobtree_first(blobs);
  while( cur != NULL ){
    // bounding box
    Blob *data = (Blob*)cur->data;
    BlobtreeRect *rect = &data->roi;
    printf("Blob with id %u: x=%u y=%u w=%u h=%u area=%u"
#ifdef BLOB_BARYCENTER
            " center=(%u,%u)"
#endif
            "\n",
            data->id,
            rect->x, rect->y,
            rect->width, rect->height,
            data->area
#ifdef BLOB_BARYCENTER
            , data->barycenter[0]
            , data->barycenter[1]
#endif
          );

    cur = blobtree_next(blobs);
  }

  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 1...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 1);
  printf("Filtered Coloured map of blob ids (last digit only and blobid(1)=' '):\n");
  print_coloured_depthtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");

#if 1 // More filtering examples
  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 2...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 2);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 2);
  printf("Filtered Coloured map of blob ids (last digit only and blobid(1)=' '):\n");
  print_coloured_depthtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");

  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 1 and 2...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 2);
  printf("Filtered Coloured map of blob ids (last digit only and blobid(1)=' '):\n");
  print_coloured_depthtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");
#endif

  tree_print_integrity_check(blobs->tree->root);

  // Clean up.
  depthtree_destroy_workspace( &workspace );

  blobtree_destroy(&blobs);

  // Free image data
  free(sw);

  return 0;
}
