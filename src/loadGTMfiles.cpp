/**********************************************************************************************************
FILE: loadGTMfiles.cpp

PLATFORM: Windows 7, MS Visual Studio 2010, OpenCV 2.4.2

CODE: C++

AUTOR: Josef Maier, AIT Austrian Institute of Technology

DATE: May 2017

LOCATION: TechGate Vienna, Donau-City-Stra�e 1, 1220 Vienna

VERSION: 1.0

DISCRIPTION: This file provides functionalities for loading and showing the GTMs.
**********************************************************************************************************/

#include <stdio.h>
#include "loadGTMfiles.h"
#include "io_helper.h"
#include "readGTM.h"
#include <bits/stdc++.h> 

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include <iostream>
#include <fstream>
#include <typeinfo>

using namespace std;
using namespace cv;
std::fstream outputFile;

//Show a fraction of the matches
int showMatches(std::vector <cv::DMatch> matches,
                std::vector <cv::KeyPoint> keypL,
                std::vector <cv::KeyPoint> keypR,
                cv::Mat imgs[2],
                size_t keepNMatches = 50);

/* Reads all images and GTMs and shows the matches of each image pair in the given folder
*
* string img_path    Input  -> Path to the images
* string l_img_pref	 Input  -> Prefix and/or postfix with optional subfolder for the left/first images.
*									   It can include a folder structure that follows after the filepath, a file prefix,
*									   a '*' indicating the position of the number and a postfix. If it is empty,
*									   all files from the folder img_path are used (also if l_img_pref only contains a
*									   folder ending with '/', every file within this folder is used). It is possible to
*									   specify only a prefix with or without '*' at the end. If a prefix is used, all
*									   characters until the first number (excluding) must be provided. For a
*									   postfix, '*' must be placed before the postfix. Valid examples :
*									   folder/pre_*post, *post, pre_*, pre_, folder/*post, folder/pre_*, folder/pre_,
*									   folder/, folder/folder/, folder/folder/pre_*post, ...
*									   For non stereo images (consecutive images), r_img_pref must be empty.
* string r_img_pref	 Input  -> Prefix and/or postfix with optional subfolder for the right/second images.
*									   For non stereo images (consecutive images), r_img_pref must be empty.
*									   For further details see the description of l_img_pref.
* string gtm_path		 Input  -> Path to the ground truth images. It must contain all GTM files that correspond to all
*									   image pairs specified by img_path, l_img_pref, and r_img_pref.
* string gtm_postfix Input  -> Specifies the postfix of GTM files. It must include the intended inlier
*									   ratio (10 * inlier ratio in percent) and keypoint type. E.g. 'inlRat950FAST.gtm'.
* 									 Specifying an additional folder is also possible: e.g. 'folder/*inlRat950FAST.gtm'
*
* Return value:      0:  Everything ok
*							      -1:  Error while reading GTM file
*/
int showGTM(std::string img_path, std::string l_img_pref, std::string r_img_pref,
            std::string gtm_path, std::string gtm_postfix)
{
  int             err;
  cv::Mat         src[2];
  string          fileprefl, fileprefr;
  vector <string> filenamesl, filenamesr, filenamesgtm;

  fileprefl   = l_img_pref;
  if(r_img_pref.empty())
    fileprefr = fileprefl;
  else
    fileprefr = r_img_pref;

  //Load corresponding image names
  err = loadImgStereoSequence(img_path, fileprefl, fileprefr, filenamesl, filenamesr);
  if(err || filenamesl.empty() || filenamesr.empty() || (filenamesl.size() != filenamesr.size()))
  {
    cout << "Could not find flow images! Exiting." << endl;
    exit(0);
  }

  //load GTM names
  err = loadGTMSequence(gtm_path, gtm_postfix, filenamesgtm);
  if(err || filenamesgtm.empty() || (filenamesgtm.size() != filenamesl.size()))
  {
    cout << "Could not find GTM files! Exiting." << endl;
    exit(0);
  }

  // read images and GTMs
  err                          = 0;
  std::vector<bool>          leftInlier;
  std::vector <cv::DMatch>   matchesGT;
  std::vector <cv::KeyPoint> keypL;
  std::vector <cv::KeyPoint> keypR;
  double                     inlRatioL, inlRatioR, inlRatioO, positivesGT, negativesGTl, negativesGTr, usedMatchTH;
  for(int                    k = 0; k < (int) filenamesl.size(); k++)
  {
    cv::Mat flowimg;
    src[0] = cv::imread(filenamesl[k]);
    src[1] = cv::imread(filenamesr[k]);
    if(readGTMatchesDisk(filenamesgtm[k],
                         leftInlier,
                         matchesGT,
                         keypL,
                         keypR,
                         &inlRatioL,
                         &inlRatioR,
                         &inlRatioO,
                         &positivesGT,
                         &negativesGTl,
                         &negativesGTr,
                         &usedMatchTH))
    {
      // for(int i=0; i<100; i++){
      //   cout << (keypL[i].pt.x <<", "<< keypL[i].pt.y) << std::endl;
      // }

      // ofstream MyFile("matched points.txt");      
      // for( int ii = 0; ii < keypL.size( ); ++ii ){
      //   MyFile << keypL[ii].pt.x << ", " << keypL[ii].pt.y <<std::endl;
      //   // cout << "("<< keypL[ii].pt.x << ", "<< keypL[ii].pt.y << ")" << std::endl;
      // }


      cout << "Succesfully read GTM file " << filenamesgtm[k] << endl;
      cout << "Inlier ratio in first/left image: " << inlRatioL << endl;
      cout << "Inlier ratio in second/right image: " << inlRatioR << endl;
      cout << "Mean inlier ratio of both images: " << inlRatioO << endl;
      cout << "Number of true positive matches: " << positivesGT << endl;
      cout << "Number of left negatives (having no corresponding right match): " << negativesGTl << endl;
      cout << "Number of right negatives (having no corresponding left match): " << negativesGTr << endl;
      cout << "Threshold used to generate GTM: " << usedMatchTH << endl << endl;
      showMatches(matchesGT, keypL, keypR, src, positivesGT);
    }
    else
    {
      cout << "Error while reading GTM file " << filenamesgtm[k] << endl << endl;
      err = -1;
    }
  }

  return err;
}

