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


static int BLUR_KERNEL = 131;
static double CANNY_T = 7.0;



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
    
    addWeighted(blurred_img, 3.0, src, 1, 0.0, weighted_img);
    
    imshow("weighted_img", weighted_img);
    waitKey();
    
    
    //split into rgb
    //
    split(weighted_img, rgb);
    
    imshow("R of weighted_img", rgb[2]);
    waitKey();
    
    
    //downsample
    //
    Mat downsampled_r;
    pyrDown( rgb[2], downsampled_r, Size( rgb[2].cols/2, rgb[2].rows/2 ));
    
    imshow("downsampled_r", downsampled_r);
    waitKey();
    
    //edge detect on r
    //
    Mat edges;
    Canny(downsampled_r, edges, CANNY_T, 3*CANNY_T);
    
    imshow("edges", edges);
    waitKey();
    
    
    
    //find lines in split image
    //
    /*
    HoughLinesP(rgb[2], lines, 1, CV_PI/180, 50, 50, 10 );
    for( size_t i = 0; i < lines.size(); i++ )
    {
        Vec4i l = lines[i];
        line( final_img, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
    }
     */
    
    
    //narrow down lines to ones that intersect in the image (set 1)
    
    //narrow down set 1 further to sets of three lines that have two intersects in frame (set 2)
    
    //use set 1 to find a line having intersects with the two outer lines of each entry in set 2 (set 3)
    
    //check set 3 for the pool + draw it
    
    
    //show image
    //imshow(windowName, blurred_img);
    //waitKey();
    
    return 0;
}

