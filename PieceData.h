#ifndef _PIECE_DATA_
#define _PIECE_DATA_

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <list>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

#define PI 3.14159265
#define TO_DEGREE(X) (X * 180.0 / PI)
#define TO_RAD(x) (x * PI / 180.0)

#define EDGE_COUNT 4
#define EDGE_TYPE_FLAT 0
#define EDGE_TYPE_IN 1
#define EDGE_TYPE_OUT 2

#define EDGE_TOP 0
#define EDGE_LEFT 1
#define EDGE_BOT 2
#define EDGE_RIGHT 3

#define CORNER_TOPRIGHT 0
#define CORNER_TOPLEFT 1
#define CORNER_BOTLEFT 2
#define CORNER_BOTRIGHT 3

using namespace cv;
using namespace std;

extern const string EDGE_DIR_NAMES[4];
extern const string EDGE_TYPE_NAMES[3];

typedef vector<Point>::iterator ptIter;

class PieceData {
	private:
		Mat m_imageData;
		vector<Point> m_edgeData;
		vector<int> m_cornerIndexs;
		vector<int> m_edgeType;
		Point m_origin;


	public:
		PieceData(Mat image_data, vector<Point> edge_data);
		PieceData(Mat* src_data, vector<Point> edge_data);
		PieceData(string filename);

		void setOrigin(Point origin);
		void setCornerIndexs(vector<int> indexs);
		void setEdgeType(int edge, int type);

		void rotate(double rotation);

		void write(string filename);
	
		Mat image();
		vector<Point> edge();
		Point origin();
		int getCornerIndex(int num);

		Point getTopLeftCorner();
		Point getTopRightCorner();
		Point getBotLeftCorner();
		Point getBotRightCorner();

		ptIter begin();
		ptIter end();
		ptIter getEdgeBegin(int num);
		ptIter getEdgeEnd(int num);
		ptIter increment(ptIter iter, int dir, ptIter end);
		
		int getEdgeType(int num);
	};



#endif
