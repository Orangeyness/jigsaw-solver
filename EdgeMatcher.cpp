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

void get_edge_points(Edge* edge, vector<Point>& out)
{
	PieceData* pd = edge->piece();
	int edge_index = edge->index();

	ptIter edge_begin = pd->getEdgeBegin(edge_index);
	ptIter edge_end = pd->getEdgeEnd(edge_index);
	ptIter piece_begin = pd->begin();
	ptIter piece_end = pd->end();

	ptIter iter = edge_begin;

	while(iter != edge_end)
	{
		out.push_back(*iter);

		iter ++;

		if (iter == piece_end && piece_end != edge_end)
		{
			iter = piece_begin;	
		}		
	}
}

void get_reverse_edge_points(Edge* edge, vector<Point>& out)
{
	PieceData* pd = edge->piece();
	int edge_index = edge->index();

	ptIter edge_begin = pd->getEdgeBegin(edge_index);
	ptIter edge_end = pd->getEdgeEnd(edge_index);
	ptIter piece_begin = pd->begin();
	ptIter piece_end = pd->end();

	ptIter iter = edge_end;

	do 
	{
		iter --;

		out.push_back(*iter);

		if (iter == piece_begin && piece_begin != edge_begin)
		{
			iter = piece_end;	
		}		
	}
	while(iter != edge_begin);
}


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

double coupling_distance(Edge* edgeA, Edge* edgeB)
{
	vector<Point> curveA;
	vector<Point> curveB;

	get_edge_points(edgeA, curveA);
	get_reverse_edge_points(edgeB, curveB);
	
	vector<vector<double> > ca (curveA.size());

	for (int i = 0; i < curveA.size(); i++) 
	{
		ca[i] = vector<double>(curveB.size());
		for(int j = 0; j < curveB.size(); j++)
		{
			ca[i][j] = -1;
		}
	}

	return compute_coupling_distance(curveA, curveB, curveA.size() - 1, curveB.size() - 1, ca);
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

void drawEdge(Mat display_img, PieceData* pd, int edge_index, Scalar color, int line_width, Point origin = Point(0, 0))
{
	ptIter iter = pd->getEdgeBegin(edge_index);
	ptIter edge_end = pd->getEdgeEnd(edge_index);
	ptIter piece_begin = pd->begin();
	ptIter piece_end = pd->end();

	Point prev_point = *iter;
	prev_point.x = prev_point.x + origin.x;
	prev_point.y = prev_point.y + origin.y;

	while(iter != edge_end)
	{
		Point current_point = *iter;
		current_point.x = current_point.x + origin.x;
		current_point.y = current_point.y + origin.y;

		line(display_img, prev_point, current_point, color, line_width);

		prev_point = current_point;

		iter ++;

		if (iter == piece_end && piece_end != edge_end)
		{
			iter = piece_begin;	
		}		
	}
}

void display_edge(Edge* edge, string window_name, Scalar color = Scalar(0, 255, 0))
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edge->piece()->image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());

	circle(display_img, edge->piece()->origin(), 5, Scalar(0, 255, 0), -1);

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		if (i == edge->index()) continue;
		
		drawEdge(display_img, edge->piece(), i, Scalar(128, 128, 128), 1, edge->piece()->origin());
	}

	drawEdge(display_img, edge->piece(), edge->index(), color, 2, edge->piece()->origin());

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

void display_edge_comparision(Edge* edgeIn, Edge* edgeOut, String window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edgeIn->piece()->image();
	Mat display_img;
	display_img.create(Size(600, 600), raw_img.type());

	Point origin (20, 100);
	circle(display_img, origin, 6, Scalar(255, 255, 255), -1);

	circle(display_img, edgeIn->getFirstCorner() + origin, 4, Scalar(255, 0, 0), -1);
	circle(display_img, edgeIn->getSecondCorner() + origin, 4, Scalar(0, 0, 255), -1);
	circle(display_img, edgeOut->getFirstCorner() + origin, 4, Scalar(255, 0, 0), -1);
	circle(display_img, edgeOut->getSecondCorner() + origin, 4, Scalar(0, 0, 255), -1);

	line(display_img, edgeIn->getFirstCorner() + origin, edgeIn->getSecondCorner() + origin, Scalar(128, 128, 128), 1);
	line(display_img, edgeOut->getFirstCorner() + origin, edgeOut->getSecondCorner() + origin, Scalar(128, 128, 128), 1);

	drawEdge(display_img, edgeIn->piece(), edgeIn->index(), Scalar(0, 255, 0), 2, origin);
	drawEdge(display_img, edgeOut->piece(), edgeOut->index(), Scalar(255, 0, 0), 2, origin);

	imshow(window_name, display_img);
}

//I don't know...
//Am I depressed? I can't tell. How does one tell.
//Seriously. I might be. Fuck.
//Probably just being dramatic.

double getEdgeAtan(Edge* edge)
{
	Point corner_first = edge->getFirstCorner();
	Point corner_second = edge->getSecondCorner();

	double angle = atan2(corner_first.y - corner_second.y, corner_first.x - corner_second.x);

	return angle;
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
		display_edge(&edge1, "Edge A");
		display_edge(&edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	if (edge2.type() == EDGE_TYPE_FLAT)
	{
		cout << "Second edge is flat" << endl;
		display_edge(&edge1, "Edge A");
		display_edge(&edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	if (edge1.type() == edge2.type())
	{
		cout << "Edges same type." << endl;
		display_edge(&edge1, "Edge A");
		display_edge(&edge2, "Edge B");

		waitKey();
		return EXIT_FAILURE;
	}

	edgeIn = (edge1.type() == EDGE_TYPE_IN ? &edge1 : &edge2);
	edgeOut = (edge1.type() == EDGE_TYPE_OUT ? &edge1 : &edge2);

	double angle = getEdgeAtan(edgeIn);
	edgeIn->piece()->rotate(-angle);

	angle = getEdgeAtan(edgeOut);
	edgeOut->piece()->rotate(-angle + PI);
	
	display_edge(edgeIn, "Edge In", Scalar(0, 255, 0));
	display_edge(edgeOut, "Edge Out", Scalar(255, 0, 0));

	edgeIn->piece()->setOrigin(edgeIn->getSecondCorner());
	edgeOut->piece()->setOrigin(edgeOut->getFirstCorner());

	display_edge_comparision(edgeIn, edgeOut, "Edge Comparision");

	double coupling_dist = coupling_distance(edgeIn, edgeOut);

	if (coupling_dist <= COUPLING_DISTANCE_THRESHOLD)
	{
		cout << "MATCH" << endl;
	}
	else
	{
		cout << "Probably not match" << endl;
	}
	cout << coupling_dist << endl;
	//cout << h_dist << endl;

	waitKey();
	return EXIT_SUCCESS;
}
