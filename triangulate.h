////////////////////////////////////////////////////////////////////////////////
///	\file	triangulate.h
///
///	\author	ELREG, most of the code from
///         https://www.flipcode.com/archives/Efficient_Polygon_Triangulation.shtml
///
///	\brief	Implementation of the Triangulate class
///         which  triangulates any polygon without hole.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////
#ifndef TRIANGULATE_H
#define TRIANGULATE_H

#include <vector>
#include "../OpenGLBaseLib/imagetexture.h"
#include "../OpenGLBaseLib/vertexbuffer.h"

typedef std::vector<GenericVertexData> Vector2dVector;

////////////////////////////////////////////////////////////////////////////////
/// \brief Triangulate - Class representing polygon triangulation.
////////////////////////////////////////////////////////////////////////////////
class Triangulate
{
public:

	// Triangulates a contour/polygon and places results in a vector
	// as series of triangles
	static bool Process(const Vector2dVector &contour,
						Vector2dVector &result);

	// Computes area of a contour/polygon
	static float Area(const Vector2dVector &contour);

	// Calculates whether point P is inside triangle defined by
	// (Ax,Ay) (Bx,By) (Cx,Cy) or not
	static bool InsideTriangle(float Ax, float Ay,
							   float Bx, float By,
							   float Cx, float Cy,
							   float Px, float Py);

private:
	static bool Snip(const Vector2dVector &contour,int u,int v,int w,int n,int *V);
};

#endif // TRIANGULATE_H

