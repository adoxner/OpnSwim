//
//  Quadrilateral.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/10/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include "Quadrilateral.h"
#include "getIntersection.h"



bool lineCompare (const Vec4i l1, const Vec4i l2){
    double m1 = double(l1[3] - l1[1])/double(l1[2] - l1[0]);
    double m2 = double(l2[3] - l2[1])/double(l2[2] - l2[0]);
        
    return m1 < m2;
}

Quadrilateral::Quadrilateral(){
    
}


Quadrilateral::Quadrilateral(Vec4i l1, Vec4i l2, Vec4i l3, Vec4i l4, int rows, int cols, vector<Quadrilateral> &quads, vector<thread> &threads){
    
    bounds.x = cols;
    bounds.y = rows;
    
    sides.push_back(l1);
    sides.push_back(l2);
    sides.push_back(l3);
    sides.push_back(l4);
    

    while (next_permutation(sides.begin(), sides.end(), lineCompare)) {
        
        Point p1 = getIntersection(sides.at(0), sides.at(1));
        if (!pointWithinBounds(p1, bounds))
            continue;
        Point p2 = getIntersection(sides.at(1), sides.at(2));
        if (!pointWithinBounds(p2, bounds))
            continue;
        Point p3 = getIntersection(sides.at(2), sides.at(3));
        if (!pointWithinBounds(p3, bounds))
            continue;
        Point p4 = getIntersection(sides.at(3), sides.at(0));
        if (!pointWithinBounds(p4, bounds))
            continue;
        
        
        break;
    }
    
    corners.push_back(getIntersection(sides.at(0), sides.at(1)));
    corners.push_back(getIntersection(sides.at(1), sides.at(2)));
    corners.push_back(getIntersection(sides.at(2), sides.at(3)));
    corners.push_back(getIntersection(sides.at(3), sides.at(0)));
    inBounds = true;
    
    for (int ll=0; ll<4; ll++) {
        if (!pointWithinBounds(corners.at(ll), bounds)) {
            inBounds = false;
            return;
        }
    }
    
    

    quads.push_back(*this);
    cout << quads.size() << '\n';
    
}

void Quadrilateral::solve(Vec4i l1, Vec4i l2, Vec4i l3, Vec4i l4, int rows, int cols, vector<Quadrilateral> &quads){
    
    bounds.x = cols;
    bounds.y = rows;
    
    sides.push_back(l1);
    sides.push_back(l2);
    sides.push_back(l3);
    sides.push_back(l4);
    
    
    while (next_permutation(sides.begin(), sides.end(), lineCompare)) {
        
        Point p1 = getIntersection(sides.at(0), sides.at(1));
        if (!pointWithinBounds(p1, bounds))
            continue;
        Point p2 = getIntersection(sides.at(1), sides.at(2));
        if (!pointWithinBounds(p2, bounds))
            continue;
        Point p3 = getIntersection(sides.at(2), sides.at(3));
        if (!pointWithinBounds(p3, bounds))
            continue;
        Point p4 = getIntersection(sides.at(3), sides.at(0));
        if (!pointWithinBounds(p4, bounds))
            continue;
        
        
        break;
    }
    
    corners.push_back(getIntersection(sides.at(0), sides.at(1)));
    corners.push_back(getIntersection(sides.at(1), sides.at(2)));
    corners.push_back(getIntersection(sides.at(2), sides.at(3)));
    corners.push_back(getIntersection(sides.at(3), sides.at(0)));
    inBounds = true;
    
    for (int ll=0; ll<4; ll++) {
        if (!pointWithinBounds(corners.at(ll), bounds)) {
            inBounds = false;
            return;
        }
    }
    
    quads.push_back(*this);
    
}




vector<Point> Quadrilateral::getCorners(){
    return corners;
}

bool Quadrilateral::isWithinBounds(){
    
    return inBounds;
}

