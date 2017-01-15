//
//  ColorDetector.cpp
//  RubikSolver
//
//  Created by Andrei Ciobanu on 11/07/16.
//  Copyright © 2016 GTeam. All rights reserved.
//
#include "ColorDetector.hpp"

std::vector<float> ColorDetector::GetPixelFeatures(const cv::Mat &bgrImage, const cv::Mat &hsvImage, const cv::Point &location)
{
	std::vector<float> features(6);
	auto bgrPixel = bgrImage.at<cv::Vec3b>(location);
	auto hsvPixel = hsvImage.at<cv::Vec3b>(location);

	features[0] = bgrPixel.val[0];
	features[1] = bgrPixel.val[1];
	features[2] = bgrPixel.val[2];
	features[3] = hsvPixel.val[0];
	features[4] = hsvPixel.val[1];
	features[5] = hsvPixel.val[2];

	return features;
}

std::vector<std::vector<float>> ColorDetector::GetFaceFeatures(const cv::Mat &bgrImage, const cv::Mat &hsvImage)
{
	std::vector<std::vector<float>> features;
	for (int i = 0; i < bgrImage.rows; i++)
	{
		for (int j = 0; j < bgrImage.cols; j++)
		{
			auto bgrPixel = bgrImage.at<cv::Vec3b>(i, j);
			auto hsvPixel = hsvImage.at<cv::Vec3b>(i, j);

			features.push_back(std::vector<float>
			{
				static_cast<float>(bgrPixel.val[0]),
				static_cast<float>(bgrPixel.val[1]),
				static_cast<float>(bgrPixel.val[2]),
				static_cast<float>(hsvPixel.val[0]),
				static_cast<float>(hsvPixel.val[1]),
				static_cast<float>(hsvPixel.val[2])
			});
		}
	}

	return features;
}

ColorDetector::ColorDetector()
{
}

ColorDetector::~ColorDetector()
{
}

void ColorDetector::LoadSVMFromFile(const std::string& filePath)
{
	_svmClassifier = cv::Algorithm::load<cv::ml::SVM>(filePath);
}

std::vector<std::string> ColorDetector::RecognizeColors(const cv::Mat& cubeFaceImage)
{
    cv::Mat inputImage = cubeFaceImage.clone();
    
    cv::line(inputImage, cv::Point(inputImage.cols / 3, 0), cv::Point(inputImage.cols / 3, inputImage.rows), cv::Scalar(0, 0, 255), 5);
    cv::line(inputImage, cv::Point((inputImage.cols / 3) * 2, 0), cv::Point((inputImage.cols / 3) * 2, inputImage.rows), cv::Scalar(0, 0, 255), 5);
    
    cv::line(inputImage, cv::Point(0, inputImage.rows / 3), cv::Point(inputImage.cols, inputImage.rows / 3), cv::Scalar(0, 0, 255), 5);
    cv::line(inputImage, cv::Point(0, (inputImage.rows / 3) * 2), cv::Point(inputImage.cols, (inputImage.rows / 3) * 2), cv::Scalar(0, 0, 255), 5);
    
    auto sampleSize = 100;
    auto sampleDistance = (inputImage.cols / 3.0) / 2 - (sampleSize / 2);
    
    // Take samples from the image
    cv::Rect sampleRectangle = cv::Rect(sampleDistance, sampleDistance, sampleSize, sampleSize);
    std::vector<cv::Mat> faceSamples;
    
    for (size_t j = 0; j < 3; j++)
    {
        for (size_t i = 0; i < 3; i++)
        {
            sampleRectangle.x = sampleDistance + (i * inputImage.cols / 3);
            sampleRectangle.y = sampleDistance + (j * inputImage.rows / 3);
            cv::rectangle(inputImage, sampleRectangle, cv::Scalar(255, 255, 0), 2);
            
            faceSamples.push_back(cubeFaceImage(sampleRectangle));
        }
    }
    
    std::vector<std::string> stringResults;
    for (size_t sampleIndex = 0; sampleIndex < faceSamples.size(); sampleIndex++)
    {
        cv::Mat hsvImage;
        cv::cvtColor(faceSamples[sampleIndex], hsvImage, CV_BGR2HSV_FULL);
        
        auto faceFeatures = GetFaceFeatures(faceSamples[sampleIndex], hsvImage);
        cv::Mat detectionData = cv::Mat((int)faceFeatures.size(), 6, CV_32F);
        
        for (int i = 0; i < faceFeatures.size(); i++)
        {
            for (int featureIndex = 0; featureIndex < 6; featureIndex++)
            {
                detectionData.at<float>(i, featureIndex) = faceFeatures[i][featureIndex];
            }
        }
        
        cv::Mat svmResults;
        _svmClassifier->predict(detectionData, svmResults);
        
        cv::Mat svmHistogram;
        int histSize = 6;
        float range[] = { 0, 6 };
        const float* histRange = { range };
        cv::calcHist(&svmResults, 1, 0, cv::Mat(), svmHistogram, 1, &histSize, &histRange, true, false);
        
        double maxValue;
        cv::Point maxLocation;
        
        cv::minMaxLoc(svmHistogram, (double*)0, &maxValue, 0, &maxLocation);
        
        if (maxLocation.y == 0) stringResults.push_back("Y");
        if (maxLocation.y == 1) stringResults.push_back("R");
        if (maxLocation.y == 2) stringResults.push_back("B");
        if (maxLocation.y == 3) stringResults.push_back("G");
        if (maxLocation.y == 4) stringResults.push_back("W");
        if (maxLocation.y == 5) stringResults.push_back("O");
        
    }
    
    /*
     SVM Classes:
     0 - yellow
     1 - red
     2 - white
     3 - green
     4 - blue
     5 - orange
     */
    
    return stringResults;
}