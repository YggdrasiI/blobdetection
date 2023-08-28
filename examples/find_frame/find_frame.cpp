#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include <cstdint>
#include <cstdio>

#include "../helpers.hpp"


#include "blobdetection/threshtree.h"
#include "blobdetection/depthtree.h"

//using namespace cv;
using std::cout;
const int32_t alpha_slider_max(100);
const std::string win_name("Find frame algorithm");
const std::string trackbar_name("Unused");
int32_t alpha_slider;
double alpha;
double beta;
cv::Mat src1;     // CV_8UC1
cv::Mat area_ids; // CV_16UC1 // Id's of depthtree algorithm (stage 1)
cv::Mat area_ids_coloured; // CV_8UC3 // Better human readable representation of ids
cv::Mat stage2;  // modificated src1
cv::Mat stage3;  // TODO
cv::Mat dst;     // CV_8UC3 Printed on screen


static void on_trackbar( int32_t, void* )
{
	alpha_slider = cv::getTrackbarPos(trackbar_name, win_name);
 alpha = (double) alpha_slider/alpha_slider_max ;
 beta = ( 1.0 - alpha );
 //addWeighted( src1, alpha, stage2, beta, 0.0, dst);
 //cv::Mat tmp = cv::Scalar(255, 255, 255);
 //cv::scaleAdd(src1, -1, tmp, dst);
 cv::imshow(win_name, dst );
}

//static void on_click( int32_t, void* ){
//}

/* Store blob id of each pixel in grayscale (16 bit) image  */
static void	img_of_blob_ids(
		uint32_t W, uint32_t H, DepthtreeWorkspace *workspace,
		cv::Mat &out)
{
	out.create(cv::Size(W,H),CV_16UC1);

	for( uint32_t y=0; y<H; ++y){
		for( uint32_t x=0; x<W; ++x) {
			int32_t id;
			// id = depthtree_get_filtered_id_roi(blobs, roi, x, y, workspace);
			id = depthtree_get_id(x, y, workspace); 
			//out.at<cv::CV_16UC1>(y,x)[0] = id;
			out.at<ushort>(y,x) = id;
		}
	}
}

// Function handle
typedef void ID_TO_BGR_MAP(int32_t id, cv::Vec3b &out);

void id_to_bgr_map(int32_t id, cv::Vec3b &out){
	++id;
	out[0] = id*111234 % 255;
	out[1] = id*920248 % 255;
	out[2] = id*788239 % 255;
}
void id_to_bgr_map2(int32_t id, cv::Vec3b &out){
	++id;
	out[0] = id*10 % 255;
	out[1] = id*10 % 255;
	out[2] = id*10 % 200;
}

/* Store blob id of each pixel in bgr image  */
static void	img_of_blob_ids_coloured(
		ID_TO_BGR_MAP *map,
		uint32_t W, uint32_t H, DepthtreeWorkspace *workspace,
		cv::Mat &out)
{
	out.create(cv::Size(W,H),CV_8UC3);

	for( uint32_t y=0; y<H; ++y){
		for( uint32_t x=0; x<W; ++x) {
			int32_t id;
			id = depthtree_get_id(x, y, workspace);
			map(id, out.at<cv::Vec3b>(y,x));
		}
	}
}


