#include "Edge.h"


Edge::Edge(string filename, int edge) : pd (filename)
{
	edge_index = edge;
}

list<Point>* Edge::points()
{
	return pd.getEdgePoints(edge_index);
}

Point Edge::firstPoint()
{
	list<Point>* p = points();
	return p->front();
}

Point Edge::lastPoint()
{
	list<Point>* p = points();
	return p->back();
}

int Edge::index()
{
	return edge_index;
}
	
int Edge::type()
{
	return pd.getEdgeType(edge_index);
}

PieceData* Edge::piece()
{
	return &pd; 
}
