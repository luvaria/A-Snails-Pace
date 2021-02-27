#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "subject.hpp"

static class Geometry
{
public:
	struct Line
	{
		float x0;
		float y0;
		float x1;
		float y1;
		bool operator==(const Line& rhs) const
		{
			return
				this->x0 == rhs.x0 &&
				this->x1 == rhs.x1 &&
				this->y0 == rhs.y0 &&
				this->y1 == rhs.y1;
		}
	};
	static bool segmentsOverlap(float x0, float x1, float otherx0, float otherx1);
	static bool parallelLinesIntersect(Line line, Line otherLine);
	static bool linesIntersect(Line line, Line otherLine);
	static float getIntersectingPointOnTwoParallelSegments(float x0, float otherx0);
	static vec2 intersectionOfLines(Line line, Line otherLine);
	static bool pointInsideConvexHull(vec2 point, std::vector<Line> lines);
	static bool pointIsOnLine(vec2 point, Line line);
};