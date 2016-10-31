//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/ahnt/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/ahnt/MABE/wiki/License

#pragma once

#include <math.h>
#include <utility>
#include <limits>
#include <vector>

/*
 * Holds helper functions and classes for TrackWorld.  
 * Most of these should be self explanatory. 
 */
class Utils
{
public:
	~Utils() = default;

	struct Vector2D {
		double x;
		double y;

		Vector2D(double i, double j) : x(i), y(j) {}
		Vector2D(const Utils::Vector2D &other) : x(other.x), y(other.y) {}

		double GetMagnitude() { return sqrt(pow(x, 2) + pow(y, 2)); }
		double DotProduct(const Utils::Vector2D &other) { return (x * other.x + y * other.y); }

		// Allow for addition of two vectors. 
		Vector2D& operator+=(const Vector2D& rhs) { x += rhs.x; y += rhs.y; return *this; }
		friend Vector2D operator+(Vector2D lhs, Vector2D rhs) { lhs += rhs; return lhs; }

		double AngleBetween(Utils::Vector2D other) 
		{
			double dot = DotProduct(other);
			double mag_product = GetMagnitude() * other.GetMagnitude();

			return Utils::RadToDeg(acos(dot / mag_product));
		}
	};

	struct LineSegment
	{
		bool vertical; // Instead of setting the slope to infinity, we keep a boolean to track undefined slope. 
		double yIntercept;
		double slope;
		std::pair<double, double> xInterval;
		std::pair<double, double> yInterval;

		std::vector<std::pair<double, double>> Intersects(LineSegment line);
		bool ValueInBounds(double value, double lwr, double upr) { return (value >= lwr) && (value <= upr); }

		// Evaluate the line segment on an x value, if the bounds are invalid or the line is vertical we return infinity. 
		double GetY(double x) 
		{
			if (ValueInBounds(x, xInterval.first, xInterval.second) && !vertical)
				return slope * x + yIntercept;
			else
				return std::numeric_limits<double>::infinity();
		}
	};

	static double RadToDeg(double rad) { return rad / DEG_TO_RAD; }
	static double DegToRad(double deg) { return deg * DEG_TO_RAD;  }

	static constexpr double DEG_TO_RAD = 3.1415926535 / 180.0;
	static constexpr double EPSILON = 0.00000002; // Used for slope calculations. 

private:
	Utils() {}; // Cannot instantiate an instance of the utilities class (static). 
};

