//
//  getIntersection.h
//  OpnSwim2
//
//  Created by Adam Oxner on 11/10/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#ifndef __OpnSwim2__getIntersection__
#define __OpnSwim2__getIntersection__

#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>

using namespace cv;

/*
 * Returns intersect of two lines. 
 * 
 * Bug: Returns (-1,-1) if parallel.
 */
Point getIntersection(Vec4i l1, Vec4i l2);

/*
 * Returns an intersection within given bounds
 */
Point intersectionWithinBounds(Vec4i l1, Vec4i l2, int rows, int cols);

/*
 * Returns true if point p is within (0,0) to bounds.
 */
bool pointWithinBounds(Point p, Point bounds);

/*
 * Returns true if point p is within lower_bounds to upper_bounds.
 */
bool pointWithinBounds(Point p, Point lower_bounds, Point upper_bounds);

/*
 * Unused k-means algorithm for finding k number of clustered points
 * from input_v.
 */
vector<Point> my_kmeans(vector<Point> input_v, int k, int threshold);


#endif /* defined(__OpnSwim2__getIntersection__) */
