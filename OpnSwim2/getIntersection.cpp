//
//  getIntersection.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/10/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include "getIntersection.h"


/*
 *Finds intersection of two lines
 */
Point getIntersection(Vec4i l1, Vec4i l2){
    Point intersect = Point(-1,-1);
    
    double m1 = double(l1[3] - l1[1])/double(l1[2] - l1[0]);
    double m2 = double(l2[3] - l2[1])/double(l2[2] - l2[0]);
    
    if (m1 == m2 || abs(m1-m2)<0.3) //difference in slope threshold
        return intersect;
    
    
    
    double b1 = (m1 * -l1[0]) + l1[1];
    double b2 = (m2 * -l2[0]) + l2[1];
    
    double m3 = m1-m2;
    double b3 = b2-b1;
    double x = b3/m3;
    double y = m1*x+b1;
    
    intersect.x = x;
    intersect.y = y;
    
    
    return intersect;
}

bool intersectionWithinBounds(Vec4i l1, Vec4i l2, Point bounds){
    Point temp = getIntersection(l1, l2);
    if (temp.x < 0
        || temp.y < 0
        || temp.x > bounds.x
        || temp.y > bounds.y){
        return false;
        
    }
    
    return true;
}

bool pointWithinBounds(Point p, Point bounds){
    if (p.x < 0
        || p.y < 0
        || p.x > bounds.x
        || p.y > bounds.y) {
        return false;
    }
    return true;
}