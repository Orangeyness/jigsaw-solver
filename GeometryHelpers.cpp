#include "GeometryHelpers.h"

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

	double angle = acos((dot_product)/(m1 * m2));

	return angle;
}

double interior_angle_d(Point middle_vertex, Point prev_vertex, Point next_vertex)
{
	return TO_DEGREE(interior_angle(middle_vertex, prev_vertex, next_vertex));
}

double area_of_rectangle(Point middle_vertex, Point prev_vertex, Point next_vertex) 
{
	double length1 = euclid_distance(middle_vertex, prev_vertex);
	double length2 = euclid_distance(middle_vertex, next_vertex);

	return length1 * length2;
}

Rect contour_bounding_rect(vector<Point>& contour)
{
	vector<Point> contour_poly;
	approxPolyDP(Mat(contour), contour_poly, 3, true);
	return boundingRect(Mat(contour_poly));
}

int estimate_contour_area(vector<Point>& contour)
{
	Rect bounding_rect = contour_bounding_rect(contour);

	return bounding_rect.area();
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

int side_of_line(Point a, Point b, Point c)
{
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0 ? 1 : -1;
}

Point midpoint_of_line(Point a, Point b)
{
	int minx = min(a.x, b.x);
	int miny = min(a.y, b.y);
	int maxx = max(a.x, b.x);
	int maxy = max(a.y, b.y);

	return Point(minx + (maxx-minx)/2, miny + (maxy - miny)/2);
}
