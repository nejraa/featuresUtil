////////////////////////////////////////////////////////////////////////////////
///	\file	triangulate.cpp
///
///	\author	ELREG, most of the code from
///         https://www.flipcode.com/archives/Efficient_Polygon_Triangulation.shtml
///
///	\brief	Implementation of the Triangulate class
///         which triangulates any polygon without hole.
///
///	(C) Kelvin Hughes, 2020.
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "triangulate.h"

static const float EPSILON = 0.0000000001f; ///< Used to denote a small quantity, error offset.

////////////////////////////////////////////////////////////////////////////////
/// \fn float Triangulate::Area(const Vector2dVector &contour)
///
/// \brief  Computes an area of contour.
///
/// \param  contour - Contour area.
///
/// \return Returns calculated area of polygon.
////////////////////////////////////////////////////////////////////////////////
float Triangulate::Area(const Vector2dVector &contour)
{
	int n = contour.size();
	float A = 0.0f;

	for(int p = n-1, q = 0; q < n; p = q++)
	{
		A+= contour[p].position().x()*contour[q].position().y() -
				contour[q].position().x()*contour[p].position().y();
	}
	return A*0.5f;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn bool Triangulate::InsideTriangle(float Ax, float Ay,
///                                      float Bx, float By,
///                                      float Cx, float Cy,
///                                      float Px, float Py)
///
/// \brief  Checks if a point P is inside the triangle
///         defined by points A, B, and C.
///
/// \param  Ax - Value on xAxis for point A.
///         Ay - Value on yAxis for point A.
///         Bx - Value on xAxis for point B.
///         By - Value on yAxis for point B.
///         Cx - Value on xAxis for point C.
///         Cy - Value on yAxis for point C.
///         Px - Value on xAxis for point P.
///         Py - Value on yAxis for point P.
///
/// \return Returns true when the point lies inside triangle,
///         otherwise false.
////////////////////////////////////////////////////////////////////////////////
bool Triangulate::InsideTriangle(float Ax, float Ay,
								 float Bx, float By,
								 float Cx, float Cy,
								 float Px, float Py)

{
	float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
	float cCROSSap, bCROSScp, aCROSSbp;

	ax = Cx - Bx;  ay = Cy - By;
	bx = Ax - Cx;  by = Ay - Cy;
	cx = Bx - Ax;  cy = By - Ay;
	apx= Px - Ax;  apy= Py - Ay;
	bpx= Px - Bx;  bpy= Py - By;
	cpx= Px - Cx;  cpy= Py - Cy;

	aCROSSbp = ax*bpy - ay*bpx;
	cCROSSap = cx*apy - cy*apx;
	bCROSScp = bx*cpy - by*cpx;

	return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};

////////////////////////////////////////////////////////////////////////////////
/// \fn bool Triangulate::Snip(const Vector2dVector &contour,int u,int v,int w,int n,int *V)
///
/// \brief  Checks if consecutive points can form closed polygon.
///
/// \param  contour - Contour area.
///         u - Three consecutive vertices in polygon - previous
///         v - Three consecutive vertices in polygon - current
///         w - Three consecutive vertices in polygon - next
///         n - Number of vertices in polygon
///         V - Vector of polygon points as series of Triangles
///
/// \return Returns true if given consequtive points can be points of a polygon
///         (polygon created successfully), otherwise false.
////////////////////////////////////////////////////////////////////////////////
bool Triangulate::Snip(const Vector2dVector &contour,int u,int v,int w,int n,int *V)
{
	int p;
	float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

	Ax = contour[V[u]].position().x();
	Ay = contour[V[u]].position().y();

	Bx = contour[V[v]].position().x();
	By = contour[V[v]].position().y();

	Cx = contour[V[w]].position().x();
	Cy = contour[V[w]].position().y();

	if ( EPSILON > (((Bx-Ax)*(Cy-Ay)) - ((By-Ay)*(Cx-Ax))) )
		return false;

	for (p = 0; p < n; p++)
	{
		if( (p == u) || (p == v) || (p == w) )
			continue;
		Px = contour[V[p]].position().x();
		Py = contour[V[p]].position().y();
		if (InsideTriangle(Ax,Ay,Bx,By,Cx,Cy,Px,Py))
			return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/// \fn bool Triangulate::Process(const Vector2dVector &contour, Vector2dVector &result)
///
/// \brief  Triangulate a contour/polygon and places results in a vector
///         as series of triangles.
///
/// \param  contour - Contour area.
///         result  - A vector containing a series of triangles.
///
/// \return Returns true if a polygon created successfully.
////////////////////////////////////////////////////////////////////////////////
bool Triangulate::Process(const Vector2dVector &contour, Vector2dVector &result)
{
	// allocate and initialize list of Vertices in polygon
	int n = contour.size();
	if ( n < 3 ) return false;

	int *V = new int[n];

	// a counter-clockwise polygon in V
	if ( 0.0f < Area(contour) )
	{
		for (int v = 0; v < n; v++)
			V[v] = v;
	}
	else
	{
		for(int v = 0; v < n; v++)
			V[v] = (n-1)-v;
	}

	int nv = n;
	int count = 2*nv;

	// removes nv-2 Vertices, creating one triangle every time
	for(int m = 0, v = nv-1; nv > 2; )
	{
		// check whether it is a non-simple polygon or not
		if (0 >= (count--))
		{
			//Triangulate error - a non-simple polygon (probably bad polygon)
			return false;
		}

		// three consecutive vertices in current polygon, <u,v,w>
		int u = v;
		if (nv <= u)
			u = 0;
		v = u+1;
		if (nv <= v)
			v = 0;
		int w = v+1;
		if (nv <= w)
			w = 0;

		if ( Snip(contour, u, v, w, nv, V) )
		{
			int a,b,c,s,t;

			// true names of the vertices
			a = V[u];
			b = V[v];
			c = V[w];

			// output Triangle
			result.push_back( contour[a] );
			result.push_back( contour[b] );
			result.push_back( contour[c] );
			m++;

			// remove v from remaining polygon
			for(s = v, t = v+1; t < nv; s++, t++)
				V[s] = V[t];
			nv--;

			// resest error detection counter
			count = 2*nv;
		}
	}
	delete[] V;
	return true;
}
