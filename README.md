jigsaw-solver
=============

To solve jigsaw puzzles.

Currently split into 3 programs,
#Segmenter
	Splits a picture of a jigsaw puzzle up into the individual jigsaw pieces. 
	Pieces are stored as a masked, cropped porition of the original image and a .edg file which contains 
	information about the edge of the piece.

#PieceClassifier
	Takes an individual piece output from the segmenter, finds the corners of the piece and uses that to 
	seperate the edge into four sides. It then classifys each edge as either flat, in or out. 

#EdgeMatcher
	Matches edges (or will soon).
