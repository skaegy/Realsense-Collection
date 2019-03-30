#include "rssavethread.h"

using namespace cv;

rssavethread::rssavethread(QObject* parent)
    : QThread(parent)
{
    mutex.lock();
    abort = false;
    mbSavedFinish = false;
    mutex.unlock();

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

void rssavethread::startSave(){
    mlColor.clear();
    mlDepth.clear();
    mlImageName.clear();
    mlColorName.clear();
    mlDepthName.clear();

    start();
}

void rssavethread::run(){
    while (!abort || !mbSavedFinish) //While application is running
    {
        usleep(100);
        if (!mlColor.empty() && !mlDepth.empty() && !mlImageName.empty()){
            //QList<cv::Mat>::iterator itImColor = mlColor.begin();
            //QList<cv::Mat>::iterator itImDepth = mlDepth.begin();
            //QList<std::string>::iterator itName = mlImageName.begin();
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


            // Here
            cv::Mat BufColor = mlColor.front();//*itImColor;
            cv::Mat BufDepth = mlDepth.front();//*itImDepth;
            std::string Image_name = mlImageName.front(); //*itName;

            std::string Color_names = mColor_path.toStdString();
            imwrite(Color_names + Image_name + ".jpg", BufColor);

            std::string Depth_names = mDepth_path.toStdString();
            imwrite(Depth_names + Image_name + ".png", BufDepth);

            mutex.lock();
            mlColor.pop_front();
            mlDepth.pop_front();
            mlImageName.pop_front();
            mutex.unlock();
            mFrameTobeSaved = mlDepth.size() > mlColor.size() ? mlDepth.size() : mlColor.size();
            SaveFrameCnt++;
            mbSavedFinish = mFrameTobeSaved > 0 ? false : true;
            if (abort && mbSavedFinish){
                qDebug() << "[RGBD]Frames " << SaveFrameCnt << " are saved";
                SaveFrameCnt = 0;
            }
        }
    }
}

void rssavethread::save_RGBD_mat(cv::Mat &color_mat, cv::Mat &depth_mat, qint64 timestamp){
    if (!abort){


        Mat Buf_color_mat = color_mat.clone();
        Mat Buf_depth_mat = depth_mat.clone();
        char image_name[13];
        sprintf(image_name, "%13lld", timestamp);
        mutex.lock();
        mlColor.push_back(Buf_color_mat);
        mlDepth.push_back(Buf_depth_mat);
        mlImageName.push_back(image_name);

        /*
        if (mlColor.size() > CAPACITY){
            mlColor.pop_front();
        }
        if (mlDepth.size() > CAPACITY){
            mlDepth.pop_front();
        }
        if (mlImageName.size() > CAPACITY){
            mlImageName.pop_front();
        }
        */
        mutex.unlock();
        ReceiveFrameCnt++;

        //qDebug() << image_name << " size: " << mlImageName.size();
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
    mColor_path = QString("/home/hamlyn/data/Yao/Capture/RGBD/%1/%2/rgb/").arg(Subject).arg(Action+Index);
    mDepth_path = QString("/home/hamlyn/data/Yao/Capture/RGBD/%1/%2/depth/").arg(Subject).arg(Action+Index);
}
