#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <list>

#include "PieceData.h"
#include "Edge.h"
#include "GeometryHelpers.h"

#define COUPLING_DISTANCE_THRESHOLD 35
#define H_DISTANCE_THRESHOLD 12

#define ROTATE_PADDING 50


double compute_coupling_distance(vector<Point>& curveA, vector<Point>& curveB, int i, int j, vector<vector<double> >& ca)
{
	if (ca[i][j] > -1) return ca[i][j];

	if (i == 0 && j == 0) 
	{
		ca[i][j] = euclid_distance(curveA[i], curveB[j]);
	}
	else if (i > 0 && j == 0) 
	{
		ca[i][j] = max(compute_coupling_distance(curveA, curveB, i - 1, j, ca), euclid_distance(curveA[i], curveB[j]));
	}
	else if (i == 0 && j > 0) 
	{
		ca[i][j] = max(compute_coupling_distance(curveA, curveB, i, j -1, ca), euclid_distance(curveA[i], curveB[j]));
	}
	else
	{
		// i > 0 && j > 0
		double minus_i = compute_coupling_distance(curveA, curveB, i - 1, j, ca);
		double minus_j = compute_coupling_distance(curveA, curveB, i, j -1, ca);
		double minus_ij = compute_coupling_distance(curveA, curveB, i - 1, j - 1, ca);

		ca[i][j] = max(min(minus_i, min(minus_j, minus_ij)), euclid_distance(curveA[i], curveB[j]));
	}

	//cout << i << ", " << j << " = " << ca[i][j] << endl;

	return ca[i][j];
}

double coupling_distance(list<Point>* curveA, list<Point>* curveB)
{
	vector<vector<double> > ca (curveA->size());

	for (int i = 0; i < curveA->size(); i++) 
	{
		ca[i] = vector<double>(curveB->size());
		for(int j = 0; j < curveB->size(); j++)
		{
			ca[i][j] = -1;
		}
	}

	curveB->reverse();

	vector<Point> vec_curveA (curveA->begin(), curveA->end());
	vector<Point> vec_curveB (curveB->begin(), curveB->end());

	curveB->reverse();

	return compute_coupling_distance(vec_curveA, vec_curveB, curveA->size() - 1, curveB->size() - 1, ca);
}

double hausdorff_measure(list<Point>* curveA, list<Point>* curveB)
{
	double hausdorff_dist = 0;

	list<Point>::iterator itA = curveA->begin();

	while(++itA != curveA->end())
	{
		double shortest = 99999;

		list<Point>::iterator itB = curveB->begin();

		while(++itB != curveB->end())
		{
			double dist = euclid_distance(*itA, *itB);
			
			if (dist < shortest)
				shortest = dist;
		}

		if (shortest > hausdorff_dist)
			hausdorff_dist = shortest;
	}
	
	return hausdorff_dist;
}

double h_measure(list<Point>* curveA, list<Point>* curveB)
{
	double total_min_distances = 0;

	list<Point>::iterator itA = curveA->begin();

	while(++itA != curveA->end())
	{
		double min_distance = 99999;

		list<Point>::iterator itB = curveB->begin();

		while(++itB != curveB->end())
		{
			double dist = euclid_distance(*itA, *itB);
			
			if (dist < min_distance)
				min_distance = dist;
		}

		total_min_distances += min_distance;
	}
	
	return total_min_distances / curveA->size();
}


void overlayImage(const cv::Mat &background, const cv::Mat &foreground, cv::Mat &output, cv::Point2i location)
{
	background.copyTo(output);

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for(int y = std::max(location.y , 0); y < background.rows; ++y)
	{
		int fY = y - location.y; // because of the translation
		if(fY >= foreground.rows) break;

		// start at the column indicated by location, 
		// or at column 0 if location.x is negative.

		for(int x = std::max(location.x, 0); x < background.cols; ++x)
		{
			int fX = x - location.x; // because of the translation.
			// we are done with this row if the column is outside of the foreground image.
			// determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
			double opacity = 1;
			// and now combine the background and foreground pixel, using the opacity, 

			unsigned char f_r = foreground.data[fY * foreground.step + fX * foreground.channels() + 0];
			unsigned char f_g = foreground.data[fY * foreground.step + fX * foreground.channels() + 1];
			unsigned char f_b = foreground.data[fY * foreground.step + fX * foreground.channels() + 2];

			if (f_r == 0 && f_g == 0 && f_b == 0) opacity = 0;

			// but only if opacity > 0.
			for(int c = 0; opacity > 0 && c < output.channels(); ++c)
			{
				unsigned char foregroundPx = foreground.data[fY * foreground.step + fX * foreground.channels() + c];
				unsigned char backgroundPx = background.data[y * background.step + x * background.channels() + c];
				output.data[y*output.step + output.channels()*x + c] = backgroundPx * (1.-opacity) + foregroundPx * opacity;
      			}
		}
	}
}

