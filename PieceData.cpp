#include "PieceData.h"
#include "GeometryHelpers.h"

#include <iostream>
using namespace std;

const string EDGE_DIR_NAMES[] = { "TOP", "LEFT", "BOT", "RIGHT" };
const string EDGE_TYPE_NAMES[] = { "FLAT", "IN  ", "OUT "};

PieceData::PieceData(Mat image_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	m_imageData = image_data;
	m_edgeData = edge_data;
}

PieceData::PieceData(Mat* src_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	m_edgeData = edge_data;

	//Find piece bounding rectangle
	Rect bounding_rect = contour_bounding_rect(m_edgeData);

	//Create mask for piece
	Mat mask = Mat::zeros(src_data->rows, src_data->cols, CV_8UC3);
	
	vector<vector<Point> > contours;
	contours.push_back(m_edgeData);

	drawContours(mask, contours, 0, Scalar(255, 255, 255), -1);
	
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

void resolve_filename(string name, string& image_filename, string& edge_filename)
{
	string ext = name.substr(name.size()-4, 4);

	if (ext == string(".jpg"))
	{
		image_filename = name;
		edge_filename = name.substr(0, name.size()-4) + string(".edg");
	}
	else if (ext == string(".edg"))
	{
		image_filename  = name.substr(0, name.size()-4) + string(".jpg");
		edge_filename = name;
	}
	else
	{
		image_filename = name + string(".jpg");
		edge_filename = name + string(".edg");
	}
}

PieceData::PieceData(string name) : m_cornerIndexs(4), m_edgeList(4), m_edgeType(4)
{
	string image_filename;
	string edge_filename;

	resolve_filename(name, image_filename, edge_filename);

	m_imageData = imread(image_filename);
	
	if (!m_imageData.data) throw runtime_error("Failed to load piece image");

	fstream fs (edge_filename.c_str(), fstream::in);

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
	string image_filename;
	string edge_filename;

	resolve_filename(name, image_filename, edge_filename);

	imwrite(image_filename, m_imageData);
	
	fstream fs (edge_filename.c_str(), fstream::out);

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

	vector<Point>::iterator it;

	for (it = m_edgeData.begin(); it != m_edgeData.end(); it++)
	{
		(*it).x = (*it).x - m_origin.x;
		(*it).y = (*it).y - m_origin.y;
	}
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
/*
Point PieceData:cornerTopLeft()
{
	
	for(int i = 0; i < EDGE_COUNT; i++)
	{
		
	}
}*/

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

void PieceData::splitEdges()
{
	for (int i = 0; i < EDGE_COUNT; i++) 
	{
		m_edgeList[i].clear();
	}

	int edge_index = 0;
	int next_edge_index = (edge_index + 1) % EDGE_COUNT;
	int point_index = m_cornerIndexs[0];

	while(edge_index < EDGE_COUNT)
	{
	
		m_edgeList[edge_index].push_back(m_edgeData[point_index]);
		
		if (point_index == m_cornerIndexs[next_edge_index])
		{
			edge_index ++;
			next_edge_index = (edge_index + 1) % EDGE_COUNT;
			
			if (edge_index == EDGE_COUNT)
			{
				m_edgeList[0].insert(m_edgeList[0].begin(), m_edgeData[point_index]);
			}
			else
			{
				m_edgeList[edge_index].push_back(m_edgeData[point_index]);
			}
		}

		point_index = (point_index + 1) % m_edgeData.size();
	}
}

void PieceData::rotate(double rotation)
{
	vector<Point>::iterator it;

	double cos_r = cos(rotation);
	double sin_r = sin(rotation);

	for (it = m_edgeData.begin(); it != m_edgeData.end(); it++)
	{
		double xx = (*it).x;
		double yy = (*it).y;
		
		(*it).x = (int)(xx * cos_r - yy * sin_r);
		//(*it).x * cos(rotation) - (*it).y * sin(rotation);
		(*it).y = (int)(xx * sin_r + yy * cos_r);
		//(*it).x * sin(rotation) + (*it).y * cos(rotation);
	}

	Size orig_size = m_imageData.size();
	Mat target (Size(orig_size.width + 100, orig_size.height + 100), m_imageData.type());
	Rect roi (50, 50, orig_size.width, orig_size.height);
	
	cout << roi << endl;

	m_imageData.copyTo(target(roi));
	m_origin = m_origin + Point(50, 50);


	Mat rotated_img;
	rotated_img.create(target.size(), target.type());

	Mat rot_mat = getRotationMatrix2D(m_origin, TO_DEGREE(-rotation), 1.0);
	warpAffine(target, rotated_img, rot_mat, target.size());

	Rect bounding_rect = contour_bounding_rect(m_edgeData) + m_origin;

	m_imageData = rotated_img(bounding_rect);
	m_origin = m_origin - Point(bounding_rect.x, bounding_rect.y);
}
