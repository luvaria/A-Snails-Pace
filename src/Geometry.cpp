// Header
#include "geometry.hpp"
#include "tiny_ecs.hpp"
#include "debug.hpp"

// stlib
#include <iostream>
#include <cassert>

//given two line segments, (x0,x1) and (otherx0, otherx1) which are on the same line, see if they overlap
bool Geometry::segmentsOverlap(float x0, float x1, float otherx0, float otherx1)
{
	if (x0 <= otherx0)
	{
		return (x1 >= otherx0);
	}
	else
	{
		return (otherx1 >= x0);
	}
}

//given two parallel lines, return true iff they intersect
bool Geometry::parallelLinesIntersect(Geometry::Line line, Geometry::Line otherLine)
{
	//special case when both lines are vertical.
	if (line.x0 == line.x1)
	{
		assert(otherLine.x0 == otherLine.x1);
		if (line.x0 != otherLine.x0)
		{
			return false;
		}
		return segmentsOverlap(line.y0, line.y1, otherLine.y0, otherLine.y1);
	}
	float slope1 = (line.y1 - line.y0) / (line.x1 - line.x0);
	float slope2 = (otherLine.y1 - otherLine.y0) / (otherLine.x1 - otherLine.x0);
	float c1 = line.y0 - slope1 * line.x0;
	float c2 = otherLine.y0 - slope2 * otherLine.x0;
	assert(abs(slope1 - slope2) < 0.01); //account for rounding error
	if (c1 != c2)
	{
		//then they aren't the same line, so they don't intersect
		return false;
	}
	return segmentsOverlap(line.x0, line.x1, otherLine.x0, otherLine.x1);
}

// find a solution to the two lines using matrix algebra.
// there's only 1 point of intersection unless the lines are parallel, so by solving Ax = b, we will have the unique solution, and 
// we can simply check if our r,t values are in [0,1]x[0,1]
// if A is not invertible, then this means the lines are parallel, so we do some extra checks then.
bool Geometry::linesIntersect(Geometry::Line line, Geometry::Line otherLine)
{
	float a11 = line.x1 - line.x0;
	float a12 = otherLine.x0 - otherLine.x1;
	float a21 = line.y1 - line.y0;
	float a22 = otherLine.y0 - otherLine.y1;
	mat2 A = mat2{ {a11, a21}, {a12, a22} }; //each {} is a column, not a row.
	float b1 = otherLine.x0 - line.x0;
	float b2 = otherLine.y0 - line.y0;
	vec2 b = vec2{ b1, b2 };
	float det = glm::determinant(A);
	if (det != 0)
	{
		//invertible
		vec2 x = glm::inverse(A) * b;
		return (0 <= x[0] && x[0] <= 1 && 0 <= x[1] && x[1] <= 1); //(x0,x1) in [0,1]x[0,1]
	}
	else
	{
		// lines are parallel
		return parallelLinesIntersect(line, otherLine);
	}
}

float Geometry::getIntersectingPointOnTwoParallelSegments(float x0, float otherx0)
{
	if (x0 <= otherx0)
	{
		return otherx0;
	}
	else
	{
		return x0;
	}
}