void drawLineList(Mat display_img, list<Point>* point_list, Scalar color, int line_width)
{
	list<Point>::iterator it = point_list->begin();
	Point* prev_point = &*it;

	while(++it != point_list->end())
	{
		Point* current_point = &*it;
		line(display_img, *prev_point, *current_point, color, line_width);

		prev_point = current_point;
	}
}

void drawLineListOffset(Mat display_img, list<Point>* point_list, Scalar color, int line_width, Point origin)
{
	list<Point>::iterator it = point_list->begin();
	Point prev_point = *it;
	prev_point.x = prev_point.x + origin.x;
	prev_point.y = prev_point.y + origin.y;

	while(++it != point_list->end())
	{
		Point current_point = *it;
		current_point.x = current_point.x + origin.x;
		current_point.y = current_point.y + origin.y;

		line(display_img, prev_point, current_point, color, line_width);

		prev_point = current_point;
	}
}

void display_edge(Edge& edge, string window_name, Scalar color = Scalar(0, 255, 0))
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edge.piece()->image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		if (i == edge.index()) continue;
		drawLineList(display_img, edge.piece()->getEdgePoints(i), Scalar(128, 128, 128), 1);
	}

	drawLineList(display_img, edge.points(), color, 2);

	imshow(window_name, display_img);
}

void display_image(Mat& img, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);
	
	imshow(window_name, img);
}

void display_image_comparision(Mat edgeIn, Mat edgeOut, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edgeIn;
	Mat display_img;
	display_img.create(Size(1000, 1000), raw_img.type());

	//overlayImage(display_img, edgeIn, display_img, Point(10, 10));	
	//overlayImage(display_img, edgeOut, display_img, Point(10, edgeIn.size().height));	

	imshow(window_name, display_img);
}

void display_edge_comparision(Edge& edgeIn, Edge& edgeOut, String window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edgeIn.piece()->image();
	Mat display_img;
	display_img.create(Size(600, 600), raw_img.type());

	Point origin = Point(200, 200);
	circle(display_img, origin, 6, Scalar(255, 255, 255), -1);

	circle(display_img, edgeIn.firstPoint() + origin, 4, Scalar(255, 0, 0), -1);
	circle(display_img, edgeIn.lastPoint() + origin, 4, Scalar(0, 0, 255), -1);
	circle(display_img, edgeOut.firstPoint() + origin, 4, Scalar(255, 0, 0), -1);
	circle(display_img, edgeOut.lastPoint() + origin, 4, Scalar(0, 0, 255), -1);

	line(display_img, edgeOut.firstPoint() + origin, edgeOut.lastPoint() + origin, Scalar(128, 128, 128), 1);
	line(display_img, edgeIn.firstPoint() + origin, edgeIn.lastPoint() + origin, Scalar(128, 128, 128), 1);

	drawLineListOffset(display_img, edgeIn.points(), Scalar(0, 255, 0), 2, origin);
	drawLineListOffset(display_img, edgeOut.points(), Scalar(255, 0, 0), 2, origin);

	imshow(window_name, display_img);
}

void normalise_point_position(list<Point>* point_list, Point base)
{
	list<Point>::iterator it;

	for (it = point_list->begin(); it != point_list->end(); it++)
	{
		(*it).x = (*it).x - base.x;
		(*it).y = (*it).y - base.y;
	}
}

void normalise_point_rotation(list<Point>* point_list, double rotation)
{
	list<Point>::iterator it;

	double cos_r = cos(rotation);
	double sin_r = sin(rotation);

	for (it = point_list->begin(); it != point_list->end(); it++)
	{
		double xx = (*it).x;
		double yy = (*it).y;
		
		(*it).x = (int)(xx * cos_r - yy * sin_r);
		//(*it).x * cos(rotation) - (*it).y * sin(rotation);
		(*it).y = (int)(xx * sin_r + yy * cos_r);
		//(*it).x * sin(rotation) + (*it).y * cos(rotation);
	}
}

void rotate_edge_around_origin(Edge* edge, Mat& edge_image)
{
	Point piece_origin = edge->piece()->origin();
	normalise_point_position(edge->points(), piece_origin);

	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());
	Point desired_pos = Point(0, 100);

	double angle = interior_angle(Point(0, 0), midpoint, desired_pos);

	Mat rot_mat = getRotationMatrix2D(piece_origin, TO_DEGREE(angle), 1.0);
	warpAffine(edge_image, edge_image, rot_mat, edge_image.size());

	normalise_point_rotation(edge->points(), -angle);
}

