#include "rsfilterthread.h"

using namespace cv;
using namespace rs2;

rsFilterThread::rsFilterThread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mlColorFrame.clear();
    mlDepthFrame.clear();
    mlTimestamp.clear();
    mutex.unlock();
}

rsFilterThread::~rsFilterThread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void rsFilterThread::startFilter()
{
    mutex.lock();
    abort = false;
    mutex.unlock();

    // depth filter
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.8f);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 50);

    start();
}

void rsFilterThread::run(){
    mutex.lock();
    abort = false;
    mutex.unlock();

    disparity_transform depth_to_disparity(true);
    disparity_transform disparity_to_depth(false);

    while(!abort){
        if (!mlColorFrame.empty() && !mlDepthFrame.empty() && !mlTimestamp.empty()){
            mutex.lock();
            frame color_frame = mlColorFrame.back();
            frame depth_frame = mlDepthFrame.back();
            qint64 timestamp = mlTimestamp.back();
            mlColorFrame.pop_back();
            mlDepthFrame.pop_back();
            mlTimestamp.pop_back();
            mutex.unlock();

            // Color frame --> Color Mat --> Emit Signal
            Mat color_mat = rsColorFrame2Mat(color_frame);
            Mat send_color = color_mat.clone();
            //emit sendColorFiltered(send_color, timestamp);

            // Depth frame --> Depth Mat --> Emit Signal
            depth_frame = depth_to_disparity.process(depth_frame);
            depth_frame = spat_filter.process(depth_frame);
            depth_frame = temp_filter.process(depth_frame);
            depth_frame = disparity_to_depth.process(depth_frame);
            Mat depth_mat = rsDepthFrame2Mat(depth_frame);
            Mat send_depth = depth_mat.clone();

            //emit sendDepthFiltered(send_depth, timestamp);
            emit sendRGBDFiltered(send_color, send_depth, timestamp);
        }
    }
}

void rsFilterThread::stop(){
    mutex.lock();
    abort = true;
    mutex.unlock();
}

cv::Mat rsFilterThread::rsDepthFrame2Mat(rs2::frame DepthFrame){
    Mat DepthMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_16UC1, (void*)DepthFrame.get_data(), Mat::AUTO_STEP);
    return DepthMat;
}

cv::Mat rsFilterThread::rsColorFrame2Mat(rs2::frame ColorFrame){
    Mat ColorMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, (void*)ColorFrame.get_data(), Mat::AUTO_STEP);
    return ColorMat;
}

void rsFilterThread::receiveColorFrame(rs2::frame ColorFrame){
    mColorFrame = ColorFrame;
    mlColorFrame.push_front(mColorFrame);
    if (mlColorFrame.size()>CAPACITY){
        mlColorFrame.pop_back();
    }
}

void rsFilterThread::receiveDepthFrame(rs2::frame DepthFrame){
    mDepthFrame = DepthFrame;
    mlDepthFrame.push_front(mDepthFrame);
    if (mlDepthFrame.size()>CAPACITY){
        mlDepthFrame.pop_back();
    }
}

void rsFilterThread::receiveRGBDFrame(rs2::frame ColorFrame, rs2::frame DepthFrame, qint64 timestamp){
    mutex.lock();
    mColorFrame = ColorFrame;
    mlColorFrame.push_front(mColorFrame);
    if (mlColorFrame.size()>CAPACITY){
        mlColorFrame.pop_back();
    }
    mDepthFrame = DepthFrame;
    mlDepthFrame.push_front(mDepthFrame);
    if (mlDepthFrame.size()>CAPACITY){
        mlDepthFrame.pop_back();
    }
    mlTimestamp.push_front(timestamp);
    if (mlTimestamp.size()>CAPACITY){
        mlTimestamp.pop_back();
    }
    mutex.unlock();
}
