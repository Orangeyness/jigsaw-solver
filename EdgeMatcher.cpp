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
#define AVG_MIN_DISTANCE_THRESHOLD 20

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

double average_min_dist_measure(Edge* edgeA, Edge* edgeB)
{
	vector<Point> curveA;
	vector<Point> curveB;

	get_edge_points(edgeA, curveA);
	get_edge_points(edgeB, curveB);

	double total_min_distances = 0;

	vector<Point>::iterator itA = curveA.begin();

	while(++itA != curveA.end())
	{
		double min_distance = 99999;

		vector<Point>::iterator itB = curveB.begin();

		while(++itB != curveB.end())
		{
			double dist = euclid_distance(*itA, *itB);
			
			if (dist < min_distance)
				min_distance = dist;
		}

		total_min_distances += min_distance;
	}
	
	return total_min_distances / curveA.size();
}

#define COLOUR_AVERAGE_COUNT 25
vector<Scalar> avg_colour(Edge* edge, bool average_down = true)
{
	ptIter start = edge->begin();
	ptIter end = edge->end();
	Mat img = edge->piece()->image();

	int width = abs((*start).x - (*end).x);
	int origin_x = min((*start).x, (*end).x);
	int y_offset = average_down ? 0 : -COLOUR_AVERAGE_COUNT;

	vector<Scalar> colours (width);

	ptIter iter = start;
	ptIter nextIter = edge->piece()->increment(iter, 1, end);

	while (nextIter != end)
	{
		int begin_x = (*iter).x;
		int end_x = (*nextIter).x;
		int begin_y = (*iter).y;
		
		if (begin_x > end_x)
		{
			int tmp = begin_x;
			begin_x = end_x;
			end_x = tmp;
		}
		
		for (int x = begin_x; x < end_x; x++)
		{
			Rect roi (x, begin_y + y_offset, 1, COLOUR_AVERAGE_COUNT);

			colours[x - origin_x] = mean(img(roi));
		}

		iter = nextIter;
		nextIter = edge->piece()->increment(iter, 1, end);
	}

	return colours;
}

/*
vector<vector<double> > avg_colour(Edge* edge)
{
	ptIter start = edge->begin();
	ptIter end = edge->end();
	int dir = +1;

	cout << *start << " " << *end << endl;

	Point firstCorner = *start;
	Point secondCorner = *end;

	int width = abs(firstCorner.x - secondCorner.x);

	vector<vector<double> > colours;
	Mat img = edge->piece()->image();

	ptIter iter = start;
	ptIter nextIter = edge->piece()->increment(iter, dir, end);

	int start_x = firstCorner.x;
	int start_y = firstCorner.y;
	int end_x = secondCorner.x;

	cout << start_x << " " << end_x << endl;
	cout << dir << endl;

	while (nextIter != end)
	{
		int begin_x = (*iter).x;
		int end_x = (*nextIter).x;
		int begin_y = (*iter).y;
		
		if (begin_x > end_x)
		{
			int tmp = begin_x;
			begin_x = end_x;
			end_x = tmp;
		}
		
		for (int x = begin_x; x < end_x; x++)
		{
			double r = 0;
			double g = 0;
			double b = 0;

			for (int y = begin_y; y < begin_y + 50; y++)
			{
				Vec3b bgrPixel = img.at<Vec3b>(x, y);
				b += (uchar)bgrPixel.val[0];
				r += (uchar)bgrPixel.val[1];
				g += (uchar)bgrPixel.val[2];
			}

			vector<double> channel_averages (3);
			channel_averages[0] = b / 50;
			channel_averages[1] = r / 50;
			channel_averages[2] = g / 50;
			
			cout << "(" << x << ", " << begin_y << ") = ["<< channel_averages[0] << ", " << channel_averages[1] << ", " << channel_averages[2] << "] " <<endl;

			colours.push_back(channel_averages);
		}

		iter = nextIter;
		nextIter = edge->piece()->increment(iter, dir, end);
	}

	/*for (int x = start_x; x < end_x; x++)
	{
			cout << x << " " << (*iter).x << ", " << (*iter).y << endl;
		if (x >= (*iter).x && iter != end)
		{
			iter = edge->piece()->increment(iter, dir, end);
			start_y = (*iter).y;
		}	
	
		double r = 0;
		double g = 0;
		double b = 0;
		for (int y = start_y; y < start_y + 10; y++)
		{
			Vec3b bgrPixel = img.at<Vec3b>(x, y);
			b += (uchar)bgrPixel.val[0];
			r += (uchar)bgrPixel.val[1];
			g += (uchar)bgrPixel.val[2];
		}
		
		vector<double> channel_averages (3);
		channel_averages[0] = r / 10;
		channel_averages[1] = g / 10;
		channel_averages[2] = b / 10;

		//cout << "(" << x << ", " << start_y << ") = ["<< channel_averages[0] << ", " << channel_averages[1] << ", " << channel_averages[2] << "] " <<endl;

		colours.push_back(channel_averages);
	}

	cout << firstCorner << secondCorner << img.size() << endl;

	return colours;
}*/

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

		iter = pd->increment(iter, +1, edge_end);
	}
}

