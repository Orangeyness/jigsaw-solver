#ifndef _GEOMETRY_HELPERS_
#define _GEOMETRY_HELPERS_

#define PI 3.14159265
#define TO_DEGREE(X) (X * 180.0 / PI)
#define TO_RAD(x) (x * PI / 180.0)

#include "opencv2/imgproc/imgproc.hpp"
using namespace std;
using namespace cv;

double euclid_distance(int x1, int y1, int x2, int y2);
double euclid_distance(Point p1, Point p2);

double interior_angle(Point middle_vertex, Point prev_vertex, Point next_vertex);
double interior_angle_d(Point middle_vertex, Point prev_vertex, Point next_vertex);

double area_of_rectangle(Point middle_vertex, Point prev_vertex, Point next_vertex);

Rect contour_bounding_rect(vector<Point>& contour);
int estimate_contour_area(vector<Point>& contour);

double distance_from_line(double cx, double cy, double ax, double ay, double bx, double by);
double distance_from_line(Point p, Point l1, Point l2);
int side_of_line(Point a, Point b, Point c);
Point midpoint_of_line(Point a, Point b);

#endif