static void stage2_remove_id(Blobtree *blobs, int32_t x, int32_t y)
{
		uint8_t color_on_edge = stage2.at<uchar>(0,x);
		if (color_on_edge == 0) {
			// Its a background pixel or its id was already removed
			// in previous loop step
			return;
		}

		unsigned short id_on_edge = area_ids.at<ushort>(0,x);

		/* Hm, brauche ich erst in stage3! */
		//cv::Mat id_mask = (area_ids == id_on_edge);
		//uint8_t minimal_color_in_area = min(stage2, id_mask);

		//Node *root = blobs->tree->root;
		Node *node = blobtree_find_id(blobs, id_on_edge, 0);
		assert( node != NULL);
		while( node != NULL ) {
			auto id = ((Blob*)(node->data))->id;
			if( id == -1U ){
				assert(node == blobs->tree->root);
				// Sometimes root node is a dummy and everything is done at its childs.
				// TODO: Check if this catch is still needed.
				break;
			}
			cout << "Handle id " << id << std::endl;
			cv::Mat id_mask_inv = (area_ids != id);

			cv::Mat xxx;
			id_mask_inv.convertTo(xxx, CV_8UC1, 255.0, 0.0);
			std::string sm(string_format("Mask_id%d.png", id));
			cv::imwrite(sm, xxx );

			cv::bitwise_and(stage2, cv::Scalar(0), stage2, id_mask_inv);
			//stage2 &= xxx; // wrong

			std::string ss(string_format("Stage2_id%d.png", id));
			cv::imwrite(ss, stage2 );

			// Go upwards
			node = node->parent;
		}
}


static int32_t foobar() {
	uint32_t W=src1.size().width;
	uint32_t H=src1.size().height;

  // Stepwidth
  uint32_t w=1, h=1;
  // Region of interest
  BlobtreeRect roi= {0, 0, W, H};


	// Fetch data array
	cv::Mat depth;
	cv::cvtColor(src1, depth, cv::COLOR_BGR2GRAY);
  //cvtColor(src1, depth, CV_8U);

  uint8_t* sw = (uint8_t *)depth.data;

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
  uint32_t i; for( i=0; i<256; i++) depth_map[i] = i/5;

	// Stage 1
  // Now search the blobs.
  depthtree_find_blobs(blobs, sw, W, H, roi, depth_map, workspace);

#if 1 // Debug stuff
  //printf("Allocated node size: %i, Tree null? %s\n", blobs->tree->size, blobs->tree->root==NULL?"Yes.":"No.");
  tree_print(blobs->tree, NULL, 0);
  printf("—————————————————\n");
  blobtree_print(blobs, 0);

	// Check result
  tree_print_integrity_check(blobs->tree->root);
#endif

	/*  Converting result into output image  */

	// Area ids image
	img_of_blob_ids(W, H, workspace, area_ids);

	// Area ids as coloured output
	img_of_blob_ids_coloured(id_to_bgr_map2, W, H, workspace, area_ids_coloured);
	cv::imwrite("area_ids.png", area_ids_coloured );
	dst = area_ids_coloured;

if(1){
	// Stage 2
	stage2 = src1.clone();
	// Filter out areas at the image border and all areas behind them (= areas with lower depth, which are ancestor node)
	for( uint32_t x=0; x<W; ++x) {
		stage2_remove_id(blobs, x, 1);
		break;
	}
	for( uint32_t y=0; y<H; ++y) {
	}
}


  // Clean up.
  depthtree_destroy_workspace( &workspace );

  blobtree_destroy(&blobs);

	return 0;
}


int32_t main(int32_t argc, char **argv)
{
	cv::samples::addSamplesDataSearchPath("../examples/assets/example_images/kinect/depth");

	if (argc > 1) {
		src1 = cv::imread( cv::samples::findFile(argv[1]) );
	}	else {
		src1 = cv::imread( cv::samples::findFile("depth001.png") );
	}
	//src2 = src1.clone(); //imread( samples::findFile("depth001.png") );
	if( src1.empty() ) { cout << "Error loading src1 \n"; return -1; }
	//if( src2.empty() ) { cout << "Error loading src2 \n"; return -1; }


	foobar();
	return 0;

	alpha_slider = 0;
	cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE); // Create Window
	cv::createTrackbar( trackbar_name, win_name, NULL, alpha_slider_max, on_trackbar );
	cv::setTrackbarPos( trackbar_name, win_name, alpha_slider);

	// Draw
	on_trackbar( alpha_slider, 0 );
	// Wait with input loop until ESC key pressed
	while( cv::waitKey(0) != 27 )
	{
	}


	return 0;
}

