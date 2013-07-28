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

#define RIGHT_ANGLE_DIFF 8
#define RIGHT_ANGLE_MIN 90 - RIGHT_ANGLE_DIFF
#define RIGHT_ANGLE_MAX 90 + RIGHT_ANGLE_DIFF
#define CRITICAL_THRESHOLD 110

Point origin_point(vector<Point>& edge);
double interior_angle(Point middle_vertex, Point prev_vertex, Point next_vertex);

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

double euclid_distance(int x1, int y1, int x2, int y2) 
{
	double dist1 = (double)(x1 - x2);
	double dist2 = (double)(y1 - y2);

	return sqrt(dist1 * dist1 + dist2 * dist2);
}

double euclid_distance(Point p1, Point p2)
{
	return euclid_distance(p1.x, p1.y, p2.x, p2.y);
}

double interior_angle(Point middle_vertex, Point prev_vertex, Point next_vertex)
{
	int x_diff1 = middle_vertex.x - prev_vertex.x;
	int x_diff2 = middle_vertex.x - next_vertex.x;
	int y_diff1 = middle_vertex.y - prev_vertex.y;
	int y_diff2 = middle_vertex.y - next_vertex.y;

	double dot_product = x_diff1 * x_diff2 + y_diff1 * y_diff2;

	double m1 = sqrt(x_diff1 * x_diff1 + y_diff1 * y_diff1);
	double m2 = sqrt(x_diff2 * x_diff2 + y_diff2 * y_diff2);

	double angle = acos((dot_product)/(m1 * m2)) * 180.0 / PI;

	return angle;
}

double area_of_rectangle(Point middle_vertex, Point prev_vertex, Point next_vertex) 
{
	double length1 = euclid_distance(middle_vertex, prev_vertex);
	double length2 = euclid_distance(middle_vertex, next_vertex);

	return length1 * length2;
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

vector<Point> find_corner_points(vector<Point>& smoothed_edge, Point origin)
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
				
				angle = interior_angle(smoothed_edge[i], smoothed_edge[j], smoothed_edge[k]);
				if (angle < RIGHT_ANGLE_MIN || angle > RIGHT_ANGLE_MAX) continue;

				for (int l = 0; l < smoothed_edge.size(); l++) 
				{
					if (l == k || l == j || l == i) continue;

					angle = interior_angle(smoothed_edge[k], smoothed_edge[i], smoothed_edge[l]);
					if (angle < RIGHT_ANGLE_MIN || angle > RIGHT_ANGLE_MAX) continue;
					
					angle = interior_angle(smoothed_edge[l], smoothed_edge[k], smoothed_edge[j]);
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


double distance_from_line(double cx, double cy, double ax, double ay, double bx, double by)
{
	double r_numerator = (cx-ax)*(bx-ax) + (cy-ay)*(by-ay);
	double r_denomenator = (bx-ax)*(bx-ax) + (by-ay)*(by-ay);
	double r = r_numerator / r_denomenator;
   	double px = ax + r*(bx-ax);
  	double py = ay + r*(by-ay);
    	double s =  ((ay-cy)*(bx-ax)-(ax-cx)*(by-ay) ) / r_denomenator;

	return fabs(s)*sqrt(r_denomenator);
}

double distance_from_line(Point p, Point l1, Point l2)
{
	return distance_from_line(p.x, p.y, l1.x, l1.y, l2.x, l2.y);
}

bool side_of_line(Point a, Point b, Point c)
{
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0;
}

int classify_edge(list<Point>* edge, Point origin)
{
	Point first_corner = edge->front();
	Point second_corner = edge->back();
	list<Point>::iterator it;
	
	bool origin_side_of_line = side_of_line(origin, first_corner, second_corner);
	
	for(it = edge->begin(); it != edge->end(); it++) 
	{
		if (distance_from_line(*it, first_corner, second_corner) > 30)
		{
			bool side = side_of_line(*it, first_corner, second_corner);
			
			if (side == origin_side_of_line)  
				return EDGE_TYPE_IN;
			else
				return EDGE_TYPE_OUT;
		}
	}

	return EDGE_TYPE_FLAT;
}

int main(int argc, char* argv[]) 
{
	string piece_filename = string(argv[1]);

	PieceData pd (piece_filename);

	vector<Point> edge = pd.edge();
	vector<Point> smoothed_edge (edge.size());

	approxPolyDP(edge, smoothed_edge, 20, true);

	pd.setOrigin(origin_point(smoothed_edge));

	vector<Point> corner_points = find_corner_points(smoothed_edge, pd.origin());

	if (corner_points.size() == 0)
	{
		cout << "Does not appear to be valid piece" << endl;
		return EXIT_FAILURE;
	}

	vector<int> corner_indexs = find_corner_indexs(edge, corner_points);

	sort(corner_indexs.begin(), corner_indexs.end());
	
	pd.setCornerIndexs(corner_indexs);


	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		int type = classify_edge(pd.getEdgePoints(i), pd.origin());

		pd.setEdgeType(i, type);

		cout << EDGE_DIR_NAMES[i] << ": " << EDGE_TYPE_NAMES[type] << endl;
	}

	
	/*
	Point top_left = pd.edge()[pd.getCornerIndex(0)];
	Point bottom_left = pd.edge()[pd.getCornerIndex(1)];
	Point normalised_bottom_left = bottom_left;
	normalised_bottom_left.x = top_left.x;
	
	double rotation = interior_angle(top_left, bottom_left, normalised_bottom_left);
	cout << rotation << endl;
	*/

	display(pd, smoothed_edge, piece_filename);

	pd.write(piece_filename);

	waitKey();

	return EXIT_SUCCESS;
}
