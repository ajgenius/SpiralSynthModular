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

#include "Bezier.h"

void Bezier(Vec2 &Out, Vec2 a, Vec2 b, Vec2 c, Vec2 d, float t)
{
	float tc=t*t*t;
	float ts=t*t;

	Out.x=(-a.x+3*b.x-3*c.x+d.x)*tc + (3*a.x-6*b.x+3*c.x)*ts + (-3*a.x+3*b.x)*t + a.x;
	Out.y=(-a.y+3*b.y-3*c.y+d.y)*tc + (3*a.y-6*b.y+3*c.y)*ts + (-3*a.y+3*b.y)*t + a.y;
}

void CalculateBezierCurve(vector<Vec2> *List, float x1,float y1,float x2,float y2,float x3,float y3,float x4,float y4,int Sections)
{
	Vec2 g1(x1,y1); Vec2 g2(x2,y2); Vec2 g3(x3,y3); Vec2 g4(x4,y4);

	float t=0;	
	Vec2 p(0,0),lp(0,0);
	
	for (int n=0; n<=Sections; n++)
	{		 		
		Bezier(p,g1,g2,g3,g4,t);		
		List->push_back(Vec2(p.x,p.y));
		lp.x=p.x; lp.y=p.y; 
		t+=0.1;
	}
} 

void AddToSpline(const Vec2 &CV) 
{ 
	Spline.push_back(CV); 
}

void CalculateBezierSpline(vector<Vec2> *List,int Sections)
{

	if (Spline.size()==0) return;
	
	Vec2 SplineSection[4];
	SplineSection[0].x=Spline[0].x;
	SplineSection[0].y=Spline[0].y;
	SplineSection[1].x=Spline[0].x;
	SplineSection[1].y=Spline[0].y;
	SplineSection[2].x=Spline[0].x;
	SplineSection[2].y=Spline[0].y;
	SplineSection[3].x=Spline[0].x;
	SplineSection[3].y=Spline[0].y;
	int SCount=0;
	
	for(vector<Vec2>::iterator i=Spline.begin();
		i!=Spline.end(); ++i)
	{
	
		SplineSection[3].x=i->x;
		SplineSection[3].y=i->y;									
			
		if (SCount>3)
		{
			vector<Vec2> BezierLineList;					
			CalculateBezierCurve(&BezierLineList,
								SplineSection[0].x,SplineSection[0].y,
								SplineSection[1].x,SplineSection[1].y,
			          			SplineSection[2].x,SplineSection[2].y,
								SplineSection[3].x,SplineSection[3].y,
								Sections);
						  						
			for (vector<Vec2>::iterator bi=BezierLineList.begin();
				 bi!=BezierLineList.end(); bi++)
			{					
				List->push_back(*bi);				
			}
												
			SCount=1;
		}
					
		for (int m=0; m<3; m++)
		{
			SplineSection[m].x=SplineSection[m+1].x;
			SplineSection[m].y=SplineSection[m+1].y;
		}
					
		SCount++;
	}	
	
	Spline.clear();
}
