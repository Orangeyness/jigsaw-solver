#include "PieceData.h"

#include <iostream>
using namespace std;

const string EDGE_DIR_NAMES[] = { "TEAL", "GREEN", "RED", "WHITE" };
const string EDGE_TYPE_NAMES[] = { "FLAT", "IN", "OUT"};

PieceData::PieceData(Mat image_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	m_imageData = image_data;
	m_edgeData = edge_data;
}

PieceData::PieceData(Mat* src_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	m_edgeData = edge_data;

	//Find piece bounding rectangle
	vector<Point> contour_poly;
	approxPolyDP(Mat(m_edgeData), contour_poly, 3, true);
	Rect bounding_rect = boundingRect(Mat(contour_poly));

	//Create mask for piece
	Mat mask = Mat::zeros(src_data->rows, src_data->cols, CV_8UC3);
	
	vector<vector<Point> > contours;
	contours.push_back(m_edgeData);

	drawContours(mask, contours, 0, Scalar(255, 255, 255), -1);
	
	Point center = Point(	bounding_rect.x + bounding_rect.width/2,
				bounding_rect.y + bounding_rect.height/2
				);
	
	//Copy and crop the piece
	bitwise_and(mask, *src_data, m_imageData);
	m_imageData = m_imageData(bounding_rect);

	//Crop edge_data info
	for (int i = 0; i < m_edgeData.size(); i++) 
	{
		m_edgeData[i].x = m_edgeData[i].x - bounding_rect.x;
		m_edgeData[i].y = m_edgeData[i].y - bounding_rect.y;
	}

}

PieceData::PieceData(string name) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	stringstream image_filename;
	stringstream edge_filename;

	image_filename << name << ".jpg";
	edge_filename << name << ".edg";

	m_imageData = imread(image_filename.str());
	
	if (!m_imageData.data) throw runtime_error("Failed to load piece image");

	fstream fs (edge_filename.str().c_str(), fstream::in);

	int point_count, x, y;
	fs >> point_count;

	for (int i = 0; i < point_count; i++) 
	{
		fs >> x;
		fs >> y;

		m_edgeData.push_back(Point(x, y));
	}

	fs >> x;
	fs >> y;

	m_origin = Point(x, y);

	bool non_zero = false;

	for (int i = 0; i < 4; i++)
	{
		fs >> x;
		fs >> y;
		m_cornerIndexs[i] = x;
		m_edgeType[i] = y;
		
		if (x != 0) non_zero = true;
	}

	if (non_zero) splitEdges();

}


void PieceData::write(string name) 
{
	stringstream image_filename;
	stringstream edge_filename;

	image_filename << name << ".jpg";
	edge_filename << name << ".edg";
	
	imwrite(image_filename.str(), m_imageData);
	
	fstream fs (edge_filename.str().c_str(), fstream::out);

	fs << m_edgeData.size() << endl;
	for (int i = 0; i < m_edgeData.size(); i++) 
	{
		fs << m_edgeData[i].x << " " << m_edgeData[i].y << endl;
	}

	fs << m_origin.x << " " << m_origin.y << endl;
	
	for (int i = 0; i < m_cornerIndexs.size(); i++)
	{
		fs << m_cornerIndexs[i] << " " << m_edgeType[i] << endl;
	}

	fs.close();
}

void PieceData::setOrigin(Point origin)
{
	m_origin = origin;
}

void PieceData::setCornerIndexs(vector<int> indexs)
{
	m_cornerIndexs = indexs;
	
	splitEdges();
}

void PieceData::setEdgeType(int edge, int type)
{
	m_edgeType[edge] = type;
}

Mat PieceData::image() 
{
	return m_imageData;
}

vector<Point> PieceData::edge()
{
	return m_edgeData;
}

Point PieceData::origin()
{
	return m_origin;
}

int PieceData::getCornerIndex(int num)
{
	return m_cornerIndexs[num];
}


list<Point>* PieceData::getEdgePoints(int num)
{
	return &m_edgeList[num];
}

int PieceData::getEdgeType(int num)
{
	return m_edgeType[num];
}

// Doesn't work quite yet...
void PieceData::fixRotation(double rotation)
{
	rotation = TO_RAD(rotation);

	cout << rotation << endl;

	int rotation_origin_x = m_edgeData[m_cornerIndexs[0]].x;
	int rotation_origin_y = m_edgeData[m_cornerIndexs[0]].y;

	for (int i = 0; i < m_edgeData.size(); i++) 
	{
		m_edgeData[i].x = rotation_origin_x + (m_edgeData[i].x - rotation_origin_x)*cos(rotation) - (m_edgeData[i].x - rotation_origin_x)*sin(rotation);
		m_edgeData[i].y = rotation_origin_y + (m_edgeData[i].y - rotation_origin_y)*sin(rotation) + (m_edgeData[i].y - rotation_origin_y)*cos(rotation);
	}

	splitEdges();
}

void PieceData::splitEdges()
{
	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		m_edgeList[i].clear();
	}

	int edge_index = 0;
	bool insert_end = true;
	list<Point>::iterator it;
	for (int i = 0; i < m_edgeData.size(); i++) 
	{
		if (insert_end)
		{
			m_edgeList[edge_index].push_back(m_edgeData[i]);
		}
		else
		{
			m_edgeList[edge_index].insert(it, m_edgeData[i]);
		}


		if (i == m_cornerIndexs[edge_index])
		{
			edge_index = (edge_index + 1) % 4;
			if (edge_index == 0)
			{
				insert_end = false;
				it = m_edgeList[edge_index].begin();
			}

			i--;
		}
	}
	
}
