#include "rscapturethread.h"

using namespace cv;
using namespace rs2;

extern const int queueCapacity = 3;
extern rs2::frame_queue original_color_queue(queueCapacity);
extern rs2::frame_queue original_depth_queue(queueCapacity);
extern rs2::frame_queue filtered_color_queue(queueCapacity);
extern rs2::frame_queue flitered_depth_queue(queueCapacity);

rsCaptureThread::rsCaptureThread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}

rsCaptureThread::~rsCaptureThread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void rsCaptureThread::run(){
    mutex.lock();
    abort = false;
    mutex.unlock();

    // Open RS
    config rs_cfg;
    rs_cfg.enable_stream(RS2_STREAM_DEPTH, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_Z16, IN_FRAME); // Enable default depth
    rs_cfg.enable_stream(RS2_STREAM_COLOR, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_BGR8, IN_FRAME);
    //rs_cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    //rs_cfg.enable_stream(RS2_STREAM_INFRARED, 2);
    colorizer color_map;
    pipeline_profile rs_device = pipe.start(rs_cfg);
    device selected_device = rs_device.get_device();
    auto depth_sensor = selected_device.first<rs2::depth_sensor>();
    if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)){
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f);
    }
    if (depth_sensor.supports(RS2_OPTION_LASER_POWER)){
        auto range = depth_sensor.get_option_range(RS2_OPTION_LASER_POWER);
        depth_sensor.set_option(RS2_OPTION_LASER_POWER, range.max);
    }
    align align(RS2_STREAM_COLOR);


    while(!abort){
        frameset rs_d415 = pipe.wait_for_frames();
        frameset align_d415 = align.process(rs_d415);
        frame depth_frame = align_d415.get_depth_frame();
        frame color_frame = align_d415.get_color_frame();
        //mutex.lock();
        original_color_queue.enqueue(color_frame);
        original_depth_queue.enqueue(depth_frame);
        //mutex.unlock();
    }
}

void rsCaptureThread::startCollect()
{
    start();
}

void rsCaptureThread::stop(){
    abort = true;
    //pipeline pipe;
    //pipe.start();
    pipe.stop();
}

cv::Mat rsCaptureThread::rsDepthFrame2Mat(rs2::frame DepthFrame){
    Mat DepthMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_16UC1, (void*)DepthFrame.get_data(), Mat::AUTO_STEP);
    return DepthMat;
}

cv::Mat rsCaptureThread::rsColorFrame2Mat(rs2::frame ColorFrame){
    Mat ColorMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, (void*)ColorFrame.get_data(), Mat::AUTO_STEP);
    return ColorMat;
}


/*
Class: rsFilteredThread
*/
rsFilteredThread::rsFilteredThread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}

rsFilteredThread::~rsFilteredThread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}

void rsFilteredThread::startFilter()
{
    mutex.lock();
    abort = false;
    mutex.unlock();
    start();
}

void rsFilteredThread::run(){
    mutex.lock();
    abort = false;
    mutex.unlock();

    // depth filter
    spatial_filter spat_filter;    //
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);
    temporal_filter temp_filter;   //
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.8);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 50);
    disparity_transform depth_to_disparity(true);
    disparity_transform disparity_to_depth(false);
    colorizer color_map;

    while(!abort){

        //mutex.lock();
        auto depth_frame = original_depth_queue.wait_for_frame();
        auto color_frame = original_color_queue.wait_for_frame();
        //mutex.unlock();

        depth_frame = depth_to_disparity.process(depth_frame);
        depth_frame = spat_filter.process(depth_frame);
        depth_frame = temp_filter.process(depth_frame);
        depth_frame = disparity_to_depth.process(depth_frame);
        mutex.lock();
        filtered_color_queue.enqueue(color_frame);
        flitered_depth_queue.enqueue(depth_frame);
        mutex.unlock();

        Mat color_mat = rsColorFrame2Mat(color_frame);
        Mat depth_mat = rsDepthFrame2Mat(depth_frame);
        depth_mat.convertTo(depth_mat, CV_16UC1);
        //auto depth_show = color_map(depth_frame);
        //Mat depth_show_mat = rsColorFrame2Mat(depth_show);

        emit sendColorFiltered(color_mat);
        emit sendDepthFiltered(depth_mat);
        //emit sendDepthShow(depth_show_mat);

    }
}

void rsFilteredThread::stop(){
    mutex.lock();
    abort = true;
    mutex.unlock();
}

cv::Mat rsFilteredThread::rsDepthFrame2Mat(rs2::frame DepthFrame){
    Mat DepthMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_16UC1, (void*)DepthFrame.get_data(), Mat::AUTO_STEP);
    return DepthMat;
}

cv::Mat rsFilteredThread::rsColorFrame2Mat(rs2::frame ColorFrame){
    Mat ColorMat(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3, (void*)ColorFrame.get_data(), Mat::AUTO_STEP);
    return ColorMat;
}
