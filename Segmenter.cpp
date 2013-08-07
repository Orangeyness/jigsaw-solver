#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "PieceData.h"
#include "GeometryHelpers.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

#define RESIZE_DIVIDER 1

#define BLUR_KERNEL_SIZE 5
#define CANNY_RATIO 3
#define CANNY_THRESHOLD_R 250
#define CANNY_THRESHOLD_G 250
#define CANNY_THRESHOLD_B 550

#define MORPH_CHANNEL_ELEM MORPH_RECT
#define MORPH_CHANNEL_SIZE 1
#define MORPH_CHANNEL_OP 4

#define MORPH_FINAL_ELEM MORPH_ELLIPSE
#define MORPH_FINAL_SIZE 2
#define MORPH_FINAL_OP 3

#define SMOOTH_EPSILON 2

#define OUTPUT_FOLDER "output/"

using namespace std;
using namespace cv;

//--- Forward declarations
int segmenter(string filename, int output_offset, bool debug);

int find_min_piece_area(vector< vector<Point> >& contours);
vector<vector<Point> > filter_contours_by_area(vector< vector<Point> >& contours);
vector<Point> smooth_contour(vector<Point>& contour);
void display(string window_prefix, string window_name, Mat display_img, double scale);
//---


// argv should contain list of filenames for images to segment
// and optionally '-v' which will cause debug information to be
// shown for all images which come after that argument
int main(int argc, char* argv[])
{
	bool debug = false;
	int total_piece_count = 0;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-v") == 0)
		{
			debug = true;
			continue;
		}

		int found_pieces = segmenter(string(argv[i]), total_piece_count, debug);

		if (found_pieces < 0)
		{
			cout << "Error on file '" << argv[i] << "'. Could not read file." << endl;
			return EXIT_FAILURE;
		}

		cout << "File '"<< argv[i] << "' - piece count: " << found_pieces << endl;
		total_piece_count += found_pieces;
	}

	waitKey();

	return EXIT_SUCCESS;
}

// The segmenter
int segmenter(string filename, int output_offset, bool debug)
{
	Mat src_image = imread(filename);
	Mat src_resized;

	if (!src_image.data) { return -1; }

	resize(src_image, src_resized, Size(src_image.cols/RESIZE_DIVIDER, src_image.rows/RESIZE_DIVIDER), 0, 0, INTER_LINEAR);

	vector<Mat> channels;
	split(src_resized, channels);

	vector<int> thresholds (3);
	thresholds[0] = CANNY_THRESHOLD_B;
	thresholds[1] = CANNY_THRESHOLD_G;
	thresholds[2] = CANNY_THRESHOLD_R;

	Mat channel_morph_element = getStructuringElement(MORPH_CHANNEL_ELEM, Size(2*MORPH_CHANNEL_SIZE+1, 2*MORPH_CHANNEL_SIZE+1), Point(MORPH_CHANNEL_SIZE, MORPH_CHANNEL_SIZE));

	for(int i = 0; i < 3; i++)
	{
		blur(channels[i], channels[i], Size(BLUR_KERNEL_SIZE, BLUR_KERNEL_SIZE));
		Canny(channels[i], channels[i], thresholds[i], thresholds[i]*CANNY_RATIO, BLUR_KERNEL_SIZE);

		morphologyEx(channels[i], channels[i], MORPH_CHANNEL_OP, channel_morph_element);
	}

	Mat edge_map;
	edge_map.create(src_image.size(), src_image.type());

	bitwise_or(channels[0], channels[1], edge_map);
	bitwise_or(channels[2], edge_map, edge_map);

	Mat final_morph_element = getStructuringElement(MORPH_FINAL_ELEM, Size(2*MORPH_FINAL_SIZE+1, 2*MORPH_FINAL_SIZE+1), Point(MORPH_FINAL_SIZE, MORPH_FINAL_SIZE));
	morphologyEx(edge_map, edge_map, MORPH_FINAL_OP, final_morph_element);

	if (debug) display(filename, "Edge Map", edge_map, 0.3);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	resize(edge_map, edge_map, Size(src_image.cols, src_image.rows), 0, 0, INTER_LINEAR);
	findContours(edge_map, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_KCOS, Point(0, 0));

	contours = filter_contours_by_area(contours);

	for (int i = 0; i < contours.size(); i++) 
	{
		contours[i] = smooth_contour(contours[i]);
	}

	if (debug)
	{
		Mat contour_img = src_image.clone();
		Mat mask = Mat::zeros(src_image.rows, src_image.cols, CV_8UC3);
		Mat output = src_image.clone();

		for (int i = 0; i < contours.size(); i++) 
		{
			drawContours(contour_img, contours, i, Scalar(0, 0, 255));
			drawContours(mask, contours, i, Scalar(255, 255, 255), -1);
		}

		bitwise_and(mask, src_image, output);
		display(filename, "Output", output, 0.3);
		display(filename, "Contour Map", contour_img, 0.3);
		display(filename, "Mask", mask, 0.3);
	}

	for(int i = 0; i < contours.size(); i++)
	{
		stringstream output_name;
		output_name << OUTPUT_FOLDER << (output_offset + i);

		PieceData piece (&src_image, contours[i]);
		piece.write(output_name.str());
	}


	return contours.size();
}

// Attempts to filter false positives out of the contour list. 
// False positives contours tend to be small parts of the background
// so this filters based on the area of the contours.
vector< vector<Point> > filter_contours_by_area(vector< vector<Point> >& contours)
{
	vector< vector<Point> > filter_contours;
	int min_area = find_min_piece_area(contours);

	for (int i = 0; i < contours.size(); i++)
	{
		int area = estimate_contour_area(contours[i]);	

		if (area >= min_area)
		{
			filter_contours.push_back(contours[i]);
		}
	}

	return filter_contours;
}

// Looks at contours and attempts to find the area of the smallest
// puzzle piece by ordering all the contours and finding the biggest
// difference between contours adjacent in the ordered array.
int find_min_piece_area(vector< vector<Point> >& contours)
{
	vector<int> contour_sizes (contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		contour_sizes[i] = estimate_contour_area(contours[i]);
	}

	sort(contour_sizes.begin(), contour_sizes.end());

	int prev_value = contour_sizes[0];
	int min_value = prev_value;
	int max_change = 0;
	for (int i = 1; i < contour_sizes.size(); i++) 
	{
		int change = contour_sizes[i] - prev_value;

		if (change > max_change)
		{
			max_change = change;
			min_value = contour_sizes[i];
		}

		prev_value = contour_sizes[i];
	}

	return min_value;
}

// Smooths the contour area using a constant epsilon value.
vector<Point> smooth_contour(vector<Point>& contour)
{
	vector<Point> smoothed_contour; 

	approxPolyDP(contour, smoothed_contour, SMOOTH_EPSILON, true);

	return smoothed_contour;
}


void display(string window_prefix, string window_name, Mat display_img, double scale)
{
	string prefixed_window_name = window_prefix + window_name;
		
	namedWindow(prefixed_window_name, CV_WINDOW_AUTOSIZE);

	if (scale == 1)
	{
		imshow(prefixed_window_name, display_img);
		return ;
	}

	Mat scaled_display_img;
	resize(display_img, scaled_display_img, Size(display_img.cols * scale, display_img.rows * scale), 0, 0, INTER_LINEAR);

	imshow(prefixed_window_name, scaled_display_img);
}
