/*  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 

#include <vector>

#ifndef BEZIER_CURVE_STUFF
#define BEZIER_CURVE_STUFF

struct Vec2 
{ 
	Vec2(float a=0, float b=0) { x=a; y=b;}
	float x; float y;
};

using std::vector;

static vector<Vec2> Spline;

void Bezier(Vec2 &Out, Vec2 a, Vec2 b, Vec2 c, Vec2 d, float t);

void CalculateBezierCurve(vector<Vec2> *List,
						  float x1,float y1,float x2,float y2,
						  float x3,float y3,float x4,float y4,
						  int Sections=16);
		
void AddToSpline(const Vec2 &CV);
void CalculateBezierSpline(vector<Vec2> *List,int Sections=16);
						  
#endif
