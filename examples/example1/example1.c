#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "threshtree.h"
#include "output_threshtree.h"

static const unsigned int W=28;
static const unsigned int H=28;

#include "../example.h"


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
  // Because it is not a [0,1]-Image, but a [0,200]-image we need a longer 
  // array for print_matrix_char_with_roi.
  char level_chars[201][5]; // 5 = 4 byte char + '\0'
  for( int i=0; i<200; ++i) { strcpy(level_chars[i],"░"); }
  strcpy(level_chars[200], "█");

  print_matrix_char_with_roi( sw, W, H, roi, 1, 1, level_chars, 201);

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
  print_coloured_threshtree_areas(sw, blobs, &roi, workspace, 0);

#if 1
  printf("Coloured map of connection component ids (last digit only and id(0)=' '):\n");
  char *ids_out = sprint_coloured_threshtree_ids(sw, blobs, &roi, workspace,
      0, 0, "0123456789");
  printf("%s\n", ids_out);
  free(ids_out);
#endif

#if 0
  // Print out result tree.
  printf("===========\n");
  printf("Treesize: %u, Tree null? %s\n", blobs->tree->size, blobs->tree->root==NULL?"Yes.":"No.");
  print_tree(blobs->tree->root, 0);
#endif

#if 1
  // Filter results
  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 2....\n");
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
#endif

  //printf("Print filtered coloured map of connection components:\n");
  //print_coloured_threshtree_areas(sw, blobs, &roi, workspace, 1);

  printf("Filtered Coloured map of blob ids (last digit only and blobid(0)=' '):\n");
  print_coloured_threshtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");
#endif

#if 0
  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 1...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 1);
  printf("Filtered Coloured map of blob ids (last digit only and blobid(0)=' '):\n");
  print_coloured_threshtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");

  printf("===========\n");
  printf("Restrict on blobs on tree level/depth 1 and 2...\n");
  blobtree_set_filter(blobs, F_CLEAR, 0);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MIN, 1);
  blobtree_set_filter(blobs, F_TREE_DEPTH_MAX, 2);
  printf("Filtered Coloured map of blob ids (last digit only and blobid(0)=' '):\n");
  print_coloured_threshtree_ids(sw, blobs, &roi, workspace, 1, 0, "0123456789");
#endif

  // Clean up.
  blobtree_destroy(&blobs);
  threshtree_destroy_workspace( &workspace );

  // Free image data
  free(sw);

  return 0;
}
