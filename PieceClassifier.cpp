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

#define CRITICAL_THRESHOLD 110
#define PI 3.14159265

using namespace std;
using namespace cv;

void display(PieceData &piece, string window_name, vector<Point> &critical_points, vector<Point> &smoothed_edge) 
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

	for (int i = 0; i < critical_points.size(); i++) 
	{
		circle(display_img, critical_points[i], 5, Scalar(255, 0, 0), -1);
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

int main(int argc, char* argv[]) 
{
	string piece_filename = string(argv[1]);

	PieceData pd (piece_filename);

	vector<Point> edge = pd.edge();
	list<Point> smoothed_edge;
	vector<Point> critical_points;

	for (int i = 0; i < edge.size(); i++) 
		smoothed_edge.push_back(edge[i]);

	int prev_x = edge[0].x;
	int prev_y = edge[0].y;
	double prev_angle = 0;

	list<Point>::iterator it;
	for (it = ++(smoothed_edge.begin()); it != smoothed_edge.end(); ++it)
	{
		if (++it == smoothed_edge.end()) break;
		it--;

		Point prevPoint = *(--it);
		Point currentPoint = *(++it);
		Point nextPoint = *(++it);

		it--;

		double dist_a = euclid_distance(prevPoint.x, prevPoint.y, currentPoint.x, currentPoint.y);
		double dist_b = euclid_distance(currentPoint.x, currentPoint.y, nextPoint.x, nextPoint.y);
		double dist_total = euclid_distance(prevPoint.x, prevPoint.y, nextPoint.x, nextPoint.y);

		//double first_angle = atan2(edge[i].x - edge[i+1].x, edge[i].y - edge[i+1].y) * 180 / PI;
		//double second_angle = atan2(edge[i+1].x - edge[i+3].x, edge[i+1].y - edge[i+3].y) * 180 / PI;
		//double complete_angle = atan2(edge[i].x - edge[i+2].x, edge[i].y - edge[i+2].y) * 180 / PI;

		//cout << dist_a << " | " << dist_b << " | " << dist_total << " | A+B = " << dist_a + dist_b << " | C*1.1 = " << dist_total*1.1 << endl;
		//if (abs(first_angle - second_angle) > 30)
		if (dist_a + dist_b < dist_total*1.05)
		{
			it = smoothed_edge.erase(it);
			it--;
		}
	}

	vector<Point> smoothed;
	for (it = smoothed_edge.begin(); it != smoothed_edge.end(); it++)
		smoothed.push_back(*it);


	for (int i = 1; i < smoothed.size()-1; i+=1) 
	{
		Point prevPoint = smoothed[i-1];
		Point currentPoint = smoothed[i];
		Point nextPoint = smoothed[i+1];

		//double angle = atan2(x - prev_x, y - prev_y) * 180 / PI;
		//double delta_angle = abs(angle - prev_angle);
	
		//cout << x << ", " << y << " | a = " << angle << " | delta_a = " << delta_angle << endl;

		double angle = interior_angle(currentPoint, prevPoint, nextPoint);

		cout << angle << endl;

		if (angle < CRITICAL_THRESHOLD) 
		{
			critical_points.push_back(smoothed[i]);
		}

		//prev_angle = angle;
		//prev_x = x;
		//prev_y = y;
	}

	display(pd, piece_filename, critical_points, smoothed);

	waitKey();

	return EXIT_SUCCESS;
}
