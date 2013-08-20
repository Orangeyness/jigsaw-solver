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

#define EDGE_SIMPLIFY_AMOUNT 15
#define RIGHT_ANGLE_DIFF 10.0
#define RIGHT_ANGLE_MIN 90 - RIGHT_ANGLE_DIFF
#define RIGHT_ANGLE_MAX 90 + RIGHT_ANGLE_DIFF
#define NOT_RIGHT_ANGLE(x) (x < RIGHT_ANGLE_MIN || x > RIGHT_ANGLE_MAX)
#define NOT_NEARLY_RIGHT_ANGLE(x) (x < RIGHT_ANGLE_MIN - RIGHT_ANGLE_DIFF*2.5 || x > RIGHT_ANGLE_MAX + RIGHT_ANGLE_DIFF*2.5)
//#define NOT_NEARLY_RIGHT_ANGLE(x) false

//--- Forward declarations
Point origin_point(vector<Point>& edge);
vector<Point> find_corner_points(vector<Point>& smoothed_edge, Size area);
vector<int> find_corner_indexs(vector<Point>& edge, vector<Point>& corner_points);
int classify_edge(list<Point>* edge);
int piece_classifier(string piece_filename, bool debug);

void drawLineList(Mat display_img, list<Point>* point_list, Scalar color, int line_width);
void display(PieceData &piece, string window_name);
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

	approxPolyDP(edge, smoothed_edge, EDGE_SIMPLIFY_AMOUNT, true);

	vector<Point> corner_points = find_corner_points(smoothed_edge, pd.image().size());
	pd.setOrigin(origin_point(corner_points));

	if (corner_points.size() == 0)
	{
		return EXIT_FAILURE;
	}

	vector<int> corner_indexs = find_corner_indexs(edge, corner_points);
	
	/*Point top_right = edge[corner_indexs[0]];
	Point top_left = edge[corner_indexs[1]];
	
	double angle = atan2(top_right.y - top_left.y, top_right.x - top_left.x);
	pd.rotate(-angle);*/
	/*
	double cos_r = cos(angle);
	double sin_r = sin(angle);
	(*it).x = (int)(xx * cos_r - yy * sin_r);
	//(*it).x * cos(rotation) - (*it).y * sin(rotation);
	(*it).y = (int)(xx * sin_r + yy * cos_r);
	*/

	pd.setCornerIndexs(corner_indexs);

	cout << "Piece '"<< piece_filename << "'\t - Edges: \t";
	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		int type = classify_edge(pd.getEdgePoints(i));

		pd.setEdgeType(i, type);

		cout << EDGE_DIR_NAMES[i] << ": " << EDGE_TYPE_NAMES[type] << "\t";
	}
	cout << endl;

	if (debug) display(pd, piece_filename);

	pd.write(piece_filename);

	return EXIT_SUCCESS;
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

void normalise_point_position(list<Point>* point_list, Point base)
{
	list<Point>::iterator it;

	for (it = point_list->begin(); it != point_list->end(); it++)
	{
		(*it).x = (*it).x - base.x;
		(*it).y = (*it).y - base.y;
	}
}


