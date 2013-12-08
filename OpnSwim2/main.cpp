//
//  main.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/3/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include <iostream>
#include <vector>

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


static int BLUR_KERNEL = 61;
static double CANNY_T = 7.2;
static int NUM_LANES = 8;


struct size_sort
{
    inline bool operator() (const Vec4i& struct1, const Vec4i& struct2)
    {
        Point a = {struct1[0], struct1[1]};
        Point b = {struct1[2], struct1[3]};
        Point c = {struct2[0], struct2[1]};
        Point d = {struct2[2], struct2[3]};
        
        return (norm(a-b) < norm(c-d));
    }
};
struct vec_pair_sort
{
    inline bool operator() (const pair<int, vector<Point>>& struct1, const pair<int, vector<Point>>& struct2)
    {
        return (struct1.second.size() < struct2.second.size());
    }
};


/** @function main */
int main( int argc, char** argv )
{
    //open file
    const char* filename = argc >= 2 ? argv[1] : "pools/1.jpg";
    src = imread(filename, 1);
    if(src.empty())
    {
        //help();
        std::cout << "can not open " << filename << endl;
        return -1;
    }
    cvtColor(src, src_gray, CV_RGB2GRAY);
    
    
    //calculate appropriate blur
    BLUR_KERNEL =  ceil(MIN(src.rows, src.cols)/7);
    if (BLUR_KERNEL % 2 == 0) BLUR_KERNEL++;
    if (BLUR_KERNEL > 90) BLUR_KERNEL = 91;
    cout << "BLUR_KERNEL: " << BLUR_KERNEL << '\n';
    
    
    
    //blur source image
    //
    GaussianBlur( src, blurred_img, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    //GaussianBlur( src_gray, src_gray, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    
    imshow("blurred_img", blurred_img);
    //waitKey();
    
    
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

    //GaussianBlur( src_gray+rgb[0]-rgb[2], src_gray, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    GaussianBlur( src_gray + rgb[0] - rgb[2] , src_gray, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    
    Mat temp_mat;
    GaussianBlur(src_gray, temp_mat, cv::Size(0,0), 3);
    addWeighted(src_gray, 1.5, temp_mat, -0.9, 0, temp_mat);
    src_gray = temp_mat;
    
    imshow("b of blurred_img", src_gray);
    //imshow("g", rgb[1]);
    //imshow("R of weighted_img", rgb[2]);
    //waitKey();
    
    
    
    
    //downsample
    //
    Mat downsampled_b;
    if ( MIN(src.rows, src.cols) > 100 ) {
        pyrDown( src_gray, downsampled_b, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    }else{
        downsampled_b = src_gray;
    }
    
    //pyrDown( downsampled_b, downsampled_b, Size( downsampled_b.cols/2, downsampled_b.rows/2 ));
    //downsampled_b = rgb[2];
    
    // threshold
    cout << "mean: " << cv::mean(downsampled_b) << '\n';
    threshold(downsampled_b, downsampled_b, cv::mean(downsampled_b)[0]+10, 255, 3);
    

    
    
    imshow("downsampled_b", downsampled_b);
    //waitKey();
    
    
    
    //edge detect on r
    //
    Mat edges;
    Canny(downsampled_b, edges, CANNY_T, 3*CANNY_T);
    
    imshow("edges", edges);
    //waitKey();
    
    
    
    //find lines in edges image
    //
    HoughLinesP(edges, lines, 3, CV_PI/180, 50, BLUR_KERNEL/2, 10 );
    
    
    
    //drawing lines
    cout << "Found " << lines.size() << " lines.\n";
    cvtColor(downsampled_b, final_img, CV_GRAY2BGR);
    pyrDown( src, final_img, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( final_img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,0), 3, CV_AA);
    }
    imshow("edges", edges);
    
    
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
        for (size_t j=0; j<set1.size(); ++j) {
            
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
    Vector< tuple<Vec4i, Vec4i, Vec4i, Vec4i> > set3_largest;
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
                    set3_largest.push_back(t);
                }
                
                
                set3.push_back(t);
            }
        }
    }
    cout << "Found " << set3.size() << " pool canidates. (set3)\n";
    
    if (set3.size()<1){ waitKey(); return -1;}
    
    
    
    //check set 3 for the pool + draw it
    
    
    //attempt 1: find largest area
    //
    cout << "Here's the largest:\n";
    for (int i=0; i<large.size(); ++i) {
        cout << '(' << large[i].x << ',' << large[i].y << ") ";
        circle(final_img, large[i], 4, CV_RGB(0, 255, 0));
    }
    cout << '\n';
    
    vector<Vec4i> pool_outline;
    if (large.size() == 4){
        /*
        line( final_img, large[0], large[1], Scalar(0,0,255), 2, CV_AA);
        line( final_img, large[1], large[2], Scalar(0,0,255), 2, CV_AA);
        line( final_img, large[2], large[3], Scalar(0,0,255), 2, CV_AA);
        line( final_img, large[0], large[3], Scalar(0,0,255), 2, CV_AA);
        */
        
        Vec4i sa = {large[0].x, large[0].y, large[1].x, large[1].y};
        Vec4i sb = {large[1].x, large[1].y, large[2].x, large[2].y};
        Vec4i sc = {large[2].x, large[2].y, large[3].x, large[3].y};
        Vec4i sd = {large[0].x, large[0].y, large[3].x, large[3].y};
        pool_outline.push_back(sa);
        pool_outline.push_back(sb);
        pool_outline.push_back(sc);
        pool_outline.push_back(sd);
    }
    
    //sort pool_outline by side length
    sort(pool_outline.begin(), pool_outline.end(), size_sort());
    
    
    vector<Vec4i> short_side, long_side;
    for (int i=0; i<pool_outline.size(); ++i) {
        if (i !=3 &&
             !(pool_outline[3][0] != pool_outline[i][0]
            && pool_outline[3][1] != pool_outline[i][1]
            && pool_outline[3][2] != pool_outline[i][2]
            && pool_outline[3][3] != pool_outline[i][3]
               
            && pool_outline[3][0] != pool_outline[i][2]
            && pool_outline[3][1] != pool_outline[i][3]
            && pool_outline[3][2] != pool_outline[i][0]
            && pool_outline[3][3] != pool_outline[i][1])) {
                 
                 short_side.push_back(pool_outline[i]);
                 //line( final_img, Point(pool_outline[i][0], pool_outline[i][1]), Point(pool_outline[i][2], pool_outline[i][3]), Scalar(0,255,255), 2, CV_AA);
             }else{
                 //THIS IS A 25yrd SIDE
                 //line( final_img, Point(pool_outline[i][0], pool_outline[i][1]), Point(pool_outline[i][2], pool_outline[i][3]), Scalar(0,0,255), 2, CV_AA);
                 long_side.push_back(pool_outline[i]);
             }
    }
    
    
    
    //find lane lines
    //
    double ll_blur = 5;
    double ll_edge = 8;
    
    //re-blur src
    Mat ll_src = rgb[0]-rgb[1]+rgb[2];
    pyrDown( ll_src, ll_src, Size( ll_src.cols/2, ll_src.rows/2 ));
    
    //GaussianBlur( ll_src, ll_src, Size(ll_blur, ll_blur), 0, 0 );
    
    //edges on that
    Mat ll_edges;
    Canny(ll_src, ll_edges, ll_edge, 3*ll_edge);
    
    
    //imshow("ll_src", ll_src);
    
    //get lines
    vector<Vec4i> ll_lines;
    HoughLinesP(ll_edges, ll_lines, 1, CV_PI/180, 50, 50, 10 );
    
    //imshow("ll_edges", ll_edges);
    
    
    //find line line intersections
    vector<pair<int,vector<Point>>> pool_intersects = vector<pair<int,vector<Point>>>(4);
    for (int i=0; i<ll_lines.size(); ++i) {
        
        for (int j=0; j<pool_outline.size(); ++j) {
            temp = getIntersection(ll_lines[i], pool_outline[j]);
            Point small, big;
            small = Point(min(pool_outline[j][0], pool_outline[j][2]),min(pool_outline[j][1], pool_outline[j][3]));
            big = Point(max(pool_outline[j][0], pool_outline[j][2]),max(pool_outline[j][1], pool_outline[j][3]));
            
            if (pointWithinBounds(temp, bounds)
                && pointWithinBounds(temp, small, big)){/*
                && (pointWithinBounds(Point(ll_lines[i][0], ll_lines[i][1]), small, big)
                || pointWithinBounds(Point(ll_lines[i][2], ll_lines[i][3]), small, big))) {*/
                
                circle(final_img, temp, 3, Scalar(255,255,0));
                
                pool_intersects[j].first = j;
                pool_intersects[j].second.push_back(temp);
            }
        }
    }
    imshow("final_img", final_img);
    
    //walls with most intersects are the ends
    sort(pool_intersects.begin(), pool_intersects.end(), vec_pair_sort());
    
    line( final_img, Point(pool_outline[pool_intersects[0].first][0], pool_outline[pool_intersects[0].first][1]), Point(pool_outline[pool_intersects[0].first][2], pool_outline[pool_intersects[0].first][3]), Scalar(0,255,255), 2, CV_AA);
    line( final_img, Point(pool_outline[pool_intersects[1].first][0], pool_outline[pool_intersects[1].first][1]), Point(pool_outline[pool_intersects[1].first][2], pool_outline[pool_intersects[1].first][3]), Scalar(0,255,255), 2, CV_AA);
    line( final_img, Point(pool_outline[pool_intersects[2].first][0], pool_outline[pool_intersects[2].first][1]), Point(pool_outline[pool_intersects[2].first][2], pool_outline[pool_intersects[2].first][3]), Scalar(0,0,255), 2, CV_AA);
    line( final_img, Point(pool_outline[pool_intersects[3].first][0], pool_outline[pool_intersects[3].first][1]), Point(pool_outline[pool_intersects[3].first][2], pool_outline[pool_intersects[3].first][3]), Scalar(0,0,255), 2, CV_AA);
    
    vector<Point> kmeans1, kmeans2;
    vector<Point> poolside_side;
    
    cout <<pool_intersects[0].second.size() << ' '<<pool_intersects[1].second.size() << ' '<< pool_intersects[2].second.size() << ' '<< pool_intersects[3].second.size() << endl;
    
    /*
    int my_k = NUM_LANES-1, my_threshold = 4;
    kmeans1 = my_kmeans(pool_intersects[2].second, my_k, my_threshold);
    kmeans2 = my_kmeans(pool_intersects[3].second, my_k, my_threshold);
    //kmeans(pool_intersects[2].second, 3, kmeans1, TermCriteria(), 3, KMEANS_RANDOM_CENTERS);
    //kmeans(pool_intersects[3].second, 3, kmeans2, TermCriteria(), 3, KMEANS_RANDOM_CENTERS);
    
    
    for (int i=0; i<my_k; ++i) {
        circle(final_img, kmeans1[i], 5, Scalar(255, 255, 255));
        circle(final_img, kmeans2[i], 5, Scalar(255, 255, 255));
    }
    */
    
    //label lanes
    
    Mat text = Mat::zeros(35, 200, CV_8UC3);
    text = imread("stripes_neg_10.jpg");
    const string blah = "A. Swimmer";
    putText(text, blah, Point(0,25), FONT_HERSHEY_PLAIN, 1.7, CV_RGB(255, 255, 255));
    //imshow("text", text);
    
    vector<Point2f> pool_corners{
        Point(pool_outline[0][0],pool_outline[0][1]),
        Point(pool_outline[1][0],pool_outline[1][1]),
        Point(pool_outline[2][0],pool_outline[2][1]),
        Point(pool_outline[3][2],pool_outline[3][3])
    };
    vector<Point2f> project_corners{Point2f(text.cols,text.rows),Point2f(0,0),Point2f(text.cols,0),Point2f(0,text.rows)};
    
    
    /// Get the Perspective Transform
    Mat warp_mat = getPerspectiveTransform(project_corners, pool_corners);
    warpPerspective(text, text, warp_mat, Size(final_img.cols,final_img.rows));
    //imshow("warped text", text);
    //final_img = final_img + text;

    //show image
    imshow("final_img", final_img);
    waitKey();
    
    return 0;
}

