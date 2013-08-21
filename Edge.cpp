#include "Edge.h"


Edge::Edge(string filename, int edge) : pd (filename)
{
	edge_index = edge;
}

Point Edge::getFirstCorner()
{
	return *(pd.getEdgeBegin(edge_index));
}

Point Edge::getSecondCorner()
{
	return *(pd.getEdgeEnd(edge_index) - 1);
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
