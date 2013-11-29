//
//  main.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/3/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include <iostream>

// Put images in /Users/adoxner/Documents/School/EECS 442/Final project/OpnSwim2/DerivedData/OpnSwim2/Build/Products



#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <queue>
#include <thread>
#include <utility>
#include "getIntersection.h"
#include "Quadrilateral.h"

using namespace cv;
using namespace std;


string windowName = "find the pool...";

Mat src;            //original
Mat src_gray;       //gray
Mat blurred_img;    //blurred
Mat final_img;      //output
vector<Mat> rgb;    //rgb split {b,g,r}
Mat edges_img;      //edges

vector<Vec4i> lines;


static int BLUR_KERNEL = 71;
static double CANNY_T = 8.0;



/** @function main */
int main( int argc, char** argv )
{
    //open file
    const char* filename = argc >= 2 ? argv[1] : "pool.jpg";
    src = imread(filename, 1);
    if(src.empty())
    {
        //help();
        std::cout << "can not open " << filename << endl;
        return -1;
    }
    cvtColor(src, src_gray, CV_RGB2GRAY);
    
    
    //blur source image
    //
    Mat weighted_img;
    GaussianBlur( src, blurred_img, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    
    imshow("blurred_img", blurred_img);
    waitKey();
    
    
    /*
    //combine with source
    //addWeighted(blurred_img, 3, src, 0.5, 0.0, weighted_img);
    weighted_img = blurred_img;
    
    imshow("weighted_img", weighted_img);
    waitKey();
     */
    
    
    //split into rgb
    //
    split(blurred_img, rgb);
    
    imshow("b of blurred_img", rgb[0]);
    //imshow("g", rgb[1]);
    //imshow("R of weighted_img", rgb[2]);
    waitKey();
    
    
    //downsample
    //
    Mat downsampled_b;
    pyrDown( rgb[0], downsampled_b, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    //downsampled_b = rgb[0];
    
    imshow("downsampled_b", downsampled_b);
    waitKey();
    
    
    //edge detect on r
    //
    Mat edges;
    Canny(downsampled_b, edges, CANNY_T, 3*CANNY_T);
    
    imshow("edges", edges);
    waitKey();
    
    
    
    //find lines in edges image
    //
    HoughLinesP(edges, lines, 1, CV_PI/180, 50, 50, 10 );
    
    
    
    //drawing lines
    cout << "Found " << lines.size() << " lines.\n";
    cvtColor(downsampled_b, final_img, CV_GRAY2BGR);
    pyrDown( blurred_img, final_img, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( final_img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }
    
    
    
    //narrow down lines to ones that intersect in the image (set 1)
    //
    Vector<pair<Vec4i, Vec4i>> set1;
    Point temp, bounds = Point(downsampled_b.cols, downsampled_b.rows);
    double dist_limit = max(downsampled_b.cols, downsampled_b.rows)/3.0;
    for (size_t i=0; i<lines.size(); ++i) {
        for (size_t j=i+1; j<lines.size(); ++j) {
            
            
            temp = getIntersection(lines.at(i), lines.at(j));
            //cout << "x: " << temp.x << " y: " << temp.y << '\n';
            
            //circle(final_img, temp, 4, CV_RGB(0, 255, 0));
            
            if (pointWithinBounds(temp, bounds)) {
                //if it's within bounds, add it
                pair<Vec4i, Vec4i> p = pair<Vec4i, Vec4i>(lines.at(i), lines.at(j));
                set1.push_back(p);
            }
            
        }
    }
    cout << "Added " << set1.size() << " inbounds intersections. (set1)\n";
    
    
    //narrow down set 1 further to sets of three lines that have two intersects in frame (set 2)
    //
    Vector< pair< pair<Vec4i, Vec4i>, pair<Vec4i, Vec4i> > > set2;
    
    for (size_t i=0; i<set1.size(); ++i) {
        for (size_t j=i+1; j<set1.size(); ++j) {
            
            temp = getIntersection(set1[i].second, set1[j].first);
            
            Point pi = getIntersection(set1[i].first, set1[i].second);
            Point pj = getIntersection(set1[j].first, set1[j].second);
            
            double dista1 = norm(temp - pi);
            double dista2 = norm(temp - pj);
            double hy =     norm(pi - pj);
            
            if (pointWithinBounds(temp, bounds)
                && set1[i].second != set1[j].first
                && dista1 > dist_limit
                && dista2 > dist_limit
                && hy > dista1
                && hy > dista2) {
                
                //if it's within bounds, add it
                pair< pair<Vec4i, Vec4i>, pair<Vec4i, Vec4i> > p =
                    pair< pair<Vec4i, Vec4i>, pair<Vec4i, Vec4i> >(
                        set1[i], set1[j]);
                
                set2.push_back(p);
            }
            
        }
    }
    cout << "Added " << set2.size() << " inbounds corner pairs. (set2)\n";
    
    imshow("final_img", final_img);
    
    
    
    
    //use set 1 to find a line having intersects with the two outer lines of each entry in set 2 (set 3)
    //
    Vector< tuple<Vec4i, Vec4i, Vec4i, Vec4i> > set3;
    double current_largest_area = 0.0;
    vector<Point> large;
    
    for (size_t i=0; i<set2.size(); ++i) {
        for (size_t j=i+1; j<set2.size(); ++j) {
            temp = getIntersection(set2[i].first.first, set2[j].second.second);
            if (pointWithinBounds(temp, bounds) && set2[i].first.first != set2[j].second.second) {
                
                tuple<Vec4i, Vec4i, Vec4i, Vec4i> t =
                    tuple<Vec4i, Vec4i, Vec4i, Vec4i>(set2[i].first.first, set2[i].first.second,
                                                      set2[j].second.first, set2[j].second.second);
                
                
                
                Point a = getIntersection(get<0>(t), get<1>(t));
                Point b = getIntersection(get<1>(t), get<2>(t));
                Point c = getIntersection(get<2>(t), get<3>(t));
                Point d = getIntersection(get<0>(t), get<3>(t));
                
                if (!((a.x < downsampled_b.cols/2 || b.x < downsampled_b.cols/2 || c.x < downsampled_b.cols/2 || d.x < downsampled_b.cols)
                    &&(a.x > downsampled_b.cols/2 || b.x > downsampled_b.cols/2 || c.x > downsampled_b.cols/2 || d.x > downsampled_b.cols)
                    &&(a.y < downsampled_b.rows/2 || b.y < downsampled_b.rows/2 || c.y < downsampled_b.rows/2 || d.y < downsampled_b.rows)
                    &&(a.y > downsampled_b.rows/2 || b.y > downsampled_b.rows/2 || c.y > downsampled_b.rows/2 || d.y > downsampled_b.rows))
                    || !pointWithinBounds(a, bounds)
                    || !pointWithinBounds(b, bounds)
                    || !pointWithinBounds(c, bounds)
                    || !pointWithinBounds(d, bounds)) {
                    continue;
                }
                
                vector<Point> set3_contours;
                set3_contours.push_back(a);
                set3_contours.push_back(b);
                set3_contours.push_back(c);
                set3_contours.push_back(d);
                
                double the_area = contourArea(set3_contours);
                if (the_area > current_largest_area) {
                    current_largest_area = the_area;
                    large = set3_contours;
                }
                
                
                set3.push_back(t);
            }
        }
    }
    cout << "Found " << set3.size() << " pool canidates. (set3)\n";
    
    
    cout << "Here's the largest:\n";
    for (int i=0; i<large.size(); ++i) {
        cout << '(' << large[i].x << ',' << large[i].y << ") ";
        circle(final_img, large[i], 4, CV_RGB(0, 255, 0));
    }
    cout << '\n';
    
    
    
    //check set 3 for the pool + draw it
    
    
    //attempt 1: find largest area
    
    
    
    //show image
    imshow("final_img", final_img);
    waitKey();
    
    return 0;
}

