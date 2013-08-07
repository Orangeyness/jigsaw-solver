#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <math.h>
#include <list>

#include "PieceData.h"
#include "GeometryHelpers.h"

#define EDGE_STRAY_THRESHOLD 5
#define EDGE_BIAS_THRESHOLD 5 

#define RIGHT_ANGLE_DIFF 8
#define RIGHT_ANGLE_MIN 90 - RIGHT_ANGLE_DIFF
#define RIGHT_ANGLE_MAX 90 + RIGHT_ANGLE_DIFF

//--- Forward declarations
Point origin_point(vector<Point>& edge);
vector<Point> find_corner_points(vector<Point>& smoothed_edge);
vector<int> find_corner_indexs(vector<Point>& edge, vector<Point>& corner_points);
int classify_edge(list<Point>* edge, Point origin);
int piece_classifier(string piece_filename, bool debug);

void drawLineList(Mat display_img, list<Point>* point_list, Scalar color, int line_width);
void display(PieceData &piece, vector<Point> &smoothed_edge, string window_name);
//---

// argv should contain list of filenames for image segments 
// (either the .edg, .jpg or no extension)
// and optionally '-v' which will cause debug information to be
// shown for all images which come after that argument
int main(int argc, char* argv[]) 
{
	string piece_filename = string(argv[1]);

	bool debug = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-v") == 0)
		{
			debug = true;
			continue;
		}

		int success = piece_classifier(string(argv[i]), debug);

		if (success == EXIT_FAILURE) 
		{
			cout << "Error on piece '" << argv[i] << "'. Does not appear to be valid piece." << endl;
			return EXIT_FAILURE;
		}

	}

	waitKey();

	return EXIT_SUCCESS;
}

// The piece classifier.
int piece_classifier(string piece_filename, bool debug)
{
	PieceData pd (piece_filename);

	vector<Point> edge = pd.edge();
	vector<Point> smoothed_edge (edge.size());

	approxPolyDP(edge, smoothed_edge, 20, true);

	vector<Point> corner_points = find_corner_points(smoothed_edge);
	pd.setOrigin(origin_point(corner_points));

	if (corner_points.size() == 0)
	{
		return EXIT_FAILURE;
	}

	vector<int> corner_indexs = find_corner_indexs(edge, corner_points);

	sort(corner_indexs.begin(), corner_indexs.end());
	
	pd.setCornerIndexs(corner_indexs);

	cout << "Piece '"<< piece_filename << "'\t - Edges: \t";
	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		int type = classify_edge(pd.getEdgePoints(i), pd.origin());

		pd.setEdgeType(i, type);

		cout << EDGE_DIR_NAMES[i] << ": " << EDGE_TYPE_NAMES[type] << "\t";
	}
	cout << endl;

	if (debug) display(pd, smoothed_edge, piece_filename);

	pd.write(piece_filename);

	return EXIT_SUCCESS;
}

// Classifies an edge as one of {EDGE_TYPE_FLAT, EDGE_TYPE_IN, EDGE_TYPE_OUT}. 
int classify_edge(list<Point>* edge, Point origin)
{
	Point first_corner = edge->front();
	Point second_corner = edge->back();
	list<Point>::iterator it;
	
	int origin_side_of_line = side_of_line(origin, first_corner, second_corner);
	double origin_dist_to_line = distance_from_line(origin, first_corner, second_corner);

	// Keeps track of direction lumps on the line tend to be pointing
	int edge_bias = 0;

	for(it = edge->begin(); it != edge->end(); it++) 
	{
		if (distance_from_line(*it, first_corner, second_corner) > origin_dist_to_line / EDGE_STRAY_THRESHOLD)
		{
			//Either 1 or -1 depending on which side of line it falls on
			int side = side_of_line(*it, first_corner, second_corner);			
		
			edge_bias += side;			
		}
	}

	// If there are few points straying from the optimal line
	// then the edge is probably flat
	if (abs(edge_bias) < EDGE_BIAS_THRESHOLD)
		return EDGE_TYPE_FLAT;

	// otherwise check if the majority of points straying from
	// the line are on the origin side of the line or not.
	if (edge_bias < 0 == origin_side_of_line < 0)
		return EDGE_TYPE_IN;
	else 
		return EDGE_TYPE_OUT;
}