/* Shows a fraction of the matches.
*
* vector<DMatch> matches	Input  -> Matches
* vector<KeyPoint> keypL	Input  -> left/first keypoints
* vector<KeyPoint> keypR	Input  -> right/second keypoints
* Mat imgs[2]				Input  -> Images used for matching [0] = left/first image, [1] = right/second image
* size_t keepNMatches		Input  -> Number of matches to display [Default = 20]
*
* Return value:				 0:		  Everything ok
*/
int showMatches(std::vector <cv::DMatch> matches,
                std::vector <cv::KeyPoint> keypL, 
                std::vector <cv::KeyPoint> keypR,
                cv::Mat imgs[2],
                size_t keepNMatches)
{
  vector<char> matchesMask(matches.size(), false);
  Mat   img_correctMatches;
  float keepXthMatch;
  float oldremainder, newremainder = 0;

  // cout << 
  //Reduce number of displayed matches
  keepXthMatch   = 1.0f;
  if(matches.size() > keepNMatches)
    keepXthMatch = (float) matches.size() / (float) keepNMatches;
  oldremainder   = 0;
  for(unsigned int i = 0; i < matches.size(); i++)
  {
    newremainder = fmod((float) i, keepXthMatch);
    if(oldremainder >= newremainder)
    {
      matchesMask[i] = true;
    }
    oldremainder = newremainder;
  }

  int num = matches.size();
  vector<cv::KeyPoint> matchedPointsLeft(num), matchedPointsRight(num);
  
  ofstream TruePositive("wall_1_6_TP.txt");
  // ofstream RightTP("Wall_1_2_Right_TP.txt"); 
 
  for(int i=0; i<num; i++){
    int idxForLeft = matches[i].queryIdx;
    KeyPoint left = keypL[idxForLeft];
    matchedPointsLeft.push_back(left);
    // LeftTP << idxForLeft <<" "<< left.pt.x << ", " << left.pt.y <<std::endl;
    TruePositive << left.pt.x << ", " << left.pt.y << ", ";

    int idxForRight = matches[i].trainIdx;
    KeyPoint right = keypR[idxForRight];
    matchedPointsRight.push_back(right);
    // RightTP << idxForRight <<" " << right.pt.x << ", " << right.pt.y <<std::endl;
    TruePositive << right.pt.x << ", " << right.pt.y <<std::endl;
  }


  // Save TP points and TN points in left pic:
  // unordered_set<int> leftImageMatchedIdx;
  // for(int i=0; i<matches.size(); i++){
  //   leftImageMatchedIdx.insert(matches[i].queryIdx);
  // }
  cout << "leftMatchedSize is: " << matchedPointsLeft.size() <<std::endl;

  // ofstream LeftTP("Left_TP.txt");
  // ofstream LeftTN("Left_TN.txt");    
  // for( int ii = 0; ii < keypL.size( ); ++ii ){
  //   if(leftImageMatchedIdx.find(ii)!=leftImageMatchedIdx.end()){
  //     LeftTP << keypL[ii].pt.x << ", " << keypL[ii].pt.y <<std::endl;
  //     matchedPointsLeft.push_back(keypL[ii]);
  //   }else{
  //     LeftTN << keypL[ii].pt.x << ", " << keypL[ii].pt.y <<std::endl;
  //   }
  //   // cout << "("<< keypL[ii].pt.x << ", "<< keypL[ii].pt.y << ")" << std::endl;
  // }
  
  // Save TP points and TN points in right pic:
  // unordered_set<int> rightImageMatchedIdx;
  // for(int i=0; i<matches.size(); i++){
  //   rightImageMatchedIdx.insert(matches[i].trainIdx);
  // }
  cout << "rightMatchedSize is: " << matchedPointsRight.size() <<std::endl;

  // ofstream RightTP("Right_TP.txt");
  // ofstream RightTN("Right_TN.txt");    
  // for( int ii = 0; ii < keypR.size( ); ++ii ){
  //   if(rightImageMatchedIdx.find(ii)!=rightImageMatchedIdx.end()){
  //     RightTP << keypR[ii].pt.x << ", " << keypR[ii].pt.y <<std::endl;
  //     matchedPointsRight.push_back(keypR[ii]);
  //   }else{
  //     RightTN << keypR[ii].pt.x << ", " << keypR[ii].pt.y <<std::endl;
  //   }
  // }

  vector<Point2f> points1, points2;
  KeyPoint::convert(matchedPointsLeft, points1);
  KeyPoint::convert(matchedPointsRight, points2);
  // find essential matrix:
  cout << "points1 size is: " << points1.size() << endl;
  cout << "points2 size is: " << points2.size() << endl;
  Mat fundamental_matrix_1 = findFundamentalMat(points1, points2, FM_8POINT);
  // Mat fundamental_matrix_2 = findFundamentalMat(points2, points1, FM_8POINT);
  cout << "M1 = "<< endl <<" "<< fundamental_matrix_1 <<endl;
  // cout << "M2 = "<< endl <<" "<< fundamental_matrix_2 <<endl;


  //Draw true positive matches
  drawMatches(imgs[0],
              keypL,
              imgs[1],
              keypR,
              matches,
              img_correctMatches,
              Scalar::all(-1),
              Scalar::all(-1),
              matchesMask,
              cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

  //Show result
  // cvNamedWindow("Ground Truth Matches");
  // cout << "M = " << endl << " "  << img_correctMatches << endl << endl;
  imshow("Ground Truth Matches", img_correctMatches);
  waitKey(0);
  cv::destroyWindow("Ground Truth Matches");

  return 0;
}
