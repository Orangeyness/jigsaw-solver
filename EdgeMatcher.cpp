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

class Edge
{
	private:
		PieceData pd;
		int edge_index;

	public:
		Edge(string filename, int edge) : pd (filename)
		{
			edge_index = edge;
		}

		list<Point>* points()
		{
			return pd.getEdgePoints(edge_index);
		}

		Point firstPoint()
		{
			list<Point>* p = points();
			return p->front();
		}

		Point lastPoint()
		{
			list<Point>* p = points();
			return p->back();
		}

		int index()
		{
			return edge_index;
		}
			
		int type()
		{
			return pd.getEdgeType(edge_index);
		}
		
		PieceData* piece()
		{
			return &pd; 
		}
};

Point midpoint_of_line(Point a, Point b)
{
	int minx = min(a.x, b.x);
	int miny = min(a.y, b.y);
	int maxx = max(a.x, b.x);
	int maxy = max(a.y, b.y);

	return Point(minx + (maxx-minx)/2, miny + (maxy - miny)/2);
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


bool side_of_line(Point a, Point b, Point c)
{
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0;
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

void display_edge(Edge& edge, String window_name, Scalar color = Scalar(0, 255, 0))
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

void rotate_edge_around_origin(Edge* edge)
{
	Point piece_origin = edge->piece()->origin();
	normalise_point_position(edge->points(), piece_origin);
	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());

	Point desired_pos = Point(0, 100);

	double angle = interior_angle(Point(0, 0), midpoint, desired_pos);

	normalise_point_rotation(edge->points(), -angle);
}

void flip_edge_around_midpoint(Edge* edge)
{
	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());
	normalise_point_rotation(edge->points(), PI);
}

void rotate_edge_around_midpoint(Edge* edge)
{
	Point midpoint = midpoint_of_line(edge->firstPoint(), edge->lastPoint());
	normalise_point_position(edge->points(), midpoint);

	Point normalised_first_point = edge->firstPoint();
	normalised_first_point.y = 0;

	Point normalised_last_point = edge->lastPoint();
	normalised_last_point.y = 0;

	double angle = interior_angle(Point(0, 0), normalised_first_point, edge->firstPoint());
	normalise_point_rotation(edge->points(), angle);

	//TODO: fixthis when awake and understand angles

	// check if line is now perpendicular to y axis
	if (abs(TO_DEGREE(interior_angle(Point(0, 0), Point(0, 10), edge->lastPoint())) - 90) > 0.01 )
	{
		// if not we rotated in wrong direction?
		// so rotate back twice as much
		normalise_point_rotation(edge->points(), -angle*2);
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
	
	display_edge(*edgeIn, "Edge In", Scalar(0, 255, 0));
	display_edge(*edgeOut, "Edge Out", Scalar(255, 0, 0));

	rotate_edge_around_origin(edgeOut);
	rotate_edge_around_midpoint(edgeOut);

	if (edgeOut->firstPoint().x < edgeOut->lastPoint().x)
	{
		flip_edge_around_midpoint(edgeOut);
	}

	rotate_edge_around_origin(edgeIn);
	rotate_edge_around_midpoint(edgeIn);

	if (edgeIn->firstPoint().x > edgeIn->lastPoint().x)
	{
		flip_edge_around_midpoint(edgeIn);
	}

	display_edge_comparision(*edgeIn, *edgeOut, "Edge Comparision");

	waitKey();
	return EXIT_SUCCESS;
}
