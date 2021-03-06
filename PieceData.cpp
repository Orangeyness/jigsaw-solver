#include "PieceData.h"
#include "GeometryHelpers.h"

#include <iostream>
using namespace std;

const string EDGE_DIR_NAMES[] = { "TOP", "LEFT", "BOT", "RIGHT" };
const string EDGE_TYPE_NAMES[] = { "FLAT", "IN  ", "OUT "};

PieceData::PieceData(Mat image_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeType(4)
{
	m_imageData = image_data;
	m_edgeData = edge_data;
}


// Just realised how memory bad this is. TODO: Move masking shit back out of here or something.
PieceData::PieceData(Mat* src_data, vector<Point> edge_data) : m_cornerIndexs(4), m_edgeType(4)
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

PieceData::PieceData(string name) : m_cornerIndexs(4), m_edgeType(4)
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

Point PieceData::getTopRightCorner()
{
	return m_edgeData[m_cornerIndexs[CORNER_TOPRIGHT]];
}
Point PieceData::getTopLeftCorner()
{
	return m_edgeData[m_cornerIndexs[CORNER_TOPLEFT]];
}
Point PieceData::getBotLeftCorner()
{
	return m_edgeData[m_cornerIndexs[CORNER_BOTLEFT]];
}
Point PieceData::getBotRightCorner()
{
	return m_edgeData[m_cornerIndexs[CORNER_BOTRIGHT]];
}

ptIter PieceData::getEdgeBegin(int num)
{
	vector<Point>::iterator it;
	it = m_edgeData.begin();
	it += m_cornerIndexs[num];

	return it;
}

ptIter PieceData::getEdgeEnd(int num)
{
	vector<Point>::iterator it;
	it = m_edgeData.begin();
	it += m_cornerIndexs[(num + 1) % EDGE_COUNT] + 1;

	return it;
}

ptIter PieceData::begin()
{
	return m_edgeData.begin();	
}

ptIter PieceData::end()
{
	return m_edgeData.end();
}

int PieceData::getEdgeType(int num)
{
	return m_edgeType[num];
}

// Helper function to increment an iterator taking into account
// it wrapping around the vector until it finds the end. 
ptIter PieceData::increment(ptIter iter, int dir, ptIter end)
{
	ptIter piece_end = this->end();
	ptIter piece_begin = this->begin();

	iter += dir;

	if (iter == piece_end && piece_end != end)
	{
		iter = piece_begin;	
	}		
	else if (iter == piece_begin && piece_begin != end)
	{
		iter = piece_end;	
	}		

	return iter;
}

#define ROTATION_PADDING 200
#define ROTATION_PADDING_OFFSET Point(ROTATION_PADDING, ROTATION_PADDING)
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
	Mat target = Mat::zeros(Size(orig_size.width + ROTATION_PADDING*2, orig_size.height + ROTATION_PADDING*2), m_imageData.type());
	Rect roi (ROTATION_PADDING, ROTATION_PADDING, orig_size.width, orig_size.height);
	
	m_imageData.copyTo(target(roi));
	m_origin = m_origin + ROTATION_PADDING_OFFSET;

	Mat rotated_img;
	rotated_img.create(target.size(), target.type());

	Mat rot_mat = getRotationMatrix2D(m_origin, TO_DEGREE(-rotation), 1.0);
	warpAffine(target, rotated_img, rot_mat, target.size());

	Rect bounding_rect = contour_bounding_rect(m_edgeData) + m_origin;
	bounding_rect.x -= 5;
	bounding_rect.y -= 5;
	bounding_rect.width += 10;
	bounding_rect.height += 10;

	m_imageData = rotated_img(bounding_rect);
	m_origin = m_origin - Point(bounding_rect.x, bounding_rect.y);
}
