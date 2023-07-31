#ifndef SETTINGS_H
#define SETTINGS_H

// For compiling (of interfacse/examples) with
// Visual Studio C++ 2010 and above.
#ifdef _MSC_VER
#define VISUAL_STUDIO
#endif

/* 0, 1, 2(with stops) */
#ifndef VERBOSE
#define VERBOSE 0 
#endif

/* Sort tree by child node structure and area size. This provide 
   some robustness on rotation for tree comparison. */
//#define BLOB_SORT_TREE

/* Set this env. variable to enable diagonal checks between compared pixels. */
#ifndef NO_BLOB_DIAGONAL_CHECK
#ifndef BLOB_DIAGONAL_CHECK
#define BLOB_DIAGONAL_CHECK
#endif
#endif

/* Eval vertical and horizontal dimension */
#ifndef NO_BLOB_DIMENSION
#ifndef BLOB_DIMENSION
#define BLOB_DIMENSION
#endif
#endif

/* Extend blob dimension of parent blobs (for depthblob algorithm).
 * If darker pixels do not sourrounding lighter pixels the bounding box
 * of the darker layer id does not superseed the lighter layer bounding box.
 * Set this env. variable to checked and fix this mostly unwanted behaviour.
 * The used algorithm is non-recursive relating to the tree depth.
 */
#ifndef NO_EXTEND_BOUNDING_BOXES
#ifndef EXTEND_BOUNDING_BOXES
#define EXTEND_BOUNDING_BOXES
#endif
#endif

/* Count pixels of each area. (Stored in node.data.area).
 * This is more accurate then
 * area.width*area.height but not exact if stepwidth>1.
 * If stepwidth>1 and BLOB_DIMENSION is set,
 * the node.data.area value will be estimated
 * by the 'wetted area' of the bounding box and the
 * bounding boxes of children areas.
 */
#ifndef NO_BLOB_COUNT_PIXEL
#ifndef BLOB_COUNT_PIXEL
#define BLOB_COUNT_PIXEL
#endif
#endif

/* Evaluate the barycenter in both directions.
 *
 * Interally all pixel positions will be summed up
 * and finally divided by the area size. Thus, bigger images
 * requires the long long data type. 
 * If your images not superseeded 2000x2000 pixels you
 * are still on the secure side with unsigned long (32bit).
 *
 * Requires BLOB_COUNT_PIXEL.
 */
#ifdef BLOB_COUNT_PIXEL
#ifndef BLOB_BARYCENTER
//#define BLOB_BARYCENTER
#endif
#define BLOB_BARYCENTER_TYPE unsigned long
#else
#undef BLOB_BARYCENTER
#endif


/*
 * Expands bounding box evaluation on pixels between coarse grid.
 *
 * Otherwise the bounding box values are restricted 
 * on the coarse grid coordinates.
 * See README
 */
//#define BLOB_SUBGRID_CHECK 

/* For depthtree algorithm.
 * Use identity function to distict the pixel values into
 * different depth ranges. This saves a small amount of time,
 * but the result tree of blobs can be very big on inhomogenous
 * images.
 * */
//#define NO_DEPTH_MAP


/* For depthtree algorithm.
 * Save depth_map value for each node
 * (node.data.depth)
 * */
#define SAVE_DEPTH_MAP_VALUE


/* Force inlining of main function to create
 * constant stepwidth variable.
 * Increases compile time by factor 12. Comment out to disable optimization.
 * */
#ifndef FORCEINLINE
#ifdef VISUAL_STUDIO
#define FORCEINLINE __forceinline static
#else
#define FORCEINLINE __attribute__((always_inline)) static
#endif
#endif



#if VERBOSE > 0
#define VPRINTF(...) printf(__VA_ARGS__);
#else
#ifdef VISUAL_STUDIO  // No variadic template support in 2010
#define VPRINTF()
#else
#define VPRINTF(...)
#endif
#endif


#if VERBOSE > 0
#define VPRINTF(...) printf(__VA_ARGS__);
#else
#define VPRINTF(...) 
#endif

//=============================================================================
/* Derive some environment variables from the above settings.
 */

//#ifdef BLOB_DIMENSION ## BLOB_BARYCENTER //short version throws waring in gcc.
#ifdef BLOB_DIMENSION 
/* Update current position for each step */
  #define PIXEL_POSITION
#else
#ifdef BLOB_BARYCENTER
/* Update current position for each step */
  #define PIXEL_POSITION
#endif
#endif


#endif
