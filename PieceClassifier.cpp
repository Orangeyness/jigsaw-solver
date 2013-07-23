#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
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
#define PI 3.14159265

using namespace std;
using namespace cv;

Point origin_point(vector<Point>& edge);
double interior_angle(Point middle_vertex, Point prev_vertex, Point next_vertex);

void display(PieceData &piece, string window_name, Point origin, vector<Point> &smoothed_edge, vector<Point> &corner_points)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = piece.image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());
	raw_img.copyTo(display_img);

	vector< vector<Point> > contours;
	contours.push_back(piece.edge());
	contours.push_back(smoothed_edge);

	drawContours(display_img, contours, 0, Scalar(0, 0, 255), 2);
	drawContours(display_img, contours, 1, Scalar(0, 255, 0), 2);

	circle(display_img, origin, 5, Scalar(0, 255, 0), -1);

	for (int i = 0; i < smoothed_edge.size(); i++) 
	{
		circle(display_img, smoothed_edge[i], 4, Scalar(255, 0, 0), -1);
	}

	for (int i = 0; i < corner_points.size(); i++) 
	{
		circle(display_img, corner_points[i], 6, Scalar(128, 0, 255), -1);
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

vector<Point> find_critical_points(vector<Point>& smoothed_edge, Point origin)
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

int main(int argc, char* argv[]) 
{
	string piece_filename = string(argv[1]);

	PieceData pd (piece_filename);

	vector<Point> edge = pd.edge();
	vector<Point> smoothed_edge (edge.size());

	approxPolyDP(edge, smoothed_edge, 20, true);

	Point origin = origin_point(smoothed_edge);

	vector<Point> corner_points = find_critical_points(smoothed_edge, origin);

	if (corner_points.size() == 0)
	{
		cout << "Does not appear to be valid piece" << endl;
	}

	display(pd, piece_filename, origin, smoothed_edge, corner_points);

	waitKey();

	return EXIT_SUCCESS;
}
