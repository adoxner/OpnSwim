//
//  getIntersection.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/10/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include "getIntersection.h"

double getAngle(Vec4i l1, Vec4i l2){
    
    double a1 =  atan2(l1[3] - l1[1], l1[2] - l1[0]) * 180.0 / CV_PI;
    double a2 =  atan2(l2[3] - l2[1], l2[2] - l2[0]) * 180.0 / CV_PI;
    return a1-a2;
}

/*
 *Finds intersection of two lines
 */
Point getIntersection(Vec4i l1, Vec4i l2){
    Point intersect = Point(-1,-1);
    
    double m1 = double(l1[3] - l1[1])/double(l1[2] - l1[0]);
    double m2 = double(l2[3] - l2[1])/double(l2[2] - l2[0]);
    
    

    if (m1 == m2)// || (abs(getAngle(l1, l2)) >178 && abs(getAngle(l1, l2))<182))
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

bool pointWithinBounds(Point p, Point lower_bounds, Point upper_bounds){
    if (p.x < lower_bounds.x
        || p.y < lower_bounds.y
        || p.x > upper_bounds.x
        || p.y > upper_bounds.y) {
        return false;
    }
    return true;
}

Point v_avg(vector<Point> input){
    int x_sum = 0, y_sum = 0;
    for (int i=0; i<input.size(); ++i) {
        x_sum += input[i].x;
        y_sum += input[i].y;
    }
    return Point(x_sum/input.size(), y_sum/input.size());
}

vector<Point> my_kmeans(vector<Point> input_v, int k, int threshold){
    
    vector<Point> current_means;
    vector<Point> new_means = vector<Point>(k);
    vector<vector<Point>> cluster_values = vector<vector<Point>>(k);
    
    //get k random points
    for (int i=0; i<k; ++i) {
        int r;

        while(true){
            r = rand()%input_v.size();

            bool temp_check = true;
            for (int j=0; j<current_means.size(); ++j) {
                if (current_means.at(j) == input_v.at(r)) {
                    temp_check = false;
                    break;
                }
            }
            if (temp_check) break;
        }
            
        current_means.push_back(input_v.at(r));
    }
    
    while (true) {
        
        //group points
        for (int i=0; i<input_v.size(); ++i) {
            
            int closest_mean = -1, dist = -1;
            for (int j=0; j<k; ++j) {
                int temp_dist = norm(input_v[i] - current_means[j]);
                if (dist == -1 || temp_dist < dist) {
                    dist = temp_dist;
                    closest_mean = j;
                }
            }
            cluster_values.at(closest_mean).push_back(input_v.at(i));
        }
        
        //calculate new means
        for (int i=0; i<k; ++i) {
            new_means[i] = v_avg(cluster_values[i]);
        }
        
        //if new means == old means (convergence), break
        bool real_okay = false;
        for (int i=0; i<k; ++i) {
            bool temp_okay = false;
            
            for (int j=0; j<threshold; ++j) {
                for (int jj=0; jj<threshold; ++jj) {
                    if (new_means[i].x+j == current_means[i].x+jj
                        && new_means[i].y+j == current_means[i].y+jj) {
                        temp_okay = true;
                        break;
                    }
                }
                if (temp_okay) break;
            }
            
            if (i==k-1 && temp_okay) real_okay = true;
        }
        
        if (real_okay)
            break;
        
        current_means = new_means;
        
    }
    
    return current_means;
}








