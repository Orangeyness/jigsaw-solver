#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "PieceData.h"

#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#define RESIZE_DIVIDER 2

#define BLUR_KERNEL_SIZE 5
#define CANNY_RATIO 4
#define CANNY_THRESHOLD_R 250
#define CANNY_THRESHOLD_G 250
#define CANNY_THRESHOLD_B 550

#define MORPH_CHANNEL_ELEM 1
#define MORPH_CHANNEL_SIZE 1
#define MORPH_CHANNEL_OP 1

#define MORPH_FINAL_ELEM 2
#define MORPH_FINAL_SIZE 2
#define MORPH_FINAL_OP 3

#define OUTPUT_FOLDER "output/"

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
	Mat src_image = imread(argv[1]);
	Mat src_resized;

	if (!src_image.data) { return EXIT_FAILURE; }

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

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	resize(edge_map, edge_map, Size(src_image.cols, src_image.rows), 0, 0, INTER_LINEAR);
	findContours(edge_map, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_KCOS, Point(0, 0));

	for(int i = 0; i < contours.size(); i++)
	{
		stringstream output_name;
		output_name << OUTPUT_FOLDER << i;

		PieceData piece (&src_image, contours[i]);
		piece.write(output_name.str());
	}

	return EXIT_SUCCESS;
}
