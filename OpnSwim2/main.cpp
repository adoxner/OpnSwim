//
//  main.cpp
//  OpnSwim2
//
//  Created by Adam Oxner on 11/3/13.
//  Copyright (c) 2013 University of Michigan. All rights reserved.
//

#include <iostream>
#include <vector>



#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cvaux.h>
#include <queue>
#include <thread>
#include <utility>
#include "getIntersection.h"

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

static int NUM_LANES = 8;

static unsigned BLUR_KERNEL = 61;
static unsigned BINARY_THRESHOLD_OFFSET = 11;
static double CANNY_T = 7.2;

vector<Point> input_corners;

//Callback for mousclick event, the x-y coordinate of mouse button-up and button-down
//are stored in two points pt1, pt2.
void mouse_click(int event, int x, int y, int flags, void *param)
{
    
    switch(event)
    {
        case CV_EVENT_LBUTTONDOWN:
        {
            
            if (input_corners.size() < 4) {
                cout << "Input: (" << x << ',' << y << ")\n";
                input_corners.push_back(Point(x,y));
                if (input_corners.size() == 4)
                    cout << "Press any key to continue.\n";
            }else{
                cout << "Four points already entered!\n";
                cout << "Press any key to continue.\n";
            }
            
            break;
        }
            
    }
    
}



