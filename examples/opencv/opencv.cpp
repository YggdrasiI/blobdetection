#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui.hpp"

#include <iostream>
#include <cstdint>


#include "blobdetection/threshtree.h"
#include "blobdetection/depthtree.h"

//using namespace cv;
using std::cout;
const int32_t alpha_slider_max(100);
const std::string win_name("Depthblob parameter test");
const std::string trackbar_name("Depthblob parameter test");
int32_t alpha_slider;
double alpha;
double beta;
cv::Mat src1;
cv::Mat src2;
cv::Mat dst;
cv::Mat area_ids;
cv::Mat area_ids_coloured;


static void on_trackbar( int32_t, void* )
{
	alpha_slider = cv::getTrackbarPos(trackbar_name, win_name);
 alpha = (double) alpha_slider/alpha_slider_max ;
 beta = ( 1.0 - alpha );
 //addWeighted( src1, alpha, src2, beta, 0.0, dst);
 src1 = cv::Scalar(255, 255, 255);
 cv::scaleAdd(src2, -1, src1, dst);
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
			id = depthtree_get_id(x, y, workspace) + 1;
			//out.at<cv::CV_16UC1>(y,x)[0] = id;
			out.at<uchar>(y,x) = id;
		}
	}
}

// Function handle
typedef void ID_TO_BGR_MAP(int32_t id, cv::Vec3b &out);

void id_to_bgr_map(int32_t id, cv::Vec3b &out){
	out[0] = id*111234 % 255;
	out[1] = id*920248 % 255;
	out[2] = id*788239 % 255;
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
			id = depthtree_get_id(x, y, workspace) + 1;
			map(id, out.at<cv::Vec3b>(y,x));
		}
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

  // Now search the blobs.
  depthtree_find_blobs(blobs, sw, W, H, roi, depth_map, workspace);


	//cv::cvtColor(depth, src2, cv::COLOR_GRAY2BGR);
	/*  Converting result into output image  */

	// Area ids image
	img_of_blob_ids(W, H, workspace, area_ids);

	// Area ids as coloured output
	img_of_blob_ids_coloured(id_to_bgr_map, W, H, workspace, area_ids_coloured);
	src2 = area_ids_coloured;


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
	src2 = src1.clone(); //imread( samples::findFile("depth001.png") );
	if( src1.empty() ) { cout << "Error loading src1 \n"; return -1; }
	if( src2.empty() ) { cout << "Error loading src2 \n"; return -1; }


	foobar();

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