void drawEdge(Mat display_img, Edge* edge, Scalar color, int line_width, Point origin = Point(0, 0))
{
	drawEdge(display_img, edge->piece(), edge->index(), color, line_width, origin);
}

void display_edge(Edge* edge, string window_name, Scalar color = Scalar(0, 255, 0))
{ 
	namedWindow(window_name, CV_WINDOW_AUTOSIZE); Mat raw_img = edge->piece()->image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());

	circle(display_img, edge->piece()->origin(), 5, Scalar(0, 255, 0), -1);

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		if (i == edge->index()) continue;
		
		drawEdge(display_img, edge->piece(), i, Scalar(128, 128, 128), 1, edge->piece()->origin());
	}

	drawEdge(display_img, edge, color, 2, edge->piece()->origin());

	imshow(window_name, display_img);
}

void display_image(Mat& img, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);
	
	imshow(window_name, img);
}

void display_image_comparision(Edge* edgeIn, Edge* edgeOut, string window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edgeIn->piece()->image();
	Mat display_img;
	display_img.create(Size(1000, 1200), raw_img.type());

	Point first_origin = edgeIn->piece()->origin();
	Point second_origin = edgeOut->piece()->origin();
	Point first_midpoint = midpoint_of_line(edgeIn->getFirstCorner(), edgeIn->getSecondCorner());
	Point second_midpoint = midpoint_of_line(edgeOut->getFirstCorner(), edgeOut->getSecondCorner());

	int ydiff = edgeOut->piece()->image().size().height - (second_origin.y + second_midpoint.y);

	first_midpoint.y = 0;
	second_midpoint.y = 0;
	first_origin.y = 0;
 	second_origin.y = 0;

	Point display_offset = Point(-150, 5);

	overlayImage(display_img, edgeOut->piece()->image(), display_img, display_offset + first_midpoint + first_origin);
	overlayImage(display_img, edgeIn->piece()->image(), display_img, display_offset + Point(0, edgeOut->piece()->image().size().height - ydiff) + second_midpoint + second_origin);

	imshow(window_name, display_img);
}

void display_colour(Edge* edge, vector<Scalar>& colours, String window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = edge->piece()->image();
	Mat display_img = Mat::zeros(Size(raw_img.cols, raw_img.rows + 100), raw_img.type());

	Rect roi (0, 100, raw_img.cols, raw_img.rows);

	raw_img.copyTo(display_img(roi));
	drawEdge(display_img, edge, Scalar(0, 255, 0), 2, Point(0, 100));
	
	Point start;
	start.x = min(edge->getFirstCorner().x, edge->getSecondCorner().x);

	cout << start << endl;

	for (int i = 0; i < colours.size(); i++)
	{
		Point draw_pos = start + Point(i, 10);
		rectangle(display_img, draw_pos, draw_pos + Point(1, 50), colours[i], -1);
	}

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

	drawEdge(display_img, edgeIn, Scalar(0, 255, 0), 2, origin);
	drawEdge(display_img, edgeOut, Scalar(255, 0, 0), 2, origin);

	imshow(window_name, display_img);
}

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

	display_image_comparision(edgeIn, edgeOut, "Image Comparision");

	edgeIn->piece()->setOrigin(-edgeIn->piece()->origin());
	edgeOut->piece()->setOrigin(-edgeOut->piece()->origin());

	vector<Scalar> v1 = avg_colour(edgeIn, true);
	display_colour(edgeIn, v1, "Colours In");

	vector<Scalar> v2 = avg_colour(edgeOut, false);
	display_colour(edgeOut, v2, "Colours Out");

	edgeIn->piece()->setOrigin(edgeIn->getSecondCorner());
	edgeOut->piece()->setOrigin(edgeOut->getFirstCorner());


	display_edge_comparision(edgeIn, edgeOut, "Edge Comparision");

	double coupling_dist = coupling_distance(edgeIn, edgeOut);
	double average_min_dist = average_min_dist_measure(edgeIn, edgeOut);


	if (coupling_dist <= COUPLING_DISTANCE_THRESHOLD && average_min_dist <= AVG_MIN_DISTANCE_THRESHOLD)
	{
		cout << "MATCH" << endl;
	}
	else
	{
		cout << "Probably not match" << endl;
	}
	cout << coupling_dist << endl;
	cout << average_min_dist << endl;

	waitKey();
	return EXIT_SUCCESS;
}