/** @function main */
int main( int argc, char** argv )
{
    //open file
    //const char* filename = argc >= 2 ? argv[1] : "pools/2.jpg";
    string filename = "";
    
    //debug
    filename = "pools/24.jpg";
    NUM_LANES = 1;
    
    
    //user input
    cout << "Enter file name: ";
    cin >> filename;
    cout << "Number of lanes: ";
    cin >> NUM_LANES;
    
    
    while (NUM_LANES < 1) {
        cout << "Must be at least one lane. Please try again.\nNumber of lanes: ";
        cin >> NUM_LANES;
    }
    
    src = imread(filename, 1);
    if(src.empty())
    {
        std::cout << "can not open " << filename << endl;
        return -1;
    }
    cvtColor(src, src_gray, CV_RGB2GRAY);
    
    
    
    
    //get user input for RMS testing
    //error checking
    /*
    imshow("Click corners", src);
    cvSetMouseCallback("Click corners", mouse_click, 0);
    waitKey();
     */
    
    
    
    
    //**********  CALCULATE BLUR RADIUS  ************
    
    BLUR_KERNEL =  ceil(MIN(src.rows, src.cols)/10);
    if (BLUR_KERNEL % 2 == 0) BLUR_KERNEL++;
    //BLUR_KERNEL -= 10;
    if (BLUR_KERNEL > 90) BLUR_KERNEL = 91;
    

    //cout << "BLUR_KERNEL: " << BLUR_KERNEL << '\n';
    
    
    
    //***********  GAUSSIAN BLUR  **********
    
    GaussianBlur( src, blurred_img, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );

    
    
    
    
    
    //***********  RGB SPLIT  *************
    
    split(blurred_img, rgb);

    //GaussianBlur( src_gray + rgb[0] - rgb[2] , src_gray, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    GaussianBlur( rgb[1] - rgb[2], src_gray, Size(BLUR_KERNEL, BLUR_KERNEL), 0, 0 );
    //src_gray = src_gray + rgb[0] - rgb[2];
    
    Mat temp_mat;
    GaussianBlur(src_gray, temp_mat, cv::Size(0,0), 3);
    addWeighted(src_gray, 1.5, temp_mat, -0.9, 0, temp_mat);
    src_gray = temp_mat;
    
    //imshow("src_gray", src_gray);
    
    
    
    
    
    //************  DOWNSAMPLE  *************
    
    Mat downsampled_b;
    if ( MIN(src.rows, src.cols) > 100 ) {
        pyrDown( src_gray, downsampled_b, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    }else{
        downsampled_b = src_gray;
    }
    
    
    
    //************  THRESHOLD  ***************
    //cout << "mean: " << cv::mean(downsampled_b)[0] << '\n';
    threshold(downsampled_b, downsampled_b, cv::mean(downsampled_b)[0]+BINARY_THRESHOLD_OFFSET, 255, 3);
    
    //imshow("threshold", downsampled_b);
    //waitKey();
    
    
    
    
    
    //**********  CANNY EDGE DETECTION  *************

    Mat edges;
    Canny(downsampled_b, edges, CANNY_T, 3*CANNY_T);
    
    //imshow("edges", edges);
    //waitKey();
    
    
    
    
    
    //***********  CONTOUR DETECTION  *************
    
    // find largest contour
    Mat contours_img;
    pyrDown( src, contours_img, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(downsampled_b, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    double current_largest_contour_area = 0.0;
    int that_countour = -1;
    
    for (int i=0; i<contours.size(); ++i) {
        double area = contourArea(contours[i]);
        if (area > current_largest_contour_area
            && boundingRect(contours[i]).br().y > downsampled_b.rows/2) {
            current_largest_contour_area = area;
            that_countour = i;
        }
    }
    
    edges = Mat::zeros(edges.rows, edges.cols, CV_8U);
    drawContours(edges, contours, that_countour, Scalar(255,0,255));
    //imshow("edges", edges);
    
    vector<Point> pool_contour = contours[that_countour];
    Moments m = moments(pool_contour);
    Point contour_moment = Point(m.m10/m.m00, m.m01/m.m00);
    
    Rect pool_rect = boundingRect(pool_contour);
    int pool_fix = BLUR_KERNEL/2;
    pool_rect.x -= pool_fix;
    pool_rect.y -= pool_fix;
    pool_rect.width += pool_fix*2;
    pool_rect.height += pool_fix*2;
    
    
    
    
    
    //***********  HOUGH TRANSFORM ****************
    
    //(out, lines, resolution_in_pixels, resolution_in_radians, threshold, minLinLength, maxLineGap)
    HoughLinesP(edges, lines, 3, 1*CV_PI/180, 25, BLUR_KERNEL/3, BLUR_KERNEL/8 );
    
    
    
    
    
    //drawing lines
    
    //cout << "Found " << lines.size() << " lines.\n";
    cvtColor(downsampled_b, final_img, CV_GRAY2BGR);
    pyrDown( src, final_img, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    
    /*
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        //cout << '(' << l[0] << ',' << l[1] << ") (" << l[2] << ',' << l[3] << ")\n";
        //line( final_img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,0), 2, CV_AA);
    }
    */
    
    
    
    
    
    //*********  SET 1  *************
    
    //narrow down lines to ones that intersect in the image (set 1)
    Vector<pair<Vec4i, Vec4i>> set1;
    Point temp, temp1, temp2, bounds = Point(downsampled_b.cols, downsampled_b.rows);
    
    for (size_t i=0; i<lines.size(); ++i) {
        //check to see if line is image frame
        if (lines.at(i)[0] == 1 && lines.at(i)[2] == 1) continue;
        if (lines.at(i)[0] == downsampled_b.cols && lines.at(i)[2] == downsampled_b.cols-1) continue;
        if (lines.at(i)[1] == 1 && lines.at(i)[2] == 1) continue;
        if (lines.at(i)[0] == downsampled_b.rows && lines.at(i)[2] == downsampled_b.rows-1) continue;
        
        
        for (size_t j=i+1; j<lines.size(); ++j) {
            //check to see if line is image frame
            if (lines.at(j)[0] == 1 && lines.at(j)[2] == 1) continue;
            if (lines.at(j)[0] == downsampled_b.cols && lines.at(j)[2] == downsampled_b.cols-1) continue;
            if (lines.at(j)[1] == 1 && lines.at(j)[2] == 1) continue;
            if (lines.at(j)[0] == downsampled_b.rows && lines.at(j)[2] == downsampled_b.rows-1) continue;

            
            temp = getIntersection(lines.at(i), lines.at(j));

            if (pointWithinBounds(temp, bounds) && pool_rect.contains(temp)) {
                //if it's within bounds, add it
                pair<Vec4i, Vec4i> p = pair<Vec4i, Vec4i>(lines.at(i), lines.at(j));
                set1.push_back(p);
                circle(final_img, temp, 5, Scalar(255,0,255));
            }

        }
    }
    //cout << "Added " << set1.size() << " inbounds intersections. (set1)\n";
    
    
    
    
    
    
    //************  SET 2  ************
    
    vector< vector<Point> > set2;
    double dist_limit = max(downsampled_b.cols, downsampled_b.rows)/3.0;
    double angle_threshold_low = 20.0, angle_threshold_high = 170;
    
    for (size_t i=0; i<set1.size(); ++i) {
        for (size_t j=0; j<set1.size(); ++j) {
            
            temp1 = getIntersection(set1[i].second, set1[j].first);
            temp2 = getIntersection(set1[i].first, set1[j].second);
            
            // check if whole quadrangle within bounds
            if (pointWithinBounds(temp1, bounds)
                && pointWithinBounds(temp2, bounds)
                && pool_rect.contains(temp1)
                && pool_rect.contains(temp2)) {
                
                
                Point pi = getIntersection(set1[i].first, set1[i].second);
                Point pj = getIntersection(set1[j].first, set1[j].second);
                
                double side_i1_len = norm(pi-temp1);
                double side_i2_len = norm(pi-temp2);
                double side_j1_len = norm(pj-temp2);
                double side_j2_len = norm(pj-temp1);
                
                // check if lengths are long enough
                // at least two sides must be greater than dist_limit (1/3 image size)
                if ( ((MAX(side_i1_len, side_i2_len) > dist_limit
                     && MAX(side_j1_len, side_j2_len) > dist_limit)
                    ||(MAX(side_i1_len, side_j1_len) > dist_limit
                     && MAX(side_i2_len, side_j2_len) > dist_limit))
                    && side_i1_len > dist_limit/8
                    && side_i2_len > dist_limit/8
                    && side_j1_len > dist_limit/8
                    && side_j2_len > dist_limit/8
                    && pointPolygonTest(vector<Point>{temp2, pi, temp1, pj}, contour_moment, 0) > 0)
                {
                    
                    
                    
                    // check for sharp angles?
                    double angle_i = abs(( atan2(pi.y-temp2.y, pi.x-temp2.x) - atan2(pi.y-temp1.y, pi.x-temp1.x) )*180.0/CV_PI);
                    if (angle_i > 360.0) angle_i -= 360.0;
                    if (angle_i > 180.0) angle_i -= 180.0;
                    double angle_j = abs(( atan2(pj.y-temp2.y, pj.x-temp2.x) - atan2(pj.y-temp1.y, pj.x-temp1.x) )*180.0/CV_PI);
                    if (angle_j > 360.0) angle_j -= 360.0;
                    if (angle_j > 180.0) angle_j -= 180.0;
                    double angle_t1 = abs(( atan2(temp1.y-pi.y, temp1.x-pi.x) - atan2(temp1.y-pj.y, temp1.x-pj.x) )*180.0/CV_PI);
                    if (angle_t1 > 360.0) angle_t1 -= 360.0;
                    if (angle_t1 > 180.0) angle_t1 -= 180.0;
                    double angle_t2 = abs(( atan2(temp2.y-pi.y, temp2.x-pi.x) - atan2(temp2.y-pj.y, temp2.x-pj.x) )*180.0/CV_PI);
                    if (angle_t2 > 360.0) angle_t2 -= 360.0;
                    if (angle_t2 > 180.0) angle_t2 -= 180.0;
                    
                    if (angle_i < angle_threshold_low || angle_i > angle_threshold_high) {
                        continue;
                        cout << angle_i << '\n';
                        //circle(final_img, pi, 10, Scalar(0,0,255));
                    }
                    if (angle_j < angle_threshold_low || angle_j > angle_threshold_high) {
                        continue;
                        cout << angle_j << '\n';
                        //circle(final_img, pj, 10, Scalar(0,0,255));
                    }
                    if (angle_t1 < angle_threshold_low || angle_t1 > angle_threshold_high) {
                        continue;
                        cout << angle_t1 << '\n';
                        //circle(final_img, temp1, 10, Scalar(0,0,255));
                    }
                    if (angle_t2 < angle_threshold_low || angle_t2 > angle_threshold_high) {
                        continue;
                        cout << angle_t2 << '\n';
                        //circle(final_img, temp2, 10, Scalar(0,0,255));
                    }
                    
                    
                    
                    Scalar color = Scalar(rand()%255, rand()%255, 0);
                    circle(final_img, temp1, 5, color);
                    circle(final_img, temp2, 5, color);
                    circle(final_img, pi, 5, color);
                    circle(final_img, pj, 5, color);
                    line(final_img, pi, temp2, color);
                    line(final_img, temp2, pj, color);
                    line(final_img, pj, temp1, color);
                    line(final_img, temp1, pi, color);
                    
                    set2.push_back({pi, temp1, pj, temp2});
                }
                
            }
        }
    }
    //cout << "Found " << set2.size() << " pool candidates. (set2)\n";
    
    //imshow("final_img", final_img);
    
    
    //*************  FINAL FILTER  ******************
    
    // naive solution: largest
    vector<Point> largest_pool;
    double current_largest_pool_area = 0;
    for (int i=0; i<set2.size(); ++i) {
        double area = contourArea(set2[i]);
        if (area > current_largest_pool_area) {
            current_largest_pool_area = area;
            largest_pool = set2[i];
        }
    }
    
    //if there is no pool, show the original with some marks, then return
    if (largest_pool.size() == 0){
        rectangle(final_img, pool_rect, Scalar(0,255,255));
        imshow("final_img", final_img);
        waitKey();
        return 0;
    }
    
    
    
    
    //**************  ORIENTATION  *****************

    double ll_blur = 5;
    double ll_edge = 3;
    
    //re-blur src
    Mat ll_src = rgb[1]-rgb[2];
    pyrDown( ll_src, ll_src, Size( ll_src.cols/2, ll_src.rows/2 ));
    
    GaussianBlur( ll_src, ll_src, Size(ll_blur, ll_blur), 0, 0 );
    
    //imshow("ll_src", ll_src);
    
    //edges on that
    Mat ll_edges;
    Canny(ll_src, ll_edges, ll_edge, 3*ll_edge);
    

    //imshow("ll lines", ll_edges);
    
    //(out, lines, resolution_in_pixels, resolution_in_radians, threshold, minLinLength, maxLineGap)
    HoughLinesP(ll_edges, lines, 1, 1*CV_PI/180, 20, BLUR_KERNEL/5, 3);
    vector<Vec4i> inbounds_lines;
    vector<unsigned> intersect_count = {0,0,0,0};
    
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        if ( pointPolygonTest(largest_pool, Point(l[0], l[1]), 0) > 0
            || pointPolygonTest(largest_pool, Point(l[2], l[3]), 0) > 0) {
            
            Vec4i a = {largest_pool[0].x,largest_pool[0].y,largest_pool[1].x,largest_pool[1].y},
                    b = {largest_pool[1].x,largest_pool[1].y,largest_pool[2].x,largest_pool[2].y},
                    c = {largest_pool[2].x,largest_pool[2].y,largest_pool[3].x,largest_pool[3].y},
                    d = {largest_pool[3].x,largest_pool[3].y,largest_pool[0].x,largest_pool[0].y};
            
            if (boundingRect(vector<Point>{Point(a[0],a[1]), Point(a[2],a[3])}).contains(getIntersection(l, a)) ){
                //circle(final_img, getIntersection(l, a), 4, Scalar(255,255,0));
                intersect_count[0]++;
            }
            if (boundingRect(vector<Point>{Point(b[0],b[1]), Point(b[2],b[3])}).contains(getIntersection(l, b)) ){
                //circle(final_img, getIntersection(l, b), 4, Scalar(255,255,0));
                intersect_count[1]++;
            }
            if (boundingRect(vector<Point>{Point(c[0],c[1]), Point(c[2],c[3])}).contains(getIntersection(l, c)) ){
                //circle(final_img, getIntersection(l, c), 4, Scalar(255,255,0));
                intersect_count[2]++;
            }
            if (boundingRect(vector<Point>{Point(d[0],d[1]), Point(d[2],d[3])}).contains(getIntersection(l, d)) ){
                //circle(final_img, getIntersection(l, d), 4, Scalar(255,255,0));
                intersect_count[3]++;
            }
            
            
            inbounds_lines.push_back(l);
            line( final_img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), 2, CV_AA);
        }
        
    }
    
    unsigned side_1 = intersect_count[0] +intersect_count[2], side_2 = intersect_count[1]+intersect_count[3];
    
    
    
    //cout << "side_1: " << side_1 << " side_2: " << side_2 << '\n';
    
    //***************  LABELING  *****************
    
    Mat _lane = imread("resources/_lane.jpg");
    Mat _line = imread("resources/_line.jpg");
    
    //check load
    if (_lane.empty() || _line.empty() || _lane.cols != _line.cols) {
        cout << "Error reading lane line image.\n";
        imshow("final_img", final_img);
        waitKey();
        return 2;
    }
    
    unsigned pool_height = NUM_LANES*_lane.rows + (NUM_LANES-1)*_line.rows;
    Mat text = Mat::zeros(pool_height, _lane.cols, CV_8UC3);
    unsigned current_row = 0;
    
    for (int i=0; i<NUM_LANES; ++i) {
        text(Range(current_row, current_row+_lane.rows), Range::all()) += _lane;
        current_row += _lane.rows;
        if (i < NUM_LANES-1)
            text(Range(current_row, current_row + _line.rows), Range::all()) += _line;
        current_row +=  _line.rows;
    }
    

    //const string blah = "A. Swimmer";
    //putText(text, blah, Point(0,25), FONT_HERSHEY_PLAIN, 1.7, CV_RGB(255, 255, 255));
    //imshow("text", text);
    //waitKey();
    
    final_img = src;
    int scale = 2;
    
    vector<Point2f> project_corners{Point2f(text.cols,text.rows),Point2f(0,0),Point2f(text.cols,0),Point2f(0,text.rows)};
    vector<Point2f> pool_corners1 = {largest_pool[2]*scale, largest_pool[0]*scale, largest_pool[3]*scale, largest_pool[1]*scale};
    vector<Point2f> pool_corners2 = {largest_pool[0]*scale, largest_pool[2]*scale, largest_pool[3]*scale, largest_pool[1]*scale};
    
    
    
    /// Get the Perspective Transform and plot
    
    if (largest_pool.size() > 0) {
        
        // pool ends are red
        if (side_1 >= side_2) {
            line(final_img, largest_pool[0]*scale, largest_pool[1]*scale, Scalar(0,0,255), 2);
            line(final_img, largest_pool[1]*scale, largest_pool[2]*scale, Scalar(255,255,255), 2);
            line(final_img, largest_pool[2]*scale, largest_pool[3]*scale, Scalar(0,0,255), 2);
            line(final_img, largest_pool[3]*scale, largest_pool[0]*scale, Scalar(255,255,255), 2);
            
            Mat warp_mat = getPerspectiveTransform(project_corners, pool_corners1);
            warpPerspective(text, text, warp_mat, Size(final_img.cols,final_img.rows));
            final_img = final_img + text;
        }else{
            line(final_img, largest_pool[0]*scale, largest_pool[1]*scale, Scalar(255,255,255), 2);
            line(final_img, largest_pool[1]*scale, largest_pool[2]*scale, Scalar(0,0,255), 2);
            line(final_img, largest_pool[2]*scale, largest_pool[3]*scale, Scalar(255,255,255), 2);
            line(final_img, largest_pool[3]*scale, largest_pool[0]*scale, Scalar(0,0,255), 2);
            
            Mat warp_mat = getPerspectiveTransform(project_corners, pool_corners2);
            warpPerspective(text, text, warp_mat, Size(final_img.cols,final_img.rows));
            final_img = final_img + text;
        }
        
        
    }
    
    
    //*******************  RMS TESTING  ************************
    Point inp;
    double temp_closest_d=0;
    vector<Point> pcorns= {largest_pool[2]*scale, largest_pool[0]*scale, largest_pool[3]*scale, largest_pool[1]*scale};
    vector<Point> matching_pcorns;
    double RMS = 0.0;
    int ttt=-1;
    for (int i=0; i<input_corners.size(); ++i) {
        inp = input_corners[i];
        temp_closest_d = -1;
        
        
        for (int j=0; j<pcorns.size(); j++) {
            if (temp_closest_d == -1 || norm(inp-pcorns[j])<temp_closest_d){
                temp_closest_d = norm(inp-pcorns[j]);
                ttt=j;
            }
        }
        pcorns.erase(pcorns.begin()+ttt);
        RMS += pow(temp_closest_d, 2);
    }
    
    
    if (input_corners.size() > 0){
        RMS /= 4;
        RMS = sqrt(RMS);
        RMS /= sqrt(pow(src.cols,2) + pow(src.rows,2));
        RMS *= 100;
        cout << "RMS Error: " << RMS << "%\n";
    }
    
    
    
    
    //circle(final_img, contour_moment, 7, Scalar(0,255,255));
    //rectangle(final_img, pool_rect, Scalar(0,255,255));
    

    //show image
    imshow("Result: " + filename, final_img);
    //cout << "Press any key to exit.\n";
    waitKey();
    
    return 0;
}