void flip_edge_around_midpoint(Edge* edge, Mat& edge_image)
{
	Point piece_origin = edge->piece()->origin();
	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());
	normalise_point_rotation(edge->points(), PI);


	Mat rot_mat = getRotationMatrix2D(piece_origin + midpoint, TO_DEGREE(PI), 1.0);
	warpAffine(edge_image, edge_image, rot_mat, edge_image.size());
}

void rotate_edge_around_midpoint(Edge* edge, Mat& edge_image)
{
	Point piece_origin = edge->piece()->origin();
	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());
	normalise_point_position(edge->points(), midpoint);

	Point normalised_first_point = edge->firstPoint();
	normalised_first_point.y = 0;

	double angle = interior_angle(Point(0, 0), normalised_first_point, edge->firstPoint());
	normalise_point_rotation(edge->points(), angle);

	Mat rot_mat = getRotationMatrix2D(piece_origin + midpoint, TO_DEGREE(-angle), 1.0);
	warpAffine(edge_image, edge_image, rot_mat, edge_image.size());

	//TODO: fix this when awake and understand angles
	// check if line is now perpendicular to y axis
	if (abs(TO_DEGREE(interior_angle(Point(0, 0), Point(0, 10), edge->lastPoint())) - 90) > 0.01 )
	{
		// if not we rotated in wrong direction?
		// so rotate back twice as much
		normalise_point_rotation(edge->points(), -angle*2);

		rot_mat = getRotationMatrix2D(piece_origin + midpoint, TO_DEGREE(angle*2), 1.0);
		warpAffine(edge_image, edge_image, rot_mat, edge_image.size());
	}
}

int main(int argc, char* argv[]) 
{
	string first_piece_filename = string(argv[1]);
	string second_piece_filename = string(argv[2]);

	int first_edge_index = atoi(argv[3]);
	int second_edge_index = atoi(argv[4]);

	Edge edge1 (first_piece_filename, first_edge_index);
	Edge edge2 (second_piece_filename, second_edge_index);

	Edge* edgeIn = NULL;
	Edge* edgeOut = NULL;

	if (edge1.type() == EDGE_TYPE_FLAT)
	{
		cout << "First edge is flat." << endl;
		display_edge(edge1, "Edge A");
		display_edge(edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	if (edge2.type() == EDGE_TYPE_FLAT)
	{
		cout << "Second edge is flat" << endl;
		display_edge(edge1, "Edge A");
		display_edge(edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	if (edge1.type() == edge2.type())
	{
		cout << "Edges same type." << endl;
		display_edge(edge1, "Edge A");
		display_edge(edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	edgeIn = (edge1.type() == EDGE_TYPE_IN ? &edge1 : &edge2);
	edgeOut = (edge1.type() == EDGE_TYPE_OUT ? &edge1 : &edge2);

	Mat edge_in_image = edgeIn->piece()->image().clone();
	Mat edge_out_image = edgeOut->piece()->image().clone();

	display_edge(*edgeIn, "Edge In", Scalar(0, 255, 0));
	display_edge(*edgeOut, "Edge Out", Scalar(255, 0, 0));

	rotate_edge_around_origin(edgeOut, edge_out_image);
	rotate_edge_around_midpoint(edgeOut, edge_out_image);

	if (edgeOut->firstPoint().x < edgeOut->lastPoint().x)
	{
		flip_edge_around_midpoint(edgeOut, edge_out_image);
	}

	rotate_edge_around_origin(edgeIn, edge_in_image);
	rotate_edge_around_midpoint(edgeIn, edge_in_image);

	if (edgeIn->firstPoint().x > edgeIn->lastPoint().x)
	{
		flip_edge_around_midpoint(edgeIn, edge_in_image);
	}

	normalise_point_position(edgeIn->points(), edgeIn->firstPoint());
	normalise_point_position(edgeOut->points(), edgeOut->lastPoint());

	display_edge_comparision(*edgeIn, *edgeOut, "Edge Comparision");
	//display_image_comparision(edge_in_image, edge_out_image, "Image Comparision");

	display_image(edge_in_image, "Image - Edge In");
	display_image(edge_out_image, "Image - Edge Out");

	double coupling_dist = coupling_distance(edgeIn->points(), edgeOut->points());
	double h_dist = h_measure(edgeIn->points(), edgeOut->points());

	if (coupling_dist <= COUPLING_DISTANCE_THRESHOLD && h_dist <= H_DISTANCE_THRESHOLD)
	{
		cout << "MATCH" << endl;
	}
	else
	{
		cout << "Probably not match" << endl;
	}

	cout << coupling_dist << endl;
	cout << h_dist << endl;
	

	waitKey();
	return EXIT_SUCCESS;
}
