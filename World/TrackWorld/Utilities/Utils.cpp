//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/ahnt/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/ahnt/MABE/wiki/License

#include "Utils.h"

using std::pair; using std::make_pair;
using std::vector;

/**
 * Returns the point of intersection or bounds of intersection within two points.
 * Returned vector may be empty. 
 */
vector<pair<double, double>> Utils::LineSegment::Intersects(Utils::LineSegment line) 
{
	vector<pair<double, double>> intersection;
	
	bool bothVertical = vertical && line.vertical;
	bool parallel = slope == line.slope;

	// Both lines are vertical or parallel, so they will intersect at an interval rather than just one point. 
	if (bothVertical || parallel) 
	{
		if ((bothVertical && xInterval.first == line.xInterval.first) || (parallel && yIntercept == line.yIntercept))
		{
			bool lowerInBounds = ValueInBounds(yInterval.first, line.yInterval.first, line.yInterval.second);
			bool upperInBounds = ValueInBounds(yInterval.second, line.yInterval.first, line.yInterval.second);

			if (lowerInBounds && upperInBounds)
			{
				intersection.push_back(make_pair(xInterval.first, yInterval.first));
				intersection.push_back(make_pair(xInterval.first, yInterval.second));
			}
			else if (lowerInBounds)
			{
				intersection.push_back(make_pair(xInterval.first, yInterval.first));
				intersection.push_back(make_pair(xInterval.first, line.yInterval.second));
			}
			else if (upperInBounds)
			{
				intersection.push_back(make_pair(xInterval.first, line.yInterval.first));
				intersection.push_back(make_pair(xInterval.first, yInterval.second));
			}
			else if (ValueInBounds(line.yInterval.first, yInterval.first, yInterval.second) &&
				ValueInBounds(line.yInterval.second, yInterval.first, yInterval.second))
			{
				intersection.push_back(make_pair(xInterval.first, line.yInterval.first));
				intersection.push_back(make_pair(xInterval.first, line.yInterval.second));
			}
		}

	}
	
	// Both lines are not vertical or parallel, so there will only be one point of intersection.

	else if (vertical)
	{
		double y = line.GetY(xInterval.first);

		// Make sure the returned y is valid. 
		if (!(isinf(y) || !line.ValueInBounds(y, yInterval.first, yInterval.second)))
			intersection.push_back(make_pair(xInterval.first, y));
	}
	else if (line.vertical)
	{
		double y = GetY(line.xInterval.first);

		// Make sure the returned y is valid.
		if (!(isinf(y) || !ValueInBounds(y, line.yInterval.first, line.yInterval.second)))
			intersection.push_back(make_pair(line.xInterval.first, y));
	}
	else // A normal intersection.
	{
		double xIntersect = (line.yIntercept - yIntercept) / (slope - line.slope); // Division is safe since we checked for parallel. 

		if (ValueInBounds(xIntersect, xInterval.first, xInterval.second) &&
			ValueInBounds(xIntersect, line.xInterval.first, line.xInterval.second))
			intersection.push_back(make_pair(xIntersect, GetY(xIntersect))); // Don't need to check for infinity from GetY since the xIntersect is within bounds.
	}

	return intersection;
}