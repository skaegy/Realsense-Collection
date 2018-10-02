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

    // Save all the images in the list file
    // --- Color images
    QList<cv::Mat>::iterator itIm ;
    QList<std::string>::iterator itName;
    for(itIm = mlColor.begin(), itName = mlColorName.begin();
        itIm!=mlColor.end() && itName != mlColorName.end();
        itIm++, itName++){
        std::string file_name = *itName;
        cv::Mat Image = *itIm;
        imwrite(file_name, Image);
    }

    for(itIm = mlDepth.begin(), itName = mlDepthName.begin();
        itIm!=mlDepth.end() && itName != mlDepthName.end();
        itIm++, itName++){
        std::string file_name = *itName;
        cv::Mat Image = *itIm;
        imwrite(file_name, Image);
    }

    wait();
}
void rssavethread::run(){
    // To be added
}

void rssavethread::save_color_mat(cv::Mat &color_mat){
    if (!abort){
        Mat Buf_color_mat = color_mat.clone();
        char color_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
        imwrite(color_file_name, Buf_color_mat);

    }
}

void rssavethread::save_depth_mat(cv::Mat &depth_mat){
    if (!abort){
        Mat Buf_depth_mat = depth_mat.clone();
        char depth_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
        imwrite(depth_file_name, Buf_depth_mat);

    }
}

void rssavethread::save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat){
    if (!abort){
        Mat Buf_color_mat = color_mat.clone();
        Mat Buf_depth_mat = depth_mat.clone();
        char color_file_name[50];
        char depth_file_name[50];
        qint64 currTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        sprintf(color_file_name, "/home/skaegy/Data/RS/rgb/%13ld.png", currTime);
        sprintf(depth_file_name, "/home/skaegy/Data/RS/depth/%13ld.png", currTime);
        mlColor.push_back(Buf_color_mat);
        mlDepth.push_back(Buf_depth_mat);
        mlColorName.push_back(color_file_name);
        mlDepthName.push_back(depth_file_name);

        //imwrite(depth_file_name, Buf_depth_mat);
        //imwrite(color_file_name, Buf_color_mat);
    }
}
