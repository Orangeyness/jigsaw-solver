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

void display_edge(PieceData pd, int edge_index, String window_name)
{
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);

	Mat raw_img = pd.image();
	Mat display_img;
	display_img.create(raw_img.size(), raw_img.type());

	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		if (i == edge_index) continue;
		drawLineList(display_img, pd.getEdgePoints(i), Scalar(128, 128, 128), 1);
	}

	drawLineList(display_img, pd.getEdgePoints(edge_index), Scalar(0, 255, 0), 2);

	imshow(window_name, display_img);
}

int main(int argc, char* argv[]) 
{
	string first_piece_filename = string(argv[1]);
	string second_piece_filename = string(argv[2]);

	int first_edge_index = atoi(argv[3]);
	int second_edge_index = atoi(argv[4]);

	PieceData pd1 (first_piece_filename);
	PieceData pd2 (second_piece_filename);

	display_edge(pd1, first_edge_index, "Edge A");
	display_edge(pd2, second_edge_index, "Edge B");

	if (pd1.getEdgeType(first_edge_index) == EDGE_TYPE_FLAT)
	{
		cout << "First edge is flat." << endl;
		goto end;
	}

	if (pd2.getEdgeType(second_edge_index) == EDGE_TYPE_FLAT)
	{
		cout << "Second edge is flat" << endl;
		goto end;
	}

	if (pd1.getEdgeType(first_edge_index) == pd2.getEdgeType(second_edge_index))
	{
		cout << "Edges same type." << endl;
		goto end;
	}

	

end:
	waitKey();

	return EXIT_SUCCESS;
}
