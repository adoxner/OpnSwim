//
//  Quadrilateral.h
//  OpnSwim2
//
//  Created by Adam Oxner on 11/10/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#ifndef __OpnSwim2__Quadrilateral__
#define __OpnSwim2__Quadrilateral__

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <thread>
using namespace std;
using namespace cv;

class Quadrilateral{
private:
    vector<Vec4i> sides;
    vector<Point> corners;
    Point bounds;
    bool inBounds;
public:
    Quadrilateral(Vec4i l1, Vec4i l2, Vec4i l3, Vec4i l4, int rows, int cols, vector<Quadrilateral> &quads, vector<thread> &threads);
    Quadrilateral();
    vector<Point> getCorners();
    bool isWithinBounds();
    void solve(Vec4i l1, Vec4i l2, Vec4i l3, Vec4i l4, int rows, int cols, vector<Quadrilateral> &quads);
};

#endif /* defined(__OpnSwim2__Quadrilateral__) */