//returns the point of intersection of the lines that intersect
vec2 Geometry::intersectionOfLines(Geometry::Line line, Geometry::Line otherLine)
{
	assert(linesIntersect(line, otherLine));
	float a11 = line.x1 - line.x0;
	float a12 = otherLine.x0 - otherLine.x1;
	float a21 = line.y1 - line.y0;
	float a22 = otherLine.y0 - otherLine.y1;
	mat2 A = mat2{ {a11, a21}, {a12, a22} };
	float b1 = otherLine.x0 - line.x0;
	float b2 = otherLine.y0 - line.y0;
	vec2 b = vec2{ b1, b2 };
	float det = glm::determinant(A);
	if (det != 0)
	{
		//invertible, and lines intersect, so we have a unique solution.
		vec2 x = glm::inverse(A) * b;
		assert(0 <= x[0] && x[0] <= 1 && 0 <= x[1] && x[1] <= 1); //(x0,x1) in [0,1]x[0,1]
		float x_point = line.x0 + x[0] * (line.x1 - line.x0);
		float y_point = line.y0 + x[0] * (line.y1 - line.y0);
		float x_point_other = otherLine.x0 + x[1] * (otherLine.x1 - otherLine.x0);
		float y_point_other = otherLine.y0 + x[1] * (otherLine.y1 - otherLine.y0);
		assert(abs(x_point_other - x_point) < 0.01); //accout for rounding error
		assert(abs(y_point_other - y_point) < 0.01); //accout for rounding error
		return vec2(x_point, y_point);
	}
	else
	{
		//lines are parallel
		if (line.x0 == line.x1)
		{
			//both lines are vertical
			assert(otherLine.x0 == otherLine.x1);
			assert(line.x0 == otherLine.x0);
			float yval = getIntersectingPointOnTwoParallelSegments(line.y0, otherLine.y0);
			return vec2(line.x0, yval);
		}
		else
		{
			//lines are not vertical
			float xval = getIntersectingPointOnTwoParallelSegments(line.x0, otherLine.x0);
			float yval = (xval == line.x0) ? line.y0 : otherLine.y0;
			return vec2(xval, yval);
		}
	}
}

//given a point, return true if it is inside the convex hull
bool Geometry::pointInsideConvexHull(vec2 point, std::vector<Geometry::Line> lines)
{
	std::vector<bool> sides = std::vector<bool>();
	for (auto line : lines)
	{
		float A = line.y1 - line.y0; //dy
		float B = line.x0 - line.x1; //-dx
		if (B == 0)
		{
			if (line.y1 > line.y0)
			{
				//bottom to top
				bool pointIsOnRHSOfVerticalLine = point.x >= line.x0;
				sides.push_back(pointIsOnRHSOfVerticalLine);
			}
			else
			{
				//top to bottom
				bool pointIsOnRHSOfVerticalLine = point.x < line.x0;
				sides.push_back(pointIsOnRHSOfVerticalLine);
			}
		}
		else
		{
			float C = -(A * line.x0 + B * line.y0);//-(Ax + By)
			float C_other = -(A * line.x1 + B * line.y1);
			assert(abs(C - C_other) < 0.01); //should be the same for either point, accounting for rounding error
			float valueAtPoint = A * point.x + B * point.y + C;
			bool pointIsOnRHSOfLine = valueAtPoint >= 0;
			sides.push_back(pointIsOnRHSOfLine);
		}
	}

	//check if all the sides are the same
	if (std::all_of(sides.begin(), sides.end(), [sides](bool val) {return val == sides[0]; }))
	{
		return true;
	}
	return false;
}

//accounts for rounding error, is meant to double check your intersection code was correct.
//do not use this function for anything other than double checking, as it is not precise.
bool Geometry::pointIsOnLine(vec2 point, Geometry::Line line)
{
	//get an equation for the line of the form Ax + By = C, and plug in the point.
	float A = line.y1 - line.y0; //dy
	float B = line.x0 - line.x1; //-dx
	if (B == 0)
	{
		//the line is vertical
		//check if the x-coordinate is the same, and the y-coordinate lies in the desired range.
		bool xValIsMatching = abs(point.x - line.x0) < 0.01; //account for rounding error
		bool yValIsInRange = ((min(line.y0, line.y1) - 0.01) < point.y) && (point.y < (max(line.y0, line.y1) + 0.01));
		return xValIsMatching && yValIsInRange;
	}
	else
	{
		float C = -(A * line.x0 + B * line.y0);//-(Ax + By)
		float C_other = -(A * line.x1 + B * line.y1);
		assert(abs(C - C_other) < 0.01); //should be the same for either point
		float valueAtPoint = A * point.x + B * point.y + C;
		return (abs(valueAtPoint) < 0.01); //account for rounding error
	}
}
