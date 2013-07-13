#ifndef _PIECE_DATA_
#define _PIECE_DATA_

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace cv;
using namespace std;

class PieceData {
	private:
		Mat m_imageData;
		vector<Point> m_edgeData;

	public:
		PieceData(Mat image_data, vector<Point> edge_data);
		PieceData(Mat* src_data, vector<Point> edge_data);
		PieceData(string filename);
		void write(string filename);
	
		Mat image();
		vector<Point> edge();

	};



#endif
