#include "PieceData.h"


PieceData::PieceData(Mat image_data, vector<Point> edge_data) 
{
	m_imageData = image_data;
	m_edgeData = edge_data;
}

PieceData::PieceData(Mat* src_data, vector<Point> edge_data) 
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
	
	floodFill(mask, center, Scalar(255, 255, 255));

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

PieceData::PieceData(string name)
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

	fs.close();
}

Mat PieceData::image() 
{
	return m_imageData;
}

vector<Point> PieceData::edge()
{
	return m_edgeData;
}
