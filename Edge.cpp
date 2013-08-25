#include "Edge.h"


Edge::Edge(string filename, int edge) : pd (filename)
{
	edge_index = edge;
}

Point Edge::getFirstCorner()
{
	return *begin();
}

Point Edge::getSecondCorner()
{
	return *end();
}

ptIter Edge::begin()
{
	return pd.getEdgeBegin(edge_index);
}

ptIter Edge::end()
{
	return pd.getEdgeEnd(edge_index);
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
