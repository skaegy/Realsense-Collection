#include "rscapturethread.h"

using namespace cv;
using namespace rs2;

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
    align alignRGBD(RS2_STREAM_COLOR);
    disparity_transform depth_to_disparity(true);
    disparity_transform disparity_to_depth(false);

    while(!abort){
        usleep(100);
        frameset rs_d415 = pipe.wait_for_frames();
        //qint64 t1 = QDateTime::currentDateTime().toMSecsSinceEpoch();
        frameset align_d415 = alignRGBD.process(rs_d415);
        //qint64 t2 = QDateTime::currentDateTime().toMSecsSinceEpoch();
        //qDebug() << "Align time = " << t2 - t1;
        frame depth_frame = align_d415.get_depth_frame();
        frame color_frame = align_d415.get_color_frame();
        qint64 rsTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

        emit sendRGBDFrame(color_frame, depth_frame, rsTime);

        /*
        depth_frame = depth_to_disparity.process(depth_frame);
        depth_frame = spat_filter.process(depth_frame);
        depth_frame = temp_filter.process(depth_frame);
        depth_frame = disparity_to_depth.process(depth_frame);

        Mat color_mat = rsColorFrame2Mat(color_frame);
        Mat depth_mat = rsDepthFrame2Mat(depth_frame);

        emit sendRGBDMat(color_mat, depth_mat, rsTime);
        */
    }
}

void rsCaptureThread::startCollect()
{
    // Initialize the Realsense configuration
    // --- Camera parameters
    rs_cfg.enable_stream(RS2_STREAM_DEPTH, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_Z16, IN_FRAME); // Enable default depth
    rs_cfg.enable_stream(RS2_STREAM_COLOR, IMG_WIDTH, IMG_HEIGHT, RS2_FORMAT_BGR8, IN_FRAME);
    rs_device = pipe.start(rs_cfg);
    selected_device = rs_device.get_device();
    depth_sensor = selected_device.first<rs2::depth_sensor>();

    if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)){
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f);
    }

    if (depth_sensor.supports(RS2_OPTION_LASER_POWER)){
        auto range = depth_sensor.get_option_range(RS2_OPTION_LASER_POWER);
        depth_sensor.set_option(RS2_OPTION_LASER_POWER, range.max);
    }

    // --- Filter parameters
    /*
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.8f);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 50);
    */

    start();
}


void rsCaptureThread::stop(){
    abort = true;
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
