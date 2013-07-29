#ifndef _EDGE_
#define _EDGE_

#include <list>

#include "PieceData.h"

class Edge
{
	private:
		PieceData pd;
		int edge_index;

	public:
		Edge(string filename, int edge);
		list<Point>* points();
		Point firstPoint();
		Point lastPoint();
		int index();
		int type();
		PieceData* piece();
};

#endif