// Classifies an edge as one of {EDGE_TYPE_FLAT, EDGE_TYPE_IN, EDGE_TYPE_OUT}. 
int classify_edge(list<Point>* edge)
{
	Point first_corner = edge->front();
	Point second_corner = edge->back();
	list<Point>::iterator it;
	
	int origin_side_of_line = side_of_line(Point(0, 0), first_corner, second_corner);
	double origin_dist_to_line = distance_from_line(Point(0, 0), first_corner, second_corner);

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
vector<Point> find_corner_points(vector<Point>& smoothed_edge, Size area_size)
{
	double greatest_area = 0;
	vector<Point> corner_points (4);

	Point estimated_origin (area_size.width / 2, area_size.height / 2);

	for (int i = 0; i < smoothed_edge.size(); i++) 
	{
		double angle;
		for (int j = 0; j < smoothed_edge.size(); j++) 
		{
			if (j == i) continue;

			angle = interior_angle_d(estimated_origin, smoothed_edge[i], smoothed_edge[j]);
			if (NOT_NEARLY_RIGHT_ANGLE(angle)) continue;

			for (int k = 0; k < smoothed_edge.size(); k++) 
			{
				if (k == j || k == i) continue;
				
				angle = interior_angle_d(smoothed_edge[i], smoothed_edge[j], smoothed_edge[k]);
				if (NOT_RIGHT_ANGLE(angle)) continue;

				angle = interior_angle_d(estimated_origin, smoothed_edge[i], smoothed_edge[k]);
				if (NOT_NEARLY_RIGHT_ANGLE(angle)) continue;

				for (int l = 0; l < smoothed_edge.size(); l++) 
				{
					if (l == k || l == j || l == i) continue;

					angle = interior_angle_d(smoothed_edge[k], smoothed_edge[i], smoothed_edge[l]);
					if (NOT_RIGHT_ANGLE(angle)) continue;
					
					angle = interior_angle_d(smoothed_edge[l], smoothed_edge[k], smoothed_edge[j]);
					if (NOT_RIGHT_ANGLE(angle)) continue;

					angle = interior_angle_d(estimated_origin, smoothed_edge[k], smoothed_edge[l]);
					if (NOT_NEARLY_RIGHT_ANGLE(angle)) continue;

					angle = interior_angle_d(estimated_origin, smoothed_edge[j], smoothed_edge[l]);
					if (NOT_NEARLY_RIGHT_ANGLE(angle)) continue;
					
					double area = area_of_rectangle(smoothed_edge[l], smoothed_edge[k], smoothed_edge[j]);
					//TODO: use area of intersection
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

	/*
	cout << corner_points[0] << corner_points[1] << corner_points[2] << corner_points[3] << endl;

	double angle;
	angle = interior_angle_d(estimated_origin, corner_points[0], corner_points[1]);
	cout << "i-j " << angle << endl;
	angle = interior_angle_d(estimated_origin, corner_points[0], corner_points[2]);
	cout << "i-k " << angle << endl;
	angle = interior_angle_d(estimated_origin, corner_points[2], corner_points[3]);
	cout << "k-l " << angle << endl;
	angle = interior_angle_d(estimated_origin, corner_points[1], corner_points[3]);
	cout << "j-l " << angle << endl;
	*/

	return corner_points;
}


vector<int> find_corner_indexs(vector<Point>& edge, vector<Point>& corner_points)
{
	vector<int> corner_indexs;
	
	int lefts[2] = { -1, -1 };
	int rights[2] = { -1, -1 };

	for (int i = 0; i < corner_points.size(); i++)
	{
		for (int j = 0; j < edge.size(); j++)
		{
			if (corner_points[i] == edge[j])
			{
				int point_index = j;

				if (lefts[0] == -1 || corner_points[i].x < edge[lefts[0]].x)
				{
					lefts[1] = lefts[0];
					lefts[0] = point_index;
				}
				else if (lefts[1] == -1 || corner_points[i].x < edge[lefts[1]].x)
				{
					lefts[1] = point_index;
				}

				if (rights[0] == -1 || corner_points[i].x > edge[rights[0]].x)
				{
					rights[1] = rights[0];
					rights[0] = point_index;
				}
				else if (rights[1] == -1 || corner_points[i].x > edge[rights[1]].x)
				{
					rights[1] = point_index;
				}
				break;
			}
		}
	}

	// find top left and bot left corner index
	int top_left_index = lefts[0];
	int bot_left_index = lefts[1];

	if (edge[lefts[1]].y < edge[lefts[0]].y)
	{
		top_left_index = lefts[1];
		bot_left_index = lefts[0];
	}

	// find top right and bot right corner index
	int top_right_index = rights[0];
	int bot_right_index = rights[1];

	if (edge[rights[1]].y < edge[rights[0]].y)
	{
		top_right_index = rights[1];
		bot_right_index = rights[0];
	}

	/*
	cout << edge[lefts[0]] << edge[lefts[1]] << endl;
	cout << edge[rights[0]] << edge[rights[1]] << endl;

	cout << "topleft :" << top_left_index << " | " << edge[top_left_index] << endl;
	cout << "topright :" << top_right_index << " | " << edge[top_right_index] << endl;
	cout << "botleft :" << bot_left_index << " | " << edge[bot_left_index] << endl;
	cout << "botright :" << bot_right_index << " | " << edge[bot_right_index] << endl;
	*/

	corner_indexs.push_back(top_right_index);
	corner_indexs.push_back(top_left_index);
	corner_indexs.push_back(bot_left_index);
	corner_indexs.push_back(bot_right_index);

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


void drawLineList(Mat display_img, list<Point>* point_list, Point offset, Scalar color, int line_width)
{
	list<Point>::iterator it = point_list->begin();
	Point* prev_point = &*it;

	while(++it != point_list->end())
	{
		Point* current_point = &*it;
		line(display_img, *prev_point + offset, *current_point + offset, color, line_width);

		prev_point = current_point;
	}
}

void display(PieceData &piece, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = piece.image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());
	raw_img.copyTo(display_img);

	drawLineList(display_img, piece.getEdgePoints(0), piece.origin(), Scalar(128, 128, 0), 2);
	drawLineList(display_img, piece.getEdgePoints(1), piece.origin(), Scalar(0, 255, 0), 2);
	drawLineList(display_img, piece.getEdgePoints(2), piece.origin(), Scalar(0, 0, 255), 2);
	drawLineList(display_img, piece.getEdgePoints(3), piece.origin(), Scalar(255, 255, 210), 2);

	Point test_origin (raw_img.cols / 2, raw_img.rows / 2);

	circle(display_img, piece.origin(), 5, Scalar(0, 255, 0), -1);
	circle(display_img, test_origin, 5, Scalar(255, 0, 0), -1);

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		circle(display_img, piece.edge()[piece.getCornerIndex(i)] + piece.origin(), 6, Scalar(128, i*60, 255), -1);
	}

	imshow(window_name, display_img);
	imwrite("output.png", display_img);
}