// Attempts to find the corner points of the piece contour by finding the four points
// which create the (roughly) rectanglaur quadralateral with the largest area.
vector<Point> find_corner_points(vector<Point>& smoothed_edge)
{
	double greatest_area = 0;
	vector<Point> corner_points (4);

	for (int i = 0; i < smoothed_edge.size(); i++) 
	{
		double angle;
		for (int j = 0; j < smoothed_edge.size(); j++) 
		{
			if (j == i) continue;

			for (int k = 0; k < smoothed_edge.size(); k++) 
			{
				if (k == j || k == i) continue;
				
				angle = interior_angle_d(smoothed_edge[i], smoothed_edge[j], smoothed_edge[k]);
				if (angle < RIGHT_ANGLE_MIN || angle > RIGHT_ANGLE_MAX) continue;

				for (int l = 0; l < smoothed_edge.size(); l++) 
				{
					if (l == k || l == j || l == i) continue;

					angle = interior_angle_d(smoothed_edge[k], smoothed_edge[i], smoothed_edge[l]);
					if (angle < RIGHT_ANGLE_MIN || angle > RIGHT_ANGLE_MAX) continue;
					
					angle = interior_angle_d(smoothed_edge[l], smoothed_edge[k], smoothed_edge[j]);
					if (angle < RIGHT_ANGLE_MIN || angle > RIGHT_ANGLE_MAX) continue;

					double area = area_of_rectangle(smoothed_edge[l], smoothed_edge[k], smoothed_edge[j]);
					if (area > greatest_area) 
					{
						greatest_area = area;
						corner_points[0] = smoothed_edge[i];
						corner_points[1] = smoothed_edge[j];
						corner_points[2] = smoothed_edge[k];
						corner_points[3] = smoothed_edge[l];
					}
				}
			}
		}
	}

	if (greatest_area == 0)
	{
		corner_points.clear();
	}

	return corner_points;
}


vector<int> find_corner_indexs(vector<Point>& edge, vector<Point>& corner_points)
{
	vector<int> corner_indexs;
	
	for (int i = 0; i < corner_points.size(); i++)
	{
		for (int j = 0; j < edge.size(); j++)
		{
			if (corner_points[i] == edge[j])
			{
				corner_indexs.push_back(j);
				break;
			}
		}
	}

	return corner_indexs;
}


Point origin_point(vector<Point>& edge) 
{
	long total_x = 0;
	long total_y = 0;
	
	for (int i = 0; i < edge.size(); i++) 
	{
		total_x += edge[i].x;
		total_y += edge[i].y;
	}

	int point_x = (int) (((double)total_x) / edge.size());
	int point_y = (int) (((double)total_y) / edge.size());

	return Point (point_x, point_y);
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

void display(PieceData &piece, vector<Point> &smoothed_edge, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = piece.image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());
	raw_img.copyTo(display_img);

	drawLineList(display_img, piece.getEdgePoints(0), Scalar(128, 128, 0), 2);
	drawLineList(display_img, piece.getEdgePoints(1), Scalar(0, 255, 0), 2);
	drawLineList(display_img, piece.getEdgePoints(2), Scalar(0, 0, 255), 2);
	drawLineList(display_img, piece.getEdgePoints(3), Scalar(255, 255, 210), 2);

	circle(display_img, piece.origin(), 5, Scalar(0, 255, 0), -1);

	for (int i = 0; i < smoothed_edge.size(); i++) 
	{
		circle(display_img, smoothed_edge[i], 4, Scalar(255, 0, 0), -1);
	}

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		circle(display_img, piece.edge()[piece.getCornerIndex(i)], 6, Scalar(128, 0, 255), -1);
	}

	imshow(window_name, display_img);
	imwrite("output.png", display_img);
}
