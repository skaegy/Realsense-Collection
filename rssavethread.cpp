#include "rssavethread.h"

using namespace cv;

rssavethread::rssavethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mbSavedFinish = false;
    mutex.unlock();
    mlColor.clear();
    mlDepth.clear();
    mlImageName.clear();
    mlColorName.clear();
    mlDepthName.clear();

    start();
}

rssavethread::~rssavethread()
{
    mutex.lock();
    abort = true;
    mbSavedFinish = false;
    mutex.unlock();
    wait();
}

void rssavethread::stop(){
    mutex.lock();
    abort = true;
    mutex.unlock();
}

void rssavethread::run(){
    while (!abort || !mbSavedFinish) //While application is running
    {
        /*
        if (mlColor.size()>0 && mlColorName.size()>0){

            QList<cv::Mat>::iterator itImColor = mlColor.begin();
            QList<std::string>::iterator itName = mlColorName.begin();
            QFileInfo check_color(mColor_path);
            if (!check_color.exists())
            {
                qDebug() << "Mkdir color::" << mColor_path;
                QDir color_dir = QDir::root();
                color_dir.mkpath(mColor_path);
            }

            std::string Image_name = *itName;
            cv::Mat BufColor = *itImColor;

            std::string Color_names = mColor_path.toStdString();
            imwrite(Color_names + Image_name, BufColor);

            mutex.lock();
            mlColor.pop_front();
            mlColorName.pop_front();
            mutex.unlock();

            SaveColorCnt++;
            qDebug() << "Saved Images::" << SaveColorCnt;
        }

        if (mlDepth.size()>0 && mlDepthName.size()>0){

            QList<cv::Mat>::iterator itImDepth = mlDepth.begin();
            QList<std::string>::iterator itName = mlDepthName.begin();
            QFileInfo check_depth(mDepth_path);
            if (!check_depth.exists())
            {
                qDebug() << "Mkdir depth::" << mDepth_path;
                QDir depth_dir = QDir::root();
                depth_dir.mkpath(mDepth_path);
            }

            std::string Image_name = *itName;
            cv::Mat BufDepth = *itImDepth;

            std::string Depth_names = mDepth_path.toStdString();
            imwrite(Depth_names + Image_name, BufDepth);

            mutex.lock();
            mlDepth.pop_front();
            mlDepthName.pop_front();
            mutex.unlock();
        }
        */

        usleep(100);
        if ((mlColor.size()>0) && (mlDepth.size()>0) && (mlImageName.size()>0)){
            QList<cv::Mat>::iterator itImColor = mlColor.begin();
            QList<cv::Mat>::iterator itImDepth = mlDepth.begin();
            QList<std::string>::iterator itName = mlImageName.begin();
            QFileInfo check_color(mColor_path);
            QFileInfo check_depth(mDepth_path);
            if (!check_color.exists())
            {
                qDebug() << "Mkdir color::" << mColor_path;
                QDir color_dir = QDir::root();
                color_dir.mkpath(mColor_path);
            }
            if (!check_depth.exists())
            {
                qDebug() << "Mkdir depth::" << mDepth_path;
                QDir depth_dir = QDir::root();
                depth_dir.mkpath(mDepth_path);
            }

            std::string Image_name = *itName;
            cv::Mat BufColor = *itImColor;
            cv::Mat BufDepth = *itImDepth;

            std::string Color_names = mColor_path.toStdString();
            imwrite(Color_names + Image_name, BufColor);

            std::string Depth_names = mDepth_path.toStdString();
            imwrite(Depth_names + Image_name, BufDepth);

            mutex.lock();
            mlColor.pop_front();
            mlDepth.pop_front();
            mlImageName.pop_front();
            mutex.unlock();
            mFrameTobeSaved = mlColor.size();
            SaveFrameCnt++;
            mbSavedFinish = mFrameTobeSaved > 0 ? false : true;
            if (mbSavedFinish)
                qDebug() << "Frames are all saved ....";
            //qDebug() << "Saved frames::" << SaveFrameCnt << "---" << "Saved FINISH??" << mbSavedFinish;
        }
    }
}

void rssavethread::save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp){
    if (!abort){
        Mat Buf_color_mat = color_mat.clone();
        Mat Buf_depth_mat = depth_mat.clone();
        char image_name[20];

        sprintf(image_name, "%13lld.png", timestamp);
        mutex.lock();
        mlColor.push_back(Buf_color_mat);
        mlDepth.push_back(Buf_depth_mat);
        mlImageName.push_back(image_name);
        mutex.unlock();
        ReceiveFrameCnt++;
        //qDebug() << "Received::" << ReceiveFrameCnt << "----" << timestamp;
    }
}

void rssavethread::save_color_mat(cv::Mat &color_mat, qint64 timestamp){
    Mat Buf_color_mat = color_mat.clone();
    char color_name[20];
    sprintf(color_name, "%13lld.png", timestamp);
    mutex.lock();
    mlColor.push_back(Buf_color_mat);
    mlColorName.push_back(color_name);
    mutex.unlock();
    ReceiveFrameCnt++;
    //qDebug() << "Received::" << ReceiveFrameCnt << "----" << timestamp;
}

void rssavethread::save_depth_mat(cv::Mat &depth_mat, qint64 timestamp){
    Mat Buf_depth_mat = depth_mat.clone();
    char depth_name[20];
    sprintf(depth_name, "%13lld.png", timestamp);
    mutex.lock();
    mlDepth.push_back(Buf_depth_mat);
    mlDepthName.push_back(depth_name);
    mutex.unlock();
}

void rssavethread::receive_RGBD_name(QString Subject, QString Action, QString Index){
    mColor_path = QString("/home/skaegy/Data/RGBD/%1/%2/rgb/").arg(Subject).arg(Action+Index);
    mDepth_path = QString("/home/skaegy/Data/RGBD/%1/%2/depth/").arg(Subject).arg(Action+Index);
}
