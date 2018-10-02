#include "rssavethread.h"
using namespace cv;
rssavethread::rssavethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mutex.unlock();
}
rssavethread::~rssavethread()
{
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}
void rssavethread::stop(){
    mutex.lock();
    abort = true;
    mutex.unlock();
    wait();
}
void rssavethread::run(){
    // To be added
}

void rssavethread::save_color_mat(cv::Mat &color_mat){
    if (!abort){
        Mat Buf_color_mat = color_mat.clone();
        //Mat Buf_color_mat = color_mat->clone();
        char color_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
        imwrite(color_file_name, Buf_color_mat);
    }
}

void rssavethread::save_depth_mat(cv::Mat &depth_mat){
    if (!abort){
        Mat Buf_depth_mat = depth_mat.clone();
        //Mat Buf_depth_mat = depth_mat->clone();
        char depth_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
        imwrite(depth_file_name, Buf_depth_mat);
    }
}

void rssavethread::save_color_frame(rs2::frame& color_frame){
    if (!abort){
        Mat Buf_color_mat(cv::Size(640, 480), CV_8UC3, (void*)color_frame.get_data(), Mat::AUTO_STEP);
        char color_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
        imwrite(color_file_name, Buf_color_mat);
    }
}

void rssavethread::save_depth_frame(rs2::frame& depth_frame){
    if (!abort){
        Mat Buf_depth_mat(cv::Size(640, 480), CV_16UC1, (void*)depth_frame.get_data(), Mat::AUTO_STEP);
        char depth_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
        imwrite(depth_file_name, Buf_depth_mat);
    }
}
